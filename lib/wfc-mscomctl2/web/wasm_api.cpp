// The member table of the library for a web page: the class and member names
// the table lists, read through a few C functions (WebAssembly demo).

#include <wfc/mscomctl2/mscomctl2.hpp>

#include <emscripten/emscripten.h>

namespace {

[[nodiscard]] const wfc::ui::ControlClass* find(const int index) {
    const auto classes = wfc::ui::mscomctl2_classes();
    return index >= 0 && static_cast<std::size_t>(index) < classes.size()
               ? &classes[static_cast<std::size_t>(index)]
               : nullptr;
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE const char* library_name() {
    return "wfc-mscomctl2";
}

EMSCRIPTEN_KEEPALIVE const char* library_version() {
    return WFC_WEB_VERSION;
}

EMSCRIPTEN_KEEPALIVE const char* library_target() {
    return WFC_WEB_TARGET;
}

EMSCRIPTEN_KEEPALIVE int class_count() {
    return static_cast<int>(wfc::ui::mscomctl2_classes().size());
}

EMSCRIPTEN_KEEPALIVE const char* class_name(const int index) {
    const auto* entry = find(index);
    return entry != nullptr ? entry->name : "";
}

EMSCRIPTEN_KEEPALIVE int member_count(const int index) {
    const auto* entry = find(index);
    return entry != nullptr ? static_cast<int>(entry->member_count) : 0;
}

EMSCRIPTEN_KEEPALIVE int event_count(const int index) {
    const auto* entry = find(index);
    return entry != nullptr ? static_cast<int>(entry->event_count) : 0;
}

EMSCRIPTEN_KEEPALIVE const char* member_name(const int index, const int member) {
    const auto* entry = find(index);
    return entry != nullptr && member >= 0 &&
                   static_cast<std::size_t>(member) < entry->member_count
               ? entry->members[member].name
               : "";
}

EMSCRIPTEN_KEEPALIVE const char* event_name(const int index, const int event) {
    const auto* entry = find(index);
    return entry != nullptr && event >= 0 &&
                   static_cast<std::size_t>(event) < entry->event_count
               ? entry->events[event].name
               : "";
}

}  // extern "C"
