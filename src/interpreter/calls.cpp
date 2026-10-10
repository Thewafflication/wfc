// Interpreter: Procedure calls, arguments, class instances, and member access.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

bool Interpreter::parse_raise_event_statement(
    const std::size_t statement_offset) {
    skip_horizontal_whitespace();
    const auto name_offset = offset_;
    char type_character{};
    const auto event_name = parse_identifier(&type_character);
    if (!event_name.has_value() || type_character != '\0') {
        set_error("WFC0011", "expected event name", name_offset);
        return false;
    }
    InstanceData* const source = current_instance();
    const ClassDef* const source_class = current_class_def();
    if (source == nullptr || source_class == nullptr ||
        !source_class->events.contains(*event_name)) {
        set_error("WFC0015", "event is not declared in this class",
                  statement_offset);
        return false;
    }
    std::vector<CallArgument> arguments;
    skip_horizontal_whitespace();
    if (!at_end() && current() == '(') {
        auto parsed = parse_call_argument_list();
        if (!parsed.has_value()) {
            return false;
        }
        arguments = std::move(*parsed);
    }
    if (!execute_) {
        return true;
    }
    const auto sinks = source->event_sinks;  // handlers may resubscribe
    for (const auto& [weak_sink, field_name] : sinks) {
        const auto sink = weak_sink.lock();
        if (sink == nullptr) {
            continue;
        }
        const auto sink_class = class_definitions_.find(sink->class_name);
        if (sink_class == class_definitions_.end()) {
            continue;
        }
        const std::string handler = field_name + "_" + *event_name;
        const auto method = sink_class->second.methods.find(handler);
        if (method == sink_class->second.methods.end()) {
            continue;
        }
        if (!invoke_definition(method->second, handler, arguments,
                               statement_offset, sink_class->second.source,
                               sink.get())
                 .has_value()) {
            return false;
        }
    }
    return true;
}

std::optional<Value> Interpreter::instantiate_class(
    const std::string& class_name, const std::size_t offset) {
    const auto class_iterator = class_definitions_.find(class_name);
    if (class_iterator == class_definitions_.end()) {
        set_error("WFC0134", "unknown class name", offset);
        return std::nullopt;
    }
    if (constant_expression_) {
        set_error("WFC0074", "constant initializer cannot call a procedure",
                  offset);
        return std::nullopt;
    }
    auto instance = std::make_shared<InstanceData>();
    instance->class_name = class_name;
    for (const auto& [constant_name, constant_value] :
         class_iterator->second.constants) {
        instance->fields.variables.emplace(constant_name, constant_value);
        instance->fields.constants.insert(constant_name);
    }
    for (const auto& [field_name, field_def] : class_iterator->second.fields) {
        Value initial_value;
        if (field_def.is_array) {
            Value element_default =
                field_def.is_variant ? Value{Empty{}}
                : field_def.is_object
                    ? Value{Nothing{}}
                    : array_element_default(field_def.type_index);
            const std::size_t element_type_index = element_default.index();
            ArrayValue array{/*elements=*/{},
                             /*lower_bound=*/0,
                             /*is_dynamic=*/field_def.dimensions.empty(),
                             /*is_allocated=*/!field_def.dimensions.empty(),
                             element_type_index,
                             field_def.dimensions.size() > 1U
                                 ? field_def.dimensions
                                 : std::vector<std::pair<Integer, Integer>>{},
                             field_def.is_variant,
                             field_def.is_object,
                             field_def.class_name};
            if (!field_def.dimensions.empty()) {
                std::size_t total = 1U;
                for (const auto& dimension : field_def.dimensions) {
                    total *= static_cast<std::size_t>(dimension.second -
                                                      dimension.first) +
                             1U;
                }
                array.lower_bound = field_def.dimensions.front().first;
                const bool udt_elements = field_def.is_object &&
                                          !field_def.class_name.empty() &&
                                          is_udt_class(field_def.class_name);
                for (std::size_t i = 0; i < total; ++i) {
                    if (udt_elements) {
                        auto nested =
                            instantiate_class(field_def.class_name, offset);
                        if (!nested.has_value()) {
                            return std::nullopt;
                        }
                        array.elements.push_back(std::move(*nested));
                    } else {
                        array.elements.push_back(element_default);
                    }
                }
            }
            instance->fields.variables.emplace(field_name,
                                               Value{std::move(array)});
            continue;
        }
        if (field_def.is_object && !field_def.class_name.empty() &&
            is_udt_class(field_def.class_name)) {
            auto nested = instantiate_class(field_def.class_name, offset);
            if (!nested.has_value()) {
                return std::nullopt;
            }
            initial_value = std::move(*nested);
        } else if (field_def.is_object && field_def.auto_new &&
                   !field_def.class_name.empty()) {
            auto nested = instantiate_class(field_def.class_name, offset);
            if (!nested.has_value()) {
                return std::nullopt;
            }
            initial_value = std::move(*nested);
        } else if (field_def.is_object) {
            initial_value = Value{Nothing{}};
        } else if (field_def.is_variant) {
            initial_value = Value{Empty{}};
        } else if (field_def.fixed_length != 0U) {
            initial_value = Value{std::string(field_def.fixed_length, ' ')};
            instance->fields.fixed_string_lengths[field_name] =
                field_def.fixed_length;
        } else {
            initial_value = zero_value_for_index(field_def.type_index);
        }
        instance->fields.variables.emplace(field_name,
                                           std::move(initial_value));
        if (field_def.is_variant) {
            instance->fields.variant_variables.insert(field_name);
        } else if (field_def.is_object) {
            instance->fields.object_variables.insert(field_name);
            if (!field_def.class_name.empty()) {
                instance->fields.object_class_names.emplace(
                    field_name, field_def.class_name);
            }
        }
    }
    const auto initializer_iterator =
        class_iterator->second.methods.find("class_initialize");
    if (initializer_iterator != class_iterator->second.methods.end()) {
        // invoke_definition's own !execute_ short-circuit already skips
        // actually running the body during a dry-run/type-check-only
        // pass (an unreached If branch, ...), so New still allocates a
        // correctly-typed placeholder instance there without invoking
        // any Class_Initialize side effect.
        if (!invoke_definition(initializer_iterator->second, "class_initialize",
                               {}, offset, class_iterator->second.source,
                               instance.get())
                 .has_value()) {
            return std::nullopt;
        }
    }
    return Value{ObjectInstance{std::move(instance)}};
}

std::optional<CallArgument> Interpreter::parse_call_argument() {
    skip_horizontal_whitespace();
    if (!at_end() && is_identifier_start(current())) {
        // `name:=value`
        const auto before_name = offset_;
        char name_type_character{};
        auto argument_name = parse_identifier(&name_type_character);
        skip_horizontal_whitespace();
        if (argument_name.has_value() && name_type_character == '\0' &&
            !at_end() && current() == ':' && peek(1) == '=') {
            offset_ += 2;
            auto named = parse_call_argument();
            if (named.has_value()) {
                named->name = std::move(*argument_name);
            }
            return named;
        }
        offset_ = before_name;
    }
    if (!at_end() && (is_identifier_start(current()) || at_with_member())) {
        const auto saved_offset = offset_;
        char type_character{};
        auto identifier = parse_identifier(&type_character);
        skip_horizontal_whitespace();
        const bool bare_candidate =
            type_character == '\0' &&
            (at_end() || current() == ',' || current() == ')' ||
             current() == '\r' || current() == '\n' || current() == ':' ||
             current() == '\'');
        if (bare_candidate) {
            const auto variable = find_variable(*identifier);
            if (variable.value != nullptr) {
                return CallArgument{*variable.value, variable.value};
            }
        } else if (type_character == '\0' && execute_ && !at_end() &&
                   (current() == '(' || current() == '.')) {
            // `arr(i)`, `rec.Field`, `objs(i).Field` alone in an argument slot
            // are ByRef targets too.
            const auto variable = find_variable(*identifier);
            if (variable.value != nullptr &&
                (std::holds_alternative<ArrayValue>(*variable.value) ||
                 std::holds_alternative<ObjectInstance>(*variable.value))) {
                const auto saved_error = error_;
                offset_ = saved_offset;
                LValue target;
                if (parse_lvalue_path(target)) {
                    skip_horizontal_whitespace();
                    if (target.ptr != nullptr &&
                        (at_end() || current() == ',' || current() == ')' ||
                         current() == '\r' || current() == '\n' ||
                         current() == ':' || current() == '\'')) {
                        return CallArgument{*target.ptr, target.ptr};
                    }
                }
                error_ = saved_error;
            }
        }
        offset_ = saved_offset;
    }
    auto value = parse_expression();
    if (!value.has_value()) {
        return std::nullopt;
    }
    return CallArgument{std::move(*value), nullptr};
}

bool Interpreter::run_procedure_body(const std::size_t body_end) {
    while (true) {
        skip_program_leading_trivia();
        if (offset_ >= body_end || at_end()) {
            return true;
        }
        if (!parse_statement()) {
            if (take_pending_jump()) {
                continue;
            }
            return false;
        }
        if (!consume_statement_end()) {
            return false;
        }
        if (exit_sub_requested_ || exit_function_requested_) {
            exit_sub_requested_ = false;
            exit_function_requested_ = false;
            return true;
        }
    }
}

std::optional<std::vector<CallArgument>>
Interpreter::parse_call_argument_list() {
    // REQ-0243: `Name a, b` / `obj.Method a, b` (no parentheses).
    const bool bare_arguments = bare_call_arguments_;
    bare_call_arguments_ = false;
    skip_horizontal_whitespace();
    if (bare_arguments) {
        std::vector<CallArgument> arguments;
        while (true) {
            skip_horizontal_whitespace();
            std::optional<CallArgument> argument;
            if (!at_end() && current() == ',') {
                argument = CallArgument{Value{Empty{}}, nullptr,
                                        true};  // omitted slot
            } else {
                argument = parse_call_argument();
            }
            if (!argument.has_value()) {
                return std::nullopt;
            }
            arguments.push_back(std::move(*argument));
            skip_horizontal_whitespace();
            if (!consume(',')) {
                return arguments;
            }
            skip_horizontal_whitespace();
        }
    }
    if (!consume('(')) {
        return std::vector<CallArgument>{};
    }
    skip_horizontal_whitespace();
    std::vector<CallArgument> arguments;
    if (!consume(')')) {
        while (true) {
            skip_horizontal_whitespace();
            std::optional<CallArgument> argument;
            if (!at_end() && (current() == ',' || current() == ')')) {
                argument = CallArgument{Value{Empty{}}, nullptr, true};
            } else {
                argument = parse_call_argument();
            }
            if (!argument.has_value()) {
                return std::nullopt;
            }
            arguments.push_back(std::move(*argument));
            skip_horizontal_whitespace();
            if (consume(')')) {
                break;
            }
            if (!consume(',')) {
                set_error("WFC0005", "expected closing parenthesis", offset_);
                return std::nullopt;
            }
            skip_horizontal_whitespace();
        }
    }
    return arguments;
}

std::optional<Value> Interpreter::invoke_definition(
    const ProcedureDef& definition, const std::string& binding_name,
    std::vector<CallArgument> arguments, const std::size_t identifier_offset,
    const std::string_view body_source, InstanceData* const instance) {
    // REQ-0206: Optional parameters make the required argument count a
    // range rather than a fixed number; a trailing ParamArray removes
    // the upper bound entirely (every argument from its position
    // onward is collected into it, including zero of them).
    if (definition.is_external) {
        if (!execute_) {
            return Value{Empty{}};
        }
        // An ANSI/Unicode (`A`/`W`) export suffix selects the same emulation.
        std::string export_name = definition.external_name.empty()
                                      ? binding_name
                                      : definition.external_name;
        if (export_name == "messageboxa" || export_name == "messageboxw") {
            export_name.pop_back();
        }
        // A few ubiquitous Win32 timing calls are emulated natively.
        if (export_name == "gettickcount" || export_name == "timegettime") {
            const auto ticks =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
            return Value{
                static_cast<Integer>(static_cast<std::uint32_t>(ticks))};
        }
        if ((export_name == "queryperformancecounter" ||
             export_name == "queryperformancefrequency") &&
            arguments.size() == 1U && arguments[0].byref_target != nullptr &&
            std::holds_alternative<Currency>(*arguments[0].byref_target)) {
            // A 10 MHz counter; a Currency receives the raw 64-bit count.
            std::int64_t count = 10000000;
            if (export_name == "queryperformancecounter") {
                count = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count() /
                        100;
            }
            *arguments[0].byref_target = Value{Currency{count}};
            return Value{Integer{1}};
        }
        if (export_name == "messagebox" && arguments.size() == 4U) {
            // No UI: answer with the default button of the requested set.
            const auto flags = whole_value(arguments[3].value).value_or(0);
            static const std::array<Integer, 6> defaults{1, 1, 3, 6, 6, 4};
            const auto set = static_cast<std::size_t>(flags & 7);
            return Value{set < defaults.size() ? defaults[set] : Integer{1}};
        }
        if (export_name == "sleep" && arguments.size() == 1U) {
            if (const auto milliseconds = whole_value(arguments[0].value)) {
                if (*milliseconds > 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(*milliseconds));
                }
                return Value{Empty{}};
            }
        }
        static_cast<void>(raise_runtime(453, "Specified DLL function not found",
                                        identifier_offset));
        return std::nullopt;
    }
    const auto& parameters = definition.parameters;
    if (std::any_of(arguments.begin(), arguments.end(),
                    [](const CallArgument& a) { return !a.name.empty(); })) {
        // Bind `name:=value` arguments to their parameter positions.
        std::vector<CallArgument> ordered;
        std::size_t positional = 0;
        while (positional < arguments.size() &&
               arguments[positional].name.empty()) {
            ++positional;
        }
        for (std::size_t i = 0; i < positional; ++i) {
            ordered.push_back(std::move(arguments[i]));
        }
        for (std::size_t i = positional; i < arguments.size(); ++i) {
            if (arguments[i].name.empty()) {
                set_error("WFC0072",
                          "positional argument follows a named argument",
                          identifier_offset);
                return std::nullopt;
            }
            std::size_t slot = parameters.size();
            for (std::size_t k = 0; k < parameters.size(); ++k) {
                if (!parameters[k].is_param_array &&
                    parameters[k].name == arguments[i].name) {
                    slot = k;
                    break;
                }
            }
            if (slot == parameters.size()) {
                static_cast<void>(raise_runtime(448, "Named argument not found",
                                                identifier_offset));
                return std::nullopt;
            }
            while (ordered.size() <= slot) {
                ordered.push_back(CallArgument{Value{Empty{}}, nullptr, true});
            }
            if (!ordered[slot].omitted) {
                set_error("WFC0072", "argument specified more than once",
                          identifier_offset);
                return std::nullopt;
            }
            ordered[slot] = std::move(arguments[i]);
            ordered[slot].name.clear();
            ordered[slot].omitted = false;
        }
        arguments = std::move(ordered);
    }
    const bool has_param_array =
        !parameters.empty() && parameters.back().is_param_array;
    const std::size_t fixed_and_optional_count =
        has_param_array ? parameters.size() - 1U : parameters.size();
    std::size_t required_count = fixed_and_optional_count;
    for (std::size_t index = 0U; index < fixed_and_optional_count; ++index) {
        if (parameters[index].is_optional) {
            required_count = index;
            break;
        }
    }
    if (arguments.size() < required_count ||
        (!has_param_array && arguments.size() > fixed_and_optional_count)) {
        set_error("WFC0072", "procedure received the wrong number of arguments",
                  identifier_offset);
        return std::nullopt;
    }

    if (!execute_) {
        if (!definition.is_function) {
            return Value{Empty{}};
        }
        if (definition.return_is_object) {
            return Value{Nothing{}};
        }
        if (definition.return_is_array) {
            return Value{ArrayValue{
                /*elements=*/{}, /*lower_bound=*/0, /*is_dynamic=*/true,
                /*is_allocated=*/false, definition.return_type_index}};
        }
        return definition.return_is_variant
                   ? Value{Empty{}}
                   : zero_value_for_index(definition.return_type_index);
    }
    // See call_procedure's own identical guard: every nested call
    // recurses through this same C++ function, so unbounded VB6
    // recursion (now including a method calling another method, or
    // itself) must still be bounded to avoid a native stack overflow.
    if (procedure_depth_ >= max_procedure_depth_ || stack_nearly_exhausted()) {
        set_error("WFC0123", "procedure call nesting is too deep",
                  identifier_offset);
        return std::nullopt;
    }

    Scope frame;
    for (std::size_t index = 0U; index < fixed_and_optional_count; ++index) {
        const auto& parameter = parameters[index];
        // An omitted trailing Optional argument binds its own default
        // (or the type's zero value when no `= expr` was written) --
        // this synthesized argument is never a ByRef write-back target,
        // matching a literal/expression argument.
        CallArgument synthesized_argument;
        CallArgument* argument_ptr;
        if (index < arguments.size() && !arguments[index].omitted) {
            argument_ptr = &arguments[index];
        } else {
            if (!parameter.is_optional) {
                static_cast<void>(raise_runtime(449, "Argument not optional",
                                                identifier_offset));
                return std::nullopt;
            }
            // REQ-0224: only an omitted Optional Variant argument
            // with no explicit default is a candidate for IsMissing
            // -- recorded by name now, while it is still known which
            // parameters were actually supplied, since the frame
            // this scope belongs to is the only place IsMissing can
            // later check it from. A Variant Optional parameter that
            // *does* have an explicit default (`Optional x As
            // Variant = 5`) reports IsMissing = False even when
            // omitted in real VB6, matching REQ-0206's own
            // documented fact: the default value counts as having
            // been supplied.
            if (parameter.is_optional && parameter.is_variant &&
                !parameter.has_default) {
                frame.missing_parameter_names.insert(parameter.name);
            }
            synthesized_argument.value =
                parameter.has_default
                    ? parameter.default_value
                    : (parameter.is_variant
                           ? Value{Empty{}}
                           : zero_value_for_index(parameter.type_index));
            argument_ptr = &synthesized_argument;
        }
        auto& argument = *argument_ptr;
        if (parameter.is_object_reference) {
            if (!std::holds_alternative<Nothing>(argument.value) &&
                !std::holds_alternative<ObjectInstance>(argument.value)) {
                set_error("WFC0106",
                          "Object parameter requires an object reference",
                          identifier_offset);
                return std::nullopt;
            }
            // REQ-0228: a specific-class parameter (`As SomeClassName`,
            // `class_name` non-empty) requires the argument to match
            // exactly, the same check a class-typed field/return
            // already applies at its own assignment point. Threading
            // `class_name` into the callee's own `object_class_names`
            // (mirroring a class-typed field/return's identical
            // bookkeeping) means a `Set param = ...` inside the body is
            // then class-checked by the existing, unmodified Set
            // machinery too -- no separate enforcement needed there.
            if (!parameter.class_name.empty() &&
                std::holds_alternative<ObjectInstance>(argument.value) &&
                !class_satisfies(
                    std::get<ObjectInstance>(argument.value).data->class_name,
                    parameter.class_name)) {
                set_error(
                    "WFC0137",
                    "argument does not match the parameter's declared class",
                    identifier_offset);
                return std::nullopt;
            }
            if (!parameter.class_name.empty() &&
                is_udt_class(parameter.class_name) &&
                (parameter.by_val || argument.byref_target == nullptr)) {
                // REQ-0241: a UDT passed ByVal is a private copy.
                if (const auto* udt =
                        std::get_if<ObjectInstance>(&argument.value)) {
                    argument.value =
                        Value{ObjectInstance{clone_udt(*udt->data)}};
                }
            }
            frame.variables.emplace(parameter.name, std::move(argument.value));
            frame.object_variables.insert(parameter.name);
            if (!parameter.class_name.empty()) {
                frame.object_class_names.emplace(parameter.name,
                                                 parameter.class_name);
            }
            continue;
        }
        if (parameter.is_array_parameter) {
            const auto* array = std::get_if<ArrayValue>(&argument.value);
            // REQ-0215: a Variant-/Object-element array parameter
            // matches only an argument array of that same element
            // kind (never a fixed-scalar-typed one, and vice versa);
            // a fixed-type parameter matches only a fixed-type
            // argument array with the same element type.
            const bool element_kind_matches =
                array != nullptr &&
                (parameter.is_variant_array_parameter
                     ? array->is_variant_element
                 : parameter.is_object_array_parameter
                     ? (array->is_object_element &&
                        (parameter.class_name.empty() ||
                         array->element_class_name == parameter.class_name))
                     : (!array->is_variant_element &&
                        !array->is_object_element &&
                        array->element_type_index == parameter.type_index));
            if (!element_kind_matches) {
                set_error("WFC0016", "argument type mismatch",
                          identifier_offset);
                return std::nullopt;
            }
            // An array that is not a variable (a function result such as
            // `Show Split(s)`) is a temporary the procedure may change
            // freely.
            frame.variables.emplace(parameter.name, std::move(argument.value));
            continue;
        }
        if (parameter.is_variant) {
            frame.variables.emplace(parameter.name,
                                    copy_if_udt(std::move(argument.value)));
            frame.variant_variables.insert(parameter.name);
            continue;
        }
        if (argument.byref_target != nullptr && !parameter.by_val &&
            argument.value.index() != parameter.type_index) {
            // VB6: "ByRef argument type mismatch" -- a variable passed by
            // reference must already have the parameter's exact type
            // (REQ-0270); convert it first (CLng(x)) or declare ByVal.
            set_error("WFC0016", "ByRef argument type mismatch",
                      identifier_offset);
            return std::nullopt;
        }
        if (!coerce_numeric_value(argument.value, parameter.type_index,
                                  identifier_offset)) {
            return std::nullopt;
        }
        if (argument.value.index() != parameter.type_index) {
            set_error("WFC0016", "argument type mismatch", identifier_offset);
            return std::nullopt;
        }
        frame.variables.emplace(parameter.name, std::move(argument.value));
    }
    if (has_param_array) {
        const auto& param_array_parameter = parameters.back();
        std::vector<Value> elements;
        for (std::size_t index = fixed_and_optional_count;
             index < arguments.size(); ++index) {
            Value element_value = std::move(arguments[index].value);
            if (!param_array_parameter.is_variant) {
                if (!coerce_numeric_value(element_value,
                                          param_array_parameter.type_index,
                                          identifier_offset)) {
                    return std::nullopt;
                }
                if (element_value.index() != param_array_parameter.type_index) {
                    set_error("WFC0016", "argument type mismatch",
                              identifier_offset);
                    return std::nullopt;
                }
            }
            elements.push_back(std::move(element_value));
        }
        ArrayValue param_array{std::move(elements), 0, /*is_dynamic=*/false,
                               /*is_allocated=*/true,
                               param_array_parameter.type_index};
        param_array.is_variant_element = param_array_parameter.is_variant;
        frame.variables.emplace(param_array_parameter.name,
                                Value{std::move(param_array)});
    }
    if (definition.is_function) {
        frame.is_function_frame = true;
        Value initial_return_value;
        if (definition.return_is_array) {
            // Starts as an unallocated dynamic array (REQ-0216), the
            // same as `Dim identifier() As Type`: the body may either
            // assign a whole array to its own name (`Foo = someArray`)
            // or `ReDim`/`ReDim Preserve` it directly, both through
            // existing, unmodified array machinery.
            ArrayValue returned{
                /*elements=*/{}, /*lower_bound=*/0, /*is_dynamic=*/true,
                /*is_allocated=*/false, definition.return_type_index};
            returned.is_variant_element = definition.return_is_variant;
            returned.is_object_element = definition.return_is_object;
            returned.element_class_name = definition.return_class_name;
            initial_return_value = std::move(returned);
        } else if (definition.return_is_object &&
                   !definition.return_class_name.empty() &&
                   is_udt_class(definition.return_class_name)) {
            auto fresh = instantiate_class(definition.return_class_name,
                                           identifier_offset);
            if (!fresh.has_value()) {
                return std::nullopt;
            }
            initial_return_value = std::move(*fresh);
        } else if (definition.return_is_object) {
            initial_return_value = Value{Nothing{}};
        } else if (definition.return_is_variant) {
            initial_return_value = Value{Empty{}};
        } else if (definition.return_is_array) {
            // Starts as an unallocated dynamic array (REQ-0216), the
            // same as `Dim identifier() As Type`: the body may either
            // assign a whole array to its own name (`Foo = someArray`)
            // or `ReDim`/`ReDim Preserve` it directly, both through
            // existing, unmodified array machinery.
            initial_return_value = ArrayValue{
                /*elements=*/{}, /*lower_bound=*/0, /*is_dynamic=*/true,
                /*is_allocated=*/false, definition.return_type_index};
        } else {
            initial_return_value =
                zero_value_for_index(definition.return_type_index);
        }
        frame.variables.emplace(binding_name, std::move(initial_return_value));
        if (definition.return_is_array) {
            // The array slot is neither a Variant nor an object variable.
        } else if (definition.return_is_variant) {
            frame.variant_variables.insert(binding_name);
        } else if (definition.return_is_object) {
            // The return-value slot behaves exactly like an Object-
            // typed local (REQ-0205): plain `Name = expr` inside the
            // body is rejected (WFC0108, via parse_assignment's
            // existing object_variables check) -- only `Set Name =
            // expr` may assign it, reusing REQ-0200's existing Set
            // machinery (including the class-match check when
            // return_class_name is non-empty) with no new code.
            frame.object_variables.insert(binding_name);
            if (!definition.return_class_name.empty()) {
                frame.object_class_names.emplace(binding_name,
                                                 definition.return_class_name);
            }
        }
    }

    scopes_.push_back(std::move(frame));
    if (instance != nullptr) {
        instance_scopes_.push_back(instance);
    }
    const auto saved_offset = offset_;
    const auto saved_source = source_;
    const auto enclosing_execution = execute_;
    const auto* const enclosing_procedure_def = current_procedure_def_;
    current_procedure_def_ = &definition;
    ++procedure_depth_;
    offset_ = definition.body_start;
    source_ = body_source;
    execute_ = true;
    const bool enclosing_declaration_permission = allow_declarations_;
    allow_declarations_ = true;
    // The callee's statements must not disturb the caller's expression state.
    const bool saved_variant_operand = variant_operand_seen_;
    const bool saved_variant_string = variant_string_seen_;
    const bool saved_variant_number = variant_number_seen_;
    const bool ran_ok = run_procedure_body(definition.body_end);
    variant_operand_seen_ = saved_variant_operand;
    variant_string_seen_ = saved_variant_string;
    variant_number_seen_ = saved_variant_number;
    allow_declarations_ = enclosing_declaration_permission;
    --procedure_depth_;
    execute_ = enclosing_execution;
    offset_ = saved_offset;
    source_ = saved_source;
    current_procedure_def_ = enclosing_procedure_def;
    if (instance != nullptr) {
        instance_scopes_.pop_back();
    }

    // REQ-0206: copy each Static variable's final value in this call's
    // frame back into the procedure's own persistent storage before the
    // frame itself is discarded, so the next call to this same
    // procedure sees it. Done regardless of ran_ok, on the same
    // reasoning Class_Terminate's drain does not run on a failed call:
    // once an error is fatal to the whole program anyway, whether a
    // Static happened to get one more write makes no observable
    // difference, so this simply is not reached for a failed call.
    bool statics_copy_back_ok = true;
    if (ran_ok) {
        for (const auto& name : scopes_.back().static_variable_names) {
            // REQ-0230: the persistent slot being overwritten here may
            // itself be the *last* reference to an ObjectInstance (a
            // `Static` Variant can already hold one via `Set`, REQ-0200)
            // -- for example after `Set v = New C` replaced the frame's
            // own copy mid-call, leaving only this persistent slot
            // holding the original instance. A plain assignment would
            // silently drop that last reference without ever running
            // `Class_Terminate`, the same class of bug REQ-0228 fixed
            // for a ByRef parameter's own write-back.
            auto& persistent =
                (instance != nullptr ? instance->static_scopes[&definition]
                                     : definition.statics)
                    .variables[name];
            if (!terminate_if_last_reference(persistent)) {
                statics_copy_back_ok = false;
                break;
            }
            persistent = scopes_.back().variables.at(name);
        }
    }

    if (!ran_ok || !statics_copy_back_ok) {
        scopes_.pop_back();
        return std::nullopt;
    }

    std::optional<Value> result =
        definition.is_function ? scopes_.back().variables.at(binding_name)
                               : Value{Empty{}};
    // Bounded by fixed_and_optional_count, not arguments.size(): a
    // ParamArray's own collected elements (beyond that point) are
    // always ByVal, with no corresponding `parameters` entry per
    // argument to look up in the first place.
    bool write_back_ok = true;
    for (std::size_t index = 0U;
         index < std::min(arguments.size(), fixed_and_optional_count);
         ++index) {
        const auto& parameter = parameters[index];
        if (!parameter.by_val && arguments[index].byref_target != nullptr) {
            // REQ-0228: an object-typed ByRef parameter makes this
            // write-back reachable for an ObjectInstance for the first
            // time -- terminate the caller's *old* value first (the
            // same call `Set` already makes before overwriting a
            // target elsewhere), so an instance the caller was still
            // holding here does not silently skip `Class_Terminate`
            // when this call replaces it. A no-op for every other
            // value kind, the same as everywhere else this is called.
            if (!terminate_if_last_reference(*arguments[index].byref_target)) {
                write_back_ok = false;
                break;
            }
            *arguments[index].byref_target =
                scopes_.back().variables.at(parameter.name);
        }
    }
    // The return value and any ByRef write-backs above are already
    // copied out, bumping their use_count, so an instance among them
    // correctly survives this drain rather than being (wrongly) treated
    // as going out of scope here.
    const bool drained_ok =
        write_back_ok && drain_scope_instances(scopes_.back());
    scopes_.pop_back();
    if (!drained_ok) {
        return std::nullopt;
    }
    if (result.has_value() && definition.is_function &&
        definition.return_is_variant) {
        note_variant_value(*result);
    }
    return result;
}

std::optional<Value> Interpreter::call_procedure(
    const std::string& name, const std::size_t identifier_offset,
    const bool require_function) {
    const auto definition_iterator = procedures_.find(name);
    const auto& definition = definition_iterator->second;
    if (require_function && !definition.is_function) {
        set_error("WFC0122", "a Sub cannot be used in an expression",
                  identifier_offset);
        return std::nullopt;
    }
    if (constant_expression_) {
        set_error("WFC0074", "constant initializer cannot call a procedure",
                  identifier_offset);
        return std::nullopt;
    }
    auto arguments = parse_call_argument_list();
    if (!arguments.has_value()) {
        return std::nullopt;
    }
    return invoke_definition(definition, name, std::move(*arguments),
                             identifier_offset, main_source_, nullptr);
}

std::optional<Value> Interpreter::parse_procedure_call(
    const std::string& name, const std::size_t identifier_offset) {
    return call_procedure(name, identifier_offset, /*require_function=*/true);
}

InstanceData* Interpreter::current_instance() noexcept {
    return instance_scopes_.empty() ? nullptr : instance_scopes_.back();
}

const ClassDef* Interpreter::current_class_def() {
    auto* const instance = current_instance();
    if (instance == nullptr) {
        return nullptr;
    }
    const auto iterator = class_definitions_.find(instance->class_name);
    return iterator == class_definitions_.end() ? nullptr : &iterator->second;
}

bool Interpreter::member_accessible(const ClassDef& class_def,
                                    const bool is_private,
                                    const bool bypass_for_interface_dispatch) {
    return !is_private || bypass_for_interface_dispatch ||
           current_class_def() == &class_def;
}

std::optional<Value> Interpreter::me_value(const std::size_t offset) {
    auto* const instance = current_instance();
    if (instance == nullptr) {
        set_error("WFC0138", "Me is only valid inside a class member", offset);
        return std::nullopt;
    }
    return Value{ObjectInstance{instance->shared_from_this()}};
}

bool Interpreter::terminate_if_last_reference(Value& value) {
    if (auto* const array = std::get_if<ArrayValue>(&value)) {
        // The objects an array holds are released with the array.
        for (auto& element : array->elements) {
            if (!terminate_if_last_reference(element)) {
                return false;
            }
            if (std::holds_alternative<ObjectInstance>(element)) {
                element = Value{Nothing{}};
            }
        }
        return true;
    }
    const auto* const instance_value = std::get_if<ObjectInstance>(&value);
    if (instance_value == nullptr || instance_value->data.use_count() != 1) {
        return true;
    }
    InstanceData* const instance = instance_value->data.get();
    const auto class_iterator = class_definitions_.find(instance->class_name);
    if (class_iterator != class_definitions_.end()) {
        const auto terminate_iterator =
            class_iterator->second.methods.find("class_terminate");
        if (terminate_iterator != class_iterator->second.methods.end()) {
            if (!invoke_definition(terminate_iterator->second,
                                   "class_terminate", {}, 0,
                                   class_iterator->second.source, instance)
                     .has_value()) {
                return false;
            }
        }
    }
    if (instance->store) {
        for (auto& stored : instance->store->values) {
            if (!terminate_if_last_reference(stored)) {
                return false;
            }
        }
        instance->store->values.clear();
    }
    return drain_scope_instances(instance->fields);
}

bool Interpreter::drain_scope_instances(Scope& scope) {
    for (auto& [name, value] : scope.variables) {
        if (!terminate_if_last_reference(value)) {
            return false;
        }
        if (std::holds_alternative<ObjectInstance>(value)) {
            value = Value{Empty{}};
        }
    }
    return true;
}

std::optional<Value> Interpreter::call_class_method(
    InstanceData& instance, const ClassDef& class_def,
    const std::string& member_name, const std::size_t member_offset,
    const bool require_function, const bool bypass_for_interface_dispatch) {
    const auto method_iterator = class_def.methods.find(member_name);
    if (method_iterator == class_def.methods.end()) {
        set_error("WFC0135", "unknown member", member_offset);
        return std::nullopt;
    }
    if (!member_accessible(class_def, method_iterator->second.is_private,
                           bypass_for_interface_dispatch)) {
        set_error("WFC0142", "member is not accessible outside its class",
                  member_offset);
        return std::nullopt;
    }
    if (require_function && !method_iterator->second.is_function) {
        set_error("WFC0122", "a Sub cannot be used in an expression",
                  member_offset);
        return std::nullopt;
    }
    if (constant_expression_) {
        set_error("WFC0074", "constant initializer cannot call a procedure",
                  member_offset);
        return std::nullopt;
    }
    auto arguments = parse_call_argument_list();
    if (!arguments.has_value()) {
        return std::nullopt;
    }
    return invoke_definition(method_iterator->second, member_name,
                             std::move(*arguments), member_offset,
                             class_def.source, &instance);
}

std::optional<Value> Interpreter::call_default_member(
    const ObjectInstance& holder, const std::size_t member_offset) {
    const auto held_class = class_definitions_.find(holder.data->class_name);
    if (held_class == class_definitions_.end() ||
        held_class->second.default_member.empty()) {
        set_error("WFC0135", "unknown member", member_offset);
        return std::nullopt;
    }
    const auto& default_member = held_class->second.default_member;
    if (held_class->second.methods.contains(default_member)) {
        return call_class_method(*holder.data, held_class->second,
                                 default_member, member_offset,
                                 /*require_function=*/true);
    }
    const auto default_getter =
        held_class->second.property_get.find(default_member);
    if (default_getter != held_class->second.property_get.end()) {
        auto default_arguments = parse_call_argument_list();
        if (!default_arguments.has_value()) {
            return std::nullopt;
        }
        return invoke_definition(default_getter->second, default_member,
                                 std::move(*default_arguments), member_offset,
                                 held_class->second.source, holder.data.get());
    }
    set_error("WFC0135", "unknown member", member_offset);
    return std::nullopt;
}

std::optional<Value> Interpreter::parse_member_access_after_dot(
    const Value base, const std::size_t base_offset,
    const bool require_function, const std::string& via_interface_class) {
    if (!std::holds_alternative<Nothing>(base) &&
        !std::holds_alternative<ObjectInstance>(base)) {
        if (!execute_) {
            // A placeholder from a not-taken branch: parse through it.
            return parse_member_access_after_dot(Value{Nothing{}}, base_offset,
                                                 require_function,
                                                 via_interface_class);
        }
        set_error("WFC0136", "member access requires an object reference",
                  base_offset);
        return std::nullopt;
    }
    skip_horizontal_whitespace();
    const auto member_offset = offset_;
    char type_character{};
    auto member_name = parse_identifier(&type_character);
    if (!member_name.has_value() || type_character != '\0') {
        set_error("WFC0011", "expected member name after '.'", member_offset);
        return std::nullopt;
    }
    if (std::holds_alternative<Nothing>(base)) {
        if (!execute_) {
            // Dry-run parsing of a not-taken branch (REQ-0229): `base`
            // really is `Nothing` here, so unlike every dispatch
            // branch below (which always has a live instance's own
            // class to resolve against, dry-run or not), there is
            // nothing at all to resolve `.member` against. Still
            // parses through an optional `(args)` list, so the source
            // text and each argument's own shape are validated the
            // same as they would be for a live instance, but returns
            // a placeholder `Long` rather than erroring -- matching
            // this evaluator's established convention of not raising
            // a value-dependent runtime error for code that will not
            // actually execute, the same as an arithmetic overflow or
            // division by zero in a dead branch.
            skip_horizontal_whitespace();
            if (!at_end() && current() == '(') {
                if (!parse_call_argument_list().has_value()) {
                    return std::nullopt;
                }
            }
            return Value{Integer{}};
        }
        set_error("WFC0106", "Invalid use of Nothing", base_offset);
        return std::nullopt;
    }
    InstanceData& instance = *std::get<ObjectInstance>(base).data;
    const auto class_iterator = class_definitions_.find(instance.class_name);
    const ClassDef& class_def = class_iterator->second;

    // REQ-0233: dispatch through an interface-typed reference resolves
    // to the implementing class's own `InterfaceName_MemberName`, not
    // the bare member name (real VB6's mandatory interface-member
    // naming convention) -- but only when the live instance's own
    // class actually `Implements` that interface; a generic `Object`
    // reference, or one declared as a concrete class the instance
    // simply is, leaves `member_name` untouched and resolves normally.
    // REQ-0233: a `Private` interface-implementation method/property
    // (real VB6 practice -- `Private Sub IShape_Draw()` is the norm,
    // since it should only ever be reachable through the interface
    // reference, not directly) is accessible here despite being
    // `Private`, since reaching it through the interface reference is
    // exactly the sanctioned way to call it.
    bool dispatched_via_interface = false;
    if (!via_interface_class.empty()) {
        for (const auto& implemented : class_def.implements) {
            if (implemented == via_interface_class) {
                member_name = via_interface_class + "_" + *member_name;
                dispatched_via_interface = true;
                break;
            }
        }
    }

    skip_horizontal_whitespace();
    if (!at_end() && current() == '(') {
        if (class_def.methods.contains(*member_name)) {
            return call_class_method(instance, class_def, *member_name,
                                     member_offset, require_function,
                                     dispatched_via_interface);
        }
        // An indexed Property Get (REQ-0205): `obj.Name(args)` reaches
        // the same parenthesized-call shape a method call would, since
        // a class cannot declare both a method and a property under the
        // same name (see scan_class_body's WFC0128 check).
        const auto indexed_getter_iterator =
            class_def.property_get.find(*member_name);
        if (indexed_getter_iterator != class_def.property_get.end()) {
            if (!member_accessible(class_def,
                                   indexed_getter_iterator->second.is_private,
                                   dispatched_via_interface)) {
                set_error("WFC0142",
                          "member is not accessible outside its class",
                          member_offset);
                return std::nullopt;
            }
            if (constant_expression_) {
                set_error("WFC0074",
                          "constant initializer cannot call a procedure",
                          member_offset);
                return std::nullopt;
            }
            if (indexed_getter_iterator->second.parameters.empty()) {
                // `obj.Prop(i)` where Prop takes no index: apply the
                // parentheses to the object (or array) it returns.
                auto held = invoke_definition(indexed_getter_iterator->second,
                                              *member_name, {}, member_offset,
                                              class_def.source, &instance);
                if (!held.has_value()) {
                    return std::nullopt;
                }
                if (const auto* holder = std::get_if<ObjectInstance>(&*held)) {
                    const auto held_class =
                        class_definitions_.find(holder->data->class_name);
                    if (held_class != class_definitions_.end() &&
                        !held_class->second.default_member.empty()) {
                        const auto& default_member =
                            held_class->second.default_member;
                        if (held_class->second.methods.contains(
                                default_member)) {
                            return call_class_method(
                                *holder->data, held_class->second,
                                default_member, member_offset,
                                /*require_function=*/true);
                        }
                        const auto default_getter =
                            held_class->second.property_get.find(
                                default_member);
                        if (default_getter !=
                            held_class->second.property_get.end()) {
                            auto default_arguments = parse_call_argument_list();
                            if (!default_arguments.has_value()) {
                                return std::nullopt;
                            }
                            return invoke_definition(
                                default_getter->second, default_member,
                                std::move(*default_arguments), member_offset,
                                held_class->second.source, holder->data.get());
                        }
                    }
                } else if (std::holds_alternative<ArrayValue>(*held)) {
                    return parse_array_index(*held);
                }
                set_error("WFC0135", "unknown member", member_offset);
                return std::nullopt;
            }
            auto arguments = parse_call_argument_list();
            if (!arguments.has_value()) {
                return std::nullopt;
            }
            return invoke_definition(indexed_getter_iterator->second,
                                     *member_name, std::move(*arguments),
                                     member_offset, class_def.source,
                                     &instance);
        }
        {
            // REQ-0251: `obj.arrayField(i)`.
            const auto array_field =
                instance.fields.variables.find(*member_name);
            if (array_field != instance.fields.variables.end() &&
                std::holds_alternative<ArrayValue>(array_field->second)) {
                const auto field_def_iterator =
                    class_def.fields.find(*member_name);
                if (field_def_iterator != class_def.fields.end() &&
                    !member_accessible(class_def,
                                       field_def_iterator->second.is_private)) {
                    set_error("WFC0142",
                              "member is not accessible outside its class",
                              member_offset);
                    return std::nullopt;
                }
                return parse_array_index(array_field->second);
            }
            // `obj.collectionField(key)`: the default member of the object
            // the field holds.
            if (array_field != instance.fields.variables.end() &&
                std::holds_alternative<ObjectInstance>(array_field->second)) {
                const auto field_def_iterator =
                    class_def.fields.find(*member_name);
                if (field_def_iterator != class_def.fields.end() &&
                    !member_accessible(class_def,
                                       field_def_iterator->second.is_private)) {
                    set_error("WFC0142",
                              "member is not accessible outside its class",
                              member_offset);
                    return std::nullopt;
                }
                return call_default_member(
                    std::get<ObjectInstance>(array_field->second),
                    member_offset);
            }
        }
        set_error("WFC0135", "unknown member", member_offset);
        return std::nullopt;
    }
    const auto getter_iterator = class_def.property_get.find(*member_name);
    if (getter_iterator != class_def.property_get.end()) {
        if (!member_accessible(class_def, getter_iterator->second.is_private,
                               dispatched_via_interface)) {
            set_error("WFC0142", "member is not accessible outside its class",
                      member_offset);
            return std::nullopt;
        }
        return invoke_definition(getter_iterator->second, *member_name, {},
                                 member_offset, class_def.source, &instance);
    }
    // `obj.Method` / `Call obj.Method` with no parentheses (REQ-0217):
    // a zero-argument dotted method call, checked after Property Get
    // (a class cannot declare both a method and a property under the
    // same name) and before a field, matching the with-parens branch
    // above's own method-then-property-then-unknown order.
    // call_class_method's own parse_call_argument_list tolerates the
    // missing '(' as zero arguments (REQ-0213), rejecting it with
    // WFC0072 if the method actually requires one or more.
    const auto method_iterator = class_def.methods.find(*member_name);
    if (method_iterator != class_def.methods.end()) {
        return call_class_method(instance, class_def, *member_name,
                                 member_offset, require_function,
                                 dispatched_via_interface);
    }
    const auto field_iterator = instance.fields.variables.find(*member_name);
    if (field_iterator != instance.fields.variables.end()) {
        const auto field_def_iterator = class_def.fields.find(*member_name);
        if (field_def_iterator != class_def.fields.end() &&
            !member_accessible(class_def,
                               field_def_iterator->second.is_private)) {
            set_error("WFC0142", "member is not accessible outside its class",
                      member_offset);
            return std::nullopt;
        }
        if (std::holds_alternative<ArrayValue>(field_iterator->second)) {
            const auto after_name = offset_;
            skip_horizontal_whitespace();
            if (!at_end() && current() == '(') {
                return parse_array_index(field_iterator->second);
            }
            offset_ = after_name;
        }
        if (instance.fields.variant_variables.contains(*member_name)) {
            note_variant_value(field_iterator->second);
        }
        return field_iterator->second;
    }
    set_error("WFC0135", "unknown member", member_offset);
    return std::nullopt;
}

}  // namespace wfc::detail
