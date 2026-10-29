// Lookup helpers over the generated control tables.
// Internal to the WFC runtime; not part of the public API.

#include <cctype>

#include "control_tables.hpp"

#include <wfc/comdlg32/comdlg32.hpp>
#include <wfc/mscomctl/mscomctl.hpp>
#include <wfc/mswinsck/mswinsck.hpp>

namespace wfc::ui {
namespace {

[[nodiscard]] bool same_name(const std::string_view left,
                             const std::string_view right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

}  // namespace

const ControlClass* find_control_class(const std::string_view name) {
    for (const auto& control : control_classes()) {
        if (same_name(control.name, name)) {
            return &control;
        }
    }
    return nullptr;
}

std::span<const ControlClass> comctl_classes() {
    return mscomctl_classes();
}

const ControlClass* find_comctl_class(const std::string_view name) {
    for (const auto& control : comctl_classes()) {
        if (same_name(control.name, name)) {
            return &control;
        }
    }
    return nullptr;
}

std::span<const ControlClass> ocx_classes() {
    static const std::vector<ControlClass> all = [] {
        std::vector<ControlClass> result;
        for (const auto& control : comdlg32_classes()) {
            result.push_back(control);
        }
        for (const auto& control : mswinsck_classes()) {
            result.push_back(control);
        }
        return result;
    }();
    return all;
}

const ControlClass* find_ocx_class(const std::string_view name) {
    for (const auto& control : ocx_classes()) {
        if (same_name(control.name, name)) {
            return &control;
        }
    }
    return nullptr;
}

const MemberRow* find_member(const ControlClass& control,
                             const std::string_view name, const char kind) {
    for (std::size_t i = 0; i < control.member_count; ++i) {
        const auto& row = control.members[i];
        if (same_name(row.name, name) && (kind == '*' || row.kind == kind)) {
            return &row;
        }
    }
    return nullptr;
}

const MemberRow* find_event(const ControlClass& control,
                            const std::string_view name) {
    for (std::size_t i = 0; i < control.event_count; ++i) {
        if (same_name(control.events[i].name, name)) {
            return &control.events[i];
        }
    }
    return nullptr;
}

std::vector<ParameterInfo> parameters_of(const MemberRow& row) {
    std::vector<ParameterInfo> result;
    const std::string_view text = row.parameters;
    std::size_t start = 0;
    while (start < text.size()) {
        auto end = text.find(';', start);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        const auto item = text.substr(start, end - start);
        ParameterInfo info;
        const auto first = item.find(':');
        info.name = std::string(item.substr(0, first));
        if (first != std::string_view::npos) {
            const auto second = item.find(':', first + 1U);
            info.type = std::string(
                item.substr(first + 1U, second == std::string_view::npos
                                            ? std::string_view::npos
                                            : second - first - 1U));
            info.optional = second != std::string_view::npos;
        }
        result.push_back(std::move(info));
        start = end + 1U;
    }
    return result;
}

}  // namespace wfc::ui
