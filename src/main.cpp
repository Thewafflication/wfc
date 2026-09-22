#include "wfc/evaluator.hpp"
#include "wfc/version.hpp"

#include <iostream>
#include <string_view>
#include <vector>

namespace {

void print_usage() {
    std::cerr << "usage: wfc --eval <VB source>\n"
              << "       wfc [--class <Name> <class source>]... --eval <VB source>\n"
              << "       wfc --version\n";
}

}  // namespace

int main(const int argument_count, const char* const arguments[]) {
    if (argument_count == 2 && std::string_view(arguments[1]) == "--version") {
        std::cout << "wfc " << wfc::version << '\n';
        return 0;
    }

    // `--class <Name> <source>` may repeat any number of times, supplying a
    // separate class module source alongside the standard module -- this
    // evaluator's stand-in for a real VB6 project's separate .cls files
    // (see wfc::ClassModuleSource). `--eval <source>` (the standard module)
    // is required exactly once, in any position relative to the `--class`
    // options.
    std::vector<wfc::ClassModuleSource> classes;
    const char* module_source = nullptr;
    int index = 1;
    while (index < argument_count) {
        const std::string_view argument = arguments[index];
        if (argument == "--class" && index + 2 < argument_count) {
            classes.push_back(wfc::ClassModuleSource{arguments[index + 1], arguments[index + 2]});
            index += 3;
            continue;
        }
        if (argument == "--eval" && index + 1 < argument_count && module_source == nullptr) {
            module_source = arguments[index + 1];
            index += 2;
            continue;
        }
        print_usage();
        return 2;
    }
    if (module_source == nullptr) {
        print_usage();
        return 2;
    }

    const auto evaluation = classes.empty() ? wfc::evaluate_program(module_source)
                                             : wfc::evaluate_program(module_source, classes);
    if (!evaluation.success) {
        std::cerr << evaluation.diagnostic << '\n';
        return 1;
    }

    std::cout << evaluation.output << '\n';
    return 0;
}
