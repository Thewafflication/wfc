#include "wfc/evaluator.hpp"
#include "wfc/version.hpp"

#include <iostream>
#include <string_view>

namespace {

void print_usage() {
    std::cerr << "usage: wfc --eval <VB source>\n"
              << "       wfc --version\n";
}

}  // namespace

int main(const int argument_count, const char* const arguments[]) {
    if (argument_count == 2 && std::string_view(arguments[1]) == "--version") {
        std::cout << "wfc " << wfc::version << '\n';
        return 0;
    }

    if (argument_count != 3 || std::string_view(arguments[1]) != "--eval") {
        print_usage();
        return 2;
    }

    const auto evaluation = wfc::evaluate_program(arguments[2]);
    if (!evaluation.success) {
        std::cerr << evaluation.diagnostic << '\n';
        return 1;
    }

    std::cout << evaluation.output << '\n';
    return 0;
}
