// Interpreter: File I/O statements and functions (Open, Print #, Get/Put,
// Input, EOF, ...). Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#include "interpreter.hpp"

namespace wfc::detail {

std::FILE* Interpreter::open_file(const std::string& path, const char* mode) {
#ifdef _WIN32
    std::FILE* handle = nullptr;
    return fopen_s(&handle, path.c_str(), mode) == 0 ? handle : nullptr;
#else
    return std::fopen(path.c_str(), mode);
#endif
}

std::string Interpreter::environment_variable(const std::string& name) {
#ifdef _WIN32
    char* buffer = nullptr;
    std::size_t length = 0;
    std::string result;
    if (_dupenv_s(&buffer, &length, name.c_str()) == 0 && buffer != nullptr) {
        result = buffer;
        std::free(buffer);
    }
    return result;
#else
    const char* found = std::getenv(name.c_str());
    return found != nullptr ? found : "";
#endif
}

auto Interpreter::find_open_file(const Integer number, const std::size_t offset)
    -> OpenFile* {
    const auto found = files_.find(number);
    if (found == files_.end()) {
        static_cast<void>(raise_runtime(52, "Bad file name or number", offset));
        return nullptr;
    }
    return &found->second;
}

bool Interpreter::write_to_file(const Integer number, const std::string& text,
                                const std::size_t offset) {
    auto* const file = find_open_file(number, offset);
    if (file == nullptr) {
        return false;
    }
    if (file->mode == 1) {
        return raise_runtime(54, "Bad file mode", offset);
    }
    const std::string bytes = text_to_ansi_bytes(text);
    if (!bytes.empty() && std::fwrite(bytes.data(), 1, bytes.size(),
                                      file->handle) != bytes.size()) {
        return raise_runtime(57, "Device I/O error", offset);
    }
    return true;
}

bool Interpreter::read_file_line(std::FILE* handle, std::string& line) {
    line.clear();
    int c = std::fgetc(handle);
    if (c == EOF) {
        return false;
    }
    while (c != EOF && c != '\n') {
        if (c == '\r') {
            const int next = std::fgetc(handle);
            if (next != '\n' && next != EOF) {
                std::ungetc(next, handle);
            }
            break;
        }
        line.push_back(static_cast<char>(c));
        c = std::fgetc(handle);
    }
    return true;
}

bool Interpreter::file_at_eof(std::FILE* handle) {
    const int c = std::fgetc(handle);
    if (c == EOF) {
        return true;
    }
    std::ungetc(c, handle);
    return false;
}

bool Interpreter::read_input_field(std::FILE* handle, std::string& token,
                                   bool& quoted) {
    token.clear();
    quoted = false;
    int c = std::fgetc(handle);
    while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
        c = std::fgetc(handle);
    }
    if (c == EOF) {
        return false;
    }
    if (c == '"') {
        quoted = true;
        c = std::fgetc(handle);
        while (c != EOF && c != '"') {
            token.push_back(static_cast<char>(c));
            c = std::fgetc(handle);
        }
        c = std::fgetc(handle);
        while (c == ' ' || c == '\t') {
            c = std::fgetc(handle);
        }
        if (c != ',' && c != '\n' && c != '\r' && c != EOF) {
            std::ungetc(c, handle);
        }
        return true;
    }
    while (c != EOF && c != ',' && c != '\n' && c != '\r') {
        token.push_back(static_cast<char>(c));
        c = std::fgetc(handle);
    }
    if (c == '\r') {
        const int next = std::fgetc(handle);
        if (next != '\n' && next != EOF) {
            std::ungetc(next, handle);
        }
    }
    while (!token.empty() && (token.back() == ' ' || token.back() == '\t')) {
        token.pop_back();
    }
    return true;
}

bool Interpreter::store_input_token(Value& target, const bool is_variant,
                                    const std::string& token, const bool quoted,
                                    const std::size_t offset) {
    if (is_variant) {
        if (!quoted && token.size() >= 2 && token.front() == '#' &&
            token.back() == '#') {
            std::string inner = token.substr(1, token.size() - 2);
            std::string lowered;
            for (const char ch : inner) {
                lowered.push_back(ascii_lower(ch));
            }
            if (lowered == "true" || lowered == "false") {
                target = lowered == "true";
                return true;
            }
            if (lowered == "null") {
                target = Null{};
                return true;
            }
            if (const auto parsed = parse_date_text(inner)) {
                target = DateValue{*parsed};
                return true;
            }
        }
        if (!quoted) {
            const auto number = parse_numeric_string(token);
            if (number.status == NumericStringStatus::valid) {
                const bool whole = std::floor(number.value) == number.value;
                if (whole && number.value >= -32768.0 &&
                    number.value <= 32767.0) {
                    target = Value{static_cast<Int16>(number.value)};
                } else if (whole && std::fabs(number.value) < 2147483648.0) {
                    target = Value{static_cast<Integer>(number.value)};
                } else {
                    target = Value{number.value};
                }
                return true;
            }
        }
        target = token;
        return true;
    }
    if (std::holds_alternative<std::string>(target)) {
        target = token;
        return true;
    }
    if (std::holds_alternative<bool>(target)) {
        std::string lowered;
        for (const char ch : token) {
            lowered.push_back(ascii_lower(ch));
        }
        target = lowered == "#true#" || lowered == "true";
        return true;
    }
    if (std::holds_alternative<DateValue>(target)) {
        std::string inner = token;
        if (inner.size() >= 2 && inner.front() == '#' && inner.back() == '#') {
            inner = inner.substr(1, inner.size() - 2);
        }
        const auto parsed = parse_date_text(inner);
        target = DateValue{parsed.value_or(0.0)};
        return true;
    }
    const auto number = parse_numeric_string(token.empty() ? "0" : token);
    if (number.status != NumericStringStatus::valid) {
        return raise_runtime(13, "Type mismatch", offset);
    }
    Value converted{number.value};
    if (std::holds_alternative<Integer>(target)) {
        const double rounded = std::nearbyint(number.value);
        if (!(rounded >= -2147483648.0 && rounded <= 2147483647.0)) {
            return raise_runtime(6, "Overflow", offset);
        }
        converted = static_cast<Integer>(rounded);
    } else if (!coerce_numeric_value(converted, target.index(), offset)) {
        return false;
    }
    if (converted.index() != target.index()) {
        return raise_runtime(13, "Type mismatch", offset);
    }
    target = std::move(converted);
    return true;
}

std::string Interpreter::write_item_text(const Value& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        return "\"" + *text + "\"";
    }
    if (const auto* flag = std::get_if<bool>(&value)) {
        return *flag ? "#TRUE#" : "#FALSE#";
    }
    if (std::holds_alternative<Null>(value)) {
        return "#NULL#";
    }
    if (std::holds_alternative<Empty>(value)) {
        return "";
    }
    if (const auto* date = std::get_if<DateValue>(&value)) {
        const auto parts = split_date(date->serial);
        char buffer[64];
        const bool has_time =
            parts.hour != 0 || parts.minute != 0 || parts.second != 0;
        const bool has_date = std::floor(date->serial) != 0.0 || !has_time;
        std::string text = "#";
        if (has_date) {
            std::snprintf(buffer, sizeof(buffer), "%04lld-%02lld-%02lld",
                          static_cast<long long>(parts.year),
                          static_cast<long long>(parts.month),
                          static_cast<long long>(parts.day));
            text += buffer;
        }
        if (has_time) {
            std::snprintf(buffer, sizeof(buffer), "%02lld:%02lld:%02lld",
                          static_cast<long long>(parts.hour),
                          static_cast<long long>(parts.minute),
                          static_cast<long long>(parts.second));
            text += (has_date ? " " : "") + std::string(buffer);
        }
        return text + "#";
    }
    return render(value);
}

bool Interpreter::parse_hash_file_number(Integer& number) {
    skip_horizontal_whitespace();
    static_cast<void>(consume('#'));
    skip_horizontal_whitespace();
    const auto offset = offset_;
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    if (!coerce_numeric_value(*value, Value{Integer{}}.index(), offset) ||
        !std::holds_alternative<Integer>(*value)) {
        set_error("WFC0073", "file number must be a Long", offset);
        return false;
    }
    number = std::get<Integer>(*value);
    return true;
}

void Interpreter::seek_record(OpenFile& file, const long position) {
    const long unit = file.mode == 5 ? file.record_length : 1;
    std::fseek(file.handle, (position - 1) * unit, SEEK_SET);
}

std::optional<bool> Interpreter::parse_binary_statement(
    const std::size_t statement_offset) {
    const auto start = offset_;
    const bool is_get = consume_keyword("get");
    const bool is_put = !is_get && consume_keyword("put");
    const bool is_seek = !is_get && !is_put && consume_keyword("seek");
    if (!is_get && !is_put && !is_seek) {
        return std::nullopt;
    }
    skip_horizontal_whitespace();
    if (!at_end() &&
        (current() == '=' || current() == '(' || current() == '.')) {
        offset_ = start;  // a variable named Get/Put/Seek
        return std::nullopt;
    }
    Integer number{};
    if (!parse_hash_file_number(number)) {
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume(',')) {
        set_error("WFC0014", "expected comma after file number", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    std::optional<long> position;
    if (is_seek || (!at_end() && current() != ',')) {
        const auto position_offset = offset_;
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (!coerce_numeric_value(*value, Value{Integer{}}.index(),
                                  position_offset) ||
            !std::holds_alternative<Integer>(*value)) {
            set_error("WFC0073", "file position must be a Long",
                      position_offset);
            return false;
        }
        position = static_cast<long>(std::get<Integer>(*value));
    }
    if (is_seek) {
        if (!execute_) {
            return true;
        }
        auto* const file = find_open_file(number, statement_offset);
        if (file == nullptr) {
            return false;
        }
        if (*position < 1) {
            return raise_runtime(63, "Bad record number", statement_offset);
        }
        seek_record(*file, *position);
        return true;
    }
    skip_horizontal_whitespace();
    if (!consume(',')) {
        set_error("WFC0014", "expected comma before variable", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    LValue lvalue;
    if (!parse_lvalue_path(lvalue)) {
        return false;
    }
    if (!execute_) {
        return true;
    }
    auto* const file = find_open_file(number, statement_offset);
    if (file == nullptr) {
        return false;
    }
    if (file->mode < 4) {
        return raise_runtime(54, "Bad file mode", statement_offset);
    }
    if (position.has_value()) {
        if (*position < 1) {
            return raise_runtime(63, "Bad record number", statement_offset);
        }
        seek_record(*file, *position);
    } else {
        std::fseek(file->handle, std::ftell(file->handle), SEEK_SET);
    }
    const long record_start = std::ftell(file->handle);
    const auto transfer = [&](void* data, const std::size_t size) {
        return is_get ? std::fread(data, 1, size, file->handle) == size
                      : std::fwrite(data, 1, size, file->handle) == size;
    };
    const std::function<bool(Value&, std::size_t, bool)> transfer_value =
        [&](Value& target, const std::size_t fixed, const bool nested) -> bool {
        bool ok = true;
        if (auto* v1 = std::get_if<Integer>(&target)) {
            std::int32_t x = *v1;
            ok = transfer(&x, 4);
            if (is_get) {
                *v1 = x;
            }
        } else if (auto* v2 = std::get_if<Int16>(&target)) {
            std::int16_t x = *v2;
            ok = transfer(&x, 2);
            if (is_get) {
                *v2 = x;
            }
        } else if (auto* v3 = std::get_if<Byte>(&target)) {
            std::uint8_t x = *v3;
            ok = transfer(&x, 1);
            if (is_get) {
                *v3 = x;
            }
        } else if (auto* v4 = std::get_if<float>(&target)) {
            float x = *v4;
            ok = transfer(&x, 4);
            if (is_get) {
                *v4 = x;
            }
        } else if (auto* v5 = std::get_if<double>(&target)) {
            double x = *v5;
            ok = transfer(&x, 8);
            if (is_get) {
                *v5 = x;
            }
        } else if (auto* v6 = std::get_if<Currency>(&target)) {
            std::int64_t x = v6->scaled;
            ok = transfer(&x, 8);
            if (is_get) {
                v6->scaled = x;
            }
        } else if (auto* v7 = std::get_if<DateValue>(&target)) {
            double x = v7->serial;
            ok = transfer(&x, 8);
            if (is_get) {
                v7->serial = x;
            }
        } else if (auto* v8 = std::get_if<bool>(&target)) {
            std::int16_t x = *v8 ? -1 : 0;
            ok = transfer(&x, 2);
            if (is_get) {
                *v8 = x != 0;
            }
        } else if (auto* v9 = std::get_if<std::string>(&target)) {
            // On disk a string is ANSI bytes; in memory it is Unicode.
            std::string bytes;
            if (fixed != 0U) {
                if (!is_get) {
                    fit_to_units(*v9, fixed);
                    bytes = text_to_ansi_bytes(*v9);
                } else {
                    bytes.assign(fixed, '\0');
                }
                ok = transfer(bytes.data(), fixed);
            } else {
                bytes = is_get ? std::string(utf16_length(*v9), '\0')
                               : text_to_ansi_bytes(*v9);
                if (file->mode == 5 || nested) {
                    std::uint16_t length =
                        static_cast<std::uint16_t>(bytes.size());
                    ok = transfer(&length, 2);
                    if (is_get && ok) {
                        bytes.assign(length, '\0');
                    }
                }
                if (ok && !bytes.empty()) {
                    ok = transfer(bytes.data(), bytes.size());
                }
            }
            if (is_get && ok) {
                *v9 = ansi_bytes_to_text(bytes);
            }
        } else if (auto* array = std::get_if<ArrayValue>(&target)) {
            for (auto& element : array->elements) {
                if (!transfer_value(element, 0, true)) {
                    return false;
                }
            }
        } else if (auto* instance = std::get_if<ObjectInstance>(&target)) {
            const auto class_iterator =
                class_definitions_.find(instance->data->class_name);
            if (class_iterator == class_definitions_.end() ||
                !class_iterator->second.is_udt) {
                return raise_runtime(5, "Invalid procedure call or argument",
                                     statement_offset);
            }
            for (const auto& field_name : class_iterator->second.field_order) {
                const auto field =
                    instance->data->fields.variables.find(field_name);
                if (field == instance->data->fields.variables.end()) {
                    continue;
                }
                const auto fixed_length =
                    instance->data->fields.fixed_string_lengths.find(
                        field_name);
                if (!transfer_value(
                        field->second,
                        fixed_length != instance->data->fields
                                            .fixed_string_lengths.end()
                            ? fixed_length->second
                            : 0U,
                        true)) {
                    return false;
                }
            }
        } else {
            return raise_runtime(5, "Invalid procedure call or argument",
                                 statement_offset);
        }
        if (!ok) {
            if (is_get) {
                return raise_runtime(62, "Input past end of file",
                                     statement_offset);
            }
            return raise_runtime(57, "Device I/O error", statement_offset);
        }
        return true;
    };
    if (!transfer_value(*lvalue.ptr, lvalue.fixed, false)) {
        return false;
    }
    if (file->mode == 5) {
        const long end = record_start + file->record_length;
        if (!is_get && std::ftell(file->handle) < end) {
            std::fseek(file->handle, 0, SEEK_END);
            if (std::ftell(file->handle) < end) {
                const std::string padding(
                    static_cast<std::size_t>(end - std::ftell(file->handle)),
                    '\0');
                std::fwrite(padding.data(), 1, padding.size(), file->handle);
            }
        }
        std::fseek(file->handle, end, SEEK_SET);
    }
    return true;
}

std::optional<bool> Interpreter::parse_file_statement(
    const std::size_t statement_offset) {
    const auto start = offset_;
    if (consume_keyword("open")) {
        skip_horizontal_whitespace();
        auto path = parse_expression();
        if (!path.has_value()) {
            return false;
        }
        skip_horizontal_whitespace();
        int mode = 0;
        if (consume_keyword("for")) {
            skip_horizontal_whitespace();
            if (consume_keyword("input")) {
                mode = 1;
            } else if (consume_keyword("output")) {
                mode = 2;
            } else if (consume_keyword("append")) {
                mode = 3;
            } else if (consume_keyword("binary")) {
                mode = 4;
            } else if (consume_keyword("random")) {
                mode = 5;
            } else {
                set_error("WFC0321", "unsupported Open mode", offset_);
                return false;
            }
        } else {
            set_error("WFC0321", "expected For after Open path", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        if (consume_keyword("access")) {
            skip_horizontal_whitespace();
            static_cast<void>(consume_keyword("read"));
            skip_horizontal_whitespace();
            static_cast<void>(consume_keyword("write"));
            skip_horizontal_whitespace();
        }
        if (consume_keyword("shared") || consume_keyword("lock")) {
            skip_horizontal_whitespace();
            static_cast<void>(consume_keyword("read") ||
                              consume_keyword("write"));
            skip_horizontal_whitespace();
            static_cast<void>(consume_keyword("write"));
            skip_horizontal_whitespace();
        }
        if (!consume_keyword("as")) {
            set_error("WFC0147", "expected As in Open statement", offset_);
            return false;
        }
        Integer number{};
        if (!parse_hash_file_number(number)) {
            return false;
        }
        skip_horizontal_whitespace();
        long record_length = 128;
        if (consume_keyword("len")) {
            skip_horizontal_whitespace();
            static_cast<void>(consume('='));
            auto length = parse_expression();
            if (!length.has_value()) {
                return false;
            }
            if (const auto size = whole_value(*length)) {
                record_length = *size;
            }
        }
        if (!execute_) {
            return true;
        }
        const auto* path_text = std::get_if<std::string>(&*path);
        if (path_text == nullptr) {
            set_error("WFC0073", "Open requires a String path",
                      statement_offset);
            return false;
        }
        if (number < 1 || number > 511) {
            return raise_runtime(52, "Bad file name or number",
                                 statement_offset);
        }
        if (files_.contains(number)) {
            return raise_runtime(55, "File already open", statement_offset);
        }
        std::FILE* handle = nullptr;
        if (mode >= 4) {
            handle = open_file(*path_text, "r+b");
            if (handle == nullptr) {
                handle = open_file(*path_text, "w+b");
            }
        } else {
            handle = open_file(*path_text, mode == 1   ? "rb"
                                           : mode == 2 ? "wb"
                                                       : "ab");
        }
        if (handle == nullptr) {
            std::error_code path_error;
            const auto parent = std::filesystem::path(*path_text).parent_path();
            if (!parent.empty() &&
                !std::filesystem::is_directory(parent, path_error)) {
                return raise_runtime(76, "Path not found", statement_offset);
            }
            if (mode == 1 &&
                !std::filesystem::exists(std::filesystem::path(*path_text),
                                         path_error)) {
                return raise_runtime(53, "File not found", statement_offset);
            }
            return raise_runtime(70, "Permission denied", statement_offset);
        }
        files_[number] =
            OpenFile{handle, mode, record_length > 0 ? record_length : 128};
        return true;
    }
    if (consume_keyword("close")) {
        skip_horizontal_whitespace();
        std::vector<Integer> numbers;
        while (!at_statement_end()) {
            Integer number{};
            if (!parse_hash_file_number(number)) {
                return false;
            }
            numbers.push_back(number);
            skip_horizontal_whitespace();
            if (!consume(',')) {
                break;
            }
        }
        if (!execute_) {
            return true;
        }
        if (numbers.empty()) {
            for (auto& [n, file] : files_) {
                std::fclose(file.handle);
            }
            files_.clear();
            return true;
        }
        for (const auto number : numbers) {
            const auto found = files_.find(number);
            if (found != files_.end()) {
                std::fclose(found->second.handle);
                files_.erase(found);
            }
        }
        return true;
    }
    if (consume_keyword("write")) {
        skip_horizontal_whitespace();
        if (at_end() || current() != '#') {
            offset_ = start;
            return std::nullopt;
        }
        Integer number{};
        if (!parse_hash_file_number(number)) {
            return false;
        }
        skip_horizontal_whitespace();
        std::string line;
        if (consume(',')) {
            bool first = true;
            while (true) {
                skip_horizontal_whitespace();
                if (at_statement_end()) {
                    break;
                }
                auto value = parse_expression();
                if (!value.has_value()) {
                    return false;
                }
                if (execute_) {
                    if (!first) {
                        line += ",";
                    }
                    line += write_item_text(*value);
                }
                first = false;
                skip_horizontal_whitespace();
                if (!consume(',') && !consume(';')) {
                    break;
                }
            }
        }
        return execute_ ? write_to_file(number, line + "\r\n", statement_offset)
                        : true;
    }
    if (consume_keyword("line")) {
        skip_horizontal_whitespace();
        if (!consume_keyword("input")) {
            offset_ = start;
            return std::nullopt;
        }
        Integer number{};
        if (!parse_hash_file_number(number)) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume(',')) {
            set_error("WFC0014", "expected comma after file number", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        const auto variable_offset = offset_;
        char type_character{};
        auto name = parse_identifier(&type_character);
        if (!name.has_value()) {
            set_error("WFC0011", "expected variable name", variable_offset);
            return false;
        }
        const auto variable = find_variable(*name);
        if (variable.value == nullptr) {
            set_error("WFC0015", "undeclared variable", variable_offset);
            return false;
        }
        if (!execute_) {
            return true;
        }
        auto* const file = find_open_file(number, statement_offset);
        if (file == nullptr) {
            return false;
        }
        if (file->mode != 1) {
            return raise_runtime(54, "Bad file mode", statement_offset);
        }
        std::string line;
        if (!read_file_line(file->handle, line)) {
            return raise_runtime(62, "Input past end of file",
                                 statement_offset);
        }
        line = ansi_bytes_to_text(line);
        if (!std::holds_alternative<std::string>(*variable.value) &&
            !variable.scope->variant_variables.contains(*name)) {
            set_error("WFC0016",
                      "Line Input requires a String or Variant variable",
                      variable_offset);
            return false;
        }
        *variable.value = std::move(line);
        return true;
    }
    if (consume_keyword("input")) {
        skip_horizontal_whitespace();
        if (at_end() || current() != '#') {
            offset_ = start;
            return std::nullopt;
        }
        Integer number{};
        if (!parse_hash_file_number(number)) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume(',')) {
            set_error("WFC0014", "expected comma after file number", offset_);
            return false;
        }
        while (true) {
            skip_horizontal_whitespace();
            const auto variable_offset = offset_;
            char type_character{};
            auto name = parse_identifier(&type_character);
            if (!name.has_value()) {
                set_error("WFC0011", "expected variable name", variable_offset);
                return false;
            }
            const auto variable = find_variable(*name);
            if (variable.value == nullptr) {
                set_error("WFC0015", "undeclared variable", variable_offset);
                return false;
            }
            if (execute_) {
                auto* const file = find_open_file(number, statement_offset);
                if (file == nullptr) {
                    return false;
                }
                if (file->mode != 1) {
                    return raise_runtime(54, "Bad file mode", statement_offset);
                }
                std::string token;
                bool quoted{};
                if (!read_input_field(file->handle, token, quoted)) {
                    return raise_runtime(62, "Input past end of file",
                                         statement_offset);
                }
                token = ansi_bytes_to_text(token);
                if (!store_input_token(
                        *variable.value,
                        variable.scope->variant_variables.contains(*name),
                        token, quoted, variable_offset)) {
                    return false;
                }
            }
            skip_horizontal_whitespace();
            if (!consume(',')) {
                return true;
            }
        }
    }
    if (consume_keyword("kill") || consume_keyword("mkdir") ||
        consume_keyword("rmdir")) {
        const std::string_view word = source_.substr(start, offset_ - start);
        char first = ascii_lower(word.front());
        skip_horizontal_whitespace();
        auto path = parse_expression();
        if (!path.has_value()) {
            return false;
        }
        if (!execute_) {
            return true;
        }
        const auto* text = std::get_if<std::string>(&*path);
        if (text == nullptr) {
            set_error("WFC0073", "path must be a String", statement_offset);
            return false;
        }
        std::error_code ec;
        const std::filesystem::path target(*text);
        if (first == 'k' && target.filename().string().find_first_of("*?") !=
                                std::string::npos) {
            const auto directory = target.has_parent_path()
                                       ? target.parent_path()
                                       : std::filesystem::path(".");
            const std::string mask = target.filename().string();
            std::vector<std::filesystem::path> matches;
            for (const auto& entry :
                 std::filesystem::directory_iterator(directory, ec)) {
                if (entry.is_regular_file(ec) &&
                    like_match(entry.path().filename().string(), mask, true)) {
                    matches.push_back(entry.path());
                }
            }
            if (matches.empty()) {
                return raise_runtime(53, "File not found", statement_offset);
            }
            for (const auto& match : matches) {
                std::filesystem::remove(match, ec);
            }
        } else if (first == 'k') {
            if (!std::filesystem::is_regular_file(target, ec)) {
                return raise_runtime(53, "File not found", statement_offset);
            }
            std::filesystem::remove(target, ec);
        } else if (first == 'm') {
            if (!std::filesystem::create_directory(target, ec) || ec) {
                return raise_runtime(75, "Path/File access error",
                                     statement_offset);
            }
        } else {
            if (!std::filesystem::is_directory(target, ec)) {
                return raise_runtime(76, "Path not found", statement_offset);
            }
            std::filesystem::remove(target, ec);
            if (ec) {
                return raise_runtime(75, "Path/File access error",
                                     statement_offset);
            }
        }
        return true;
    }
    if (consume_keyword("name") || consume_keyword("chdir")) {
        const bool is_name = ascii_lower(source_[start + 0]) == 'n';
        skip_horizontal_whitespace();
        if (is_name &&
            (at_statement_end() || current() == '=' || current() == '(' ||
             current() == '.' || current() == ',')) {
            offset_ = start;  // an ordinary variable called Name
            return std::nullopt;
        }
        auto first = parse_expression();
        if (!first.has_value()) {
            return false;
        }
        std::optional<Value> second;
        if (is_name) {
            skip_horizontal_whitespace();
            if (!consume_keyword("as")) {
                set_error("WFC0147", "expected As in Name statement", offset_);
                return false;
            }
            skip_horizontal_whitespace();
            second = parse_expression();
            if (!second.has_value()) {
                return false;
            }
        }
        if (!execute_) {
            return true;
        }
        const auto* from_text = std::get_if<std::string>(&*first);
        const auto* to_text =
            second ? std::get_if<std::string>(&*second) : nullptr;
        if (from_text == nullptr || (is_name && to_text == nullptr)) {
            set_error("WFC0073", "path must be a String", statement_offset);
            return false;
        }
        std::error_code ec;
        if (is_name) {
            if (!std::filesystem::exists(*from_text, ec)) {
                return raise_runtime(53, "File not found", statement_offset);
            }
            if (std::filesystem::exists(*to_text, ec)) {
                return raise_runtime(58, "File already exists",
                                     statement_offset);
            }
            std::filesystem::rename(*from_text, *to_text, ec);
            if (ec) {
                return raise_runtime(75, "Path/File access error",
                                     statement_offset);
            }
        } else {
            std::filesystem::current_path(*from_text, ec);
            if (ec) {
                return raise_runtime(76, "Path not found", statement_offset);
            }
        }
        return true;
    }
    if (consume_keyword("filecopy")) {
        skip_horizontal_whitespace();
        auto from = parse_expression();
        if (!from.has_value()) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume(',')) {
            set_error("WFC0014", "expected comma in FileCopy", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        auto to = parse_expression();
        if (!to.has_value()) {
            return false;
        }
        if (!execute_) {
            return true;
        }
        const auto* source_text = std::get_if<std::string>(&*from);
        const auto* target_text = std::get_if<std::string>(&*to);
        if (source_text == nullptr || target_text == nullptr) {
            set_error("WFC0073", "FileCopy requires String paths",
                      statement_offset);
            return false;
        }
        std::error_code ec;
        std::filesystem::copy_file(
            *source_text, *target_text,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            return raise_runtime(53, "File not found", statement_offset);
        }
        return true;
    }
    offset_ = start;
    return std::nullopt;
}

bool Interpreter::is_file_function_name(const std::string_view name) {
    return name == "eof" || name == "lof" || name == "freefile" ||
           name == "dir" || name == "dir$" || name == "curdir" ||
           name == "curdir$" || name == "filelen" || name == "input" ||
           name == "input$" || name == "inputb" || name == "inputb$" ||
           name == "environ" || name == "environ$" || name == "loc" ||
           name == "seek";
}

std::optional<Value> Interpreter::evaluate_file_function(
    const std::string_view name, std::vector<Value>& arguments,
    const std::size_t offset) {
    const auto count = arguments.size();
    const auto arity = [&](const std::size_t low, const std::size_t high) {
        if (count < low || count > high) {
            set_error("WFC0072",
                      "function received the wrong number of arguments",
                      offset);
            return false;
        }
        return true;
    };
    const auto long_at =
        [&](const std::size_t index) -> std::optional<Integer> {
        if (const auto* i = std::get_if<Integer>(&arguments[index])) {
            return *i;
        }
        if (const auto* i = std::get_if<Int16>(&arguments[index])) {
            return static_cast<Integer>(*i);
        }
        return std::nullopt;
    };
    if (name == "freefile") {
        if (!arity(0, 1)) {
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        for (Integer n = 1; n <= 255; ++n) {
            if (!files_.contains(n)) {
                return Value{n};
            }
        }
        return Value{Integer{}};
    }
    if (name == "seek") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto number = long_at(0);
        if (!number) {
            set_error("WFC0073", "file number must be a Long", offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        auto* const file = find_open_file(*number, offset);
        if (file == nullptr) {
            return std::nullopt;
        }
        const long position = std::ftell(file->handle);
        const long unit = file->mode == 5 ? file->record_length : 1;
        return Value{static_cast<Integer>(position / unit + 1)};
    }
    if (name == "eof" || name == "lof" || name == "loc") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto number = long_at(0);
        if (!number) {
            set_error("WFC0073", "file number must be a Long", offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{name == "eof" ? Value{false} : Value{Integer{}}};
        }
        auto* const file = find_open_file(*number, offset);
        if (file == nullptr) {
            return std::nullopt;
        }
        if (name == "eof") {
            return Value{file->mode == 1 || file->mode >= 4
                             ? file_at_eof(file->handle)
                             : true};
        }
        std::fflush(file->handle);
        const long position = std::ftell(file->handle);
        if (name == "loc") {
            return Value{static_cast<Integer>(position < 0 ? 0 : position)};
        }
        std::fseek(file->handle, 0, SEEK_END);
        const long size = std::ftell(file->handle);
        std::fseek(file->handle, position, SEEK_SET);
        return Value{static_cast<Integer>(size < 0 ? 0 : size)};
    }
    if (name == "curdir" || name == "curdir$") {
        if (!arity(0, 1)) {
            return std::nullopt;
        }
        std::error_code ec;
        return Value{execute_ ? std::filesystem::current_path(ec).string()
                              : std::string{}};
    }
    if (name == "filelen") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        const auto* path = std::get_if<std::string>(&arguments[0]);
        if (path == nullptr) {
            set_error("WFC0073", "FileLen requires a String path", offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{Integer{}};
        }
        std::error_code ec;
        const auto size = std::filesystem::file_size(*path, ec);
        if (ec) {
            static_cast<void>(raise_runtime(53, "File not found", offset));
            return std::nullopt;
        }
        return Value{static_cast<Integer>(size)};
    }
    if (name == "environ" || name == "environ$") {
        if (!arity(1, 1)) {
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        if (const auto* variable = std::get_if<std::string>(&arguments[0])) {
            return Value{environment_variable(*variable)};
        }
        set_error("WFC0073", "Environ requires a String name", offset);
        return std::nullopt;
    }
    if (name == "dir" || name == "dir$") {
        if (!arity(0, 2)) {
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        if (count >= 1U) {
            const auto* pattern = std::get_if<std::string>(&arguments[0]);
            if (pattern == nullptr) {
                set_error("WFC0073", "Dir requires a String pattern", offset);
                return std::nullopt;
            }
            dir_matches_.clear();
            dir_index_ = 0;
            std::error_code ec;
            std::filesystem::path full(*pattern);
            const auto directory = full.has_parent_path()
                                       ? full.parent_path()
                                       : std::filesystem::path(".");
            const std::string mask = full.filename().string();
            const bool want_directories =
                count >= 2U && long_at(1).value_or(0) & 16;
            if (mask.find_first_of("*?") == std::string::npos) {
                if (std::filesystem::exists(full, ec) &&
                    (want_directories ||
                     !std::filesystem::is_directory(full, ec))) {
                    dir_matches_.push_back(full.filename().string());
                }
            } else {
                for (const auto& entry :
                     std::filesystem::directory_iterator(directory, ec)) {
                    const std::string entry_name =
                        entry.path().filename().string();
                    if (!want_directories && entry.is_directory(ec)) {
                        continue;
                    }
                    if (like_match(entry_name, mask, true)) {
                        dir_matches_.push_back(entry_name);
                    }
                }
                std::sort(dir_matches_.begin(), dir_matches_.end());
                if (want_directories) {
                    for (const char* dots : {"..", "."}) {
                        if (like_match(dots, mask, true)) {
                            dir_matches_.insert(dir_matches_.begin(), dots);
                        }
                    }
                }
            }
        }
        if (dir_index_ < dir_matches_.size()) {
            return Value{dir_matches_[dir_index_++]};
        }
        return Value{std::string{}};
    }
    if (name == "input" || name == "input$" || name == "inputb" ||
        name == "inputb$") {
        if (!arity(2, 2)) {
            return std::nullopt;
        }
        const auto length = long_at(0);
        const auto number = long_at(1);
        if (!length || !number) {
            set_error("WFC0073", "Input requires Long arguments", offset);
            return std::nullopt;
        }
        if (!execute_) {
            return Value{std::string{}};
        }
        auto* const file = find_open_file(*number, offset);
        if (file == nullptr) {
            return std::nullopt;
        }
        if (file->mode != 1) {
            static_cast<void>(raise_runtime(54, "Bad file mode", offset));
            return std::nullopt;
        }
        std::string text;
        for (Integer i = 0; i < *length; ++i) {
            const int c = std::fgetc(file->handle);
            if (c == EOF) {
                static_cast<void>(
                    raise_runtime(62, "Input past end of file", offset));
                return std::nullopt;
            }
            text.push_back(static_cast<char>(c));
        }
        return Value{ansi_bytes_to_text(text)};
    }
    set_error("WFC0071", "unsupported function", offset);
    return std::nullopt;
}

bool Interpreter::parse_file_number_prefix(Integer& file_number, bool& found) {
    found = false;
    file_number = -1;
    skip_horizontal_whitespace();
    if (at_end() || current() != '#') {
        return true;
    }
    std::size_t look = offset_ + 1;
    while (look < source_.size() && is_identifier_part(source_[look])) {
        ++look;
    }
    std::size_t after = look;
    while (after < source_.size() &&
           (source_[after] == ' ' || source_[after] == '\t')) {
        ++after;
    }
    if (look == offset_ + 1 || after >= source_.size() ||
        (source_[after] != ',' && source_[after] != '\r' &&
         source_[after] != '\n' && source_[after] != ':')) {
        return true;
    }
    advance();
    const auto number_offset = offset_;
    auto number = parse_expression();
    if (!number.has_value()) {
        return false;
    }
    if (!coerce_numeric_value(*number, Value{Integer{}}.index(),
                              number_offset) ||
        !std::holds_alternative<Integer>(*number)) {
        set_error("WFC0073", "file number must be a Long", number_offset);
        return false;
    }
    file_number = std::get<Integer>(*number);
    found = true;
    skip_horizontal_whitespace();
    static_cast<void>(consume(','));
    return true;
}

}  // namespace wfc::detail
