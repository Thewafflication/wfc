// Interpreter: calling the routines of `Declare Function|Sub ... Lib "dll"`
// in the real DLL (REQ-0307). Windows only. Internal to the WFC evaluator;
// not part of the public API.

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "interpreter.hpp"

namespace wfc::detail {

namespace {

[[nodiscard]] std::string lower(std::string text) {
    for (auto& c : text) {
        c = ascii_lower(c);
    }
    return text;
}

[[nodiscard]] std::string trimmed(const std::string_view text) {
    std::size_t first = 0;
    std::size_t last = text.size();
    while (first < last && (text[first] == ' ' || text[first] == '\t')) {
        ++first;
    }
    while (last > first && (text[last - 1U] == ' ' || text[last - 1U] == '\t' ||
                            text[last - 1U] == '\r')) {
        --last;
    }
    return std::string(text.substr(first, last - first));
}

[[nodiscard]] ExternalType type_named(const std::string& name,
                                      std::string& udt) {
    const std::string key = lower(name);
    if (key == "long") {
        return ExternalType::Long;
    }
    if (key == "integer") {
        return ExternalType::Integer;
    }
    if (key == "byte") {
        return ExternalType::Byte;
    }
    if (key == "boolean") {
        return ExternalType::Boolean;
    }
    if (key == "single") {
        return ExternalType::Single;
    }
    if (key == "double") {
        return ExternalType::Double;
    }
    if (key == "string") {
        return ExternalType::String;
    }
    if (key == "any") {
        return ExternalType::Any;
    }
    if (key == "variant" || key == "object") {
        return ExternalType::Variant;
    }
    udt = key;
    return ExternalType::Udt;
}

}  // namespace

void Interpreter::scan_declare_signature(ProcedureDef& external,
                                         const std::string& declared_name,
                                         const std::size_t from) {
    const auto line_end = std::min(source_.find('\n', from), source_.size());
    const std::string line(source_.substr(from, line_end - from));
    const std::string key = lower(line);
    external.external_export = declared_name;
    const auto quoted_after = [&](const std::string& word) -> std::string {
        const auto at = key.find(word);
        if (at == std::string::npos) {
            return {};
        }
        const auto open = line.find('"', at);
        const auto close =
            open == std::string::npos ? open : line.find('"', open + 1U);
        return close == std::string::npos
                   ? std::string{}
                   : line.substr(open + 1U, close - open - 1U);
    };
    external.external_library = quoted_after(" lib ");
    if (const auto alias = quoted_after(" alias "); !alias.empty()) {
        external.external_export = alias;
    }
    // The parameter list and the result type.
    const auto open = line.find(
        '(', key.find(" lib ") == std::string::npos ? 0U : key.find(" lib "));
    if (open == std::string::npos) {
        return;
    }
    int depth = 0;
    std::size_t close = std::string::npos;
    for (std::size_t i = open; i < line.size(); ++i) {
        depth += line[i] == '(' ? 1 : line[i] == ')' ? -1 : 0;
        if (depth == 0) {
            close = i;
            break;
        }
    }
    if (close == std::string::npos) {
        return;
    }
    external.external_signature_known = true;
    std::vector<std::string> pieces;
    {
        std::string piece;
        int nested = 0;
        for (std::size_t i = open + 1U; i < close; ++i) {
            const char c = line[i];
            nested += c == '(' ? 1 : c == ')' ? -1 : 0;
            if (c == ',' && nested == 0) {
                pieces.push_back(piece);
                piece.clear();
            } else {
                piece.push_back(c);
            }
        }
        if (!trimmed(piece).empty()) {
            pieces.push_back(piece);
        }
    }
    for (const auto& piece : pieces) {
        ExternalParameter parameter;
        std::vector<std::string> words;
        std::string word;
        for (const char c : piece + " ") {
            if (c == ' ' || c == '\t') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            } else {
                word.push_back(c);
            }
        }
        std::size_t at = 0;
        while (at < words.size()) {
            const std::string w = lower(words[at]);
            if (w == "optional" || w == "byref") {
                ++at;
            } else if (w == "byval") {
                parameter.by_val = true;
                ++at;
            } else {
                break;
            }
        }
        // `name [()] [As Type]`: array parameters pass their data pointer.
        std::string type_name = "Variant";
        for (std::size_t i = at; i + 1U < words.size(); ++i) {
            if (lower(words[i]) == "as") {
                type_name = words[i + 1U];
                break;
            }
        }
        parameter.type = type_named(type_name, parameter.udt);
        external.external_parameters.push_back(std::move(parameter));
    }
    // `As Type` after the closing parenthesis.
    const std::string tail = lower(trimmed(line.substr(close + 1U)));
    if (tail.rfind("as ", 0) == 0) {
        std::string ignored;
        external.external_return = type_named(trimmed(tail.substr(3)), ignored);
        external.external_has_return = true;
    }
}

std::optional<Value> Interpreter::call_declared_dll(
    const ProcedureDef& definition, std::vector<CallArgument>& arguments,
    const std::size_t offset) {
#ifdef _WIN32
    if (!definition.external_signature_known ||
        definition.external_library.empty()) {
        return std::nullopt;
    }
    const auto& parameters = definition.external_parameters;
    if (arguments.size() > parameters.size() || parameters.size() > 12U) {
        return std::nullopt;
    }
    // The first slot of a double or float parameter would not reach the
    // function through integer registers: not supported.
    for (const auto& parameter : parameters) {
        if (parameter.type == ExternalType::Single ||
            parameter.type == ExternalType::Double ||
            parameter.type == ExternalType::Any ||
            parameter.type == ExternalType::Variant) {
            return std::nullopt;
        }
    }
    HMODULE module = LoadLibraryA(definition.external_library.c_str());
    if (module == nullptr) {
        static_cast<void>(raise_runtime(48, "Error in loading DLL", offset));
        return std::nullopt;
    }
    FARPROC function =
        GetProcAddress(module, definition.external_export.c_str());
    if (function == nullptr) {
        function =
            GetProcAddress(module, (definition.external_export + "A").c_str());
    }
    if (function == nullptr) {
        return std::nullopt;
    }

    // Marshal the arguments: ByVal numbers directly, everything else through
    // buffers the call can write to, copied back afterwards.
    struct Buffer {
        std::size_t argument{};
        ExternalType type{};
        std::string bytes;
        std::string udt;
    };
    std::vector<Buffer> buffers;
    buffers.reserve(parameters.size());
    std::array<std::intptr_t, 12> slots{};
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        const auto& parameter = parameters[i];
        Value value = i < arguments.size() && !arguments[i].omitted
                          ? arguments[i].value
                          : Value{Integer{0}};
        if (parameter.type == ExternalType::String) {
            const std::string text = std::holds_alternative<std::string>(value)
                                         ? std::get<std::string>(value)
                                         : std::string{};
            buffers.push_back(
                Buffer{i, parameter.type, text + std::string(1, '\0'), {}});
            slots[i] =
                reinterpret_cast<std::intptr_t>(buffers.back().bytes.data());
            continue;
        }
        if (parameter.type == ExternalType::Udt) {
            const auto* holder = std::get_if<ObjectInstance>(&value);
            const auto definition_found =
                class_definitions_.find(parameter.udt);
            if (holder == nullptr || holder->data == nullptr ||
                definition_found == class_definitions_.end()) {
                return std::nullopt;
            }
            // Fields in declaration order with natural alignment.
            std::string bytes;
            for (const auto& field_name :
                 definition_found->second.field_order) {
                const auto found =
                    holder->data->fields.variables.find(field_name);
                if (found == holder->data->fields.variables.end()) {
                    return std::nullopt;
                }
                std::size_t size = 0;
                std::array<char, 8> raw{};
                const Value& field = found->second;
                if (const auto* v = std::get_if<Integer>(&field)) {
                    size = 4;
                    std::memcpy(raw.data(), v, 4);
                } else if (const auto* v16 = std::get_if<Int16>(&field)) {
                    size = 2;
                    std::memcpy(raw.data(), v16, 2);
                } else if (const auto* v8 = std::get_if<Byte>(&field)) {
                    size = 1;
                    std::memcpy(raw.data(), v8, 1);
                } else if (const auto* vf = std::get_if<float>(&field)) {
                    size = 4;
                    std::memcpy(raw.data(), vf, 4);
                } else if (const auto* vd = std::get_if<double>(&field)) {
                    size = 8;
                    std::memcpy(raw.data(), vd, 8);
                } else {
                    return std::nullopt;
                }
                while (bytes.size() % size != 0U) {
                    bytes.push_back('\0');
                }
                bytes.append(raw.data(), size);
            }
            while (bytes.size() % 4U != 0U) {
                bytes.push_back('\0');
            }
            buffers.push_back(
                Buffer{i, parameter.type, std::move(bytes), parameter.udt});
            slots[i] =
                reinterpret_cast<std::intptr_t>(buffers.back().bytes.data());
            continue;
        }
        const auto number = whole_value(value);
        if (!parameter.by_val) {
            // ByRef Long, Integer, Byte or Boolean: a pointer to a temporary.
            std::string bytes(4, '\0');
            const std::int32_t whole =
                static_cast<std::int32_t>(number.value_or(
                    std::holds_alternative<bool>(value) && std::get<bool>(value)
                        ? -1
                        : 0));
            const std::size_t size = parameter.type == ExternalType::Long ? 4U
                                     : parameter.type == ExternalType::Byte
                                         ? 1U
                                         : 2U;
            std::memcpy(bytes.data(), &whole, size);
            buffers.push_back(Buffer{i, parameter.type, std::move(bytes), {}});
            slots[i] =
                reinterpret_cast<std::intptr_t>(buffers.back().bytes.data());
            continue;
        }
        std::int64_t whole = number.value_or(0);
        if (std::holds_alternative<bool>(value)) {
            whole = std::get<bool>(value) ? -1 : 0;
        }
        slots[i] = static_cast<std::intptr_t>(whole);
    }

    const auto invoke = [&]<std::size_t... I>(std::index_sequence<I...>) {
        using Function = std::intptr_t(__stdcall*)(
            decltype(static_cast<void>(I), std::intptr_t{})...);
        return reinterpret_cast<Function>(function)(slots[I]...);
    };
    std::intptr_t raw = 0;
    switch (parameters.size()) {
#define WFC_DLL_CALL(N)                              \
    case N:                                          \
        raw = invoke(std::make_index_sequence<N>{}); \
        break;
        WFC_DLL_CALL(0)
        WFC_DLL_CALL(1)
        WFC_DLL_CALL(2)
        WFC_DLL_CALL(3)
        WFC_DLL_CALL(4)
        WFC_DLL_CALL(5)
        WFC_DLL_CALL(6)
        WFC_DLL_CALL(7)
        WFC_DLL_CALL(8)
        WFC_DLL_CALL(9)
        WFC_DLL_CALL(10)
        WFC_DLL_CALL(11)
        WFC_DLL_CALL(12)
#undef WFC_DLL_CALL
        default:
            break;
    }

    // Copy the buffers back into the caller's variables.
    for (const auto& buffer : buffers) {
        if (buffer.argument >= arguments.size()) {
            continue;
        }
        Value* target = arguments[buffer.argument].byref_target;
        if (target == nullptr) {
            continue;
        }
        if (buffer.type == ExternalType::String) {
            if (parameters[buffer.argument].by_val) {
                continue;  // a ByVal string is not written back
            }
            // The function may write a shorter, NUL-terminated string.
            std::string text = buffer.bytes;
            text.pop_back();  // the terminator added for the call
            *target = Value{text};
        } else if (buffer.type == ExternalType::Udt) {
            auto* holder = std::get_if<ObjectInstance>(target);
            const auto found = class_definitions_.find(buffer.udt);
            if (holder == nullptr || holder->data == nullptr ||
                found == class_definitions_.end()) {
                continue;
            }
            std::size_t at = 0;
            for (const auto& field_name : found->second.field_order) {
                auto slot = holder->data->fields.variables.find(field_name);
                if (slot == holder->data->fields.variables.end()) {
                    break;
                }
                Value& field = slot->second;
                const auto take = [&](auto zero) {
                    constexpr std::size_t size = sizeof(zero);
                    while (at % size != 0U) {
                        ++at;
                    }
                    std::memcpy(&zero, buffer.bytes.data() + at, size);
                    at += size;
                    return zero;
                };
                if (std::holds_alternative<Integer>(field)) {
                    field = Value{take(Integer{})};
                } else if (std::holds_alternative<Int16>(field)) {
                    field = Value{take(Int16{})};
                } else if (std::holds_alternative<Byte>(field)) {
                    field = Value{take(Byte{})};
                } else if (std::holds_alternative<float>(field)) {
                    field = Value{take(0.0F)};
                } else if (std::holds_alternative<double>(field)) {
                    field = Value{take(0.0)};
                }
            }
        } else if (!parameters[buffer.argument].by_val) {
            std::int32_t whole = 0;
            const std::size_t size = buffer.type == ExternalType::Long   ? 4U
                                     : buffer.type == ExternalType::Byte ? 1U
                                                                         : 2U;
            std::memcpy(&whole, buffer.bytes.data(), size);
            if (buffer.type == ExternalType::Integer) {
                *target = Value{static_cast<Int16>(whole)};
            } else if (buffer.type == ExternalType::Byte) {
                *target = Value{static_cast<Byte>(whole)};
            } else if (buffer.type == ExternalType::Boolean) {
                *target = Value{static_cast<Int16>(whole) != 0};
            } else {
                *target = Value{static_cast<Integer>(whole)};
            }
        }
    }

    if (!definition.is_function) {
        return Value{Empty{}};
    }
    switch (definition.external_return) {
        case ExternalType::Integer:
            return Value{static_cast<Int16>(raw)};
        case ExternalType::Byte:
            return Value{static_cast<Byte>(raw)};
        case ExternalType::Boolean:
            return Value{static_cast<std::int16_t>(raw) != 0};
        default:
            return Value{static_cast<Integer>(raw)};
    }
#else
    static_cast<void>(definition);
    static_cast<void>(arguments);
    static_cast<void>(offset);
    return std::nullopt;
#endif
}

}  // namespace wfc::detail
