// Helpers shared by the OCX control libraries (wfc-comdlg32, wfc-mswinsck).
// Included once by forms.cpp before them, inside namespace wfc::detail. Internal to
// the WFC evaluator; not part of the public API.

// ----- Shared helpers ----------------------------------------------------------

namespace {

constexpr Integer kCancelled = 32755;        // "Cancel was selected"
constexpr Integer kNoFonts = 24574;          // "No fonts exist"
constexpr Integer kInvalidState = 40006;     // wrong protocol or state
constexpr Integer kInvalidOperation = 40020;  // invalid operation at this state

[[nodiscard]] std::string property_text(ControlObject& control,
                                        const char* name) {
    const auto found = control.values.find(name);
    return found == control.values.end() ? std::string{}
                                         : FormSupport::text_of(found->second);
}

[[nodiscard]] long property_whole(ControlObject& control, const char* name) {
    return static_cast<long>(FormSupport::number_of(control, name));
}

[[nodiscard]] bool property_flag(ControlObject& control, const char* name) {
    const auto found = control.values.find(name);
    return found != control.values.end() &&
           std::holds_alternative<bool>(found->second) &&
           std::get<bool>(found->second);
}

// A number as VB writes it: decimal, `&H1F` hexadecimal.
[[nodiscard]] long parse_number(std::string text) {
    while (!text.empty() && text.front() == ' ') {
        text.erase(0, 1);
    }
    if (text.size() > 2U && text[0] == '&' && (text[1] == 'h' || text[1] == 'H')) {
        return static_cast<long>(
            std::strtoul(text.c_str() + 2, nullptr, 16) & 0xFFFFFFFFUL);
    }
    return std::strtol(text.c_str(), nullptr, 10);
}

[[nodiscard]] ui::WindowId owner_window(ControlObject& control) {
    const auto form = control.form.lock();
    const ControlObject* owner = form != nullptr ? control_of(*form) : nullptr;
    return owner != nullptr ? owner->window : ui::kNoWindow;
}

}  // namespace
