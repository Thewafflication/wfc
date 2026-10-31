// The table types shared by the WFC control libraries (wfc-mscomctl,
// wfc-comdlg32, wfc-mswinsck) and the WFC runtime. This file is identical in
// every one of them; a WFC test fails when the copies differ. Internal to the
// WFC runtime; not part of the public API.

#ifndef WFC_OCX_CONTROL_CLASS_HPP
#define WFC_OCX_CONTROL_CLASS_HPP

#include <cstddef>

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

}  // namespace wfc::ui

#endif  // WFC_OCX_CONTROL_CLASS_HPP
