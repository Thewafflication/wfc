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
    return (service >= 0 && service <= 47) || service == 80 || service == 81 ||
           service == 89 || service == 103 || service == 107 ||
           service == 108 || service == 112 || service == 113 ||
           (service >= 115 && service <= 117);
}

// A word for what a service without the small runtime belongs to.
[[nodiscard]] constexpr const char* small_runtime_feature(const int service) {
    if (service >= 48 && service <= 68) {
        return "Variants";
    }
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
