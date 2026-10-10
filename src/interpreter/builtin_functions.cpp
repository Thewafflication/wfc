// Interpreter: The intrinsic VBA function library (parse_function_call).
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

std::optional<Value> Interpreter::parse_function_call(
    const std::string_view identifier, const std::size_t identifier_offset) {
    const bool dry_run = !execute_;
    auto result = parse_function_call_impl(identifier, identifier_offset);
    if (dry_run && !result.has_value()) {
        // A not-taken branch can hold placeholder arguments (a member
        // of `Nothing`), so a value-type complaint there is not an
        // error; syntax and arity errors still are.
        const std::string_view code =
            std::string_view(error_.diagnostic).substr(0, 7);
        if (code == "WFC0073" || code == "WFC0095" || code == "WFC0018" ||
            code == "WFC0016" || code == "WFC0007" || code == "WFC0101" ||
            (code != "WFC0300" && code != "WFC0072" && code != "WFC0071" &&
             runtime_error_number() != 0)) {
            error_ = wfc::Evaluation{};
            static const std::set<std::string, std::less<>> string_results = {
                "left",          "right",
                "mid",           "trim",
                "ltrim",         "rtrim",
                "lcase",         "ucase",
                "replace",       "string",
                "space",         "chr",
                "hex",           "oct",
                "str",           "cstr",
                "format",        "join",
                "strreverse",    "left$",
                "right$",        "mid$",
                "trim$",         "ltrim$",
                "rtrim$",        "lcase$",
                "ucase$",        "chr$",
                "hex$",          "oct$",
                "str$",          "format$",
                "space$",        "string$",
                "typename",      "strconv",
                "formatnumber",  "formatcurrency",
                "formatpercent", "formatdatetime",
                "monthname",     "weekdayname",
                "environ",       "environ$"};
            if (string_results.contains(std::string(identifier))) {
                return Value{std::string{}};
            }
            return Value{Integer{}};
        }
    }
    // REQ-0247: CByte's range-checked Long result is a Byte.
    if (identifier == "cbyte" && result.has_value()) {
        if (const auto* number = std::get_if<Integer>(&*result)) {
            return Value{static_cast<Byte>(*number)};
        }
    }
    return result;
}

std::optional<Value> Interpreter::parse_function_call_impl(
    const std::string_view identifier, const std::size_t identifier_offset) {
    const bool is_len = identifier == "len" || identifier == "lenb";
    const bool is_lower = identifier == "lcase" || identifier == "lcase$";
    const bool is_upper = identifier == "ucase" || identifier == "ucase$";
    const bool is_left_trim = identifier == "ltrim" || identifier == "ltrim$";
    const bool is_right_trim = identifier == "rtrim" || identifier == "rtrim$";
    const bool is_trim = identifier == "trim" || identifier == "trim$";
    const bool is_left = identifier == "left" || identifier == "left$" ||
                         identifier == "leftb" || identifier == "leftb$";
    const bool is_right = identifier == "right" || identifier == "right$" ||
                          identifier == "rightb" || identifier == "rightb$";
    const bool is_mid = identifier == "mid" || identifier == "mid$" ||
                        identifier == "midb" || identifier == "midb$";
    const bool is_asc =
        identifier == "asc" || identifier == "ascb" || identifier == "ascw";
    const bool is_chr_b = identifier == "chrb" || identifier == "chrb$";
    const bool is_chr = identifier == "chr" || identifier == "chr$" ||
                        identifier == "chrw" || identifier == "chrw$" ||
                        is_chr_b;
    const bool is_reverse = identifier == "strreverse";
    const bool is_space = identifier == "space" || identifier == "space$";
    const bool is_string = identifier == "string" || identifier == "string$";
    const bool is_instr = identifier == "instr" || identifier == "instrb";
    const bool is_instr_rev = identifier == "instrrev";
    const bool is_strcomp = identifier == "strcomp";
    const bool is_replace = identifier == "replace";
    const bool is_hex = identifier == "hex" || identifier == "hex$";
    const bool is_oct = identifier == "oct" || identifier == "oct$";
    const bool is_str = identifier == "str" || identifier == "str$";
    const bool is_val = identifier == "val";
    const bool is_abs = identifier == "abs";
    const bool is_sgn = identifier == "sgn";
    const bool is_cstr = identifier == "cstr";
    const bool is_clng = identifier == "clng";
    const bool is_cbool = identifier == "cbool";
    const bool is_cbyte = identifier == "cbyte";
    const bool is_cint = identifier == "cint";
    const bool is_cdbl = identifier == "cdbl";
    const bool is_csng = identifier == "csng";
    const bool is_ccur = identifier == "ccur";
    const bool is_cvar = identifier == "cvar";
    const bool is_cdec = identifier == "cdec";
    const bool is_macid = identifier == "macid";
    const bool is_error_message =
        identifier == "error" || identifier == "error$";
    const bool is_isnumeric = identifier == "isnumeric";
    const bool is_typename = identifier == "typename";
    const bool is_vartype = identifier == "vartype";
    const bool is_iif = identifier == "iif";
    const bool is_choose = identifier == "choose";
    const bool is_switch = identifier == "switch";
    const bool is_int = identifier == "int";
    const bool is_fix = identifier == "fix";
    const bool is_round = identifier == "round";
    const bool is_sqr = identifier == "sqr";
    const bool is_sin = identifier == "sin";
    const bool is_cos = identifier == "cos";
    const bool is_tan = identifier == "tan";
    const bool is_atn = identifier == "atn";
    const bool is_exp = identifier == "exp";
    const bool is_log = identifier == "log";
    const bool is_float_math =
        is_sqr || is_sin || is_cos || is_tan || is_atn || is_exp || is_log;
    const bool is_isarray = identifier == "isarray";
    const bool is_isobject = identifier == "isobject";
    const bool is_isnull = identifier == "isnull";
    const bool is_isempty = identifier == "isempty";
    const bool is_iserror = identifier == "iserror";
    const bool is_ismissing = identifier == "ismissing";
    const bool is_constant_false_predicate = is_iserror;
    const bool is_lbound = identifier == "lbound";
    const bool is_ubound = identifier == "ubound";
    const bool is_qbcolor = identifier == "qbcolor";
    const bool is_rgb = identifier == "rgb";
    const bool is_strconv = identifier == "strconv";
    const bool is_format = identifier == "format" || identifier == "format$";
    const bool is_rnd = identifier == "rnd";
    const bool is_array_fn = identifier == "array";
    const bool is_split = identifier == "split";
    const bool is_join = identifier == "join";
    const bool is_filter = identifier == "filter";
    const bool is_date_fn = is_date_function_name(identifier);
    const bool is_file_fn = is_file_function_name(identifier);
    const bool is_misc_fn = is_misc_function_name(identifier);
    if (!is_len && !is_lower && !is_upper && !is_left_trim && !is_right_trim &&
        !is_trim && !is_left && !is_right && !is_mid && !is_asc && !is_chr &&
        !is_reverse && !is_space && !is_string && !is_instr && !is_strcomp &&
        !is_instr_rev && !is_replace && !is_hex && !is_oct && !is_str &&
        !is_val && !is_abs && !is_sgn && !is_cstr && !is_clng && !is_cbool &&
        !is_cbyte && !is_cint && !is_isnumeric && !is_typename && !is_vartype &&
        !is_iif && !is_choose && !is_switch && !is_int && !is_fix &&
        !is_constant_false_predicate && !is_qbcolor && !is_rgb && !is_strconv &&
        !is_round && !is_cdbl && !is_csng && !is_ccur && !is_cvar &&
        !is_macid && !is_error_message && !is_float_math && !is_format &&
        !is_rnd && !is_isnull && !is_isempty && !is_cdec && !is_ismissing &&
        !is_isarray && !is_isobject && !is_lbound && !is_ubound &&
        !is_array_fn && !is_split && !is_join && !is_filter && !is_date_fn &&
        !is_file_fn && !is_misc_fn) {
        set_error("WFC0071", "unsupported function", identifier_offset);
        return std::nullopt;
    }
    if (constant_expression_) {
        set_error("WFC0074", "constant initializer cannot call a function",
                  identifier_offset);
        return std::nullopt;
    }

    // `IsMissing(paramName)` (REQ-0224) takes its own parameter's bare
    // *name*, not an evaluated expression -- unlike every other
    // intrinsic function's arguments, parsed generically below. An
    // omitted Optional Variant argument is bound to a real (default)
    // value indistinguishable from a caller-supplied one by the time
    // it would reach the generic `parse_expression()` loop, so the
    // only way to answer correctly is to look the name up directly
    // against `missing_parameter_names`, recorded by `invoke_definition`
    // at the moment the omission was still known. A parameter that
    // is not Optional/Variant, or an identifier that does not name a
    // parameter of the current procedure at all (including at module
    // level), answers `False` -- matching this evaluator's existing
    // constant-`False` stub for every case except the one this
    // requirement now gives a real answer for.
    if (is_ismissing) {
        skip_horizontal_whitespace();
        if (!consume('(')) {
            set_error("WFC0072",
                      "function received the wrong number of arguments",
                      identifier_offset);
            return std::nullopt;
        }
        skip_horizontal_whitespace();
        if (!at_end() && current() == ')') {
            set_error("WFC0072",
                      "function received the wrong number of arguments",
                      offset_);
            return std::nullopt;
        }
        const auto parameter_offset = offset_;
        char type_character{};
        auto parameter_name = parse_identifier(&type_character);
        if (!parameter_name.has_value() || type_character != '\0') {
            set_error("WFC0011", "IsMissing requires a parameter name",
                      parameter_offset);
            return std::nullopt;
        }
        skip_horizontal_whitespace();
        if (!at_end() && current() == ',') {
            set_error("WFC0072",
                      "function received the wrong number of arguments",
                      offset_);
            return std::nullopt;
        }
        if (!consume(')')) {
            set_error("WFC0005", "expected closing parenthesis", offset_);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{false};
        }
        // Membership in `missing_parameter_names` already implies
        // Optional/Variant/no-default (see where it is populated
        // above); confirming the name is at least a real parameter
        // of the current procedure first (rather than checking the
        // set directly) keeps `IsMissing(someUnrelatedName)` a plain
        // False instead of an accidental True from a stale name.
        bool is_missing = false;
        if (in_procedure() && current_procedure_def_ != nullptr) {
            for (const auto& parameter : current_procedure_def_->parameters) {
                if (parameter.name == *parameter_name) {
                    is_missing =
                        current_scope().missing_parameter_names.contains(
                            *parameter_name);
                    break;
                }
            }
        }
        return Value{is_missing};
    }

    // A missing `(` is a parenthesis-free, zero-argument call (REQ-0213)
    // -- e.g. `Print Rnd` -- reachable only through
    // parse_primary_base's bare-identifier intrinsic-function fallback,
    // never through the ordinary `identifier(...)` call site (which
    // only reaches here once it has already confirmed `(`). The
    // function's own arity check below rejects this with `WFC0072` if
    // it actually requires one or more arguments.
    std::vector<Value> arguments;
    if (consume('(')) {
        skip_horizontal_whitespace();
        if (!consume(')')) {
            while (true) {
                if (!at_end() && current() == ',') {
                    // An omitted optional slot of Replace/InStr/InStrRev
                    // takes that parameter's default.
                    const auto slot = arguments.size();
                    const Integer compare_default =
                        option_compare_text_ ? 1 : 0;
                    std::optional<Integer> default_value;
                    if (identifier == "replace") {
                        if (slot == 3U) {
                            default_value = 1;
                        } else if (slot == 4U) {
                            default_value = -1;
                        } else if (slot == 5U) {
                            default_value = compare_default;
                        }
                    } else if (identifier == "instr") {
                        if (slot == 0U) {
                            default_value = 1;
                        } else if (slot == 3U) {
                            default_value = compare_default;
                        }
                    } else if (identifier == "instrrev") {
                        if (slot == 2U) {
                            default_value = -1;
                        } else if (slot == 3U) {
                            default_value = compare_default;
                        }
                    } else if (identifier == "formatnumber" ||
                               identifier == "formatcurrency" ||
                               identifier == "formatpercent") {
                        if (slot == 1U) {
                            default_value = -1;
                        } else if (slot >= 2U && slot <= 4U) {
                            default_value = -2;  // vbUseDefault
                        }
                    }
                    if (!default_value.has_value()) {
                        set_error("WFC0072",
                                  "function received an empty argument",
                                  offset_);
                        return std::nullopt;
                    }
                    arguments.push_back(Value{*default_value});
                    advance();
                    skip_horizontal_whitespace();
                    continue;
                }
                auto argument = parse_expression();
                if (!argument.has_value()) {
                    return std::nullopt;
                }
                arguments.push_back(std::move(*argument));
                skip_horizontal_whitespace();
                if (consume(')')) {
                    break;
                }
                if (!consume(',')) {
                    set_error("WFC0005", "expected closing parenthesis",
                              offset_);
                    return std::nullopt;
                }
                skip_horizontal_whitespace();
                if (consume(')')) {
                    set_error("WFC0072", "function received an empty argument",
                              offset_ - 1U);
                    return std::nullopt;
                }
            }
        }
    }

    {
        // Numeric strings are accepted where a math routine wants a number.
        static const std::set<std::string, std::less<>> math_functions = {
            "abs", "sgn", "int", "fix", "sqr", "sin",
            "cos", "tan", "atn", "exp", "log", "round"};
        if (!arguments.empty() &&
            math_functions.contains(std::string(identifier))) {
            if (const auto* text = std::get_if<std::string>(&arguments[0])) {
                const auto parsed = parse_numeric_string(*text);
                if (parsed.status == NumericStringStatus::valid) {
                    arguments[0] = Value{parsed.value};
                }
            }
        }
        if (identifier == "strcomp" && execute_ && arguments.size() >= 2U &&
            (std::holds_alternative<Null>(arguments[0]) ||
             std::holds_alternative<Null>(arguments[1]))) {
            return Value{Null{}};
        }
    }
    {
        // `Integer` (Int16) and `Byte` arguments reach library routines as
        // Long, except for functions whose result depends on the subtype.
        static const std::set<std::string, std::less<>> keep_subtype = {
            "typename",  "vartype",   "hex",           "hex$",      "oct",
            "oct$",      "isnumeric", "isempty",       "isnull",    "isobject",
            "isarray",   "isdate",    "iserror",       "ismissing", "cvar",
            "cstr",      "cbool",     "cbyte",         "cint",      "clng",
            "csng",      "cdbl",      "ccur",          "cdec",      "cdate",
            "cvdate",    "cverr",     "abs",           "sgn",       "int",
            "fix",       "format",    "format$",       "str",       "str$",
            "len",       "lenb",      "array",         "iif",       "switch",
            "isnumeric", "varptr",    "formatdatetime"};
        const bool is_choose_fn = identifier == "choose";
        const bool keep_first =
            identifier == "round";  // Round(Integer) stays Integer
        if (!keep_subtype.contains(std::string(identifier))) {
            for (std::size_t index = keep_first ? 1U : 0U;
                 index < arguments.size(); ++index) {
                if (is_choose_fn && index > 0U) {
                    break;
                }
                if (const auto* short_value =
                        std::get_if<Int16>(&arguments[index])) {
                    arguments[index] =
                        Value{static_cast<Integer>(*short_value)};
                } else if (const auto* byte_value =
                               std::get_if<Byte>(&arguments[index])) {
                    arguments[index] = Value{static_cast<Integer>(*byte_value)};
                }
            }
        }
    }
    if (is_date_fn) {
        static const std::set<std::string, std::less<>> null_propagating_dates =
            {"year",   "month",   "day",     "hour",     "minute",
             "second", "weekday", "dateadd", "datediff", "datepart"};
        if (execute_ &&
            null_propagating_dates.contains(std::string(identifier)) &&
            std::any_of(arguments.begin(), arguments.end(),
                        [](const Value& value) {
                            return std::holds_alternative<Null>(value);
                        })) {
            return Value{Null{}};
        }
        return evaluate_date_function(identifier, arguments, identifier_offset);
    }
    if (is_file_fn) {
        return evaluate_file_function(identifier, arguments, identifier_offset);
    }
    if (is_misc_fn) {
        return evaluate_misc_function(identifier, arguments, identifier_offset);
    }
    // REQ-0247: functions other than the type probes see a Byte as a Long.
    if (!is_typename && !is_vartype && !is_cvar && !is_iif && !is_choose &&
        !is_switch && !is_isnumeric && !is_isarray && !is_isobject &&
        !is_isnull && !is_isempty) {
        for (auto& argument : arguments) {
            if (const auto* byte = std::get_if<Byte>(&argument)) {
                argument = Value{static_cast<Integer>(*byte)};
            }
        }
    }
    // REQ-0259: an error-subtype Variant reaching a function that needs a
    // real value raises the error it carries.
    if (!is_typename && !is_vartype && !is_cvar && !is_iif && !is_choose &&
        !is_switch && !is_isnumeric && !is_isarray && !is_isobject &&
        !is_isnull && !is_isempty && !is_constant_false_predicate && !is_cstr &&
        !is_ismissing && execute_) {
        for (const auto& argument : arguments) {
            if (const auto* error_value = std::get_if<ErrorValue>(&argument)) {
                static_cast<void>(raise_runtime(
                    error_value->code == 0 ? 5 : error_value->code,
                    vb_error_description(error_value->code),
                    identifier_offset));
                return std::nullopt;
            }
        }
    }
    // REQ-0242: numeric conversions/functions see a Date as its serial.
    if (!arguments.empty() && std::holds_alternative<DateValue>(arguments[0]) &&
        (is_cdbl || is_csng || is_clng || is_cint || is_ccur || is_cdec ||
         is_cbyte || is_cbool || is_int || is_fix || is_round || is_abs ||
         is_sgn)) {
        arguments[0] = Value{std::get<DateValue>(arguments[0]).serial};
    }
    bool valid_arity{};
    if (is_array_fn) {
        valid_arity = true;
    } else if (is_split || is_filter) {
        valid_arity =
            arguments.size() >= (is_split ? 1U : 2U) && arguments.size() <= 4U;
    } else if (is_join || is_round || is_format || is_lbound || is_ubound) {
        valid_arity = arguments.size() == 1U || arguments.size() == 2U;
    } else if (is_error_message || is_rnd) {
        valid_arity = arguments.size() <= 1U;
    } else if (is_mid || is_strcomp) {
        valid_arity = arguments.size() == 2U || arguments.size() == 3U;
    } else if (is_instr || is_instr_rev) {
        valid_arity = arguments.size() >= 2U && arguments.size() <= 4U;
    } else if (is_replace) {
        valid_arity = arguments.size() >= 3U && arguments.size() <= 6U;
    } else if (is_iif || is_rgb) {
        valid_arity = arguments.size() == 3U;
    } else if (is_choose) {
        valid_arity = arguments.size() >= 2U;
    } else if (is_switch) {
        valid_arity = arguments.size() >= 2U && arguments.size() % 2U == 0U;
    } else if (is_left || is_right || is_string || is_strconv) {
        valid_arity = arguments.size() == 2U;
    } else {
        valid_arity = arguments.size() == 1U;
    }
    if (!valid_arity) {
        set_error("WFC0072", "function received the wrong number of arguments",
                  identifier_offset);
        return std::nullopt;
    }

    // REQ-0239: Array/Split/Join/Filter.
    if (is_array_fn) {
        ArrayValue result{std::move(arguments),
                          /*lower_bound=*/option_base_one_ ? 1 : 0,
                          /*is_dynamic=*/false, /*is_allocated=*/true,
                          Value{Empty{}}.index()};
        result.is_variant_element = true;
        return Value{std::move(result)};
    }
    if (is_split || is_join || is_filter) {
        const auto make_string_array = [](std::vector<std::string> parts) {
            std::vector<Value> elements;
            elements.reserve(parts.size());
            for (auto& part : parts) {
                elements.emplace_back(std::move(part));
            }
            return Value{ArrayValue{
                std::move(elements), /*lower_bound=*/0, /*is_dynamic=*/false,
                /*is_allocated=*/true, Value{std::string{}}.index()}};
        };
        const auto text_argument =
            [&](const std::size_t index) -> const std::string* {
            return std::get_if<std::string>(&arguments[index]);
        };
        if (!execute_) {
            return is_join ? Value{std::string{}} : make_string_array({});
        }
        if (is_split) {
            const auto* text = text_argument(0);
            std::string delimiter = " ";
            if (arguments.size() >= 2U) {
                const auto* delimiter_argument = text_argument(1);
                if (delimiter_argument == nullptr) {
                    set_error("WFC0073", "Split requires a String delimiter",
                              identifier_offset);
                    return std::nullopt;
                }
                delimiter = *delimiter_argument;
            }
            Integer limit = -1;
            if (arguments.size() >= 3U) {
                const auto* limit_argument =
                    std::get_if<Integer>(&arguments[2]);
                if (limit_argument == nullptr) {
                    set_error("WFC0073", "Split limit must be Long",
                              identifier_offset);
                    return std::nullopt;
                }
                limit = *limit_argument;
            }
            if (text == nullptr) {
                set_error("WFC0073", "Split requires a String argument",
                          identifier_offset);
                return std::nullopt;
            }
            if (text->empty() || limit == 0) {
                return make_string_array({});
            }
            bool text_compare = option_compare_text_;
            if (arguments.size() >= 4U) {
                if (const auto* mode = std::get_if<Integer>(&arguments[3])) {
                    text_compare =
                        *mode == 1 || (*mode == -1 && option_compare_text_);
                } else if (const auto* mode16 =
                               std::get_if<Int16>(&arguments[3])) {
                    text_compare =
                        *mode16 == 1 || (*mode16 == -1 && option_compare_text_);
                }
            }
            std::string search_text = *text;
            if (text_compare) {
                search_text = fold_case(search_text);
                delimiter = fold_case(delimiter);
            }
            std::vector<std::string> parts;
            if (delimiter.empty()) {
                parts.push_back(*text);
            } else {
                std::size_t position = 0;
                while (limit < 0 ||
                       static_cast<Integer>(parts.size()) < limit - 1) {
                    const auto found = search_text.find(delimiter, position);
                    if (found == std::string::npos) {
                        break;
                    }
                    parts.push_back(text->substr(position, found - position));
                    position = found + delimiter.size();
                }
                parts.push_back(text->substr(position));
            }
            return make_string_array(std::move(parts));
        }
        const auto* array = std::get_if<ArrayValue>(&arguments[0]);
        if (array == nullptr || !array->dimensions.empty()) {
            set_error("WFC0073",
                      "function requires a one-dimensional array argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (is_join) {
            std::string delimiter = " ";
            if (arguments.size() == 2U) {
                const auto* delimiter_argument = text_argument(1);
                if (delimiter_argument == nullptr) {
                    set_error("WFC0073", "Join requires a String delimiter",
                              identifier_offset);
                    return std::nullopt;
                }
                delimiter = *delimiter_argument;
            }
            std::string joined;
            bool first = true;
            for (const auto& element : array->elements) {
                if (std::holds_alternative<Null>(element) ||
                    is_object_reference(element) ||
                    std::holds_alternative<ArrayValue>(element)) {
                    set_error("WFC0073", "Join element must be a scalar",
                              identifier_offset);
                    return std::nullopt;
                }
                if (!first) {
                    joined += delimiter;
                }
                first = false;
                joined += render(element);
            }
            return Value{std::move(joined)};
        }
        const auto* match = text_argument(1);
        if (match == nullptr) {
            set_error("WFC0073", "Filter requires a String match",
                      identifier_offset);
            return std::nullopt;
        }
        bool include = true;
        if (arguments.size() >= 3U) {
            const auto* include_argument = std::get_if<bool>(&arguments[2]);
            if (include_argument == nullptr) {
                set_error("WFC0073", "Filter include must be Boolean",
                          identifier_offset);
                return std::nullopt;
            }
            include = *include_argument;
        }
        bool text_compare = option_compare_text_;
        if (arguments.size() == 4U) {
            const auto* compare_argument = std::get_if<Integer>(&arguments[3]);
            if (compare_argument == nullptr) {
                set_error("WFC0073", "Filter compare must be Long",
                          identifier_offset);
                return std::nullopt;
            }
            text_compare = *compare_argument == 1;
        }
        const auto fold = [&](std::string text) {
            if (text_compare) {
                text = fold_case(text);
            }
            return text;
        };
        const std::string needle = fold(*match);
        std::vector<std::string> kept;
        for (const auto& element : array->elements) {
            const std::string text = render(element);
            if ((fold(text).find(needle) != std::string::npos) == include) {
                kept.push_back(text);
            }
        }
        return make_string_array(std::move(kept));
    }

    if (is_chr) {
        const auto* character_code = std::get_if<Integer>(&arguments[0]);
        if (character_code == nullptr) {
            set_error("WFC0073", "Chr requires a Long argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        const bool wide = identifier == "chrw" || identifier == "chrw$";
        const Integer maximum = wide ? 65535 : 255;
        if (*character_code < 0 || *character_code > maximum) {
            set_error("WFC0078",
                      is_chr_b ? "ChrB code must be in the byte range"
                               : "Chr code must be in the character range",
                      identifier_offset);
            return std::nullopt;
        }
        if (is_chr_b) {
            return Value{bytes_to_string(
                std::string(1U, static_cast<char>(*character_code)))};
        }
        if (*character_code >= 0x80) {
            // Chr maps through Windows-1252; ChrW is the code unit itself.
            const auto code = static_cast<unsigned>(*character_code);
            std::string utf8;
            append_utf8_unit(utf8, wide ? code : ansi_to_unicode(code));
            return Value{std::move(utf8)};
        }
        return Value{std::string(1U, static_cast<char>(*character_code))};
    }

    if (is_abs || is_sgn) {
        if (!is_number(arguments[0])) {
            set_error("WFC0073",
                      is_abs ? "Abs requires a numeric argument"
                             : "Sgn requires a numeric argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return is_abs ? arguments[0] : Value{Integer{}};
        }
        if (is_sgn) {
            const double number = as_double(arguments[0]);
            return Value{static_cast<Integer>((number > 0.0) - (number < 0.0))};
        }
        if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
            if (*integer == std::numeric_limits<Integer>::min()) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{*integer < 0 ? static_cast<Integer>(-*integer)
                                      : *integer};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            if (*short_integer == std::numeric_limits<Int16>::min()) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{*short_integer < 0
                             ? static_cast<Int16>(-*short_integer)
                             : *short_integer};
        }
        if (const auto* single = std::get_if<float>(&arguments[0])) {
            return Value{std::abs(*single)};
        }
        if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
            if (currency->scaled == std::numeric_limits<std::int64_t>::min()) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{Currency{currency->scaled < 0 ? -currency->scaled
                                                       : currency->scaled}};
        }
        if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
            Decimal result = *decimal;
            result.negative = false;
            return Value{result};
        }
        return Value{std::abs(std::get<double>(arguments[0]))};
    }

    if (is_qbcolor) {
        const auto* color = std::get_if<Integer>(&arguments[0]);
        if (color == nullptr) {
            set_error("WFC0073", "QBColor requires a Long color index",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        if (*color < 0 || *color > 15) {
            set_error("WFC0092", "QBColor index must be from 0 through 15",
                      identifier_offset);
            return std::nullopt;
        }
        constexpr Integer colors[] = {0x000000, 0x800000, 0x008000, 0x808000,
                                      0x000080, 0x800080, 0x008080, 0xC0C0C0,
                                      0x808080, 0xFF0000, 0x00FF00, 0xFFFF00,
                                      0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF};
        return Value{colors[*color]};
    }

    if (is_rgb) {
        Integer component[3]{};
        for (std::size_t index = 0U; index < 3U; ++index) {
            const auto* value = std::get_if<Integer>(&arguments[index]);
            if (value == nullptr) {
                set_error("WFC0073",
                          "RGB requires Long red, green, and blue components",
                          identifier_offset);
                return std::nullopt;
            }
            component[index] = *value;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        for (Integer& value : component) {
            if (value < 0) {
                set_error("WFC0091", "RGB component must be non-negative",
                          identifier_offset);
                return std::nullopt;
            }
            if (value > 255) {
                value = 255;  // VB6 assumes any component above 255 is 255.
            }
        }
        return Value{static_cast<Integer>(component[0] + component[1] * 256 +
                                          component[2] * 65536)};
    }

    if (is_constant_false_predicate) {
        return Value{execute_ &&
                     std::holds_alternative<ErrorValue>(arguments[0])};
    }
    if (is_isnull) {
        return Value{execute_ && std::holds_alternative<Null>(arguments[0])};
    }

    if (is_isempty) {
        return Value{execute_ && std::holds_alternative<Empty>(arguments[0])};
    }

    if (is_isarray) {
        return Value{execute_ &&
                     std::holds_alternative<ArrayValue>(arguments[0])};
    }

    if (is_isobject) {
        // Verified VB6 fact: IsObject(Nothing) is True -- Nothing is
        // still an object reference (just an unset one), distinct from
        // Null/Empty. A live instance is of course an object too.
        return Value{execute_ && is_object_reference(arguments[0])};
    }

    if (is_lbound || is_ubound) {
        const auto* array = std::get_if<ArrayValue>(&arguments[0]);
        if (array == nullptr) {
            set_error("WFC0073",
                      is_lbound ? "LBound requires an array argument"
                                : "UBound requires an array argument",
                      identifier_offset);
            return std::nullopt;
        }
        // The optional second argument is a 1-based dimension number
        // (REQ-0210), defaulting to 1 -- the only meaningful value for
        // an ordinary 1-D array.
        Integer dimension = 1;
        if (arguments.size() == 2U) {
            const auto* dimension_argument =
                std::get_if<Integer>(&arguments[1]);
            if (dimension_argument == nullptr) {
                set_error("WFC0073", "LBound/UBound dimension must be Long",
                          identifier_offset);
                return std::nullopt;
            }
            dimension = *dimension_argument;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        if (!array->is_allocated) {
            // Matches real VB6: LBound/UBound on a dynamic array before
            // its first ReDim raises the same "subscript out of range"
            // error an out-of-bounds index does, rather than silently
            // answering from an empty [0, -1] range. Checked before the
            // dimension-count range check below, since an unallocated
            // array's own dimension count may itself only be
            // provisional (REQ-0219's `dynamic_dimension_count`), not
            // yet the authoritative shape a dimension argument should
            // be validated against.
            set_error("WFC0111", "array subscript out of range",
                      identifier_offset);
            return std::nullopt;
        }
        const std::size_t dimension_count = array->dimensions.empty()
                                                ? std::size_t{1}
                                                : array->dimensions.size();
        if (dimension < 1 ||
            static_cast<std::size_t>(dimension) > dimension_count) {
            set_error("WFC0148", "LBound/UBound dimension is out of range",
                      identifier_offset);
            return std::nullopt;
        }
        if (array->dimensions.empty()) {
            return Value{
                is_lbound
                    ? array->lower_bound
                    : array->lower_bound +
                          static_cast<Integer>(array->elements.size()) - 1};
        }
        const auto& dimension_bound =
            array->dimensions[static_cast<std::size_t>(dimension) - 1U];
        return Value{is_lbound ? dimension_bound.first
                               : dimension_bound.second};
    }

    if (is_cdec) {
        if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
            return Value{execute_ ? *decimal : Decimal{}};
        }
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
        if (is_object_reference(arguments[0]) ||
            std::holds_alternative<ArrayValue>(arguments[0])) {
            set_error("WFC0105", "CDec requires a numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Decimal{}};
        }
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{Decimal{}};
        }
        if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
            Decimal result;
            result.negative = *integer < 0;
            result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                *integer < 0 ? -static_cast<std::int64_t>(*integer)
                             : *integer));
            return Value{result};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            Decimal result;
            result.negative = *short_integer < 0;
            result.mantissa = big_from_u32(static_cast<std::uint32_t>(
                *short_integer < 0 ? -static_cast<std::int32_t>(*short_integer)
                                   : *short_integer));
            return Value{result};
        }
        if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
            Decimal result;
            result.negative = currency->scaled < 0;
            const auto magnitude =
                currency->scaled < 0
                    ? (~static_cast<std::uint64_t>(currency->scaled) + 1ULL)
                    : static_cast<std::uint64_t>(currency->scaled);
            result.mantissa.limb[0] = static_cast<std::uint32_t>(magnitude);
            result.mantissa.limb[1] =
                static_cast<std::uint32_t>(magnitude >> 32U);
            result.scale = 4U;
            return Value{result};
        }
        if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
            Decimal result;
            result.negative = *boolean;
            result.mantissa = big_from_u32(*boolean ? 1U : 0U);
            return Value{result};
        }
        double numeric_value{};
        if (const auto* number = std::get_if<double>(&arguments[0])) {
            numeric_value = *number;
        } else if (const auto* single = std::get_if<float>(&arguments[0])) {
            numeric_value = static_cast<double>(*single);
        } else {
            const auto parsed =
                parse_decimal_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0105", "CDec requires a numeric value",
                          identifier_offset);
                return std::nullopt;
            }
            return Value{parsed.value};
        }
        const auto converted = decimal_from_double(numeric_value);
        if (!converted.has_value()) {
            set_error("WFC0009", "numeric overflow", identifier_offset);
            return std::nullopt;
        }
        return Value{*converted};
    }

    if (is_int || is_fix) {
        if (!is_number(arguments[0])) {
            set_error("WFC0073",
                      is_int ? "Int requires a numeric argument"
                             : "Fix requires a numeric argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
            return Value{execute_ ? *integer : Integer{}};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            return Value{execute_ ? *short_integer : Int16{}};
        }
        if (!execute_) {
            return Value{0.0};
        }
        if (const auto* single = std::get_if<float>(&arguments[0])) {
            return Value{is_int ? std::floor(*single) : std::trunc(*single)};
        }
        if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
            const std::int64_t scaled = currency->scaled;
            const std::int64_t remainder = scaled % 10000;
            std::int64_t truncated = scaled - remainder;
            if (is_int && remainder != 0 && scaled < 0) {
                truncated -= 10000;
            }
            return Value{Currency{truncated}};
        }
        if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
            if (decimal->scale == 0U) {
                return Value{*decimal};
            }
            BigUInt quotient;
            BigUInt remainder;
            divide_big(decimal->mantissa, power_of_ten_big(decimal->scale),
                       quotient, remainder);
            Decimal truncated;
            truncated.negative = decimal->negative;
            truncated.mantissa = quotient;
            if (is_int && decimal->negative && !is_zero_big(remainder)) {
                truncated.mantissa =
                    add_big(truncated.mantissa, big_from_u32(1U));
            }
            if (is_zero_big(truncated.mantissa)) {
                truncated.negative = false;
            }
            return Value{truncated};
        }
        const double number = std::get<double>(arguments[0]);
        return Value{is_int ? std::floor(number) : std::trunc(number)};
    }

    if (is_float_math) {
        if (!is_number(arguments[0])) {
            set_error("WFC0073", "math function requires a numeric argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{0.0};
        }
        const double argument = as_double(arguments[0]);
        double result{};
        if (is_sqr) {
            if (argument < 0.0) {
                set_error("WFC0096", "Sqr argument must be non-negative",
                          identifier_offset);
                return std::nullopt;
            }
            result = std::sqrt(argument);
        } else if (is_log) {
            if (argument <= 0.0) {
                set_error("WFC0096", "Log argument must be positive",
                          identifier_offset);
                return std::nullopt;
            }
            result = std::log(argument);
        } else if (is_sin) {
            result = std::sin(argument);
        } else if (is_cos) {
            result = std::cos(argument);
        } else if (is_tan) {
            result = std::tan(argument);
        } else if (is_atn) {
            result = std::atan(argument);
        } else {
            result = std::exp(argument);  // is_exp
        }
        if (!std::isfinite(result)) {
            set_error("WFC0009", "numeric overflow", identifier_offset);
            return std::nullopt;
        }
        return Value{result};
    }

    if (is_round) {
        if (!is_number(arguments[0])) {
            set_error("WFC0073", "Round requires a numeric argument",
                      identifier_offset);
            return std::nullopt;
        }
        Integer digits = 0;
        if (arguments.size() == 2U) {
            const auto* requested_digits = std::get_if<Integer>(&arguments[1]);
            if (requested_digits == nullptr) {
                set_error("WFC0073", "Round requires a Long digit count",
                          identifier_offset);
                return std::nullopt;
            }
            digits = *requested_digits;
            if (execute_ && digits < 0) {
                set_error("WFC0094", "Round digit count must be non-negative",
                          identifier_offset);
                return std::nullopt;
            }
        }
        if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
            return Value{execute_ ? *integer : Integer{}};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            return Value{execute_ ? *short_integer : Int16{}};
        }
        if (!execute_) {
            return Value{0.0};
        }
        if (const auto* single = std::get_if<float>(&arguments[0])) {
            if (digits >= std::numeric_limits<float>::max_digits10) {
                return Value{*single};
            }
            const float scale = std::pow(10.0f, static_cast<float>(digits));
            if (std::abs(*single) > std::numeric_limits<float>::max() / scale) {
                return Value{*single};
            }
            return Value{std::nearbyint(*single * scale) / scale};
        }
        if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
            if (digits >= 4) {
                return Value{*currency};
            }
            static constexpr std::int64_t divisors[4] = {10000, 1000, 100, 10};
            const std::int64_t divisor = divisors[digits];
            const std::int64_t scaled = currency->scaled;
            const std::int64_t quotient = scaled / divisor;
            const std::int64_t remainder = scaled % divisor;
            const std::int64_t abs_remainder =
                remainder < 0 ? -remainder : remainder;
            const std::int64_t half = divisor / 2;
            std::int64_t rounded_quotient = quotient;
            if (abs_remainder > half ||
                (abs_remainder == half && (quotient % 2 != 0))) {
                rounded_quotient += (scaled < 0 ? -1 : 1);
            }
            return Value{Currency{rounded_quotient * divisor}};
        }
        if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
            if (digits >= decimal->scale) {
                return Value{*decimal};
            }
            Decimal rounded = *decimal;
            const Integer reduce_by =
                static_cast<Integer>(decimal->scale) - digits;
            for (Integer step = 0; step < reduce_by; ++step) {
                rounded.mantissa = divide_by_ten_rounded_big(rounded.mantissa);
            }
            rounded.scale = static_cast<std::uint8_t>(digits);
            if (is_zero_big(rounded.mantissa)) {
                rounded.negative = false;
            }
            return Value{rounded};
        }
        const double number = std::get<double>(arguments[0]);
        if (digits >= std::numeric_limits<double>::max_digits10) {
            return Value{number};
        }
        const double scale = std::pow(10.0, static_cast<double>(digits));
        if (std::abs(number) > std::numeric_limits<double>::max() / scale) {
            return Value{number};
        }
        return Value{std::nearbyint(number * scale) / scale};
    }

    if (is_format) {
        if (arguments.size() == 1U) {
            return Value{execute_ ? render(arguments[0]) : std::string{}};
        }
        const auto* style = std::get_if<std::string>(&arguments[1]);
        if (style == nullptr) {
            set_error("WFC0073", "Format requires a String Style argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (const auto* date_argument = std::get_if<DateValue>(&arguments[0])) {
            return Value{
                execute_ ? format_date_pattern(date_argument->serial, *style)
                         : std::string{}};
        }
        if (is_number(arguments[0]) &&
            style->find_first_of("@&") != std::string::npos &&
            style->find_first_of("0#") == std::string::npos) {
            arguments[0] = Value{
                render(arguments[0])};  // `@@@@` formats the digits as text
        }
        if (const auto* text_argument =
                std::get_if<std::string>(&arguments[0])) {
            // REQ-0264: string formats -- `@`/`&` placeholders, `<`, `>`, `!`.
            if (!execute_) {
                return Value{std::string{}};
            }
            std::string fmt = *style;
            const auto section = fmt.find(';');
            if (section != std::string::npos) {
                fmt = text_argument->empty() &&
                              fmt.find(';', section + 1) != std::string::npos
                          ? fmt.substr(fmt.find(';', section + 1) + 1)
                          : fmt.substr(0, section);
            }
            bool upper = false, lower = false, left_fill = false;
            // Work in UTF-16 units;  marks a literal character in the mask.
            const std::u16string wide_fmt = to_utf16_units(fmt);
            std::u16string mask;
            for (std::size_t i = 0; i < wide_fmt.size(); ++i) {
                const char16_t c = wide_fmt[i];
                if (c == u'>') {
                    upper = true;
                } else if (c == u'<') {
                    lower = true;
                } else if (c == u'!') {
                    left_fill = true;
                } else if (c == u'\\' && i + 1 < wide_fmt.size()) {
                    mask.push_back(u'');
                    mask.push_back(wide_fmt[++i]);
                } else if (c == u'"') {
                    while (++i < wide_fmt.size() && wide_fmt[i] != u'"') {
                        mask.push_back(u'');
                        mask.push_back(wide_fmt[i]);
                    }
                } else {
                    mask.push_back(c);
                }
            }
            std::u16string chars = to_utf16_units(*text_argument);
            for (auto& c : chars) {
                if (upper) {
                    c = unit_to_upper(c);
                }
                if (lower) {
                    c = unit_to_lower(c);
                }
            }
            std::size_t placeholders = 0;
            for (std::size_t i = 0; i < mask.size(); ++i) {
                if (mask[i] == u'') {
                    ++i;
                    continue;
                }
                if (mask[i] == u'@' || mask[i] == u'&') {
                    ++placeholders;
                }
            }
            if (placeholders == 0) {
                return Value{from_utf16_units(chars)};
            }
            std::u16string out;
            // `@` pads with a space, `&` with nothing; characters fill right to
            // left unless `!` asks for left to right.
            std::size_t next = left_fill ? 0
                                         : (chars.size() > placeholders
                                                ? chars.size() - placeholders
                                                : 0);
            const std::size_t skip =
                left_fill
                    ? 0
                    : (placeholders > chars.size() ? placeholders - chars.size()
                                                   : 0);
            std::size_t seen = 0;
            std::u16string tail;
            if (left_fill && chars.size() > placeholders) {
                tail = chars.substr(placeholders);
            }
            if (!left_fill && chars.size() > placeholders) {
                out = chars.substr(0, chars.size() - placeholders);
            }
            for (std::size_t i = 0; i < mask.size(); ++i) {
                if (mask[i] == u'') {
                    out.push_back(mask[++i]);
                    continue;
                }
                if (mask[i] == u'@' || mask[i] == u'&') {
                    const bool pad = !left_fill && seen < skip;
                    if (pad) {
                        if (mask[i] == u'@') {
                            out.push_back(u' ');
                        }
                    } else if (next < chars.size()) {
                        out.push_back(chars[next++]);
                    } else if (mask[i] == u'@') {
                        out.push_back(u' ');
                    }
                    ++seen;
                } else {
                    out.push_back(mask[i]);
                }
            }
            return Value{from_utf16_units(out + tail)};
        }
        if (std::holds_alternative<Null>(arguments[0])) {
            return Value{Null{}};
        }
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{std::string{}};
        }
        if (!is_number(arguments[0]) &&
            !std::holds_alternative<bool>(arguments[0])) {
            set_error("WFC0073",
                      "Format with a Style argument requires a Long, Double, "
                      "or Boolean expression",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        std::string lowered_style = *style;
        for (char& character : lowered_style) {
            character = ascii_lower(character);
        }
        // An empty Style behaves exactly like the one-argument form
        // (REQ-0218), matching real VB6.
        if (style->empty() || lowered_style == "general number") {
            if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
                return Value{std::to_string(*integer)};
            }
            if (std::holds_alternative<bool>(arguments[0])) {
                return Value{
                    std::string{std::get<bool>(arguments[0]) ? "-1" : "0"}};
            }
            return Value{render(arguments[0])};
        }
        const double widened = std::holds_alternative<bool>(arguments[0])
                                   ? (std::get<bool>(arguments[0]) ? -1.0 : 0.0)
                                   : as_double(arguments[0]);
        if (lowered_style == "yes/no") {
            return Value{std::string{widened != 0.0 ? "Yes" : "No"}};
        }
        if (lowered_style == "true/false") {
            return Value{std::string{widened != 0.0 ? "True" : "False"}};
        }
        if (lowered_style == "on/off") {
            return Value{std::string{widened != 0.0 ? "On" : "Off"}};
        }
        if (lowered_style == "fixed") {
            return Value{render_fixed_style(widened, false)};
        }
        if (lowered_style == "standard") {
            return Value{render_fixed_style(widened, true)};
        }
        if (lowered_style == "currency") {
            // A disclosed, unverified-against-the-reference-runtime
            // simplification (REQ-0193): real VB6's Currency style
            // uses the *system locale's* currency symbol and negative-
            // value convention (commonly parenthesized, e.g.
            // "($1,234.50)" under a US locale), neither of which this
            // evaluator has any notion of. This renders a fixed
            // US-dollar-sign prefix ahead of the same grouped,
            // two-decimal "Standard" magnitude, with a leading '-' for
            // a negative value (matching every other numeric style's
            // own negative-sign convention) rather than parentheses.
            std::string rendered = render_fixed_style(widened, true);
            const std::string::size_type dollar_position =
                (!rendered.empty() && rendered.front() == '-') ? 1U : 0U;
            rendered.insert(dollar_position, "$");
            return Value{std::move(rendered)};
        }
        if (lowered_style == "percent") {
            const double scaled = widened * 100.0;
            if (!std::isfinite(scaled)) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{render_fixed_style(scaled, false) + "%"};
        }
        if (lowered_style == "scientific") {
            return Value{render_scientific_style(widened)};
        }
        // Any Style that names none of the reserved styles above is a
        // custom numeric picture (REQ-0218), matching real VB6: it is
        // never rejected outright, only rendered character by
        // character (retiring the old WFC0102 "unsupported Style"
        // diagnostic this branch used to report unconditionally).
        return Value{render_custom_numeric_picture(widened, *style)};
    }

    if (is_rnd) {
        double argument{};
        if (!arguments.empty()) {
            if (!is_number(arguments[0]) &&
                !std::holds_alternative<bool>(arguments[0])) {
                set_error("WFC0073", "Rnd requires a numeric argument",
                          identifier_offset);
                return std::nullopt;
            }
            argument = std::holds_alternative<bool>(arguments[0])
                           ? (std::get<bool>(arguments[0]) ? -1.0 : 0.0)
                           : as_double(arguments[0]);
        }
        if (!execute_) {
            return Value{0.0f};
        }
        if (!arguments.empty() && argument == 0.0) {
            return Value{rnd_last_value_};
        }
        rnd_state_ = (!arguments.empty() && argument < 0.0)
                         ? seed_from_number(argument)
                         : rnd_step(rnd_state_);
        // REQ-0195's own Scope explicitly deferred this: real VB6's Rnd
        // returns Single, not Double. Narrowing rnd_value's double
        // result to float here (rather than computing state / 2^24 in
        // float from the start) is exact for this specific case --
        // dividing by a power of two is exact/correctly-rounded in
        // both precisions, and rounding a correctly-rounded double
        // result to the nearest float gives the same answer a genuine
        // single-precision division would.
        rnd_last_value_ = static_cast<float>(rnd_value(rnd_state_));
        return Value{rnd_last_value_};
    }

    if (is_cstr) {
        // Verified: CStr(Null), like CBool(Null), raises "Invalid use of
        // Null" rather than returning a string.
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
        if (is_object_reference(arguments[0])) {
            set_error("WFC0106", "Invalid use of Nothing", identifier_offset);
            return std::nullopt;
        }
        if (std::holds_alternative<ArrayValue>(arguments[0])) {
            set_error("WFC0073", "CStr does not accept an array argument",
                      identifier_offset);
            return std::nullopt;
        }
        return Value{execute_ ? render(arguments[0]) : std::string{}};
    }

    if (is_typename) {
        if (!execute_) {
            return Value{std::string{}};
        }
        if (std::holds_alternative<Integer>(arguments[0])) {
            return Value{std::string{"Long"}};
        }
        if (std::holds_alternative<Int16>(arguments[0])) {
            return Value{std::string{"Integer"}};
        }
        if (std::holds_alternative<double>(arguments[0])) {
            return Value{std::string{"Double"}};
        }
        if (std::holds_alternative<float>(arguments[0])) {
            return Value{std::string{"Single"}};
        }
        if (std::holds_alternative<Currency>(arguments[0])) {
            return Value{std::string{"Currency"}};
        }
        if (std::holds_alternative<Byte>(arguments[0])) {
            return Value{std::string{"Byte"}};
        }
        if (std::holds_alternative<ErrorValue>(arguments[0])) {
            return Value{std::string{"Error"}};
        }
        if (std::holds_alternative<DateValue>(arguments[0])) {
            return Value{std::string{"Date"}};
        }
        if (std::holds_alternative<Decimal>(arguments[0])) {
            return Value{std::string{"Decimal"}};
        }
        if (std::holds_alternative<bool>(arguments[0])) {
            return Value{std::string{"Boolean"}};
        }
        // Verified against the local VB6 6.00.8176 reference:
        // TypeName(Null) = "Null", TypeName(Empty) = "Empty".
        if (std::holds_alternative<Null>(arguments[0])) {
            return Value{std::string{"Null"}};
        }
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{std::string{"Empty"}};
        }
        // Verified VB6 fact: TypeName(Nothing) = "Nothing".
        if (std::holds_alternative<Nothing>(arguments[0])) {
            return Value{std::string{"Nothing"}};
        }
        // A live instance's TypeName is its own class's name (its
        // as-supplied spelling, not the lowercased lookup key).
        if (const auto* instance = std::get_if<ObjectInstance>(&arguments[0])) {
            const auto& shown =
                class_definitions_.at(instance->data->class_name).display_name;
            if (shown == "WfcDictionary") {
                return Value{std::string{"Dictionary"}};
            }
            if (shown == "WfcFileSystemObject") {
                return Value{std::string{"FileSystemObject"}};
            }
            if (shown == "WfcTextStream") {
                return Value{std::string{"TextStream"}};
            }
            if (shown == "WfcFile") {
                return Value{std::string{"File"}};
            }
            if (shown == "WfcRegExp") {
                return Value{std::string{"RegExp"}};
            }
            if (shown == "WfcMatchCollection") {
                return Value{std::string{"MatchCollection"}};
            }
            if (shown == "WfcMatch") {
                return Value{std::string{"Match"}};
            }
            if (shown == "WfcSubMatches") {
                return Value{std::string{"SubMatches"}};
            }
            return Value{shown};
        }
        if (const auto* array = std::get_if<ArrayValue>(&arguments[0])) {
            // Real VB6 renders an array's TypeName as its element type
            // name plus "()", e.g. "Long()", "Variant()", "Object()".
            // Uses the array's declared element type (not its current
            // first element, which may not exist for an unallocated
            // dynamic array, and which -- for a Variant/Object-element
            // array, REQ-0212 -- is not fixed at all).
            if (array->is_variant_element) {
                return Value{std::string{"Variant()"}};
            }
            if (array->is_object_element) {
                // A class-typed array element (REQ-0214) renders its
                // own class's display name, the same as a live
                // instance does; the generic `As Object` form (no
                // declared class) renders "Object()".
                if (!array->element_class_name.empty()) {
                    return Value{
                        class_definitions_.at(array->element_class_name)
                            .display_name +
                        "()"};
                }
                return Value{std::string{"Object()"}};
            }
            return Value{element_type_name(
                             array_element_default(array->element_type_index)) +
                         "()"};
        }
        return Value{std::string{"String"}};
    }

    if (is_vartype) {
        if (!execute_) {
            return Value{Integer{}};
        }
        if (std::holds_alternative<Integer>(arguments[0])) {
            return Value{Integer{3}};
        }
        if (std::holds_alternative<Int16>(arguments[0])) {
            return Value{Integer{2}};  // vbInteger
        }
        if (std::holds_alternative<double>(arguments[0])) {
            return Value{Integer{5}};
        }
        if (std::holds_alternative<float>(arguments[0])) {
            return Value{Integer{4}};
        }
        if (std::holds_alternative<Currency>(arguments[0])) {
            return Value{Integer{6}};
        }
        if (std::holds_alternative<Byte>(arguments[0])) {
            return Value{Integer{17}};
        }
        if (std::holds_alternative<ErrorValue>(arguments[0])) {
            return Value{Integer{10}};
        }
        if (std::holds_alternative<DateValue>(arguments[0])) {
            return Value{Integer{7}};
        }
        if (std::holds_alternative<Decimal>(arguments[0])) {
            return Value{Integer{14}};  // vbDecimal
        }
        if (std::holds_alternative<bool>(arguments[0])) {
            return Value{Integer{11}};
        }
        // Verified against the local VB6 6.00.8176 reference:
        // VarType(Empty) = 0 (vbEmpty), VarType(Null) = 1 (vbNull).
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{Integer{0}};
        }
        if (std::holds_alternative<Null>(arguments[0])) {
            return Value{Integer{1}};
        }
        // Verified VB6 fact: VarType(Nothing) = 9 (vbObject). A live
        // instance is also vbObject: VarType never encodes which class,
        // only that the value is an object reference.
        if (is_object_reference(arguments[0])) {
            return Value{Integer{9}};
        }
        if (const auto* array = std::get_if<ArrayValue>(&arguments[0])) {
            // Real VB6 ORs the element VarType with vbArray (8192). Uses
            // the array's declared element type, the same as TypeName
            // above.
            if (array->is_variant_element) {
                return Value{Integer{12 + 8192}};  // vbVariant Or vbArray
            }
            if (array->is_object_element) {
                return Value{Integer{9 + 8192}};  // vbObject Or vbArray
            }
            return Value{Integer{element_vartype_code(array_element_default(
                                     array->element_type_index)) +
                                 8192}};
        }
        return Value{Integer{8}};
    }

    if (is_iif) {
        bool iif_flag = false;
        const bool* condition = nullptr;
        if (const auto converted = coerce_condition_boolean(
                arguments[0], identifier_offset, "WFC0021",
                "IIf condition must be Boolean")) {
            iif_flag = *converted;
            condition = &iif_flag;
        }
        if (condition == nullptr) {
            return std::nullopt;
        }
        if (!execute_) {
            return arguments[1];
        }
        return *condition ? arguments[1] : arguments[2];
    }

    if (is_choose) {
        const auto* index = std::get_if<Integer>(&arguments[0]);
        if (index == nullptr) {
            set_error("WFC0073", "Choose requires a Long index",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return arguments[1];
        }
        const auto choice_count = static_cast<Integer>(arguments.size() - 1U);
        if (*index < 1 || *index > choice_count) {
            return Value{Null{}};  // VB: an out-of-range index yields Null
        }
        return arguments[static_cast<std::size_t>(*index)];
    }

    if (is_switch) {
        for (std::size_t pair = 0U; pair < arguments.size(); pair += 2U) {
            const auto* condition = std::get_if<bool>(&arguments[pair]);
            if (condition == nullptr && execute_) {
                set_error("WFC0021", "Switch expressions must be Boolean",
                          identifier_offset);
                return std::nullopt;
            }
            if (execute_ && *condition) {
                return arguments[pair + 1U];
            }
        }
        if (!execute_) {
            return arguments[1];
        }
        return Value{Null{}};  // VB: no expression matched
    }

    if (is_isnumeric) {
        if (!execute_) {
            return Value{false};
        }
        // Per documented VB6 behavior (not independently probed this
        // session): IsNumeric(Empty) is True (Empty coerces to 0, a
        // number), and IsNumeric(Null) is False.
        if (std::holds_alternative<Null>(arguments[0]) ||
            is_object_reference(arguments[0]) ||
            std::holds_alternative<ArrayValue>(arguments[0])) {
            return Value{false};
        }
        if (std::holds_alternative<Integer>(arguments[0]) ||
            std::holds_alternative<Int16>(arguments[0]) ||
            std::holds_alternative<bool>(arguments[0]) ||
            std::holds_alternative<float>(arguments[0]) ||
            std::holds_alternative<double>(arguments[0]) ||
            std::holds_alternative<Currency>(arguments[0]) ||
            std::holds_alternative<Decimal>(arguments[0]) ||
            std::holds_alternative<Empty>(arguments[0])) {
            return Value{true};
        }

        if (!std::holds_alternative<std::string>(arguments[0])) {
            return Value{false};  // Date, error values, ...
        }
        const auto parsed =
            parse_numeric_string(std::get<std::string>(arguments[0]));
        return Value{parsed.status == NumericStringStatus::valid};
    }

    if (is_cbyte) {
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
        if (!is_number(arguments[0]) &&
            !std::holds_alternative<std::string>(arguments[0]) &&
            !std::holds_alternative<bool>(arguments[0]) &&
            !std::holds_alternative<Empty>(arguments[0])) {
            set_error("WFC0073", "CByte requires a numeric argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{Integer{0}};
        }

        if (const auto* number = std::get_if<Integer>(&arguments[0])) {
            if (*number < 0 || *number > 255) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{*number};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            if (*short_integer < 0 || *short_integer > 255) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{static_cast<Integer>(*short_integer)};
        }
        if (const auto* number = std::get_if<double>(&arguments[0])) {
            return round_double_to_long(*number, 0, 255, identifier_offset);
        }
        if (const auto* single = std::get_if<float>(&arguments[0])) {
            return round_double_to_long(static_cast<double>(*single), 0, 255,
                                        identifier_offset);
        }
        if (std::holds_alternative<Currency>(arguments[0]) ||
            std::holds_alternative<Decimal>(arguments[0])) {
            return round_double_to_long(as_double(arguments[0]), 0, 255,
                                        identifier_offset);
        }
        if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
            return Value{*boolean ? Integer{255} : Integer{0}};
        }

        const auto parsed =
            parse_numeric_string(std::get<std::string>(arguments[0]));
        if (parsed.status == NumericStringStatus::out_of_range) {
            set_error("WFC0009", "integer overflow", identifier_offset);
            return std::nullopt;
        }
        if (parsed.status != NumericStringStatus::valid) {
            set_error("WFC0098", "CByte requires a numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        return round_double_to_long(parsed.value, 0, 255, identifier_offset);
    }

    if (is_error_message) {
        if (!execute_) {
            return Value{std::string{}};
        }
        if (arguments.empty()) {
            return Value{std::string{}};
        }
        const auto* number = std::get_if<Integer>(&arguments[0]);
        if (number == nullptr) {
            set_error("WFC0073", "Error requires a Long argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (*number < 0 || *number > 65535) {
            set_error("WFC0101", "Error number is outside the valid range",
                      identifier_offset);
            return std::nullopt;
        }
        return Value{vb_error_description(*number)};
    }

    if (is_cvar) {
        return arguments[0];
    }

    if (is_cdbl || is_csng) {
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
        if (is_object_reference(arguments[0]) ||
            std::holds_alternative<ArrayValue>(arguments[0])) {
            set_error("WFC0073",
                      is_cdbl ? "CDbl requires a numeric value"
                              : "CSng requires a numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        double value{};
        if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
            value = static_cast<double>(*integer);
        } else if (const auto* short_integer =
                       std::get_if<Int16>(&arguments[0])) {
            value = static_cast<double>(*short_integer);
        } else if (const auto* number = std::get_if<double>(&arguments[0])) {
            value = *number;
        } else if (const auto* single = std::get_if<float>(&arguments[0])) {
            value = static_cast<double>(*single);
        } else if (std::holds_alternative<Currency>(arguments[0]) ||
                   std::holds_alternative<Decimal>(arguments[0])) {
            value = as_double(arguments[0]);
        } else if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
            value = *boolean ? -1.0 : 0.0;
        } else if (std::holds_alternative<Empty>(arguments[0])) {
            value = 0.0;
        } else {
            if (!execute_) {
                return is_csng ? Value{0.0f} : Value{0.0};
            }
            const auto parsed =
                parse_numeric_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0095",
                          is_cdbl ? "CDbl requires a numeric value"
                                  : "CSng requires a numeric value",
                          identifier_offset);
                return std::nullopt;
            }
            value = parsed.value;
        }
        if (!execute_) {
            return is_csng ? Value{0.0f} : Value{0.0};
        }
        if (is_csng) {
            const auto narrowed = static_cast<float>(value);
            if (!std::isfinite(narrowed)) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{narrowed};
        }
        return Value{value};
    }

    if (is_ccur) {
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
        if (is_object_reference(arguments[0]) ||
            std::holds_alternative<ArrayValue>(arguments[0])) {
            set_error("WFC0073", "CCur requires a numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        if (const auto* integer = std::get_if<Integer>(&arguments[0])) {
            if (!execute_) {
                return Value{Currency{}};
            }
            return Value{Currency{static_cast<std::int64_t>(*integer) * 10000}};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            if (!execute_) {
                return Value{Currency{}};
            }
            return Value{
                Currency{static_cast<std::int64_t>(*short_integer) * 10000}};
        }
        if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
            return Value{execute_ ? *currency : Currency{}};
        }
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{Currency{}};
        }
        double value{};
        if (const auto* number = std::get_if<double>(&arguments[0])) {
            value = *number;
        } else if (const auto* single = std::get_if<float>(&arguments[0])) {
            value = static_cast<double>(*single);
        } else if (std::holds_alternative<Decimal>(arguments[0])) {
            value = as_double(arguments[0]);
        } else if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
            value = *boolean ? -1.0 : 0.0;
        } else {
            if (!execute_) {
                return Value{Currency{}};
            }
            const auto parsed =
                parse_numeric_string(std::get<std::string>(arguments[0]));
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0103", "CCur requires a numeric value",
                          identifier_offset);
                return std::nullopt;
            }
            value = parsed.value;
        }
        if (!execute_) {
            return Value{Currency{}};
        }
        const auto scaled = currency_from_double(value);
        if (!scaled.has_value()) {
            set_error("WFC0009", "numeric overflow", identifier_offset);
            return std::nullopt;
        }
        return Value{Currency{*scaled}};
    }

    if (is_cint) {
        // CInt returns a genuine Int16 (VB6 Integer), not a Long narrowed
        // to the Integer range but still typed Long.
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
        if (is_object_reference(arguments[0]) ||
            std::holds_alternative<ArrayValue>(arguments[0])) {
            set_error("WFC0088", "CInt requires a numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{execute_ ? Int16{0} : Int16{}};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            return Value{execute_ ? *short_integer : Int16{}};
        }
        if (const auto* number = std::get_if<Integer>(&arguments[0])) {
            if (!execute_) {
                return Value{Int16{}};
            }
            if (*number < std::numeric_limits<Int16>::min() ||
                *number > std::numeric_limits<Int16>::max()) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{static_cast<Int16>(*number)};
        }
        if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
            return Value{execute_ && *boolean ? Int16{-1} : Int16{0}};
        }
        if (const auto* number = std::get_if<double>(&arguments[0])) {
            if (!execute_) {
                return Value{Int16{}};
            }
            return round_double_to_short_integer(*number, identifier_offset);
        }
        if (const auto* single = std::get_if<float>(&arguments[0])) {
            if (!execute_) {
                return Value{Int16{}};
            }
            return round_double_to_short_integer(static_cast<double>(*single),
                                                 identifier_offset);
        }
        if (std::holds_alternative<Currency>(arguments[0]) ||
            std::holds_alternative<Decimal>(arguments[0])) {
            if (!execute_) {
                return Value{Int16{}};
            }
            return round_double_to_short_integer(as_double(arguments[0]),
                                                 identifier_offset);
        }
        if (!execute_) {
            return Value{Int16{}};
        }

        const auto parsed =
            parse_numeric_string(std::get<std::string>(arguments[0]));
        if (parsed.status == NumericStringStatus::out_of_range) {
            set_error("WFC0009", "integer overflow", identifier_offset);
            return std::nullopt;
        }
        if (parsed.status != NumericStringStatus::valid) {
            set_error("WFC0088", "CInt requires a numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        return round_double_to_short_integer(parsed.value, identifier_offset);
    }

    if (is_clng) {
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
        if (is_object_reference(arguments[0]) ||
            std::holds_alternative<ArrayValue>(arguments[0])) {
            set_error("WFC0086", "CLng requires a numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{execute_ ? Integer{0} : Integer{}};
        }
        if (const auto* number = std::get_if<Integer>(&arguments[0])) {
            return Value{execute_ ? *number : Integer{}};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            return Value{execute_ ? static_cast<Integer>(*short_integer)
                                  : Integer{}};
        }
        if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
            return Value{execute_ && *boolean ? Integer{-1} : Integer{0}};
        }
        if (const auto* number = std::get_if<double>(&arguments[0])) {
            if (!execute_) {
                return Value{Integer{}};
            }
            return round_double_to_long(
                *number, std::numeric_limits<Integer>::min(),
                std::numeric_limits<Integer>::max(), identifier_offset);
        }
        if (const auto* single = std::get_if<float>(&arguments[0])) {
            if (!execute_) {
                return Value{Integer{}};
            }
            return round_double_to_long(static_cast<double>(*single),
                                        std::numeric_limits<Integer>::min(),
                                        std::numeric_limits<Integer>::max(),
                                        identifier_offset);
        }
        if (std::holds_alternative<Currency>(arguments[0]) ||
            std::holds_alternative<Decimal>(arguments[0])) {
            if (!execute_) {
                return Value{Integer{}};
            }
            return round_double_to_long(
                as_double(arguments[0]), std::numeric_limits<Integer>::min(),
                std::numeric_limits<Integer>::max(), identifier_offset);
        }
        if (!execute_) {
            return Value{Integer{}};
        }

        const auto parsed =
            parse_numeric_string(std::get<std::string>(arguments[0]));
        if (parsed.status == NumericStringStatus::out_of_range) {
            set_error("WFC0009", "integer overflow", identifier_offset);
            return std::nullopt;
        }
        if (parsed.status != NumericStringStatus::valid) {
            set_error("WFC0086", "CLng requires a numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        return round_double_to_long(
            parsed.value, std::numeric_limits<Integer>::min(),
            std::numeric_limits<Integer>::max(), identifier_offset);
    }

    if (is_cbool) {
        // Verified against the local VB6 6.00.8176 reference:
        // CBool(Null) raises "Invalid use of Null"; CBool(Empty) = False.
        if (std::holds_alternative<Null>(arguments[0])) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
        if (is_object_reference(arguments[0]) ||
            std::holds_alternative<ArrayValue>(arguments[0])) {
            set_error("WFC0087", "CBool requires a Boolean or numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        if (std::holds_alternative<Empty>(arguments[0])) {
            return Value{false};
        }
        if (const auto* boolean = std::get_if<bool>(&arguments[0])) {
            return Value{execute_ && *boolean};
        }
        if (const auto* number = std::get_if<Integer>(&arguments[0])) {
            return Value{execute_ && *number != 0};
        }
        if (const auto* short_integer = std::get_if<Int16>(&arguments[0])) {
            return Value{execute_ && *short_integer != 0};
        }
        if (const auto* number = std::get_if<double>(&arguments[0])) {
            return Value{execute_ && *number != 0.0};
        }
        if (const auto* single = std::get_if<float>(&arguments[0])) {
            return Value{execute_ && *single != 0.0f};
        }
        if (const auto* currency = std::get_if<Currency>(&arguments[0])) {
            return Value{execute_ && currency->scaled != 0};
        }
        if (const auto* decimal = std::get_if<Decimal>(&arguments[0])) {
            return Value{execute_ && !is_zero_big(decimal->mantissa)};
        }
        if (!execute_) {
            return Value{false};
        }

        const auto& text = std::get<std::string>(arguments[0]);
        std::size_t first{};
        std::size_t last = text.size();
        while (first < last && (text[first] == ' ' || text[first] == '\t' ||
                                text[first] == '\r' || text[first] == '\n')) {
            ++first;
        }
        while (last > first &&
               (text[last - 1U] == ' ' || text[last - 1U] == '\t' ||
                text[last - 1U] == '\r' || text[last - 1U] == '\n')) {
            --last;
        }
        std::string normalized;
        normalized.reserve(last - first);
        for (std::size_t index = first; index < last; ++index) {
            normalized.push_back(ascii_lower(text[index]));
        }
        if (normalized == "true") {
            return Value{true};
        }
        if (normalized == "false") {
            return Value{false};
        }

        const auto parsed = parse_numeric_string(normalized);
        if (parsed.status != NumericStringStatus::valid) {
            set_error("WFC0087", "CBool requires a Boolean or numeric value",
                      identifier_offset);
            return std::nullopt;
        }
        return Value{parsed.value != 0.0};
    }

    if (is_space) {
        const auto* count = std::get_if<Integer>(&arguments[0]);
        if (count == nullptr) {
            set_error("WFC0073", "Space requires a Long argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        if (*count < 0) {
            set_error("WFC0075", "function length cannot be negative",
                      identifier_offset);
            return std::nullopt;
        }
        if (*count > 268435456) {
            static_cast<void>(
                raise_runtime(7, "Out of memory", identifier_offset));
            return std::nullopt;
        }
        return Value{std::string(static_cast<std::size_t>(*count), ' ')};
    }

    if (is_string) {
        const auto* count = std::get_if<Integer>(&arguments[0]);
        const bool fill_is_code = std::holds_alternative<Integer>(arguments[1]);
        const bool fill_is_text =
            std::holds_alternative<std::string>(arguments[1]);
        if (count == nullptr || (!fill_is_code && !fill_is_text)) {
            set_error("WFC0073",
                      "String requires a Long count and a Long or String fill",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        if (*count < 0) {
            set_error("WFC0075", "function length cannot be negative",
                      identifier_offset);
            return std::nullopt;
        }
        if (*count > 268435456) {
            static_cast<void>(
                raise_runtime(7, "Out of memory", identifier_offset));
            return std::nullopt;
        }
        char fill{};
        std::string wide_fill;
        if (fill_is_code) {
            const auto code = std::get<Integer>(arguments[1]);
            if (code < 0 || code > 255) {
                set_error("WFC0079",
                          "String fill code must be in the character range",
                          identifier_offset);
                return std::nullopt;
            }
            if (code > 127) {
                append_utf8_unit(wide_fill,
                                 ansi_to_unicode(static_cast<unsigned>(code)));
            }
            fill = static_cast<char>(code);
        } else {
            const auto& text = std::get<std::string>(arguments[1]);
            if (text.empty()) {
                set_error("WFC0080", "String requires a non-empty fill String",
                          identifier_offset);
                return std::nullopt;
            }
            fill = text.front();
            if (static_cast<unsigned char>(fill) >= 0x80U) {
                const auto first_unit = to_utf16_units(text).front();
                append_utf8_unit(wide_fill, first_unit);
            }
        }
        if (!wide_fill.empty()) {
            std::string repeated;
            repeated.reserve(wide_fill.size() *
                             static_cast<std::size_t>(*count));
            for (Integer i = 0; i < *count; ++i) {
                repeated += wide_fill;
            }
            return Value{std::move(repeated)};
        }
        return Value{std::string(static_cast<std::size_t>(*count), fill)};
    }

    if (is_instr) {
        const bool has_start = arguments.size() >= 3U;
        const bool has_compare = arguments.size() == 4U;
        Integer start = 1;
        if (has_start) {
            const auto* start_argument = std::get_if<Integer>(&arguments[0]);
            if (start_argument == nullptr) {
                set_error("WFC0073", "InStr start requires a Long argument",
                          identifier_offset);
                return std::nullopt;
            }
            start = *start_argument;
        }
        const std::size_t haystack_index = has_start ? 1U : 0U;
        const auto* haystack =
            std::get_if<std::string>(&arguments[haystack_index]);
        const auto* needle =
            std::get_if<std::string>(&arguments[haystack_index + 1U]);
        if (haystack == nullptr || needle == nullptr) {
            set_error("WFC0073", "InStr requires String arguments",
                      identifier_offset);
            return std::nullopt;
        }
        const auto* compare_method =
            has_compare ? std::get_if<Integer>(&arguments[3]) : nullptr;
        if (has_compare && compare_method == nullptr) {
            set_error("WFC0073", "InStr compare requires a Long argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        bool text_compare = option_compare_text_;
        if (compare_method != nullptr) {
            if (*compare_method < -1 || *compare_method > 1) {
                set_error("WFC0081", "unsupported comparison method",
                          identifier_offset);
                return std::nullopt;
            }
            text_compare = *compare_method == -1 ? option_compare_text_
                                                 : *compare_method >= 1;
        }
        if (start < 1) {
            set_error("WFC0076", "InStr start must be positive",
                      identifier_offset);
            return std::nullopt;
        }
        if (identifier == "instrb") {
            const std::string hay_bytes = string_to_bytes(*haystack);
            const std::string needle_bytes = string_to_bytes(*needle);
            const auto byte_begin = static_cast<std::size_t>(start - 1);
            if (byte_begin > hay_bytes.size()) {
                return Value{Integer{0}};
            }
            if (needle_bytes.empty()) {
                return Value{byte_begin < hay_bytes.size()
                                 ? static_cast<Integer>(start)
                                 : Integer{0}};
            }
            const auto byte_found = hay_bytes.find(needle_bytes, byte_begin);
            return Value{byte_found == std::string::npos
                             ? Integer{0}
                             : static_cast<Integer>(byte_found + 1U)};
        }
        if (!is_ascii_text(*haystack) || !is_ascii_text(*needle)) {
            auto hay_units = to_utf16_units(*haystack);
            auto needle_units = to_utf16_units(*needle);
            if (text_compare) {
                for (auto& unit : hay_units) {
                    unit = unit_to_lower(unit);
                }
                for (auto& unit : needle_units) {
                    unit = unit_to_lower(unit);
                }
            }
            const auto unit_begin = static_cast<std::size_t>(start - 1);
            if (unit_begin > hay_units.size()) {
                return Value{Integer{0}};
            }
            if (needle_units.empty()) {
                return Value{unit_begin < hay_units.size()
                                 ? static_cast<Integer>(start)
                                 : Integer{0}};
            }
            const auto unit_found = hay_units.find(needle_units, unit_begin);
            return Value{unit_found == std::u16string::npos
                             ? Integer{0}
                             : static_cast<Integer>(unit_found + 1U)};
        }
        const auto begin = static_cast<std::size_t>(start - 1);
        if (begin > haystack->size()) {
            return Value{Integer{0}};
        }
        if (needle->empty()) {
            return Value{begin < haystack->size() ? static_cast<Integer>(start)
                                                  : Integer{0}};
        }
        std::size_t found{};
        if (text_compare) {
            std::string lowered_haystack = *haystack;
            std::string lowered_needle = *needle;
            for (char& character : lowered_haystack) {
                character = ascii_lower(character);
            }
            for (char& character : lowered_needle) {
                character = ascii_lower(character);
            }
            found = lowered_haystack.find(lowered_needle, begin);
        } else {
            found = haystack->find(*needle, begin);
        }
        if (found == std::string::npos) {
            return Value{Integer{0}};
        }
        return Value{static_cast<Integer>(found + 1U)};
    }

    if (is_instr_rev) {
        const auto* haystack = std::get_if<std::string>(&arguments[0]);
        const auto* needle = std::get_if<std::string>(&arguments[1]);
        const bool has_start = arguments.size() >= 3U;
        const bool has_compare = arguments.size() == 4U;
        const auto* start_argument =
            has_start ? std::get_if<Integer>(&arguments[2]) : nullptr;
        const auto* compare_method =
            has_compare ? std::get_if<Integer>(&arguments[3]) : nullptr;
        if (haystack == nullptr || needle == nullptr) {
            set_error("WFC0073", "InStrRev requires String arguments",
                      identifier_offset);
            return std::nullopt;
        }
        if ((has_start && start_argument == nullptr) ||
            (has_compare && compare_method == nullptr)) {
            set_error("WFC0073",
                      "InStrRev start and compare require Long arguments",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }

        const Integer start = start_argument == nullptr ? -1 : *start_argument;
        if (start < -1 || start == 0) {
            set_error("WFC0083", "InStrRev start must be -1 or positive",
                      identifier_offset);
            return std::nullopt;
        }
        bool text_compare = option_compare_text_;
        if (compare_method != nullptr) {
            if (*compare_method < -1 || *compare_method > 1) {
                set_error("WFC0081", "unsupported comparison method",
                          identifier_offset);
                return std::nullopt;
            }
            text_compare = *compare_method == -1 ? option_compare_text_
                                                 : *compare_method >= 1;
        }
        if (!is_ascii_text(*haystack) || !is_ascii_text(*needle)) {
            auto hay_units = to_utf16_units(*haystack);
            auto needle_units = to_utf16_units(*needle);
            if (text_compare) {
                for (auto& unit : hay_units) {
                    unit = unit_to_lower(unit);
                }
                for (auto& unit : needle_units) {
                    unit = unit_to_lower(unit);
                }
            }
            if (hay_units.empty()) {
                return Value{Integer{0}};
            }
            const auto unit_start = start == -1
                                        ? hay_units.size()
                                        : static_cast<std::size_t>(start);
            if (unit_start > hay_units.size()) {
                return Value{Integer{0}};
            }
            if (needle_units.empty()) {
                return Value{static_cast<Integer>(unit_start)};
            }
            if (needle_units.size() > unit_start) {
                return Value{Integer{0}};
            }
            const auto unit_found =
                hay_units.rfind(needle_units, unit_start - needle_units.size());
            return Value{unit_found == std::u16string::npos
                             ? Integer{0}
                             : static_cast<Integer>(unit_found + 1U)};
        }
        if (haystack->empty()) {
            return Value{Integer{0}};
        }

        const auto effective_start =
            start == -1 ? haystack->size() : static_cast<std::size_t>(start);
        if (effective_start > haystack->size()) {
            return Value{Integer{0}};
        }
        if (needle->empty()) {
            return Value{static_cast<Integer>(effective_start)};
        }
        if (needle->size() > effective_start) {
            return Value{Integer{0}};
        }

        std::string searchable = *haystack;
        std::string sought = *needle;
        if (text_compare) {
            for (char& character : searchable) {
                character = ascii_lower(character);
            }
            for (char& character : sought) {
                character = ascii_lower(character);
            }
        }
        const auto latest_start = effective_start - sought.size();
        const auto found = searchable.rfind(sought, latest_start);
        return Value{found == std::string::npos
                         ? Integer{0}
                         : static_cast<Integer>(found + 1U)};
    }

    if (is_strcomp) {
        const auto* left = std::get_if<std::string>(&arguments[0]);
        const auto* right = std::get_if<std::string>(&arguments[1]);
        if (left == nullptr || right == nullptr) {
            set_error("WFC0073", "StrComp requires String arguments",
                      identifier_offset);
            return std::nullopt;
        }
        const bool has_compare = arguments.size() == 3U;
        const auto* compare_method =
            has_compare ? std::get_if<Integer>(&arguments[2]) : nullptr;
        if (has_compare && compare_method == nullptr) {
            set_error("WFC0073", "StrComp compare requires a Long argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        bool text_compare = option_compare_text_;
        if (compare_method != nullptr) {
            if (*compare_method < -1 || *compare_method > 1) {
                set_error("WFC0081", "unsupported comparison method",
                          identifier_offset);
                return std::nullopt;
            }
            text_compare = *compare_method == -1 ? option_compare_text_
                                                 : *compare_method >= 1;
        }
        int comparison{};
        if (text_compare) {
            const std::string lowered_left = fold_case(*left);
            const std::string lowered_right = fold_case(*right);
            comparison = lowered_left.compare(lowered_right);
        } else {
            comparison = left->compare(*right);
        }
        return Value{static_cast<Integer>((comparison > 0) - (comparison < 0))};
    }

    if (is_replace) {
        const auto* expression = std::get_if<std::string>(&arguments[0]);
        const auto* find = std::get_if<std::string>(&arguments[1]);
        const auto* replacement = std::get_if<std::string>(&arguments[2]);
        if (expression == nullptr || find == nullptr ||
            replacement == nullptr) {
            set_error("WFC0073", "Replace requires String arguments",
                      identifier_offset);
            return std::nullopt;
        }
        const bool has_start = arguments.size() >= 4U;
        const bool has_count = arguments.size() >= 5U;
        const bool has_compare = arguments.size() == 6U;
        const auto* start_argument =
            has_start ? std::get_if<Integer>(&arguments[3]) : nullptr;
        const auto* count_argument =
            has_count ? std::get_if<Integer>(&arguments[4]) : nullptr;
        const auto* compare_method =
            has_compare ? std::get_if<Integer>(&arguments[5]) : nullptr;
        if ((has_start && start_argument == nullptr) ||
            (has_count && count_argument == nullptr) ||
            (has_compare && compare_method == nullptr)) {
            set_error(
                "WFC0073",
                "Replace start, count, and compare require Long arguments",
                identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        const Integer start = start_argument == nullptr ? 1 : *start_argument;
        const Integer count = count_argument == nullptr ? -1 : *count_argument;
        if (start < 1) {
            set_error("WFC0076", "Replace start must be positive",
                      identifier_offset);
            return std::nullopt;
        }
        if (count < -1) {
            set_error("WFC0082", "Replace count must be -1 or non-negative",
                      identifier_offset);
            return std::nullopt;
        }
        bool text_compare = option_compare_text_;
        if (compare_method != nullptr) {
            if (*compare_method < -1 || *compare_method > 1) {
                set_error("WFC0081", "unsupported comparison method",
                          identifier_offset);
                return std::nullopt;
            }
            text_compare = *compare_method == -1 ? option_compare_text_
                                                 : *compare_method >= 1;
        }

        const auto begin = static_cast<std::size_t>(start - 1);
        if (begin >= expression->size()) {
            return Value{std::string{}};
        }
        const std::string source = expression->substr(begin);
        if (find->empty() || count == 0) {
            return Value{source};
        }
        std::string haystack = text_compare ? fold_case(source) : source;
        std::string needle = text_compare ? fold_case(*find) : *find;
        std::string result;
        std::size_t position{};
        Integer replacements{};
        while (true) {
            const std::size_t found = haystack.find(needle, position);
            if (found == std::string::npos ||
                (count >= 0 && replacements >= count)) {
                result.append(source, position, std::string::npos);
                break;
            }
            result.append(source, position, found - position);
            result.append(*replacement);
            position = found + needle.size();
            ++replacements;
        }
        return Value{std::move(result)};
    }

    if (is_str) {
        if (!is_number(arguments[0])) {
            set_error("WFC0073", "Str requires a numeric argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        std::string digits = render(arguments[0]);
        if (as_double(arguments[0]) == 0.0) {
            digits = "0";
        }
        if (as_double(arguments[0]) >= 0.0) {
            digits.insert(digits.begin(), ' ');
        }
        return Value{std::move(digits)};
    }

    if (is_hex || is_oct) {
        if (!is_number(arguments[0]) &&
            !std::holds_alternative<std::string>(arguments[0])) {
            set_error("WFC0073",
                      is_hex ? "Hex requires a numeric argument"
                             : "Oct requires a numeric argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        std::optional<Integer> number;
        if (const auto* string = std::get_if<std::string>(&arguments[0])) {
            const auto parsed = parse_numeric_string(*string);
            if (parsed.status == NumericStringStatus::out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            if (parsed.status != NumericStringStatus::valid) {
                set_error("WFC0099",
                          is_hex ? "Hex requires a numeric value"
                                 : "Oct requires a numeric value",
                          identifier_offset);
                return std::nullopt;
            }
            const auto rounded = round_double_to_long(
                parsed.value, std::numeric_limits<Integer>::min(),
                std::numeric_limits<Integer>::max(), identifier_offset);
            if (!rounded.has_value()) {
                return std::nullopt;
            }
            number = std::get<Integer>(*rounded);
        } else {
            number = coerce_long(arguments[0], identifier_offset);
        }
        if (!number.has_value()) {
            return std::nullopt;
        }
        auto magnitude = static_cast<std::uint32_t>(*number);
        if (std::holds_alternative<Int16>(arguments[0])) {
            magnitude &= 0xFFFFU;  // an Integer prints as 16 bits
        }
        if (magnitude == 0U) {
            return Value{std::string{"0"}};
        }
        const std::uint32_t radix = is_hex ? 16U : 8U;
        std::string digits;
        while (magnitude != 0U) {
            const auto value = static_cast<int>(magnitude % radix);
            digits.push_back(value < 10
                                 ? static_cast<char>('0' + value)
                                 : static_cast<char>('A' + (value - 10)));
            magnitude /= radix;
        }
        std::reverse(digits.begin(), digits.end());
        return Value{std::move(digits)};
    }

    if (execute_ && !arguments.empty() &&
        std::holds_alternative<Null>(arguments[0])) {
        // The Variant-returning string functions propagate Null; their `$`
        // forms reject it (error 94).
        static const std::set<std::string, std::less<>> null_propagating = {
            "trim", "ltrim",      "rtrim", "ucase",  "lcase", "left", "right",
            "mid",  "strreverse", "space", "string", "chr",   "chrw"};
        static const std::set<std::string, std::less<>> null_rejecting = {
            "trim$",  "ltrim$", "rtrim$", "ucase$",  "lcase$", "left$",
            "right$", "mid$",   "space$", "string$", "chr$",   "chrw$"};
        const std::string key(identifier);
        if (null_propagating.contains(key)) {
            return Value{Null{}};
        }
        if (null_rejecting.contains(key)) {
            set_error("WFC0104", "Invalid use of Null", identifier_offset);
            return std::nullopt;
        }
    }
    if (is_len && execute_ && std::holds_alternative<Empty>(arguments[0])) {
        return Value{Integer{0}};
    }
    if (is_len && execute_ && std::holds_alternative<Null>(arguments[0])) {
        return Value{Null{}};
    }
    if (is_len && execute_ &&
        !std::holds_alternative<std::string>(arguments[0])) {
        if (const auto* instance = std::get_if<ObjectInstance>(&arguments[0])) {
            if (is_udt_class(instance->data->class_name)) {
                return Value{static_cast<Integer>(udt_byte_size(arguments[0]))};
            }
        }
        if (is_number(arguments[0]) ||
            std::holds_alternative<bool>(arguments[0]) ||
            std::holds_alternative<DateValue>(arguments[0])) {
            return Value{static_cast<Integer>(render(arguments[0]).size())};
        }
    }
    if (is_strconv && execute_ &&
        std::holds_alternative<ArrayValue>(arguments[0])) {
        const auto& bytes = std::get<ArrayValue>(arguments[0]);
        const auto* mode = std::get_if<Integer>(&arguments[1]);
        if (mode != nullptr && *mode == 64 &&
            bytes.element_type_index == Value{Byte{}}.index()) {  // vbUnicode
            std::string text;
            for (const auto& element : bytes.elements) {
                if (const auto* byte = std::get_if<Byte>(&element)) {
                    append_utf8_unit(text, ansi_to_unicode(*byte));
                }
            }
            return Value{std::move(text)};
        }
    }
    const auto* string = std::get_if<std::string>(&arguments[0]);
    static const std::string dry_run_string;
    if (string == nullptr && !execute_) {
        // A placeholder from a not-taken branch (a member of `Nothing`).
        string = &dry_run_string;
    }
    if (string == nullptr) {
        set_error("WFC0073", "function requires a String argument",
                  identifier_offset);
        return std::nullopt;
    }
    if (is_macid) {
        if (!execute_) {
            return Value{Integer{}};
        }
        if (string->size() != 4U) {
            set_error("WFC0100", "MacID requires exactly four bytes",
                      identifier_offset);
            return std::nullopt;
        }
        std::uint32_t packed{};
        for (const unsigned char byte : *string) {
            packed = (packed << 8U) | byte;
        }
        const std::int64_t signed_value =
            packed >= 0x80000000U
                ? static_cast<std::int64_t>(packed) - 0x100000000LL
                : static_cast<std::int64_t>(packed);
        return Value{static_cast<Integer>(signed_value)};
    }
    if (is_len) {
        if (!execute_) {
            return Value{Integer{}};
        }
        if (string->size() >
            static_cast<std::size_t>(std::numeric_limits<Integer>::max())) {
            set_error("WFC0009", "integer overflow", identifier_offset);
            return std::nullopt;
        }
        // LenB counts the bytes of the UTF-16 form; Len counts UTF-16 units.
        return Value{static_cast<Integer>(identifier == "lenb"
                                              ? string_to_bytes(*string).size()
                                              : utf16_length(*string))};
    }

    if (is_asc) {
        if (!execute_) {
            return Value{Integer{}};
        }
        if (string->empty()) {
            set_error("WFC0077", "Asc requires a non-empty String",
                      identifier_offset);
            return std::nullopt;
        }
        if (identifier == "ascb") {
            return Value{static_cast<Integer>(
                static_cast<unsigned char>(string_to_bytes(*string).front()))};
        }
        const auto lead = static_cast<unsigned char>(string->front());
        if (lead < 0x80U) {
            return Value{static_cast<Integer>(lead)};
        }
        const auto units = to_utf16_units(*string);
        const unsigned first_unit = units.front();
        if (identifier == "ascw") {
            // AscW is a signed 16-bit value: units above 32767 are negative.
            return Value{static_cast<Integer>(
                first_unit > 0x7FFFU ? static_cast<int>(first_unit) - 0x10000
                                     : static_cast<int>(first_unit))};
        }
        return Value{static_cast<Integer>(unicode_to_ansi(first_unit))};
    }

    if (is_strconv) {
        const auto* conversion = std::get_if<Integer>(&arguments[1]);
        if (conversion == nullptr) {
            set_error("WFC0073", "StrConv requires a Long conversion argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        if (*conversion == 1 || *conversion == 2 || *conversion == 3) {
            // vbUpperCase, vbLowerCase, vbProperCase (per UTF-16 unit).
            auto units = to_utf16_units(*string);
            const auto is_letter = [](const char16_t c) {
                return unit_to_lower(c) != c || unit_to_upper(c) != c;
            };
            bool word_start = true;
            for (auto& unit : units) {
                if (*conversion == 1) {
                    unit = unit_to_upper(unit);
                } else if (*conversion == 2) {
                    unit = unit_to_lower(unit);
                } else if (is_letter(unit)) {
                    unit =
                        word_start ? unit_to_upper(unit) : unit_to_lower(unit);
                    word_start = false;
                } else {
                    word_start = true;
                }
            }
            return Value{from_utf16_units(units)};
        }
        if (*conversion == 128) {  // vbFromUnicode: the string's ANSI bytes
            ArrayValue bytes{};
            bytes.is_dynamic = true;
            bytes.element_type_index = Value{Byte{}}.index();
            for (const char16_t unit : to_utf16_units(*string)) {
                bytes.elements.emplace_back(
                    static_cast<Byte>(unicode_to_ansi(unit)));
            }
            return Value{std::move(bytes)};
        }
        if (*conversion == 64) {  // vbUnicode on a String: unchanged
            return Value{*string};
        }
        set_error("WFC0093",
                  "StrConv conversion is not supported in the current model",
                  identifier_offset);
        return std::nullopt;
    }

    if (is_val) {
        if (!execute_) {
            return Value{Integer{}};
        }
        std::string compact;
        compact.reserve(string->size());
        for (const char character : *string) {
            if (character != ' ' && character != '\t' && character != '\r' &&
                character != '\n') {
                compact.push_back(character);
            }
        }
        if (compact.empty()) {
            return Value{Integer{0}};
        }

        std::size_t digit_start{};
        if (compact[0] == '+' || compact[0] == '-') {
            digit_start = 1U;
        }
        if (digit_start + 1U < compact.size() && compact[digit_start] == '&' &&
            (compact[digit_start + 1U] == 'h' ||
             compact[digit_start + 1U] == 'H' ||
             compact[digit_start + 1U] == 'o' ||
             compact[digit_start + 1U] == 'O')) {
            const int base = compact[digit_start + 1U] == 'h' ||
                                     compact[digit_start + 1U] == 'H'
                                 ? 16
                                 : 8;
            const std::size_t radix_start = digit_start + 2U;
            std::size_t radix_end = radix_start;
            const auto is_radix_digit = [base](const char character) {
                if (character >= '0' && character <= '7') {
                    return true;
                }
                return base == 16 && ((character >= '8' && character <= '9') ||
                                      (character >= 'a' && character <= 'f') ||
                                      (character >= 'A' && character <= 'F'));
            };
            while (radix_end < compact.size() &&
                   is_radix_digit(compact[radix_end])) {
                ++radix_end;
            }
            if (radix_end == radix_start) {
                return Value{Integer{0}};
            }

            std::uint32_t magnitude{};
            const auto conversion =
                std::from_chars(compact.data() + radix_start,
                                compact.data() + radix_end, magnitude, base);
            if (conversion.ec == std::errc::result_out_of_range) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }

            std::int64_t value = magnitude;
            if (magnitude >= 0x8000U && magnitude <= 0xFFFFU) {
                value -= 0x10000LL;
            } else if (magnitude >= 0x80000000U) {
                value -= 0x100000000LL;
            }
            if (digit_start == 1U && compact[0] == '-') {
                value = -value;
            }
            if (value < std::numeric_limits<Integer>::min() ||
                value > std::numeric_limits<Integer>::max()) {
                set_error("WFC0009", "integer overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{static_cast<Integer>(value)};
        }
        const auto is_digit = [](const char character) {
            return std::isdigit(static_cast<unsigned char>(character)) != 0;
        };
        std::size_t pos = digit_start;
        bool had_int_digits = false;
        while (pos < compact.size() && is_digit(compact[pos])) {
            ++pos;
            had_int_digits = true;
        }
        const std::size_t int_end = pos;
        bool is_float = false;
        bool had_frac_digits = false;
        if (pos < compact.size() && compact[pos] == '.') {
            is_float = true;
            ++pos;
            while (pos < compact.size() && is_digit(compact[pos])) {
                ++pos;
                had_frac_digits = true;
            }
        }
        if ((had_int_digits || had_frac_digits) && pos < compact.size() &&
            (compact[pos] == 'e' || compact[pos] == 'E')) {
            std::size_t exponent = pos + 1U;
            if (exponent < compact.size() &&
                (compact[exponent] == '+' || compact[exponent] == '-')) {
                ++exponent;
            }
            if (exponent < compact.size() && is_digit(compact[exponent])) {
                is_float = true;
                pos = exponent;
                while (pos < compact.size() && is_digit(compact[pos])) {
                    ++pos;
                }
            }
        }
        if (!had_int_digits && !had_frac_digits) {
            return Value{Integer{0}};
        }

        const std::size_t conversion_start = compact[0] == '+' ? 1U : 0U;
        if (is_float) {
            double value{};
            const auto conversion = std::from_chars(
                compact.data() + conversion_start, compact.data() + pos, value);
            if (conversion.ec == std::errc::result_out_of_range) {
                set_error("WFC0009", "numeric overflow", identifier_offset);
                return std::nullopt;
            }
            return Value{value};
        }
        Integer result{};
        const auto conversion =
            std::from_chars(compact.data() + conversion_start,
                            compact.data() + int_end, result);
        if (conversion.ec == std::errc::result_out_of_range) {
            set_error("WFC0009", "integer overflow", identifier_offset);
            return std::nullopt;
        }
        return Value{result};
    }

    if (is_left || is_right) {
        const auto* length = std::get_if<Integer>(&arguments[1]);
        if (length == nullptr) {
            set_error("WFC0073", "function length requires a Long argument",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        if (*length < 0) {
            set_error("WFC0075", "function length cannot be negative",
                      identifier_offset);
            return std::nullopt;
        }
        const auto requested = static_cast<std::size_t>(*length);
        if (identifier[identifier.size() - 1U] == 'b' ||
            identifier.ends_with("b$")) {
            const std::string bytes = string_to_bytes(*string);
            const auto byte_count = std::min(requested, bytes.size());
            return Value{bytes_to_string(
                is_left ? bytes.substr(0U, byte_count)
                        : bytes.substr(bytes.size() - byte_count))};
        }
        if (!is_ascii_text(*string)) {
            const auto units = to_utf16_units(*string);
            const auto unit_count = std::min(requested, units.size());
            return Value{from_utf16_units(
                is_left ? std::u16string_view(units).substr(0U, unit_count)
                        : std::u16string_view(units).substr(units.size() -
                                                            unit_count))};
        }
        const auto count = std::min(requested, string->size());
        return Value{is_left ? string->substr(0U, count)
                             : string->substr(string->size() - count)};
    }

    if (is_mid) {
        const auto* start = std::get_if<Integer>(&arguments[1]);
        const auto* length = arguments.size() == 3U
                                 ? std::get_if<Integer>(&arguments[2])
                                 : nullptr;
        if (start == nullptr || (arguments.size() == 3U && length == nullptr)) {
            set_error("WFC0073", "Mid start and length require Long arguments",
                      identifier_offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        if (*start < 1) {
            set_error("WFC0076", "Mid start must be positive",
                      identifier_offset);
            return std::nullopt;
        }
        if (length != nullptr && *length < 0) {
            set_error("WFC0075", "function length cannot be negative",
                      identifier_offset);
            return std::nullopt;
        }

        const auto first = static_cast<std::size_t>(*start - 1);
        if (identifier == "midb" || identifier == "midb$") {
            const std::string bytes = string_to_bytes(*string);
            if (first >= bytes.size()) {
                return Value{std::string{}};
            }
            const auto byte_available = bytes.size() - first;
            const auto byte_count =
                length == nullptr ? byte_available
                                  : std::min(static_cast<std::size_t>(*length),
                                             byte_available);
            return Value{bytes_to_string(bytes.substr(first, byte_count))};
        }
        if (!is_ascii_text(*string)) {
            const auto units = to_utf16_units(*string);
            if (first >= units.size()) {
                return Value{std::string{}};
            }
            const auto unit_available = units.size() - first;
            const auto unit_count =
                length == nullptr ? unit_available
                                  : std::min(static_cast<std::size_t>(*length),
                                             unit_available);
            return Value{from_utf16_units(
                std::u16string_view(units).substr(first, unit_count))};
        }
        if (first >= string->size()) {
            return Value{std::string{}};
        }
        const auto available = string->size() - first;
        const auto count =
            length == nullptr
                ? available
                : std::min(static_cast<std::size_t>(*length), available);
        return Value{string->substr(first, count)};
    }

    if (!execute_) {
        return Value{std::string{}};
    }

    if (is_left_trim || is_right_trim || is_trim) {
        std::size_t first{};
        std::size_t last = string->size();
        if (is_left_trim || is_trim) {
            while (first < last && (*string)[first] == ' ') {
                ++first;
            }
        }
        if (is_right_trim || is_trim) {
            while (last > first && (*string)[last - 1U] == ' ') {
                --last;
            }
        }
        return Value{string->substr(first, last - first)};
    }

    if (is_reverse) {
        if (!is_ascii_text(*string)) {
            auto units = to_utf16_units(*string);
            std::reverse(units.begin(), units.end());
            return Value{from_utf16_units(units)};
        }
        std::string result = *string;
        std::reverse(result.begin(), result.end());
        return Value{std::move(result)};
    }

    if (!is_ascii_text(*string)) {
        auto units = to_utf16_units(*string);
        for (auto& unit : units) {
            unit = is_lower ? unit_to_lower(unit) : unit_to_upper(unit);
        }
        return Value{from_utf16_units(units)};
    }
    std::string result = *string;
    for (char& character : result) {
        character = is_lower ? ascii_lower(character) : ascii_upper(character);
    }
    return Value{std::move(result)};
}

}  // namespace wfc::detail
