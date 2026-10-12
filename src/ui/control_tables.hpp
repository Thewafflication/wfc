// Form and control member tables generated from the VB6 type-library
// evidence (scripts/New-UiControlTables.ps1). Internal to the WFC runtime;
// not part of the public API.

#ifndef WFC_UI_CONTROL_TABLES_HPP
#define WFC_UI_CONTROL_TABLES_HPP

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace wfc::ui {

// One property accessor, method, or event of a control class.
struct MemberRow {
    const char* name;
    // 'g' Property Get, 'l' Property Let, 's' Property Set, 'm' method/event.
    char kind;
    // Return type name as the type library spells it ("String", "Single").
    const char* type;
    long id;
    // "Name:Type[:o];..." -- ":o" marks an optional parameter.
    const char* parameters;
};

struct ControlClass {
    const char* name;
    const MemberRow* members;
    std::size_t member_count;
    const MemberRow* events;
    std::size_t event_count;
};

struct ParameterInfo {
    std::string name;
    std::string type;
    bool optional{};
};

[[nodiscard]] std::span<const ControlClass> control_classes();
// The MSComctlLib controls (generated from the mscomctl type-library evidence).
[[nodiscard]] std::span<const ControlClass> comctl_classes();
// The MSComctlLib class named `name` (case-insensitive), or nullptr.
[[nodiscard]] const ControlClass* find_comctl_class(std::string_view name);

// The Common Dialog and Winsock controls (ocx_tables.cpp).
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
