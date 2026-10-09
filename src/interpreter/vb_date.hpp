// VB Date serials, calendar arithmetic, and date formatting helpers.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#ifndef WFC_INTERPRETER_VB_DATE_HPP
#define WFC_INTERPRETER_VB_DATE_HPP

#include "vb_value.hpp"

namespace wfc::detail {

// ---- Date support (REQ-0242) ----------------------------------------------

[[nodiscard]] inline std::int64_t days_from_civil(
    std::int64_t year, const std::int64_t month, const std::int64_t day) noexcept {
    year -= month <= 2 ? 1 : 0;
    const std::int64_t era = (year >= 0 ? year : year - 399) / 400;
    const std::int64_t year_of_era = year - era * 400;
    const std::int64_t day_of_year = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const std::int64_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + day_of_era - 719468;
}

inline void civil_from_days(
    std::int64_t days, std::int64_t& year, std::int64_t& month, std::int64_t& day) noexcept {
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const std::int64_t day_of_era = days - era * 146097;
    const std::int64_t year_of_era =
        (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
    year = year_of_era + era * 400;
    const std::int64_t day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
    const std::int64_t mp = (5 * day_of_year + 2) / 153;
    day = day_of_year - (153 * mp + 2) / 5 + 1;
    month = mp < 10 ? mp + 3 : mp - 9;
    year += month <= 2 ? 1 : 0;
}

inline constexpr std::int64_t kDateEpochOffset = 25569;  // 1970-01-01 as an OLE serial

[[nodiscard]] inline bool is_leap_year(const std::int64_t year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

[[nodiscard]] inline std::int64_t days_in_month(
    const std::int64_t year, const std::int64_t month) noexcept {
    constexpr std::int64_t lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return month == 2 && is_leap_year(year) ? 29 : lengths[month - 1];
}

// Serial for a (possibly out-of-range, e.g. month 13 / day 0) y-m-d.
[[nodiscard]] inline double date_serial(
    std::int64_t year, std::int64_t month, const std::int64_t day) noexcept {
    year += (month - 1 >= 0 ? (month - 1) / 12 : -((12 - month) / 12));
    month = ((month - 1) % 12 + 12) % 12 + 1;
    return static_cast<double>(days_from_civil(year, month, 1) + (day - 1) + kDateEpochOffset);
}

struct DateParts {
    std::int64_t year{}, month{}, day{}, hour{}, minute{}, second{}, weekday{};  // weekday: 1 = Sunday
};

[[nodiscard]] inline DateParts split_date(const double serial) noexcept {
    double whole = std::floor(serial);
    std::int64_t seconds = static_cast<std::int64_t>(std::llround((serial - whole) * 86400.0));
    if (seconds >= 86400) {
        seconds -= 86400;
        whole += 1.0;
    }
    DateParts parts;
    const auto days = static_cast<std::int64_t>(whole);
    civil_from_days(days - kDateEpochOffset, parts.year, parts.month, parts.day);
    parts.hour = seconds / 3600;
    parts.minute = seconds % 3600 / 60;
    parts.second = seconds % 60;
    parts.weekday = ((days % 7) + 7 + 6) % 7 + 1;
    return parts;
}

// DatePart "ww": the week number of `serial` for a week starting on `first_day` (1 = Sunday)
// under `first_week` (1 = week containing Jan 1, 2 = first week with four days, 3 = first full week).
[[nodiscard]] inline std::int64_t vb_week_of_year(
    const double serial, const std::int64_t first_day, const std::int64_t first_week) noexcept {
    const double day = std::floor(serial);
    const auto parts = split_date(day);
    const auto week1_start = [&](const std::int64_t year) {
        const double jan1 = date_serial(year, 1, 1);
        const auto offset = (split_date(jan1).weekday - first_day + 7) % 7;
        const double week_start = jan1 - static_cast<double>(offset);
        if (first_week == 3) return offset == 0 ? jan1 : week_start + 7.0;
        if (first_week == 2) return (7 - offset) >= 4 ? week_start : week_start + 7.0;
        return week_start;
    };
    double start = week1_start(parts.year);
    if (first_week != 1 && day < start) {
        start = week1_start(parts.year - 1);
    } else if (first_week == 2 && day >= week1_start(parts.year + 1)) {
        return 1;
    }
    return static_cast<std::int64_t>(std::floor((day - start) / 7.0)) + 1;
}

[[nodiscard]] inline std::string render_time_part(const DateParts& parts) {
    const auto hour12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
    char buffer[32];
    std::snprintf(
        buffer, sizeof(buffer), "%lld:%02lld:%02lld %s", static_cast<long long>(hour12),
        static_cast<long long>(parts.minute), static_cast<long long>(parts.second),
        parts.hour < 12 ? "AM" : "PM");
    return buffer;
}

[[nodiscard]] inline std::string render_date_part(const DateParts& parts) {
    return std::to_string(parts.month) + "/" + std::to_string(parts.day) + "/" +
           std::to_string(parts.year);
}

// VB6 "General Date" (US locale): date, time, or both.
[[nodiscard]] inline std::string render_date(const double serial) {
    const auto parts = split_date(serial);
    const bool has_time = parts.hour != 0 || parts.minute != 0 || parts.second != 0;
    const bool has_date = std::floor(serial) != 0.0;
    if (has_date && has_time) {
        return render_date_part(parts) + " " + render_time_part(parts);
    }
    if (has_time || !has_date) {
        return render_time_part(parts);
    }
    return render_date_part(parts);
}

// Parses "m/d/yyyy", "yyyy-mm-dd" or "yyyy/m/d" (2-digit years: 00-29 ->
// 2000s, 30-99 -> 1900s), each optionally followed by "h:mm[:ss][ AM|PM]",
// or a time alone. Returns the serial, or nullopt when malformed.
[[nodiscard]] inline std::optional<double> parse_date_text(const std::string_view text) {
    std::size_t i = 0;
    const auto skip_space = [&] {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i;
    };
    const auto read_number = [&](std::int64_t& value, std::size_t& digits) {
        digits = 0;
        value = 0;
        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
            value = value * 10 + (text[i] - '0');
            ++i;
            ++digits;
        }
        return digits > 0;
    };
    skip_space();
    double result = 0.0;
    bool have_date = false;
    const auto start = i;
    std::int64_t a{}, b{}, c{};
    std::size_t da{}, db{}, dc{};
    // Month names: "March 5, 2024", "Fri, Mar 15 2024", "5 March 2024", "5-Mar-24".
    const auto read_word = [&]() {
        std::string word;
        while (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i])) != 0) {
            word.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(text[i]))));
            ++i;
        }
        return word;
    };
    const auto month_from_word = [](const std::string& word) -> std::int64_t {
        static const char* const names[] = {"january", "february", "march", "april", "may", "june",
            "july", "august", "september", "october", "november", "december"};
        if (word.size() < 3U) return 0;
        for (std::int64_t m = 0; m < 12; ++m) {
            const std::string_view name = names[m];
            if (word == name || (word.size() == 3U && name.substr(0, 3) == word)) return m + 1;
        }
        return 0;
    };
    const auto skip_separators = [&] {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == ',' ||
                                   text[i] == '-' || text[i] == '.')) {
            ++i;
        }
    };
    const auto finish_named = [&](std::int64_t month, std::int64_t day, std::int64_t year,
                                  const bool have_year) -> bool {
        if (!have_year) year = 2000;  // no year given: a fixed year keeps parsing deterministic
        if (year < 100) year += year < 30 ? 2000 : 1900;
        if (month < 1 || day < 1 || year < 100 || year > 9999 || day > days_in_month(year, month)) {
            return false;
        }
        result = date_serial(year, month, day);
        have_date = true;
        return true;
    };
    if (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i])) != 0) {
        auto word = read_word();
        static const char* const weekdays[] = {"sunday", "monday", "tuesday", "wednesday",
            "thursday", "friday", "saturday"};
        for (const char* weekday : weekdays) {
            const std::string_view name = weekday;
            if (word == name || (word.size() == 3U && name.substr(0, 3) == word)) {
                skip_separators();
                word = read_word();
                break;
            }
        }
        const auto month = month_from_word(word);
        if (month == 0) return std::nullopt;
        skip_separators();
        std::int64_t first{}, second{};
        std::size_t first_digits{}, second_digits{};
        if (!read_number(first, first_digits)) {
            return std::nullopt;
        }
        std::int64_t day = first, year = 0;
        bool have_year = false;
        skip_separators();
        const auto before_next = i;
        if (read_number(second, second_digits) && !(i < text.size() && text[i] == ':')) {
            year = second;
            have_year = true;
        } else {
            i = before_next;
            if (first_digits > 2U) {  // "March 2024"
                year = first;
                day = 1;
                have_year = true;
            }
        }
        if (!finish_named(month, day, year, have_year)) return std::nullopt;
    } else if (read_number(a, da) && i < text.size() &&
               (text[i] == ' ' || text[i] == '-') && [&] {
                   const auto save = i;
                   skip_separators();
                   const bool alpha = i < text.size() &&
                                      std::isalpha(static_cast<unsigned char>(text[i])) != 0;
                   i = save;
                   return alpha;
               }()) {
        // "5 March 2024" / "5-Mar-24"
        skip_separators();
        const auto month = month_from_word(read_word());
        if (month == 0) return std::nullopt;
        skip_separators();
        std::int64_t year{};
        std::size_t year_digits{};
        const auto before_year = i;
        bool have_year = false;
        if (read_number(year, year_digits) && !(i < text.size() && text[i] == ':')) {
            have_year = true;
        } else {
            i = before_year;
        }
        if (!finish_named(month, a, year, have_year)) return std::nullopt;
    } else if (da > 0 && i < text.size() && (text[i] == '/' || text[i] == '-')) {
        const char separator = text[i++];
        if (!read_number(b, db) || i >= text.size() || text[i] != separator) {
            return std::nullopt;
        }
        ++i;
        if (!read_number(c, dc)) {
            return std::nullopt;
        }
        std::int64_t year{}, month{}, day{};
        if (da >= 3) {
            year = a; month = b; day = c;
        } else {
            month = a; day = b; year = c;
            if (dc <= 2) year += year < 30 ? 2000 : 1900;
        }
        if (month < 1 || month > 12 || day < 1 || year < 100 || year > 9999 ||
            day > days_in_month(year, month)) {
            return std::nullopt;
        }
        result = date_serial(year, month, day);
        have_date = true;
    } else {
        i = start;
    }
    skip_space();
    if (i < text.size()) {
        std::int64_t hour{}, minute{}, second{};
        std::size_t digits{};
        if (!read_number(hour, digits) || i >= text.size() || text[i] != ':') {
            return std::nullopt;
        }
        ++i;
        if (!read_number(minute, digits)) {
            return std::nullopt;
        }
        if (i < text.size() && text[i] == ':') {
            ++i;
            if (!read_number(second, digits)) {
                return std::nullopt;
            }
        }
        skip_space();
        if (i + 1 < text.size() + 0 && i + 2 <= text.size()) {
            const char m0 = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
            const char m1 = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i + 1])));
            if ((m0 == 'a' || m0 == 'p') && m1 == 'm') {
                if (hour < 1 || hour > 12) return std::nullopt;
                hour = hour % 12 + (m0 == 'p' ? 12 : 0);
                i += 2;
            }
        }
        skip_space();
        if (i != text.size() || hour > 23 || minute > 59 || second > 59) {
            return std::nullopt;
        }
        result += static_cast<double>(hour * 3600 + minute * 60 + second) / 86400.0;
    } else if (!have_date) {
        return std::nullopt;
    }
    return result;
}

// Custom/named date format for Format(date, style) (REQ-0242).
[[nodiscard]] inline std::string format_date_pattern(const double serial, const std::string& style) {
    static const char* const month_names[] = {"January", "February", "March", "April",
        "May", "June", "July", "August", "September", "October", "November", "December"};
    static const char* const day_names[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday"};
    const auto parts = split_date(serial);
    std::string lowered;
    for (const char c : style) lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    const auto pad2 = [](const std::int64_t v) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%02lld", static_cast<long long>(v));
        return std::string(buffer);
    };
    if (lowered == "general date" || style.empty()) return render_date(serial);
    if (lowered == "long date") {
        return std::string(day_names[parts.weekday - 1]) + ", " + month_names[parts.month - 1] +
               " " + std::to_string(parts.day) + ", " + std::to_string(parts.year);
    }
    if (lowered == "medium date") {
        return pad2(parts.day) + "-" + std::string(month_names[parts.month - 1]).substr(0, 3) +
               "-" + pad2(parts.year % 100);
    }
    if (lowered == "short date") return render_date_part(parts);
    if (lowered == "long time") return render_time_part(parts);
    if (lowered == "medium time") {
        const auto h12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
        return pad2(h12) + ":" + pad2(parts.minute) + " " + (parts.hour < 12 ? "AM" : "PM");
    }
    if (lowered == "short time") return pad2(parts.hour) + ":" + pad2(parts.minute);
    const auto hour12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
    const bool has_ampm = lowered.find("am/pm") != std::string::npos ||
                          lowered.find("ampm") != std::string::npos ||
                          lowered.find("a/p") != std::string::npos;
    std::string out;
    std::size_t i = 0;
    const auto run_length = [&](const char c) {
        std::size_t n = 0;
        while (i + n < lowered.size() && lowered[i + n] == c) ++n;
        return n;
    };
    // Whether a `m` token at lowered[pos] means minutes (follows h, precedes s).
    const auto is_minute = [&](const std::size_t pos, const std::size_t len) {
        std::size_t before = pos;
        while (before > 0 && (lowered[before - 1] == ' ' || lowered[before - 1] == ':')) --before;
        if (before > 0 && lowered[before - 1] == 'h') return true;
        std::size_t after = pos + len;
        while (after < lowered.size() && (lowered[after] == ' ' || lowered[after] == ':')) ++after;
        return after < lowered.size() && lowered[after] == 's';
    };
    while (i < style.size()) {
        const char c = lowered[i];
        if (c == '"') {
            ++i;
            while (i < style.size() && style[i] != '"') out.push_back(style[i++]);
            ++i;
        } else if (c == '\\' && i + 1 < style.size()) {
            out.push_back(style[i + 1]);
            i += 2;
        } else if (c == 'y') {
            const auto n = run_length('y');
            if (n == 1) {
                out += std::to_string(
                    static_cast<std::int64_t>(serial - date_serial(parts.year, 1, 1)) + 1);
            } else {
                out += n >= 3 ? std::to_string(parts.year) : pad2(parts.year % 100);
            }
            i += n;
        } else if (c == 'w') {
            const auto n = run_length('w');
            if (n == 1) {
                out += std::to_string(parts.weekday);
            } else {
                const auto doy = static_cast<std::int64_t>(std::floor(serial) - date_serial(parts.year, 1, 1)) + 1;
                const auto jan1_weekday = split_date(date_serial(parts.year, 1, 1)).weekday;
                out += std::to_string((doy - 1 + jan1_weekday - 1) / 7 + 1);
            }
            i += n;
        } else if (c == 'q') {
            out += std::to_string((parts.month - 1) / 3 + 1);
            ++i;
        } else if (c == 'm') {
            const auto n = run_length('m');
            if (is_minute(i, n)) {
                out += n >= 2 ? pad2(parts.minute) : std::to_string(parts.minute);
            } else if (n == 1) out += std::to_string(parts.month);
            else if (n == 2) out += pad2(parts.month);
            else if (n == 3) out += std::string(month_names[parts.month - 1]).substr(0, 3);
            else out += month_names[parts.month - 1];
            i += n;
        } else if (c == 'd' && lowered.compare(i, 6, "dddddd") == 0) {
            out += format_date_pattern(serial, "Long Date");
            i += 6;
        } else if (c == 'd' && lowered.compare(i, 5, "ddddd") == 0) {
            out += render_date_part(parts);
            i += 5;
        } else if (c == 'd') {
            const auto n = run_length('d');
            if (n == 1) out += std::to_string(parts.day);
            else if (n == 2) out += pad2(parts.day);
            else if (n == 3) out += std::string(day_names[parts.weekday - 1]).substr(0, 3);
            else out += day_names[parts.weekday - 1];
            i += n;
        } else if (c == 'h') {
            const auto n = run_length('h');
            const auto h = has_ampm ? hour12 : parts.hour;
            out += n >= 2 ? pad2(h) : std::to_string(h);
            i += n;
        } else if (c == 'n') {
            const auto n = run_length('n');
            out += n >= 2 ? pad2(parts.minute) : std::to_string(parts.minute);
            i += n;
        } else if (c == 's') {
            const auto n = run_length('s');
            out += n >= 2 ? pad2(parts.second) : std::to_string(parts.second);
            i += n;
        } else if (lowered.compare(i, 5, "am/pm") == 0) {
            out += parts.hour < 12 ? (style[i] == 'a' ? "am" : "AM") : (style[i] == 'a' ? "pm" : "PM");
            i += 5;
        } else if (lowered.compare(i, 4, "ampm") == 0) {
            out += parts.hour < 12 ? "AM" : "PM";
            i += 4;
        } else if (lowered.compare(i, 3, "a/p") == 0) {
            out += parts.hour < 12 ? (style[i] == 'a' ? "a" : "A") : (style[i] == 'a' ? "p" : "P");
            i += 3;
        } else if (lowered.compare(i, 5, "ttttt") == 0) {
            out += render_time_part(parts);
            i += 5;
        } else {
            out.push_back(style[i]);
            ++i;
        }
    }
    return out;
}

}  // namespace wfc::detail

#endif  // WFC_INTERPRETER_VB_DATE_HPP
