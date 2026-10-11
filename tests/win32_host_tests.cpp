// Integration tests for the Win32 UiHost. They create real windows, so they
// report "skipped" (exit code 77) when the process has no usable desktop.

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdlib>
#include <iostream>
#include <string>

#include "win32_host.hpp"

namespace {

int failures = 0;

void expect(const bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

// The next event with the given name, pumping messages for up to two
// seconds.
bool wait_for(wfc::ui::Win32Host& host, const std::string& name,
              wfc::ui::UiEvent& found) {
    const DWORD deadline = GetTickCount() + 2000U;
    while (GetTickCount() < deadline) {
        if (const auto event = host.next_event(false)) {
            if (event->name == name) {
                found = *event;
                return true;
            }
            continue;
        }
        Sleep(10);
    }
    return false;
}

}  // namespace

int main() {
    using namespace wfc::ui;
    Win32Host host;
    if (!host.usable()) {
        std::cout << "no usable desktop; skipped\n";
        return 77;
    }
    const WindowId form = host.create_window("Form", kNoWindow);
    const WindowId button = host.create_window("CommandButton", form);
    const WindowId edit = host.create_window("TextBox", form);
    const WindowId list = host.create_window("ListBox", form);
    const WindowId check = host.create_window("CheckBox", form);
    if (host.handle_of(form) == nullptr || host.handle_of(button) == nullptr) {
        std::cout << "cannot create windows; skipped\n";
        return 77;
    }
    host.set_bounds(form, Rect{0, 0, 6000, 4000});
    host.set_bounds(button, Rect{100, 100, 1200, 400});
    host.set_bounds(edit, Rect{100, 600, 2000, 300});
    host.set_bounds(list, Rect{100, 1000, 2000, 1000});
    host.set_bounds(check, Rect{100, 2200, 1500, 300});
    host.set_text(button, "Go");
    host.set_visible(form, true);
    host.set_visible(button, true);
    host.set_visible(edit, true);
    host.set_visible(list, true);
    host.set_visible(check, true);
    expect(host.text(button) == "Go", "caption round trip");
    expect(host.bounds(button).width == 1200, "bounds round trip");

    UiEvent event;
    SendMessageW(host.handle_of(button), BM_CLICK, 0, 0);
    expect(wait_for(host, "Click", event) && event.window == button,
           "button click becomes a Click event");

    host.set_text(edit, "hello");
    expect(host.text(edit) == "hello", "edit text round trip");
    expect(wait_for(host, "Change", event) && event.window == edit,
           "edit change becomes a Change event");

    host.set_items(list, {"a", "b", "c"});
    host.set_selection(list, 1);
    expect(host.selection(list) == 1, "list selection round trip");

    host.set_value(check, 1);
    expect(
        SendMessageW(host.handle_of(check), BM_GETCHECK, 0, 0) == BST_CHECKED,
        "check state is pushed to the control");

    host.set_timer(button, 20);
    host.kill_timer(button);

    const WindowId timer = host.create_window("Timer", form);
    host.set_timer(timer, 30);
    expect(wait_for(host, "Timer", event) && event.window == timer,
           "timer fires");
    host.kill_timer(timer);

    const WindowId file = host.create_window("Menu", form);
    const WindowId item = host.create_window("Menu", file);
    host.set_text(file, "&File");
    host.set_text(item, "&Open");
    host.set_visible(file, true);
    host.set_visible(item, true);
    expect(GetMenu(host.handle_of(form)) != nullptr, "menu bar exists");

    PostMessageW(host.handle_of(form), WM_CLOSE, 0, 0);
    expect(wait_for(host, "Close", event) && event.window == form,
           "closing the window becomes a Close event");

    host.destroy_window(form);
    expect(host.exhausted(), "no windows left");
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "win32 host tests passed\n";
    return EXIT_SUCCESS;
}

#else

int main() {
    return 77;
}

#endif
