#pragma once

#include <string_view>

#include "ast.hpp"

namespace wfc::compiler {

// Parses one standard module. On a syntax error, or on a construct the
// compiler does not support yet, `error` is set and says which.
[[nodiscard]] ParseResult parse_module(std::string_view source);

}  // namespace wfc::compiler
