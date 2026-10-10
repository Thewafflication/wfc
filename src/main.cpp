#include "wfc/evaluator.hpp"
#include "wfc/project.hpp"
#include "wfc/version.hpp"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void print_usage() {
    std::cerr
        << "usage: wfc --eval <VB source>\n"
        << "       wfc [--class <Name> <class source>]... --eval <VB source>\n"
        << "       wfc <project.vbp | module.bas [class.cls]...>\n"
        << "       wfc --version\n";
}

// "file:line:col: " for a failure inside a loaded project, or "" when unknown.
std::string project_location(const wfc::LoadedProject& project,
                             const wfc::Evaluation& result) {
    if (result.error_line == 0) {
        return {};
    }
    std::string file;
    std::size_t line = result.error_line;
    if (result.error_module.empty()) {
        for (const auto& span : project.module_spans) {
            if (span.first_line <= result.error_line) {
                file = span.file;
                line = result.error_line - span.first_line + 1U;
            }
        }
    } else {
        for (std::size_t i = 0; i < project.class_names.size(); ++i) {
            std::string a = project.class_names[i];
            std::string b = result.error_module;
            for (auto& ch : a) {
                ch = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(ch)));
            }
            for (auto& ch : b) {
                ch = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(ch)));
            }
            if (a == b && i < project.class_files.size()) {
                file = project.class_files[i];
            }
        }
    }
    if (file.empty()) {
        return {};
    }
    return file + ":" + std::to_string(line) + ":" +
           std::to_string(result.error_column) + ": ";
}

}  // namespace

int main(const int argument_count, const char* const arguments[]) {
    if (argument_count == 2 && std::string_view(arguments[1]) == "--version") {
        std::cout << "wfc " << wfc::version << '\n';
        return 0;
    }

    // `wfc file.vbp` / `wfc a.bas b.cls ...` loads real VB6 project files.
    if (argument_count >= 2 &&
        std::string_view(arguments[1]).rfind("--", 0) != 0) {
        std::vector<std::filesystem::path> files;
        for (int i = 1; i < argument_count; ++i) {
            files.emplace_back(arguments[i]);
        }
        const auto project = wfc::load_project(files);
        if (!project.ok) {
            std::cerr << project.error << '\n';
            return 2;
        }
        std::vector<wfc::ClassModuleSource> project_classes;
        for (std::size_t i = 0; i < project.class_names.size(); ++i) {
            project_classes.push_back(wfc::ClassModuleSource{
                project.class_names[i], project.class_sources[i]});
        }
        // Running real project files: print numbers the way VB6 does.
        wfc::EvaluationOptions options;
        options.vb6_print_spacing = true;
        options.app_properties = project.app_properties;
        options.per_module_option_explicit = project.per_module_option_explicit;
        options.option_explicit_ranges = project.option_explicit_ranges;
        const auto result = wfc::evaluate_program(project.module_source,
                                                  project_classes, options);
        if (!result.debug_output.empty()) {
            std::cerr << result.debug_output;
        }
        if (!result.success) {
            if (!result.partial_output.empty()) {
                std::cout << result.partial_output << '\n';
                std::cout.flush();
            }
            std::cerr << project_location(project, result) << result.diagnostic
                      << '\n';
            if (result.vb_error_number != 0) {
                std::cerr << "Run-time error '" << result.vb_error_number
                          << "': " << result.vb_error_description << '\n';
            }
            return 1;
        }
        std::cout << result.output << '\n';
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
            classes.push_back(wfc::ClassModuleSource{arguments[index + 1],
                                                     arguments[index + 2]});
            index += 3;
            continue;
        }
        if (argument == "--eval" && index + 1 < argument_count &&
            module_source == nullptr) {
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

    const auto evaluation = classes.empty()
                                ? wfc::evaluate_program(module_source)
                                : wfc::evaluate_program(module_source, classes);
    if (!evaluation.debug_output.empty()) {
        std::cerr << evaluation.debug_output;
    }
    if (!evaluation.success) {
        if (!evaluation.partial_output.empty()) {
            std::cout << evaluation.partial_output << '\n';
            std::cout.flush();
        }
        if (evaluation.error_line != 0) {
            std::cerr << "line " << evaluation.error_line << ", column "
                      << evaluation.error_column << ": ";
        }
        std::cerr << evaluation.diagnostic << '\n';
        if (evaluation.vb_error_number != 0) {
            std::cerr << "Run-time error '" << evaluation.vb_error_number
                      << "': " << evaluation.vb_error_description << '\n';
        }
        return 1;
    }

    std::cout << evaluation.output << '\n';
    return 0;
}
