#include "lower.hpp"

#include <algorithm>
#include <map>
#include <optional>
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
    VbType type{VbType::none};
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
    // A runtime service; `result` receives its value when given.
    void runtime(const Service service, std::vector<Operand> args,
                 const Operand result = {}, const bool wide = false) {
        ir::Instruction instruction;
        instruction.op = Opcode::runtime;
        instruction.target = static_cast<int>(service);
        instruction.args = std::move(args);
        instruction.dst = result;
        instruction.wide = wide;
        function_.code.push_back(std::move(instruction));
    }
    void print_text(const int string_index) {
        ir::Instruction instruction;
        instruction.op = Opcode::print_text;
        instruction.target = string_index;
        function_.code.push_back(std::move(instruction));
    }
    void call(const Operand dst, const int callee, std::vector<Operand> args,
              const bool wide) {
        ir::Instruction instruction;
        instruction.op = Opcode::call;
        instruction.dst = dst;
        instruction.target = callee;
        instruction.args = std::move(args);
        instruction.wide = wide;
        function_.code.push_back(std::move(instruction));
    }
    void ret() {
        ir::Instruction instruction;
        instruction.op = Opcode::ret;
        function_.code.push_back(std::move(instruction));
    }

private:
    ir::Function& function_;
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
    {"str", 1, 1, Service::string_from_number, VbType::string, "n"},
    {"space", 1, 1, Service::string_space, VbType::string, "n"},
};

class Lowerer final {
public:
    explicit Lowerer(const Module& module) : module_(module) {}

    ir::Program run() {
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

    void declare_globals() {
        for (const auto& declaration : module_.globals) {
            require_type(declaration);
            if (globals_.contains(declaration.name)) {
                fail("'" + declaration.name + "' is already declared",
                     declaration.at);
            }
            globals_[declaration.name] = Variable{
                ir::global(static_cast<std::int64_t>(program_.globals.size())),
                declaration.type};
            program_.globals.push_back(ir::Global{declaration.name});
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
            for (const auto& parameter : procedure.parameters) {
                if (parameter.type == VbType::none) {
                    fail("the parameter '" + parameter.name +
                             "' needs an explicit type (Variant is not "
                             "compiled yet)",
                         procedure.at);
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
                    !parameter.by_value || parameter.type == VbType::string});
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
                parameter.type};
            if (parameter.by_value && parameter.type == VbType::string) {
                // The callee owns its copy of a string parameter.
                builder.runtime(Service::string_addref, {ir::slot(index)});
                owned_slots_.push_back(ir::slot(index));
            }
        }
        if (procedure.is_function) {
            const auto result = builder.new_temp();
            function.result_slot = static_cast<int>(result.value);
            function.result_wide = procedure.return_type == VbType::string;
            zero(result, procedure.return_type);
            result_variable_ = Variable{result, procedure.return_type};
        }
        end_label_ = builder.new_label();
        declare_locals_of(procedure.body);
        lower_block(procedure.body);
        builder.place(end_label_);
        for (const auto& slot : owned_slots_) {
            builder.runtime(Service::string_release, {slot});
        }
        builder.ret();
        builder_ = nullptr;
    }

    void zero(const Operand slot, const VbType type) {
        if (type == VbType::string) {
            builder_->move_wide(slot, ir::immediate(0));
        } else {
            builder_->emit(Opcode::move, slot, ir::immediate(0));
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
        const auto slot = builder_->new_temp();
        zero(slot, declaration.type);
        locals_[declaration.name] = Variable{slot, declaration.type};
        if (declaration.type == VbType::string) {
            owned_slots_.push_back(slot);
        }
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
        }
        flush_temps();
    }

    void lower_assignment(const Stmt& statement) {
        const auto target = find_variable(statement.text, statement.at);
        const auto value = lower_expression(*statement.expr);
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
        builder_->emit(Opcode::move, target.operand, converted.operand);
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

    void lower_print(const Stmt& statement) {
        for (const auto& item : statement.items) {
            if (item->kind == ExprKind::string_literal) {
                program_.strings.push_back(item->text);
                builder_->print_text(
                    static_cast<int>(program_.strings.size() - 1U));
                continue;
            }
            const auto value = lower_expression(*item);
            const Service service =
                value.type == VbType::boolean  ? Service::print_boolean
                : value.type == VbType::string ? Service::print_string
                                               : Service::print_number;
            builder_->runtime(service, {value.operand});
        }
        if (statement.newline) {
            builder_->runtime(Service::print_newline, {});
        }
    }

    // A condition is true when its value is not zero. The temporaries of
    // its evaluation are released before the branch.
    [[nodiscard]] Operand lower_condition(const Expr& expression) {
        const auto value = lower_expression(expression);
        if (!is_integral(value.type)) {
            fail("a condition needs a numeric or Boolean value", expression.at);
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
        builder_->place(top);
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
        builder_->place(top);
        if (statement.expr && !statement.test_after) {
            const auto condition = lower_condition(*statement.expr);
            builder_->branch(
                statement.until ? Opcode::branch_nonzero : Opcode::branch_zero,
                condition, done);
        }
        lower_block(statement.body);
        if (statement.expr && statement.test_after) {
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
        builder_->place(top);
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
        builder_->emit(Opcode::add, variable.operand, variable.operand, step,
                       check_for(variable.type));
        builder_->jump(top);
        builder_->place(done);
        loops_.pop_back();
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
            if (name == "cstr" || name == "instr") {
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
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            const auto& parameter = signature.parameters[i];
            const auto& argument = *arguments[i];
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
        builder_->call(result.operand, signature.index, std::move(passed),
                       wide);
        return result;
    }

    // The address argument of a ByRef parameter.
    [[nodiscard]] Operand by_reference(const Expr& argument,
                                       const VbType type) {
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
        } else {
            builder_->emit(Opcode::move, temp,
                           convert(value, type, argument.at).operand);
        }
        return address_of(temp);
    }

    void lower_call_statement(const Stmt& statement) {
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
            } else if (!is_integral(value.type)) {
                fail(std::string(builtin.name) +
                         " needs a numeric argument here",
                     argument.at);
            }
            passed.push_back(value.operand);
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

    // CStr and the two forms of InStr.
    [[nodiscard]] Value lower_special_builtin(
        const std::string& name, const std::vector<ExprPtr>& arguments,
        const Position at) {
        if (name == "cstr") {
            if (arguments.size() != 1U) {
                fail("CStr takes 1 argument", at);
            }
            return to_string_value(lower_expression(*arguments[0]), at);
        }
        if (arguments.size() < 2U || arguments.size() > 3U) {
            fail("InStr takes 2 or 3 arguments", at);
        }
        std::vector<Operand> passed;
        std::size_t next = 0;
        if (arguments.size() == 3U) {
            const auto start = lower_expression(*arguments[0]);
            if (!is_integral(start.type)) {
                fail("InStr needs a numeric start", arguments[0]->at);
            }
            passed.push_back(start.operand);
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

    // A number or Boolean as a String (CStr); a String stays as it is.
    [[nodiscard]] Value to_string_value(const Value& value, const Position at) {
        if (value.type == VbType::string) {
            return value;
        }
        if (!is_integral(value.type)) {
            fail("this value cannot be converted to a String yet", at);
        }
        Value result;
        result.type = VbType::string;
        result.operand = new_owned_string();
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
            case ExprKind::boolean_literal:
                return Value{ir::immediate(expression.number), VbType::boolean};
            case ExprKind::string_literal:
                program_.strings.push_back(expression.text);
                return Value{ir::literal(static_cast<std::int64_t>(
                                 program_.strings.size() - 1)),
                             VbType::string};
            case ExprKind::name: {
                if (!locals_.contains(expression.text) &&
                    !(result_variable_.has_value() &&
                      expression.text == current_->name) &&
                    !globals_.contains(expression.text) &&
                    (signatures_.contains(expression.text) ||
                     find_builtin(expression.text) != nullptr)) {
                    return lower_call(expression.text, {}, expression.at, true);
                }
                const auto variable =
                    find_variable(expression.text, expression.at);
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
                return lower_call(expression.text, expression.operands,
                                  expression.at, true);
            case ExprKind::unary:
                return lower_unary(expression);
            case ExprKind::binary:
                return lower_binary(expression);
        }
        fail("unsupported expression", expression.at);
    }

    [[nodiscard]] Value lower_unary(const Expr& expression) {
        auto operand = lower_expression(*expression.operands[0]);
        if (!is_integral(operand.type)) {
            fail("the operator " + expression.text + " needs a numeric value",
                 expression.at);
        }
        const auto result = builder_->new_temp();
        if (expression.text == "not") {
            builder_->emit(Opcode::bit_not, result, operand.operand);
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

    [[nodiscard]] Value lower_binary(const Expr& expression) {
        const std::string& op = expression.text;
        const auto left = lower_expression(*expression.operands[0]);
        const auto right = lower_expression(*expression.operands[1]);
        const bool is_comparison = op == "=" || op == "<>" || op == "<" ||
                                   op == ">" || op == "<=" || op == ">=";
        if (op == "&") {
            return concatenate(left, right, expression.at);
        }
        if (left.type == VbType::string || right.type == VbType::string) {
            if (op == "+" && left.type == VbType::string &&
                right.type == VbType::string) {
                return concatenate(left, right, expression.at);
            }
            if (is_comparison && left.type == VbType::string &&
                right.type == VbType::string) {
                const auto order = builder_->new_temp();
                builder_->runtime(Service::string_compare,
                                  {left.operand, right.operand}, order);
                const auto result = builder_->new_temp();
                builder_->compare(result, order, ir::immediate(0),
                                  condition_of(op));
                return Value{result, VbType::boolean};
            }
            fail("the operator " + op +
                     " is not compiled for these String "
                     "operands yet",
                 expression.at);
        }
        if (!is_integral(left.type) || !is_integral(right.type)) {
            fail("the operator " + op + " needs numeric values", expression.at);
        }
        const auto result = builder_->new_temp();
        if (is_comparison) {
            builder_->compare(result, left.operand, right.operand,
                              condition_of(op));
            return Value{result, VbType::boolean};
        }
        if (op == "and" || op == "or" || op == "xor") {
            const Opcode code = op == "and"  ? Opcode::bit_and
                                : op == "or" ? Opcode::bit_or
                                             : Opcode::bit_xor;
            builder_->emit(code, result, left.operand, right.operand);
            const VbType type =
                left.type == VbType::boolean && right.type == VbType::boolean
                    ? VbType::boolean
                : (left.type == VbType::long_integer ||
                   right.type == VbType::long_integer)
                    ? VbType::long_integer
                    : VbType::integer;
            return Value{result, type};
        }
        if (op == "/") {
            fail("floating-point division (/) is not compiled yet; use \\",
                 expression.at);
        }
        const VbType type = (left.type == VbType::long_integer ||
                             right.type == VbType::long_integer)
                                ? VbType::long_integer
                                : VbType::integer;
        const Opcode code = op == "+"    ? Opcode::add
                            : op == "-"  ? Opcode::sub
                            : op == "*"  ? Opcode::mul
                            : op == "\\" ? Opcode::divide
                                         : Opcode::modulo;
        builder_->emit(code, result, left.operand, right.operand,
                       check_for(type));
        return Value{result, type};
    }

    // Converts a value for an assignment or a parameter.
    [[nodiscard]] Value convert(const Value& value, const VbType to,
                                const Position at) {
        if (!is_integral(value.type)) {
            fail("this value cannot be converted yet", at);
        }
        if (value.type == to) {
            return value;
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
    std::vector<Operand> owned_slots_;  // string variables released at the end
    std::vector<Operand> pending_;      // string temporaries of a statement
    const Procedure* current_{};
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
