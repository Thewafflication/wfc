#include "lower.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <utility>

namespace wfc::compiler {

namespace {

using ir::Check;
using ir::Condition;
using ir::Opcode;
using ir::Operand;
using ir::OperandKind;
using ir::Service;

struct LowerFailure final {
    std::string message;
    Position at;
};

[[noreturn]] void fail(const std::string& message, const Position at) {
    throw LowerFailure{message, at};
}

// A value and its VB type. A string value may be a temporary that this
// statement owns (it is released at the end of the statement).
struct Value final {
    Operand operand;
    VbType type{VbType::none};
};

struct Variable final {
    Operand operand;
    VbType type{VbType::none};  // the element type of an array
    bool is_array{};
    int udt{-1};  // the index of the user-defined type when type is record
};

// A field of a user-defined type and where it lies.
struct FieldInfo final {
    std::string name;
    VbType type{VbType::none};
    int udt{-1};
    std::int64_t offset{};
};

struct UdtInfo final {
    std::string name;
    std::vector<FieldInfo> fields;
    std::int64_t size{};  // a multiple of 8
    // Offsets of the String handles inside, nested records included.
    std::vector<std::int64_t> string_offsets;
};

// A local record with String fields: they are released when the procedure
// ends.
struct OwnedRecord final {
    Operand slot;
    int udt{-1};
};

// A handle held by a variable of the procedure: released when it ends.
struct Owned final {
    Operand slot;
    ir::Service release{ir::Service::string_release};
};

struct Signature final {
    int index{};
    std::vector<Parameter> parameters;
    VbType return_type{VbType::none};
    bool is_function{};
};

[[nodiscard]] bool is_integral(const VbType type) {
    return type == VbType::boolean || type == VbType::integer ||
           type == VbType::long_integer;
}

[[nodiscard]] bool is_float(const VbType type) {
    return type == VbType::single_precision || type == VbType::double_precision;
}

[[nodiscard]] bool is_numeric(const VbType type) {
    return is_integral(type) || is_float(type);
}

// Significant digits VB6 prints: 15 for a Double, 7 for a Single.
[[nodiscard]] int digits_of(const VbType type) {
    return type == VbType::single_precision ? 7 : 15;
}

// The bits of a double as an immediate operand.
[[nodiscard]] Operand double_immediate(const double value) {
    std::int64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return ir::immediate(bits);
}

[[nodiscard]] Check check_for(const VbType type) {
    return type == VbType::long_integer ? Check::int32 : Check::int16;
}

// The type a value has in arithmetic: a Boolean counts as an Integer.
[[nodiscard]] VbType arithmetic_type(const VbType type) {
    return type == VbType::boolean ? VbType::integer : type;
}

class FunctionBuilder final {
public:
    explicit FunctionBuilder(ir::Function& function) : function_(function) {}

    [[nodiscard]] Operand new_temp() {
        return ir::slot(function_.slot_count++);
    }
    [[nodiscard]] std::size_t position() const { return function_.code.size(); }
    // Where a pending run-time error goes after a call or a service.
    void set_error_label(const int label) {
        error_label_ = label;
        function_.error_label = label;
    }
    void check_error() {
        if (error_label_ < 0) {
            return;
        }
        ir::Instruction instruction;
        instruction.op = Opcode::check_error;
        instruction.target = error_label_;
        function_.code.push_back(std::move(instruction));
    }
    [[nodiscard]] static bool may_raise(const Service service) {
        switch (service) {
            case Service::string_left:
            case Service::string_right:
            case Service::string_mid:
            case Service::string_chr:
            case Service::string_asc:
            case Service::string_instr:
            case Service::string_space:
            case Service::double_to_long:
            case Service::math_function:
            case Service::double_power:
            case Service::array_redim:
            case Service::array_element1:
            case Service::array_element2:
            case Service::array_element3:
            case Service::array_bound:
            case Service::error_raise:
                return true;
            default:
                return false;
        }
    }
    // A slot that starts as the empty string or 0.0, set before the body.
    void insert_zero(const std::size_t index, const Operand slot) {
        ir::Instruction instruction;
        instruction.op = Opcode::move;
        instruction.dst = slot;
        instruction.a = ir::immediate(0);
        instruction.wide = true;
        function_.code.insert(
            function_.code.begin() + static_cast<std::ptrdiff_t>(index),
            std::move(instruction));
    }
    [[nodiscard]] int new_label() { return function_.label_count++; }
    void place(const int label) {
        ir::Instruction instruction;
        instruction.op = Opcode::label;
        instruction.target = label;
        function_.code.push_back(std::move(instruction));
    }
    void jump(const int label) {
        ir::Instruction instruction;
        instruction.op = Opcode::jump;
        instruction.target = label;
        function_.code.push_back(std::move(instruction));
    }
    void branch(const Opcode op, const Operand value, const int label) {
        ir::Instruction instruction;
        instruction.op = op;
        instruction.a = value;
        instruction.target = label;
        function_.code.push_back(std::move(instruction));
    }
    void emit(const Opcode op, const Operand dst, const Operand a,
              const Operand b = {}, const Check check = Check::none) {
        ir::Instruction instruction;
        instruction.op = op;
        instruction.dst = dst;
        instruction.a = a;
        instruction.b = b;
        instruction.check = check;
        function_.code.push_back(std::move(instruction));
    }
    void move_wide(const Operand dst, const Operand a) {
        ir::Instruction instruction;
        instruction.op = Opcode::move;
        instruction.dst = dst;
        instruction.a = a;
        instruction.wide = true;
        function_.code.push_back(std::move(instruction));
    }
    void compare(const Operand dst, const Operand a, const Operand b,
                 const Condition condition) {
        ir::Instruction instruction;
        instruction.op = Opcode::compare;
        instruction.dst = dst;
        instruction.a = a;
        instruction.b = b;
        instruction.condition = condition;
        function_.code.push_back(std::move(instruction));
    }
    void fcompare(const Operand dst, const Operand a, const Operand b,
                  const Condition condition) {
        ir::Instruction instruction;
        instruction.op = Opcode::fcompare;
        instruction.dst = dst;
        instruction.a = a;
        instruction.b = b;
        instruction.condition = condition;
        function_.code.push_back(std::move(instruction));
    }
    // A runtime service; `result` receives its value when given.
    void runtime(const Service service, std::vector<Operand> args,
                 const Operand result = {}, const bool wide = false,
                 const std::uint32_t double_args = 0,
                 const bool double_result = false) {
        ir::Instruction instruction;
        instruction.double_args = double_args;
        instruction.double_result = double_result;
        instruction.op = Opcode::runtime;
        instruction.target = static_cast<int>(service);
        instruction.args = std::move(args);
        instruction.dst = result;
        instruction.wide = wide;
        function_.code.push_back(std::move(instruction));
        if (may_raise(service)) {
            check_error();
        }
    }
    void print_text(const int string_index) {
        ir::Instruction instruction;
        instruction.op = Opcode::print_text;
        instruction.target = string_index;
        function_.code.push_back(std::move(instruction));
    }
    void call(const Operand dst, const int callee, std::vector<Operand> args,
              const bool wide, const std::uint32_t double_args = 0,
              const bool double_result = false) {
        ir::Instruction instruction;
        instruction.double_args = double_args;
        instruction.double_result = double_result;
        instruction.op = Opcode::call;
        instruction.dst = dst;
        instruction.target = callee;
        instruction.args = std::move(args);
        instruction.wide = wide;
        function_.code.push_back(std::move(instruction));
        check_error();
    }
    void ret() {
        ir::Instruction instruction;
        instruction.op = Opcode::ret;
        function_.code.push_back(std::move(instruction));
    }

private:
    ir::Function& function_;
    int error_label_{-1};
};

// A builtin function: its VB name, its arguments and the service behind it.
struct Builtin final {
    const char* name;
    int min_arguments;
    int max_arguments;
    Service service;
    VbType result;
    // The argument types: 's' string, 'n' number.
    const char* parameters;
};

constexpr Builtin kBuiltins[] = {
    {"len", 1, 1, Service::string_length, VbType::long_integer, "s"},
    {"left", 2, 2, Service::string_left, VbType::string, "sn"},
    {"right", 2, 2, Service::string_right, VbType::string, "sn"},
    {"mid", 2, 3, Service::string_mid, VbType::string, "snn"},
    {"ucase", 1, 1, Service::string_upper, VbType::string, "s"},
    {"lcase", 1, 1, Service::string_lower, VbType::string, "s"},
    {"trim", 1, 1, Service::string_trim, VbType::string, "s"},
    {"ltrim", 1, 1, Service::string_ltrim, VbType::string, "s"},
    {"rtrim", 1, 1, Service::string_rtrim, VbType::string, "s"},
    {"chr", 1, 1, Service::string_chr, VbType::string, "n"},
    {"asc", 1, 1, Service::string_asc, VbType::integer, "s"},
    {"space", 1, 1, Service::string_space, VbType::string, "n"},
};

class Lowerer final {
public:
    explicit Lowerer(const Module& module) : module_(module) {}

    ir::Program run() {
        declare_types();
        declare_constants(module_.constants, constants_);
        declare_globals();
        declare_procedures();
        for (const auto& procedure : module_.procedures) {
            lower_procedure(procedure);
        }
        const auto main = signatures_.find("main");
        if (main == signatures_.end() || main->second.is_function) {
            fail("the project has no Sub Main", Position{1, 1});
        }
        program_.entry = main->second.index;
        if (!global_arrays_.empty()) {
            make_start_function(main->second.index);
        }
        return std::move(program_);
    }

private:
    // ----- Declarations ----------------------------------------------------

    static void require_type(const Declaration& declaration) {
        if (!declaration.type_given || declaration.type == VbType::none) {
            fail("'" + declaration.name +
                     "' needs an explicit type (Variant is not compiled yet)",
                 declaration.at);
        }
    }

    struct GlobalArray final {
        Variable variable;
        const Declaration* declaration;
    };

    // `__start`: allocates the module-level arrays, then runs Sub Main.
    void make_start_function(const int main_index) {
        ir::Function function;
        function.name = "__start";
        program_.functions.push_back(std::move(function));
        auto& start = program_.functions.back();
        FunctionBuilder builder(start);
        builder_ = &builder;
        const int finish = builder.new_label();
        builder.set_error_label(finish);
        for (const auto& array : global_arrays_) {
            emit_array_allocation(array.variable, array.declaration->bounds,
                                  true, false, true);
        }
        builder.call({}, main_index, {}, false);
        builder.place(finish);
        builder.ret();
        builder_ = nullptr;
        program_.entry = static_cast<int>(program_.functions.size()) - 1;
    }

    // ----- User-defined types ----------------------------------------------

    [[nodiscard]] int find_udt(const std::string& name,
                               const Position at) const {
        const auto found = udt_index_.find(name);
        if (found == udt_index_.end()) {
            fail("the type '" + name + "' is not declared", at);
        }
        return found->second;
    }

    void declare_types() {
        for (const auto& declaration : module_.types) {
            if (udt_index_.contains(declaration.name)) {
                fail("the type '" + declaration.name + "' is declared twice",
                     declaration.at);
            }
            UdtInfo info;
            info.name = declaration.name;
            std::int64_t offset = 0;
            for (const auto& field : declaration.fields) {
                FieldInfo entry;
                entry.name = field.name;
                entry.type = field.type;
                std::int64_t size = 4;
                if (field.type == VbType::record) {
                    entry.udt = find_udt(field.type_name, field.at);
                    size = udts_[static_cast<std::size_t>(entry.udt)].size;
                } else if (field.type == VbType::string ||
                           is_float(field.type)) {
                    size = 8;
                }
                const std::int64_t align = size >= 8 ? 8 : 4;
                offset = (offset + align - 1) / align * align;
                entry.offset = offset;
                if (field.type == VbType::string) {
                    info.string_offsets.push_back(offset);
                } else if (entry.udt >= 0) {
                    for (const auto inner :
                         udts_[static_cast<std::size_t>(entry.udt)]
                             .string_offsets) {
                        info.string_offsets.push_back(offset + inner);
                    }
                }
                offset += size;
                info.fields.push_back(std::move(entry));
            }
            info.size = std::max<std::int64_t>(8, (offset + 7) / 8 * 8);
            udt_index_[declaration.name] = static_cast<int>(udts_.size());
            udts_.push_back(std::move(info));
        }
    }

    // The leaf fields of a record (nested records flattened): offset, type.
    void leaves_of(const int udt, const std::int64_t base,
                   std::vector<std::pair<std::int64_t, VbType>>& into) const {
        for (const auto& field : udts_[static_cast<std::size_t>(udt)].fields) {
            if (field.type == VbType::record) {
                leaves_of(field.udt, base + field.offset, into);
            } else {
                into.emplace_back(base + field.offset, field.type);
            }
        }
    }

    // A slot holding the address of the record `variable` stands for.
    [[nodiscard]] Operand struct_pointer(const Variable& variable) {
        if (variable.operand.kind == OperandKind::indirect) {
            return ir::slot(variable.operand.value);
        }
        const auto pointer = builder_->new_temp();
        builder_->emit(Opcode::address, pointer, variable.operand);
        return pointer;
    }

    [[nodiscard]] Operand pointer_plus(const Operand& pointer,
                                       const std::int64_t offset) {
        if (offset == 0) {
            return pointer;
        }
        const auto moved = builder_->new_temp();
        builder_->emit(Opcode::pointer_add, moved, pointer,
                       ir::immediate(offset));
        return moved;
    }

    // The place an expression stands for: a variable, an array element or a
    // field of a record.
    [[nodiscard]] Variable lower_place(const Expr& expression) {
        switch (expression.kind) {
            case ExprKind::name:
                return find_variable(expression.text, expression.at);
            case ExprKind::call: {
                const auto array = find_array(expression.text);
                if (!array.has_value()) {
                    fail("'" + expression.text + "' is not an array",
                         expression.at);
                }
                return Variable{
                    element_address(*array, expression.operands, expression.at),
                    array->type, false, array->udt};
            }
            case ExprKind::member: {
                const auto base = lower_place(*expression.operands[0]);
                if (base.type != VbType::record || base.is_array) {
                    fail("'." + expression.text + "' needs a record before it",
                         expression.at);
                }
                const auto& info = udts_[static_cast<std::size_t>(base.udt)];
                for (const auto& field : info.fields) {
                    if (field.name == expression.text) {
                        const auto where =
                            pointer_plus(struct_pointer(base), field.offset);
                        const Operand place = where.kind == OperandKind::slot
                                                  ? ir::indirect(where.value)
                                                  : where;
                        return Variable{place, field.type, false, field.udt};
                    }
                }
                fail("the type '" + info.name + "' has no field '" +
                         expression.text + "'",
                     expression.at);
            }
            default:
                fail("this cannot be assigned to", expression.at);
        }
    }

    // `destination = source` for records of one type.
    void copy_record(const Variable& destination, const Variable& source,
                     const Position at) {
        if (source.type != VbType::record || source.udt != destination.udt ||
            source.is_array || destination.is_array) {
            fail("a record can only be assigned a record of its own type", at);
        }
        std::vector<std::pair<std::int64_t, VbType>> leaves;
        leaves_of(destination.udt, 0, leaves);
        const auto to = struct_pointer(destination);
        const auto from = struct_pointer(source);
        for (const auto& [offset, type] : leaves) {
            const auto target = ir::indirect(pointer_plus(to, offset).value);
            const auto origin = ir::indirect(pointer_plus(from, offset).value);
            if (type == VbType::string) {
                const auto handle = builder_->new_temp();
                builder_->move_wide(handle, origin);
                builder_->runtime(Service::string_assign,
                                  {ir::slot(target.value), handle});
            } else if (is_float(type)) {
                builder_->move_wide(target, origin);
            } else {
                builder_->emit(Opcode::move, target, origin);
            }
        }
    }

    // Releases the Strings inside a local record when its procedure ends.
    void release_record(const OwnedRecord& record) {
        const auto base = struct_pointer(Variable{record.slot, VbType::record});
        for (const auto offset :
             udts_[static_cast<std::size_t>(record.udt)].string_offsets) {
            const auto field = pointer_plus(base, offset);
            const auto handle = builder_->new_temp();
            builder_->move_wide(handle, ir::indirect(field.value));
            builder_->runtime(Service::string_release, {handle});
        }
    }

    void declare_globals() {
        for (const auto& declaration : module_.globals) {
            require_type(declaration);
            if (globals_.contains(declaration.name)) {
                fail("'" + declaration.name + "' is already declared",
                     declaration.at);
            }
            int record = -1;
            std::int64_t slots = 1;
            if (declaration.type == VbType::record) {
                record = find_udt(declaration.type_name, declaration.at);
                if (!declaration.is_array) {
                    slots = udts_[static_cast<std::size_t>(record)].size / 8;
                }
            }
            const auto variable = Variable{
                ir::global(static_cast<std::int64_t>(program_.globals.size())),
                declaration.type, declaration.is_array, record};
            globals_[declaration.name] = variable;
            for (std::int64_t i = 0; i < slots; ++i) {
                program_.globals.push_back(ir::Global{declaration.name});
            }
            if (declaration.is_array && !declaration.bounds.empty()) {
                global_arrays_.push_back(GlobalArray{variable, &declaration});
            }
        }
    }

    void declare_procedures() {
        for (const auto& procedure : module_.procedures) {
            if (signatures_.contains(procedure.name) ||
                globals_.contains(procedure.name)) {
                fail("'" + procedure.spelling + "' is already declared",
                     procedure.at);
            }
            if (procedure.is_function &&
                procedure.return_type == VbType::none) {
                fail("the Function " + procedure.spelling +
                         " needs an explicit return type (Variant is not "
                         "compiled yet)",
                     procedure.at);
            }
            if (procedure.return_type == VbType::record) {
                fail("a Function cannot return a record", procedure.at);
            }
            for (const auto& parameter : procedure.parameters) {
                if (parameter.type == VbType::none) {
                    fail("the parameter '" + parameter.name +
                             "' needs an explicit type (Variant is not "
                             "compiled yet)",
                         procedure.at);
                }
                if (parameter.type == VbType::record) {
                    if (parameter.by_value) {
                        fail("a record parameter must be passed ByRef",
                             procedure.at);
                    }
                    static_cast<void>(
                        find_udt(parameter.type_name, procedure.at));
                }
            }
            Signature signature;
            signature.index = static_cast<int>(signatures_.size());
            signature.parameters = procedure.parameters;
            signature.return_type = procedure.return_type;
            signature.is_function = procedure.is_function;
            signatures_[procedure.name] = signature;
            ir::Function function;
            function.name = procedure.name;
            for (const auto& parameter : procedure.parameters) {
                function.parameters.push_back(ir::Parameter{
                    parameter.by_value,
                    !parameter.by_value || parameter.type == VbType::string,
                    parameter.by_value && is_float(parameter.type)});
            }
            program_.functions.push_back(std::move(function));
        }
    }

    // ----- Procedures ------------------------------------------------------

    struct Loop final {
        std::string kind;  // "for" or "do"
        int exit_label{};
    };

    // Every Dim of a procedure belongs to the procedure's scope: it is
    // declared (and zeroed) when the procedure starts, as in VB6.
    void declare_locals_of(const std::vector<StmtPtr>& statements) {
        for (const auto& statement : statements) {
            if (statement->kind == StmtKind::dim) {
                for (const auto& declaration : statement->declarations) {
                    declare_local(declaration);
                }
            }
            for (const auto& block : statement->blocks) {
                declare_locals_of(block);
            }
            declare_locals_of(statement->body);
        }
    }

    void lower_procedure(const Procedure& procedure) {
        const auto& signature = signatures_.at(procedure.name);
        auto& function =
            program_.functions[static_cast<std::size_t>(signature.index)];
        FunctionBuilder builder(function);
        builder_ = &builder;
        locals_.clear();
        loops_.clear();
        owned_slots_.clear();
        owned_records_.clear();
        pending_.clear();
        current_ = &procedure;
        result_variable_.reset();
        // Parameters occupy the first slots.
        for (const auto& parameter : procedure.parameters) {
            if (locals_.contains(parameter.name)) {
                fail("the parameter '" + parameter.name + "' is declared twice",
                     procedure.at);
            }
            const auto index = function.slot_count++;
            locals_[parameter.name] = Variable{
                parameter.by_value ? ir::slot(index) : ir::indirect(index),
                parameter.type, parameter.is_array,
                parameter.type == VbType::record
                    ? find_udt(parameter.type_name, procedure.at)
                    : -1};
            if (parameter.by_value && parameter.type == VbType::string) {
                // The callee owns its copy of a string parameter.
                builder.runtime(Service::string_addref, {ir::slot(index)});
                owned_slots_.push_back(
                    Owned{ir::slot(index), Service::string_release});
            }
        }
        if (procedure.is_function) {
            const auto result = builder.new_temp();
            function.result_slot = static_cast<int>(result.value);
            function.result_wide = procedure.return_type == VbType::string;
            function.result_double = is_float(procedure.return_type);
            zero(result, procedure.return_type);
            result_variable_ = Variable{result, procedure.return_type};
        }
        end_label_ = builder.new_label();
        builder.set_error_label(end_label_);
        error_label_ = end_label_;
        entry_index_ = builder.position();
        late_zero_.clear();
        local_constants_.clear();
        labels_.clear();
        defined_labels_.clear();
        setup_error_handling(procedure, builder);
        declare_locals_of(procedure.body);
        lower_block(procedure.body);
        if (uses_errors_) {
            builder.jump(end_label_);
            emit_error_dispatch();
        }
        for (const auto& [name, id] : labels_) {
            static_cast<void>(id);
            if (!defined_labels_.contains(name)) {
                fail("the label '" + name + "' is not defined", procedure.at);
            }
        }
        // Hidden slots that hold strings across statements start empty.
        for (std::size_t i = 0; i < late_zero_.size(); ++i) {
            builder.insert_zero(entry_index_ + i, late_zero_[i]);
        }
        builder.place(end_label_);
        for (const auto& owned : owned_slots_) {
            builder.runtime(owned.release, {owned.slot});
        }
        for (const auto& record : owned_records_) {
            release_record(record);
        }
        builder.ret();
        builder_ = nullptr;
    }

    // ----- Error handling --------------------------------------------------
    //
    // A procedure with `On Error` keeps its handler (0 none, 1 Resume Next,
    // 2 and up the GoTo handlers) and the number of the running statement in
    // slots. Every pending error goes to one dispatch, which acts on them.

    static bool contains_on_error(const std::vector<StmtPtr>& statements) {
        for (const auto& statement : statements) {
            if (statement->kind == StmtKind::on_error) {
                return true;
            }
            for (const auto& block : statement->blocks) {
                if (contains_on_error(block)) {
                    return true;
                }
            }
            for (const auto& block : statement->cases) {
                if (contains_on_error(block.body)) {
                    return true;
                }
            }
            if (contains_on_error(statement->body)) {
                return true;
            }
        }
        return false;
    }

    void setup_error_handling(const Procedure& procedure,
                              FunctionBuilder& builder) {
        uses_errors_ = contains_on_error(procedure.body);
        start_labels_.clear();
        end_labels_.clear();
        handler_labels_.clear();
        current_id_ = -1;
        if (!uses_errors_) {
            return;
        }
        handler_slot_ = builder.new_temp();
        saved_handler_slot_ = builder.new_temp();
        stmt_id_slot_ = builder.new_temp();
        error_stmt_slot_ = builder.new_temp();
        for (const auto& slot : {handler_slot_, saved_handler_slot_,
                                 stmt_id_slot_, error_stmt_slot_}) {
            builder.emit(Opcode::move, slot, ir::immediate(0));
        }
        dispatch_label_ = builder.new_label();
        resume_next_label_ = builder.new_label();
        resume_retry_label_ = builder.new_label();
        builder.set_error_label(dispatch_label_);
        error_label_ = dispatch_label_;
    }

    // Marks the statement that is running (errors resume relative to it).
    void touch_statement(const int id) {
        if (uses_errors_ && id >= 0) {
            builder_->emit(Opcode::move, stmt_id_slot_, ir::immediate(id));
        }
    }

    // Jumps to `labels[k]` when the running statement is k.
    void dispatch_on_statement(const std::vector<int>& labels) {
        for (std::size_t k = 0; k < labels.size(); ++k) {
            const auto same = builder_->new_temp();
            builder_->compare(same, error_stmt_slot_,
                              ir::immediate(static_cast<std::int64_t>(k)),
                              Condition::equal);
            builder_->branch(Opcode::branch_nonzero, same, labels[k]);
        }
        builder_->jump(end_label_);
    }

    void emit_error_dispatch() {
        builder_->place(dispatch_label_);
        // The statement that failed: a Resume goes back to or past it.
        builder_->emit(Opcode::move, error_stmt_slot_, stmt_id_slot_);
        const auto none = builder_->new_temp();
        builder_->compare(none, handler_slot_, ir::immediate(0),
                          Condition::equal);
        // No handler: the error stays pending and leaves the procedure.
        builder_->branch(Opcode::branch_nonzero, none, end_label_);
        builder_->runtime(Service::error_acknowledge, {});
        const auto resume_next = builder_->new_temp();
        builder_->compare(resume_next, handler_slot_, ir::immediate(1),
                          Condition::equal);
        builder_->branch(Opcode::branch_nonzero, resume_next,
                         resume_next_label_);
        // A GoTo handler runs with the handler off until a Resume.
        const auto which = builder_->new_temp();
        builder_->emit(Opcode::move, which, handler_slot_);
        builder_->emit(Opcode::move, handler_slot_, ir::immediate(0));
        for (std::size_t i = 0; i < handler_labels_.size(); ++i) {
            const auto matches = builder_->new_temp();
            builder_->compare(matches, which,
                              ir::immediate(static_cast<std::int64_t>(i + 2U)),
                              Condition::equal);
            builder_->branch(Opcode::branch_nonzero, matches,
                             handler_labels_[i]);
        }
        builder_->jump(end_label_);
        builder_->place(resume_next_label_);
        dispatch_on_statement(end_labels_);
        builder_->place(resume_retry_label_);
        dispatch_on_statement(start_labels_);
    }

    void lower_on_error(const Stmt& statement) {
        std::int64_t handler = 0;
        if (statement.text == "resume next") {
            handler = 1;
        } else if (statement.text != "0") {
            const int target = label_for(statement.text);
            std::size_t index = 0;
            while (index < handler_labels_.size() &&
                   handler_labels_[index] != target) {
                ++index;
            }
            if (index == handler_labels_.size()) {
                handler_labels_.push_back(target);
            }
            handler = static_cast<std::int64_t>(index) + 2;
        }
        builder_->emit(Opcode::move, handler_slot_, ir::immediate(handler));
        if (handler != 0) {
            builder_->emit(Opcode::move, saved_handler_slot_,
                           ir::immediate(handler));
        }
    }

    void lower_resume(const Stmt& statement) {
        // Resume switches the handler on again.
        builder_->emit(Opcode::move, handler_slot_, saved_handler_slot_);
        if (statement.text.empty()) {
            builder_->jump(resume_retry_label_);
        } else if (statement.text == "next") {
            builder_->jump(resume_next_label_);
        } else {
            builder_->jump(label_for(statement.text));
        }
    }

    // `Err.Number`, `Err.Description`, `Err.Source`.
    [[nodiscard]] Value lower_err_member(const Expr& expression) {
        const std::string member = expression.text.substr(4);
        Value result;
        if (member == "number") {
            result.type = VbType::long_integer;
            result.operand = builder_->new_temp();
            builder_->runtime(Service::error_number, {}, result.operand);
            return result;
        }
        if (member == "description" || member == "source") {
            result.type = VbType::string;
            result.operand = new_owned_string();
            builder_->runtime(member == "description"
                                  ? Service::error_description
                                  : Service::error_source,
                              {}, result.operand, true);
            return result;
        }
        fail("Err." + member + " is not compiled yet", expression.at);
    }

    // `Err.Clear` and `Err.Raise number [, source [, description]]`.
    void lower_err_statement(const Stmt& statement) {
        if (statement.text == "err.clear") {
            builder_->runtime(Service::error_clear, {});
            return;
        }
        if (statement.text != "err.raise" || statement.items.empty() ||
            statement.items.size() > 3U) {
            fail(statement.text + " is not compiled yet", statement.at);
        }
        std::vector<Operand> arguments;
        const auto number = lower_expression(*statement.items[0]);
        arguments.push_back(to_long(number, statement.at).operand);
        for (std::size_t i = 1; i < 3U; ++i) {
            if (i < statement.items.size()) {
                const auto text = lower_expression(*statement.items[i]);
                if (text.type != VbType::string) {
                    fail("Err.Raise needs String arguments here",
                         statement.items[i]->at);
                }
                arguments.push_back(text.operand);
            } else {
                arguments.push_back(ir::immediate(0));
            }
        }
        builder_->runtime(Service::error_raise, std::move(arguments));
    }

    void zero(const Operand slot, const VbType type) {
        if (type == VbType::string || is_float(type)) {
            builder_->move_wide(slot, ir::immediate(0));
        } else {
            builder_->emit(Opcode::move, slot, ir::immediate(0));
        }
    }

    // A Static variable keeps its value between calls: a cell of the globals
    // area that only this procedure names.
    void declare_static(const Declaration& declaration) {
        if (declaration.is_array && !declaration.bounds.empty()) {
            fail("a fixed-size Static array is not compiled yet",
                 declaration.at);
        }
        int record = -1;
        std::int64_t cells = 1;
        if (declaration.type == VbType::record) {
            record = find_udt(declaration.type_name, declaration.at);
            if (!declaration.is_array) {
                cells = udts_[static_cast<std::size_t>(record)].size / 8;
            }
        }
        locals_[declaration.name] = Variable{
            ir::global(static_cast<std::int64_t>(program_.globals.size())),
            declaration.type, declaration.is_array, record};
        for (std::int64_t i = 0; i < cells; ++i) {
            program_.globals.push_back(ir::Global{declaration.name});
        }
    }

    void declare_local(const Declaration& declaration) {
        require_type(declaration);
        if (locals_.contains(declaration.name) ||
            (result_variable_.has_value() &&
             declaration.name == current_->name)) {
            fail("'" + declaration.name + "' is already declared",
                 declaration.at);
        }
        if (declaration.is_static) {
            declare_static(declaration);
            return;
        }
        if (declaration.type == VbType::record && declaration.is_array) {
            // An array of records: a handle like any array.
            const int record = find_udt(declaration.type_name, declaration.at);
            const auto handle = builder_->new_temp();
            builder_->move_wide(handle, ir::immediate(0));
            const auto variable =
                Variable{handle, VbType::record, true, record};
            locals_[declaration.name] = variable;
            owned_slots_.push_back(Owned{handle, Service::array_release});
            if (!declaration.bounds.empty()) {
                emit_array_allocation(variable, declaration.bounds, true, false,
                                      true);
            }
            return;
        }
        if (declaration.type == VbType::record) {
            const int record = find_udt(declaration.type_name, declaration.at);
            const auto slots = udts_[static_cast<std::size_t>(record)].size / 8;
            // The record lives in consecutive slots; its first byte is in the
            // last one (memory runs up while slot numbers run down).
            Operand last;
            for (std::int64_t i = 0; i < slots; ++i) {
                last = builder_->new_temp();
                builder_->move_wide(last, ir::immediate(0));
            }
            locals_[declaration.name] =
                Variable{last, VbType::record, false, record};
            if (!udts_[static_cast<std::size_t>(record)]
                     .string_offsets.empty()) {
                owned_records_.push_back(OwnedRecord{last, record});
            }
            return;
        }
        const auto slot = builder_->new_temp();
        if (declaration.is_array) {
            // An array is a handle; a fixed-size one is allocated now.
            builder_->move_wide(slot, ir::immediate(0));
            const auto variable = Variable{slot, declaration.type, true};
            locals_[declaration.name] = variable;
            owned_slots_.push_back(Owned{slot, Service::array_release});
            if (!declaration.bounds.empty()) {
                emit_array_allocation(variable, declaration.bounds, true, false,
                                      true);
            }
            return;
        }
        zero(slot, declaration.type);
        locals_[declaration.name] = Variable{slot, declaration.type};
        if (declaration.type == VbType::string) {
            owned_slots_.push_back(Owned{slot, Service::string_release});
        }
    }

    [[nodiscard]] static int element_kind(const VbType type) {
        return type == VbType::record
                   ? 3
                   : (type == VbType::string ? 2 : (is_float(type) ? 1 : 0));
    }

    // The String offsets of a record type as a read-only blob of the image: a
    // count and the offsets (int32); it is stored like a String literal.
    [[nodiscard]] int descriptor_of(const int udt) {
        const auto found = descriptors_.find(udt);
        if (found != descriptors_.end()) {
            return found->second;
        }
        const auto& offsets =
            udts_[static_cast<std::size_t>(udt)].string_offsets;
        std::string blob;
        const auto put = [&blob](const std::int32_t value) {
            blob.append(reinterpret_cast<const char*>(&value), sizeof(value));
        };
        put(static_cast<std::int32_t>(offsets.size()));
        for (const auto offset : offsets) {
            put(static_cast<std::int32_t>(offset));
        }
        program_.strings.push_back(std::move(blob));
        const int index = static_cast<int>(program_.strings.size() - 1U);
        descriptors_[udt] = index;
        return index;
    }

    // Dimensions an array: the bounds are constants (`constant`) or
    // expressions evaluated now (ReDim).
    void emit_array_allocation(const Variable& variable,
                               const std::vector<ArrayBound>& bounds,
                               const bool fixed, const bool preserve,
                               const bool constant) {
        const auto dims = static_cast<int>(bounds.size());
        if (dims < 1 || dims > 3) {
            fail("arrays of one to three dimensions are compiled so far",
                 Position{});
        }
        // The bounds go to consecutive slots; the service reads them upwards
        // in memory, which is downwards in slot numbers.
        const auto count = 2 * dims;
        std::vector<Operand> slots;
        for (int i = 0; i < count; ++i) {
            slots.push_back(builder_->new_temp());
        }
        const auto place = [&](const int index) {
            return slots[static_cast<std::size_t>(count - 1 - index)];
        };
        for (int k = 0; k < dims; ++k) {
            const auto& bound = bounds[static_cast<std::size_t>(k)];
            for (int side = 0; side < 2; ++side) {
                const Expr* expression =
                    side == 0 ? bound.low.get() : bound.high.get();
                Operand value =
                    ir::immediate(side == 0 ? module_.option_base : 0);
                if (expression != nullptr) {
                    if (constant) {
                        const auto folded = fold(*expression);
                        value = fold_convert(folded, VbType::long_integer,
                                             expression->at)
                                    .operand;
                    } else {
                        value = to_long(lower_expression(*expression),
                                        expression->at)
                                    .operand;
                    }
                }
                builder_->emit(Opcode::move, place(2 * k + side), value);
            }
        }
        const auto block = builder_->new_temp();
        builder_->emit(Opcode::address, block, place(0));
        Operand record_size = ir::immediate(0);
        Operand record_strings = ir::immediate(0);
        if (variable.type == VbType::record) {
            record_size = ir::immediate(
                udts_[static_cast<std::size_t>(variable.udt)].size);
            // The bytes of the descriptor start after the 8-byte string header.
            const auto handle = builder_->new_temp();
            builder_->move_wide(handle,
                                ir::literal(descriptor_of(variable.udt)));
            record_strings = pointer_plus(handle, 8);
        }
        builder_->runtime(
            Service::array_redim,
            {address_of(variable.operand),
             ir::immediate(element_kind(variable.type) | (fixed ? 0x100 : 0)),
             ir::immediate(dims), block, ir::immediate(preserve ? 1 : 0),
             record_size, record_strings});
    }

    [[nodiscard]] Variable find_variable(const std::string& name,
                                         const Position at) {
        if (const auto found = locals_.find(name); found != locals_.end()) {
            return found->second;
        }
        if (result_variable_.has_value() && name == current_->name) {
            return *result_variable_;
        }
        if (const auto found = globals_.find(name); found != globals_.end()) {
            return found->second;
        }
        fail("'" + name + "' is not declared", at);
    }

    // A string temporary owned by the current statement.
    [[nodiscard]] Operand new_owned_string() {
        const auto temp = builder_->new_temp();
        pending_.push_back(temp);
        return temp;
    }

    // Releases the string temporaries of the statement that just ended.
    void flush_temps() {
        for (const auto& temp : pending_) {
            builder_->runtime(Service::string_release, {temp});
        }
        pending_.clear();
    }

    // ----- Statements ------------------------------------------------------

    void lower_block(const std::vector<StmtPtr>& statements) {
        for (const auto& statement : statements) {
            lower_statement(*statement);
        }
    }

    void lower_statement(const Stmt& statement) {
        int id = -1;
        if (uses_errors_) {
            id = static_cast<int>(start_labels_.size());
            start_labels_.push_back(builder_->new_label());
            end_labels_.push_back(builder_->new_label());
            builder_->place(start_labels_.back());
            current_id_ = id;
            touch_statement(id);
        }
        lower_statement_body(statement);
        flush_temps();
        if (id >= 0) {
            builder_->place(end_labels_[static_cast<std::size_t>(id)]);
        }
    }

    void lower_statement_body(const Stmt& statement) {
        switch (statement.kind) {
            case StmtKind::dim:
                break;  // declared when the procedure starts
            case StmtKind::assign:
                lower_assignment(statement);
                break;
            case StmtKind::print:
                lower_print(statement);
                break;
            case StmtKind::if_block:
                lower_if(statement);
                break;
            case StmtKind::for_loop:
                lower_for(statement);
                break;
            case StmtKind::do_loop:
                lower_do(statement);
                break;
            case StmtKind::while_loop:
                lower_while(statement);
                break;
            case StmtKind::call:
                lower_call_statement(statement);
                break;
            case StmtKind::exit:
                lower_exit(statement);
                break;
            case StmtKind::select_case:
                lower_select(statement);
                break;
            case StmtKind::const_decl:
                declare_constants(statement.constants, local_constants_);
                break;
            case StmtKind::label: {
                if (!defined_labels_.insert(statement.text).second) {
                    fail("the label '" + statement.text + "' is defined twice",
                         statement.at);
                }
                builder_->place(label_for(statement.text));
                break;
            }
            case StmtKind::go_to:
                builder_->jump(label_for(statement.text));
                break;
            case StmtKind::on_error:
                lower_on_error(statement);
                break;
            case StmtKind::resume:
                lower_resume(statement);
                break;
            case StmtKind::redim:
                lower_redim(statement);
                break;
            case StmtKind::erase:
                for (const auto& item : statement.items) {
                    const auto variable = find_variable(item->text, item->at);
                    if (!variable.is_array) {
                        fail("'" + item->text + "' is not an array", item->at);
                    }
                    builder_->runtime(Service::array_erase,
                                      {address_of(variable.operand)});
                }
                break;
        }
    }

    void lower_assignment(const Stmt& statement) {
        if (!locals_.contains(statement.text) &&
            !globals_.contains(statement.text) &&
            (local_constants_.contains(statement.text) ||
             constants_.contains(statement.text))) {
            fail("'" + statement.text + "' is a constant", statement.at);
        }
        if (statement.target) {
            // `a.b.c = expr`
            lower_place_assignment(statement);
            return;
        }
        auto target = find_variable(statement.text, statement.at);
        if (target.type == VbType::record) {
            const auto source = lower_place(*statement.expr);
            if (!statement.items.empty()) {
                if (!target.is_array) {
                    fail("'" + statement.text + "' is not an array",
                         statement.at);
                }
                target = Variable{
                    element_address(target, statement.items, statement.at),
                    VbType::record, false, target.udt};
            } else if (target.is_array) {
                fail("the array '" + statement.text + "' needs an index",
                     statement.at);
            }
            copy_record(target, source, statement.at);
            return;
        }
        const auto value = lower_expression(*statement.expr);
        if (!statement.items.empty()) {
            if (!target.is_array) {
                fail("'" + statement.text + "' is not an array", statement.at);
            }
            target =
                Variable{element_address(target, statement.items, statement.at),
                         target.type, false, target.udt};
            if (target.type == VbType::record) {
                copy_record(target, lower_place(*statement.expr), statement.at);
                return;
            }
        } else if (target.is_array) {
            fail("the array '" + statement.text + "' needs an index",
                 statement.at);
        }
        if (target.type == VbType::string) {
            if (value.type != VbType::string) {
                fail("a number cannot be assigned to a String variable yet",
                     statement.at);
            }
            builder_->runtime(Service::string_assign,
                              {address_of(target.operand), value.operand});
            return;
        }
        const auto converted = convert(value, target.type, statement.at);
        if (is_float(target.type)) {
            builder_->move_wide(target.operand, converted.operand);
        } else {
            builder_->emit(Opcode::move, target.operand, converted.operand);
        }
    }

    void lower_place_assignment(const Stmt& statement) {
        // The value is evaluated first: it may change what the place is.
        const auto probe = statement.expr.get();
        const auto destination_type = place_type_of(*statement.target);
        if (destination_type == VbType::record) {
            const auto source = lower_place(*probe);
            const auto destination = lower_place(*statement.target);
            copy_record(destination, source, statement.at);
            return;
        }
        const auto value = lower_expression(*probe);
        const auto target = lower_place(*statement.target);
        if (target.type == VbType::string) {
            if (value.type != VbType::string) {
                fail("a number cannot be assigned to a String field yet",
                     statement.at);
            }
            builder_->runtime(Service::string_assign,
                              {address_of(target.operand), value.operand});
            return;
        }
        const auto converted = convert(value, target.type, statement.at);
        if (is_float(target.type)) {
            builder_->move_wide(target.operand, converted.operand);
        } else {
            builder_->emit(Opcode::move, target.operand, converted.operand);
        }
    }

    // The type of a place without generating code for it.
    [[nodiscard]] VbType place_type_of(const Expr& expression) {
        switch (expression.kind) {
            case ExprKind::name:
                return find_variable(expression.text, expression.at).type;
            case ExprKind::call: {
                const auto array = find_array(expression.text);
                return array.has_value() ? array->type : VbType::none;
            }
            case ExprKind::member: {
                int udt = -1;
                const auto base = base_udt_of(*expression.operands[0], udt);
                if (!base || udt < 0) {
                    return VbType::none;
                }
                for (const auto& field :
                     udts_[static_cast<std::size_t>(udt)].fields) {
                    if (field.name == expression.text) {
                        return field.type;
                    }
                }
                return VbType::none;
            }
            default:
                return VbType::none;
        }
    }

    // The record type a place of record type has (no code is generated).
    [[nodiscard]] bool base_udt_of(const Expr& expression, int& udt) {
        switch (expression.kind) {
            case ExprKind::name:
                udt = find_variable(expression.text, expression.at).udt;
                return true;
            case ExprKind::call: {
                const auto array = find_array(expression.text);
                udt = array.has_value() ? array->udt : -1;
                return true;
            }
            case ExprKind::member: {
                int outer = -1;
                if (!base_udt_of(*expression.operands[0], outer) || outer < 0) {
                    return false;
                }
                for (const auto& field :
                     udts_[static_cast<std::size_t>(outer)].fields) {
                    if (field.name == expression.text) {
                        udt = field.udt;
                        return true;
                    }
                }
                return false;
            }
            default:
                return false;
        }
    }

    // The address of a variable (a slot, a global, or a ByRef parameter).
    [[nodiscard]] Operand address_of(const Operand& variable) {
        if (variable.kind == OperandKind::indirect) {
            return ir::slot(variable.value);  // already a pointer
        }
        const auto pointer = builder_->new_temp();
        builder_->emit(Opcode::address, pointer, variable);
        return pointer;
    }

    // A Print evaluates all its items first and then writes them, so a
    // function called by a later item cannot change what an earlier one shows.
    void lower_print(const Stmt& statement) {
        struct Item final {
            int literal{-1};
            bool empty{};
            Value value;
        };
        std::vector<Item> items;
        for (const auto& expression : statement.items) {
            Item item;
            if (!expression) {
                item.empty = true;
            } else if (expression->kind == ExprKind::string_literal) {
                program_.strings.push_back(expression->text);
                item.literal = static_cast<int>(program_.strings.size() - 1U);
            } else {
                item.value = snapshot(lower_expression(*expression));
            }
            items.push_back(item);
        }
        for (std::size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            if (item.empty) {
                // nothing to write before the separator
            } else if (item.literal >= 0) {
                builder_->print_text(item.literal);
            } else if (is_float(item.value.type)) {
                builder_->runtime(Service::print_double,
                                  {item.value.operand,
                                   ir::immediate(digits_of(item.value.type))},
                                  {}, false, 1U);
            } else {
                const Service service =
                    item.value.type == VbType::boolean  ? Service::print_boolean
                    : item.value.type == VbType::string ? Service::print_string
                                                        : Service::print_number;
                builder_->runtime(service, {item.value.operand});
            }
            if (i < statement.separators.size() &&
                statement.separators[i] == ',') {
                builder_->runtime(Service::print_zone, {});
            }
        }
        if (statement.newline) {
            builder_->runtime(Service::print_newline, {});
        }
    }

    // A copy of a variable's value as it is now (a String keeps a reference).
    [[nodiscard]] Value snapshot(const Value& value) {
        const auto kind = value.operand.kind;
        const bool is_variable = kind == OperandKind::slot ||
                                 kind == OperandKind::global ||
                                 kind == OperandKind::indirect;
        if (!is_variable) {
            return value;
        }
        const auto copy = builder_->new_temp();
        if (value.type == VbType::string) {
            builder_->move_wide(copy, value.operand);
            builder_->runtime(Service::string_addref, {copy});
            pending_.push_back(copy);
        } else if (is_float(value.type)) {
            builder_->move_wide(copy, value.operand);
        } else {
            builder_->emit(Opcode::move, copy, value.operand);
        }
        return Value{copy, value.type};
    }

    // A condition is true when its value is not zero. The temporaries of
    // its evaluation are released before the branch.
    [[nodiscard]] Operand lower_condition(const Expr& expression) {
        auto value = lower_expression(expression);
        if (!is_numeric(value.type)) {
            fail("a condition needs a numeric or Boolean value", expression.at);
        }
        if (is_float(value.type)) {
            value = convert(value, VbType::boolean, expression.at);
        }
        flush_temps();
        return value.operand;
    }

    void lower_if(const Stmt& statement) {
        const int end = builder_->new_label();
        for (std::size_t i = 0; i < statement.conditions.size(); ++i) {
            const int next = builder_->new_label();
            const auto condition = lower_condition(*statement.conditions[i]);
            builder_->branch(Opcode::branch_zero, condition, next);
            lower_block(statement.blocks[i]);
            builder_->jump(end);
            builder_->place(next);
        }
        if (statement.has_else) {
            lower_block(statement.blocks.back());
        }
        builder_->place(end);
    }

    void lower_while(const Stmt& statement) {
        const int top = builder_->new_label();
        const int done = builder_->new_label();
        loops_.push_back(Loop{"do", done});
        const int id = current_id_;
        builder_->place(top);
        touch_statement(id);
        const auto condition = lower_condition(*statement.expr);
        builder_->branch(Opcode::branch_zero, condition, done);
        lower_block(statement.body);
        builder_->jump(top);
        builder_->place(done);
        loops_.pop_back();
    }

    void lower_do(const Stmt& statement) {
        const int top = builder_->new_label();
        const int done = builder_->new_label();
        loops_.push_back(Loop{"do", done});
        const int id = current_id_;
        builder_->place(top);
        touch_statement(id);
        if (statement.expr && !statement.test_after) {
            const auto condition = lower_condition(*statement.expr);
            builder_->branch(
                statement.until ? Opcode::branch_nonzero : Opcode::branch_zero,
                condition, done);
        }
        lower_block(statement.body);
        if (statement.expr && statement.test_after) {
            touch_statement(id);
            // Loop While repeats while true; Loop Until repeats while false.
            const auto condition = lower_condition(*statement.expr);
            builder_->branch(
                statement.until ? Opcode::branch_zero : Opcode::branch_nonzero,
                condition, top);
        } else {
            builder_->jump(top);
        }
        builder_->place(done);
        loops_.pop_back();
    }

    void lower_for(const Stmt& statement) {
        const auto variable = find_variable(statement.text, statement.at);
        if (variable.type != VbType::integer &&
            variable.type != VbType::long_integer) {
            fail("the loop variable '" + statement.text +
                     "' must be an Integer or a Long",
                 statement.at);
        }
        const auto start = convert(lower_expression(*statement.expr),
                                   variable.type, statement.at);
        builder_->emit(Opcode::move, variable.operand, start.operand);
        // The limit and the step are evaluated once.
        const auto limit_value = convert(lower_expression(*statement.limit),
                                         variable.type, statement.at);
        const auto limit = builder_->new_temp();
        builder_->emit(Opcode::move, limit, limit_value.operand);
        Operand step = ir::immediate(1);
        std::optional<std::int64_t> constant_step = 1;
        if (statement.step) {
            const auto step_value = convert(lower_expression(*statement.step),
                                            variable.type, statement.at);
            if (step_value.operand.kind == OperandKind::immediate) {
                step = step_value.operand;
                constant_step = step.value;
            } else {
                step = builder_->new_temp();
                builder_->emit(Opcode::move, step, step_value.operand);
                constant_step.reset();
            }
        }
        flush_temps();
        const int top = builder_->new_label();
        const int done = builder_->new_label();
        loops_.push_back(Loop{"for", done});
        const int id = current_id_;
        builder_->place(top);
        touch_statement(id);
        const auto keep_going = builder_->new_temp();
        if (constant_step.has_value()) {
            builder_->compare(keep_going, variable.operand, limit,
                              *constant_step >= 0 ? Condition::less_equal
                                                  : Condition::greater_equal);
        } else {
            const int negative = builder_->new_label();
            const int test = builder_->new_label();
            const auto up = builder_->new_temp();
            builder_->compare(up, step, ir::immediate(0),
                              Condition::greater_equal);
            builder_->branch(Opcode::branch_zero, up, negative);
            builder_->compare(keep_going, variable.operand, limit,
                              Condition::less_equal);
            builder_->jump(test);
            builder_->place(negative);
            builder_->compare(keep_going, variable.operand, limit,
                              Condition::greater_equal);
            builder_->place(test);
        }
        builder_->branch(Opcode::branch_zero, keep_going, done);
        lower_block(statement.body);
        touch_statement(id);
        builder_->emit(Opcode::add, variable.operand, variable.operand, step,
                       check_for(variable.type));
        builder_->jump(top);
        builder_->place(done);
        loops_.pop_back();
    }

    [[nodiscard]] int label_for(const std::string& name) {
        const auto found = labels_.find(name);
        if (found != labels_.end()) {
            return found->second;
        }
        const int id = builder_->new_label();
        labels_[name] = id;
        return id;
    }

    // The operand that reads or writes element `indices` of an array: the
    // service checks the bounds (error 9) and returns the element's address.
    [[nodiscard]] Operand element_address(const Variable& array,
                                          const std::vector<ExprPtr>& indices,
                                          const Position at) {
        if (indices.empty() || indices.size() > 3U) {
            fail("an array needs one to three indices", at);
        }
        std::vector<Operand> arguments;
        const auto handle = builder_->new_temp();
        builder_->move_wide(handle, array.operand);
        arguments.push_back(handle);
        for (const auto& index : indices) {
            const auto value = lower_expression(*index);
            if (!is_numeric(value.type)) {
                fail("an array index must be a number", index->at);
            }
            arguments.push_back(to_long(value, index->at).operand);
        }
        const auto pointer = builder_->new_temp();
        const Service service = indices.size() == 1U ? Service::array_element1
                                : indices.size() == 2U
                                    ? Service::array_element2
                                    : Service::array_element3;
        builder_->runtime(service, std::move(arguments), pointer, true);
        return ir::indirect(pointer.value);
    }

    // The value of `array(indices)`.
    [[nodiscard]] Value element_value(const Variable& array,
                                      const std::vector<ExprPtr>& indices,
                                      const Position at) {
        const auto operand = element_address(array, indices, at);
        if (array.type == VbType::record) {
            fail("a record cannot be used as a value here", at);
        }
        if (array.type == VbType::string) {
            const auto copy = builder_->new_temp();
            builder_->move_wide(copy, operand);
            return Value{copy, VbType::string};
        }
        return Value{operand, array.type};
    }

    [[nodiscard]] std::optional<Variable> find_array(
        const std::string& name) const {
        if (const auto found = locals_.find(name); found != locals_.end()) {
            return found->second.is_array
                       ? std::optional<Variable>(found->second)
                       : std::nullopt;
        }
        if (const auto found = globals_.find(name); found != globals_.end()) {
            return found->second.is_array
                       ? std::optional<Variable>(found->second)
                       : std::nullopt;
        }
        return std::nullopt;
    }

    void lower_redim(const Stmt& statement) {
        for (const auto& declaration : statement.declarations) {
            const auto variable =
                find_variable(declaration.name, declaration.at);
            if (!variable.is_array) {
                fail("'" + declaration.name + "' is not an array",
                     declaration.at);
            }
            if (declaration.type_given && declaration.type != variable.type) {
                fail("ReDim cannot change the type of '" + declaration.name +
                         "'",
                     declaration.at);
            }
            emit_array_allocation(variable, declaration.bounds, false,
                                  statement.preserve, false);
        }
    }

    // The argument of an array parameter: the address of the array variable.
    [[nodiscard]] Operand array_argument(const Expr& argument,
                                         const VbType type) {
        if (argument.kind != ExprKind::name) {
            fail("an array argument must be the name of an array", argument.at);
        }
        const auto variable = find_variable(argument.text, argument.at);
        if (!variable.is_array || variable.type != type) {
            fail("'" + argument.text + "' is not an array of the type needed",
                 argument.at);
        }
        return address_of(variable.operand);
    }

    // LBound(array [, dimension]) and UBound.
    [[nodiscard]] Value lower_bound(const bool upper,
                                    const std::vector<ExprPtr>& arguments,
                                    const Position at) {
        if (arguments.empty() || arguments.size() > 2U ||
            arguments[0]->kind != ExprKind::name) {
            fail("LBound and UBound need an array name and a dimension", at);
        }
        const auto variable = find_variable(arguments[0]->text, at);
        if (!variable.is_array) {
            fail("'" + arguments[0]->text + "' is not an array", at);
        }
        const auto handle = builder_->new_temp();
        builder_->move_wide(handle, variable.operand);
        Operand dimension = ir::immediate(1);
        if (arguments.size() == 2U) {
            dimension =
                to_long(lower_expression(*arguments[1]), arguments[1]->at)
                    .operand;
        }
        Value result;
        result.type = VbType::long_integer;
        result.operand = builder_->new_temp();
        builder_->runtime(Service::array_bound,
                          {handle, dimension, ir::immediate(upper ? 1 : 0)},
                          result.operand);
        return result;
    }

    // A slot that owns a string for the whole procedure.
    [[nodiscard]] Operand reserve_owned_slot() {
        const auto slot = builder_->new_temp();
        late_zero_.push_back(slot);
        owned_slots_.push_back(Owned{slot, Service::string_release});
        return slot;
    }

    // One test of a Case against the subject; true when it matches.
    [[nodiscard]] Value case_test(const CaseClause& clause,
                                  const Value& subject, const Position at) {
        if (clause.kind == CaseClause::Kind::range) {
            const auto low = compare_values(">=", subject,
                                            lower_expression(*clause.low), at);
            const auto high = compare_values(
                "<=", subject, lower_expression(*clause.high), at);
            const auto both = builder_->new_temp();
            builder_->emit(Opcode::bit_and, both, low.operand, high.operand);
            return Value{both, VbType::boolean};
        }
        const std::string op =
            clause.kind == CaseClause::Kind::relation ? clause.op : "=";
        return compare_values(op, subject, lower_expression(*clause.low), at);
    }

    void lower_select(const Stmt& statement) {
        auto subject = lower_expression(*statement.expr);
        if (subject.type == VbType::string) {
            // The subject is read by every test: keep it in a slot of its own.
            const auto slot = reserve_owned_slot();
            builder_->runtime(Service::string_assign,
                              {address_of(slot), subject.operand});
            subject = Value{slot, VbType::string};
        } else if (is_numeric(subject.type)) {
            const auto copy = builder_->new_temp();
            if (is_float(subject.type)) {
                builder_->move_wide(copy, subject.operand);
            } else {
                builder_->emit(Opcode::move, copy, subject.operand);
            }
            subject = Value{copy, subject.type};
        } else {
            fail("Select Case needs a number or a String", statement.at);
        }
        flush_temps();
        const int end = builder_->new_label();
        for (const auto& block : statement.cases) {
            const int body = builder_->new_label();
            const int next = builder_->new_label();
            for (const auto& clause : block.clauses) {
                const auto test = case_test(clause, subject, statement.at);
                flush_temps();
                builder_->branch(Opcode::branch_nonzero, test.operand, body);
            }
            builder_->jump(next);
            builder_->place(body);
            lower_block(block.body);
            builder_->jump(end);
            builder_->place(next);
        }
        if (statement.has_else) {
            lower_block(statement.body);
        }
        builder_->place(end);
    }

    // ----- Constants -------------------------------------------------------

    void declare_constants(const std::vector<ConstantDecl>& declarations,
                           std::map<std::string, Value>& into) {
        for (const auto& declaration : declarations) {
            if (into.contains(declaration.name) ||
                globals_.contains(declaration.name) ||
                signatures_.contains(declaration.name)) {
                fail("'" + declaration.name + "' is already declared",
                     declaration.at);
            }
            auto value = fold(*declaration.value);
            if (declaration.type_given) {
                value = fold_convert(value, declaration.type, declaration.at);
            }
            into[declaration.name] = value;
        }
    }

    [[nodiscard]] std::optional<Value> find_constant(
        const std::string& name) const {
        if (const auto found = local_constants_.find(name);
            found != local_constants_.end()) {
            return found->second;
        }
        if (const auto found = constants_.find(name);
            found != constants_.end()) {
            return found->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] static double double_of(const Value& value) {
        if (is_float(value.type)) {
            double result = 0;
            const auto bits = value.operand.value;
            std::memcpy(&result, &bits, sizeof(result));
            return result;
        }
        return static_cast<double>(value.operand.value);
    }

    [[nodiscard]] Value float_constant(const double number, const VbType type) {
        const double stored =
            type == VbType::single_precision
                ? static_cast<double>(static_cast<float>(number))
                : number;
        if (std::isinf(stored) || std::isnan(stored)) {
            fail("the constant overflows", Position{});
        }
        return Value{double_immediate(stored), type};
    }

    [[nodiscard]] Value integer_constant(const std::int64_t number,
                                         const VbType type, const Position at) {
        if (type == VbType::integer && (number < -32768 || number > 32767)) {
            fail("the constant overflows an Integer", at);
        }
        if (type == VbType::long_integer &&
            (number < -2147483648LL || number > 2147483647LL)) {
            fail("the constant overflows a Long", at);
        }
        return Value{ir::immediate(number), type};
    }

    [[nodiscard]] std::string literal_text(const Value& value) const {
        if (value.type == VbType::string) {
            return program_
                .strings[static_cast<std::size_t>(value.operand.value)];
        }
        if (value.type == VbType::boolean) {
            return value.operand.value != 0 ? "True" : "False";
        }
        if (is_float(value.type)) {
            char buffer[48];
            std::snprintf(buffer, sizeof(buffer), "%.*G", digits_of(value.type),
                          double_of(value));
            return buffer;
        }
        return std::to_string(value.operand.value);
    }

    [[nodiscard]] Value string_constant(const std::string& text) {
        program_.strings.push_back(text);
        return Value{ir::literal(static_cast<std::int64_t>(
                         program_.strings.size() - 1U)),
                     VbType::string};
    }

    // Evaluates a constant expression at compile time.
    [[nodiscard]] Value fold(const Expr& expression) {
        switch (expression.kind) {
            case ExprKind::integer_literal: {
                const bool is_long = expression.long_literal ||
                                     expression.number > 32767 ||
                                     expression.number < -32768;
                return Value{ir::immediate(expression.number),
                             is_long ? VbType::long_integer : VbType::integer};
            }
            case ExprKind::real_literal:
                return float_constant(expression.real,
                                      expression.single_literal
                                          ? VbType::single_precision
                                          : VbType::double_precision);
            case ExprKind::boolean_literal:
                return Value{ir::immediate(expression.number), VbType::boolean};
            case ExprKind::string_literal:
                return string_constant(expression.text);
            case ExprKind::name: {
                if (const auto found = find_constant(expression.text)) {
                    return *found;
                }
                fail("'" + expression.text + "' is not a constant",
                     expression.at);
            }
            case ExprKind::call:
                fail("a function call is not a constant expression",
                     expression.at);
            case ExprKind::unary:
                return fold_unary(expression);
            case ExprKind::binary:
                return fold_binary(expression);
        }
        fail("unsupported constant expression", expression.at);
    }

    [[nodiscard]] Value fold_unary(const Expr& expression) {
        const auto operand = fold(*expression.operands[0]);
        if (!is_numeric(operand.type)) {
            fail("the operator " + expression.text + " needs a number",
                 expression.at);
        }
        if (expression.text == "not") {
            const auto whole = is_float(operand.type)
                                   ? static_cast<std::int64_t>(
                                         std::nearbyint(double_of(operand)))
                                   : operand.operand.value;
            return Value{ir::immediate(~whole), operand.type};
        }
        if (is_float(operand.type)) {
            return float_constant(-double_of(operand), operand.type);
        }
        return integer_constant(-operand.operand.value,
                                arithmetic_type(operand.type), expression.at);
    }

    [[nodiscard]] Value fold_binary(const Expr& expression) {
        const std::string& op = expression.text;
        const auto left = fold(*expression.operands[0]);
        const auto right = fold(*expression.operands[1]);
        const Position at = expression.at;
        const bool is_comparison = op == "=" || op == "<>" || op == "<" ||
                                   op == ">" || op == "<=" || op == ">=";
        if (op == "&" || (op == "+" && left.type == VbType::string &&
                          right.type == VbType::string)) {
            return string_constant(literal_text(left) + literal_text(right));
        }
        if (left.type == VbType::string || right.type == VbType::string) {
            if (is_comparison && left.type == right.type) {
                const int order =
                    literal_text(left).compare(literal_text(right));
                return Value{ir::immediate(compare_result(op, order) ? -1 : 0),
                             VbType::boolean};
            }
            fail("the operator " + op + " is not allowed on these constants",
                 at);
        }
        if (!is_numeric(left.type) || !is_numeric(right.type)) {
            fail("the operator " + op + " needs numbers", at);
        }
        const bool any_float = is_float(left.type) || is_float(right.type);
        if (is_comparison) {
            const double a = double_of(left);
            const double b = double_of(right);
            const int order = a < b ? -1 : (a > b ? 1 : 0);
            return Value{ir::immediate(compare_result(op, order) ? -1 : 0),
                         VbType::boolean};
        }
        const auto whole = [&](const Value& value) {
            return is_float(value.type) ? static_cast<std::int64_t>(
                                              std::nearbyint(double_of(value)))
                                        : value.operand.value;
        };
        if (op == "and" || op == "or" || op == "xor") {
            const auto a = whole(left);
            const auto b = whole(right);
            const auto bits = op == "and"  ? (a & b)
                              : op == "or" ? (a | b)
                                           : (a ^ b);
            const VbType type =
                left.type == VbType::boolean && right.type == VbType::boolean
                    ? VbType::boolean
                : (left.type == VbType::long_integer ||
                   right.type == VbType::long_integer || any_float)
                    ? VbType::long_integer
                    : VbType::integer;
            return Value{ir::immediate(bits), type};
        }
        if (op == "^") {
            const double a = double_of(left);
            const double b = double_of(right);
            if (a == 0.0 && b < 0.0) {
                fail("division by zero in a constant", at);
            }
            const double result = std::pow(a, b);
            if (!std::isfinite(result)) {
                fail("overflow in a constant", at);
            }
            return float_constant(result, VbType::double_precision);
        }
        if (op == "/" || (any_float && (op == "+" || op == "-" || op == "*"))) {
            const double a = double_of(left);
            const double b = double_of(right);
            if (op == "/" && b == 0.0) {
                fail("division by zero in a constant", at);
            }
            const double result = op == "+"   ? a + b
                                  : op == "-" ? a - b
                                  : op == "*" ? a * b
                                              : a / b;
            VbType type = VbType::single_precision;
            if (left.type == VbType::double_precision ||
                right.type == VbType::double_precision ||
                (op == "/" && !any_float)) {
                type = VbType::double_precision;
            }
            return float_constant(result, type);
        }
        const auto a = whole(left);
        const auto b = whole(right);
        const VbType type = any_float || left.type == VbType::long_integer ||
                                    right.type == VbType::long_integer
                                ? VbType::long_integer
                                : VbType::integer;
        if ((op == "\\" || op == "mod") && b == 0) {
            fail("division by zero in a constant", at);
        }
        const std::int64_t result = op == "+"    ? a + b
                                    : op == "-"  ? a - b
                                    : op == "*"  ? a * b
                                    : op == "\\" ? a / b
                                                 : a % b;
        return integer_constant(result, type, at);
    }

    [[nodiscard]] static bool compare_result(const std::string& op,
                                             const int order) {
        return op == "="    ? order == 0
               : op == "<>" ? order != 0
               : op == "<"  ? order < 0
               : op == ">"  ? order > 0
               : op == "<=" ? order <= 0
                            : order >= 0;
    }

    // Converts a constant to the declared type at compile time.
    [[nodiscard]] Value fold_convert(const Value& value, const VbType to,
                                     const Position at) {
        if (to == VbType::string) {
            if (value.type != VbType::string) {
                fail("a String constant needs a String value", at);
            }
            return value;
        }
        if (!is_numeric(value.type)) {
            fail("a numeric constant needs a numeric value", at);
        }
        if (is_float(to)) {
            return float_constant(double_of(value), to);
        }
        const auto whole =
            is_float(value.type)
                ? static_cast<std::int64_t>(std::nearbyint(double_of(value)))
                : value.operand.value;
        if (to == VbType::boolean) {
            return Value{ir::immediate(whole != 0 ? -1 : 0), VbType::boolean};
        }
        return integer_constant(whole, to, at);
    }

    void lower_exit(const Stmt& statement) {
        if (statement.text == "sub" || statement.text == "function") {
            if ((statement.text == "function") != current_->is_function) {
                fail("Exit " + statement.text + " is outside a " +
                         statement.text,
                     statement.at);
            }
            builder_->jump(end_label_);
            return;
        }
        if (statement.text == "end") {
            builder_->runtime(Service::end_program, {});
            return;
        }
        for (auto it = loops_.rbegin(); it != loops_.rend(); ++it) {
            if (it->kind == statement.text) {
                builder_->jump(it->exit_label);
                return;
            }
        }
        fail("Exit " + statement.text + " is outside a loop", statement.at);
    }

    // ----- Calls -----------------------------------------------------------

    [[nodiscard]] static bool is_special_builtin(const std::string& name) {
        static const char* const names[] = {
            "cstr", "instr", "str", "cint", "clng",   "cdbl",  "csng",
            "int",  "fix",   "abs", "sgn",  "sqr",    "sin",   "cos",
            "tan",  "atn",   "log", "exp",  "lbound", "ubound"};
        for (const char* candidate : names) {
            if (name == candidate) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] const Builtin* find_builtin(const std::string& name) const {
        for (const auto& builtin : kBuiltins) {
            if (name == builtin.name) {
                return &builtin;
            }
        }
        return nullptr;
    }

    // Lowers a call; the returned operand holds the function's value (an
    // unused temporary for a Sub).
    [[nodiscard]] Value lower_call(const std::string& name,
                                   const std::vector<ExprPtr>& arguments,
                                   const Position at, const bool want_value) {
        const auto found = signatures_.find(name);
        if (found == signatures_.end()) {
            if (const auto* builtin = find_builtin(name)) {
                return lower_builtin(*builtin, arguments, at);
            }
            if (is_special_builtin(name)) {
                return lower_special_builtin(name, arguments, at);
            }
            fail("'" + name + "' is not a Sub or Function of the project", at);
        }
        const auto& signature = found->second;
        if (want_value && !signature.is_function) {
            fail("'" + name + "' is a Sub and has no value", at);
        }
        if (arguments.size() != signature.parameters.size()) {
            fail("'" + name + "' takes " +
                     std::to_string(signature.parameters.size()) +
                     " argument(s), not " + std::to_string(arguments.size()),
                 at);
        }
        std::vector<Operand> passed;
        std::uint32_t double_args = 0;
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            const auto& parameter = signature.parameters[i];
            const auto& argument = *arguments[i];
            if (parameter.is_array) {
                passed.push_back(array_argument(argument, parameter.type));
                continue;
            }
            if (parameter.by_value && is_float(parameter.type)) {
                double_args |= 1U << i;
            }
            if (parameter.by_value) {
                const auto value = lower_expression(argument);
                if (parameter.type == VbType::string) {
                    if (value.type != VbType::string) {
                        fail("a String argument is needed here", argument.at);
                    }
                    passed.push_back(value.operand);
                } else {
                    passed.push_back(
                        convert(value, parameter.type, argument.at).operand);
                }
                continue;
            }
            passed.push_back(by_reference(argument, parameter.type));
        }
        Value result;
        result.type = signature.return_type;
        const bool wide = signature.return_type == VbType::string;
        result.operand = wide ? new_owned_string() : builder_->new_temp();
        builder_->call(result.operand, signature.index, std::move(passed), wide,
                       double_args, is_float(signature.return_type));
        return result;
    }

    // The address argument of a ByRef parameter.
    [[nodiscard]] Operand by_reference(const Expr& argument,
                                       const VbType type) {
        const bool is_element =
            argument.kind == ExprKind::call && find_array(argument.text);
        if (argument.kind == ExprKind::member || is_element ||
            (type == VbType::record && argument.kind == ExprKind::name)) {
            const auto place = lower_place(argument);
            if (place.type != type) {
                fail("ByRef argument type mismatch", argument.at);
            }
            return type == VbType::record ? struct_pointer(place)
                                          : address_of(place.operand);
        }
        if (argument.kind == ExprKind::name &&
            !signatures_.contains(argument.text)) {
            const auto variable = find_variable(argument.text, argument.at);
            if (variable.type != type) {
                fail("ByRef argument type mismatch for '" + argument.text + "'",
                     argument.at);
            }
            return address_of(variable.operand);
        }
        // An expression is passed by reference through a temporary.
        const auto value = lower_expression(argument);
        const auto temp = builder_->new_temp();
        if (type == VbType::string) {
            if (value.type != VbType::string) {
                fail("a String argument is needed here", argument.at);
            }
            // The temporary holds its own reference until the call ends.
            builder_->move_wide(temp, ir::immediate(0));
            builder_->runtime(Service::string_assign,
                              {address_of(temp), value.operand});
            pending_.push_back(temp);
        } else if (is_float(type)) {
            builder_->move_wide(temp,
                                convert(value, type, argument.at).operand);
        } else {
            builder_->emit(Opcode::move, temp,
                           convert(value, type, argument.at).operand);
        }
        return address_of(temp);
    }

    void lower_call_statement(const Stmt& statement) {
        if (statement.text.rfind("err.", 0) == 0) {
            lower_err_statement(statement);
            return;
        }
        static_cast<void>(
            lower_call(statement.text, statement.items, statement.at, false));
    }

    // ----- Builtin functions -----------------------------------------------

    [[nodiscard]] Value lower_builtin(const Builtin& builtin,
                                      const std::vector<ExprPtr>& arguments,
                                      const Position at) {
        const auto count = static_cast<int>(arguments.size());
        if (count < builtin.min_arguments || count > builtin.max_arguments) {
            fail(std::string(builtin.name) + " takes " +
                     std::to_string(builtin.min_arguments) + " to " +
                     std::to_string(builtin.max_arguments) + " argument(s)",
                 at);
        }
        std::vector<Operand> passed;
        for (int i = 0; i < count; ++i) {
            const auto& argument = *arguments[static_cast<std::size_t>(i)];
            const auto value = lower_expression(argument);
            if (builtin.parameters[i] == 's') {
                if (value.type != VbType::string) {
                    fail(std::string(builtin.name) +
                             " needs a String argument here",
                         argument.at);
                }
            } else if (!is_numeric(value.type)) {
                fail(std::string(builtin.name) +
                         " needs a numeric argument here",
                     argument.at);
            }
            // A number argument is a 32-bit integer (Left, Mid, Chr, Space).
            passed.push_back(builtin.parameters[i] == 'n'
                                 ? to_long(value, argument.at).operand
                                 : value.operand);
        }
        if (builtin.service == Service::string_mid && count == 2) {
            passed.push_back(ir::immediate(INT32_MIN));  // length omitted
        }
        Value result;
        result.type = builtin.result;
        const bool wide = builtin.result == VbType::string;
        result.operand = wide ? new_owned_string() : builder_->new_temp();
        builder_->runtime(builtin.service, std::move(passed), result.operand,
                          wide);
        return result;
    }

    // CStr, InStr and the numeric conversion and math functions.
    [[nodiscard]] Value lower_special_builtin(
        const std::string& name, const std::vector<ExprPtr>& arguments,
        const Position at) {
        if (name == "instr") {
            return lower_instr(arguments, at);
        }
        if (name == "lbound" || name == "ubound") {
            return lower_bound(name == "ubound", arguments, at);
        }
        if (arguments.size() != 1U) {
            fail(name + " takes 1 argument", at);
        }
        const auto value = lower_expression(*arguments[0]);
        if (name == "cstr") {
            return to_string_value(value, at);
        }
        if (name == "str") {
            if (!is_numeric(value.type)) {
                fail("Str needs a numeric argument", at);
            }
            Value result;
            result.type = VbType::string;
            result.operand = new_owned_string();
            if (is_float(value.type)) {
                builder_->runtime(
                    Service::string_from_double,
                    {value.operand, ir::immediate(digits_of(value.type)),
                     ir::immediate(1)},
                    result.operand, true, 1U);
            } else {
                builder_->runtime(Service::string_from_number,
                                  {to_long(value, at).operand}, result.operand,
                                  true);
            }
            return result;
        }
        if (!is_numeric(value.type)) {
            fail(name + " needs a numeric argument", at);
        }
        if (name == "clng") {
            return to_long(value, at);
        }
        if (name == "cint") {
            return convert(value, VbType::integer, at);
        }
        if (name == "cdbl") {
            auto result = to_double(value);
            result.type = VbType::double_precision;
            return result;
        }
        if (name == "csng") {
            return convert(value, VbType::single_precision, at);
        }
        if (name == "int" || name == "fix") {
            if (!is_float(value.type)) {
                return value;
            }
            Value result =
                math_value(name == "int" ? ir::MathFunction::floor
                                         : ir::MathFunction::truncate,
                           value);
            result.type = value.type;
            return result;
        }
        if (name == "abs") {
            return lower_abs(value);
        }
        if (name == "sgn") {
            Value result;
            result.type = VbType::integer;
            result.operand = builder_->new_temp();
            builder_->runtime(Service::double_sign, {to_double(value).operand},
                              result.operand, false, 1U);
            return result;
        }
        const ir::MathFunction which =
            name == "sqr"   ? ir::MathFunction::square_root
            : name == "sin" ? ir::MathFunction::sine
            : name == "cos" ? ir::MathFunction::cosine
            : name == "tan" ? ir::MathFunction::tangent
            : name == "atn" ? ir::MathFunction::arctangent
            : name == "log" ? ir::MathFunction::logarithm
                            : ir::MathFunction::exponential;
        return math_value(which, value);
    }

    [[nodiscard]] Value lower_instr(const std::vector<ExprPtr>& arguments,
                                    const Position at) {
        if (arguments.size() < 2U || arguments.size() > 3U) {
            fail("InStr takes 2 or 3 arguments", at);
        }
        std::vector<Operand> passed;
        std::size_t next = 0;
        if (arguments.size() == 3U) {
            const auto start = lower_expression(*arguments[0]);
            if (!is_numeric(start.type)) {
                fail("InStr needs a numeric start", arguments[0]->at);
            }
            passed.push_back(to_long(start, at).operand);
            next = 1;
        } else {
            passed.push_back(ir::immediate(1));
        }
        for (std::size_t i = next; i < arguments.size(); ++i) {
            const auto value = lower_expression(*arguments[i]);
            if (value.type != VbType::string) {
                fail("InStr needs String arguments", arguments[i]->at);
            }
            passed.push_back(value.operand);
        }
        Value result;
        result.type = VbType::long_integer;
        result.operand = builder_->new_temp();
        builder_->runtime(Service::string_instr, std::move(passed),
                          result.operand);
        return result;
    }

    // A Double result of one of the math functions.
    [[nodiscard]] Value math_value(const ir::MathFunction which,
                                   const Value& argument) {
        Value result;
        result.type = VbType::double_precision;
        result.operand = builder_->new_temp();
        builder_->runtime(Service::math_function,
                          {ir::immediate(static_cast<std::int64_t>(which)),
                           to_double(argument).operand},
                          result.operand, false, 2U, true);
        return result;
    }

    // Abs keeps the type of its argument.
    [[nodiscard]] Value lower_abs(const Value& value) {
        if (is_float(value.type)) {
            Value result = math_value(ir::MathFunction::absolute, value);
            result.type = value.type;
            return result;
        }
        const VbType type = arithmetic_type(value.type);
        Value result;
        result.type = type;
        result.operand = builder_->new_temp();
        builder_->emit(Opcode::move, result.operand, value.operand);
        const int done = builder_->new_label();
        const auto negative = builder_->new_temp();
        builder_->compare(negative, value.operand, ir::immediate(0),
                          Condition::less);
        builder_->branch(Opcode::branch_zero, negative, done);
        builder_->emit(Opcode::negate, result.operand, value.operand, {},
                       check_for(type));
        builder_->place(done);
        return result;
    }

    // A number or Boolean as a String (CStr); a String stays as it is.
    [[nodiscard]] Value to_string_value(const Value& value, const Position at) {
        if (value.type == VbType::string) {
            return value;
        }
        if (!is_numeric(value.type)) {
            fail("this value cannot be converted to a String yet", at);
        }
        Value result;
        result.type = VbType::string;
        result.operand = new_owned_string();
        if (is_float(value.type)) {
            builder_->runtime(
                Service::string_from_double,
                {value.operand, ir::immediate(digits_of(value.type)),
                 ir::immediate(0)},
                result.operand, true, 1U);
            return result;
        }
        builder_->runtime(value.type == VbType::boolean
                              ? Service::string_from_boolean
                              : Service::string_from_integer,
                          {value.operand}, result.operand, true);
        return result;
    }

    // ----- Expressions -----------------------------------------------------

    [[nodiscard]] Value lower_expression(const Expr& expression) {
        switch (expression.kind) {
            case ExprKind::integer_literal: {
                const bool is_long = expression.long_literal ||
                                     expression.number > 32767 ||
                                     expression.number < -32768;
                return Value{ir::immediate(expression.number),
                             is_long ? VbType::long_integer : VbType::integer};
            }
            case ExprKind::real_literal:
                if (expression.single_literal) {
                    return Value{double_immediate(static_cast<double>(
                                     static_cast<float>(expression.real))),
                                 VbType::single_precision};
                }
                return Value{double_immediate(expression.real),
                             VbType::double_precision};
            case ExprKind::boolean_literal:
                return Value{ir::immediate(expression.number), VbType::boolean};
            case ExprKind::string_literal:
                program_.strings.push_back(expression.text);
                return Value{ir::literal(static_cast<std::int64_t>(
                                 program_.strings.size() - 1)),
                             VbType::string};
            case ExprKind::name: {
                if (expression.text.rfind("err.", 0) == 0) {
                    return lower_err_member(expression);
                }
                if (!locals_.contains(expression.text) &&
                    !(result_variable_.has_value() &&
                      expression.text == current_->name) &&
                    !globals_.contains(expression.text) &&
                    (signatures_.contains(expression.text) ||
                     find_builtin(expression.text) != nullptr ||
                     is_special_builtin(expression.text))) {
                    return lower_call(expression.text, {}, expression.at, true);
                }
                if (!locals_.contains(expression.text) &&
                    !globals_.contains(expression.text) &&
                    !(result_variable_.has_value() &&
                      expression.text == current_->name)) {
                    if (const auto constant = find_constant(expression.text)) {
                        return *constant;
                    }
                }
                const auto variable =
                    find_variable(expression.text, expression.at);
                if (variable.is_array) {
                    fail("the array '" + expression.text + "' needs an index",
                         expression.at);
                }
                if (variable.type == VbType::string &&
                    variable.operand.kind != OperandKind::slot) {
                    // String handles are read through a slot.
                    const auto copy = builder_->new_temp();
                    builder_->move_wide(copy, variable.operand);
                    return Value{copy, VbType::string};
                }
                return Value{variable.operand, variable.type};
            }
            case ExprKind::call:
                if (const auto array = find_array(expression.text)) {
                    return element_value(*array, expression.operands,
                                         expression.at);
                }
                return lower_call(expression.text, expression.operands,
                                  expression.at, true);
            case ExprKind::member: {
                const auto place = lower_place(expression);
                if (place.type == VbType::record) {
                    fail("a record cannot be used as a value here",
                         expression.at);
                }
                if (place.type == VbType::string) {
                    const auto copy = builder_->new_temp();
                    builder_->move_wide(copy, place.operand);
                    return Value{copy, VbType::string};
                }
                return Value{place.operand, place.type};
            }
            case ExprKind::unary:
                return lower_unary(expression);
            case ExprKind::binary:
                return lower_binary(expression);
        }
        fail("unsupported expression", expression.at);
    }

    [[nodiscard]] Value lower_unary(const Expr& expression) {
        auto operand = lower_expression(*expression.operands[0]);
        if (!is_numeric(operand.type)) {
            fail("the operator " + expression.text + " needs a numeric value",
                 expression.at);
        }
        const auto result = builder_->new_temp();
        if (expression.text == "not") {
            operand = to_long_if_float(operand, expression.at);
            builder_->emit(Opcode::bit_not, result, operand.operand);
            return Value{result, operand.type};
        }
        if (is_float(operand.type)) {
            builder_->emit(Opcode::fnegate, result, operand.operand);
            return Value{result, operand.type};
        }
        const auto type = arithmetic_type(operand.type);
        builder_->emit(Opcode::negate, result, operand.operand, {},
                       check_for(type));
        return Value{result, type};
    }

    [[nodiscard]] Value concatenate(const Value& left, const Value& right,
                                    const Position at) {
        const auto a = to_string_value(left, at);
        const auto b = to_string_value(right, at);
        Value result;
        result.type = VbType::string;
        result.operand = new_owned_string();
        builder_->runtime(Service::string_concat, {a.operand, b.operand},
                          result.operand, true);
        return result;
    }

    [[nodiscard]] static Condition condition_of(const std::string& op) {
        return op == "="    ? Condition::equal
               : op == "<>" ? Condition::not_equal
               : op == "<"  ? Condition::less
               : op == ">"  ? Condition::greater
               : op == "<=" ? Condition::less_equal
                            : Condition::greater_equal;
    }

    // A floating-point value for arithmetic: an integer is converted.
    [[nodiscard]] Value to_double(const Value& value) {
        if (is_float(value.type)) {
            return value;
        }
        Value result;
        result.type = VbType::double_precision;
        result.operand = builder_->new_temp();
        builder_->emit(Opcode::int_to_double, result.operand, value.operand);
        return result;
    }

    // A 32-bit integer for \, Mod, And, Or, Not and the conversions: a
    // floating-point value is rounded (error 6 when it does not fit).
    [[nodiscard]] Value to_long(const Value& value, const Position at) {
        if (is_integral(value.type)) {
            return value;
        }
        if (!is_float(value.type)) {
            fail("this value cannot be converted to a number", at);
        }
        Value result;
        result.type = VbType::long_integer;
        result.operand = builder_->new_temp();
        builder_->runtime(Service::double_to_long, {value.operand},
                          result.operand, false, 1U);
        return result;
    }

    [[nodiscard]] Value to_long_if_float(const Value& value,
                                         const Position at) {
        return is_float(value.type) ? to_long(value, at) : value;
    }

    [[nodiscard]] Value lower_float_arithmetic(const std::string& op,
                                               const Value& left,
                                               const Value& right) {
        const bool both_integral =
            is_integral(left.type) && is_integral(right.type);
        VbType type = VbType::single_precision;
        if (left.type == VbType::double_precision ||
            right.type == VbType::double_precision ||
            (op == "/" && both_integral)) {
            type = VbType::double_precision;
        }
        const auto a = to_double(left);
        const auto b = to_double(right);
        const Opcode code = op == "+"   ? Opcode::fadd
                            : op == "-" ? Opcode::fsub
                            : op == "*" ? Opcode::fmul
                                        : Opcode::fdivide;
        const auto result = builder_->new_temp();
        builder_->emit(code, result, a.operand, b.operand);
        if (type == VbType::single_precision) {
            builder_->emit(Opcode::round_single, result, result);
        }
        return Value{result, type};
    }

    // `left op right` for the six comparison operators.
    [[nodiscard]] Value compare_values(const std::string& op, const Value& left,
                                       const Value& right, const Position at) {
        if (left.type == VbType::string && right.type == VbType::string) {
            const auto order = builder_->new_temp();
            builder_->runtime(Service::string_compare,
                              {left.operand, right.operand}, order);
            const auto result = builder_->new_temp();
            builder_->compare(result, order, ir::immediate(0),
                              condition_of(op));
            return Value{result, VbType::boolean};
        }
        if (!is_numeric(left.type) || !is_numeric(right.type)) {
            fail("the operator " + op + " needs two numbers or two Strings",
                 at);
        }
        const auto result = builder_->new_temp();
        if (is_float(left.type) || is_float(right.type)) {
            builder_->fcompare(result, to_double(left).operand,
                               to_double(right).operand, condition_of(op));
        } else {
            builder_->compare(result, left.operand, right.operand,
                              condition_of(op));
        }
        return Value{result, VbType::boolean};
    }

    [[nodiscard]] Value lower_binary(const Expr& expression) {
        const std::string& op = expression.text;
        const auto left = lower_expression(*expression.operands[0]);
        const auto right = lower_expression(*expression.operands[1]);
        const bool is_comparison = op == "=" || op == "<>" || op == "<" ||
                                   op == ">" || op == "<=" || op == ">=";
        if (op == "&") {
            return concatenate(left, right, expression.at);
        }
        if (is_comparison) {
            return compare_values(op, left, right, expression.at);
        }
        if (left.type == VbType::string || right.type == VbType::string) {
            if (op == "+" && left.type == VbType::string &&
                right.type == VbType::string) {
                return concatenate(left, right, expression.at);
            }
            fail("the operator " + op +
                     " is not compiled for these String operands yet",
                 expression.at);
        }
        if (!is_numeric(left.type) || !is_numeric(right.type)) {
            fail("the operator " + op + " needs numeric values", expression.at);
        }
        const bool any_float = is_float(left.type) || is_float(right.type);
        const auto result = builder_->new_temp();
        if (op == "^") {
            builder_->runtime(
                Service::double_power,
                {to_double(left).operand, to_double(right).operand}, result,
                false, 3U, true);
            return Value{result, VbType::double_precision};
        }
        if (op == "and" || op == "or" || op == "xor") {
            const auto a = to_long_if_float(left, expression.at);
            const auto b = to_long_if_float(right, expression.at);
            const Opcode code = op == "and"  ? Opcode::bit_and
                                : op == "or" ? Opcode::bit_or
                                             : Opcode::bit_xor;
            builder_->emit(code, result, a.operand, b.operand);
            const VbType type =
                a.type == VbType::boolean && b.type == VbType::boolean
                    ? VbType::boolean
                : (a.type == VbType::long_integer ||
                   b.type == VbType::long_integer)
                    ? VbType::long_integer
                    : VbType::integer;
            return Value{result, type};
        }
        if (op == "/" || (any_float && (op == "+" || op == "-" || op == "*"))) {
            return lower_float_arithmetic(op, left, right);
        }
        // Integer arithmetic; \ and Mod round floating-point operands.
        const auto a = to_long_if_float(left, expression.at);
        const auto b = to_long_if_float(right, expression.at);
        const VbType type = any_float || a.type == VbType::long_integer ||
                                    b.type == VbType::long_integer
                                ? VbType::long_integer
                                : VbType::integer;
        const Opcode code = op == "+"    ? Opcode::add
                            : op == "-"  ? Opcode::sub
                            : op == "*"  ? Opcode::mul
                            : op == "\\" ? Opcode::divide
                                         : Opcode::modulo;
        builder_->emit(code, result, a.operand, b.operand, check_for(type));
        return Value{result, type};
    }

    // Converts a value for an assignment or a parameter.
    [[nodiscard]] Value convert(const Value& value, const VbType to,
                                const Position at) {
        if (!is_numeric(value.type)) {
            fail("this value cannot be converted yet", at);
        }
        if (value.type == to) {
            return value;
        }
        if (is_float(to)) {
            auto result = to_double(value);
            if (to == VbType::single_precision &&
                (result.type == VbType::double_precision ||
                 value.type == VbType::long_integer)) {
                const auto rounded = builder_->new_temp();
                builder_->emit(Opcode::round_single, rounded, result.operand);
                return Value{rounded, to};
            }
            return Value{result.operand, to};
        }
        if (is_float(value.type)) {
            if (to == VbType::boolean) {
                const auto result = builder_->new_temp();
                builder_->fcompare(result, value.operand, double_immediate(0.0),
                                   Condition::not_equal);
                return Value{result, VbType::boolean};
            }
            const auto whole = to_long(value, at);
            return convert(whole, to, at);
        }
        if (to == VbType::boolean) {
            const auto result = builder_->new_temp();
            builder_->compare(result, value.operand, ir::immediate(0),
                              Condition::not_equal);
            return Value{result, VbType::boolean};
        }
        if (to == VbType::integer && value.type == VbType::long_integer) {
            if (value.operand.kind == OperandKind::immediate &&
                value.operand.value >= -32768 && value.operand.value <= 32767) {
                return Value{value.operand, VbType::integer};
            }
            // A value outside the Integer range is run-time error 6.
            const auto result = builder_->new_temp();
            builder_->emit(Opcode::narrow, result, value.operand);
            return Value{result, VbType::integer};
        }
        // Boolean or Integer to Long, Boolean to Integer: the values agree.
        return Value{value.operand, to};
    }

    const Module& module_;
    ir::Program program_;
    std::map<std::string, Variable> globals_;
    std::map<std::string, Signature> signatures_;
    std::map<std::string, Variable> locals_;
    std::optional<Variable> result_variable_;
    std::vector<Loop> loops_;
    std::vector<Owned> owned_slots_;  // handles released when a procedure ends
    std::vector<GlobalArray> global_arrays_;
    std::vector<UdtInfo> udts_;
    std::map<std::string, int> udt_index_;
    std::vector<OwnedRecord> owned_records_;
    std::map<int, int> descriptors_;  // record type -> string blob index
    std::vector<Operand> pending_;    // string temporaries of a statement
    std::map<std::string, Value> constants_;        // module constants
    std::map<std::string, Value> local_constants_;  // a procedure's constants
    std::map<std::string, int> labels_;
    std::set<std::string> defined_labels_;
    std::vector<Operand> late_zero_;  // hidden string slots, zeroed at entry
    std::size_t entry_index_{};
    const Procedure* current_{};
    int error_label_{-1};
    bool uses_errors_{};
    Operand handler_slot_;
    Operand saved_handler_slot_;
    Operand stmt_id_slot_;
    Operand error_stmt_slot_;
    int dispatch_label_{};
    int resume_next_label_{};
    int resume_retry_label_{};
    int current_id_{-1};
    std::vector<int> start_labels_;
    std::vector<int> end_labels_;
    std::vector<int> handler_labels_;
    FunctionBuilder* builder_{};
    int end_label_{};
};

}  // namespace

LowerResult lower_module(const Module& module) {
    LowerResult result;
    try {
        Lowerer lowerer(module);
        result.program = lowerer.run();
    } catch (const LowerFailure& failure) {
        result.error = failure.message;
        result.error_at = failure.at;
    }
    return result;
}

}  // namespace wfc::compiler
