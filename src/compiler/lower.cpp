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

struct LowerFailure final {
    std::string message;
    Position at;
};

[[noreturn]] void fail(const std::string& message, const Position at) {
    throw LowerFailure{message, at};
}

// A value and its VB type.
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
    void runtime(const ir::Service service, std::vector<Operand> args) {
        ir::Instruction instruction;
        instruction.op = Opcode::runtime;
        instruction.target = static_cast<int>(service);
        instruction.args = std::move(args);
        function_.code.push_back(std::move(instruction));
    }
    void print_text(const int string_index) {
        ir::Instruction instruction;
        instruction.op = Opcode::print_text;
        instruction.target = string_index;
        function_.code.push_back(std::move(instruction));
    }
    void call(const Operand dst, const int callee, std::vector<Operand> args) {
        ir::Instruction instruction;
        instruction.op = Opcode::call;
        instruction.dst = dst;
        instruction.target = callee;
        instruction.args = std::move(args);
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

    static void require_scalar(const VbType type, const Position at) {
        if (type == VbType::string) {
            fail("String variables are not compiled yet", at);
        }
    }

    void declare_globals() {
        for (const auto& declaration : module_.globals) {
            require_type(declaration);
            require_scalar(declaration.type, declaration.at);
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
            if (procedure.is_function) {
                if (procedure.return_type == VbType::none) {
                    fail("the Function " + procedure.spelling +
                             " needs an explicit return type (Variant is not "
                             "compiled yet)",
                         procedure.at);
                }
                require_scalar(procedure.return_type, procedure.at);
            }
            for (const auto& parameter : procedure.parameters) {
                if (parameter.type == VbType::none) {
                    fail("the parameter '" + parameter.name +
                             "' needs an explicit type (Variant is not "
                             "compiled yet)",
                         procedure.at);
                }
                require_scalar(parameter.type, procedure.at);
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
                function.parameters.push_back(
                    ir::Parameter{parameter.by_value});
            }
            program_.functions.push_back(std::move(function));
        }
    }

    // ----- Procedures ------------------------------------------------------

    struct Loop final {
        std::string kind;  // "for" or "do"
        int exit_label{};
    };

    void lower_procedure(const Procedure& procedure) {
        const auto& signature = signatures_.at(procedure.name);
        auto& function =
            program_.functions[static_cast<std::size_t>(signature.index)];
        FunctionBuilder builder(function);
        builder_ = &builder;
        locals_.clear();
        loops_.clear();
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
        }
        if (procedure.is_function) {
            const auto result = builder.new_temp();
            function.result_slot = static_cast<int>(result.value);
            builder.emit(Opcode::move, result, ir::immediate(0));
            result_variable_ = Variable{result, procedure.return_type};
        }
        end_label_ = builder.new_label();
        lower_block(procedure.body);
        builder.place(end_label_);
        builder.ret();
        builder_ = nullptr;
    }

    void declare_local(const Declaration& declaration) {
        require_type(declaration);
        require_scalar(declaration.type, declaration.at);
        if (locals_.contains(declaration.name) ||
            (result_variable_.has_value() &&
             declaration.name == current_->name)) {
            fail("'" + declaration.name + "' is already declared",
                 declaration.at);
        }
        const auto slot = builder_->new_temp();
        // A local starts at zero.
        builder_->emit(Opcode::move, slot, ir::immediate(0));
        locals_[declaration.name] = Variable{slot, declaration.type};
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

    // ----- Statements ------------------------------------------------------

    void lower_block(const std::vector<StmtPtr>& statements) {
        for (const auto& statement : statements) {
            lower_statement(*statement);
        }
    }

    void lower_statement(const Stmt& statement) {
        switch (statement.kind) {
            case StmtKind::dim:
                for (const auto& declaration : statement.declarations) {
                    declare_local(declaration);
                }
                break;
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
    }

    void lower_assignment(const Stmt& statement) {
        const auto target = find_variable(statement.text, statement.at);
        const auto value = lower_expression(*statement.expr);
        const auto converted = convert(value, target.type, statement.at);
        builder_->emit(Opcode::move, target.operand, converted.operand);
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
            builder_->runtime(value.type == VbType::boolean
                                  ? ir::Service::print_boolean
                                  : ir::Service::print_number,
                              {value.operand});
        }
        if (statement.newline) {
            builder_->runtime(ir::Service::print_newline, {});
        }
    }

    // A condition is true when its value is not zero.
    [[nodiscard]] Operand lower_condition(const Expr& expression) {
        const auto value = lower_expression(expression);
        if (!is_integral(value.type)) {
            fail("a condition needs a numeric or Boolean value", expression.at);
        }
        return value.operand;
    }

    void lower_if(const Stmt& statement) {
        const int end = builder_->new_label();
        for (std::size_t i = 0; i < statement.conditions.size(); ++i) {
            const int next = builder_->new_label();
            builder_->branch(Opcode::branch_zero,
                             lower_condition(*statement.conditions[i]), next);
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
        builder_->branch(Opcode::branch_zero, lower_condition(*statement.expr),
                         done);
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
            builder_->branch(
                statement.until ? Opcode::branch_nonzero : Opcode::branch_zero,
                lower_condition(*statement.expr), done);
        }
        lower_block(statement.body);
        if (statement.expr && statement.test_after) {
            // Loop While repeats while true; Loop Until repeats while false.
            builder_->branch(
                statement.until ? Opcode::branch_zero : Opcode::branch_nonzero,
                lower_condition(*statement.expr), top);
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
            builder_->runtime(ir::Service::end_program, {});
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

    // Lowers a call; the returned operand holds the function's value (an
    // unused temporary for a Sub).
    [[nodiscard]] Value lower_call(const std::string& name,
                                   const std::vector<ExprPtr>& arguments,
                                   const Position at, const bool want_value) {
        const auto found = signatures_.find(name);
        if (found == signatures_.end()) {
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
                const auto value = convert(lower_expression(argument),
                                           parameter.type, argument.at);
                passed.push_back(value.operand);
                continue;
            }
            passed.push_back(by_reference(argument, parameter.type));
        }
        Value result;
        result.type = signature.return_type;
        result.operand = builder_->new_temp();
        builder_->call(result.operand, signature.index, std::move(passed));
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
            if (variable.operand.kind == OperandKind::indirect) {
                // Already a pointer: pass it on.
                return ir::slot(variable.operand.value);
            }
            const auto pointer = builder_->new_temp();
            builder_->emit(Opcode::address, pointer, variable.operand);
            return pointer;
        }
        // An expression is passed by reference through a temporary.
        const auto value =
            convert(lower_expression(argument), type, argument.at);
        const auto temp = builder_->new_temp();
        builder_->emit(Opcode::move, temp, value.operand);
        const auto pointer = builder_->new_temp();
        builder_->emit(Opcode::address, pointer, temp);
        return pointer;
    }

    void lower_call_statement(const Stmt& statement) {
        static_cast<void>(
            lower_call(statement.text, statement.items, statement.at, false));
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
                fail("String values are only compiled as Print items so far",
                     expression.at);
            case ExprKind::name: {
                if (!locals_.contains(expression.text) &&
                    !(result_variable_.has_value() &&
                      expression.text == current_->name) &&
                    !globals_.contains(expression.text) &&
                    signatures_.contains(expression.text)) {
                    return lower_call(expression.text, {}, expression.at, true);
                }
                const auto variable =
                    find_variable(expression.text, expression.at);
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

    [[nodiscard]] Value lower_binary(const Expr& expression) {
        const std::string& op = expression.text;
        if (op == "&") {
            fail("String concatenation is not compiled yet", expression.at);
        }
        const auto left = lower_expression(*expression.operands[0]);
        const auto right = lower_expression(*expression.operands[1]);
        if (!is_integral(left.type) || !is_integral(right.type)) {
            fail("the operator " + op + " needs numeric values", expression.at);
        }
        const auto result = builder_->new_temp();
        if (op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" ||
            op == ">=") {
            const Condition condition = op == "="    ? Condition::equal
                                        : op == "<>" ? Condition::not_equal
                                        : op == "<"  ? Condition::less
                                        : op == ">"  ? Condition::greater
                                        : op == "<=" ? Condition::less_equal
                                                     : Condition::greater_equal;
            builder_->compare(result, left.operand, right.operand, condition);
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
            if (value.operand.kind == OperandKind::immediate) {
                if (value.operand.value < -32768 ||
                    value.operand.value > 32767) {
                    // Overflow is a run-time error, as in VB6.
                    const auto result = builder_->new_temp();
                    builder_->emit(Opcode::narrow, result, value.operand);
                    return Value{result, VbType::integer};
                }
                return Value{value.operand, VbType::integer};
            }
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
