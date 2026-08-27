#include "wfc/evaluator.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect_success(const std::string_view source, const std::string_view expected) {
    const auto result = wfc::evaluate_print_statement(source);
    if (!result.success || result.output != expected || !result.diagnostic.empty()) {
        std::cerr << "expected success for [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

void expect_failure(const std::string_view source, const std::string_view code) {
    const auto result = wfc::evaluate_print_statement(source);
    if (result.success || !result.output.empty() || !result.diagnostic.starts_with(code)) {
        std::cerr << "expected " << code << " for [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

}  // namespace

int main() {
    expect_success("Print \"Hello, WFC!\"", "Hello, WFC!");
    expect_success("  pRiNt  \"case insensitive\"  \r\n", "case insensitive");
    expect_success("Print \"VB says \"\"hello\"\".\"", "VB says \"hello\".");
    expect_success("Print \"\"", "");

    expect_failure("", "WFC0001");
    expect_failure("Printer \"no\"", "WFC0001");
    expect_failure("Print", "WFC0002");
    expect_failure("Print value", "WFC0002");
    expect_failure("Print \"unterminated", "WFC0003");
    expect_failure("Print \"ok\" trailing", "WFC0004");

    if (failures != 0) {
        std::cerr << failures << " evaluator test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "all evaluator tests passed\n";
    return EXIT_SUCCESS;
}
