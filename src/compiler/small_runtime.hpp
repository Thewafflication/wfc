#pragma once

// The runtime services of the small runtime for Windows 2000 (wfcw2k.exe, in
// C): the ones of `ir::Service` that a console program needs. The entry table
// of src/win2000/wfcw2k.c fills the same numbers; the two lists change
// together (a test runs programs through the small runtime).

#include <string>
#include <vector>

#include "ir.hpp"

namespace wfc::compiler {

[[nodiscard]] constexpr bool small_runtime_supports(const int service) {
    return (service >= 0 && service <= 68) || service == 77 || service == 78 ||
           service == 79 || service == 90 || service == 80 || service == 81 ||
           service == 89 || service == 103 || service == 107 ||
           service == 108 || service == 112 || service == 113 ||
           (service >= 115 && service <= 117);
}

// A word for what a service without the small runtime belongs to.
[[nodiscard]] constexpr const char* small_runtime_feature(const int service) {
    if ((service >= 69 && service <= 76) || service == 88) {
        return "classes";
    }
    if (service == 77 || service == 78 || service == 79 || service == 85 ||
        service == 86 || service == 90) {
        return "Variant arrays and library functions";
    }
    if (service == 91 || service == 87) {
        return "library statements (files, Like, Rnd and others)";
    }
    if ((service >= 92 && service <= 102) || service == 104 || service == 105 ||
        service == 106 || service == 120 || service == 121) {
        return "forms, controls and events";
    }
    return "this feature";
}

// The library functions of service 77 that the small runtime has.
// `name` is "function/arguments" (`left/2`; a trailing `$` is not part of
// the name).
[[nodiscard]] inline bool small_runtime_supports_function(std::string name) {
    const auto slash = name.find('/');
    if (slash != std::string::npos && slash > 0 && name[slash - 1] == '$') {
        name.erase(slash - 1, 1);
    }
    if (name.rfind("choose/", 0) == 0) {
        return name != "choose/1";
    }
    static const char* const known[] = {
        "left/2",    "right/2", "mid/2",    "mid/3",   "instr/2", "strcomp/2",
        "replace/3", "space/1", "string/2", "iif/3",   "abs/1",   "int/1",
        "fix/1",     "sgn/1",   "sqr/1",    "cint/1",  "clng/1",  "cbyte/1",
        "cdbl/1",    "csng/1",  "cstr/1",   "cbool/1", "len/1",   "ucase/1",
        "lcase/1",   "trim/1",  "ltrim/1",  "rtrim/1"};
    for (const char* wanted : known) {
        if (name == wanted) {
            return true;
        }
    }
    return false;
}

// The first library function of the program that the small runtime lacks.
[[nodiscard]] inline std::string small_runtime_missing_function(
    const std::vector<std::string>& names) {
    for (const auto& name : names) {
        if (!small_runtime_supports_function(name)) {
            return name;
        }
    }
    return {};
}

// The first service of the program that the small runtime lacks, or -1.
[[nodiscard]] inline int small_runtime_missing(
    const std::vector<int>& services) {
    for (const int service : services) {
        if (!small_runtime_supports(service)) {
            return service;
        }
    }
    return -1;
}

}  // namespace wfc::compiler
