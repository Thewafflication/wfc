// Unit tests for the form and control layer: generated tables, the headless
// host, and the .frm parser.

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include <wfc/ocx/control_tables.hpp>
#include "frm_parser.hpp"
#include "headless_host.hpp"

namespace {

int failures = 0;

void expect(const bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

void test_tables() {
    using namespace wfc::ui;
    const auto* button = find_control_class("commandbutton");
    expect(button != nullptr, "CommandButton class exists");
    if (button == nullptr) {
        return;
    }
    const auto* caption = find_member(*button, "caption", 'l');
    expect(caption != nullptr && std::string(caption->type) == "String",
           "Caption has a String Property Let");
    expect(find_member(*button, "Caption", 'g') != nullptr,
           "Caption has a Property Get");
    expect(find_member(*button, "NoSuchMember", '*') == nullptr,
           "unknown member is absent");
    const auto* click = find_event(*button, "click");
    expect(click != nullptr && parameters_of(*click).empty(),
           "Click takes no parameters");
    const auto* key_down = find_event(*button, "KeyDown");
    expect(key_down != nullptr && parameters_of(*key_down).size() == 2U,
           "KeyDown takes KeyCode and Shift");
    const auto* form = find_control_class("Form");
    expect(form != nullptr && find_event(*form, "Load") != nullptr,
           "Form has a Load event");
    expect(form != nullptr && find_member(*form, "Caption", 'l') != nullptr,
           "Form has Caption");
    expect(control_classes().size() >= 25U, "all runtime classes present");
}

void test_headless_host() {
    using namespace wfc::ui;
    HeadlessHost host;
    const WindowId form = host.create_window("Form", kNoWindow);
    const WindowId button = host.create_window("CommandButton", form);
    expect(host.kind_of(button) == "CommandButton", "kind recorded");
    expect(host.parent_of(button) == form, "parent recorded");
    host.set_text(button, "OK");
    expect(host.text(button) == "OK", "text round trip");
    host.set_visible(form, true);
    expect(host.visible(form), "visible round trip");
    host.set_bounds(button, Rect{10, 20, 300, 400});
    expect(host.bounds(button).width == 300, "bounds round trip");
    host.set_focus(button);
    expect(host.focus() == button, "focus moves");

    host.post_event(UiEvent{button, "Click", {}});
    const auto click = host.next_event(false);
    expect(
        click.has_value() && click->name == "Click" && click->window == button,
        "posted event is delivered");
    expect(!host.next_event(false).has_value(), "queue drains");

    host.set_timer(form, 100);
    expect(!host.exhausted(), "a timer keeps the host alive");
    host.advance_time(250);
    int ticks = 0;
    while (const auto event = host.next_event(false)) {
        if (event->name == "Timer" && event->window == form) {
            ++ticks;
        }
    }
    expect(ticks == 2, "two timer intervals elapsed in 250 ms");
    host.kill_timer(form);
    expect(host.exhausted(), "idle once the timer is killed");

    host.post_event(UiEvent{button, "Click", {}});
    host.destroy_window(form);
    expect(host.kind_of(button).empty(),
           "children are destroyed with the form");
    expect(!host.next_event(false).has_value(),
           "events for destroyed windows are dropped");
    expect(host.focus() == kNoWindow, "focus clears when its window dies");
}

void test_frm_parser() {
    using namespace wfc::ui;
    const std::string text =
        "VERSION 5.00\r\n"
        "Begin VB.Form Form1 \r\n"
        "   Caption         =   \"He said \"\"hi\"\"\"\r\n"
        "   ClientHeight    =   3195\r\n"
        "   Icon            =   \"Form1.frx\":0000\r\n"
        "   Begin VB.CommandButton Command1 \r\n"
        "      Caption         =   \"OK\"\r\n"
        "      Default         =   -1  'True\r\n"
        "      Index           =   0\r\n"
        "      BeginProperty Font \r\n"
        "         Name            =   \"MS Sans Serif\"\r\n"
        "         Size            =   8.25\r\n"
        "      EndProperty\r\n"
        "   End\r\n"
        "   Begin VB.Frame Frame1 \r\n"
        "      Begin VB.TextBox Text1 \r\n"
        "         Text            =   \"x\"\r\n"
        "      End\r\n"
        "   End\r\n"
        "End\r\n"
        "Attribute VB_Name = \"Form1\"\r\n"
        "Attribute VB_PredeclaredId = True\r\n"
        "Option Explicit\r\n"
        "Private Sub Command1_Click(Index As Integer)\r\n"
        "End Sub\r\n";
    const auto form = parse_frm(text);
    expect(form.ok, "form parses: " + form.error);
    if (!form.ok) {
        return;
    }
    expect(form.version == "5.00", "version");
    expect(form.root.type == "VB.Form" && form.root.name == "Form1",
           "root object");
    expect(form.name == "Form1", "VB_Name");
    const auto* caption = form.root.find("caption");
    expect(caption != nullptr && caption->is_string &&
               caption->value == "He said \"hi\"",
           "doubled quotes collapse");
    const auto* height = form.root.find("ClientHeight");
    expect(height != nullptr && !height->is_string && height->value == "3195",
           "number value");
    const auto* icon = form.root.find("Icon");
    expect(icon != nullptr && icon->in_resource &&
               icon->resource_file == "Form1.frx" && icon->resource_offset == 0,
           "resource reference");
    expect(form.root.children.size() == 2U, "two children");
    const auto& button = form.root.children[0];
    expect(button.type == "VB.CommandButton" && button.name == "Command1",
           "control header");
    const auto* is_default = button.find("Default");
    expect(is_default != nullptr && is_default->value == "-1",
           "comment stripped from a boolean");
    const auto* font = button.find("Font");
    expect(font != nullptr && font->nested.size() == 2U &&
               font->nested[0].value == "MS Sans Serif",
           "BeginProperty group");
    const auto& frame = form.root.children[1];
    expect(frame.children.size() == 1U && frame.children[0].name == "Text1",
           "nested container");
    expect(form.code.find("Command1_Click") != std::string::npos,
           "code part kept");
    // Header lines are blank so line numbers match the file.
    std::size_t lines_before_option = 0;
    for (std::size_t i = 0; i < form.code.find("Option Explicit"); ++i) {
        if (form.code[i] == '\n') {
            ++lines_before_option;
        }
    }
    expect(lines_before_option == 22U, "code starts on its original line");

    expect(!parse_frm("Begin VB.Form F\n").ok, "unterminated form fails");
    expect(!parse_frm("End\n").ok, "stray End fails");
}

// Whether two parsed forms hold the same objects, properties and values.
bool same_property(const wfc::ui::FrmProperty& a,
                   const wfc::ui::FrmProperty& b) {
    if (a.name != b.name || a.value != b.value || a.is_string != b.is_string ||
        a.in_resource != b.in_resource || a.resource_file != b.resource_file ||
        a.resource_offset != b.resource_offset ||
        a.nested.size() != b.nested.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.nested.size(); ++i) {
        if (!same_property(a.nested[i], b.nested[i])) {
            return false;
        }
    }
    return true;
}

bool same_object(const wfc::ui::FrmObject& a, const wfc::ui::FrmObject& b) {
    if (a.type != b.type || a.name != b.name ||
        a.properties.size() != b.properties.size() ||
        a.children.size() != b.children.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.properties.size(); ++i) {
        if (!same_property(a.properties[i], b.properties[i])) {
            return false;
        }
    }
    for (std::size_t i = 0; i < a.children.size(); ++i) {
        if (!same_object(a.children[i], b.children[i])) {
            return false;
        }
    }
    return true;
}

// Parse, write, parse again: the forms are equivalent (REQ-0115).
void check_round_trip(const std::string& label, const std::string& text) {
    using namespace wfc::ui;
    const auto first = parse_frm(text);
    if (!first.ok) {
        return;  // malformed forms are covered elsewhere
    }
    const auto second = parse_frm(write_frm_header(first));
    expect(second.ok && second.version == first.version &&
               same_object(first.root, second.root),
           "round trip: " + label);
}

void test_frm_round_trip(const std::string& directory) {
    namespace fs = std::filesystem;
    std::size_t count = 0;
    std::error_code ignored;
    for (const auto& entry :
         fs::recursive_directory_iterator(directory, ignored)) {
        if (entry.path().extension() != ".frm") {
            continue;
        }
        std::ifstream stream(entry.path(), std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(stream)),
                               std::istreambuf_iterator<char>());
        check_round_trip(entry.path().filename().string(), text);
        ++count;
    }
    expect(count > 10U, "round trip covers the form corpus");
}

}  // namespace

int main(const int argument_count, const char* const arguments[]) {
    test_tables();
    test_headless_host();
    test_frm_parser();
    if (argument_count > 1) {
        test_frm_round_trip(arguments[1]);
    }
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "ui tests passed\n";
    return EXIT_SUCCESS;
}
