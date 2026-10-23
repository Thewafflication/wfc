#pragma once

#include <set>
#include <string>
#include <string_view>

#include "ast.hpp"

namespace wfc::compiler {

struct ParseOptions final {
    std::set<std::string> class_names;   // lower-case names of the classes
    std::set<std::string> enum_names;    // lower-case names of the enums
    bool is_class{};                     // the source is a class module
    bool is_form{};                      // ... of a form
    std::set<std::string> form_names;    // lower-case names of the forms
    std::set<std::string> type_names;    // lower-case names of the Types
    std::set<std::string> module_names;  // lower-case names of the modules
    std::string name;                    // the module or class name
    std::string display_name;            // the name as written
};

// Parses one module. On a syntax error, or on a construct the compiler does
// not support yet, `error` is set and says which.
[[nodiscard]] ParseResult parse_module(std::string_view source,
                                       const ParseOptions& options = {});

}  // namespace wfc::compiler
