// Interpreter: Program setup, scopes, lexing primitives, types, and runtime
// errors. Internal to the WFC evaluator; not part of the public API. Split out
// of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

Scope& Interpreter::current_scope() noexcept {
    return scopes_.back();
}

Scope& Interpreter::module_scope() noexcept {
    return scopes_.front();
}

bool Interpreter::in_procedure() const noexcept {
    return scopes_.size() > 1U;
}

VariableLookup Interpreter::find_variable(const std::string& name) {
    auto found = find_variable_raw(name);
    if (found.value != nullptr && execute_ &&
        !found.scope->auto_new_variables.empty() &&
        found.scope->auto_new_variables.contains(name) &&
        std::holds_alternative<Nothing>(*found.value)) {
        const auto declared = found.scope->object_class_names.find(name);
        if (declared != found.scope->object_class_names.end()) {
            const auto saved_offset = offset_;
            auto instance = instantiate_class(declared->second, offset_);
            offset_ = saved_offset;
            if (instance.has_value()) {
                *found.value = std::move(*instance);
            }
        }
    }
    return found;
}

VariableLookup Interpreter::find_variable_raw(const std::string& name) {
    if (name.starts_with("with.")) {
        const auto slot = with_slots_.find(name);
        if (slot != with_slots_.end()) {
            return {&slot->second, &with_scope_};
        }
        return {};
    }
    if (in_procedure()) {
        auto& local = current_scope();
        const auto entry = local.variables.find(name);
        if (entry != local.variables.end()) {
            return {&entry->second, &local};
        }
    }
    if (!instance_scopes_.empty()) {
        auto& fields = instance_scopes_.back()->fields;
        const auto entry = fields.variables.find(name);
        if (entry != fields.variables.end()) {
            return {&entry->second, &fields};
        }
        // Module-level (global) variables and constants are visible from a
        // class member too, after the instance's own fields.
    }
    auto& module = module_scope();
    const auto entry = module.variables.find(name);
    if (entry != module.variables.end()) {
        return {&entry->second, &module};
    }
    if (name == "calendar" && !module.constants.contains(name)) {
        // The VBA `Calendar` property (REQ-0074) reads as vbCalGreg until set.
        const auto created = module.variables.emplace(name, Value{Integer{0}});
        return {&created.first->second, &module};
    }
    return {};
}

bool Interpreter::in_with_identifier(const std::string& name) {
    return name.starts_with("with.");
}

void Interpreter::set_max_procedure_depth(const std::size_t depth) noexcept {
    max_procedure_depth_ = depth;
}

void Interpreter::set_stack_budget(const char* const base,
                                   const std::size_t budget) noexcept {
    stack_base_ = base;
    stack_budget_ = budget;
}

bool Interpreter::stack_nearly_exhausted() const noexcept {
    if (stack_budget_ == 0U) {
        return false;
    }
    char marker = 0;
    const char* const here = &marker;
    return stack_base_ > here &&
           static_cast<std::size_t>(stack_base_ - here) > stack_budget_;
}

wfc::Evaluation Interpreter::evaluate() {
    auto result = evaluate_program_text();
    result.debug_output = debug_output_;
    if (!result.success) {
        result.partial_output =
            output_;  // what the program printed before it failed
        const auto saved_error = std::move(error_);
        error_ = result;
        const Integer number = runtime_error_number();
        if (number != 0) {
            result.vb_error_number = number;
            result.vb_error_description =
                std::string_view(result.diagnostic).substr(0, 7) == "WFC0300"
                    ? err_description_
                    : vb_error_description(number);
        }
        error_ = saved_error;
    }
    return result;
}

void Interpreter::set_vb_number_spacing(const bool enabled) noexcept {
    vb_number_spacing_ = enabled;
}

void Interpreter::set_app_properties(
    std::map<std::string, std::string> properties) {
    app_properties_ = std::move(properties);
}

std::string Interpreter::failing_module_name() const {
    if (error_source_data_ == nullptr ||
        error_source_data_ == main_source_data_) {
        return {};
    }
    for (const auto& [name, definition] : class_definitions_) {
        if (definition.source.data() == error_source_data_) {
            return definition.display_name;
        }
    }
    return {};
}

wfc::Evaluation Interpreter::evaluate_program_text() {
    scan_option_explicit();
    scan_deftypes();
    scan_module_names();
    scan_enum_names();
    scan_udt_types();
    scan_builtin_classes();
    if (!scan_classes()) {
        return std::move(error_);
    }
    register_predeclared_instances();
    if (!scan_procedures()) {
        return std::move(error_);
    }
    skip_program_leading_trivia();
    if (at_end()) {
        return failure("WFC0001", "expected statement", offset_);
    }
    while (!at_end()) {
        if (!parse_statement()) {
            if (take_pending_jump()) {
                skip_program_leading_trivia();
                continue;
            }
            if (end_requested_) {
                error_ = wfc::Evaluation{};
                break;
            }
            return std::move(error_);
        }
        if (!consume_statement_end()) {
            return std::move(error_);
        }
        skip_program_leading_trivia();
    }

    // Terminate any module-level variable still holding the last
    // reference to an instance, the same way a call frame's locals are
    // drained at the end of a call -- but only on this successful
    // completion path; the interpreter is already erroring out on every
    // other return in this function, and running more class-member code
    // during that unwind is more likely to compound the failure than
    // clean up after it.
    if (!drain_scope_instances(module_scope())) {
        return std::move(error_);
    }
    // REQ-0230: a `Static` Variant local (REQ-0206) can already hold an
    // object reference via `Set` (REQ-0200), persisted on its own
    // procedure's/class-member's `ProcedureDef::statics` -- storage
    // this evaluator never previously drained at all, unlike the
    // module scope just above, so `Class_Terminate` would never run
    // for an instance only ever reachable through one at program end.
    for (auto& [name, definition] : procedures_) {
        if (!drain_scope_instances(definition.statics)) {
            return std::move(error_);
        }
    }
    for (auto& [class_name, class_def] : class_definitions_) {
        for (auto& [member_name, definition] : class_def.methods) {
            if (!drain_scope_instances(definition.statics)) {
                return std::move(error_);
            }
        }
        for (auto& [member_name, definition] : class_def.property_get) {
            if (!drain_scope_instances(definition.statics)) {
                return std::move(error_);
            }
        }
        for (auto& [member_name, definition] : class_def.property_let) {
            if (!drain_scope_instances(definition.statics)) {
                return std::move(error_);
            }
        }
        for (auto& [member_name, definition] : class_def.property_set) {
            if (!drain_scope_instances(definition.statics)) {
                return std::move(error_);
            }
        }
    }

    wfc::Evaluation result;
    result.success = true;
    result.output = std::exchange(output_, {});
    return result;
}

bool Interpreter::at_end() const noexcept {
    return offset_ == source_.size();
}

char Interpreter::current() const noexcept {
    return source_[offset_];
}

char Interpreter::peek(const std::size_t ahead) const noexcept {
    const auto index = offset_ + ahead;
    return index < source_.size() ? source_[index] : '\0';
}

void Interpreter::advance() noexcept {
    ++offset_;
}

void Interpreter::skip_horizontal_whitespace() noexcept {
    while (!at_end() && (current() == ' ' || current() == '\t' ||
                         current() == '\f' || current() == '\v')) {
        advance();
    }
}

bool Interpreter::consume_line_break() noexcept {
    if (at_end()) {
        return false;
    }
    if (current() == '\r') {
        advance();
        if (!at_end() && current() == '\n') {
            advance();
        }
        return true;
    }
    if (current() == '\n') {
        advance();
        return true;
    }
    return false;
}

void Interpreter::skip_comment_free_statement_text() noexcept {
    bool in_string = false;
    while (!at_end() && current() != '\r' && current() != '\n') {
        if (current() == '"') {
            in_string = !in_string;
        } else if (!in_string && (current() == ':' || current() == '\'')) {
            break;
        }
        advance();
    }
}

void Interpreter::skip_comment() noexcept {
    while (!at_end() && current() != '\r' && current() != '\n') {
        advance();
    }
}

void Interpreter::skip_program_leading_trivia() noexcept {
    while (true) {
        skip_horizontal_whitespace();
        if (!at_end() && current() == '\'') {
            skip_comment();
        }
        if (!consume_line_break()) {
            return;
        }
    }
}

void Interpreter::skip_rest_of_line() noexcept {
    skip_comment();  // skip_comment already just means "to end of line"
    static_cast<void>(consume_line_break());
}

bool Interpreter::skip_to_matching_end(const std::string_view keyword,
                                       std::size_t& body_end) {
    while (true) {
        skip_program_leading_trivia();
        if (at_end()) {
            return false;
        }
        const auto line_offset = offset_;
        if (consume_keyword("end")) {
            skip_horizontal_whitespace();
            if (consume_keyword(keyword)) {
                body_end = line_offset;
                // Leave the cursor right after "End <keyword>", not
                // past its line break: the caller (parse_statement,
                // when skipping a declaration during the main pass;
                // scan_procedures' own loop, when continuing to scan)
                // is responsible for consuming that statement
                // separator itself, matching every other statement
                // handler in this evaluator.
                return true;
            }
            offset_ = line_offset;
        }
        // `Sub F(): Print 1: End Sub` -- look for a `: End <keyword>` on this
        // line.
        bool in_string = false;
        while (!at_end() && current() != '\r' && current() != '\n') {
            const char ch = current();
            if (ch == '"') {
                in_string = !in_string;
            } else if (!in_string && ch == '\'') {
                break;
            } else if (!in_string && ch == ':') {
                advance();
                skip_horizontal_whitespace();
                const auto end_offset = offset_;
                if (consume_keyword("end")) {
                    skip_horizontal_whitespace();
                    if (consume_keyword(keyword)) {
                        body_end = end_offset;
                        return true;
                    }
                }
                offset_ = end_offset;
                continue;
            }
            advance();
        }
        skip_rest_of_line();
    }
}

bool Interpreter::consume_statement_end() {
    skip_horizontal_whitespace();
    if (pending_next_comma_ && !at_end() && current() == ',') {
        return true;  // `Next j, i`: the enclosing For consumes `, i`.
    }
    if (!at_end() && current() == '\'') {
        skip_comment();
    }
    if (at_end()) {
        return true;
    }
    if (current() == ':') {
        advance();
        return true;
    }
    if (consume_line_break()) {
        return true;
    }
    set_error("WFC0004", "unexpected trailing input", offset_);
    return false;
}

bool Interpreter::consume_case_line_end() {
    skip_horizontal_whitespace();
    if (!at_end() && current() == ':') {
        advance();
        return true;
    }
    return consume_block_line_end();
}

bool Interpreter::consume_loop_header_end() {
    skip_horizontal_whitespace();
    if (!at_end() && current() == ':') {
        advance();
        return true;
    }
    return consume_block_line_end();
}

bool Interpreter::consume_block_line_end() {
    skip_horizontal_whitespace();
    if (!at_end() && current() == '\'') {
        skip_comment();
    }
    if (consume_line_break()) {
        return true;
    }
    set_error("WFC0004", "expected line break", offset_);
    return false;
}

bool Interpreter::consume(const char character) noexcept {
    if (at_end() || current() != character) {
        return false;
    }
    advance();
    return true;
}

bool Interpreter::consume_keyword(const std::string_view keyword) {
    // Fast reject: most calls are made where some other token starts.
    if (keyword.front() != 'l' || enum_names_.empty()) {
        if (offset_ >= source_.size() ||
            ascii_lower(source_[offset_]) != keyword.front()) {
            return false;
        }
    }
    return consume_keyword_slow(keyword);
}

bool Interpreter::consume_keyword_slow(const std::string_view keyword) {
    const auto start = offset_;
    if (!enum_names_.empty() && keyword == "long") {
        // REQ-0237: an Enum's name is a Long-typed type name, optionally
        // qualified by the module or class that declares it (`Cfg.Mode`).
        std::size_t qualifier_end = offset_;
        while (qualifier_end < source_.size() &&
               is_identifier_part(source_[qualifier_end])) {
            ++qualifier_end;
        }
        if (qualifier_end > offset_ && qualifier_end + 1U < source_.size() &&
            source_[qualifier_end] == '.' &&
            is_identifier_start(source_[qualifier_end + 1U])) {
            std::string qualifier;
            for (std::size_t k = offset_; k < qualifier_end; ++k) {
                qualifier.push_back(ascii_lower(source_[k]));
            }
            if (module_names_.contains(qualifier) ||
                class_definitions_.contains(qualifier)) {
                offset_ = qualifier_end + 1U;
            }
        }
        for (const auto& enum_name : enum_names_) {
            std::size_t i = 0;
            while (i < enum_name.size() && offset_ + i < source_.size() &&
                   ascii_lower(source_[offset_ + i]) == enum_name[i]) {
                ++i;
            }
            if (i == enum_name.size() &&
                (offset_ + i == source_.size() ||
                 !is_identifier_part(source_[offset_ + i]))) {
                offset_ += i;
                return true;
            }
        }
        offset_ = start;
    }
    for (const char expected : keyword) {
        if (at_end() || ascii_lower(current()) != expected) {
            offset_ = start;
            return false;
        }
        advance();
    }

    if (!at_end() && is_identifier_part(current())) {
        offset_ = start;
        return false;
    }
    return true;
}

bool Interpreter::at_with_member() const noexcept {
    return !with_names_.empty() && !at_end() && current() == '.' &&
           offset_ + 1 < source_.size() &&
           is_identifier_start(source_[offset_ + 1]);
}

std::optional<std::string> Interpreter::parse_identifier(
    char* const type_character) {
    // REQ-0236: a leading `.member` inside a With block reads as the
    // With object's hidden variable followed by the member access.
    if (at_with_member()) {
        if (type_character != nullptr) {
            *type_character = '\0';
        }
        return with_names_.back();
    }
    if (at_end() || !is_identifier_start(current())) {
        return std::nullopt;
    }

    std::string identifier;
    do {
        identifier.push_back(ascii_lower(current()));
        advance();
    } while (!at_end() && is_identifier_part(current()));
    // `VBA.Left$(...)`, `Strings.Left`, `Conversion.Int`, ...: the library
    // qualifier is implied; drop it (and a second one: `VBA.Strings.Left`).
    for (int qualifiers = 0; qualifiers < 2; ++qualifiers) {
        static const std::set<std::string, std::less<>> libraries = {
            "vba",
            "strings",
            "math",
            "conversion",
            "datetime",
            "interaction",
            "filesystem",
            "information",
            "fileio",
            "financial",
            "constants",
            "globals",
            "vbruntime",
            "scripting",
            "vbscript_regexp_55",
            "vbscript_regexp_10"};
        if (!at_end() && current() == '.' && offset_ + 1 < source_.size() &&
            is_identifier_start(source_[offset_ + 1]) &&
            libraries.contains(identifier) &&
            find_variable(identifier).value == nullptr &&
            !(module_names_.contains(identifier)) &&
            !class_definitions_.contains(identifier) &&
            std::find(enum_names_.begin(), enum_names_.end(), identifier) ==
                enum_names_.end()) {
            advance();
            identifier.clear();
            do {
                identifier.push_back(ascii_lower(current()));
                advance();
            } while (!at_end() && is_identifier_part(current()));
        } else {
            break;
        }
    }
    // REQ-0258: `Module1.Name` -- drop the module qualifier (standard
    // modules share one namespace), unless a variable shadows it.
    if (!module_names_.empty() && !at_end() && current() == '.' &&
        offset_ + 1 < source_.size() &&
        is_identifier_start(source_[offset_ + 1]) &&
        module_names_.contains(identifier) &&
        find_variable(identifier).value == nullptr) {
        advance();
        identifier.clear();
        do {
            identifier.push_back(ascii_lower(current()));
            advance();
        } while (!at_end() && is_identifier_part(current()));
    }
    if (type_character != nullptr) {
        *type_character = '\0';
        if (!at_end() && strchr("$%&!#@", current()) != nullptr) {
            *type_character = current();
            advance();
        }
    }
    return identifier;
}

bool Interpreter::validate_type_character(const char type_character,
                                          const std::size_t identifier_offset) {
    if (type_character == '\0' || type_character == '$' ||
        type_character == '&' || type_character == '#' ||
        type_character == '!' || type_character == '@' ||
        type_character == '%') {
        return true;
    }
    set_error("WFC0097",
              "type-declaration character requires an unsupported value type",
              identifier_offset);
    return false;
}

std::size_t Interpreter::type_character_index(const char type_character) const {
    if (type_character == '$') {
        return Value{std::string{}}.index();
    }
    if (type_character == '#') {
        return Value{0.0}.index();
    }
    if (type_character == '!') {
        return Value{0.0f}.index();
    }
    if (type_character == '@') {
        return Value{Currency{}}.index();
    }
    if (type_character == '%') {
        return Value{Int16{}}.index();
    }
    return Value{Integer{}}.index();
}

auto Interpreter::parse_type_keyword() -> std::optional<TypeKeywordResult> {
    if (consume_keyword("long")) {
        return TypeKeywordResult{Value{Integer{}}, false};
    }
    if (consume_keyword("integer")) {
        return TypeKeywordResult{Value{Int16{}}, false};
    }
    if (consume_keyword("double")) {
        return TypeKeywordResult{Value{0.0}, false};
    }
    if (consume_keyword("single")) {
        return TypeKeywordResult{Value{0.0f}, false};
    }
    if (consume_keyword("currency")) {
        return TypeKeywordResult{Value{Currency{}}, false};
    }
    if (consume_keyword("date")) {
        return TypeKeywordResult{Value{DateValue{}}, false};
    }
    if (consume_keyword("byte")) {
        return TypeKeywordResult{Value{Byte{}}, false};
    }
    if (consume_keyword("decimal")) {
        return TypeKeywordResult{Value{Decimal{}}, false};
    }
    if (consume_keyword("string")) {
        return TypeKeywordResult{Value{std::string{}}, false};
    }
    if (consume_keyword("boolean")) {
        return TypeKeywordResult{Value{false}, false};
    }
    if (consume_keyword("variant")) {
        return TypeKeywordResult{Value{Empty{}}, true};
    }
    return std::nullopt;
}

auto Interpreter::parse_scalar_object_or_class_type()
    -> std::optional<ResolvedType> {
    skip_horizontal_whitespace();
    if (consume_keyword("object")) {
        return ResolvedType{Value{Nothing{}}.index(), false, true, {}};
    }
    if (const auto type_result = parse_type_keyword()) {
        return ResolvedType{type_result->default_value.index(),
                            type_result->is_variant,
                            false,
                            {}};
    }
    const auto saved_offset = offset_;
    char class_type_character{};
    auto class_name = parse_identifier(&class_type_character);
    if (class_name.has_value() && class_type_character == '\0' &&
        class_definitions_.contains(*class_name)) {
        return ResolvedType{Value{Nothing{}}.index(), false, true,
                            std::move(*class_name)};
    }
    offset_ = saved_offset;
    return std::nullopt;
}

std::optional<bool> Interpreter::parse_function_array_return_marker(
    const ResolvedType& type_result, const std::size_t type_offset) {
    skip_horizontal_whitespace();
    if (at_end() || current() != '(') {
        return false;
    }
    static_cast<void>(type_result);
    static_cast<void>(type_offset);
    advance();
    skip_horizontal_whitespace();
    if (!consume(')')) {
        set_error("WFC0150", "expected closing parenthesis", offset_);
        return std::nullopt;
    }
    return true;
}

bool Interpreter::type_character_matches(const Value& value,
                                         const char type_character,
                                         const std::size_t identifier_offset) {
    if (type_character == '\0') {
        return true;
    }
    if (!validate_type_character(type_character, identifier_offset)) {
        return false;
    }
    if (value.index() == type_character_index(type_character)) {
        return true;
    }
    set_error("WFC0016", "identifier type-declaration character mismatch",
              identifier_offset);
    return false;
}

bool Interpreter::resolve_default_value(Value& value,
                                        const std::size_t offset) {
    const auto* holder = std::get_if<ObjectInstance>(&value);
    if (holder == nullptr || !execute_ || holder->data == nullptr) {
        return true;
    }
    const auto class_iterator =
        class_definitions_.find(holder->data->class_name);
    if (class_iterator == class_definitions_.end() ||
        class_iterator->second.is_udt ||
        class_iterator->second.default_member.empty()) {
        return true;
    }
    const auto& class_def = class_iterator->second;
    const ProcedureDef* definition = nullptr;
    if (const auto method = class_def.methods.find(class_def.default_member);
        method != class_def.methods.end()) {
        definition = &method->second;
    } else if (const auto getter =
                   class_def.property_get.find(class_def.default_member);
               getter != class_def.property_get.end()) {
        definition = &getter->second;
    }
    if (definition == nullptr) {
        return true;
    }
    const auto instance =
        holder->data;  // keep the object alive across the call
    auto result = invoke_definition(*definition, class_def.default_member, {},
                                    offset, class_def.source, instance.get());
    if (!result.has_value()) {
        return false;
    }
    value = std::move(*result);
    return true;
}

std::string Interpreter::vb_error_description(const Integer number) {
    switch (number) {
        case 0:
            return {};
        case 3:
            return "Return without GoSub";
        case 5:
            return "Invalid procedure call or argument";
        case 6:
            return "Overflow";
        case 7:
            return "Out of memory";
        case 9:
            return "Subscript out of range";
        case 10:
            return "This array is fixed or temporarily locked";
        case 11:
            return "Division by zero";
        case 13:
            return "Type mismatch";
        case 14:
            return "Out of string space";
        case 16:
            return "Expression too complex";
        case 17:
            return "Can't perform requested operation";
        case 18:
            return "User interrupt occurred";
        case 20:
            return "Resume without error";
        case 28:
            return "Out of stack space";
        case 35:
            return "Sub, Function, or Property not defined";
        case 47:
            return "Too many DLL application clients";
        case 48:
            return "Error in loading DLL";
        case 49:
            return "Bad DLL calling convention";
        case 51:
            return "Internal error";
        case 52:
            return "Bad file name or number";
        case 53:
            return "File not found";
        case 54:
            return "Bad file mode";
        case 55:
            return "File already open";
        case 57:
            return "Device I/O error";
        case 58:
            return "File already exists";
        case 59:
            return "Bad record length";
        case 61:
            return "Disk full";
        case 62:
            return "Input past end of file";
        case 63:
            return "Bad record number";
        case 67:
            return "Too many files";
        case 68:
            return "Device unavailable";
        case 70:
            return "Permission denied";
        case 71:
            return "Disk not ready";
        case 74:
            return "Can't rename with different drive";
        case 75:
            return "Path/File access error";
        case 76:
            return "Path not found";
        case 91:
            return "Object variable or With block variable not set";
        case 92:
            return "For loop not initialized";
        case 93:
            return "Invalid pattern string";
        case 94:
            return "Invalid use of Null";
        case 96:
            return "Unable to sink events of object because the object is "
                   "already firing events to the maximum number of event "
                   "receivers that it supports";
        case 97:
            return "Can't call Friend procedure on an object that is not an "
                   "instance of defining class";
        case 98:
            return "A property or method call cannot include a reference to a "
                   "private object, either as an argument or as a return "
                   "value";
        case 321:
            return "Invalid file format";
        case 322:
            return "Can't create necessary temporary file";
        case 325:
            return "Invalid format in resource file";
        case 380:
            return "Invalid property value";
        case 381:
            return "Invalid property array index";
        case 382:
            return "Set not supported at runtime";
        case 383:
            return "Set not supported (read-only property)";
        case 385:
            return "Need property array index";
        case 387:
            return "Set not permitted";
        case 393:
            return "Get not supported at runtime";
        case 394:
            return "Get not supported (write-only property)";
        case 422:
            return "Property not found";
        case 423:
            return "Property or method not found";
        case 424:
            return "Object required";
        case 429:
            return "ActiveX component can't create object";
        case 430:
            return "Class does not support Automation or does not support "
                   "expected interface";
        case 432:
            return "File name or class name not found during Automation "
                   "operation";
        case 438:
            return "Object doesn't support this property or method";
        case 440:
            return "Automation error";
        case 443:
            return "Automation object does not have a default value";
        case 445:
            return "Object doesn't support this action";
        case 446:
            return "Object doesn't support named arguments";
        case 447:
            return "Object doesn't support current locale setting";
        case 448:
            return "Named argument not found";
        case 449:
            return "Argument not optional";
        case 450:
            return "Wrong number of arguments or invalid property assignment";
        case 451:
            return "Property let procedure not defined and property get "
                   "procedure did not return an object";
        case 442:
            return "Connection to type library or object library for remote "
                   "process has been lost. Press OK for dialog to remove "
                   "reference.";
        case 452:
            return "Invalid ordinal";
        case 453:
            return "Specified DLL function not found";
        case 454:
            return "Code resource not found";
        case 455:
            return "Code resource lock error";
        case 457:
            return "This key is already associated with an element of this "
                   "collection";
        case 458:
            return "Variable uses an Automation type not supported in Visual "
                   "Basic";
        case 459:
            return "Object or class does not support the set of events";
        case 460:
            return "Invalid clipboard format";
        case 461:
            return "Method or data member not found";
        case 462:
            return "The remote server machine does not exist or is unavailable";
        case 463:
            return "Class not registered on local machine";
        case 481:
            return "Invalid picture";
        case 482:
            return "Printer error";
        case 483:
            return "Printer driver does not support specified property";
        case 485:
            return "Invalid picture type";
        case 486:
            return "Can't print form image to this type of printer";
        case 520:
            return "Can't empty Clipboard";
        case 521:
            return "Can't open Clipboard";
        case 735:
            return "Can't save file to TEMP";
        case 744:
            return "Search text not found";
        case 746:
            return "Replacements too long";
        case 31001:
            return "Out of memory";
        default:
            return "Application-defined or object-defined error";
    }
}

Integer Interpreter::runtime_error_number() const {
    const std::string_view code =
        std::string_view(error_.diagnostic).substr(0, 7);
    if (code == "WFC0300") {
        return err_number_;
    }
    if (code == "WFC0016" || code == "WFC0007" || code == "WFC0018" ||
        code == "WFC0019" || code == "WFC0020" || code == "WFC0073" ||
        code == "WFC0086" || code == "WFC0087" || code == "WFC0088" ||
        code == "WFC0095" || code == "WFC0098" || code == "WFC0103" ||
        code == "WFC0105" || code == "WFC0060" || code == "WFC0053") {
        return 13;  // Type mismatch
    }
    if (code == "WFC0136" || code == "WFC0107") {
        return 424;  // Object required
    }
    if (code == "WFC0008") {
        return 11;
    }
    if (code == "WFC0009") {
        return 6;
    }
    if (code == "WFC0111") {
        return 9;
    }
    if (code == "WFC0106") {
        return 91;
    }
    if (code == "WFC0104") {
        return 94;
    }
    if (code == "WFC0089" || code == "WFC0101" || code == "WFC0075" ||
        code == "WFC0078" || code == "WFC0076" || code == "WFC0077" ||
        code == "WFC0079" || code == "WFC0080" || code == "WFC0082" ||
        code == "WFC0083" || code == "WFC0091" || code == "WFC0092" ||
        code == "WFC0094" || code == "WFC0096") {
        return 5;
    }
    if (code == "WFC0148" || code == "WFC0151") {
        return 9;
    }
    if (code == "WFC0123") {
        return 28;
    }
    return 0;
}

bool Interpreter::raise_runtime(const Integer number,
                                const std::string& description,
                                const std::size_t offset) {
    err_number_ = number;
    err_erl_ = erl_;
    err_description_ = description;
    err_source_.clear();
    err_help_file_.clear();
    err_help_context_ = 0;
    set_error("WFC0300", description, offset);
    return false;
}

void Interpreter::set_error(const std::string_view code,
                            const std::string_view message,
                            const std::size_t offset) {
    if (code == "WFC0135" && execute_) {
        // An unknown member reached at run time is a catchable "Object doesn't
        // support..."
        err_number_ = 438;
        err_erl_ = erl_;
        err_description_ = "Object doesn't support this property or method";
        err_source_.clear();
        err_help_file_.clear();
        err_help_context_ = 0;
        error_ = failure("WFC0300", err_description_, offset);
        error_source_data_ = source_.data();
        return;
    }
    error_ = failure(code, message, offset);
    error_source_data_ = source_.data();
}

}  // namespace wfc::detail
