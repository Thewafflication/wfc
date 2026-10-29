#include "parser.hpp"

#include <cstdlib>
#include <utility>

#include "lexer.hpp"

namespace wfc::compiler {

namespace {

// The enumerations of the language (VbMsgBoxResult, ColorConstants, ...): they
// are Long as types, and `Enumeration.member` names one of their constants.
[[nodiscard]] bool is_library_enumeration(const std::string& name) {
    static const std::set<std::string> names = {"vba",
                                                "strings",
                                                "math",
                                                "conversion",
                                                "information",
                                                "interaction",
                                                "datetime",
                                                "filesystem",
                                                "constants",
                                                "financial",
                                                "colorconstants",
                                                "vbmsgboxresult",
                                                "vbmsgboxstyle",
                                                "systemcolorconstants",
                                                "vbcomparemethod",
                                                "vbtristate",
                                                "vbvartype",
                                                "vbdayofweek",
                                                "vbfirstweekofyear",
                                                "vbdateformat",
                                                "vbcalendar",
                                                "vbstrconv",
                                                "vbappwinstyle",
                                                "vbfileattribute",
                                                "vbqueryclose",
                                                "keycodeconstants",
                                                "shiftconstants",
                                                "mousebuttonconstants",
                                                "drawstyleconstants",
                                                "fillstyleconstants",
                                                "scalemodeconstants",
                                                "alignmentconstants",
                                                "borderstyleconstants",
                                                "mousepointerconstants",
                                                "windowstateconstants",
                                                "formshowconstants"};
    return names.contains(name);
}

// A syntax error or an unsupported construct unwinds to `parse_module`.
struct ParseFailure final {
    std::string message;
    Position at;
};

class Parser final {
public:
    Parser(std::vector<Token> tokens, const ParseOptions& options)
        : tokens_(std::move(tokens)), options_(options) {}

    Module parse() {
        Module module;
        module.is_class = options_.is_class;
        module.is_form = options_.is_form;
        module.name = options_.name;
        module.display_name = options_.display_name;
        skip_ends();
        while (!at_end_of_file()) {
            if (is_word("attribute")) {
                // `Attribute Name = value`: a file-format line, no code.
                if (ahead(1).text == "vb_predeclaredid" &&
                    ahead(3).text == "true") {
                    module.predeclared = true;
                }
                if (ahead(1).text == "vb_globalnamespace" &&
                    ahead(3).text == "true") {
                    module.global_namespace = true;
                }
                if (ahead(1).kind == TokenKind::identifier &&
                    ahead(2).kind == TokenKind::symbol &&
                    ahead(2).text == "." &&
                    ahead(3).text == "vb_varusermemid" &&
                    ahead(5).number == 0) {
                    module.default_member = ahead(1).text;
                }
                while (!at_end_of_statement()) {
                    advance();
                }
            } else if (accept_word("option")) {
                option_statement(module);
            } else if (is_word("dim") || is_word("private") ||
                       is_word("public") || is_word("friend") ||
                       is_word("static") || is_word("sub") ||
                       is_word("function") || is_word("property") ||
                       is_word("const") || is_word("type") ||
                       is_word("event") || is_word("implements") ||
                       is_word("enum") || is_word("declare")) {
                declaration_or_procedure(module);
            } else if (!def_statement()) {
                fail("expected a declaration, Sub or Function");
            }
            skip_ends();
        }
        for (std::size_t letter = 0; letter < 26U; ++letter) {
            module.def_types[letter] = {def_types_[letter].type,
                                        def_types_[letter].type_name};
        }
        // The type a name was first written with (`n%`).
        for (const auto& token : tokens_) {
            if (token.kind == TokenKind::identifier && token.text.size() > 1U) {
                const char last = token.text.back();
                if (last == '%' || last == '&' || last == '!' || last == '#' ||
                    last == '@' || last == '$') {
                    const auto base =
                        token.text.substr(0, token.text.size() - 1U);
                    bool known = false;
                    for (const auto& entry : module.name_suffixes) {
                        known = known || entry.first == base;
                    }
                    if (!known) {
                        module.name_suffixes.emplace_back(base, last);
                    }
                }
            }
        }
        return module;
    }

private:
    // DefInt A-C, DefStr S ...: the type by first letter (none: Variant).
    struct DefType final {
        VbType type{VbType::none};
        std::string type_name;
    };

    // ----- Token helpers ---------------------------------------------------

    [[nodiscard]] const Token& current() const { return tokens_[index_]; }
    [[nodiscard]] const Token& ahead(const std::size_t count) const {
        const auto at = index_ + count;
        return at < tokens_.size() ? tokens_[at] : tokens_.back();
    }
    [[nodiscard]] Position here() const {
        return Position{current().line, current().column};
    }
    [[nodiscard]] bool at_end_of_file() const {
        return current().kind == TokenKind::end_of_file;
    }
    [[nodiscard]] bool at_end_of_statement() const {
        return current().kind == TokenKind::end_of_statement ||
               current().kind == TokenKind::end_of_file;
    }
    void advance() {
        if (!at_end_of_file()) {
            ++index_;
        }
    }
    void skip_ends() {
        while (current().kind == TokenKind::end_of_statement) {
            advance();
        }
    }
    [[noreturn]] void fail(const std::string& message) const {
        throw ParseFailure{message, here()};
    }
    [[nodiscard]] bool is_word(const std::string_view word) const {
        return current().kind == TokenKind::identifier &&
               current().text == word;
    }
    [[nodiscard]] bool is_symbol(const std::string_view symbol) const {
        return current().kind == TokenKind::symbol && current().text == symbol;
    }
    bool accept_word(const std::string_view word) {
        if (is_word(word)) {
            advance();
            return true;
        }
        return false;
    }
    bool accept_symbol(const std::string_view symbol) {
        if (is_symbol(symbol)) {
            advance();
            return true;
        }
        return false;
    }
    void expect_word(const std::string_view word) {
        if (!accept_word(word)) {
            fail("expected " + std::string(word));
        }
    }
    void expect_symbol(const std::string_view symbol) {
        if (!accept_symbol(symbol)) {
            fail("expected '" + std::string(symbol) + "'");
        }
    }
    void expect_end_of_statement() {
        if (!at_end_of_statement()) {
            fail("expected the end of the statement");
        }
        skip_ends();
    }

    // ----- Declarations ----------------------------------------------------

    void option_statement(Module& module) {
        if (accept_word("explicit")) {
            expect_end_of_statement();
            return;
        }
        if (accept_word("base")) {
            if (current().kind != TokenKind::integer ||
                (current().number != 0 && current().number != 1)) {
                fail("Option Base must be 0 or 1");
            }
            module.option_base = static_cast<int>(current().number);
            advance();
            expect_end_of_statement();
            return;
        }
        if (accept_word("compare")) {
            if (accept_word("binary") || accept_word("database")) {
                expect_end_of_statement();
                return;
            }
            if (accept_word("text")) {
                module.option_compare_text = true;
                expect_end_of_statement();
                return;
            }
            fail("Option Compare " + current().spelling +
                 " is not compiled yet");
        }
        fail("Option " + current().spelling + " is not compiled yet");
    }

    [[nodiscard]] static VbType suffix_type(const std::string& name) {
        switch (name.back()) {
            case '%':
                return VbType::integer;
            case '&':
                return VbType::long_integer;
            case '!':
                return VbType::single_precision;
            case '#':
                return VbType::double_precision;
            case '$':
                return VbType::string;
            default:
                return VbType::none;
        }
    }

    [[nodiscard]] static std::string without_suffix(std::string name) {
        if (!name.empty() && suffix_type(name) != VbType::none) {
            name.pop_back();
        } else if (!name.empty() && name.back() == '@') {
            name.pop_back();
        }
        return name;
    }

    // The types of the objects the engine provides (forms and controls).
    [[nodiscard]] static bool is_engine_type(const std::string& name) {
        static const std::set<std::string> names = {
            "control",     "form",        "textbox",      "commandbutton",
            "label",       "picturebox",  "image",        "listbox",
            "combobox",    "checkbox",    "optionbutton", "frame",
            "timer",       "shape",       "line",         "hscrollbar",
            "vscrollbar",  "menu",        "stdpicture",   "picture",
            "font",        "stdfont",     "printer",      "mdiform",
            "usercontrol", "data",        "ole",          "drivelistbox",
            "dirlistbox",  "filelistbox", "commondialog", "winsock"};
        return names.contains(name);
    }

    [[nodiscard]] VbType type_name(std::string* record_name = nullptr) {
        if (current().kind != TokenKind::identifier) {
            fail("expected a type name");
        }
        // `VB.Image`, `Scripting.Dictionary`: the library only qualifies.
        while (ahead(1).kind == TokenKind::symbol && ahead(1).text == "." &&
               ahead(2).kind == TokenKind::identifier) {
            advance();
            advance();
        }
        const std::string& name = current().text;
        VbType type = VbType::none;
        if (name == "integer") {
            type = VbType::integer;
        } else if (name == "long") {
            type = VbType::long_integer;
        } else if (name == "boolean") {
            type = VbType::boolean;
        } else if (name == "single") {
            type = VbType::single_precision;
        } else if (name == "double") {
            type = VbType::double_precision;
        } else if (name == "byte") {
            type = VbType::byte;
        } else if (name == "variant") {
            type = VbType::variant;
        } else if (options_.enum_names.contains(name) ||
                   is_library_enumeration(name)) {
            type = VbType::long_integer;
        } else if (name == "object") {
            // A late-bound object is held like a Variant.
            type = VbType::variant;
            if (record_name != nullptr) {
                *record_name = name;
            }
        } else if (options_.form_names.contains(name) ||
                   (is_engine_type(name) &&
                    !options_.type_names.contains(name) &&
                    !options_.class_names.contains(name))) {
            // A form or a control: an object of the engine, late bound.
            type = VbType::variant;
            if (record_name != nullptr) {
                *record_name = name;
            }
        } else if (options_.class_names.contains(name)) {
            type = VbType::object;
            if (record_name != nullptr) {
                *record_name = name;
            }
        } else if (name == "string") {
            type = VbType::string;
        } else if (record_name != nullptr &&
                   options_.type_names.contains(name)) {
            // A user-defined type; the compiler checks it.
            type = VbType::record;
            *record_name = name;
        } else if (record_name != nullptr) {
            // Any other name is a class the engine provides (a dialog, a
            // Dictionary, a control): objects of it are late bound.
            type = VbType::variant;
            *record_name = name;
        } else {
            fail("the type " + current().spelling + " is not compiled yet");
        }
        advance();
        return type;
    }

    // `[low To] high {, [low To] high} )`; the opening parenthesis is read.
    void array_bounds(std::vector<ArrayBound>& into) {
        do {
            ArrayBound bound;
            auto first = expression();
            if (accept_word("to")) {
                bound.low = std::move(first);
                bound.high = expression();
            } else {
                bound.high = std::move(first);
            }
            into.push_back(std::move(bound));
        } while (accept_symbol(","));
        expect_symbol(")");
    }

    [[nodiscard]] Declaration declarator() {
        if (current().kind != TokenKind::identifier) {
            fail("expected a variable name");
        }
        Declaration declaration;
        declaration.at = here();
        const std::string spelled = current().text;
        declaration.name = without_suffix(spelled);
        const VbType by_suffix = suffix_type(spelled);
        advance();
        if (is_symbol("(")) {
            advance();
            declaration.is_array = true;
            if (!accept_symbol(")")) {
                array_bounds(declaration.bounds);
            }
        }
        if (accept_word("as")) {
            declaration.is_new = accept_word("new");
            declaration.type = type_name(&declaration.type_name);
            if (declaration.type == VbType::string && is_symbol("*")) {
                // `String * n`: fixed length.
                advance();
                if (current().kind != TokenKind::integer) {
                    fail("expected the length of the String");
                }
                declaration.fixed_length = static_cast<int>(current().number);
                advance();
            }
            if (declaration.is_new && !((declaration.type == VbType::object ||
                                         declaration.type == VbType::variant) &&
                                        !declaration.type_name.empty())) {
                fail("New needs the name of a class");
            }
            declaration.type_given = true;
        } else if (by_suffix != VbType::none) {
            declaration.type = by_suffix;
            declaration.type_given = true;
        } else {
            declaration.type = VbType::variant;
            declaration.type_given = true;
            if (const auto* def = def_type_of(declaration.name)) {
                declaration.type = def->type;
                declaration.type_name = def->type_name;
            }
        }
        return declaration;
    }

    [[nodiscard]] const DefType* def_type_of(const std::string& name) const {
        if (name.empty() || name.front() < 'a' || name.front() > 'z') {
            return nullptr;
        }
        const auto& found = def_types_[name.front() - 'a'];
        return found.type == VbType::none ? nullptr : &found;
    }

    // `DefInt A-C, X`: true when the statement was one.
    [[nodiscard]] bool def_statement() {
        static const std::map<std::string, std::pair<VbType, std::string>>
            kinds = {{"defbool", {VbType::boolean, ""}},
                     {"defbyte", {VbType::byte, ""}},
                     {"defint", {VbType::integer, ""}},
                     {"deflng", {VbType::long_integer, ""}},
                     {"defsng", {VbType::single_precision, ""}},
                     {"defdbl", {VbType::double_precision, ""}},
                     {"defstr", {VbType::string, ""}},
                     {"defcur", {VbType::variant, "currency"}},
                     {"defdate", {VbType::variant, "date"}},
                     {"defdec", {VbType::variant, "decimal"}},
                     {"defvar", {VbType::variant, ""}}};
        if (current().kind != TokenKind::identifier) {
            return false;
        }
        const auto found = kinds.find(current().text);
        if (found == kinds.end()) {
            return false;
        }
        advance();
        do {
            if (current().kind != TokenKind::identifier ||
                current().text.size() != 1U) {
                fail("expected a letter in the Def statement");
            }
            const char first = current().text.front();
            char last = first;
            advance();
            if (accept_symbol("-")) {
                if (current().kind != TokenKind::identifier ||
                    current().text.size() != 1U) {
                    fail("expected a letter in the Def statement");
                }
                last = current().text.front();
                advance();
            }
            for (char c = first; c <= last; ++c) {
                if (c >= 'a' && c <= 'z') {
                    def_types_[c - 'a'] =
                        DefType{found->second.first == VbType::variant &&
                                        found->second.second.empty()
                                    ? VbType::none
                                    : found->second.first,
                                found->second.second};
                }
            }
        } while (accept_symbol(","));
        expect_end_of_statement();
        return true;
    }

    void declaration_or_procedure(Module& module) {
        if (def_statement()) {
            return;
        }
        bool is_public = true;
        bool saw_scope = false;
        bool saw_public = false;
        bool saw_static = false;
        while (is_word("private") || is_word("public") || is_word("friend") ||
               is_word("static")) {
            if (is_word("private")) {
                is_public = false;
            }
            if (is_word("public")) {
                saw_public = true;
            }
            if (is_word("static")) {
                saw_static = true;
            }
            saw_scope = true;
            advance();
        }
        if (is_word("sub") || is_word("function") || is_word("property")) {
            // `Static Sub`: every local keeps its value between calls.
            procedure_static_ = saw_static;
            procedure(module, is_public);
            procedure_static_ = false;
            return;
        }
        if (saw_static) {
            fail("Static is not compiled yet");
        }
        if (is_word("enum")) {
            enum_block(module);
            return;
        }
        if (is_word("declare")) {
            declare_statement(module);
            return;
        }
        if (accept_word("implements")) {
            if (current().kind != TokenKind::identifier) {
                fail("expected the name of an interface");
            }
            module.implements.push_back(current().text);
            advance();
            expect_end_of_statement();
            return;
        }
        if (accept_word("event")) {
            // `Event Name(args)`: raised by RaiseEvent; nothing to compile.
            while (!at_end_of_statement()) {
                advance();
            }
            expect_end_of_statement();
            return;
        }
        if (is_word("type")) {
            type_declaration(module);
            return;
        }
        if (accept_word("const")) {
            constants(module.constants);
            expect_end_of_statement();
            return;
        }
        if (accept_word("dim") || saw_scope) {
            const bool with_events = accept_word("withevents");
            do {
                auto declaration = declarator();
                declaration.with_events = with_events;
                declaration.is_public = saw_public;
                module.globals.push_back(std::move(declaration));
            } while (accept_symbol(","));
            expect_end_of_statement();
            return;
        }
        fail("expected a declaration, Sub or Function");
    }

    // `Declare Function|Sub name Lib "library" [Alias "export"] [(...)] [As
    // type]`: the routine is called through the shared runtime, which has the
    // whole declaration.
    void declare_statement(Module& module) {
        DeclareDecl declared;
        declared.at = here();
        advance();  // Declare
        if (accept_word("function")) {
            declared.is_function = true;
        } else {
            expect_word("sub");
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected the name of the routine");
        }
        declared.name = without_suffix(current().text);
        advance();
        expect_word("lib");
        if (current().kind != TokenKind::string) {
            fail("expected the name of the library");
        }
        advance();
        if (accept_word("alias")) {
            if (current().kind != TokenKind::string) {
                fail("expected the export name");
            }
            advance();
        }
        if (is_symbol("(")) {
            int depth = 0;
            do {
                if (is_symbol("(")) {
                    ++depth;
                } else if (is_symbol(")")) {
                    --depth;
                }
                advance();
            } while (depth > 0 && !at_end_of_statement());
        }
        if (accept_word("as")) {
            declared.return_type = type_name();
        }
        module.declares.push_back(std::move(declared));
        expect_end_of_statement();
    }

    // Enum Name / members / End Enum
    void enum_block(Module& module) {
        EnumDecl declaration;
        declaration.at = here();
        advance();
        if (current().kind != TokenKind::identifier) {
            fail("expected the name of the Enum");
        }
        declaration.name = current().text;
        advance();
        expect_end_of_statement();
        while (!(is_word("end") && ahead(1).kind == TokenKind::identifier &&
                 ahead(1).text == "enum")) {
            if (at_end_of_file()) {
                fail("expected End Enum");
            }
            if (current().kind != TokenKind::identifier) {
                fail("expected the name of an Enum member");
            }
            EnumMember member;
            member.at = here();
            member.name = without_suffix(current().text);
            advance();
            if (accept_symbol("=")) {
                member.value = expression();
            }
            expect_end_of_statement();
            declaration.members.push_back(std::move(member));
        }
        advance();
        advance();
        expect_end_of_statement();
        module.enums.push_back(std::move(declaration));
    }

    // Type Name / fields / End Type
    void type_declaration(Module& module) {
        TypeDecl type;
        type.at = here();
        advance();
        if (current().kind != TokenKind::identifier) {
            fail("expected the name of the type");
        }
        type.name = current().text;
        advance();
        expect_end_of_statement();
        while (!(is_word("end") && ahead(1).kind == TokenKind::identifier &&
                 ahead(1).text == "type")) {
            if (at_end_of_file()) {
                fail("expected End Type");
            }
            FieldDecl field;
            field.at = here();
            if (current().kind != TokenKind::identifier) {
                fail("expected a field name");
            }
            const std::string spelled = current().text;
            field.name = without_suffix(spelled);
            const VbType by_suffix = suffix_type(spelled);
            advance();
            if (is_symbol("(")) {
                advance();
                field.is_array = true;
                if (accept_symbol(")")) {
                    field.dynamic = true;
                } else {
                    array_bounds(field.bounds);
                    if (field.bounds.size() != 1U) {
                        fail("an array in a type has one dimension here");
                    }
                }
            }
            if (accept_word("as")) {
                field.type = type_name(&field.type_name);
                if (field.type == VbType::string && is_symbol("*")) {
                    advance();
                    if (current().kind != TokenKind::integer) {
                        fail("expected the length of the String");
                    }
                    field.fixed_length = static_cast<int>(current().number);
                    advance();
                }
            } else if (by_suffix != VbType::none) {
                field.type = by_suffix;
            } else {
                fail("the field '" + field.name +
                     "' needs a type (Variant is not compiled yet)");
            }
            expect_end_of_statement();
            type.fields.push_back(std::move(field));
        }
        advance();
        advance();
        expect_end_of_statement();
        module.types.push_back(std::move(type));
    }

    // `name [As Type] = expression {, name [As Type] = expression}`
    void constants(std::vector<ConstantDecl>& into) {
        do {
            if (current().kind != TokenKind::identifier) {
                fail("expected the constant name");
            }
            ConstantDecl constant;
            constant.at = here();
            const std::string spelled = current().text;
            constant.name = without_suffix(spelled);
            const VbType by_suffix = suffix_type(spelled);
            advance();
            if (by_suffix != VbType::none) {
                constant.type = by_suffix;
                constant.type_given = true;
            }
            if (accept_word("as")) {
                constant.type = type_name();
                constant.type_given = true;
            }
            expect_symbol("=");
            constant.value = expression();
            into.push_back(std::move(constant));
        } while (accept_symbol(","));
    }

    void procedure(Module& module, const bool is_public) {
        Procedure result;
        result.is_public = is_public;
        result.is_function = is_word("function");
        const bool is_property = is_word("property");
        result.at = here();
        advance();
        if (is_property) {
            if (accept_word("get")) {
                result.property = PropertyKind::get;
                result.is_function = true;
            } else if (accept_word("let")) {
                result.property = PropertyKind::let;
            } else if (accept_word("set")) {
                result.property = PropertyKind::set;
            } else {
                fail("expected Get, Let or Set after Property");
            }
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected the procedure name");
        }
        result.spelling = current().spelling;
        result.name = without_suffix(current().text);
        const VbType by_suffix = suffix_type(current().text);
        advance();
        if (accept_symbol("(")) {
            if (!accept_symbol(")")) {
                do {
                    result.parameters.push_back(parameter());
                } while (accept_symbol(","));
                expect_symbol(")");
            }
        }
        if (result.is_function) {
            // Without `As` a Function returns a Variant.
            result.return_type = by_suffix;
            if (accept_word("as")) {
                result.return_type = type_name(&result.return_type_name);
                if (is_symbol("(") && ahead(1).kind == TokenKind::symbol &&
                    ahead(1).text == ")") {
                    // `As Item()`: the result is an array, held as a Variant.
                    advance();
                    advance();
                    result.return_type = VbType::variant;
                    result.return_type_name.clear();
                    result.returns_array = true;
                }
            }
            if (result.return_type == VbType::none) {
                result.return_type = VbType::variant;
                if (const auto* def = def_type_of(result.name)) {
                    result.return_type = def->type;
                }
            }
        }
        expect_end_of_statement();
        result.body = block({"end"});
        if (!default_member_.empty() && default_member_ == result.name) {
            result.is_default = true;
        }
        default_member_.clear();
        expect_word("end");
        const char* closing = is_property          ? "property"
                              : result.is_function ? "function"
                                                   : "sub";
        if (!accept_word(closing)) {
            fail(std::string("expected End ") + (is_property ? "Property"
                                                 : result.is_function
                                                     ? "Function"
                                                     : "Sub"));
        }
        expect_end_of_statement();
        module.procedures.push_back(std::move(result));
    }

    [[nodiscard]] Parameter parameter() {
        Parameter result;
        if (accept_word("paramarray")) {
            if (current().kind != TokenKind::identifier) {
                fail("expected a parameter name");
            }
            result.name = without_suffix(current().text);
            advance();
            expect_symbol("(");
            expect_symbol(")");
            if (accept_word("as")) {
                static_cast<void>(type_name());
            }
            result.type = VbType::variant;
            result.is_array = true;
            result.is_optional = true;
            result.is_param_array = true;
            return result;
        }
        if (accept_word("optional")) {
            result.is_optional = true;
        }
        if (accept_word("byval")) {
            result.by_value = true;
        } else {
            accept_word("byref");
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected a parameter name");
        }
        const std::string spelled = current().text;
        result.name = without_suffix(spelled);
        result.type = suffix_type(spelled);
        advance();
        if (is_symbol("(")) {
            advance();
            expect_symbol(")");
            result.is_array = true;
            result.by_value = false;
        }
        if (accept_word("as")) {
            result.type = type_name(&result.type_name);
        }
        if (result.type == VbType::none) {
            result.type = VbType::variant;
            if (const auto* def = def_type_of(result.name)) {
                result.type = def->type;
                result.type_name = def->type_name;
            }
        }
        if (result.is_optional && accept_symbol("=")) {
            result.default_value =
                std::shared_ptr<Expr>(expression().release());
        }
        return result;
    }

    // ----- Statements ------------------------------------------------------

    // Reads statements until one of `stoppers` starts a statement. `end`
    // stops only when it is followed by If/Sub/Function/Select/With.
    [[nodiscard]] std::vector<StmtPtr> block(
        const std::vector<std::string>& stoppers) {
        std::vector<StmtPtr> result;
        skip_ends();
        while (!at_end_of_file() && !at_stopper(stoppers) &&
               closing_loops_ == 0) {
            result.push_back(statement());
            skip_ends();
        }
        if (at_end_of_file() && closing_loops_ == 0) {
            fail("the block is not closed");
        }
        return result;
    }

    [[nodiscard]] bool at_stopper(
        const std::vector<std::string>& stoppers) const {
        if (current().kind != TokenKind::identifier) {
            return false;
        }
        for (const auto& word : stoppers) {
            if (current().text != word) {
                continue;
            }
            if (word == "end") {
                const auto& next = ahead(1);
                return next.kind == TokenKind::identifier &&
                       (next.text == "if" || next.text == "sub" ||
                        next.text == "function" || next.text == "select" ||
                        next.text == "with" || next.text == "property");
            }
            return true;
        }
        return false;
    }

    [[nodiscard]] StmtPtr make(const StmtKind kind) const {
        auto result = std::make_unique<Stmt>();
        result->kind = kind;
        result->at = here();
        return result;
    }

    [[nodiscard]] StmtPtr with_member_statement() {
        auto assignment = make(StmtKind::assign);
        auto chain = primary();
        if (is_symbol("=")) {
            advance();
            assignment->target = std::move(chain);
            assignment->expr = expression();
            return assignment;
        }
        return finish_member_statement(std::move(chain), false);
    }

    [[nodiscard]] StmtPtr with_statement() {
        auto result = make(StmtKind::with_block);
        advance();
        const bool on_err = current().text == "err" &&
                            ahead(1).kind == TokenKind::end_of_statement;
        result->expr = expression();
        expect_end_of_statement();
        with_err_.push_back(on_err);
        result->body = block({"end"});
        with_err_.pop_back();
        expect_word("end");
        expect_word("with");
        return result;
    }

    // Inside `With Err`, `.Member` is `Err.Member`: the name goes in front.
    void insert_err_before_dot() {
        if (!with_err_.empty() && with_err_.back() &&
            current().kind == TokenKind::symbol && current().text == ".") {
            Token err;
            err.kind = TokenKind::identifier;
            err.text = "err";
            err.spelling = "Err";
            err.line = current().line;
            err.column = current().column;
            err.line_start = current().line_start;
            tokens_.insert(
                tokens_.begin() + static_cast<std::ptrdiff_t>(index_), err);
        }
    }

    [[nodiscard]] StmtPtr statement() {
        insert_err_before_dot();
        if (current().kind == TokenKind::symbol && current().text == ".") {
            return with_member_statement();
        }
        if (current().kind == TokenKind::integer && current().line_start) {
            // A line number is a label.
            auto result = make(StmtKind::label);
            result->text = "line" + std::to_string(current().number);
            advance();
            return result;
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected a statement");
        }
        const std::string word = current().text;
        if (current().line_start &&
            ahead(1).kind == TokenKind::end_of_statement &&
            ahead(1).text == ":" && word != "else" && word != "do") {
            auto result = make(StmtKind::label);
            result->text = without_suffix(word);
            result->maybe_call = result->at.column != 1;
            advance();
            advance();
            return result;
        }
        if (word == "dim" || word == "static") {
            return dim_statement();
        }
        if (word == "const") {
            auto result = make(StmtKind::const_decl);
            advance();
            constants(result->constants);
            return result;
        }
        if (word == "goto") {
            auto result = make(StmtKind::go_to);
            advance();
            if (current().kind == TokenKind::integer) {
                result->text = "line" + std::to_string(current().number);
                advance();
                return result;
            }
            if (current().kind != TokenKind::identifier) {
                fail("expected a label after GoTo");
            }
            result->text = without_suffix(current().text);
            advance();
            return result;
        }
        if (word == "select") {
            return select_statement();
        }
        if (word == "on" && ahead(1).kind == TokenKind::identifier &&
            ahead(1).text == "error") {
            return on_error_statement();
        }
        if (word == "on") {
            auto result = make(StmtKind::on_goto);
            advance();
            result->expr = expression();
            expect_word("goto");
            do {
                std::string name;
                if (current().kind == TokenKind::integer) {
                    name = "line" + std::to_string(current().number);
                } else if (current().kind == TokenKind::identifier) {
                    name = without_suffix(current().text);
                } else {
                    fail("expected a label after GoTo");
                }
                advance();
                result->text += (result->text.empty() ? "" : ",") + name;
            } while (accept_symbol(","));
            return result;
        }
        if (word == "resume") {
            auto result = make(StmtKind::resume);
            advance();
            if (accept_word("next")) {
                result->text = "next";
            } else if (current().kind == TokenKind::integer) {
                result->text = "line" + std::to_string(current().number);
                advance();
            } else if (current().kind == TokenKind::identifier &&
                       !is_word("else")) {
                result->text = without_suffix(current().text);
                advance();
            }
            return result;
        }
        if (word == "redim") {
            return redim_statement();
        }
        if (word == "erase") {
            auto result = make(StmtKind::erase);
            advance();
            do {
                auto name = make_expr(ExprKind::name, here());
                if (current().kind != TokenKind::identifier) {
                    fail("expected an array name");
                }
                name->text = without_suffix(current().text);
                advance();
                result->items.push_back(std::move(name));
            } while (accept_symbol(","));
            return result;
        }
        if (word == "error" && ahead(1).kind != TokenKind::symbol) {
            // `Error number` raises a run-time error.
            auto result = make(StmtKind::call);
            advance();
            result->text = "err.raise";
            result->items.push_back(expression());
            return result;
        }
        if (word == "print") {
            return print_statement();
        }
        if (word == "write" && ahead(1).kind == TokenKind::symbol &&
            ahead(1).text == "#") {
            advance();
            return file_print_statement("Write");
        }
        if (word == "input" && ahead(1).kind == TokenKind::symbol &&
            ahead(1).text == "#") {
            return input_statement(false);
        }
        if (word == "line" && ahead(1).kind == TokenKind::identifier &&
            ahead(1).text == "input") {
            advance();
            return input_statement(true);
        }
        if (word == "open") {
            return open_statement();
        }
        if (word == "close") {
            return close_statement();
        }
        if ((word == "get" || word == "put") &&
            ahead(1).kind == TokenKind::symbol && ahead(1).text == "#") {
            return get_put_statement(word == "get");
        }
        if (word == "seek" && ahead(1).kind == TokenKind::symbol &&
            ahead(1).text == "#") {
            return simple_engine_statement("Seek #");
        }
        if (word == "width" && ahead(1).kind == TokenKind::symbol &&
            ahead(1).text == "#") {
            return simple_engine_statement("Width #");
        }
        if ((word == "lock" || word == "unlock") &&
            ahead(1).kind == TokenKind::symbol && ahead(1).text == "#") {
            return simple_engine_statement(current().spelling + " #");
        }
        if (options_.is_form && (word == "load" || word == "unload") &&
            ahead(1).kind == TokenKind::identifier &&
            ahead(2).kind == TokenKind::symbol && ahead(2).text == "(") {
            // `Load Control(i)`: an element of a control array of the form;
            // the statement runs in the scope of the form (argument 0).
            auto result = make(StmtKind::engine);
            const std::string keyword = current().spelling;
            advance();
            const std::string name = current().spelling;
            advance();
            advance();  // (
            const auto me = engine_argument(
                *result, make_expr(ExprKind::name, here()), 'i');
            result->items.back()->text = "me";
            result->text = "@" + keyword + " " + name + "(" +
                           engine_argument(*result, expression(), 'i') + ")";
            expect_symbol(")");
            return result;
        }
        if ((word == "load" || word == "unload") &&
            ahead(1).kind == TokenKind::identifier) {
            return simple_engine_statement(current().spelling + " ");
        }
        if ((word == "sendkeys" || word == "appactivate" ||
             word == "savesetting" || word == "deletesetting" ||
             word == "savepicture") &&
            !(ahead(1).kind == TokenKind::symbol && ahead(1).text == "=")) {
            return simple_engine_statement(current().spelling + " ");
        }
        if (word == "beep") {
            auto result = make(StmtKind::engine);
            advance();
            result->text = "Beep";
            return result;
        }
        if (word == "kill" || word == "mkdir" || word == "rmdir" ||
            word == "chdir" || word == "chdrive" || word == "filecopy" ||
            word == "setattr") {
            return simple_engine_statement(current().spelling + " ");
        }
        if (word == "name" && (ahead(1).kind == TokenKind::string ||
                               ahead(1).kind == TokenKind::identifier)) {
            return name_statement();
        }
        if (word == "if") {
            return if_statement();
        }
        if (word == "for") {
            return for_statement();
        }
        if (word == "do") {
            return do_statement();
        }
        if (word == "while") {
            return while_statement();
        }
        if (word == "exit") {
            return exit_statement();
        }
        if (word == "end") {
            auto result = make(StmtKind::exit);
            advance();
            result->text = "end";
            return result;
        }
        if (word == "call") {
            advance();
            return call_statement(true);
        }
        if (word == "let") {
            advance();
            return assignment_or_call();
        }
        if (word == "set") {
            auto result = make(StmtKind::set_object);
            advance();
            result->target = primary();
            expect_symbol("=");
            result->expr = expression();
            return result;
        }
        if (word == "with") {
            return with_statement();
        }
        if (word == "attribute") {
            // `Attribute Name = value`: a file-format line, no code;
            // `Attribute Name.VB_UserMemId = 0` makes Name the default member.
            if (ahead(1).kind == TokenKind::identifier &&
                ahead(2).kind == TokenKind::symbol && ahead(2).text == "." &&
                (ahead(3).text == "vb_usermemid" ||
                 ahead(3).text == "vb_varusermemid") &&
                ahead(5).number == 0) {
                default_member_ = ahead(1).text;
            }
            auto result = make(StmtKind::dim);
            while (!at_end_of_statement()) {
                advance();
            }
            return result;
        }
        if (options_.is_form &&
            (word == "line" || word == "circle" || word == "pset" ||
             word == "scale") &&
            ((ahead(1).kind == TokenKind::symbol &&
              (ahead(1).text == "(" || ahead(1).text == "-")) ||
             (ahead(1).kind == TokenKind::identifier &&
              ahead(1).text == "step") ||
             (word == "scale" &&
              (ahead(1).kind == TokenKind::end_of_statement ||
               ahead(1).kind == TokenKind::end_of_file)))) {
            std::string keyword = current().spelling;
            return surface_statement(make_expr(ExprKind::name, here()),
                                     keyword);
        }
        if (word == "raiseevent") {
            // `RaiseEvent Name(args)`: the sinks of the object are called.
            auto result = make(StmtKind::call);
            advance();
            if (current().kind != TokenKind::identifier) {
                fail("expected the name of an event");
            }
            result->text = "wfcraise_" + without_suffix(current().text);
            advance();
            if (accept_symbol("(")) {
                if (!accept_symbol(")")) {
                    do {
                        result->items.push_back(expression());
                    } while (accept_symbol(","));
                    expect_symbol(")");
                }
            }
            return result;
        }
        if (word == "gosub" || word == "on" || word == "static" ||
            word == "line") {
            fail("the statement " + current().spelling +
                 " is not compiled yet");
        }
        return assignment_or_call();
    }

    // On Error GoTo label | On Error GoTo 0 | On Error Resume Next
    [[nodiscard]] StmtPtr on_error_statement() {
        auto result = make(StmtKind::on_error);
        advance();
        advance();
        if (accept_word("resume")) {
            expect_word("next");
            result->text = "resume next";
            return result;
        }
        expect_word("goto");
        if (current().kind == TokenKind::integer) {
            if (current().number != 0) {
                result->text = "line" + std::to_string(current().number);
                advance();
                return result;
            }
            result->text = "0";
            advance();
            return result;
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected a label after On Error GoTo");
        }
        result->text = without_suffix(current().text);
        advance();
        return result;
    }

    [[nodiscard]] StmtPtr redim_statement() {
        auto result = make(StmtKind::redim);
        advance();
        if (accept_word("preserve")) {
            result->preserve = true;
        }
        do {
            if (current().kind != TokenKind::identifier) {
                fail("expected an array name");
            }
            Declaration declaration;
            declaration.at = here();
            declaration.name = without_suffix(current().text);
            declaration.is_array = true;
            auto chain = make_expr(ExprKind::name, here());
            chain->text = declaration.name;
            advance();
            while (is_symbol(".") && ahead(1).kind == TokenKind::identifier) {
                // `ReDim o.Items(1 To 2)`: an array in a record.
                advance();
                auto field = make_expr(ExprKind::member, here());
                field->text = without_suffix(current().text);
                advance();
                field->operands.push_back(std::move(chain));
                chain = std::move(field);
                declaration.name = chain->text;
            }
            if (chain->kind == ExprKind::member) {
                declaration.target = std::move(chain);
            }
            expect_symbol("(");
            array_bounds(declaration.bounds);
            if (accept_word("as")) {
                declaration.type = type_name(&declaration.type_name);
                declaration.type_given = true;
            }
            result->declarations.push_back(std::move(declaration));
        } while (accept_symbol(","));
        return result;
    }

    [[nodiscard]] StmtPtr select_statement() {
        auto result = make(StmtKind::select_case);
        advance();
        expect_word("case");
        result->expr = expression();
        expect_end_of_statement();
        while (is_word("case")) {
            advance();
            if (accept_word("else")) {
                expect_end_of_statement();
                result->body = block({"case", "end"});
                result->has_else = true;
                break;
            }
            CaseBlock block_of_case;
            do {
                CaseClause clause;
                if (accept_word("is")) {
                    if (current().kind != TokenKind::symbol ||
                        !(current().text == "=" || current().text == "<>" ||
                          current().text == "<" || current().text == ">" ||
                          current().text == "<=" || current().text == ">=")) {
                        fail("expected a comparison operator after Is");
                    }
                    clause.kind = CaseClause::Kind::relation;
                    clause.op = current().text;
                    advance();
                    clause.low = expression();
                } else {
                    clause.low = expression();
                    if (accept_word("to")) {
                        clause.kind = CaseClause::Kind::range;
                        clause.high = expression();
                    }
                }
                block_of_case.clauses.push_back(std::move(clause));
            } while (accept_symbol(","));
            expect_end_of_statement();
            block_of_case.body = block({"case", "end"});
            result->cases.push_back(std::move(block_of_case));
        }
        expect_word("end");
        expect_word("select");
        return result;
    }

    [[nodiscard]] StmtPtr dim_statement() {
        auto result = make(StmtKind::dim);
        const bool is_static = is_word("static") || procedure_static_;
        advance();
        do {
            result->declarations.push_back(declarator());
            result->declarations.back().is_static = is_static;
        } while (accept_symbol(","));
        return result;
    }

    // ----- Statements the shared runtime runs ------------------------------

    // Adds `expression` as argument __k of the statement; returns its name.
    static std::string engine_argument(Stmt& statement, ExprPtr expression,
                                       const char mode) {
        const std::string name =
            "wfcarg" + std::to_string(statement.items.size());
        statement.items.push_back(std::move(expression));
        statement.separators.push_back(mode);
        return name;
    }

    // `Print #n, items` and `Write #n, items`: the current token is the `#`.
    [[nodiscard]] StmtPtr file_print_statement(const std::string& keyword) {
        auto result = make(StmtKind::engine);
        advance();  // #
        result->text =
            keyword + " #" + engine_argument(*result, expression(), 'i');
        if (!accept_symbol(",")) {
            return result;
        }
        result->text += ",";
        while (!at_end_of_statement() && !is_word("else")) {
            if (is_symbol(";") || is_symbol(",")) {
                result->text += current().text;
                advance();
                continue;
            }
            result->text += " ";
            if ((is_word("spc") || is_word("tab")) &&
                ahead(1).kind == TokenKind::symbol && ahead(1).text == "(") {
                const std::string function = current().spelling;
                advance();
                advance();
                result->text += function + "(" +
                                engine_argument(*result, expression(), 'i') +
                                ")";
                expect_symbol(")");
                continue;
            }
            result->text += engine_argument(*result, expression(), 'i');
        }
        return result;
    }

    // `Input #n, a, b` and `Line Input #n, a`.
    [[nodiscard]] StmtPtr input_statement(const bool line) {
        auto result = make(StmtKind::engine);
        advance();  // Input
        advance();  // #
        result->text = std::string(line ? "Line Input" : "Input") + " #" +
                       engine_argument(*result, expression(), 'i');
        expect_symbol(",");
        std::string separator = ", ";
        do {
            result->text +=
                separator + engine_argument(*result, expression(), 'b');
            separator = ", ";
        } while (accept_symbol(","));
        return result;
    }

    // `Open path For mode [Access ...] [Lock ...] As #n [Len = n]`
    [[nodiscard]] StmtPtr open_statement() {
        auto result = make(StmtKind::engine);
        advance();
        result->text =
            "Open " + engine_argument(*result, expression(), 'i') + " ";
        while (current().kind == TokenKind::identifier && !is_word("as")) {
            result->text += current().spelling + " ";
            advance();
        }
        expect_word("as");
        static_cast<void>(accept_symbol("#"));
        result->text += "As #" + engine_argument(*result, expression(), 'i');
        if (accept_word("len")) {
            expect_symbol("=");
            result->text +=
                " Len = " + engine_argument(*result, expression(), 'i');
        }
        return result;
    }

    // `Close`, `Close #1`, `Close #1, #2`
    [[nodiscard]] StmtPtr close_statement() {
        auto result = make(StmtKind::engine);
        advance();
        result->text = "Close";
        std::string separator = " ";
        while (!at_end_of_statement() && !is_word("else")) {
            static_cast<void>(accept_symbol("#"));
            result->text +=
                separator + "#" + engine_argument(*result, expression(), 'i');
            separator = ", ";
            if (!accept_symbol(",")) {
                break;
            }
        }
        return result;
    }

    // `Get #n, [position], variable` and `Put #n, [position], value`
    [[nodiscard]] StmtPtr get_put_statement(const bool get) {
        auto result = make(StmtKind::engine);
        const std::string keyword = current().spelling;
        advance();
        advance();  // #
        result->text =
            keyword + " #" + engine_argument(*result, expression(), 'i') + ",";
        expect_symbol(",");
        if (!is_symbol(",")) {
            result->text += " " + engine_argument(*result, expression(), 'i');
        }
        result->text += ",";
        expect_symbol(",");
        result->text +=
            " " + engine_argument(*result, expression(), get ? 'b' : 'i');
        return result;
    }

    // `Kill path`, `Seek #n, position`, `FileCopy a, b`, ...
    [[nodiscard]] StmtPtr simple_engine_statement(const std::string& prefix) {
        auto result = make(StmtKind::engine);
        advance();
        if (prefix.back() == '#') {
            advance();  // the # of Seek
        }
        result->text = prefix;
        std::string separator;
        do {
            result->text +=
                separator + engine_argument(*result, expression(), 'i');
            separator = ", ";
        } while (accept_symbol(","));
        return result;
    }

    // `Name old As new`
    [[nodiscard]] StmtPtr name_statement() {
        auto result = make(StmtKind::engine);
        advance();
        result->text = "Name " + engine_argument(*result, expression(), 'i');
        expect_word("as");
        result->text += " As " + engine_argument(*result, expression(), 'i');
        return result;
    }

    [[nodiscard]] StmtPtr print_statement() {
        if (ahead(1).kind == TokenKind::symbol && ahead(1).text == "#") {
            advance();
            return file_print_statement("Print");
        }
        if (options_.is_form) {
            return surface_statement(make_expr(ExprKind::name, here_me()),
                                     "Print");
        }
        auto result = make(StmtKind::print);
        advance();
        while (!at_end_of_statement() && !is_word("else")) {
            // A separator with no item before it (`Print , x`) is an empty
            // item.
            result->items.push_back(
                is_symbol(",") || is_symbol(";") ? nullptr : expression());
            char separator = ' ';
            if (accept_symbol(";")) {
                separator = ';';
            } else if (accept_symbol(",")) {
                separator = ',';
            }
            result->separators.push_back(separator);
            if (separator == ' ') {
                break;
            }
            result->newline =
                at_end_of_statement() || is_word("else") ? false : true;
        }
        return result;
    }

    [[nodiscard]] Position here_me() const { return here(); }

    // `Print`, `Line`, `Circle`, `PSet` and `Scale` draw on a form or a picture
    // box: the shared runtime runs the statement with the object as its first
    // argument. The current token is the keyword.
    [[nodiscard]] StmtPtr surface_statement(ExprPtr target,
                                            const std::string& keyword) {
        if (target->kind == ExprKind::name && target->text.empty()) {
            target->text = "me";
        }
        auto result = make(StmtKind::engine);
        advance();  // the keyword
        result->text = engine_argument(*result, std::move(target), 'i') + "." +
                       keyword + " ";
        if (keyword == "Print") {
            while (!at_end_of_statement() && !is_word("else")) {
                if (is_symbol(";") || is_symbol(",")) {
                    result->text += current().text;
                    advance();
                    continue;
                }
                result->text += " ";
                if ((is_word("spc") || is_word("tab")) &&
                    ahead(1).kind == TokenKind::symbol &&
                    ahead(1).text == "(") {
                    const std::string function = current().spelling;
                    advance();
                    advance();
                    result->text +=
                        function + "(" +
                        engine_argument(*result, expression(), 'i') + ")";
                    expect_symbol(")");
                    continue;
                }
                result->text += engine_argument(*result, expression(), 'i');
            }
            return result;
        }
        // Coordinates in parentheses, joined by `-`, then arguments and the
        // B and BF flags of Line.
        while (!at_end_of_statement() && !is_word("else")) {
            if (is_symbol("(")) {
                advance();
                result->text += "(";
                bool first = true;
                do {
                    result->text += (first ? "" : ", ") +
                                    engine_argument(*result, expression(), 'i');
                    first = false;
                } while (accept_symbol(","));
                expect_symbol(")");
                result->text += ")";
            } else if (is_symbol("-")) {
                advance();
                result->text += "-";
            } else if (is_symbol(",")) {
                advance();
                result->text += ", ";
            } else if (is_word("step")) {
                advance();
                result->text += "Step ";
            } else if ((is_word("b") || is_word("bf")) &&
                       (ahead(1).kind == TokenKind::end_of_statement ||
                        ahead(1).kind == TokenKind::end_of_file ||
                        (ahead(1).kind == TokenKind::symbol &&
                         ahead(1).text == ","))) {
                result->text += current().spelling;
                advance();
            } else {
                result->text += engine_argument(*result, expression(), 'i');
            }
        }
        return result;
    }

    // `object.Print`, `.Line`, `.Circle`, `.PSet` and `.Scale` as a
    // statement; null when the statement is something else.
    [[nodiscard]] StmtPtr member_surface_statement() {
        if (current().kind != TokenKind::identifier ||
            current().text == "err") {
            return nullptr;
        }
        const auto skip_parentheses = [&](std::size_t i) {
            int depth = 0;
            while (true) {
                const Token& token = ahead(i);
                if (token.kind == TokenKind::end_of_statement ||
                    token.kind == TokenKind::end_of_file) {
                    return std::size_t{0};
                }
                if (token.kind == TokenKind::symbol && token.text == "(") {
                    ++depth;
                } else if (token.kind == TokenKind::symbol &&
                           token.text == ")") {
                    if (--depth == 0) {
                        return i + 1;
                    }
                }
                ++i;
            }
        };
        std::size_t i = 1;
        if (ahead(i).kind == TokenKind::symbol && ahead(i).text == "(") {
            i = skip_parentheses(i);
            if (i == 0) {
                return nullptr;
            }
        }
        while (ahead(i).kind == TokenKind::symbol && ahead(i).text == "." &&
               ahead(i + 1).kind == TokenKind::identifier) {
            const std::string member = ahead(i + 1).text;
            const Token& after = ahead(i + 2);
            const bool surface = member == "print" || member == "line" ||
                                 member == "circle" || member == "pset" ||
                                 member == "scale";
            if (surface && !(after.kind == TokenKind::symbol &&
                             (after.text == "=" || after.text == "."))) {
                // The object is everything before the dot.
                const std::size_t dot = index_ + i;
                const Token saved = tokens_[dot];
                tokens_[dot].kind = TokenKind::end_of_statement;
                auto object = primary();
                tokens_[dot] = saved;
                advance();  // the dot
                std::string keyword = current().spelling;
                keyword[0] = static_cast<char>(
                    std::toupper(static_cast<unsigned char>(keyword[0])));
                return surface_statement(std::move(object), keyword);
            }
            i += 2;
            if (ahead(i).kind == TokenKind::symbol && ahead(i).text == "(") {
                i = skip_parentheses(i);
                if (i == 0) {
                    return nullptr;
                }
            }
        }
        return nullptr;
    }

    // Whether `name ( ... ) =` starts here: an assignment to an element.
    [[nodiscard]] bool is_element_assignment() const {
        if (!(ahead(1).kind == TokenKind::symbol && ahead(1).text == "(")) {
            return false;
        }
        int depth = 0;
        for (std::size_t i = 1;; ++i) {
            const Token& token = ahead(i);
            if (token.kind == TokenKind::end_of_statement ||
                token.kind == TokenKind::end_of_file) {
                return false;
            }
            if (token.kind == TokenKind::symbol && token.text == "(") {
                ++depth;
            } else if (token.kind == TokenKind::symbol && token.text == ")") {
                if (--depth == 0) {
                    const Token& next = ahead(i + 1);
                    return next.kind == TokenKind::symbol && next.text == "=";
                }
            }
        }
    }

    // Whether `name(...)(...)... =` starts here.
    [[nodiscard]] bool is_index_chain_assignment() const {
        std::size_t i = 1;
        int groups = 0;
        while ((ahead(i).kind == TokenKind::symbol && ahead(i).text == "(") ||
               (ahead(i).kind == TokenKind::symbol && ahead(i).text == "!" &&
                ahead(i + 1).kind == TokenKind::identifier)) {
            if (ahead(i).text == "!") {
                // `rs!name`: counts as an index.
                i += 2;
                groups += groups == 0 ? 2 : 1;
                continue;
            }
            int depth = 0;
            while (true) {
                const Token& token = ahead(i);
                if (token.kind == TokenKind::end_of_statement ||
                    token.kind == TokenKind::end_of_file) {
                    return false;
                }
                if (token.kind == TokenKind::symbol && token.text == "(") {
                    ++depth;
                } else if (token.kind == TokenKind::symbol &&
                           token.text == ")") {
                    if (--depth == 0) {
                        break;
                    }
                }
                ++i;
            }
            ++i;
            ++groups;
        }
        return groups >= 2 && ahead(i).kind == TokenKind::symbol &&
               ahead(i).text == "=";
    }

    // Whether `name [(...)] . field ... =` starts here: an assignment to a
    // field of a record.
    [[nodiscard]] bool is_field_assignment() const {
        std::size_t i = 1;
        bool saw_field = false;
        while (true) {
            const Token& token = ahead(i);
            if (token.kind == TokenKind::symbol && token.text == "(") {
                int depth = 0;
                while (true) {
                    const Token& inner = ahead(i);
                    if (inner.kind == TokenKind::end_of_statement ||
                        inner.kind == TokenKind::end_of_file) {
                        return false;
                    }
                    if (inner.kind == TokenKind::symbol && inner.text == "(") {
                        ++depth;
                    } else if (inner.kind == TokenKind::symbol &&
                               inner.text == ")") {
                        if (--depth == 0) {
                            break;
                        }
                    }
                    ++i;
                }
                ++i;
            } else if (token.kind == TokenKind::symbol && token.text == "." &&
                       ahead(i + 1).kind == TokenKind::identifier) {
                saw_field = true;
                i += 2;
            } else {
                return saw_field && token.kind == TokenKind::symbol &&
                       token.text == "=";
            }
        }
    }

    [[nodiscard]] StmtPtr assignment_or_call() {
        if (current().kind != TokenKind::identifier) {
            fail("expected a statement");
        }
        if (is_field_assignment()) {
            auto result = make(StmtKind::assign);
            result->target = primary();
            expect_symbol("=");
            result->expr = expression();
            return result;
        }
        if (is_index_chain_assignment()) {
            // `jag(2)(0) = x`: an element of the array an element holds.
            auto result = make(StmtKind::assign);
            result->target = primary();
            expect_symbol("=");
            result->expr = expression();
            return result;
        }
        if (is_element_assignment()) {
            auto result = make(StmtKind::assign);
            result->text = without_suffix(current().text);
            advance();
            expect_symbol("(");
            do {
                result->items.push_back(expression());
            } while (accept_symbol(","));
            expect_symbol(")");
            expect_symbol("=");
            result->expr = expression();
            return result;
        }
        if (ahead(1).kind == TokenKind::symbol && ahead(1).text == "=") {
            auto result = make(StmtKind::assign);
            result->text = without_suffix(current().text);
            advance();
            advance();
            result->expr = expression();
            return result;
        }
        if (current().text == "debug" && ahead(1).kind == TokenKind::symbol &&
            ahead(1).text == "." &&
            (ahead(2).text == "print" || ahead(2).text == "assert")) {
            return debug_print_statement();
        }
        if (auto drawn = member_surface_statement()) {
            return drawn;
        }
        if (starts_member_chain()) {
            return member_statement(false);
        }
        return call_statement(false);
    }

    // `Debug.Print items`.
    [[nodiscard]] StmtPtr debug_print_statement() {
        // The Immediate window shows nothing in a compiled program: the
        // items are evaluated and dropped.
        auto result = make(StmtKind::call);
        advance();
        advance();
        advance();
        result->text = "wfcdebugprint";
        while (!at_end_of_statement() && !is_word("else")) {
            if (is_symbol(";") || is_symbol(",")) {
                advance();
                continue;
            }
            result->items.push_back(expression());
        }
        return result;
    }

    // Whether `name[(...)] . member ...` starts here (not the Err object).
    [[nodiscard]] bool starts_member_chain() const {
        if (current().kind != TokenKind::identifier ||
            current().text == "err") {
            return false;
        }
        std::size_t i = 1;
        if (ahead(i).kind == TokenKind::symbol && ahead(i).text == "(") {
            int depth = 0;
            while (true) {
                const Token& token = ahead(i);
                if (token.kind == TokenKind::end_of_statement ||
                    token.kind == TokenKind::end_of_file) {
                    return false;
                }
                if (token.kind == TokenKind::symbol && token.text == "(") {
                    ++depth;
                } else if (token.kind == TokenKind::symbol &&
                           token.text == ")") {
                    if (--depth == 0) {
                        break;
                    }
                }
                ++i;
            }
            ++i;
        }
        return ahead(i).kind == TokenKind::symbol && ahead(i).text == ".";
    }

    // `obj.Member args` or `Call obj.Member(args)`: the member is called.
    [[nodiscard]] StmtPtr member_statement(const bool with_call_keyword) {
        return finish_member_statement(primary(), with_call_keyword);
    }

    [[nodiscard]] StmtPtr finish_member_statement(
        ExprPtr chain, const bool with_call_keyword) {
        auto result = make(StmtKind::call_member);
        // `obj.Member (x)` without Call passes the parenthesized value.
        if (!with_call_keyword && chain->kind == ExprKind::member_call &&
            chain->operands.size() == 2U) {
            chain->operands[1]->parenthesized = true;
        }
        if (chain->kind == ExprKind::member && !with_call_keyword) {
            // Arguments without parentheses: `obj.Member a, b`.
            auto call = make_expr(ExprKind::member_call, chain->at);
            call->text = chain->text;
            call->operands.push_back(std::move(chain->operands[0]));
            while (!at_end_of_statement() && !is_word("else")) {
                if (is_symbol(",")) {
                    call->operands.push_back(nullptr);  // omitted
                } else {
                    call->operands.push_back(argument());
                }
                if (!accept_symbol(",")) {
                    break;
                }
            }
            chain = std::move(call);
        }
        result->expr = std::move(chain);
        return result;
    }

    // Whether the parenthesis here closes before the end of the statement
    // with more after it (`(a) + 1`, `(a), b`): it opens an expression.
    [[nodiscard]] bool parenthesis_starts_expression() const {
        int depth = 0;
        for (std::size_t i = 0;; ++i) {
            const Token& token = ahead(i);
            if (token.kind == TokenKind::end_of_statement ||
                token.kind == TokenKind::end_of_file) {
                return false;
            }
            if (token.kind == TokenKind::symbol && token.text == "(") {
                ++depth;
            } else if (token.kind == TokenKind::symbol && token.text == ")") {
                if (--depth == 0) {
                    const Token& after = ahead(i + 1);
                    return !(after.kind == TokenKind::end_of_statement ||
                             after.kind == TokenKind::end_of_file ||
                             (after.kind == TokenKind::identifier &&
                              after.text == "else"));
                }
            }
        }
    }

    [[nodiscard]] StmtPtr call_statement(const bool with_call_keyword) {
        if (with_call_keyword && starts_member_chain()) {
            return member_statement(true);
        }
        auto result = make(StmtKind::call);
        if (current().kind != TokenKind::identifier) {
            fail("expected the procedure name");
        }
        result->text = without_suffix(current().text);
        const bool is_err = current().text == "err";
        advance();
        if (is_symbol(".")) {
            if (!is_err) {
                fail("members of objects are not compiled yet");
            }
            advance();
            if (current().kind != TokenKind::identifier) {
                fail("expected a member name after Err.");
            }
            result->text = "err." + current().text;
            advance();
        }
        if (is_symbol("(") && !with_call_keyword && !is_err &&
            parenthesis_starts_expression()) {
            // `Name (a) + 1, b`: the arguments are not in parentheses.
            while (!at_end_of_statement() && !is_word("else")) {
                result->items.push_back(is_symbol(",") ? nullptr : argument());
                if (!accept_symbol(",")) {
                    break;
                }
            }
            return result;
        }
        if (accept_symbol("(")) {
            if (!accept_symbol(")")) {
                do {
                    // An omitted argument (`Err.Raise 5, , "text"`) is null.
                    result->items.push_back(is_symbol(",") ? nullptr
                                                           : argument());
                } while (accept_symbol(","));
                expect_symbol(")");
            }
            // `Name (x)` without Call passes the parenthesized value.
            if (!with_call_keyword && result->items.size() == 1U) {
                result->items.front()->parenthesized = true;
            }
        } else if (!with_call_keyword) {
            // `name arg, arg`: arguments without parentheses.
            while (!at_end_of_statement() && !is_word("else")) {
                result->items.push_back(is_symbol(",") ? nullptr : argument());
                if (!accept_symbol(",")) {
                    break;
                }
            }
        }
        return result;
    }

    [[nodiscard]] StmtPtr if_statement() {
        auto result = make(StmtKind::if_block);
        advance();
        result->conditions.push_back(expression());
        expect_word("then");
        if (!at_end_of_statement()) {
            // The single-line form: If c Then stmt [: stmt] [Else stmt ...]
            const auto branch = [&]() {
                std::vector<StmtPtr> list;
                list.push_back(statement());
                while (current().kind == TokenKind::end_of_statement &&
                       current().text == ":") {
                    advance();
                    if (at_end_of_statement() || is_word("else")) {
                        break;
                    }
                    list.push_back(statement());
                }
                return list;
            };
            result->blocks.push_back(branch());
            if (accept_word("else")) {
                result->blocks.push_back(branch());
                result->has_else = true;
            }
            return result;
        }
        result->blocks.push_back(block({"elseif", "else", "end"}));
        while (true) {
            if (accept_word("elseif")) {
                result->conditions.push_back(expression());
                expect_word("then");
                result->blocks.push_back(block({"elseif", "else", "end"}));
            } else if (accept_word("else")) {
                result->blocks.push_back(block({"end"}));
                result->has_else = true;
            } else {
                break;
            }
        }
        expect_word("end");
        expect_word("if");
        return result;
    }

    // `Next`, its optional variable and, in `Next c, r`, the variables of the
    // loops around it, which this Next closes as well.
    void finish_next() {
        if (closing_loops_ > 0) {
            --closing_loops_;
            return;
        }
        expect_word("next");
        if (current().kind == TokenKind::identifier) {
            advance();
            while (accept_symbol(",")) {
                if (current().kind != TokenKind::identifier) {
                    fail("expected a loop variable");
                }
                advance();
                ++closing_loops_;
            }
        }
    }

    [[nodiscard]] StmtPtr for_statement() {
        auto result = make(StmtKind::for_loop);
        advance();
        if (accept_word("each")) {
            result->kind = StmtKind::for_each;
            if (current().kind != TokenKind::identifier) {
                fail("expected the loop variable");
            }
            result->text = without_suffix(current().text);
            advance();
            expect_word("in");
            result->expr = expression();
            result->body = block({"next"});
            finish_next();
            return result;
        }
        if (current().kind != TokenKind::identifier) {
            fail("expected the loop variable");
        }
        result->text = without_suffix(current().text);
        advance();
        expect_symbol("=");
        result->expr = expression();
        expect_word("to");
        result->limit = expression();
        if (accept_word("step")) {
            result->step = expression();
        }
        result->body = block({"next"});
        finish_next();
        return result;
    }

    [[nodiscard]] StmtPtr do_statement() {
        auto result = make(StmtKind::do_loop);
        advance();
        bool tested_first = false;
        if (accept_word("while")) {
            result->expr = expression();
            tested_first = true;
        } else if (accept_word("until")) {
            result->expr = expression();
            result->until = true;
            tested_first = true;
        }
        result->body = block({"loop"});
        expect_word("loop");
        if (!tested_first) {
            if (accept_word("while")) {
                result->expr = expression();
                result->test_after = true;
            } else if (accept_word("until")) {
                result->expr = expression();
                result->until = true;
                result->test_after = true;
            }
        }
        return result;
    }

    [[nodiscard]] StmtPtr while_statement() {
        auto result = make(StmtKind::while_loop);
        advance();
        result->expr = expression();
        result->body = block({"wend"});
        expect_word("wend");
        return result;
    }

    [[nodiscard]] StmtPtr exit_statement() {
        auto result = make(StmtKind::exit);
        advance();
        if (is_word("sub") || is_word("function") || is_word("for") ||
            is_word("do") || is_word("property")) {
            result->text = current().text;
            advance();
            return result;
        }
        fail("expected Sub, Function, Property, For or Do after Exit");
    }

    // ----- Expressions -----------------------------------------------------

    [[nodiscard]] ExprPtr make_expr(const ExprKind kind,
                                    const Position at) const {
        auto result = std::make_unique<Expr>();
        result->kind = kind;
        result->at = at;
        return result;
    }

    [[nodiscard]] ExprPtr binary(std::string op, ExprPtr left, ExprPtr right,
                                 const Position at) const {
        auto result = make_expr(ExprKind::binary, at);
        result->text = std::move(op);
        result->operands.push_back(std::move(left));
        result->operands.push_back(std::move(right));
        return result;
    }

    [[nodiscard]] ExprPtr expression() { return xor_expression(); }

    // A call argument: `value` or `name := value`.
    [[nodiscard]] ExprPtr argument() {
        if (is_symbol("#")) {
            advance();  // `EOF(#1)`: the file number's hash is only a mark
        }
        if (current().kind == TokenKind::identifier &&
            ahead(1).kind == TokenKind::end_of_statement &&
            ahead(1).text == ":" && ahead(2).kind == TokenKind::symbol &&
            ahead(2).text == "=" && ahead(2).column == ahead(1).column + 1U) {
            const auto name = without_suffix(current().text);
            advance();
            advance();
            advance();
            auto value = expression();
            value->arg_name = name;
            return value;
        }
        return expression();
    }

    [[nodiscard]] ExprPtr xor_expression() {
        auto left = or_expression();
        while (is_word("xor")) {
            const auto at = here();
            advance();
            left = binary("xor", std::move(left), or_expression(), at);
        }
        if (is_word("eqv") || is_word("imp")) {
            fail(current().spelling + " is not compiled yet");
        }
        return left;
    }

    [[nodiscard]] ExprPtr or_expression() {
        auto left = and_expression();
        while (is_word("or")) {
            const auto at = here();
            advance();
            left = binary("or", std::move(left), and_expression(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr and_expression() {
        auto left = not_expression();
        while (is_word("and")) {
            const auto at = here();
            advance();
            left = binary("and", std::move(left), not_expression(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr not_expression() {
        if (is_word("not")) {
            const auto at = here();
            advance();
            auto result = make_expr(ExprKind::unary, at);
            result->text = "not";
            result->operands.push_back(not_expression());
            return result;
        }
        return comparison();
    }

    [[nodiscard]] ExprPtr comparison() {
        auto left = concatenation();
        while (current().kind == TokenKind::symbol &&
               (current().text == "=" || current().text == "<>" ||
                current().text == "<" || current().text == ">" ||
                current().text == "<=" || current().text == ">=")) {
            const auto at = here();
            const std::string op = current().text;
            advance();
            left = binary(op, std::move(left), concatenation(), at);
        }
        if (is_word("is")) {
            const auto at = here();
            advance();
            left = binary("is", std::move(left), concatenation(), at);
        }
        if (is_word("like")) {
            const auto at = here();
            advance();
            left = binary("like", std::move(left), concatenation(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr concatenation() {
        auto left = additive();
        while (is_symbol("&")) {
            const auto at = here();
            advance();
            left = binary("&", std::move(left), additive(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr additive() {
        auto left = modulus();
        while (is_symbol("+") || is_symbol("-")) {
            const auto at = here();
            const std::string op = current().text;
            advance();
            left = binary(op, std::move(left), modulus(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr modulus() {
        auto left = integer_division();
        while (is_word("mod")) {
            const auto at = here();
            advance();
            left = binary("mod", std::move(left), integer_division(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr integer_division() {
        auto left = multiplicative();
        while (is_symbol("\\")) {
            const auto at = here();
            advance();
            left = binary("\\", std::move(left), multiplicative(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr multiplicative() {
        auto left = unary();
        while (is_symbol("*") || is_symbol("/")) {
            const auto at = here();
            const std::string op = current().text;
            advance();
            left = binary(op, std::move(left), unary(), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr unary() {
        if (is_symbol("-") || is_symbol("+")) {
            const auto at = here();
            const bool negate = is_symbol("-");
            advance();
            auto operand = unary();
            if (!negate) {
                return operand;
            }
            auto result = make_expr(ExprKind::unary, at);
            result->text = "-";
            result->operands.push_back(std::move(operand));
            return result;
        }
        return power();
    }

    // `^` is left to right and binds tighter than a unary minus; the exponent
    // may carry a sign of its own.
    [[nodiscard]] ExprPtr power() {
        auto left = primary();
        while (is_symbol("^")) {
            const auto at = here();
            advance();
            bool negate = false;
            if (is_symbol("-") || is_symbol("+")) {
                negate = is_symbol("-");
                advance();
            }
            auto exponent = primary();
            if (negate) {
                auto minus = make_expr(ExprKind::unary, at);
                minus->text = "-";
                minus->operands.push_back(std::move(exponent));
                exponent = std::move(minus);
            }
            left = binary("^", std::move(left), std::move(exponent), at);
        }
        return left;
    }

    [[nodiscard]] ExprPtr primary() {
        const auto at = here();
        const Token token = current();
        if ((token.kind == TokenKind::integer ||
             token.kind == TokenKind::real) &&
            token.currency_literal) {
            // `922337203685477.5807@`: CCur of the digits as text.
            advance();
            auto call = make_expr(ExprKind::call, at);
            call->text = "wfcliteral";
            auto digits = make_expr(ExprKind::string_literal, at);
            digits->text = token.text + "@";
            call->operands.push_back(std::move(digits));
            return call;
        }
        if (token.kind == TokenKind::integer) {
            advance();
            auto result = make_expr(ExprKind::integer_literal, at);
            result->number = token.number;
            result->long_literal = token.long_literal;
            return result;
        }
        if (token.kind == TokenKind::real) {
            advance();
            auto result = make_expr(ExprKind::real_literal, at);
            result->real = std::strtod(token.text.c_str(), nullptr);
            result->single_literal = token.single_literal;
            return result;
        }
        if (token.kind == TokenKind::date) {
            advance();
            auto result = make_expr(ExprKind::date_literal, at);
            result->real = std::strtod(token.text.c_str(), nullptr);
            return result;
        }
        if (token.kind == TokenKind::string) {
            advance();
            auto result = make_expr(ExprKind::string_literal, at);
            result->text = token.text;
            return result;
        }
        if (token.kind == TokenKind::symbol && token.text == "(") {
            advance();
            auto inner = expression();
            expect_symbol(")");
            inner->parenthesized = true;
            return inner;
        }
        if (token.kind == TokenKind::identifier && token.text == "typeof") {
            advance();
            auto result = make_expr(ExprKind::type_of, at);
            result->operands.push_back(concatenation());
            expect_word("is");
            if (current().kind != TokenKind::identifier) {
                fail("expected a class name after TypeOf ... Is");
            }
            result->text = current().text;
            advance();
            return result;
        }
        if (token.kind == TokenKind::identifier && token.text == "new") {
            advance();
            if (current().kind != TokenKind::identifier) {
                fail("New needs the name of a class");
            }
            auto result = make_expr(ExprKind::new_object, at);
            result->text = current().text;
            advance();
            // A class of the engine may be qualified (`Scripting.Dictionary`).
            while (is_symbol(".") && ahead(1).kind == TokenKind::identifier &&
                   !options_.class_names.contains(result->text)) {
                advance();
                result->text += "." + current().text;
                advance();
            }
            return member_chain(std::move(result), at);
        }
        if (token.kind == TokenKind::identifier) {
            if (token.text == "true" || token.text == "false") {
                advance();
                auto result = make_expr(ExprKind::boolean_literal, at);
                result->number = token.text == "true" ? -1 : 0;
                return result;
            }
            advance();
            if (is_symbol(".") && token.text == "err") {
                // The Err object.
                advance();
                if (current().kind != TokenKind::identifier) {
                    fail("expected a member name after Err.");
                }
                auto member = make_expr(ExprKind::name, at);
                member->text = "err." + current().text;
                advance();
                return member;
            }
            ExprPtr node;
            if (is_symbol("(")) {
                advance();
                node = make_expr(ExprKind::call, at);
                node->text = without_suffix(token.text);
                node->suffix = token.text.back() == '$' ? '$' : '\0';
                if (!accept_symbol(")")) {
                    do {
                        // An omitted argument is null.
                        node->operands.push_back(is_symbol(",") ? nullptr
                                                                : argument());
                    } while (accept_symbol(","));
                    expect_symbol(")");
                }
            } else {
                node = make_expr(ExprKind::name, at);
                node->text = without_suffix(token.text);
                node->suffix = token.text.back() == '$' ? '$' : '\0';
            }
            return member_chain(std::move(node), at);
        }
        if (token.kind == TokenKind::symbol && token.text == "." &&
            !with_err_.empty() && with_err_.back()) {
            insert_err_before_dot();
            return primary();
        }
        if (token.kind == TokenKind::symbol && token.text == ".") {
            // `.member` inside a With block.
            return member_chain(make_expr(ExprKind::with_target, at), at);
        }
        fail("expected an expression");
    }

    // `.member` chains: a.b.c, array(i).field, obj.Method(args)
    [[nodiscard]] ExprPtr member_chain(ExprPtr node, const Position at) {
        while (true) {
            if (is_symbol("(") && (node->kind == ExprKind::call ||
                                   node->kind == ExprKind::member_call ||
                                   node->kind == ExprKind::index_of)) {
                // `Array(1, 2)(0)`, `jag(2)(1)`: an index on a result.
                advance();
                auto index = make_expr(ExprKind::index_of, at);
                index->operands.push_back(std::move(node));
                if (!accept_symbol(")")) {
                    do {
                        index->operands.push_back(
                            is_symbol(",") ? nullptr : expression());
                    } while (accept_symbol(","));
                    expect_symbol(")");
                }
                node = std::move(index);
                continue;
            }
            if (is_symbol("!") && ahead(1).kind == TokenKind::identifier &&
                (node->kind == ExprKind::name || node->kind == ExprKind::call ||
                 node->kind == ExprKind::member ||
                 node->kind == ExprKind::member_call ||
                 node->kind == ExprKind::index_of)) {
                // `rs!id`: the default member with the name as key.
                advance();
                auto index = make_expr(ExprKind::index_of, at);
                index->operands.push_back(std::move(node));
                auto key = make_expr(ExprKind::string_literal, at);
                key->text = without_suffix(current().spelling);
                advance();
                index->operands.push_back(std::move(key));
                node = std::move(index);
                continue;
            }
            if (!is_symbol(".")) {
                break;
            }
            advance();
            if (current().kind != TokenKind::identifier) {
                fail("expected a member name after '.'");
            }
            auto field = make_expr(ExprKind::member, at);
            field->text = without_suffix(current().text);
            advance();
            field->operands.push_back(std::move(node));
            if (is_symbol("(")) {
                advance();
                field->kind = ExprKind::member_call;
                if (!accept_symbol(")")) {
                    do {
                        if (is_symbol(",")) {
                            field->operands.push_back(nullptr);  // omitted
                        } else {
                            field->operands.push_back(argument());
                        }
                    } while (accept_symbol(","));
                    expect_symbol(")");
                }
            }
            node = std::move(field);
        }
        return node;
    }

    std::vector<Token> tokens_;
    DefType def_types_[26];
    std::vector<bool> with_err_;  // the With blocks that are on Err
    bool procedure_static_{};
    int closing_loops_{};  // loops still closed by an earlier `Next a, b`
    std::string default_member_;  // set by an Attribute line in a procedure
    const ParseOptions& options_;
    std::size_t index_{};
};

}  // namespace

ParseResult parse_module(const std::string_view source,
                         const ParseOptions& options) {
    ParseResult result;
    auto lexed = tokenize(source);
    if (!lexed.error.empty()) {
        result.error = lexed.error;
        result.error_at = Position{lexed.error_line, lexed.error_column};
        return result;
    }
    // `Module.name` is `name`: the modules of a project share one scope. So
    // is `VbMsgBoxResult.vbOK`: a constant named through its enumeration.
    {
        std::vector<Token> kept;
        kept.reserve(lexed.tokens.size());
        for (std::size_t i = 0; i < lexed.tokens.size(); ++i) {
            const Token& token = lexed.tokens[i];
            const bool qualifier =
                token.kind == TokenKind::identifier &&
                (options.module_names.contains(token.text) ||
                 is_library_enumeration(token.text)) &&
                i + 2 < lexed.tokens.size() &&
                lexed.tokens[i + 1].kind == TokenKind::symbol &&
                lexed.tokens[i + 1].text == "." &&
                lexed.tokens[i + 2].kind == TokenKind::identifier &&
                !(!kept.empty() && kept.back().kind == TokenKind::symbol &&
                  kept.back().text == ".");
            if (qualifier) {
                ++i;  // the module name and its dot
                continue;
            }
            kept.push_back(token);
        }
        lexed.tokens = std::move(kept);
    }
    try {
        Parser parser(std::move(lexed.tokens), options);
        result.module = parser.parse();
    } catch (const ParseFailure& failure) {
        result.error = failure.message;
        result.error_at = failure.at;
    }
    return result;
}

}  // namespace wfc::compiler
