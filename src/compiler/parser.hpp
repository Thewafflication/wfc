#pragma once

#include <set>
#include <string>
#include <string_view>

#include "ast.hpp"

namespace wfc::compiler {

struct ParseOptions final {
    std::set<std::string> class_names;  // lower-case names of the classes
    bool is_class{};                    // the source is a class module
    std::string name;                   // the module or class name
};

// Parses one module. On a syntax error, or on a construct the compiler does
// not support yet, `error` is set and says which.
[[nodiscard]] ParseResult parse_module(std::string_view source,
                                       const ParseOptions& options = {});

}  // namespace wfc::compiler
