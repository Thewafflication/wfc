#include "wfc/evaluator.hpp"
#include "wfc/project.hpp"
#include "wfc/version.hpp"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>
#include "com_server.hpp"
#include "compiler/compile.hpp"
#include "compiler/native_host.hpp"
#include "interpreter/native_engine.hpp"
#include "package.hpp"

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

namespace {

int run(int argument_count, const char* const arguments[]);

// `wfc --build <project.vbp> -o <app.exe> [--gui]`: a packaged program.
int build_package(const int argument_count, const char* const arguments[]) {
    std::filesystem::path project;
    std::filesystem::path output;
    std::vector<std::string> options;
    for (int i = 2; i < argument_count; ++i) {
        const std::string_view argument = arguments[i];
        if (argument == "-o" && i + 1 < argument_count) {
            output = arguments[++i];
        } else if (argument == "--gui") {
            options.emplace_back("--gui");
        } else if (project.empty() && argument.rfind("-", 0) != 0) {
            project = arguments[i];
        } else {
            print_usage();
            return 2;
        }
    }
    if (project.empty() || output.empty()) {
        std::cerr << "usage: wfc --build <project.vbp> -o <app.exe> [--gui]\n";
        return 2;
    }
    std::string problem;
    if (!wfc::package::build(wfc::package::running_image(), project, output,
                             options, problem)) {
        std::cerr << problem << '\n';
        return 1;
    }
    return 0;
}

// The value of an environment variable, empty when it is not set.
[[nodiscard]] std::string environment_text(const char* name) {
#ifdef _WIN32
    char* buffer = nullptr;
    std::size_t length = 0;
    std::string result;
    if (_dupenv_s(&buffer, &length, name) == 0 && buffer != nullptr) {
        result = buffer;
        std::free(buffer);
    }
    return result;
#else
    const char* found = std::getenv(name);
    return found != nullptr ? found : "";
#endif
}

// The `Declare` statements of a source (with their continuation lines): the
// shared runtime calls the routines of the DLLs.
std::string declares_of(const std::string& source) {
    std::string found;
    std::size_t position = 0;
    while (position < source.size()) {
        auto end = source.find('\n', position);
        if (end == std::string::npos) {
            end = source.size();
        }
        std::string line = source.substr(position, end - position);
        position = end + 1U;
        std::string lowered;
        for (const char c : line) {
            lowered.push_back(
                static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        const auto first = lowered.find_first_not_of(" \t");
        if (first == std::string::npos) {
            continue;
        }
        std::string_view rest = std::string_view(lowered).substr(first);
        for (const std::string_view scope : {"public ", "private "}) {
            if (rest.substr(0, scope.size()) == scope) {
                rest.remove_prefix(scope.size());
                break;
            }
        }
        if (rest.substr(0, 8) != "declare ") {
            continue;
        }
        // The declaration goes on while a line ends with a continuation.
        auto trim_end = [](std::string& text) {
            while (!text.empty() &&
                   (text.back() == '\r' || text.back() == ' ' ||
                    text.back() == '\t')) {
                text.pop_back();
            }
        };
        trim_end(line);
        std::string joined = line;
        while (!joined.empty() && joined.back() == '_' &&
               position < source.size()) {
            joined.pop_back();
            auto next_end = source.find('\n', position);
            if (next_end == std::string::npos) {
                next_end = source.size();
            }
            std::string next = source.substr(position, next_end - position);
            position = next_end + 1U;
            trim_end(next);
            joined += next;
        }
        found += joined + '\n';
    }
    return found;
}

// `wfc --native <project.vbp | module.bas>`: compiles the project to machine
// code in memory and runs it (ADR-0008). Exit code 77 means that this
// architecture has no back end yet.
int run_native(const int argument_count, const char* const arguments[]) {
    std::vector<std::filesystem::path> files;
    wfc::EvaluationOptions options;
    bool fit_screen = false;
    const auto absolute_text = [](const std::string& text) {
        return text.empty() ? text : std::filesystem::absolute(text).string();
    };
    std::string ui_script_override;
    for (int i = 2; i < argument_count; ++i) {
        const std::string_view argument = arguments[i];
        if (argument == "--gui") {
            options.ui_mode = "win32";
        } else if (argument == "--fullscreen") {
            options.ui_mode = "win32";
            fit_screen = true;
        } else if (argument == "--exit-after" && i + 1 < argument_count) {
            options.exit_after_ms = std::atol(arguments[++i]);
        } else if (argument == "--ui-script" && i + 1 < argument_count) {
            ui_script_override = absolute_text(arguments[++i]);
        } else if (argument == "--screenshots" && i + 1 < argument_count) {
            options.screenshot_directory = absolute_text(arguments[++i]);
        } else {
            files.emplace_back(arguments[i]);
        }
    }
    if (!files.empty() && files.front().extension() == ".vbp") {
        for (auto& file : files) {
            file = std::filesystem::absolute(file);
        }
    }
    const auto project = wfc::load_project(files);
    if (!project.ok) {
        std::cerr << project.error << '\n';
        return 2;
    }
    if (!files.empty() && files.front().extension() == ".vbp") {
        std::error_code ignored;
        std::filesystem::current_path(files.front().parent_path(), ignored);
    }
    const auto architecture = wfc::compiler::host_architecture();
    if (!wfc::compiler::native_back_end_available()) {
        std::cerr << "native compilation is not available on this "
                     "architecture yet\n";
        return 77;
    }
    // The loader ends a project that starts at Sub Main with a `Call Main`
    // statement for the interpreter; the compiler takes Sub Main as the entry.
    std::string source = project.module_source;
    if (const std::string_view call = "Call Main\n";
        source.size() >= call.size() &&
        std::string_view(source).substr(source.size() - call.size()) == call) {
        source.resize(source.size() - call.size());
    }
    if (!environment_text("WFC_DUMP_SOURCE").empty()) {
        std::cerr << source << '\n';
    }
    std::vector<wfc::compiler::ClassSource> classes;
    wfc::detail::EngineSetup setup;
    wfc::compiler::ProjectInfo info;
    const auto lower = [](std::string text) {
        for (auto& c : text) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return text;
    };
    for (std::size_t i = 0; i < project.class_names.size(); ++i) {
        if (i < project.class_form_texts.size() &&
            !project.class_form_texts[i].empty()) {
            // A form lives in the engine; its code is not compiled yet, so
            // only the design is loaded.
            setup.classes.push_back(wfc::ClassModuleSource{
                project.class_names[i], std::string_view{},
                project.class_form_texts[i],
                project.class_form_directories[i]});
            info.form_names.push_back(lower(project.class_names[i]));
            wfc::compiler::ClassSource form{
                project.class_names[i], project.class_sources[i], true, {}};
            // Its controls: `Begin VB.TextBox Text1`.
            const std::string& design = project.class_form_texts[i];
            std::size_t at = 0;
            while (at < design.size()) {
                auto end = design.find('\n', at);
                if (end == std::string::npos) {
                    end = design.size();
                }
                std::istringstream words(design.substr(at, end - at));
                std::string first;
                std::string kind;
                std::string name;
                words >> first >> kind >> name;
                if (lower(first) == "begin" && !name.empty()) {
                    form.controls.push_back(lower(name));
                }
                at = end + 1U;
            }
            classes.push_back(std::move(form));
            continue;
        }
        classes.push_back(wfc::compiler::ClassSource{project.class_names[i],
                                                     project.class_sources[i]});
    }
    info.app_properties = project.app_properties;
    {
        // The modules: `Attribute VB_Name = "Helpers"`.
        std::istringstream lines(source);
        std::string line;
        while (std::getline(lines, line)) {
            const std::string text = lower(line);
            const std::string head = "attribute vb_name = \"";
            if (text.rfind(head, 0) == 0) {
                const auto end = text.find('"', head.size());
                if (end != std::string::npos) {
                    info.module_names.push_back(
                        text.substr(head.size(), end - head.size()));
                }
            }
        }
    }
    info.startup_form = lower(project.startup_form);
    options.vb6_print_spacing = true;
    options.app_properties = project.app_properties;
    options.per_module_option_explicit = project.per_module_option_explicit;
    options.option_explicit_ranges = project.option_explicit_ranges;
    options.resource_file = project.resource_file;
    options.type_libraries = project.type_libraries;
    options.startup_form = project.startup_form;
    options.ui_script =
        ui_script_override.empty() ? project.ui_script : ui_script_override;
    if (const auto forced = environment_text("WFC_FULLSCREEN");
        !forced.empty() && forced != "0") {
        options.ui_mode = "win32";
        fit_screen = true;
    }
    options.fit_screen = fit_screen;
    options.fit_width = project.largest_form_width;
    options.fit_height = project.largest_form_height;
    options.fit_screen_size = environment_text("WFC_SCREEN");
    setup.options = options;
    setup.program = declares_of(project.module_source);
    for (const auto& text : project.class_sources) {
        setup.program += declares_of(text);
    }
    setup.scan_text = project.module_source;
    for (const auto& text : project.class_sources) {
        setup.scan_text += '\n';
        setup.scan_text += text;
    }
    const auto compiled =
        wfc::compiler::compile_project(source, classes, architecture, info);
    if (!compiled.error.empty()) {
        std::cerr << "native compile error";
        if (compiled.error_at.line != 0) {
            std::cerr << " (line " << compiled.error_at.line << ", column "
                      << compiled.error_at.column << ")";
            // The line of the text the module compiler read.
            const std::string* text = &source;
            if (compiled.error.rfind("class ", 0) == 0) {
                const auto name_end = compiled.error.find(':');
                const std::string name =
                    lower(compiled.error.substr(6, name_end - 6));
                for (std::size_t k = 0; k < project.class_names.size(); ++k) {
                    if (name == lower(project.class_names[k])) {
                        text = &project.class_sources[k];
                    }
                }
            }
            std::size_t line_start = 0;
            for (std::size_t n = 1; n < compiled.error_at.line; ++n) {
                const auto next = text->find('\n', line_start);
                if (next == std::string::npos) {
                    break;
                }
                line_start = next + 1U;
            }
            auto line_end = text->find('\n', line_start);
            if (line_end == std::string::npos) {
                line_end = text->size();
            }
            std::cerr << "\n    | "
                      << text->substr(line_start, line_end - line_start);
        }
        std::cerr << ": " << compiled.error << '\n';
        return 3;
    }
    std::string problem;
    const int code =
        wfc::compiler::run_native_image(compiled.image, problem, &setup);
    if (!problem.empty()) {
        std::cerr << problem << '\n';
        return 2;
    }
    return code;
}

}  // namespace

int main(const int argument_count, const char* const arguments[]) {
    // A packaged program carries its project after the image.
    std::string problem;
    if (const auto command =
            wfc::package::unpack(wfc::package::running_image(), problem)) {
        if (!problem.empty()) {
            std::cerr << problem << '\n';
            return 2;
        }
        std::vector<const char*> forwarded{arguments[0]};
        for (const auto& item : *command) {
            forwarded.push_back(item.c_str());
        }
        // Options of the packaged program's own command line go through
        // (`--exit-after 3000`, `--screenshots dir`); a value that follows an
        // option goes with it.
        for (int i = 1; i < argument_count; ++i) {
            if (std::string_view(arguments[i]).rfind("--", 0) == 0) {
                forwarded.push_back(arguments[i]);
                while (i + 1 < argument_count &&
                       std::string_view(arguments[i + 1]).rfind("--", 0) != 0) {
                    forwarded.push_back(arguments[++i]);
                }
            }
        }
        return run(static_cast<int>(forwarded.size()), forwarded.data());
    }
    if (argument_count >= 2 && std::string_view(arguments[1]) == "--build") {
        return build_package(argument_count, arguments);
    }
    if (argument_count >= 3 && std::string_view(arguments[1]) == "--native") {
        return run_native(argument_count, arguments);
    }
    return run(argument_count, arguments);
}

namespace {

int run(const int argument_count, const char* const arguments[]) {
    if (argument_count == 2 && std::string_view(arguments[1]) == "--version") {
        std::cout << "wfc " << wfc::version << '\n';
        return 0;
    }

    // `wfc file.vbp` / `wfc a.bas b.cls ...` loads real VB6 project files.
    if (argument_count >= 2 &&
        std::string_view(arguments[1]).rfind("--", 0) != 0) {
        std::vector<std::filesystem::path> files;
        std::string ui_mode;
        bool fit_screen = false;
        std::string screenshot_directory;
        std::string serve;
        std::string typelib_path;
        std::string registration;  // "register" or "unregister"
        long server_timeout_ms = 600000;
        long exit_after_ms = 0;
        std::string ui_script_override;
        for (int i = 1; i < argument_count; ++i) {
            if (std::string_view(arguments[i]) == "--gui") {
                ui_mode = "win32";  // run forms in real windows
            } else if (std::string_view(arguments[i]) == "--fullscreen") {
                // Real windows, scaled to fill the screen.
                ui_mode = "win32";
                fit_screen = true;
            } else if (std::string_view(arguments[i]) == "-Embedding" ||
                       std::string_view(arguments[i]) == "/Embedding") {
                // COM started this process to serve a class.
                if (serve.empty()) {
                    serve = "server";
                }
            } else if (std::string_view(arguments[i]) == "--write-typelib" &&
                       i + 1 < argument_count) {
                serve = "typelib";
                typelib_path = arguments[++i];
            } else if (std::string_view(arguments[i]) == "--serve") {
                serve = "inproc";  // expose the project's classes in-process
            } else if (std::string_view(arguments[i]) == "--server") {
                serve = "server";  // serve other processes
            } else if (std::string_view(arguments[i]) == "--register") {
                registration = "register";
            } else if (std::string_view(arguments[i]) == "--unregister") {
                registration = "unregister";
            } else if (std::string_view(arguments[i]) == "--server-timeout" &&
                       i + 1 < argument_count) {
                server_timeout_ms = std::atol(arguments[++i]);
            } else if (std::string_view(arguments[i]) == "--exit-after" &&
                       i + 1 < argument_count) {
                exit_after_ms = std::atol(arguments[++i]);
            } else if (std::string_view(arguments[i]) == "--ui-script" &&
                       i + 1 < argument_count) {
                ui_script_override = arguments[++i];
            } else if (std::string_view(arguments[i]) == "--screenshots" &&
                       i + 1 < argument_count) {
                screenshot_directory = arguments[++i];  // PNG of each form
            } else {
                files.emplace_back(arguments[i]);
            }
        }
        // Paths the program or its options name must survive a change of
        // directory.
        const auto absolute_text = [](const std::string& text) {
            return text.empty() ? text
                                : std::filesystem::absolute(text).string();
        };
        ui_script_override = absolute_text(ui_script_override);
        screenshot_directory = absolute_text(screenshot_directory);
        typelib_path = absolute_text(typelib_path);
        if (!files.empty() && files.front().extension() == ".vbp") {
            for (auto& file : files) {
                file = std::filesystem::absolute(file);
            }
        }
        const auto project = wfc::load_project(files);
        if (!project.ok) {
            std::cerr << project.error << '\n';
            return 2;
        }
        std::vector<wfc::ClassModuleSource> project_classes;
        for (std::size_t i = 0; i < project.class_names.size(); ++i) {
            project_classes.push_back(wfc::ClassModuleSource{
                project.class_names[i], project.class_sources[i],
                project.class_form_texts[i],
                project.class_form_directories[i]});
        }
        if (!registration.empty()) {
            const auto served =
                wfc::served_classes(project.project_name, project_classes);
            std::string problem;
            const bool done =
                registration == "register"
                    ? wfc::register_server(
                          served,
                          std::filesystem::absolute(arguments[0]).string(),
                          std::filesystem::absolute(files.front()).string(),
                          problem)
                    : wfc::unregister_server(served, problem);
            if (!done) {
                std::cerr << problem << '\n';
                return 1;
            }
            for (const auto& entry : served) {
                std::cout << entry.progid << ' ' << entry.clsid << '\n';
            }
            return 0;
        }
        // A program starts in its own folder, as one run from the VB6 IDE does,
        // so relative paths such as `data` + a backslash + `phrases.txt` find
        // the program's own files. Every path was made absolute above.
        if (!files.empty() && files.front().extension() == ".vbp" &&
            serve.empty()) {
            std::error_code ignored;
            std::filesystem::current_path(files.front().parent_path(), ignored);
        }
        // Running real project files: print numbers the way VB6 does.
        wfc::EvaluationOptions options;
        options.vb6_print_spacing = true;
        options.app_properties = project.app_properties;
        options.per_module_option_explicit = project.per_module_option_explicit;
        options.option_explicit_ranges = project.option_explicit_ranges;
        options.resource_file = project.resource_file;
        options.type_libraries = project.type_libraries;
        options.startup_form = project.startup_form;
        options.ui_script =
            ui_script_override.empty() ? project.ui_script : ui_script_override;
        options.exit_after_ms = exit_after_ms;
        options.ui_mode = ui_mode;
        if (const auto forced = environment_text("WFC_FULLSCREEN");
            !forced.empty() && forced != "0") {
            options.ui_mode = "win32";
            fit_screen = true;
        }
        options.fit_screen = fit_screen;
        options.fit_width = project.largest_form_width;
        options.fit_height = project.largest_form_height;
        options.fit_screen_size = environment_text("WFC_SCREEN");
        options.screenshot_directory = screenshot_directory;
        if (serve == "typelib") {
            std::cout << "LIBID "
                      << wfc::clsid_for_progid((project.project_name.empty()
                                                    ? std::string{"Project1"}
                                                    : project.project_name) +
                                               ".typelib")
                      << '\n';
        }
        options.serve = serve;
        options.typelib_path = typelib_path;
        options.project_name = project.project_name;
        options.server_timeout_ms = server_timeout_ms;
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

}  // namespace
