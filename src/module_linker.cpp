#include "module_linker.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string_view>

namespace wfc::detail {
namespace {

[[nodiscard]] std::string lower_of(const std::string_view text) {
    std::string result;
    for (const char c : text) {
        result.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

[[nodiscard]] bool word_start(const char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_' ||
           static_cast<unsigned char>(c) >= 0x80U;
}

[[nodiscard]] bool word_part(const char c) {
    return word_start(c) || std::isdigit(static_cast<unsigned char>(c)) != 0;
}

struct Token {
    std::string text;
    std::size_t begin{};
    std::size_t end{};
    bool word{};
};

// The code tokens of one physical line: strings are one `"` token, comments
// and `#` directives are dropped.
[[nodiscard]] std::vector<Token> lex_line(const std::string& line) {
    std::vector<Token> tokens;
    std::size_t i = 0;
    const auto size = line.size();
    while (i < size) {
        const char c = line[i];
        if (c == ' ' || c == '\t' || c == '\r') {
            ++i;
        } else if (c == '\'') {
            break;
        } else if (c == '"') {
            std::size_t j = i + 1;
            while (j < size) {
                if (line[j] == '"') {
                    if (j + 1 < size && line[j + 1] == '"') {
                        j += 2;
                    } else {
                        ++j;
                        break;
                    }
                } else {
                    ++j;
                }
            }
            tokens.push_back({"\"", i, j, false});
            i = j;
        } else if (word_start(c)) {
            std::size_t j = i;
            while (j < size && word_part(line[j])) {
                ++j;
            }
            std::string text = line.substr(i, j - i);
            if (tokens.empty() && lower_of(text) == "rem") {
                break;
            }
            tokens.push_back({std::move(text), i, j, true});
            i = j;
        } else if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
            std::size_t j = i;
            while (j < size && (word_part(line[j]) || line[j] == '.')) {
                ++j;
            }
            tokens.push_back({line.substr(i, j - i), i, j, false});
            i = j;
        } else if (c == '#' && tokens.empty()) {
            return {};  // a conditional-compilation directive
        } else {
            tokens.push_back({std::string(1U, c), i, i + 1U, false});
            ++i;
        }
    }
    return tokens;
}

[[nodiscard]] bool is(const std::vector<Token>& tokens, const std::size_t index,
                      const std::string_view word) {
    return index < tokens.size() && tokens[index].word &&
           lower_of(tokens[index].text) == word;
}

struct Definition {
    std::size_t module{};
    bool is_public{};
};

struct ModuleScan {
    std::set<std::string> names;        // lower-cased module-level names
    std::set<std::size_t> field_lines;  // physical lines inside Type blocks
};

[[nodiscard]] std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (true) {
        const auto end = text.find('\n', start);
        if (end == std::string::npos) {
            lines.push_back(text.substr(start));
            return lines;
        }
        lines.push_back(text.substr(start, end - start));
        start = end + 1U;
    }
}

[[nodiscard]] std::string join_lines(const std::vector<std::string>& lines) {
    std::string text;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0U) {
            text.push_back('\n');
        }
        text += lines[i];
    }
    return text;
}

// First word of each top-level comma-separated segment from `index` on.
void segment_names(const std::vector<Token>& tokens, std::size_t index,
                   std::vector<std::string>& out) {
    bool at_start = true;
    int depth = 0;
    for (; index < tokens.size(); ++index) {
        const auto& token = tokens[index];
        if (depth == 0 && token.text == ",") {
            at_start = true;
            continue;
        }
        if (token.text == "(") {
            ++depth;
        } else if (token.text == ")") {
            --depth;
        }
        if (at_start && token.word && depth == 0) {
            if (lower_of(token.text) == "withevents") {
                continue;
            }
            out.push_back(token.text);
            at_start = false;
        } else if (at_start && !token.word) {
            at_start = false;
        }
    }
}

// Scans one module's source for its module-level names.
void scan_module(const std::string& text, const std::size_t module,
                 std::map<std::string, std::vector<Definition>>& definitions,
                 ModuleScan& scan) {
    const auto lines = split_lines(text);
    bool in_proc = false;
    bool in_type = false;
    bool in_enum = false;
    const auto add = [&](const std::string& name, const bool is_public) {
        const auto key = lower_of(name);
        if (scan.names.insert(key).second) {
            definitions[key].push_back({module, is_public});
        }
    };
    for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const auto first_line = line_index;
        auto tokens = lex_line(lines[line_index]);
        while (!tokens.empty() && tokens.back().text == "_" &&
               line_index + 1U < lines.size()) {
            tokens.pop_back();
            ++line_index;
            const auto more = lex_line(lines[line_index]);
            tokens.insert(tokens.end(), more.begin(), more.end());
        }
        if (tokens.empty()) {
            continue;
        }
        std::size_t i = 0;
        if (!tokens[0].word &&
            std::isdigit(static_cast<unsigned char>(tokens[0].text[0])) != 0) {
            i = 1U;  // a line number
        }
        if (in_type) {
            if (is(tokens, i, "end") && is(tokens, i + 1U, "type")) {
                in_type = false;
            } else {
                for (auto k = first_line; k <= line_index; ++k) {
                    scan.field_lines.insert(k);
                }
            }
            continue;
        }
        if (in_enum) {
            if (is(tokens, i, "end") && is(tokens, i + 1U, "enum")) {
                in_enum = false;
            } else if (i < tokens.size() && tokens[i].word) {
                add(tokens[i].text, true);
            }
            continue;
        }
        if (in_proc) {
            if (is(tokens, i, "end") &&
                (is(tokens, i + 1U, "sub") || is(tokens, i + 1U, "function") ||
                 is(tokens, i + 1U, "property"))) {
                in_proc = false;
            }
            continue;
        }
        bool is_public = true;
        bool has_dim = false;
        bool explicit_modifier = false;
        while (i < tokens.size() &&
               (is(tokens, i, "public") || is(tokens, i, "private") ||
                is(tokens, i, "friend") || is(tokens, i, "global") ||
                is(tokens, i, "static") || is(tokens, i, "dim"))) {
            explicit_modifier = true;
            is_public = !(is(tokens, i, "private") || is(tokens, i, "dim"));
            has_dim = has_dim || is(tokens, i, "dim");
            ++i;
        }
        if (i >= tokens.size()) {
            continue;
        }
        if (is(tokens, i, "sub") || is(tokens, i, "function")) {
            if (i + 1U < tokens.size() && tokens[i + 1U].word) {
                add(tokens[i + 1U].text, is_public);
            }
            in_proc = true;
        } else if (is(tokens, i, "property")) {
            if (i + 2U < tokens.size() && tokens[i + 2U].word) {
                add(tokens[i + 2U].text, is_public);
            }
            in_proc = true;
        } else if (is(tokens, i, "declare")) {
            std::size_t j = i + 1U;
            if (is(tokens, j, "ptrsafe")) {
                ++j;
            }
            if (is(tokens, j, "function") || is(tokens, j, "sub")) {
                ++j;
            }
            if (j < tokens.size() && tokens[j].word) {
                add(tokens[j].text, is_public);
            }
        } else if (is(tokens, i, "const")) {
            std::vector<std::string> names;
            segment_names(tokens, i + 1U, names);
            for (const auto& name : names) {
                add(name, is_public);
            }
        } else if (is(tokens, i, "enum")) {
            if (i + 1U < tokens.size() && tokens[i + 1U].word) {
                add(tokens[i + 1U].text, is_public);
            }
            in_enum = true;
        } else if (is(tokens, i, "type")) {
            if (i + 1U < tokens.size() && tokens[i + 1U].word) {
                add(tokens[i + 1U].text, is_public);
            }
            in_type = true;
        } else if (explicit_modifier && !is(tokens, i, "event") &&
                   !is(tokens, i, "declare")) {
            std::vector<std::string> names;
            segment_names(tokens, i, names);
            for (const auto& name : names) {
                add(name, is_public);
            }
        }
        static_cast<void>(has_dim);
    }
}

[[nodiscard]] std::string mangled(const std::string& name,
                                  const std::string& module_name) {
    return name + "__" + module_name;
}

struct LinkContext {
    // lower-cased module name -> module index
    std::map<std::string, std::size_t> module_index;
    std::vector<std::string> module_names;
    std::map<std::string, std::vector<Definition>> definitions;
    std::vector<ModuleScan> scans;
};

[[nodiscard]] bool follows_member_dot(const std::string& line,
                                      const std::size_t begin) {
    std::size_t k = begin;
    while (k > 0U && (line[k - 1U] == ' ' || line[k - 1U] == '\t')) {
        --k;
    }
    return k > 0U && (line[k - 1U] == '.' || line[k - 1U] == '!');
}

[[nodiscard]] bool named_argument(const std::string& line,
                                  const std::size_t end) {
    std::size_t k = end;
    while (k < line.size() && (line[k] == ' ' || line[k] == '\t')) {
        ++k;
    }
    return k + 1U < line.size() && line[k] == ':' && line[k + 1U] == '=';
}

[[nodiscard]] bool defines(const LinkContext& context,
                           const std::string& lower_name,
                           const std::size_t module) {
    const auto found = context.definitions.find(lower_name);
    if (found == context.definitions.end()) {
        return false;
    }
    return std::any_of(found->second.begin(), found->second.end(),
                       [&](const Definition& d) { return d.module == module; });
}

[[nodiscard]] bool colliding(const LinkContext& context,
                             const std::string& lower_name) {
    const auto found = context.definitions.find(lower_name);
    return found != context.definitions.end() && found->second.size() > 1U;
}

// Rewrites one physical line. `module` is the defining module for standard
// module source, or npos for a class module (qualified references only).
[[nodiscard]] std::string rewrite_line(const LinkContext& context,
                                       const std::string& line,
                                       const std::size_t module) {
    const auto tokens = lex_line(line);
    struct Edit {
        std::size_t begin;
        std::size_t end;
        std::string text;
    };
    std::vector<Edit> edits;
    std::string declared_original;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (!token.word || follows_member_dot(line, token.begin)) {
            continue;
        }
        const auto key = lower_of(token.text);
        // `Module.Name` for a colliding name that Module defines.
        const auto module_found = context.module_index.find(key);
        if (module_found != context.module_index.end() &&
            i + 2U < tokens.size() && tokens[i + 1U].text == "." &&
            tokens[i + 2U].word) {
            const auto member_key = lower_of(tokens[i + 2U].text);
            if (colliding(context, member_key) &&
                defines(context, member_key, module_found->second)) {
                edits.push_back(
                    {token.begin, tokens[i + 2U].end,
                     mangled(tokens[i + 2U].text,
                             context.module_names[module_found->second])});
                i += 2U;
                continue;
            }
        }
        if (module == std::string::npos || !colliding(context, key) ||
            named_argument(line, token.end)) {
            continue;
        }
        if (defines(context, key, module)) {
            edits.push_back(
                {token.begin, token.end,
                 mangled(token.text, context.module_names[module])});
            if (declared_original.empty() && i > 0U &&
                (is(tokens, i - 1U, "function") || is(tokens, i - 1U, "sub"))) {
                declared_original = token.text;
            }
            continue;
        }
        // An unqualified reference to a name that exactly one other module
        // exports.
        const auto& definitions = context.definitions.at(key);
        const Definition* only = nullptr;
        std::size_t exported = 0;
        for (const auto& definition : definitions) {
            if (definition.is_public) {
                only = &definition;
                ++exported;
            }
        }
        if (exported == 1U) {
            edits.push_back(
                {token.begin, token.end,
                 mangled(token.text, context.module_names[only->module])});
        }
    }
    std::string result = line;
    // A renamed `Declare` must keep binding to the original export.
    if (!declared_original.empty() && !edits.empty()) {
        bool has_alias = false;
        std::size_t lib_end = std::string::npos;
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (is(tokens, i, "alias")) {
                has_alias = true;
            }
            if (is(tokens, i, "lib") && i + 1U < tokens.size() &&
                tokens[i + 1U].text == "\"") {
                lib_end = tokens[i + 1U].end;
            }
        }
        if (is(tokens, 0, "declare") || is(tokens, 1, "declare") ||
            is(tokens, 2, "declare")) {
            if (!has_alias && lib_end != std::string::npos) {
                edits.push_back(
                    {lib_end, lib_end, " Alias \"" + declared_original + "\""});
            }
        }
    }
    std::sort(edits.begin(), edits.end(),
              [](const Edit& a, const Edit& b) { return a.begin > b.begin; });
    for (const auto& edit : edits) {
        result.replace(edit.begin, edit.end - edit.begin, edit.text);
    }
    return result;
}

[[nodiscard]] std::string rewrite_text(const LinkContext& context,
                                       const std::string& text,
                                       const std::size_t module,
                                       const std::set<std::size_t>& skipped) {
    auto lines = split_lines(text);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (skipped.contains(i)) {
            continue;
        }
        lines[i] = rewrite_line(context, lines[i], module);
    }
    return join_lines(lines);
}

struct OptionLines {
    bool explicit_on{};
    bool base_one{};
    bool compare_text{};
    bool compare_binary{};
};

// Blanks a module's Option statements (keeping its line count) and reports
// them.
[[nodiscard]] OptionLines strip_options(std::string& text) {
    OptionLines found;
    auto lines = split_lines(text);
    for (auto& line : lines) {
        const auto tokens = lex_line(line);
        if (!is(tokens, 0, "option")) {
            continue;
        }
        if (is(tokens, 1, "explicit")) {
            found.explicit_on = true;
        } else if (is(tokens, 1, "base") && tokens.size() > 2U &&
                   tokens[2].text == "1") {
            found.base_one = true;
        } else if (is(tokens, 1, "compare") && is(tokens, 2, "text")) {
            found.compare_text = true;
        } else if (is(tokens, 1, "compare") && is(tokens, 2, "binary")) {
            found.compare_binary = true;
        }
        const bool carriage = !line.empty() && line.back() == '\r';
        line = carriage ? "\r" : "";
    }
    text = join_lines(lines);
    return found;
}

}  // namespace

std::string link_modules(std::vector<std::string>& modules,
                         const std::vector<std::string>& names,
                         std::vector<std::string>& classes) {
    if (modules.size() < 2U) {
        return {};
    }
    LinkContext context;
    context.module_names = names;
    for (std::size_t i = 0; i < names.size(); ++i) {
        context.module_index.emplace(lower_of(names[i]), i);
    }
    context.scans.resize(modules.size());
    for (std::size_t i = 0; i < modules.size(); ++i) {
        scan_module(modules[i], i, context.definitions, context.scans[i]);
    }
    for (std::size_t i = 0; i < modules.size(); ++i) {
        modules[i] =
            rewrite_text(context, modules[i], i, context.scans[i].field_lines);
    }
    for (auto& cls : classes) {
        cls = rewrite_text(context, cls, std::string::npos, {});
    }

    bool all_explicit = true;
    bool any_base = false;
    bool any_text = false;
    bool any_binary = false;
    for (auto& module : modules) {
        const auto options = strip_options(module);
        all_explicit = all_explicit && options.explicit_on;
        any_base = any_base || options.base_one;
        any_text = any_text || options.compare_text;
        any_binary = any_binary || options.compare_binary;
    }
    std::string header;
    if (all_explicit) {
        header += "Option Explicit\n";
    }
    if (any_base) {
        header += "Option Base 1\n";
    }
    if (any_text) {
        header += "Option Compare Text\n";
    } else if (any_binary) {
        header += "Option Compare Binary\n";
    }
    return header;
}

}  // namespace wfc::detail
