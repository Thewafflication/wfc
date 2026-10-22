#include "compile.hpp"

#include <cctype>

#include "lower.hpp"
#include "parser.hpp"

namespace wfc::compiler {

BackEndResult generate_code(const ir::Program& program,
                            const Architecture architecture) {
    if (architecture == Architecture::x64) {
        return generate_x64(program);
    }
    if (architecture == Architecture::x86) {
        return generate_x86(program);
    }
    return generate_arm64(program);
}

bool native_back_end_available() {
    return true;
}

CompileResult compile_source(const std::string_view source,
                             const Architecture architecture) {
    return compile_project(source, {}, architecture);
}

namespace {

[[nodiscard]] std::string lower_case(std::string text) {
    for (auto& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}

}  // namespace

CompileResult compile_project(const std::string_view source,
                              const std::vector<ClassSource>& classes,
                              const Architecture architecture) {
    CompileResult result;
    ParseOptions options;
    for (const auto& item : classes) {
        options.class_names.insert(lower_case(item.name));
    }
    std::vector<Module> class_modules;
    class_modules.reserve(classes.size());
    for (const auto& item : classes) {
        ParseOptions class_options = options;
        class_options.is_class = true;
        class_options.name = lower_case(item.name);
        auto parsed_class = parse_module(item.source, class_options);
        if (!parsed_class.error.empty()) {
            result.error = "class " + item.name + ": " + parsed_class.error;
            result.error_at = parsed_class.error_at;
            return result;
        }
        class_modules.push_back(std::move(parsed_class.module));
    }
    const auto parsed = parse_module(source, options);
    if (!parsed.error.empty()) {
        result.error = parsed.error;
        result.error_at = parsed.error_at;
        return result;
    }
    std::vector<const Module*> class_pointers;
    for (const auto& module : class_modules) {
        class_pointers.push_back(&module);
    }
    auto lowered = lower_module(parsed.module, class_pointers);
    if (!lowered.error.empty()) {
        result.error = lowered.error;
        result.error_at = lowered.error_at;
        return result;
    }
    auto generated = generate_code(lowered.program, architecture);
    if (!generated.error.empty()) {
        result.error = generated.error;
        return result;
    }
    result.image = std::move(generated.image);
    return result;
}

}  // namespace wfc::compiler
