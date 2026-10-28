// The value-level face of the interpreter for native programs (ADR-0009).

#include "native_engine.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <set>
#include <unordered_set>

#include "interpreter.hpp"

namespace wfc::detail {

// A friend of `Interpreter`: the operators and conversions are its private
// members.
struct NativeSupport {
    // What a statement run for compiled code leaves behind: it can run while
    // another one is still being parsed (a Timer event raised inside a
    // property being set calls compiled code, which runs more statements), so
    // the position of the first and its arguments are put back afterwards.
    class Frame final {
    public:
        explicit Frame(Interpreter& in)
            : in_(in),
              source_(in.source_),
              main_source_(in.main_source_),
              data_(in.error_source_data_),
              offset_(in.offset_),
              execute_(in.execute_),
              print_surface_(in.print_surface_),
              error_(std::move(in.error_)) {
            in.error_ = wfc::Evaluation{};
        }
        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;
        ~Frame() {
            for (std::size_t i = 0; i < arguments_.size(); ++i) {
                const std::string name = "wfcarg" + std::to_string(i);
                auto& scope = in_.module_scope();
                scope.variables.erase(name);
                scope.variant_variables.erase(name);
                if (arguments_[i].has_value()) {
                    scope.variables[name] = std::move(*arguments_[i]);
                    if (was_variant_[i]) {
                        scope.variant_variables.insert(name);
                    }
                }
            }
            in_.source_ = source_;
            in_.main_source_ = main_source_;
            in_.error_source_data_ = data_;
            in_.offset_ = offset_;
            in_.execute_ = execute_;
            in_.print_surface_ = print_surface_;
            in_.error_ = std::move(error_);
        }
        // Remembers the arguments the statement is about to replace.
        void keep_arguments(const std::size_t count) {
            auto& scope = in_.module_scope();
            for (std::size_t i = 0; i < count; ++i) {
                const std::string name = "wfcarg" + std::to_string(i);
                const auto found = scope.variables.find(name);
                if (found != scope.variables.end()) {
                    arguments_.emplace_back(found->second);
                } else {
                    arguments_.emplace_back(std::nullopt);
                }
                was_variant_.push_back(scope.variant_variables.contains(name));
            }
        }

    private:
        Interpreter& in_;
        std::string_view source_;
        std::string_view main_source_;
        const char* data_;
        std::size_t offset_;
        bool execute_;
        InstanceData* print_surface_;
        wfc::Evaluation error_;
        std::vector<std::optional<Value>> arguments_;
        std::vector<bool> was_variant_;
    };

    [[nodiscard]] static bool trace_enabled() {
        char* trace = nullptr;
        std::size_t trace_size = 0;
        const bool on =
            _dupenv_s(&trace, &trace_size, "WFC_NATIVE_TRACE") == 0 &&
            trace != nullptr;
        std::free(trace);
        return on;
    }

    static void take_error(Interpreter& in, NativeError& error) {
        if (trace_enabled()) {
            std::fprintf(stderr, "[engine error] %s in [%.60s]\n",
                         in.error_.diagnostic.c_str(),
                         std::string(in.source_).c_str());
        }
        error.number = static_cast<int>(in.runtime_error_number());
        if (error.number == 0) {
            error.number = 5;  // an error with no Visual Basic number
        }
        error.description =
            in.err_number_ == error.number && !in.err_description_.empty()
                ? in.err_description_
                : Interpreter::vb_error_description(
                      static_cast<Integer>(error.number));
        if (in.err_number_ == error.number) {
            error.source = in.err_source_;
        }
        in.error_ = wfc::Evaluation{};
    }

    static bool is_null(const Value& value) {
        return std::holds_alternative<Null>(value);
    }

    static std::optional<Value> binary(Interpreter& in, std::string_view op,
                                       const Value& left, const Value& right,
                                       const bool left_variant,
                                       const bool right_variant,
                                       NativeError& error) {
        std::optional<Value> result;
        // An operand is a Variant: a result that overflows promotes instead.
        in.variant_operand_seen_ = true;
        const auto holds_number = [](const Value& value) {
            return is_number(value) &&
                   !std::holds_alternative<DateValue>(value);
        };
        in.variant_string_seen_ =
            (left_variant && std::holds_alternative<std::string>(left)) ||
            (right_variant && std::holds_alternative<std::string>(right));
        in.variant_number_seen_ = (left_variant && holds_number(left)) ||
                                  (right_variant && holds_number(right));
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            result = in.numeric_binary(left, right, op.front(), 0U);
        } else if (op == "\\") {
            result = in.integer_binary(left, right, '\\', 0U);
        } else if (op == "mod") {
            result = in.integer_binary(left, right, '%', 0U);
        } else if (op == "=" || op == "<>" || op == "<" || op == ">" ||
                   op == "<=" || op == ">=") {
            result = in.compare(left, right, op, 0U);
        } else if (op == "and") {
            result = in.logical_binary(left, right, 'A', 0U);
        } else if (op == "or") {
            result = in.logical_binary(left, right, 'O', 0U);
        } else if (op == "xor") {
            result = in.logical_binary(left, right, 'X', 0U);
        } else if (op == "eqv") {
            result = in.logical_binary(left, right, 'E', 0U);
        } else if (op == "imp") {
            result = in.logical_binary(left, right, 'I', 0U);
        } else if (op == "&") {
            result = concatenate(in, left, right);
        } else if (op == "^") {
            result = power(in, left, right);
        } else {
            in.set_error("WFC0010", "unknown operator", 0U);
        }
        in.variant_operand_seen_ = false;
        in.variant_string_seen_ = false;
        in.variant_number_seen_ = false;
        if (!result.has_value()) {
            take_error(in, error);
        }
        return result;
    }

    // `&`: two Nulls are an error, one Null is an empty String.
    static std::optional<Value> concatenate(Interpreter& in, const Value& left,
                                            const Value& right) {
        const bool left_null = is_null(left);
        const bool right_null = is_null(right);
        if (left_null && right_null) {
            in.set_error("WFC0104", "Invalid use of Null", 0U);
            return std::nullopt;
        }
        Value a = left;
        Value b = right;
        if (!in.resolve_default_value(a, 0U) ||
            !in.resolve_default_value(b, 0U)) {
            return std::nullopt;
        }
        if (is_object_reference(a) || is_object_reference(b) ||
            std::holds_alternative<ArrayValue>(a) ||
            std::holds_alternative<ArrayValue>(b)) {
            in.set_error("WFC0020",
                         "concatenation requires String or Long operands", 0U);
            return std::nullopt;
        }
        return Value{(left_null ? std::string{} : NativeSupport::render(a)) +
                     (right_null ? std::string{} : NativeSupport::render(b))};
    }

    static std::string render(const Value& value) {
        return Interpreter::render(value);
    }

    static std::optional<double> number_of(const Value& value) {
        if (std::holds_alternative<Empty>(value)) {
            return 0.0;
        }
        if (is_number(value)) {
            return as_double(value);
        }
        if (const auto* date = std::get_if<DateValue>(&value)) {
            return date->serial;
        }
        if (const auto* flag = std::get_if<bool>(&value)) {
            return *flag ? -1.0 : 0.0;
        }
        if (const auto* text = std::get_if<std::string>(&value)) {
            const auto parsed = parse_numeric_string(*text);
            if (parsed.status == NumericStringStatus::valid) {
                return parsed.value;
            }
        }
        return std::nullopt;
    }

    static std::optional<Value> power(Interpreter& in, const Value& left,
                                      const Value& right) {
        if (is_null(left) || is_null(right)) {
            return Value{Null{}};
        }
        const auto base = number_of(left);
        const auto exponent = number_of(right);
        if (!base.has_value() || !exponent.has_value()) {
            in.set_error("WFC0007", "operator requires numeric operands", 0U);
            return std::nullopt;
        }
        if (*base == 0.0 && *exponent < 0.0) {
            in.set_error("WFC0008", "division by zero", 0U);
            return std::nullopt;
        }
        const double result = std::pow(*base, *exponent);
        if (!std::isfinite(result)) {
            in.set_error("WFC0009", "numeric overflow", 0U);
            return std::nullopt;
        }
        return Value{result};
    }

    static std::optional<Value> negate(Interpreter& in, Value value,
                                       NativeError& error) {
        std::optional<Value> result;
        if (const auto* flag = std::get_if<bool>(&value)) {
            value = Value{static_cast<Int16>(*flag ? -1 : 0)};
        }
        if (const auto* byte = std::get_if<Byte>(&value)) {
            value = Value{static_cast<Int16>(*byte)};
        }
        if (is_null(value)) {
            return value;
        }
        if (std::holds_alternative<Empty>(value)) {
            return Value{Integer{0}};
        }
        if (const auto* number = std::get_if<double>(&value)) {
            return Value{-*number};
        }
        if (const auto* single = std::get_if<float>(&value)) {
            return Value{-*single};
        }
        if (const auto* decimal = std::get_if<Decimal>(&value)) {
            Decimal negated = *decimal;
            if (!is_zero_big(negated.mantissa)) {
                negated.negative = !negated.negative;
            }
            return Value{negated};
        }
        if (const auto* currency = std::get_if<Currency>(&value)) {
            if (currency->scaled == std::numeric_limits<std::int64_t>::min()) {
                in.set_error("WFC0009", "numeric overflow", 0U);
                take_error(in, error);
                return std::nullopt;
            }
            return Value{Currency{-currency->scaled}};
        }
        if (const auto* short_integer = std::get_if<Int16>(&value)) {
            if (*short_integer == std::numeric_limits<Int16>::min()) {
                in.set_error("WFC0009", "integer overflow", 0U);
                take_error(in, error);
                return std::nullopt;
            }
            return Value{static_cast<Int16>(-*short_integer)};
        }
        const auto* integer = in.require_integer(value, 0U);
        if (integer == nullptr) {
            take_error(in, error);
            return std::nullopt;
        }
        if (*integer == std::numeric_limits<Integer>::min()) {
            in.set_error("WFC0009", "integer overflow", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        return Value{static_cast<Integer>(-*integer)};
    }

    static std::optional<Value> logical_not(Interpreter& in, const Value& value,
                                            NativeError& error) {
        if (const auto* integer = std::get_if<Integer>(&value)) {
            return Value{static_cast<Integer>(~*integer)};
        }
        if (const auto* short_integer = std::get_if<Int16>(&value)) {
            return Value{static_cast<Int16>(~*short_integer)};
        }
        if (const auto* byte = std::get_if<Byte>(&value)) {
            return Value{static_cast<Byte>(~*byte)};
        }
        if (!std::holds_alternative<bool>(value)) {
            Value widened = value;
            if (!in.widen_bitwise_operand(widened, 0U)) {
                take_error(in, error);
                return std::nullopt;
            }
            if (const auto* integer = std::get_if<Integer>(&widened)) {
                return Value{static_cast<Integer>(~*integer)};
            }
        }
        const auto operand = in.coerce_ternary_operand(value, 0U);
        if (!operand.has_value()) {
            take_error(in, error);
            return std::nullopt;
        }
        if (operand->is_null) {
            return Value{Null{}};
        }
        return Value{!operand->value};
    }

    static std::optional<std::int32_t> to_long(Interpreter& in,
                                               const Value& value,
                                               NativeError& error) {
        if (is_null(value)) {
            in.set_error("WFC0104", "Invalid use of Null", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        const auto converted = in.coerce_long(value, 0U);
        if (!converted.has_value()) {
            if (in.runtime_error_number() == 0) {
                in.set_error("WFC0007", "Type mismatch", 0U);
            }
            take_error(in, error);
            return std::nullopt;
        }
        return static_cast<std::int32_t>(*converted);
    }

    static std::optional<double> to_double(Interpreter& in, const Value& value,
                                           NativeError& error) {
        if (is_null(value)) {
            in.set_error("WFC0104", "Invalid use of Null", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        const auto number = number_of(value);
        if (!number.has_value()) {
            in.set_error("WFC0007", "Type mismatch", 0U);
            take_error(in, error);
        }
        return number;
    }

    static std::optional<std::string> to_text(Interpreter& in,
                                              const Value& value,
                                              NativeError& error) {
        if (is_null(value)) {
            in.set_error("WFC0104", "Invalid use of Null", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        Value resolved = value;
        if (!in.resolve_default_value(resolved, 0U)) {
            take_error(in, error);
            return std::nullopt;
        }
        if (is_object_reference(resolved) ||
            std::holds_alternative<Nothing>(resolved) ||
            std::holds_alternative<ArrayValue>(resolved)) {
            in.set_error("WFC0007", "Type mismatch", 0U);
            take_error(in, error);
            return std::nullopt;
        }
        return NativeSupport::render(resolved);
    }

    static std::optional<bool> to_boolean(Interpreter& in, const Value& value,
                                          NativeError& error) {
        if (is_null(value)) {
            return false;
        }
        const auto flag =
            in.coerce_condition_boolean(value, 0U, "WFC0007", "Type mismatch");
        if (!flag.has_value()) {
            take_error(in, error);
        }
        return flag;
    }

    static std::string lower(std::string text) {
        for (auto& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text;
    }

    static void fail(Interpreter& in, const int number, NativeError& error) {
        if (trace_enabled()) {
            std::fprintf(stderr, "[engine fail] %d\n", number);
        }
        error.number = number;
        error.description =
            Interpreter::vb_error_description(static_cast<Integer>(number));
        static_cast<void>(in);
    }

    static bool start(std::unique_ptr<Interpreter>& interpreter,
                      EngineSetup setup, NativeError& error) {
        static EngineSetup kept;  // the interpreter reads its sources later
        kept = std::move(setup);
        interpreter = std::make_unique<Interpreter>(kept.program, kept.classes);
        auto& in = *interpreter;
        in.native_scan_text_ = kept.scan_text;
        const auto& options = kept.options;
        in.set_vb_number_spacing(true);
        in.set_app_properties(options.app_properties);
        in.set_resource_file(options.resource_file);
        in.set_type_libraries(options.type_libraries);
        in.set_startup_form(options.startup_form);
        in.set_ui_script(options.ui_script);
        in.set_exit_after(options.exit_after_ms);
        in.set_ui_mode(options.ui_mode);
        in.set_fit_screen(options.fit_screen, options.fit_width,
                          options.fit_height, options.fit_screen_size);
        in.set_screenshot_directory(options.screenshot_directory);
        if (!in.initialize_program()) {
            take_error(in, error);
            return false;
        }
        return true;
    }

    static std::optional<Value> global(Interpreter& in, const std::string& name,
                                       NativeError& error) {
        const auto found = in.find_variable(lower(name));
        if (found.value == nullptr) {
            if (!in.error_.diagnostic.empty()) {
                take_error(in, error);
            } else {
                fail(in, 424, error);
            }
            return std::nullopt;
        }
        return *found.value;
    }

    // Binds the values to wfcarg0.. in the module scope while `body` runs.
    template <typename Body>
    static auto with_arguments(Interpreter& in, std::vector<Value>& values,
                               Body&& body) {
        auto& scope = in.module_scope();
        Frame frame(in);
        frame.keep_arguments(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            const std::string name = "wfcarg" + std::to_string(i);
            scope.variables[name] = values[i];
            if (std::holds_alternative<Empty>(values[i])) {
                scope.variant_variables.insert(name);
            }
        }
        auto result = body();
        for (std::size_t i = 0; i < values.size(); ++i) {
            const std::string name = "wfcarg" + std::to_string(i);
            if (const auto found = scope.variables.find(name);
                found != scope.variables.end()) {
                values[i] = found->second;  // a by-reference argument
            }
            scope.variables.erase(name);
            scope.variant_variables.erase(name);
        }
        return result;
    }

    // The arguments of the next member call that were left out (bit i).
    static inline std::uint32_t omitted_ = 0;

    // `name(wfcarg1, wfcarg2, ...)`: the member and its arguments as source.
    static std::string member_text(const std::string& name,
                                   const std::size_t count,
                                   const std::size_t first) {
        std::string text = name;
        if (count > 0) {
            text += '(';
            for (std::size_t i = 0; i < count; ++i) {
                if (i > 0) {
                    text += ", ";
                }
                if ((omitted_ >> i & 1U) == 0) {
                    text += "wfcarg" + std::to_string(first + i);
                }
            }
            text += ')';
        }
        omitted_ = 0;
        return text;
    }

    static std::optional<Value> member_get(Interpreter& in, const Value& object,
                                           const std::string& name,
                                           std::vector<Value>& arguments,
                                           NativeError& error) {
        if (!std::holds_alternative<ObjectInstance>(object)) {
            fail(in,
                 std::holds_alternative<Nothing>(object) ||
                         std::holds_alternative<Empty>(object)
                     ? 91
                     : 424,
                 error);
            return std::nullopt;
        }
        static std::unordered_set<std::string> texts;
        const std::string& text =
            *texts.insert(member_text(name, arguments.size(), 1)).first;
        if (trace_enabled()) {
            std::fprintf(stderr, "[engine member] %s\n", text.c_str());
        }
        std::vector<Value> bound{object};
        bound.insert(bound.end(), arguments.begin(), arguments.end());
        auto result = with_arguments(in, bound, [&]() -> std::optional<Value> {
            in.source_ = text;
            in.main_source_ = text;
            in.error_source_data_ = text.data();
            in.offset_ = 0;
            in.error_ = wfc::Evaluation{};
            in.execute_ = true;
            auto found = in.parse_member_access_after_dot(object, 0, false);
            if (!found.has_value()) {
                take_error(in, error);
            }
            return found;
        });
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            arguments[i] = bound[i + 1];  // by reference: the call changed it
        }
        if (trace_enabled()) {
            std::fprintf(stderr, "    -> %s\n",
                         result.has_value() ? type_name(in, *result).c_str()
                                            : "(error)");
        }
        return result;
    }

    static bool member_put(Interpreter& in, const Value& object,
                           const std::string& name,
                           const std::vector<Value>& arguments,
                           const Value& value, const bool is_set,
                           NativeError& error) {
        if (!std::holds_alternative<ObjectInstance>(object) &&
            trace_enabled()) {
            std::fprintf(stderr, "[engine put] %s on a %s", name.c_str(),
                         type_name(in, object).c_str());
            std::fputc(10, stderr);
        }
        if (!std::holds_alternative<ObjectInstance>(object)) {
            fail(in,
                 std::holds_alternative<Nothing>(object) ||
                         std::holds_alternative<Empty>(object)
                     ? 91
                     : 424,
                 error);
            return false;
        }
        std::vector<Value> bound{object, value};
        bound.insert(bound.end(), arguments.begin(), arguments.end());
        const std::string text =
            std::string(is_set ? "Set " : "") + "wfcarg0." +
            member_text(name, arguments.size(), 2) + " = wfcarg1";
        // Both are Variants: the statement assigns what they hold.
        return run_statement(in, text, bound, 0x3U, error);
    }

    static std::optional<Value> new_form(Interpreter& in,
                                         const std::string& name,
                                         NativeError& error) {
        in.execute_ = true;
        if (!in.class_definitions_.contains(lower(name))) {
            // Not a class of the project: a COM class by its name.
            return call_function(in, "createobject", {Value{name}}, error);
        }
        auto made = in.instantiate_class(lower(name), 0);
        if (!made.has_value()) {
            take_error(in, error);
        }
        return made;
    }

    static std::optional<std::vector<Value>> items(Interpreter& in,
                                                   const Value& object,
                                                   NativeError& error) {
        if (const auto* array = std::get_if<ArrayValue>(&object)) {
            return array->elements;  // a Variant that holds an array
        }
        const auto* instance = std::get_if<ObjectInstance>(&object);
        if (instance != nullptr && instance->data->com != nullptr) {
            // An object of COM lists its items itself.
            in.execute_ = true;
            auto elements = in.com_enumerate(*instance->data, 0);
            if (!elements.has_value()) {
                take_error(in, error);
                return std::nullopt;
            }
            return elements->elements;
        }
        if (instance != nullptr && instance->data->native == nullptr) {
            // A class written in Visual Basic lists its items itself (a
            // Collection of the interpreter).
            std::vector<Value> none;
            auto listed = member_get(in, object, "wfcitems", none, error);
            if (listed.has_value()) {
                if (const auto* array = std::get_if<ArrayValue>(&*listed)) {
                    return array->elements;
                }
            }
            return std::nullopt;
        }
        if (instance == nullptr || instance->data->native == nullptr) {
            fail(in, 438, error);
            return std::nullopt;
        }
        std::vector<Value> list;
        if (!instance->data->native->enumerate(in, list)) {
            fail(in, 438, error);
            return std::nullopt;
        }
        return list;
    }

    static bool run_forms(Interpreter& in, NativeError& error) {
        in.execute_ = true;
        if (!in.run_forms_at_exit()) {
            take_error(in, error);
            return false;
        }
        return true;
    }

    static bool run_statement(Interpreter& in, const std::string& text,
                              std::vector<Value>& values,
                              const std::uint32_t variant_mask,
                              NativeError& error) {
        if (text == "wfcoptionbase") {
            in.option_base_one_ = true;
            return true;
        }
        if (text == "wfcoptioncompare") {
            in.option_compare_text_ = true;
            return true;
        }
        // The interpreter keeps pointers into the text it runs, so every
        // statement gets text of its own.
        static std::unordered_set<std::string> texts;
        const std::string& statement = *texts.insert(text).first;
        char* trace = nullptr;
        std::size_t trace_size = 0;
        const bool tracing =
            _dupenv_s(&trace, &trace_size, "WFC_NATIVE_TRACE") == 0 &&
            trace != nullptr;
        std::free(trace);
        if (tracing) {
            std::fprintf(stderr, "[engine statement] %s\n", text.c_str());
            for (const auto& value : values) {
                const std::string shown =
                    std::holds_alternative<ObjectInstance>(value)
                        ? "object " + type_name(in, value)
                        : render(value);
                std::fprintf(stderr, "    [%s]\n", shown.c_str());
            }
        }
        auto& scope = in.module_scope();
        Frame frame(in);
        frame.keep_arguments(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            const std::string name = "wfcarg" + std::to_string(i);
            scope.variables[name] = values[i];
            if (std::holds_alternative<Empty>(values[i]) ||
                ((variant_mask >> i) & 1U) != 0U) {
                scope.variant_variables.insert(name);
            }
        }
        // "@Statement ...": argument 0 is the form whose code it is, so the
        // names of its controls are in scope.
        std::string_view code = statement;
        InstanceData* self = nullptr;
        if (!code.empty() && code.front() == '@') {
            code.remove_prefix(1);
            if (!values.empty()) {
                if (const auto* form =
                        std::get_if<ObjectInstance>(&values[0])) {
                    self = form->data.get();
                }
            }
        }
        if (self != nullptr) {
            in.instance_scopes_.push_back(self);
        }
        const auto attempt = [&](const std::string_view text_to_run) {
            in.source_ = text_to_run;
            in.main_source_ = text_to_run;
            in.error_source_data_ = text_to_run.data();
            in.offset_ = 0;
            in.error_ = wfc::Evaluation{};
            in.execute_ = true;
            in.print_surface_ = nullptr;  // a Print of an earlier statement
            return in.parse_statement();
        };
        bool ok = false;
        if (!code.empty() && code.front() == '?') {
            // "?target = value": the value is an object when it is one (Set),
            // else it is the value of a Let.
            code.remove_prefix(1);
            const std::string& object_form =
                *texts.insert("Set " + std::string(code)).first;
            ok = attempt(object_form);
            if (!ok) {
                NativeError scratch;
                take_error(in, scratch);
                ok = attempt(code);
            }
        } else {
            ok = attempt(code);
        }
        if (!ok && values.size() > 1 &&
            std::holds_alternative<ObjectInstance>(values[1])) {
            // `obj(i)` on a class whose default member is Item (Collection).
            const auto where = code.find("wfcarg1(");
            if (where != std::string_view::npos) {
                NativeError scratch;
                take_error(in, scratch);
                std::string rewritten(code);
                rewritten.replace(where, 8, "wfcarg1.Item(");
                const std::string& plain = *texts.insert(rewritten).first;
                const std::string& as_object =
                    *texts.insert("Set " + rewritten).first;
                ok = attempt(as_object);
                if (!ok) {
                    take_error(in, scratch);
                    ok = attempt(plain);
                }
            }
        }
        if (self != nullptr) {
            in.instance_scopes_.pop_back();
        }
        if (ok) {
            for (std::size_t i = 0; i < values.size(); ++i) {
                const std::string name = "wfcarg" + std::to_string(i);
                if (const auto found = scope.variables.find(name);
                    found != scope.variables.end()) {
                    values[i] = found->second;
                }
            }
            if (tracing && !values.empty()) {
                std::fprintf(stderr, "    => %s",
                             type_name(in, values[0]).c_str());
                std::fputc(10, stderr);
            }
        } else {
            take_error(in, error);
        }
        for (std::size_t i = 0; i < values.size(); ++i) {
            const std::string name = "wfcarg" + std::to_string(i);
            scope.variables.erase(name);
            scope.variant_variables.erase(name);
        }
        return ok;
    }

    static std::string take_output(Interpreter& in, bool& line_open) {
        line_open = in.has_output_line_;
        return std::exchange(in.output_, {});
    }

    static void close_output_line(Interpreter& in) {
        in.has_output_line_ = false;
        in.output_line_open_ = false;
    }

    static bool like(const std::string& text, const std::string& pattern,
                     const bool fold) {
        return Interpreter::like_match(text, pattern, fold);
    }

    static bool resolve_default(Interpreter& in, Value& value) {
        return in.resolve_default_value(value, 0U);
    }

    static void fetch_error(Interpreter& in, NativeError& error) {
        take_error(in, error);
    }

    static void attach_events(Interpreter& in, const Value& source,
                              NativeEngine::EventHandler handler) {
        const auto* instance = std::get_if<ObjectInstance>(&source);
        if (instance == nullptr || instance->data == nullptr) {
            return;
        }
        auto data = instance->data;
        if (!handler) {
            data->external_sink = nullptr;
            return;
        }
        data->external_sink = [handler](const std::string& name,
                                        const std::vector<CallArgument>& list) {
            std::vector<Value> values;
            for (const auto& argument : list) {
                values.push_back(argument.value);
            }
            handler(name, values);
            for (std::size_t i = 0; i < list.size(); ++i) {
                if (list[i].byref_target != nullptr) {
                    *list[i].byref_target = values[i];
                }
            }
        };
        if (data->com != nullptr) {
            in.execute_ = true;
            static_cast<void>(in.com_connect_events(*data));
        }
    }

    static bool text_compare(const Interpreter& in) {
        return in.option_compare_text_;
    }

    static void randomize(Interpreter& in, const bool has_seed, double seed,
                          const bool is_timer) {
        if (!has_seed) {
            in.rnd_state_ = in.seed_from_number(in.entropy_seed());
            return;
        }
        if (is_timer) {
            if (const auto fixed = in.fixed_random_seed()) {
                seed = *fixed;  // `Randomize Timer` replays under a test
            }
        }
        in.rnd_state_ = in.seed_from_number(seed);
    }

    static std::optional<Value> call_function(
        Interpreter& in, const std::string_view name,
        const std::vector<Value>& arguments, NativeError& error) {
        std::vector<Value> preset = arguments;
        in.preset_arguments_ = &preset;
        auto result = in.parse_function_call(name, 0U);
        in.preset_arguments_ = nullptr;
        if (!result.has_value()) {
            take_error(in, error);
        }
        return result;
    }

    static bool is_library_function(const std::string_view name) {
        static const std::set<std::string, std::less<>> names = {
            "len",       "lenb",     "lcase",    "lcase$",     "ucase",
            "ucase$",    "ltrim",    "ltrim$",   "rtrim",      "rtrim$",
            "trim",      "trim$",    "left",     "left$",      "leftb",
            "leftb$",    "right",    "right$",   "rightb",     "rightb$",
            "mid",       "mid$",     "midb",     "midb$",      "asc",
            "ascb",      "ascw",     "chrb",     "chrb$",      "chr",
            "chr$",      "chrw",     "chrw$",    "strreverse", "space",
            "space$",    "string",   "string$",  "instr",      "instrb",
            "instrrev",  "strcomp",  "replace",  "hex",        "hex$",
            "oct",       "oct$",     "str",      "str$",       "val",
            "abs",       "sgn",      "cstr",     "clng",       "cbool",
            "cbyte",     "cint",     "cdbl",     "csng",       "ccur",
            "cvar",      "cdec",     "macid",    "error",      "error$",
            "isnumeric", "typename", "vartype",  "iif",        "choose",
            "switch",    "int",      "fix",      "round",      "sqr",
            "sin",       "cos",      "tan",      "atn",        "exp",
            "log",       "isarray",  "isobject", "isnull",     "isempty",
            "iserror",   "lbound",   "ubound",   "qbcolor",    "rgb",
            "strconv",   "format",   "format$",  "rnd",        "array",
            "split",     "join",     "filter"};
        return names.contains(name) ||
               Interpreter::is_date_function_name(name) ||
               Interpreter::is_file_function_name(name) ||
               Interpreter::is_misc_function_name(name);
    }

    static std::string type_name(Interpreter& in, const Value& value) {
        if (std::holds_alternative<Integer>(value)) {
            return "Long";
        }
        if (std::holds_alternative<Int16>(value)) {
            return "Integer";
        }
        if (std::holds_alternative<double>(value)) {
            return "Double";
        }
        if (std::holds_alternative<float>(value)) {
            return "Single";
        }
        if (std::holds_alternative<Currency>(value)) {
            return "Currency";
        }
        if (std::holds_alternative<Decimal>(value)) {
            return "Decimal";
        }
        if (std::holds_alternative<Byte>(value)) {
            return "Byte";
        }
        if (std::holds_alternative<bool>(value)) {
            return "Boolean";
        }
        if (std::holds_alternative<DateValue>(value)) {
            return "Date";
        }
        if (std::holds_alternative<std::string>(value)) {
            return "String";
        }
        if (std::holds_alternative<Empty>(value)) {
            return "Empty";
        }
        if (std::holds_alternative<Null>(value)) {
            return "Null";
        }
        if (std::holds_alternative<ErrorValue>(value)) {
            return "Error";
        }
        if (std::holds_alternative<Nothing>(value)) {
            return "Nothing";
        }
        if (std::holds_alternative<ObjectInstance>(value) ||
            std::holds_alternative<ArrayValue>(value)) {
            NativeError ignored;
            if (const auto name =
                    call_function(in, "typename", {value}, ignored)) {
                return render(*name);
            }
        }
        return "Variant()";
    }
};

NativeEngine::NativeEngine()
    : interpreter_(std::make_unique<Interpreter>(std::string_view{})) {
    interpreter_->set_vb_number_spacing(true);
}

NativeEngine::~NativeEngine() = default;

std::optional<Value> NativeEngine::binary(const std::string_view op,
                                          const Value& left, const Value& right,
                                          const bool left_variant,
                                          const bool right_variant,
                                          NativeError& error) {
    return NativeSupport::binary(*interpreter_, op, left, right, left_variant,
                                 right_variant, error);
}

std::optional<Value> NativeEngine::negate(const Value& value,
                                          NativeError& error) {
    return NativeSupport::negate(*interpreter_, value, error);
}

std::optional<Value> NativeEngine::logical_not(const Value& value,
                                               NativeError& error) {
    return NativeSupport::logical_not(*interpreter_, value, error);
}

std::optional<std::int32_t> NativeEngine::to_long(const Value& value,
                                                  NativeError& error) {
    return NativeSupport::to_long(*interpreter_, value, error);
}

std::optional<double> NativeEngine::to_double(const Value& value,
                                              NativeError& error) {
    return NativeSupport::to_double(*interpreter_, value, error);
}

std::optional<std::string> NativeEngine::to_text(const Value& value,
                                                 NativeError& error) {
    return NativeSupport::to_text(*interpreter_, value, error);
}

std::optional<bool> NativeEngine::to_boolean(const Value& value,
                                             NativeError& error) {
    return NativeSupport::to_boolean(*interpreter_, value, error);
}

void NativeEngine::randomize(const bool has_seed, const double seed,
                             const bool is_timer) {
    NativeSupport::randomize(*interpreter_, has_seed, seed, is_timer);
}

bool NativeEngine::like(const std::string& text, const std::string& pattern,
                        const bool fold) {
    return NativeSupport::like(text, pattern, fold);
}

std::optional<Value> NativeEngine::default_value(const Value& value,
                                                 NativeError& error) {
    Value resolved = value;
    if (!NativeSupport::resolve_default(*interpreter_, resolved)) {
        NativeSupport::fetch_error(*interpreter_, error);
        return std::nullopt;
    }
    return resolved;
}

void NativeEngine::attach_events(const Value& source, EventHandler handler) {
    NativeSupport::attach_events(*interpreter_, source, std::move(handler));
}

bool NativeEngine::text_compare() const {
    return NativeSupport::text_compare(*interpreter_);
}

bool NativeEngine::start(EngineSetup setup, NativeError& error) {
    return NativeSupport::start(interpreter_, std::move(setup), error);
}

std::optional<Value> NativeEngine::global(const std::string& name,
                                          NativeError& error) {
    return NativeSupport::global(*interpreter_, name, error);
}

void NativeEngine::omit_arguments(const std::uint32_t mask) {
    NativeSupport::omitted_ = mask;
}

std::optional<Value> NativeEngine::member_get(const Value& object,
                                              const std::string& name,
                                              std::vector<Value>& arguments,
                                              NativeError& error) {
    return NativeSupport::member_get(*interpreter_, object, name, arguments,
                                     error);
}

bool NativeEngine::member_put(const Value& object, const std::string& name,
                              const std::vector<Value>& arguments,
                              const Value& value, const bool is_set,
                              NativeError& error) {
    return NativeSupport::member_put(*interpreter_, object, name, arguments,
                                     value, is_set, error);
}

std::optional<Value> NativeEngine::new_record(const std::string& name,
                                              NativeError& error) {
    return NativeSupport::new_form(*interpreter_, name, error);
}

void NativeEngine::set_field(Value& record, const std::string& field,
                             Value value) {
    if (auto* instance = std::get_if<ObjectInstance>(&record)) {
        std::string key = field;
        for (auto& c : key) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        instance->data->fields.variables[key] = std::move(value);
    }
}

Value NativeEngine::field(const Value& record, const std::string& name) {
    if (const auto* instance = std::get_if<ObjectInstance>(&record)) {
        std::string key = name;
        for (auto& c : key) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        const auto found = instance->data->fields.variables.find(key);
        if (found != instance->data->fields.variables.end()) {
            return found->second;
        }
    }
    return Value{Empty{}};
}

std::optional<Value> NativeEngine::new_form(const std::string& name,
                                            NativeError& error) {
    return NativeSupport::new_form(*interpreter_, name, error);
}

std::optional<std::vector<Value>> NativeEngine::items(const Value& object,
                                                      NativeError& error) {
    return NativeSupport::items(*interpreter_, object, error);
}

bool NativeEngine::run_forms(NativeError& error) {
    return NativeSupport::run_forms(*interpreter_, error);
}

bool NativeEngine::same_object(const Value& a, const Value& b) {
    const auto* first = std::get_if<ObjectInstance>(&a);
    const auto* second = std::get_if<ObjectInstance>(&b);
    if (first != nullptr && second != nullptr) {
        return first->data == second->data;
    }
    return first == nullptr && second == nullptr;
}

bool NativeEngine::run_statement(const std::string& text,
                                 std::vector<Value>& values,
                                 const std::uint32_t variant_mask,
                                 NativeError& error) {
    return NativeSupport::run_statement(*interpreter_, text, values,
                                        variant_mask, error);
}

std::optional<Value> NativeEngine::call_function(
    const std::string_view name, const std::vector<Value>& arguments,
    NativeError& error) {
    return NativeSupport::call_function(*interpreter_, name, arguments, error);
}

std::string NativeEngine::take_output(bool& line_open) {
    return NativeSupport::take_output(*interpreter_, line_open);
}

void NativeEngine::close_output_line() {
    NativeSupport::close_output_line(*interpreter_);
}

bool NativeEngine::is_library_function(const std::string_view name) {
    return NativeSupport::is_library_function(name);
}

std::string NativeEngine::render(const Value& value) {
    return NativeSupport::render(value);
}

std::string NativeEngine::print_text(const Value& value) {
    if (std::holds_alternative<Null>(value)) {
        return "Null";
    }
    if (is_number(value) && !std::holds_alternative<DateValue>(value)) {
        // A sign position before a number and a space after it.
        const std::string digits = NativeSupport::render(value);
        return (digits.empty() || digits.front() != '-' ? " " : "") + digits +
               " ";
    }
    return NativeSupport::render(value);
}

std::string NativeEngine::type_name(const Value& value) {
    return NativeSupport::type_name(*interpreter_, value);
}

int NativeEngine::var_type(const Value& value) {
    if (std::holds_alternative<Empty>(value)) {
        return 0;
    }
    if (std::holds_alternative<Null>(value)) {
        return 1;
    }
    if (std::holds_alternative<Int16>(value)) {
        return 2;
    }
    if (std::holds_alternative<Integer>(value)) {
        return 3;
    }
    if (std::holds_alternative<float>(value)) {
        return 4;
    }
    if (std::holds_alternative<double>(value)) {
        return 5;
    }
    if (std::holds_alternative<Currency>(value)) {
        return 6;
    }
    if (std::holds_alternative<DateValue>(value)) {
        return 7;
    }
    if (std::holds_alternative<std::string>(value)) {
        return 8;
    }
    if (std::holds_alternative<ErrorValue>(value)) {
        return 10;
    }
    if (std::holds_alternative<bool>(value)) {
        return 11;
    }
    if (std::holds_alternative<Decimal>(value)) {
        return 14;
    }
    if (std::holds_alternative<Byte>(value)) {
        return 17;
    }
    return 9;  // an object reference, Nothing included
}

}  // namespace wfc::detail
