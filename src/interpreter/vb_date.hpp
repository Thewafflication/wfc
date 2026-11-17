// VB Date serials, calendar arithmetic, and date formatting helpers.
// Internal to the WFC evaluator; not part of the public API.
// Split out of src/evaluator.cpp; see src/interpreter/README.md.

#ifndef WFC_INTERPRETER_VB_DATE_HPP
#define WFC_INTERPRETER_VB_DATE_HPP

#include <ctime>
#include <vector>

#include "vb_value.hpp"

namespace wfc::detail {

// ---- Date support (REQ-0242) ----------------------------------------------

[[nodiscard]] inline std::int64_t days_from_civil(
    std::int64_t year, const std::int64_t month,
    const std::int64_t day) noexcept {
    year -= month <= 2 ? 1 : 0;
    const std::int64_t era = (year >= 0 ? year : year - 399) / 400;
    const std::int64_t year_of_era = year - era * 400;
    const std::int64_t day_of_year =
        (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const std::int64_t day_of_era =
        year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + day_of_era - 719468;
}

inline void civil_from_days(std::int64_t days, std::int64_t& year,
                            std::int64_t& month, std::int64_t& day) noexcept {
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const std::int64_t day_of_era = days - era * 146097;
    const std::int64_t year_of_era =
        (day_of_era - day_of_era / 1460 + day_of_era / 36524 -
         day_of_era / 146096) /
        365;
    year = year_of_era + era * 400;
    const std::int64_t day_of_year =
        day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
    const std::int64_t mp = (5 * day_of_year + 2) / 153;
    day = day_of_year - (153 * mp + 2) / 5 + 1;
    month = mp < 10 ? mp + 3 : mp - 9;
    year += month <= 2 ? 1 : 0;
}

inline constexpr std::int64_t kDateEpochOffset =
    25569;  // 1970-01-01 as an OLE serial

[[nodiscard]] inline bool is_leap_year(const std::int64_t year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

[[nodiscard]] inline std::int64_t days_in_month(
    const std::int64_t year, const std::int64_t month) noexcept {
    constexpr std::int64_t lengths[] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};
    return month == 2 && is_leap_year(year) ? 29 : lengths[month - 1];
}

// Serial for a (possibly out-of-range, e.g. month 13 / day 0) y-m-d.
[[nodiscard]] inline double date_serial(std::int64_t year, std::int64_t month,
                                        const std::int64_t day) noexcept {
    year += (month - 1 >= 0 ? (month - 1) / 12 : -((12 - month) / 12));
    month = ((month - 1) % 12 + 12) % 12 + 1;
    return static_cast<double>(days_from_civil(year, month, 1) + (day - 1) +
                               kDateEpochOffset);
}

struct DateParts {
    std::int64_t year{}, month{}, day{}, hour{}, minute{}, second{},
        weekday{};  // weekday: 1 = Sunday
};

[[nodiscard]] inline DateParts split_date(const double serial) noexcept {
    // The time of a date before the epoch counts up from midnight whatever
    // the sign (CDate(-1.5) is 12/29/1899 12:00:00 PM), so the day is the
    // serial cut toward zero.
    double whole = std::trunc(serial);
    std::int64_t seconds = static_cast<std::int64_t>(
        std::llround(std::fabs(serial - whole) * 86400.0));
    if (seconds >= 86400) {
        seconds -= 86400;
        whole += serial < 0.0 ? -1.0 : 1.0;
    }
    DateParts parts;
    const auto days = static_cast<std::int64_t>(whole);
    civil_from_days(days - kDateEpochOffset, parts.year, parts.month,
                    parts.day);
    parts.hour = seconds / 3600;
    parts.minute = seconds % 3600 / 60;
    parts.second = seconds % 60;
    parts.weekday = ((days % 7) + 7 + 6) % 7 + 1;
    return parts;
}

// DatePart "ww": the week number of `serial` for a week starting on `first_day`
// (1 = Sunday) under `first_week` (1 = week containing Jan 1, 2 = first week
// with four days, 3 = first full week).
[[nodiscard]] inline std::int64_t vb_week_of_year(
    const double serial, const std::int64_t first_day,
    const std::int64_t first_week) noexcept {
    const double day = std::floor(serial);
    const auto parts = split_date(day);
    const auto week1_start = [&](const std::int64_t year) {
        const double jan1 = date_serial(year, 1, 1);
        const auto offset = (split_date(jan1).weekday - first_day + 7) % 7;
        const double week_start = jan1 - static_cast<double>(offset);
        if (first_week == 3) {
            return offset == 0 ? jan1 : week_start + 7.0;
        }
        if (first_week == 2) {
            return (7 - offset) >= 4 ? week_start : week_start + 7.0;
        }
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
        buffer, sizeof(buffer), "%lld:%02lld:%02lld %s",
        static_cast<long long>(hour12), static_cast<long long>(parts.minute),
        static_cast<long long>(parts.second), parts.hour < 12 ? "AM" : "PM");
    return buffer;
}

[[nodiscard]] inline std::string render_date_part(const DateParts& parts) {
    return std::to_string(parts.month) + "/" + std::to_string(parts.day) + "/" +
           std::to_string(parts.year);
}

// VB6 "General Date" (US locale): date, time, or both.
[[nodiscard]] inline std::string render_date(const double serial) {
    const auto parts = split_date(serial);
    const bool has_time =
        parts.hour != 0 || parts.minute != 0 || parts.second != 0;
    const bool has_date = std::trunc(serial) != 0.0;
    if (has_date && has_time) {
        return render_date_part(parts) + " " + render_time_part(parts);
    }
    if (has_time || !has_date) {
        return render_time_part(parts);
    }
    return render_date_part(parts);
}

// Reads a date, a time, or both from text, by the rules measured under Visual
// Basic 6 (the statement probe, "date text ..."):
//  - A date is two or three parts: numbers, or numbers and one month name
//    (the full name or at least its first three letters, with a period after
//    it if wished). The parts are separated by "/", "-", "," or blanks, mixed
//    freely. Three numbers are month/day/year; if that is no date, then
//    year/month/day, then day/month/year. With a month name a number that
//    can be the day is the day and the other is the year. Two parts are a
//    month and a day of the current year when they are one, otherwise a month
//    and a year. A year below 100 is in 1950 to 2049. Names of weekdays are
//    not read.
//  - A time is hours:minutes[:seconds]; a period separates as a colon does
//    ("1.5" is five past one). AM, PM, A or P may follow, or come before a
//    time with minutes; with one of them the hour alone is a time ("1 PM").
//  - A time may come before or after the date, blanks between them.
// Returns the serial, or nullopt when the text is none of these.
[[nodiscard]] inline std::optional<double> parse_date_text(
    const std::string_view text) {
    struct Token {
        char kind;  // 'n' number, 'w' word, 'p' punctuation, 's' blanks
        std::int64_t number;
        std::size_t digits;
        std::string word;
        char mark;
    };
    std::vector<Token> tokens;
    for (std::size_t i = 0; i < text.size();) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c == ' ' || c == '\t') {
            while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) {
                ++i;
            }
            tokens.push_back({'s', 0, 0, {}, ' '});
        } else if (std::isdigit(c) != 0) {
            Token token{'n', 0, 0, {}, '\0'};
            while (i < text.size() &&
                   std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
                if (token.number < 100000000) {
                    token.number = token.number * 10 + (text[i] - '0');
                }
                ++token.digits;
                ++i;
            }
            tokens.push_back(std::move(token));
        } else if (std::isalpha(c) != 0) {
            Token token{'w', 0, 0, {}, '\0'};
            while (i < text.size() &&
                   std::isalpha(static_cast<unsigned char>(text[i])) != 0) {
                token.word.push_back(static_cast<char>(
                    std::tolower(static_cast<unsigned char>(text[i]))));
                ++i;
            }
            tokens.push_back(std::move(token));
        } else if (c == '/' || c == '-' || c == ',' || c == '.' || c == ':') {
            tokens.push_back({'p', 0, 0, {}, static_cast<char>(c)});
            ++i;
        } else {
            return std::nullopt;
        }
    }
    const auto trim = [](std::vector<Token>& list) {
        while (!list.empty() && list.front().kind == 's') {
            list.erase(list.begin());
        }
        while (!list.empty() && list.back().kind == 's') {
            list.pop_back();
        }
    };
    trim(tokens);
    if (tokens.empty()) {
        return std::nullopt;
    }
    const auto is_meridiem = [](const Token& token) {
        return token.kind == 'w' && (token.word == "am" || token.word == "pm" ||
                                     token.word == "a" || token.word == "p");
    };
    const auto is_time_mark = [&](const std::size_t at) {
        // A colon, or a period that stands between two numbers.
        if (tokens[at].kind != 'p' || at == 0 || at + 1 >= tokens.size() ||
            tokens[at - 1].kind != 'n' || tokens[at + 1].kind != 'n') {
            return false;
        }
        return tokens[at].mark == ':' || tokens[at].mark == '.';
    };

    // The time, if there is one, is taken out of the tokens.
    bool have_time = false;
    std::int64_t hour = 0;
    std::int64_t minute = 0;
    std::int64_t second = 0;
    std::size_t time_first = tokens.size();
    std::size_t time_last = tokens.size();  // one past the last token
    for (std::size_t at = 0; at < tokens.size(); ++at) {
        if (tokens[at].kind == 'p' &&
            (tokens[at].mark == ':' || (tokens[at].mark == '.' && at > 0 &&
                                        tokens[at - 1].kind == 'n'))) {
            if (!is_time_mark(at)) {
                return std::nullopt;  // ":30", "1:", "1::3"
            }
            time_first = at - 1;
            hour = tokens[at - 1].number;
            minute = tokens[at + 1].number;
            time_last = at + 2;
            if (time_last < tokens.size() && tokens[time_last].kind == 'p' &&
                (tokens[time_last].mark == ':' ||
                 tokens[time_last].mark == '.')) {
                if (!is_time_mark(time_last)) {
                    return std::nullopt;
                }
                second = tokens[time_last + 1].number;
                time_last += 2;
            }
            have_time = true;
            break;
        }
    }
    int meridiem = 0;  // 1 AM, 2 PM
    const auto meridiem_of = [](const Token& token) {
        return token.word.front() == 'p' ? 2 : 1;
    };
    if (have_time) {
        // AM or PM after the time ("1:2 PM", "1:2PM"), or before it.
        std::size_t after = time_last;
        if (after < tokens.size() && tokens[after].kind == 's') {
            ++after;
        }
        if (after < tokens.size() && is_meridiem(tokens[after])) {
            meridiem = meridiem_of(tokens[after]);
            time_last = after + 1;
        } else if (time_first > 0) {
            std::size_t before = time_first - 1;
            if (tokens[before].kind == 's' && before > 0) {
                --before;
            }
            if (is_meridiem(tokens[before])) {
                meridiem = meridiem_of(tokens[before]);
                time_first = before;
            }
        }
    } else {
        // The hour alone with AM or PM after it ("1 PM", "1PM").
        for (std::size_t at = 0; at < tokens.size(); ++at) {
            if (!is_meridiem(tokens[at])) {
                continue;
            }
            std::size_t before = at;
            if (before > 0 && tokens[before - 1].kind == 's') {
                --before;
            }
            if (before == 0 || tokens[before - 1].kind != 'n') {
                return std::nullopt;  // "pm 5"
            }
            hour = tokens[before - 1].number;
            meridiem = meridiem_of(tokens[at]);
            time_first = before - 1;
            time_last = at + 1;
            have_time = true;
            break;
        }
    }
    double time_part = 0.0;
    if (have_time) {
        if (hour > 23 || minute > 59 || second > 59) {
            return std::nullopt;
        }
        if (meridiem == 2 && hour < 12) {
            hour += 12;
        } else if (meridiem == 1 && hour == 12) {
            hour = 0;
        }
        time_part =
            static_cast<double>(hour * 3600 + minute * 60 + second) / 86400.0;
    }

    // What is left, before or after the time, is the date.
    std::vector<Token> date(
        tokens.begin(), tokens.begin() + static_cast<std::ptrdiff_t>(std::min(
                                             time_first, tokens.size())));
    std::vector<Token> rest;
    if (have_time) {
        rest.assign(tokens.begin() + static_cast<std::ptrdiff_t>(time_last),
                    tokens.end());
    }
    trim(date);
    trim(rest);
    if (!date.empty() && !rest.empty()) {
        return std::nullopt;
    }
    if (date.empty()) {
        date = std::move(rest);
    } else if (have_time && tokens[time_first - 1].kind != 's') {
        return std::nullopt;  // the time needs a blank before it
    }
    if (date.empty()) {
        return have_time ? std::optional<double>{time_part} : std::nullopt;
    }
    if (have_time && time_first == 0 && time_last < tokens.size() &&
        tokens[time_last].kind != 's') {
        return std::nullopt;  // and a blank after it
    }

    // The parts of the date.
    static const char* const month_names[] = {
        "january", "february", "march",     "april",   "may",      "june",
        "july",    "august",   "september", "october", "november", "december"};
    std::vector<std::pair<std::int64_t, std::size_t>> numbers;  // value, digits
    std::int64_t named_month = 0;
    bool expect_part = true;
    bool mark_seen = false;  // a "/", "-" or "," between two parts
    for (std::size_t at = 0; at < date.size(); ++at) {
        const Token& token = date[at];
        if (token.kind == 's') {
            continue;
        }
        if (token.kind == 'p') {
            const bool after_name = at > 0 && date[at - 1].kind == 'w';
            if (token.mark == '.' && after_name) {
                continue;  // "Jan. 5"
            }
            if (expect_part || token.mark == '.' || token.mark == ':') {
                return std::nullopt;
            }
            expect_part = true;
            mark_seen = true;
            continue;
        }
        // A part must not touch the part before it ("Jan5").
        if (at > 0 && date[at - 1].kind != 's' && date[at - 1].kind != 'p') {
            return std::nullopt;
        }
        if (token.kind == 'w') {
            if (named_month != 0 || token.word.size() < 3U) {
                return std::nullopt;
            }
            for (std::int64_t m = 0; m < 12; ++m) {
                const std::string_view name = month_names[m];
                if (name.substr(0, token.word.size()) == token.word) {
                    named_month = m + 1;
                    break;
                }
            }
            if (named_month == 0) {
                return std::nullopt;
            }
        } else {
            numbers.emplace_back(token.number, token.digits);
        }
        expect_part = false;
    }
    if (expect_part) {
        return std::nullopt;  // nothing after the last "/", "-" or ","
    }

    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    const std::int64_t this_year = local.tm_year + 1900;
    const auto full_year = [](const std::int64_t year) {
        return year < 100 ? year + (year < 50 ? 2000 : 1900) : year;
    };
    const auto valid = [](const std::int64_t year, const std::int64_t month,
                          const std::int64_t day) {
        return year >= 100 && year <= 9999 && month >= 1 && month <= 12 &&
               day >= 1 && day <= days_in_month(year, month);
    };
    std::int64_t year = 0;
    std::int64_t month = 0;
    std::int64_t day = 0;
    bool found = false;
    const auto take = [&](const std::int64_t y, const std::int64_t m,
                          const std::int64_t d) {
        if (!found && valid(y, m, d)) {
            year = y;
            month = m;
            day = d;
            found = true;
        }
    };
    if (named_month != 0) {
        if (numbers.size() == 2U) {
            const auto first = numbers[0].first;
            const auto second_number = numbers[1].first;
            take(full_year(second_number), named_month, first);
            take(full_year(first), named_month, second_number);
        } else if (numbers.size() == 1U) {
            take(this_year, named_month, numbers[0].first);
            take(full_year(numbers[0].first), named_month, 1);
        }
    } else if (numbers.size() == 3U) {
        const auto a = numbers[0].first;
        const auto b = numbers[1].first;
        const auto c = numbers[2].first;
        take(full_year(c), a, b);  // month/day/year
        take(full_year(a), b, c);  // year/month/day
        take(full_year(c), b, a);  // day/month/year
    } else if (numbers.size() == 2U && mark_seen) {
        const auto a = numbers[0].first;
        const auto b = numbers[1].first;
        if (numbers[0].second >= 3U) {
            take(a, b, 1);  // "2000/1"
        } else if (numbers[1].second >= 3U) {
            take(full_year(b), a, 1);  // "1/2000", "1,000"
        } else {
            take(this_year, a, b);
            take(full_year(b), a, 1);  // "1/32"
        }
    }
    if (!found) {
        return std::nullopt;
    }
    return date_serial(year, month, day) + time_part;
}

// True when a Format picture is a date/time picture: a named date format, or
// a picture with date/time tokens and no numeric digit placeholders.
[[nodiscard]] inline bool is_date_picture(const std::string& style) {
    std::string lowered;
    for (const char c : style) {
        lowered.push_back(c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c);
    }
    for (const char* named :
         {"general date", "long date", "medium date", "short date", "long time",
          "medium time", "short time"}) {
        if (lowered == named) {
            return true;
        }
    }
    for (const char* named :
         {"general number", "currency", "fixed", "standard", "percent",
          "scientific", "yes/no", "true/false", "on/off"}) {
        if (lowered == named) {
            return false;
        }
    }
    bool token = lowered.find("ttttt") != std::string::npos;
    for (std::size_t i = 0; i < lowered.size(); ++i) {
        const char c = lowered[i];
        if (c == '\\') {
            ++i;
        } else if (c == '"') {
            const auto close = lowered.find('"', i + 1U);
            if (close == std::string::npos) {
                break;
            }
            i = close;
        } else if (c == '0' || c == '#') {
            return false;
        } else if (std::string_view{"dmyhnsqwc"}.find(c) !=
                   std::string_view::npos) {
            token = true;
        } else if (lowered.compare(i, 4U, "am/p") == 0 ||
                   lowered.compare(i, 3U, "a/p") == 0) {
            token = true;
        }
    }
    return token;
}

// Custom/named date format for Format(date, style) (REQ-0242).
[[nodiscard]] inline std::string format_date_pattern(const double serial,
                                                     const std::string& style) {
    static const char* const month_names[] = {
        "January", "February", "March",     "April",   "May",      "June",
        "July",    "August",   "September", "October", "November", "December"};
    static const char* const day_names[] = {"Sunday",    "Monday",   "Tuesday",
                                            "Wednesday", "Thursday", "Friday",
                                            "Saturday"};
    const auto parts = split_date(serial);
    std::string lowered;
    for (const char c : style) {
        lowered.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    const auto pad2 = [](const std::int64_t v) {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "%02lld",
                      static_cast<long long>(v));
        return std::string(buffer);
    };
    if (lowered == "general date" || style.empty()) {
        return render_date(serial);
    }
    if (lowered == "long date") {
        return std::string(day_names[parts.weekday - 1]) + ", " +
               month_names[parts.month - 1] + " " + std::to_string(parts.day) +
               ", " + std::to_string(parts.year);
    }
    if (lowered == "medium date") {
        return pad2(parts.day) + "-" +
               std::string(month_names[parts.month - 1]).substr(0, 3) + "-" +
               pad2(parts.year % 100);
    }
    if (lowered == "short date") {
        return render_date_part(parts);
    }
    if (lowered == "long time") {
        return render_time_part(parts);
    }
    if (lowered == "medium time") {
        const auto h12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
        return pad2(h12) + ":" + pad2(parts.minute) + " " +
               (parts.hour < 12 ? "AM" : "PM");
    }
    if (lowered == "short time") {
        return pad2(parts.hour) + ":" + pad2(parts.minute);
    }
    const auto hour12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
    const bool has_ampm = lowered.find("am/pm") != std::string::npos ||
                          lowered.find("ampm") != std::string::npos ||
                          lowered.find("a/p") != std::string::npos;
    std::string out;
    std::size_t i = 0;
    const auto run_length = [&](const char c) {
        std::size_t n = 0;
        while (i + n < lowered.size() && lowered[i + n] == c) {
            ++n;
        }
        return n;
    };
    // Whether a `m` token at lowered[pos] means minutes (follows h, precedes
    // s).
    const auto is_minute = [&](const std::size_t pos, const std::size_t len) {
        std::size_t before = pos;
        while (before > 0 &&
               (lowered[before - 1] == ' ' || lowered[before - 1] == ':')) {
            --before;
        }
        if (before > 0 && lowered[before - 1] == 'h') {
            return true;
        }
        std::size_t after = pos + len;
        while (after < lowered.size() &&
               (lowered[after] == ' ' || lowered[after] == ':')) {
            ++after;
        }
        return after < lowered.size() && lowered[after] == 's';
    };
    while (i < style.size()) {
        const char c = lowered[i];
        if (c == '"') {
            ++i;
            while (i < style.size() && style[i] != '"') {
                out.push_back(style[i++]);
            }
            ++i;
        } else if (c == '\\' && i + 1 < style.size()) {
            out.push_back(style[i + 1]);
            i += 2;
        } else if (c == 'y') {
            const auto n = run_length('y');
            if (n == 1) {
                out += std::to_string(
                    static_cast<std::int64_t>(std::trunc(serial) -
                                              date_serial(parts.year, 1, 1)) +
                    1);
            } else {
                out += n >= 3 ? std::to_string(parts.year)
                              : pad2(parts.year % 100);
            }
            i += n;
        } else if (c == 'w') {
            const auto n = run_length('w');
            if (n == 1) {
                out += std::to_string(parts.weekday);
            } else {
                const auto doy =
                    static_cast<std::int64_t>(std::floor(serial) -
                                              date_serial(parts.year, 1, 1)) +
                    1;
                const auto jan1_weekday =
                    split_date(date_serial(parts.year, 1, 1)).weekday;
                out += std::to_string((doy - 1 + jan1_weekday - 1) / 7 + 1);
            }
            i += n;
        } else if (c == 'q') {
            out += std::to_string((parts.month - 1) / 3 + 1);
            ++i;
        } else if (c == 'm') {
            const auto n = run_length('m');
            if (is_minute(i, n)) {
                out +=
                    n >= 2 ? pad2(parts.minute) : std::to_string(parts.minute);
            } else if (n == 1) {
                out += std::to_string(parts.month);
            } else if (n == 2) {
                out += pad2(parts.month);
            } else if (n == 3) {
                out += std::string(month_names[parts.month - 1]).substr(0, 3);
            } else {
                out += month_names[parts.month - 1];
            }
            i += n;
        } else if (c == 'd' && lowered.compare(i, 6, "dddddd") == 0) {
            out += format_date_pattern(serial, "Long Date");
            i += 6;
        } else if (c == 'd' && lowered.compare(i, 5, "ddddd") == 0) {
            out += render_date_part(parts);
            i += 5;
        } else if (c == 'd') {
            const auto n = run_length('d');
            if (n == 1) {
                out += std::to_string(parts.day);
            } else if (n == 2) {
                out += pad2(parts.day);
            } else if (n == 3) {
                out += std::string(day_names[parts.weekday - 1]).substr(0, 3);
            } else {
                out += day_names[parts.weekday - 1];
            }
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
            out += parts.hour < 12 ? (style[i] == 'a' ? "am" : "AM")
                                   : (style[i] == 'a' ? "pm" : "PM");
            i += 5;
        } else if (lowered.compare(i, 4, "ampm") == 0) {
            out += parts.hour < 12 ? "AM" : "PM";
            i += 4;
        } else if (lowered.compare(i, 3, "a/p") == 0) {
            out += parts.hour < 12 ? (style[i] == 'a' ? "a" : "A")
                                   : (style[i] == 'a' ? "p" : "P");
            i += 3;
        } else if (lowered.compare(i, 5, "ttttt") == 0) {
            out += render_time_part(parts);
            i += 5;
        } else if (lowered[i] == 'c' && i + 1U >= lowered.size()) {
            // `c` alone is the general date and time (ddddd ttttt).
            out += render_date(serial);
            ++i;
        } else {
            out.push_back(style[i]);
            ++i;
        }
    }
    return out;
}

}  // namespace wfc::detail

#endif  // WFC_INTERPRETER_VB_DATE_HPP
