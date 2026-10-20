#include "compile.hpp"

#include "lower.hpp"
#include "parser.hpp"

namespace wfc::compiler {

BackEndResult generate_code(const ir::Program& program,
                            const Architecture architecture) {
    if (architecture == Architecture::x64) {
        return generate_x64(program);
    }
    BackEndResult result;
    result.error = std::string("there is no back end for ") +
                   (architecture == Architecture::x86 ? "x86" : "ARM64") +
                   " yet";
    return result;
}

bool native_back_end_available() {
    return host_architecture() == Architecture::x64;
}

CompileResult compile_source(const std::string_view source,
                             const Architecture architecture) {
    CompileResult result;
    const auto parsed = parse_module(source);
    if (!parsed.error.empty()) {
        result.error = parsed.error;
        result.error_at = parsed.error_at;
        return result;
    }
    auto lowered = lower_module(parsed.module);
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
