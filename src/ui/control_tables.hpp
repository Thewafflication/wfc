// Form and control member tables: the VB runtime classes generated from the
// VB6 type-library evidence (scripts/New-UiControlTables.ps1) and the lookups
// over them and over the tables of the control libraries (wfc-mscomctl,
// wfc-comdlg32, wfc-mswinsck). Internal to the WFC runtime; not part of the
// public API.

#ifndef WFC_UI_CONTROL_TABLES_HPP
#define WFC_UI_CONTROL_TABLES_HPP

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <wfc/ocx/control_class.hpp>

namespace wfc::ui {

struct ParameterInfo {
    std::string name;
    std::string type;
    bool optional{};
};

// The VB runtime classes (control_tables.cpp).
[[nodiscard]] std::span<const ControlClass> control_classes();
// The MSComctlLib controls (the wfc-mscomctl library).
[[nodiscard]] std::span<const ControlClass> comctl_classes();
// The MSComctlLib class named `name` (case-insensitive), or nullptr.
[[nodiscard]] const ControlClass* find_comctl_class(std::string_view name);

// The Common Dialog and Winsock controls (the wfc-comdlg32 and wfc-mswinsck
// libraries).
[[nodiscard]] std::span<const ControlClass> ocx_classes();
// The Common Dialog or Winsock class named `name`, or nullptr.
[[nodiscard]] const ControlClass* find_ocx_class(std::string_view name);

// The class named `name` (case-insensitive), or nullptr.
[[nodiscard]] const ControlClass* find_control_class(std::string_view name);

// The first row named `name` (case-insensitive) with the given kind, or
// nullptr. A property exposed through Get/Let/Set rows is found by any kind.
[[nodiscard]] const MemberRow* find_member(const ControlClass& control,
                                           std::string_view name, char kind);
[[nodiscard]] const MemberRow* find_event(const ControlClass& control,
                                          std::string_view name);

// The parameters of a member row.
[[nodiscard]] std::vector<ParameterInfo> parameters_of(const MemberRow& row);

}  // namespace wfc::ui

#endif
