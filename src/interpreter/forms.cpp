// Interpreter: forms and intrinsic controls (REQ-0289). A form is a class
// module with a predeclared instance whose controls are fields; controls are
// native objects whose properties come from the generated type-library
// tables. Windows live in a UiHost (headless by default). Internal to the WFC
// evaluator; not part of the public API.

#include "forms.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

#include "../ui/control_tables.hpp"
#include "pictures.hpp"
#include "winsock_io.hpp"
#ifdef _WIN32
#include "../ui/win32_host.hpp"
#endif

namespace wfc::detail {

struct ComctlState;
struct ComctlSupport;
// The lower-case names of the MSComctlLib classes and item types (comctl.inc).
[[nodiscard]] std::vector<std::string> comctl_type_names();
// The merged table (type library members and container extender) of the
// MSComctlLib class named `leaf`, or nullptr (comctl.inc).
[[nodiscard]] const ui::ControlClass* comctl_class(std::string_view leaf);
// Defaults of the MSComctlLib control properties (comctl.inc).
[[nodiscard]] std::optional<Value> comctl_default(const std::string& class_name,
                                                  const std::string& property);
// Polls the Winsock controls for connections and data (ocx.inc).
[[nodiscard]] bool ocx_poll_sockets(Interpreter& in);
[[nodiscard]] bool ocx_has_sockets();
// The control class of `library`.`leaf` among the MSComctlLib, MSComDlg and
// MSWinsockLib controls, or nullptr (comctl.inc).
[[nodiscard]] const ui::ControlClass* library_control_class(
    std::string_view library, std::string_view leaf);

namespace {

[[nodiscard]] std::string lower_text(const std::string_view text) {
    std::string result;
    for (const char c : text) {
        result.push_back(ascii_lower(c));
    }
    return result;
}

// The zero value of a type-library type name.
[[nodiscard]] Value default_for_type(const std::string& type) {
    if (type == "String") {
        return Value{std::string{}};
    }
    if (type == "Integer") {
        return Value{Int16{0}};
    }
    if (type == "Long") {
        return Value{Integer{0}};
    }
    if (type == "Single") {
        return Value{0.0F};
    }
    if (type == "Double") {
        return Value{0.0};
    }
    if (type == "Boolean") {
        return Value{false};
    }
    if (type == "Variant") {
        return Value{Empty{}};
    }
    return Value{Nothing{}};
}

// Design-time defaults that differ from the zero value of the type; a
// property a .frm omits keeps its default.
// A directory path as VB shows it: backslashes, no trailing separator
// except on a root.
[[nodiscard]] std::string directory_text(const std::filesystem::path& path) {
    std::error_code ignored;
    auto text = std::filesystem::absolute(path, ignored)
                    .lexically_normal()
                    .make_preferred()
                    .string();
    while (text.size() > 3U && (text.back() == '\\' || text.back() == '/')) {
        text.pop_back();
    }
    return text;
}

[[nodiscard]] std::string current_drive_text() {
    std::error_code ignored;
    const auto root = std::filesystem::current_path(ignored).root_name();
    std::string text = root.string();
    for (char& c : text) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text.empty() ? std::string{"c:"} : text;
}

[[nodiscard]] bool wildcard_match(const std::string& pattern,
                                  const std::string& name) {
    const auto lower = [](const char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    };
    std::size_t p = 0;
    std::size_t n = 0;
    std::size_t star = std::string::npos;
    std::size_t mark = 0;
    while (n < name.size()) {
        if (p < pattern.size() &&
            (pattern[p] == '?' || lower(pattern[p]) == lower(name[n]))) {
            ++p;
            ++n;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            mark = n;
        } else if (star != std::string::npos) {
            p = star + 1U;
            n = ++mark;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }
    return p == pattern.size();
}

[[nodiscard]] std::optional<Value> property_default(
    const std::string& class_name, const std::string& property) {
    if (const auto common_control = comctl_default(class_name, property)) {
        return common_control;
    }
    if ((class_name == "filelistbox" || class_name == "dirlistbox") &&
        property == "path") {
        return Value{directory_text(std::filesystem::current_path())};
    }
    if (class_name == "filelistbox" && property == "pattern") {
        return Value{std::string{"*.*"}};
    }
    if (class_name == "drivelistbox" && property == "drive") {
        return Value{current_drive_text()};
    }
    static const std::map<std::string, Value> common = {
        {"visible", Value{true}},
        {"enabled", Value{true}},
        {"tabstop", Value{true}},
        {"causesvalidation", Value{true}},
        {"forecolor", Value{Integer{-2147483630}}},
        {"backcolor", Value{Integer{-2147483633}}},
        {"drawwidth", Value{Int16{1}}},
        {"scalemode", Value{Int16{1}}},
        {"fontsize", Value{8.25F}},
        {"fontname", Value{std::string{"MS Sans Serif"}}},
        {"appearance", Value{Int16{1}}},
        {"fillstyle", Value{Int16{1}}},  // transparent
    };
    if (class_name == "textbox" && property == "backcolor") {
        return Value{Integer{-2147483643}};
    }
    if (class_name == "textbox" && property == "borderstyle") {
        return Value{Int16{1}};
    }
    if (class_name == "form" && property == "borderstyle") {
        return Value{Int16{2}};
    }
    if ((class_name == "hscrollbar" || class_name == "vscrollbar") &&
        property == "max") {
        return Value{Int16{32767}};
    }
    if ((class_name == "hscrollbar" || class_name == "vscrollbar") &&
        (property == "smallchange" || property == "largechange")) {
        return Value{Int16{1}};
    }
    if (class_name == "form" && property == "visible") {
        return Value{false};
    }
    if (class_name == "form" || class_name == "mdiform") {
        if (property == "width") {
            return Value{4800.0F};
        }
        if (property == "height") {
            return Value{3600.0F};
        }
        if (property == "clientwidth") {
            return Value{4680.0F};
        }
        if (property == "clientheight") {
            return Value{3195.0F};
        }
    }
    const auto found = common.find(property);
    if (found != common.end()) {
        return found->second;
    }
    return std::nullopt;
}

// The property a bare reference to the object stands for.
[[nodiscard]] std::string default_property_of(const std::string& class_name) {
    if (class_name == "textbox" || class_name == "combobox" ||
        class_name == "listbox") {
        return "text";
    }
    if (class_name == "label" || class_name == "frame") {
        return "caption";
    }
    if (class_name == "commandbutton" || class_name == "checkbox" ||
        class_name == "optionbutton" || class_name == "hscrollbar" ||
        class_name == "vscrollbar" || class_name == "progressbar" ||
        class_name == "slider") {
        return "value";
    }
    if (class_name == "imagecombo") {
        return "text";
    }
    if (class_name == "picturebox" || class_name == "image") {
        return "picture";
    }
    if (class_name == "drivelistbox") {
        return "drive";
    }
    if (class_name == "dirlistbox") {
        return "path";
    }
    if (class_name == "filelistbox") {
        return "filename";
    }
    return {};
}

constexpr Integer kErrorAlreadyReported = -1;

// ----- ActiveX controls (the extender the container adds) -------------------

const ui::MemberRow kExtenderMembers[] = {
    {"Left", 'g', "Single", 1, ""},
    {"Left", 'l', "Single", 1, ""},
    {"Top", 'g', "Single", 2, ""},
    {"Top", 'l', "Single", 2, ""},
    {"Width", 'g', "Single", 3, ""},
    {"Width", 'l', "Single", 3, ""},
    {"Height", 'g', "Single", 4, ""},
    {"Height", 'l', "Single", 4, ""},
    {"Visible", 'g', "Boolean", 5, ""},
    {"Visible", 'l', "Boolean", 5, ""},
    {"TabIndex", 'g', "Integer", 6, ""},
    {"TabIndex", 'l', "Integer", 6, ""},
    {"TabStop", 'g', "Boolean", 7, ""},
    {"TabStop", 'l', "Boolean", 7, ""},
    {"Tag", 'g', "String", 8, ""},
    {"Tag", 'l', "String", 8, ""},
    {"ToolTipText", 'g', "String", 9, ""},
    {"ToolTipText", 'l', "String", 9, ""},
    {"HelpContextID", 'g', "Long", 10, ""},
    {"HelpContextID", 'l', "Long", 10, ""},
    {"CausesValidation", 'g', "Boolean", 11, ""},
    {"CausesValidation", 'l', "Boolean", 11, ""},
    {"DragMode", 'g', "Integer", 12, ""},
    {"DragMode", 'l', "Integer", 12, ""},
    {"Name", 'g', "String", 13, ""},
    {"Parent", 'g', "Object", 14, ""},
    {"Container", 'g', "Object", 15, ""},
    {"Object", 'g', "Object", 16, ""},
    {"SetFocus", 'm', "Void", 17, ""},
    {"ZOrder", 'm', "Void", 18, "Position:Variant:o"},
    {"Move", 'm', "Void", 19,
     "Left:Single;Top:Variant:o;Width:Variant:o;Height:Variant:o"},
    {"Drag", 'm', "Void", 20, "Action:Variant:o"},
};
const ui::MemberRow kExtenderEvents[] = {
    {"GotFocus", 'm', "HRESULT", 1, ""},
    {"LostFocus", 'm', "HRESULT", 2, ""},
    {"DragDrop", 'm', "HRESULT", 3, "Source:Control;X:Single;Y:Single"},
    {"DragOver", 'm', "HRESULT", 4,
     "Source:Control;X:Single;Y:Single;State:Integer"},
    {"Validate", 'm', "HRESULT", 5, "Cancel:Boolean"},
};
const ui::ControlClass kExtenderClass = {
    "ActiveXControl", kExtenderMembers,
    sizeof(kExtenderMembers) / sizeof(kExtenderMembers[0]), kExtenderEvents,
    sizeof(kExtenderEvents) / sizeof(kExtenderEvents[0])};

// A form or an intrinsic control.
class ControlObject final : public NativeObject {
public:
    ControlObject(const ui::ControlClass* control_class,
                  std::string object_name, const long array_index,
                  const bool form_object)
        : cls(control_class),
          name(lower_text(object_name)),
          display_name(object_name),
          index(array_index),
          is_form(form_object),
          class_key(lower_text(control_class->name)) {}

    [[nodiscard]] std::string type_name() const override {
        if (!type_label.empty()) {
            return type_label;
        }
        return is_form ? "Form" : std::string(cls->name);
    }

    [[nodiscard]] bool has_member(const std::string& member) const override {
        return member == "name" || member == "index" ||
               ui::find_member(*cls, member, '*') != nullptr;
    }

    [[nodiscard]] bool get(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) override;
    [[nodiscard]] bool put(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments,
                           const Value& value, NativeFailure& failure) override;

    const ui::ControlClass* cls;
    std::string name;  // lower-cased: the key of the form's field
    std::string display_name;
    long index;
    bool is_form;
    std::string class_key;
    std::weak_ptr<InstanceData> form;  // the owning form (itself for a form)
    std::weak_ptr<InstanceData> self;  // the instance this object belongs to
    std::string container;             // enclosing frame/picture box, or empty
    ui::WindowId window{ui::kNoWindow};
    std::map<std::string, Value> values;
    std::map<std::string, Value> design;  // values at load time
    // Form state.
    bool loaded{false};
    bool shown{false};
    bool unloading{false};
    bool activated{false};
    // Control arrays: the element objects by index.
    std::map<long, Value> elements;
    // ListBox and ComboBox items (REQ-0289).
    std::vector<std::string> items;
    // What a directory list box shows (indented names); empty: `items`.
    std::vector<std::string> display;
    std::vector<long> item_data;
    std::vector<bool> chosen;  // MultiSelect selection flags
    long list_index{-1};
    long new_index{-1};
    long order{0};          // creation order (the default TabIndex)
    bool dynamic{false};    // loaded at run time (Load Control(i))
    bool bind_busy{false};  // a bound control is being refreshed
    // An ActiveX control: its class id and the COM object the site created.
    std::string activex_clsid;
    // An MSComctlLib control: its collections and items (comctl.inc).
    std::shared_ptr<ComctlState> comctl;
    // An in-project UserControl hosted by a form: its class name; and, on the
    // UserControl itself, the hosting form's extender object.
    std::string usercontrol_class;
    std::string type_label;  // TypeName of a hosted control (its own class)
    Value extender{Nothing{}};
    Value inner{Nothing{}};
    ui::WindowId embedded{ui::kNoWindow};  // the window of an OLE object
    [[nodiscard]] bool is_activex() const {
        return class_key == "activexcontrol";
    }
    [[nodiscard]] bool is_user_control_host() const {
        return is_activex() && !usercontrol_class.empty();
    }
    [[nodiscard]] bool is_list() const {
        return class_key == "listbox" || class_key == "combobox" ||
               is_file_list();
    }
    [[nodiscard]] bool is_file_list() const {
        return class_key == "filelistbox" || class_key == "dirlistbox" ||
               class_key == "drivelistbox";
    }
    // Whether the control can hold the keyboard focus.
    [[nodiscard]] bool focusable() const {
        return class_key != "label" && class_key != "timer" &&
               class_key != "frame" && class_key != "shape" &&
               class_key != "line" && class_key != "image" &&
               class_key != "menu";
    }
    [[nodiscard]] bool is_scroll() const {
        return class_key == "hscrollbar" || class_key == "vscrollbar";
    }
};

// A control array: `Command1(0)`, `Command1.Count`.
class ControlArray final : public NativeObject {
public:
    [[nodiscard]] std::string type_name() const override { return "Object"; }
    [[nodiscard]] bool has_member(const std::string& member) const override {
        return member == "count" || member == "lbound" || member == "ubound" ||
               member == "item" || member == "_default";
    }
    [[nodiscard]] bool get(Interpreter&, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) override {
        if (member == "count") {
            result = Value{static_cast<Integer>(elements.size())};
            return true;
        }
        if (member == "lbound" || member == "ubound") {
            long value = 0;
            if (!elements.empty()) {
                value = member == "lbound" ? elements.begin()->first
                                           : elements.rbegin()->first;
            }
            result = Value{static_cast<Integer>(value)};
            return true;
        }
        if (member == "item" || member == "_default") {
            if (arguments.size() != 1U) {
                failure = NativeFailure{450, "Wrong number of arguments"};
                return true;
            }
            const auto wanted = whole_value(arguments[0].value);
            if (!wanted.has_value()) {
                failure = NativeFailure{13, "Type mismatch"};
                return true;
            }
            const auto found = elements.find(*wanted);
            if (found == elements.end()) {
                failure = NativeFailure{340, "Control array element '" +
                                                 std::to_string(*wanted) +
                                                 "' doesn't exist"};
                return true;
            }
            result = found->second;
            return true;
        }
        return false;
    }
    [[nodiscard]] bool put(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments,
                           const Value& value,
                           NativeFailure& failure) override {
        if ((member != "item" && member != "_default") ||
            arguments.size() != 1U) {
            return false;
        }
        // `Text1(i) = x`: the default property of the element.
        const auto wanted = whole_value(arguments[0].value);
        const auto found =
            wanted.has_value() ? elements.find(*wanted) : elements.end();
        if (found == elements.end()) {
            failure = NativeFailure{340, "Control array element doesn't exist"};
            return true;
        }
        const auto* holder = std::get_if<ObjectInstance>(&found->second);
        if (holder == nullptr || holder->data == nullptr ||
            holder->data->native == nullptr) {
            return false;
        }
        std::vector<CallArgument> none;
        return holder->data->native->put(in, "_default", none, value, failure);
    }
    std::map<long, Value> elements;
};

[[nodiscard]] ControlObject* control_of(InstanceData& instance) {
    return dynamic_cast<ControlObject*>(instance.native.get());
}

[[nodiscard]] ControlObject* control_of(const Value& value) {
    const auto* holder = std::get_if<ObjectInstance>(&value);
    if (holder == nullptr || holder->data == nullptr) {
        return nullptr;
    }
    return control_of(*holder->data);
}

// Converts `value` to the type-library type of a property.
[[nodiscard]] bool coerce_property(Interpreter& in, Value& value,
                                   const std::string& type,
                                   NativeFailure& failure);

}  // namespace

struct ComctlSupport {
    static void init(ControlObject& control);
    // Marks changes made while handling host input (not echoed back).
    static void from_host(ControlObject& control, bool on);
    [[nodiscard]] static bool get(Interpreter& in, ControlObject& self,
                                  const std::string& member,
                                  std::vector<CallArgument>& arguments,
                                  Value& result, NativeFailure& failure);
    [[nodiscard]] static bool put(Interpreter& in, ControlObject& self,
                                  const std::string& member,
                                  std::vector<CallArgument>& arguments,
                                  const Value& value, NativeFailure& failure);
    // A scripted user action (`ui <target> <action> ...`).
    [[nodiscard]] static bool user_input(Interpreter& in, InstanceData& form,
                                         ControlObject& self,
                                         const std::string& action,
                                         const std::string& rest);
};

// Pushes the state of a common control to the host (comctl.inc).
void comctl_push(Interpreter& in, ControlObject& control);

// HKEY_CURRENT_USER string values (com_automation.cpp).
bool wfc_registry_write(const std::string& key, const std::string& subkey,
                        const std::string& name, const std::string& data);
std::optional<std::string> wfc_registry_read(const std::string& key,
                                             const std::string& subkey,
                                             const std::string& name);

// The diagnostic of a persisted control state that is not read.
constexpr const char* kUnreadableStateCode = "WFC0293";

// Reads the BeginProperty groups a form file holds for a common control
// (comctl.inc).
void comctl_load_persisted(Interpreter& in, ControlObject& control,
                           const ui::FrmObject& source,
                           const std::string& frx_directory);

// Everything that needs Interpreter internals; a friend of Interpreter.
struct FormSupport {
    [[nodiscard]] static std::string text_of(const Value& value) {
        return Interpreter::render(value);
    }
    [[nodiscard]] static std::size_t offset_of(Interpreter& in) {
        return in.offset_;
    }
    // The next scripted answer for a dialog of `kind` (open, save, color...).
    [[nodiscard]] static std::optional<std::string> dialog_answer(
        Interpreter& in, const std::string& kind) {
        return in.form_dialog_answer(kind);
    }
    // A load-time warning goes to the Immediate output.
    static void warn(Interpreter& in, const std::string& text) {
        in.debug_output_ += text + "\n";
    }

    static FormRuntime& runtime(Interpreter& in) {
        if (!in.forms_) {
            in.forms_ = std::make_shared<FormRuntime>();
#ifdef _WIN32
            if (in.ui_mode_ == "win32") {
                auto native = std::make_unique<ui::Win32Host>();
                if (native->usable()) {
                    in.forms_->host = std::move(native);
                    return *in.forms_;
                }
            }
#endif
            auto host = std::make_unique<ui::HeadlessHost>();
            in.forms_->headless = host.get();
            in.forms_->host = std::move(host);
        }
        return *in.forms_;
    }

    [[nodiscard]] static Value control_value(InstanceData& form,
                                             const ControlObject& object);
    // Whether `member` of an ActiveX control belongs to the control itself
    // rather than to the container's extender.
    [[nodiscard]] static bool activex_forwards(const ControlObject& self,
                                               const std::string& member) {
        if (member == "name" || member == "index" || member == "object") {
            return false;
        }
        if (member == "_default") {
            return true;
        }
        return ui::find_member(*self.cls, member, 'g') == nullptr &&
               ui::find_member(*self.cls, member, 'l') == nullptr &&
               ui::find_member(*self.cls, member, 's') == nullptr &&
               ui::find_member(*self.cls, member, 'm') == nullptr;
    }

    [[nodiscard]] static bool activex_get(Interpreter& in, ControlObject& self,
                                          const std::string& member,
                                          std::vector<CallArgument>& arguments,
                                          Value& result,
                                          NativeFailure& failure) {
        const auto* held = std::get_if<ObjectInstance>(&self.inner);
        if (held == nullptr || held->data == nullptr) {
            failure = NativeFailure{
                91, "Object variable or With block variable not set"};
            return true;
        }
        const auto keep = held->data;
        if (keep->com == nullptr) {
            // An in-project UserControl: its public members by name.
            std::vector<Value> call{
                self.inner,
                Value{member == "_default" ? std::string{} : member},
                Value{Integer{2}}};
            for (const auto& argument : arguments) {
                call.push_back(argument.value);
            }
            auto value =
                in.evaluate_misc_function("callbyname", call, in.offset_);
            if (!value.has_value()) {
                failure = reported();
                return true;
            }
            result = std::move(*value);
            return true;
        }
        auto value = in.com_get_member_with(
            *keep, member == "_default" ? std::string{} : member, arguments,
            in.offset_);
        if (!value.has_value()) {
            failure = reported();
            return true;
        }
        result = std::move(*value);
        return true;
    }

    [[nodiscard]] static bool activex_put(Interpreter& in, ControlObject& self,
                                          const std::string& member,
                                          std::vector<CallArgument>& arguments,
                                          const Value& value,
                                          NativeFailure& failure) {
        const auto* held = std::get_if<ObjectInstance>(&self.inner);
        if (held == nullptr || held->data == nullptr) {
            failure = NativeFailure{
                91, "Object variable or With block variable not set"};
            return true;
        }
        const auto keep = held->data;
        if (keep->com == nullptr) {
            const bool object = std::holds_alternative<ObjectInstance>(value);
            std::vector<Value> call{
                self.inner,
                Value{member == "_default" ? std::string{} : member},
                Value{Integer{object ? 8 : 4}}};
            for (const auto& argument : arguments) {
                call.push_back(argument.value);
            }
            call.push_back(value);
            if (!in.evaluate_misc_function("callbyname", call, in.offset_)
                     .has_value()) {
                failure = reported();
            }
            return true;
        }
        if (!in.com_put_member(*keep,
                               member == "_default" ? std::string{} : member,
                               arguments, value, in.offset_)) {
            failure = reported();
        }
        return true;
    }

    // The members of the OLE container control: an embedded object is created
    // from its class (a ProgID or CLSID) and reached through `Object`.
    [[nodiscard]] static bool ole_member(Interpreter& in, ControlObject& self,
                                         const std::string& member,
                                         std::vector<CallArgument>& arguments,
                                         Value& result,
                                         NativeFailure& failure) {
        const bool present = std::holds_alternative<ObjectInstance>(self.inner);
        if (member == "object") {
            result = self.inner;
            return true;
        }
        if (member == "appisrunning") {
            result = Value{present};
            return true;
        }
        if (member == "oletype") {
            result = Value{Int16{static_cast<std::int16_t>(present ? 1 : 3)}};
            return true;
        }
        if (member == "createembed") {
            std::string text;
            if (arguments.size() >= 2U &&
                std::holds_alternative<std::string>(arguments[1].value)) {
                text = std::get<std::string>(arguments[1].value);
            } else {
                const auto known = self.values.find("class");
                if (known != self.values.end() &&
                    std::holds_alternative<std::string>(known->second)) {
                    text = std::get<std::string>(known->second);
                }
            }
            const std::string clsid = in.com_progid_to_clsid(text);
            if (clsid.empty()) {
                failure = NativeFailure{
                    31018, "Class is not set"};  // the OLE container's error
                return true;
            }
            auto& host = *runtime(in).host;
            if (self.embedded != ui::kNoWindow) {
                host.destroy_window(self.embedded);
            }
            self.embedded = host.create_window("ActiveX:" + clsid, self.window);
            void* unknown = host.activex_object(self.embedded);
            if (unknown == nullptr) {
                host.destroy_window(self.embedded);
                self.embedded = ui::kNoWindow;
                failure =
                    NativeFailure{429, "ActiveX component can't create object"};
                return true;
            }
            self.inner = in.com_wrap_unknown(unknown);
            self.values["class"] = Value{text};
            result = Value{Empty{}};
            return true;
        }
        if (member == "close" || member == "delete") {
            if (self.embedded != ui::kNoWindow) {
                runtime(in).host->destroy_window(self.embedded);
                self.embedded = ui::kNoWindow;
            }
            self.inner = Value{Nothing{}};
            result = Value{Empty{}};
            return true;
        }
        if (member == "doverb" || member == "update" || member == "copy" ||
            member == "paste" || member == "fetchverbs") {
            result = Value{Empty{}};  // there is no server UI to drive
            return true;
        }
        return false;
    }

    // Creates the UserControl a form hosts: its own constituent controls in a
    // child window, the Initialize / ReadProperties / Resize events, and the
    // event link to the form.
    [[nodiscard]] static bool attach_user_control(Interpreter& in,
                                                  InstanceData& form,
                                                  ControlObject& form_object,
                                                  ControlObject& host) {
        auto made = in.instantiate_class(host.usercontrol_class, in.offset_);
        if (!made.has_value()) {
            return false;
        }
        const auto* held = std::get_if<ObjectInstance>(&*made);
        ControlObject* control = held != nullptr && held->data != nullptr
                                     ? control_of(*held->data)
                                     : nullptr;
        if (control == nullptr || control->class_key != "usercontrol") {
            static_cast<void>(in.raise_runtime(
                429, "ActiveX component can't create object: " + host.name,
                in.offset_));
            return false;
        }
        const auto uc = held->data;
        host.inner = *made;
        control->extender = control_value(form, host);
        control->loaded = true;
        for (const char* key : {"width", "height"}) {
            const auto found = host.values.find(key);
            if (found != host.values.end()) {
                control->values[key] = found->second;
            }
        }
        if (!create_windows(in, *uc, *control,
                            parent_window(form, form_object, host))) {
            return false;
        }
        host.window = control->window;
        mirror_all(in, host);
        uc->event_sinks.emplace_back(form.weak_from_this(), host.name);
        // ReadProperties: the properties the form file holds for the control.
        if (in.class_definitions_.contains("wfcpropertybag")) {
            auto bag = in.instantiate_class("wfcpropertybag", in.offset_);
            if (!bag.has_value()) {
                return false;
            }
            for (const auto& [key, value] : host.design) {
                if (key.empty() || key.front() == '_' ||
                    !activex_forwards(host, key)) {
                    continue;
                }
                std::vector<Value> call{*bag,
                                        Value{std::string{"WriteProperty"}},
                                        Value{Integer{1}}, Value{key}, value};
                if (!in.evaluate_misc_function("callbyname", call, in.offset_)
                         .has_value()) {
                    return false;
                }
            }
            std::vector<Value> arguments{*bag};
            if (!raise(in, *uc, "", -1, "readproperties", arguments,
                       in.offset_)) {
                return false;
            }
        }
        std::vector<Value> none;
        return raise(in, *uc, "", -1, "resize", none, in.offset_) &&
               raise(in, *uc, "", -1, "show", none, in.offset_);
    }

    // Unloads the UserControls a form hosts (Terminate, bindings).
    static void terminate_user_controls(Interpreter& in, InstanceData& form) {
        auto& state = runtime(in);
        std::vector<std::shared_ptr<InstanceData>> hosted;
        for_each_control(form, [&](ControlObject& item) {
            if (item.is_user_control_host()) {
                if (const auto* held =
                        std::get_if<ObjectInstance>(&item.inner)) {
                    hosted.push_back(held->data);
                }
                item.inner = Value{Nothing{}};
            }
        });
        for (const auto& uc : hosted) {
            std::vector<Value> none;
            static_cast<void>(
                raise(in, *uc, "", -1, "terminate", none, in.offset_));
            std::erase_if(state.bindings, [&](const auto& entry) {
                return entry.second.form.lock() == uc;
            });
            if (ControlObject* control = control_of(*uc)) {
                control->loaded = false;
                control->window = ui::kNoWindow;
            }
        }
    }

    // A hosting extender changed size: the UserControl follows.
    [[nodiscard]] static bool resize_user_control(Interpreter& in,
                                                  ControlObject& host) {
        const auto* held = std::get_if<ObjectInstance>(&host.inner);
        ControlObject* control = held != nullptr && held->data != nullptr
                                     ? control_of(*held->data)
                                     : nullptr;
        if (control == nullptr) {
            return true;
        }
        for (const char* key : {"width", "height"}) {
            const auto found = host.values.find(key);
            if (found != host.values.end()) {
                control->values[key] = found->second;
            }
        }
        std::vector<Value> none;
        return raise(in, *held->data, "", -1, "resize", none, in.offset_);
    }

    // Creates the control a site hosts and wires its events and design-time
    // properties; false (with an error raised) when it cannot be created.
    [[nodiscard]] static bool attach_activex(Interpreter& in,
                                             InstanceData& form,
                                             ControlObject& control) {
        void* unknown = runtime(in).host->activex_object(control.window);
        if (unknown == nullptr) {
            static_cast<void>(in.raise_runtime(
                429, "ActiveX component can't create object: " + control.name,
                in.offset_));
            return false;
        }
        control.inner = in.com_wrap_unknown(unknown);  // takes the reference
        const auto* held = std::get_if<ObjectInstance>(&control.inner);
        if (held == nullptr || held->data == nullptr) {
            static_cast<void>(in.raise_runtime(
                429, "ActiveX component can't create object: " + control.name,
                in.offset_));
            return false;
        }
        // Events reach `Name_Event` handlers of the form.
        held->data->event_sinks.emplace_back(form.weak_from_this(),
                                             control.name);
        static_cast<void>(in.com_connect_events(*held->data));
        // Design-time properties the extender does not own go to the control.
        for (const auto& [key, value] : control.design) {
            if (key.empty() || key.front() == '_' ||
                !activex_forwards(control, key)) {
                continue;
            }
            std::vector<CallArgument> none;
            if (!in.com_put_member(*held->data, key, none, value, in.offset_)) {
                in.clear_error_for_retry();  // an unsupported property
            }
        }
        return true;
    }

    [[nodiscard]] static std::string render_value(const Value& value) {
        return Interpreter::render(value);
    }
    [[nodiscard]] static bool notify_sinks(
        Interpreter& in, InstanceData& source, const std::string& event,
        const std::vector<CallArgument>& arguments) {
        return in.dispatch_event_to_sinks(source, event, arguments, in.offset_);
    }
    [[nodiscard]] static bool controls_edit(
        Interpreter& in, const std::shared_ptr<InstanceData>& form,
        const std::string& member, std::vector<CallArgument>& arguments,
        Value& result, NativeFailure& failure);
    [[nodiscard]] static bool array_element(Interpreter& in,
                                            ControlArray& array, long index,
                                            bool unload, std::size_t offset);
    [[nodiscard]] static bool form_member(Interpreter& in, ControlObject& self,
                                          const std::string& member,
                                          Value& result);

    [[nodiscard]] static bool resolve(Interpreter& in, Value& value) {
        return in.resolve_default_value(value, in.offset_);
    }

    static NativeFailure reported() {
        return NativeFailure{kErrorAlreadyReported, {}};
    }

    // Calls `visit` for every control object of the form (arrays expanded).
    template <typename Visit>
    static void for_each_control(InstanceData& form, Visit&& visit) {
        for (auto& [name, value] : form.fields.variables) {
            if (auto* object = control_of(value)) {
                if (!object->is_form) {
                    visit(*object);
                }
            } else if (const auto* holder = std::get_if<ObjectInstance>(&value);
                       holder != nullptr && holder->data != nullptr) {
                if (auto* array = dynamic_cast<ControlArray*>(
                        holder->data->native.get())) {
                    for (auto& [position, element] : array->elements) {
                        if (auto* member = control_of(element)) {
                            visit(*member);
                        }
                    }
                }
            }
        }
    }

    // Pushes the list items and selection to the host.
    static void sync_list(Interpreter& in, ControlObject& list) {
        if (list.window == ui::kNoWindow) {
            return;
        }
        auto& host = *runtime(in).host;
        host.set_items(list.window,
                       list.display.empty() ? list.items : list.display);
        host.set_selection(list.window, list.list_index);
        if (list.class_key == "combobox") {
            // Resetting the items clears a combo box's edit text.
            const auto text = list.values.find("text");
            if (text != list.values.end() &&
                std::holds_alternative<std::string>(text->second)) {
                host.set_text(list.window, std::get<std::string>(text->second));
            }
        }
    }

    [[nodiscard]] static std::string item_text(const ControlObject& list) {
        if (list.list_index >= 0 &&
            static_cast<std::size_t>(list.list_index) < list.items.size()) {
            return list.items[static_cast<std::size_t>(list.list_index)];
        }
        return {};
    }

    // Moves the selection (from code or from the user), mirroring the text
    // of a combo box and raising Click (and Change for a combo's text).
    [[nodiscard]] static bool select_item(Interpreter& in, ControlObject& list,
                                          const long index,
                                          const bool raise_events) {
        const long previous = list.list_index;
        list.list_index = index;
        if (index >= 0 && static_cast<std::size_t>(index) < list.items.size()) {
            if (list.class_key == "filelistbox") {
                list.values["filename"] =
                    Value{list.items[static_cast<std::size_t>(index)]};
            } else if (list.class_key == "drivelistbox" && index != previous) {
                list.values["drive"] =
                    Value{list.items[static_cast<std::size_t>(index)]};
                sync_list(in, list);
                if (raise_events) {
                    return raise_table_event(in, list, "Change", {},
                                             in.offset_);
                }
            }
        }
        if (list.class_key == "combobox") {
            list.values["text"] = Value{item_text(list)};
        }
        sync_list(in, list);
        if (list.window != ui::kNoWindow && list.class_key == "combobox") {
            runtime(in).host->set_text(list.window, item_text(list));
        }
        if (!raise_events || index == previous) {
            return true;
        }
        if (!raise_table_event(in, list, "Click", {}, in.offset_)) {
            return false;
        }
        return list.class_key != "combobox" ||
               raise_table_event(in, list, "Change", {}, in.offset_);
    }

    // List members of get(); false when `member` is not a list member.
    [[nodiscard]] static bool list_get(Interpreter& in, ControlObject& self,
                                       const std::string& member,
                                       std::vector<CallArgument>& arguments,
                                       Value& result, NativeFailure& failure) {
        static_cast<void>(in);
        const auto indexed = [&](const auto& fetch) {
            const auto position = arguments.size() == 1U
                                      ? whole_value(arguments[0].value)
                                      : std::nullopt;
            if (!position.has_value() || *position < 0 ||
                static_cast<std::size_t>(*position) >= self.items.size()) {
                failure = NativeFailure{381, "Invalid property array index"};
                return true;
            }
            result = fetch(static_cast<std::size_t>(*position));
            return true;
        };
        if (member == "listcount") {
            result = Value{static_cast<Integer>(self.items.size())};
            return true;
        }
        if (member == "listindex") {
            result = Value{static_cast<Int16>(self.list_index)};
            return true;
        }
        if (member == "newindex") {
            result = Value{static_cast<Int16>(self.new_index)};
            return true;
        }
        if (member == "list") {
            return indexed(
                [&](const std::size_t at) { return Value{self.items[at]}; });
        }
        if (member == "itemdata") {
            return indexed([&](const std::size_t at) {
                return Value{static_cast<Integer>(self.item_data[at])};
            });
        }
        if (member == "selected") {
            return indexed([&](const std::size_t at) {
                const bool picked =
                    flag_of(self, "multiselect")
                        ? self.chosen[at] != false
                        : static_cast<long>(at) == self.list_index;
                return Value{picked};
            });
        }
        if (member == "selcount") {
            long count = 0;
            for (std::size_t i = 0; i < self.items.size(); ++i) {
                count += self.chosen[i] ? 1 : 0;
            }
            result = Value{static_cast<Int16>(
                flag_of(self, "multiselect") ? count
                                             : (self.list_index >= 0 ? 1 : 0))};
            return true;
        }
        if (member == "text" && self.class_key == "listbox") {
            result = Value{item_text(self)};
            return true;
        }
        if (member == "topindex") {
            result = Value{Int16{0}};
            return true;
        }
        return false;
    }

    // List members of put(); false when `member` is not a list member.
    [[nodiscard]] static bool list_put(Interpreter& in, ControlObject& self,
                                       const std::string& member,
                                       std::vector<CallArgument>& arguments,
                                       const Value& value,
                                       NativeFailure& failure) {
        const auto position = [&]() -> std::optional<std::size_t> {
            const auto at = arguments.size() == 1U
                                ? whole_value(arguments[0].value)
                                : std::nullopt;
            if (!at.has_value() || *at < 0 ||
                static_cast<std::size_t>(*at) >= self.items.size()) {
                failure = NativeFailure{381, "Invalid property array index"};
                return std::nullopt;
            }
            return static_cast<std::size_t>(*at);
        };
        if (member == "listindex") {
            const auto wanted = whole_value(value);
            if (!wanted.has_value()) {
                failure = NativeFailure{13, "Type mismatch"};
                return true;
            }
            if (*wanted < -1 ||
                static_cast<std::size_t>(*wanted + 1) > self.items.size()) {
                failure = NativeFailure{381, "Invalid property array index"};
                return true;
            }
            if (!select_item(in, self, *wanted, true)) {
                failure = reported();
            }
            return true;
        }
        if (member == "list") {
            if (const auto at = position()) {
                self.items[*at] = text_of(value);
                sync_list(in, self);
            }
            return true;
        }
        if (member == "itemdata") {
            if (const auto at = position()) {
                self.item_data[*at] = whole_value(value).value_or(0);
            }
            return true;
        }
        if (member == "selected") {
            if (const auto at = position()) {
                self.chosen[*at] = std::get_if<bool>(&value) != nullptr
                                       ? std::get<bool>(value)
                                       : as_double(value) != 0.0;
                if (!flag_of(self, "multiselect") && self.chosen[*at]) {
                    if (!select_item(in, self, static_cast<long>(*at), true)) {
                        failure = reported();
                    }
                }
            }
            return true;
        }
        if (member == "newindex" || member == "listcount" ||
            member == "selcount" ||
            (member == "text" && self.class_key == "listbox")) {
            failure = NativeFailure{383, "Property is read-only"};
            return true;
        }
        return false;
    }

    // AddItem, RemoveItem and Clear.
    [[nodiscard]] static bool list_method(Interpreter& in, ControlObject& self,
                                          const std::string& member,
                                          std::vector<CallArgument>& arguments,
                                          NativeFailure& failure) {
        if (member == "additem") {
            if (arguments.empty() || arguments.size() > 2U) {
                failure = NativeFailure{450,
                                        "Wrong number of arguments or "
                                        "invalid property assignment"};
                return true;
            }
            std::size_t at = self.items.size();
            const std::string text = text_of(arguments[0].value);
            if (arguments.size() == 2U && !arguments[1].omitted) {
                const auto wanted = whole_value(arguments[1].value);
                if (!wanted.has_value() || *wanted < 0 ||
                    static_cast<std::size_t>(*wanted) > self.items.size()) {
                    failure = NativeFailure{381,
                                            "Invalid property array "
                                            "index"};
                    return true;
                }
                at = static_cast<std::size_t>(*wanted);
            } else if (flag_of(self, "sorted")) {
                at = 0;
                while (at < self.items.size() &&
                       lower_text(self.items[at]) <= lower_text(text)) {
                    ++at;
                }
            }
            self.items.insert(self.items.begin() + static_cast<long>(at), text);
            self.item_data.insert(
                self.item_data.begin() + static_cast<long>(at), 0L);
            self.chosen.insert(self.chosen.begin() + static_cast<long>(at),
                               false);
            if (self.list_index >= static_cast<long>(at)) {
                ++self.list_index;
            }
            self.new_index = static_cast<long>(at);
            sync_list(in, self);
            return true;
        }
        if (member == "removeitem") {
            const auto wanted = arguments.size() == 1U
                                    ? whole_value(arguments[0].value)
                                    : std::nullopt;
            if (!wanted.has_value() || *wanted < 0 ||
                static_cast<std::size_t>(*wanted) >= self.items.size()) {
                failure = NativeFailure{381, "Invalid property array index"};
                return true;
            }
            const auto at = static_cast<std::size_t>(*wanted);
            self.items.erase(self.items.begin() + static_cast<long>(at));
            self.item_data.erase(self.item_data.begin() +
                                 static_cast<long>(at));
            self.chosen.erase(self.chosen.begin() + static_cast<long>(at));
            if (self.list_index == static_cast<long>(at)) {
                self.list_index = -1;
            } else if (self.list_index > static_cast<long>(at)) {
                --self.list_index;
            }
            sync_list(in, self);
            return true;
        }
        if (member == "clear") {
            self.items.clear();
            self.item_data.clear();
            self.chosen.clear();
            self.list_index = -1;
            self.new_index = -1;
            sync_list(in, self);
            return true;
        }
        return false;
    }

    // A numeric property's value, falling back to its design-time default.
    [[nodiscard]] static double number_of(const ControlObject& object,
                                          const std::string& key) {
        const auto found = object.values.find(key);
        if (found != object.values.end() && is_number(found->second)) {
            return as_double(found->second);
        }
        if (const auto fallback = property_default(object.class_key, key)) {
            if (is_number(*fallback)) {
                return as_double(*fallback);
            }
        }
        return 0.0;
    }

    // Moves a scroll bar to `target` (clamped), raising Scroll when
    // `dragging` and Change when the value changed.
    [[nodiscard]] static bool scroll_to(Interpreter& in, ControlObject& bar,
                                        double target, const bool dragging) {
        const double low =
            std::min(number_of(bar, "min"), number_of(bar, "max"));
        const double high =
            std::max(number_of(bar, "min"), number_of(bar, "max"));
        target = std::clamp(target, low, high);
        const double previous = number_of(bar, "value");
        bar.values["value"] = Value{static_cast<Int16>(target)};
        if (dragging && target != previous &&
            !raise_table_event(in, bar, "Scroll", {}, in.offset_)) {
            return false;
        }
        return target == previous ||
               raise_table_event(in, bar, "Change", {}, in.offset_);
    }

    // ----- SendKeys -----------------------------------------------------

    struct Keystroke {
        long code{0};   // virtual-key code
        long shift{0};  // 1 shift, 2 ctrl, 4 alt
        long ascii{0};  // the character, or 0 for a non-character key
    };

    // Parses a SendKeys string: plain characters, {NAME} keys with an optional
    // repeat count ({LEFT 3}), ~ (Enter), and the + ^ % modifiers (applied to
    // the next key or to a parenthesized group).
    [[nodiscard]] static bool parse_sendkeys(const std::string& text,
                                             std::vector<Keystroke>& out) {
        static const std::map<std::string, long> named = {
            {"enter", 13},    {"tab", 9},          {"esc", 27},
            {"escape", 27},   {"backspace", 8},    {"bs", 8},
            {"bksp", 8},      {"delete", 46},      {"del", 46},
            {"insert", 45},   {"ins", 45},         {"home", 36},
            {"end", 35},      {"pgup", 33},        {"pgdn", 34},
            {"left", 37},     {"up", 38},          {"right", 39},
            {"down", 40},     {"break", 3},        {"capslock", 20},
            {"numlock", 144}, {"scrolllock", 145}, {"help", 47},
            {"prtsc", 44},
        };
        long pending = 0;
        std::vector<long> group_shift;  // shift to apply inside a group
        const auto push = [&](Keystroke key) {
            long inherited = pending;
            for (const long extra : group_shift) {
                inherited |= extra;
            }
            key.shift |= inherited;
            pending = 0;
            out.push_back(key);
        };
        std::size_t i = 0;
        while (i < text.size()) {
            const char c = text[i];
            if (c == '+' || c == '^' || c == '%') {
                pending |= c == '+' ? 1 : c == '^' ? 2 : 4;
                ++i;
            } else if (c == '(' && pending != 0) {
                group_shift.push_back(pending);
                pending = 0;
                ++i;
            } else if (c == ')' && !group_shift.empty()) {
                group_shift.pop_back();
                ++i;
            } else if (c == '~') {
                push(Keystroke{13, 0, 13});
                ++i;
            } else if (c == '{') {
                const auto close = text.find('}', i + 2U);
                if (close == std::string::npos) {
                    return false;
                }
                std::string body = text.substr(i + 1U, close - i - 1U);
                i = close + 1U;
                long repeat = 1;
                const auto space = body.find(' ');
                if (space != std::string::npos) {
                    repeat = std::max(1L, std::atol(body.c_str() + space + 1U));
                    body.resize(space);
                }
                const std::string key = lower_text(body);
                Keystroke stroke;
                if (body.size() == 1U) {
                    stroke.ascii = static_cast<unsigned char>(body[0]);
                    stroke.code =
                        std::toupper(static_cast<unsigned char>(body[0]));
                } else if (const auto found = named.find(key);
                           found != named.end()) {
                    stroke.code = found->second;
                    stroke.ascii = found->second == 13 || found->second == 27 ||
                                           found->second == 8
                                       ? found->second
                                       : 0;
                } else if (key.size() >= 2U && key[0] == 'f' &&
                           std::isdigit(static_cast<unsigned char>(key[1]))) {
                    const long number = std::atol(key.c_str() + 1);
                    if (number < 1 || number > 16) {
                        return false;
                    }
                    stroke.code = 111 + number;
                } else {
                    return false;
                }
                for (long n = 0; n < repeat; ++n) {
                    push(stroke);
                }
            } else {
                Keystroke stroke;
                stroke.ascii = static_cast<unsigned char>(c);
                stroke.code = std::toupper(static_cast<unsigned char>(c));
                if (std::isupper(static_cast<unsigned char>(c)) != 0) {
                    stroke.shift |= 1;
                }
                push(stroke);
                ++i;
            }
        }
        return true;
    }

    // Edits a headless text box the way typing would.
    [[nodiscard]] static bool default_typing(Interpreter& in,
                                             ControlObject& source,
                                             const long ascii) {
        if (ascii < 8 || (ascii > 8 && ascii < 32)) {
            return true;
        }
        const auto previous = source.values.find("text");
        std::string text =
            previous != source.values.end() &&
                    std::holds_alternative<std::string>(previous->second)
                ? std::get<std::string>(previous->second)
                : std::string{};
        if (ascii == 8) {
            if (text.empty()) {
                return true;
            }
            // Remove one UTF-8 character.
            std::size_t cut = text.size() - 1U;
            while (cut > 0 &&
                   (static_cast<unsigned char>(text[cut]) & 0xC0U) == 0x80U) {
                --cut;
            }
            text.erase(cut);
        } else {
            text.push_back(static_cast<char>(ascii));
        }
        source.values["text"] = Value{text};
        runtime(in).host->set_text(source.window, text);
        return raise_table_event(in, source, "Change", {}, in.offset_);
    }

    // Delivers SendKeys keystrokes to the focused control.
    [[nodiscard]] static bool send_keys(Interpreter& in,
                                        const std::vector<Keystroke>& keys) {
        auto& state = runtime(in);
        for (const Keystroke& key : keys) {
            if (state.host->send_key(key.code, key.shift, key.ascii)) {
                continue;  // the host injected real input
            }
            // Delivered one at a time to whatever has the focus then.
            state.keys.push_back(PendingKey{key.code, key.shift, key.ascii});
        }
        return true;
    }

    // Posts the next pending keystroke to the focused control (or the first
    // form); false when none is waiting.
    [[nodiscard]] static bool deliver_pending_key(Interpreter& in) {
        auto& state = runtime(in);
        if (state.keys.empty()) {
            return false;
        }
        const PendingKey key = state.keys.front();
        state.keys.pop_front();
        ui::WindowId window = state.host->focus();
        if (window == ui::kNoWindow && !state.loaded.empty()) {
            if (const auto first = state.loaded.front().lock()) {
                window = control_of(*first)->window;
            }
        }
        if (window == ui::kNoWindow) {
            return true;
        }
        state.host->post_event(
            ui::UiEvent{window, "KeyDown", {key.code, key.shift}});
        if (key.ascii != 0 && (key.shift & 6) == 0) {
            state.host->post_event(
                ui::UiEvent{window, "KeyPress", {key.ascii}});
        }
        state.host->post_event(
            ui::UiEvent{window, "KeyUp", {key.code, key.shift}});
        return true;
    }

    // ----- Focus order, default keys, accelerators, validation ----------

    // Tabbable controls in tab order (TabIndex, else creation order).
    [[nodiscard]] static std::vector<ControlObject*> tab_sequence(
        InstanceData& form) {
        std::vector<ControlObject*> sequence;
        for_each_control(form, [&](ControlObject& object) {
            if (object.focusable() && flag_of(object, "tabstop") &&
                flag_of(object, "enabled") && flag_of(object, "visible")) {
                sequence.push_back(&object);
            }
        });
        std::stable_sort(
            sequence.begin(), sequence.end(),
            [](const ControlObject* left, const ControlObject* right) {
                const double a = left->values.contains("tabindex")
                                     ? as_double(left->values.at("tabindex"))
                                     : static_cast<double>(left->order);
                const double b = right->values.contains("tabindex")
                                     ? as_double(right->values.at("tabindex"))
                                     : static_cast<double>(right->order);
                return a != b ? a < b : left->order < right->order;
            });
        return sequence;
    }

    // Moves focus to the next (or previous) tab stop after `from`.
    [[nodiscard]] static bool move_focus(Interpreter& in, InstanceData& form,
                                         const ControlObject* from,
                                         const bool forward) {
        auto sequence = tab_sequence(form);
        if (sequence.empty()) {
            return true;
        }
        std::size_t at = sequence.size();
        for (std::size_t i = 0; i < sequence.size(); ++i) {
            if (sequence[i] == from) {
                at = i;
            }
        }
        std::size_t next = 0;
        if (at == sequence.size()) {
            next = forward ? 0 : sequence.size() - 1U;
        } else if (forward) {
            next = (at + 1U) % sequence.size();
        } else {
            next = (at + sequence.size() - 1U) % sequence.size();
        }
        return focus_control(in, form, sequence[next]->name,
                             sequence[next]->index, in.offset_);
    }

    // The command button with the given flag (`default` or `cancel`).
    [[nodiscard]] static ControlObject* special_button(InstanceData& form,
                                                       const char* flag) {
        ControlObject* found = nullptr;
        for_each_control(form, [&](ControlObject& object) {
            if (object.class_key == "commandbutton" && flag_of(object, flag) &&
                flag_of(object, "enabled") && flag_of(object, "visible") &&
                found == nullptr) {
                found = &object;
            }
        });
        return found;
    }

    static void click_later(Interpreter& in, const ControlObject& object) {
        if (object.window != ui::kNoWindow) {
            runtime(in).host->post_event(
                ui::UiEvent{object.window, "Click", {}});
        }
    }

    // The accelerator letter of a caption ("&Open" -> 'o'), or 0.
    [[nodiscard]] static char accelerator_of(const ControlObject& object) {
        const auto caption = object.values.find("caption");
        if (caption == object.values.end() ||
            !std::holds_alternative<std::string>(caption->second)) {
            return 0;
        }
        const auto& text = std::get<std::string>(caption->second);
        for (std::size_t i = 0; i + 1U < text.size(); ++i) {
            if (text[i] == '&') {
                if (text[i + 1U] == '&') {
                    ++i;
                    continue;
                }
                return ascii_lower(text[i + 1U]);
            }
        }
        return 0;
    }

    // Alt+letter: activates the control whose caption has that accelerator.
    [[nodiscard]] static bool accelerator(Interpreter& in, InstanceData& form,
                                          const char letter) {
        ControlObject* match = nullptr;
        for_each_control(form, [&](ControlObject& object) {
            if (match == nullptr && accelerator_of(object) == letter &&
                flag_of(object, "enabled") && flag_of(object, "visible")) {
                match = &object;
            }
        });
        if (match == nullptr) {
            return true;
        }
        if (match->class_key == "label" || match->class_key == "frame") {
            return move_focus(in, form, match, true);
        }
        if (!focus_control(in, form, match->name, match->index, in.offset_)) {
            return false;
        }
        if (match->class_key == "commandbutton" ||
            match->class_key == "checkbox" ||
            match->class_key == "optionbutton" || match->class_key == "menu") {
            click_later(in, *match);
        }
        return true;
    }

    // What a key does when the program does not swallow it.
    [[nodiscard]] static bool key_default(Interpreter& in, InstanceData& form,
                                          ControlObject& source,
                                          const long code, const long shift) {
        if ((shift & 4) != 0 && code >= 'A' && code <= 'Z') {
            return accelerator(
                in, form,
                static_cast<char>(ascii_lower(static_cast<char>(code))));
        }
        if (code == 9) {
            return move_focus(in, form, source.is_form ? nullptr : &source,
                              (shift & 1) == 0);
        }
        if (code == 13) {
            if (source.class_key == "commandbutton") {
                click_later(in, source);
            } else if (const auto* fallback = special_button(form, "default")) {
                click_later(in, *fallback);
            }
            return true;
        }
        if (code == 27) {
            if (const auto* cancel = special_button(form, "cancel")) {
                click_later(in, *cancel);
            }
            return true;
        }
        if (code == 32 && (source.class_key == "commandbutton" ||
                           source.class_key == "checkbox" ||
                           source.class_key == "optionbutton")) {
            click_later(in, source);
        }
        return true;
    }

    // Key names of the input script's `press` command.
    [[nodiscard]] static bool parse_key(const std::string& spec, long& code,
                                        long& shift, long& ascii) {
        shift = 0;
        ascii = 0;
        std::string name = lower_text(spec);
        for (;;) {
            if (name.rfind("shift+", 0) == 0) {
                shift |= 1;
                name.erase(0, 6);
            } else if (name.rfind("ctrl+", 0) == 0) {
                shift |= 2;
                name.erase(0, 5);
            } else if (name.rfind("alt+", 0) == 0) {
                shift |= 4;
                name.erase(0, 4);
            } else {
                break;
            }
        }
        if (name == "tab") {
            code = 9;
        } else if (name == "enter" || name == "return") {
            code = 13;
            ascii = 13;
        } else if (name == "esc" || name == "escape") {
            code = 27;
            ascii = 27;
        } else if (name == "space") {
            code = 32;
            ascii = 32;
        } else if (name == "backspace") {
            code = 8;
            ascii = 8;
        } else if (name == "delete") {
            code = 46;
        } else if (name == "left" || name == "up" || name == "right" ||
                   name == "down") {
            code = name == "left"    ? 37
                   : name == "up"    ? 38
                   : name == "right" ? 39
                                     : 40;
        } else if (name.size() >= 2U && name.front() == 'f' &&
                   std::isdigit(static_cast<unsigned char>(name[1])) != 0) {
            code = 111 + std::atol(name.c_str() + 1);
        } else if (name.size() == 1U &&
                   std::isalnum(static_cast<unsigned char>(name[0])) != 0) {
            code = std::toupper(static_cast<unsigned char>(name[0]));
            if ((shift & 6) == 0) {
                ascii = (shift & 1) != 0
                            ? std::toupper(static_cast<unsigned char>(name[0]))
                            : name[0];
            }
        } else {
            return false;
        }
        return true;
    }

    // ----- Pictures -----------------------------------------------------

    [[nodiscard]] static bool is_picture_property(const std::string& key) {
        return key == "picture" || key == "icon" || key == "mouseicon" ||
               key == "dragicon" || key == "downpicture" ||
               key == "disabledpicture";
    }

    // HIMETRIC (0.01 mm) to twips.
    [[nodiscard]] static double himetric_to_twips(const long himetric) {
        return static_cast<double>(himetric) * 1440.0 / 2540.0;
    }

    // The host's id for a picture, decoding it on first use.
    [[nodiscard]] static long host_picture(Interpreter& in,
                                           PictureObject& picture) {
        if (picture.host_id == 0 && !picture.bytes.empty()) {
            picture.host_id = runtime(in).host->load_picture(picture.bytes);
        }
        return picture.host_id;
    }

    // Gives a graphical button (Style = 1) its Picture.
    static void apply_button_picture(Interpreter& in, ControlObject& button) {
        if (button.window == ui::kNoWindow ||
            (button.class_key != "commandbutton" &&
             button.class_key != "checkbox" &&
             button.class_key != "optionbutton") ||
            number_of(button, "style") != 1.0) {
            return;
        }
        const auto found = button.values.find("picture");
        if (found == button.values.end()) {
            return;
        }
        if (PictureObject* picture = picture_of(found->second)) {
            if (const long id = host_picture(in, *picture); id != 0) {
                runtime(in).host->set_button_image(button.window, id);
            }
        }
    }

    // Draws `picture` into (x, y, width, height) twips of `surface`.
    static void paint_picture(Interpreter& in, ControlObject& surface,
                              PictureObject& picture, const double x,
                              const double y, const double width,
                              const double height) {
        if (picture.bytes.empty() || surface.window == ui::kNoWindow) {
            return;
        }
        ui::DrawOp operation;
        operation.kind = "picture";
        operation.picture = host_picture(in, picture);
        operation.x1 = x;
        operation.y1 = y;
        operation.x2 = width;
        operation.y2 = height;
        draw_op(in, surface, operation);
    }

    // The surface's own Picture drawn at its natural size at the origin.
    static void paint_surface_picture(Interpreter& in, ControlObject& surface) {
        const auto found = surface.values.find("picture");
        if (found == surface.values.end()) {
            return;
        }
        if (PictureObject* picture = picture_of(found->second)) {
            paint_picture(in, surface, *picture, 0.0, 0.0,
                          himetric_to_twips(picture->info.width),
                          himetric_to_twips(picture->info.height));
        }
    }

    // ----- MDI ----------------------------------------------------------

    [[nodiscard]] static bool is_mdi_child(const ControlObject& form) {
        return form.is_form && form.class_key == "form" &&
               flag_of(form, "mdichild");
    }

    // The loaded-or-loadable MDI form of the program, or nullptr.
    [[nodiscard]] static std::shared_ptr<InstanceData> mdi_parent(
        Interpreter& in) {
        for (const auto& [key, definition] : in.class_definitions_) {
            if (definition.form != nullptr &&
                definition.form->root.type == "VB.MDIForm") {
                const auto variable = in.find_variable(key);
                if (variable.value != nullptr) {
                    if (const auto* holder =
                            std::get_if<ObjectInstance>(variable.value)) {
                        return holder->data;
                    }
                }
            }
        }
        return nullptr;
    }

    // Child forms (loaded, MDIChild = True), in load order.
    [[nodiscard]] static std::vector<std::shared_ptr<InstanceData>>
    mdi_children(Interpreter& in) {
        std::vector<std::shared_ptr<InstanceData>> children;
        if (!in.forms_) {
            return children;
        }
        for (const auto& weak : in.forms_->loaded) {
            const auto form = weak.lock();
            const ControlObject* object = form ? control_of(*form) : nullptr;
            if (object != nullptr && is_mdi_child(*object)) {
                children.push_back(form);
            }
        }
        return children;
    }

    // ----- Lightweight controls (Image, Shape, Line) --------------------

    [[nodiscard]] static bool is_lightweight(const ControlObject& object) {
        return object.class_key == "image" || object.class_key == "shape" ||
               object.class_key == "line";
    }

    // The surface a lightweight control is drawn on: the enclosing picture
    // box, else the form (with the offset of enclosing frames).
    [[nodiscard]] static ControlObject* surface_for(InstanceData& form,
                                                    const ControlObject& object,
                                                    double& dx, double& dy) {
        dx = 0.0;
        dy = 0.0;
        std::string container = object.container;
        for (int depth = 0; depth < 8 && !container.empty(); ++depth) {
            ControlObject* holder = control_named(form, container, -1);
            if (holder == nullptr) {
                break;
            }
            if (holder->class_key == "picturebox") {
                return holder;
            }
            dx += number_of(*holder, "left");
            dy += number_of(*holder, "top");
            container = holder->container;
        }
        return control_of(form);
    }

    static void paint_lightweight(Interpreter& in, ControlObject& surface,
                                  const ControlObject& object, const double dx,
                                  const double dy) {
        if (!flag_of(object, "visible")) {
            return;
        }
        if (object.class_key == "image") {
            const auto found = object.values.find("picture");
            PictureObject* picture = found != object.values.end()
                                         ? picture_of(found->second)
                                         : nullptr;
            if (picture == nullptr || picture->bytes.empty()) {
                return;
            }
            const bool stretch = flag_of(object, "stretch");
            const double width = stretch
                                     ? number_of(object, "width")
                                     : himetric_to_twips(picture->info.width);
            const double height = stretch
                                      ? number_of(object, "height")
                                      : himetric_to_twips(picture->info.height);
            paint_picture(in, surface, *picture, dx + number_of(object, "left"),
                          dy + number_of(object, "top"), width, height);
        } else if (object.class_key == "shape") {
            ui::DrawOp operation = base_op(surface, "shape");
            operation.x1 = dx + number_of(object, "left");
            operation.y1 = dy + number_of(object, "top");
            operation.x2 = number_of(object, "width");
            operation.y2 = number_of(object, "height");
            operation.radius = number_of(object, "shape");
            operation.color = color_of(object, "bordercolor");
            operation.fill_color = color_of(object, "fillcolor");
            operation.fill_style =
                object.values.contains("fillstyle")
                    ? static_cast<int>(number_of(object, "fillstyle"))
                    : 1;
            operation.draw_width =
                std::max(1, static_cast<int>(number_of(object, "borderwidth")));
            operation.draw_style =
                object.values.contains("borderstyle")
                    ? static_cast<int>(number_of(object, "borderstyle"))
                    : 1;
            draw_op(in, surface, operation);
        } else if (object.class_key == "line") {
            ui::DrawOp operation = base_op(surface, "line");
            operation.x1 = dx + number_of(object, "x1");
            operation.y1 = dy + number_of(object, "y1");
            operation.x2 = dx + number_of(object, "x2");
            operation.y2 = dy + number_of(object, "y2");
            operation.color = color_of(object, "bordercolor");
            operation.draw_width =
                std::max(1, static_cast<int>(number_of(object, "borderwidth")));
            draw_op(in, surface, operation);
        }
    }

    // Clears `surface`, restores its picture and redraws the lightweight
    // controls it holds (drawings made by the program are lost, as when an
    // AutoRedraw = False form repaints).
    static void repaint_surface(Interpreter& in, InstanceData& form,
                                ControlObject& surface) {
        if (surface.window == ui::kNoWindow) {
            return;
        }
        ui::DrawOp clear = base_op(surface, "cls");
        clear.color = color_of(surface, "backcolor");
        draw_op(in, surface, clear);
        paint_surface_picture(in, surface);
        std::vector<ControlObject*> lightweights;
        for_each_control(form, [&](ControlObject& object) {
            if (is_lightweight(object)) {
                lightweights.push_back(&object);
            }
        });
        std::sort(lightweights.begin(), lightweights.end(),
                  [](const ControlObject* a, const ControlObject* b) {
                      return a->order != b->order ? a->order < b->order
                                                  : a->index < b->index;
                  });
        for (ControlObject* object : lightweights) {
            double dx = 0.0;
            double dy = 0.0;
            if (surface_for(form, *object, dx, dy) == &surface) {
                paint_lightweight(in, surface, *object, dx, dy);
            }
        }
    }

    // Repaints every surface that holds a lightweight control.
    static void repaint_lightweight_surfaces(Interpreter& in,
                                             InstanceData& form) {
        std::vector<ControlObject*> surfaces;
        for_each_control(form, [&](ControlObject& object) {
            if (!is_lightweight(object)) {
                return;
            }
            double dx = 0.0;
            double dy = 0.0;
            ControlObject* surface = surface_for(form, object, dx, dy);
            if (surface != nullptr &&
                std::find(surfaces.begin(), surfaces.end(), surface) ==
                    surfaces.end()) {
                surfaces.push_back(surface);
            }
        });
        for (ControlObject* surface : surfaces) {
            repaint_surface(in, form, *surface);
        }
    }

    // ----- Drawing surfaces (forms and picture boxes) -------------------

    [[nodiscard]] static bool is_surface(const ControlObject& object) {
        return object.is_form || object.class_key == "picturebox";
    }

    // The drawable area in twips.
    [[nodiscard]] static double client_width(const ControlObject& object) {
        return std::max(
            number_of(object, object.is_form ? "clientwidth" : "width"),
            number_of(object, "width"));
    }
    [[nodiscard]] static double client_height(const ControlObject& object) {
        return std::max(
            number_of(object, object.is_form ? "clientheight" : "height"),
            number_of(object, "height"));
    }

    // Twips in one unit of a ScaleMode (x and y differ for characters).
    [[nodiscard]] static double unit_x(const long mode) {
        switch (mode) {
            case 2:
                return 20.0;
            case 3:
                return 15.0;
            case 4:
                return 120.0;
            case 5:
                return 1440.0;
            case 6:
                return 56.6929;
            case 7:
                return 566.929;
            default:
                return 1.0;
        }
    }
    [[nodiscard]] static double unit_y(const long mode) {
        return mode == 4 ? 240.0 : unit_x(mode);
    }

    struct Scale {
        double left{0};
        double top{0};
        double width{1};
        double height{1};
    };

    [[nodiscard]] static Scale scale_of(const ControlObject& object) {
        const long mode = static_cast<long>(number_of(object, "scalemode"));
        Scale scale;
        if (mode == 0) {
            scale.left = number_of(object, "scaleleft");
            scale.top = number_of(object, "scaletop");
            scale.width = number_of(object, "scalewidth");
            scale.height = number_of(object, "scaleheight");
            if (scale.width == 0.0) {
                scale.width = client_width(object);
            }
            if (scale.height == 0.0) {
                scale.height = client_height(object);
            }
            return scale;
        }
        scale.width = client_width(object) / unit_x(mode);
        scale.height = client_height(object) / unit_y(mode);
        return scale;
    }

    [[nodiscard]] static double to_twips_x(const ControlObject& object,
                                           const double x) {
        const Scale scale = scale_of(object);
        return (x - scale.left) * client_width(object) / scale.width;
    }
    [[nodiscard]] static double to_twips_y(const ControlObject& object,
                                           const double y) {
        const Scale scale = scale_of(object);
        return (y - scale.top) * client_height(object) / scale.height;
    }
    [[nodiscard]] static double from_twips_x(const ControlObject& object,
                                             const double twips) {
        const Scale scale = scale_of(object);
        const double width = client_width(object);
        return width == 0.0 ? 0.0 : scale.left + twips * scale.width / width;
    }
    [[nodiscard]] static double from_twips_y(const ControlObject& object,
                                             const double twips) {
        const Scale scale = scale_of(object);
        const double height = client_height(object);
        return height == 0.0 ? 0.0 : scale.top + twips * scale.height / height;
    }

    [[nodiscard]] static ui::FontSpec font_of(const ControlObject& object) {
        ui::FontSpec font;
        const auto name = object.values.find("fontname");
        if (name != object.values.end() &&
            std::holds_alternative<std::string>(name->second)) {
            font.name = std::get<std::string>(name->second);
        }
        const double size = number_of(object, "fontsize");
        if (size > 0.0) {
            font.size = size;
        }
        font.bold = flag_of(object, "fontbold");
        font.italic = flag_of(object, "fontitalic");
        font.underline = flag_of(object, "fontunderline");
        font.strikethrough = flag_of(object, "fontstrikethru");
        return font;
    }

    // A VB color value as 0x00BBGGRR; system colors map to stock values.
    [[nodiscard]] static long resolve_color(const long color) {
        if ((color & 0x80000000L) == 0 || color >= 0) {
            return color & 0xFFFFFFL;
        }
        // The index of the Windows system color (VB's vbScrollBars to
        // vbInfoBackground), as the classic Windows palette has it: caption
        // text is white (Windows 11 reports black, which turns Capture King's
        // checkerboard, built from &H80000009 and &H80000007, all black).
        const auto rgb = [](const long r, const long g, const long b) {
            return r | (g << 8) | (b << 16);
        };
        const long palette[] = {
            rgb(200, 200, 200), rgb(0, 0, 0),       rgb(153, 180, 209),
            rgb(191, 205, 219), rgb(240, 240, 240), rgb(255, 255, 255),
            rgb(100, 100, 100), rgb(0, 0, 0),       rgb(0, 0, 0),
            rgb(255, 255, 255), rgb(180, 180, 180), rgb(244, 247, 252),
            rgb(171, 171, 171), rgb(51, 153, 255),  rgb(255, 255, 255),
            rgb(240, 240, 240), rgb(160, 160, 160), rgb(109, 109, 109),
            rgb(0, 0, 0),       rgb(67, 78, 84),    rgb(255, 255, 255),
            rgb(105, 105, 105), rgb(227, 227, 227), rgb(0, 0, 0),
            rgb(255, 255, 225)};
        const auto index = static_cast<std::size_t>(
            static_cast<unsigned long>(color) & 0xFFUL);
        return index < std::size(palette) ? palette[index] : 0xC0C0C0;
    }
    [[nodiscard]] static long color_of(const ControlObject& object,
                                       const std::string& key) {
        return resolve_color(static_cast<long>(number_of(object, key)));
    }

    static void draw_op(Interpreter& in, ControlObject& surface,
                        const ui::DrawOp& operation) {
        if (surface.window != ui::kNoWindow) {
            runtime(in).host->draw(surface.window, operation);
        }
    }

    [[nodiscard]] static ui::DrawOp base_op(const ControlObject& surface,
                                            const char* kind) {
        ui::DrawOp operation;
        operation.kind = kind;
        operation.color = color_of(surface, "forecolor");
        operation.fill_color = color_of(surface, "fillcolor");
        operation.fill_style =
            static_cast<int>(number_of(surface, "fillstyle"));
        operation.draw_width =
            std::max(1, static_cast<int>(number_of(surface, "drawwidth")));
        operation.draw_style =
            static_cast<int>(number_of(surface, "drawstyle"));
        operation.font = font_of(surface);
        return operation;
    }

    // Text at CurrentX/CurrentY; advances the current position.
    [[nodiscard]] static bool surface_print(Interpreter& in,
                                            ControlObject& surface,
                                            const std::string& text,
                                            const bool newline) {
        if (!touch_ok(in, surface)) {
            return false;
        }
        auto& state = runtime(in);
        const ui::FontSpec font = font_of(surface);
        const Scale scale = scale_of(surface);
        double current_x = number_of(surface, "currentx");
        double current_y = number_of(surface, "currenty");
        std::size_t start = 0;
        while (start <= text.size()) {
            auto end = text.find('\n', start);
            const bool last = end == std::string::npos;
            if (last) {
                end = text.size();
            }
            const std::string line = text.substr(start, end - start);
            const bool terminated = !last || newline;
            if (!line.empty() || terminated) {
                ui::DrawOp operation = base_op(surface, "text");
                operation.x1 = to_twips_x(surface, current_x);
                operation.y1 = to_twips_y(surface, current_y);
                operation.text = line;
                operation.back_color = color_of(surface, "backcolor");
                const auto metrics =
                    state.host->measure_text(surface.window, line, font);
                if (!line.empty()) {
                    draw_op(in, surface, operation);
                }
                if (terminated) {
                    current_x = scale.left;
                    current_y =
                        from_twips_y(surface, operation.y1 + metrics.height);
                } else {
                    current_x =
                        from_twips_x(surface, operation.x1 + metrics.width);
                }
            }
            if (last) {
                break;
            }
            start = end + 1U;
        }
        surface.values["currentx"] = Value{static_cast<float>(current_x)};
        surface.values["currenty"] = Value{static_cast<float>(current_y)};
        // The headless host also echoes printed text to the console, so
        // scripted runs can be checked from their output.
        if (state.headless != nullptr) {
            if (in.has_output_line_ && !in.output_line_open_) {
                in.output_.push_back('\n');
            }
            in.output_ += text;
            in.has_output_line_ = true;
            in.output_line_open_ = !newline;
        }
        return true;
    }

    [[nodiscard]] static bool touch_ok(Interpreter& in,
                                       ControlObject& surface) {
        NativeFailure failure;
        return touch(in, surface, failure);
    }

    // Computed surface properties (ScaleLeft, ...); false for other members.
    [[nodiscard]] static bool surface_get(const ControlObject& self,
                                          const std::string& member,
                                          Value& result) {
        const Scale scale = scale_of(self);
        if (member == "scaleleft") {
            result = Value{static_cast<float>(scale.left)};
        } else if (member == "scaletop") {
            result = Value{static_cast<float>(scale.top)};
        } else if (member == "scalewidth") {
            result = Value{static_cast<float>(scale.width)};
        } else if (member == "scaleheight") {
            result = Value{static_cast<float>(scale.height)};
        } else {
            return false;
        }
        return true;
    }

    [[nodiscard]] static bool surface_put(ControlObject& self,
                                          const std::string& member,
                                          const Value& value,
                                          NativeFailure& failure) {
        if (member == "scalemode") {
            const auto mode = whole_value(value);
            if (!mode.has_value() || *mode < 0 || *mode > 7) {
                failure = NativeFailure{380, "Invalid property value"};
                return true;
            }
            self.values["scalemode"] = Value{static_cast<Int16>(*mode)};
            self.values["scaleleft"] = Value{0.0F};
            self.values["scaletop"] = Value{0.0F};
            return true;
        }
        if (member == "scaleleft" || member == "scaletop" ||
            member == "scalewidth" || member == "scaleheight") {
            if (!is_number(value)) {
                failure = NativeFailure{13, "Type mismatch"};
                return true;
            }
            const Scale scale = scale_of(self);
            self.values["scaleleft"] = Value{static_cast<float>(scale.left)};
            self.values["scaletop"] = Value{static_cast<float>(scale.top)};
            self.values["scalewidth"] = Value{static_cast<float>(scale.width)};
            self.values["scaleheight"] =
                Value{static_cast<float>(scale.height)};
            self.values[member] = Value{static_cast<float>(as_double(value))};
            self.values["scalemode"] = Value{Int16{0}};
            return true;
        }
        return false;
    }

    // Methods of a drawing surface; false when `member` is not one.
    [[nodiscard]] static bool surface_method(
        Interpreter& in, ControlObject& surface, const std::string& member,
        std::vector<CallArgument>& arguments, Value& result,
        NativeFailure& failure) {
        const auto number_at = [&](const std::size_t index,
                                   const double fallback) {
            if (index < arguments.size() && !arguments[index].omitted &&
                is_number(arguments[index].value)) {
                return as_double(arguments[index].value);
            }
            return fallback;
        };
        if (member == "cls") {
            ui::DrawOp operation = base_op(surface, "cls");
            operation.color = color_of(surface, "backcolor");
            draw_op(in, surface, operation);
            paint_surface_picture(in, surface);
            if (const auto owner = surface.form.lock()) {
                // Lightweight controls are part of the surface, not drawings.
                std::vector<ControlObject*> lightweights;
                for_each_control(*owner, [&](ControlObject& object) {
                    if (is_lightweight(object)) {
                        lightweights.push_back(&object);
                    }
                });
                for (ControlObject* object : lightweights) {
                    double dx = 0.0;
                    double dy = 0.0;
                    if (surface_for(*owner, *object, dx, dy) == &surface) {
                        paint_lightweight(in, surface, *object, dx, dy);
                    }
                }
            }
            const Scale scale = scale_of(surface);
            surface.values["currentx"] = Value{static_cast<float>(scale.left)};
            surface.values["currenty"] = Value{static_cast<float>(scale.top)};
            result = Value{Empty{}};
            return true;
        }
        if (member == "textwidth" || member == "textheight") {
            if (arguments.empty()) {
                failure = NativeFailure{450, "Wrong number of arguments"};
                return true;
            }
            const auto metrics = runtime(in).host->measure_text(
                surface.window, text_of(arguments[0].value), font_of(surface));
            result = Value{static_cast<float>(
                member == "textwidth" ? from_twips_x(surface, metrics.width) -
                                            from_twips_x(surface, 0.0)
                                      : from_twips_y(surface, metrics.height) -
                                            from_twips_y(surface, 0.0))};
            return true;
        }
        if (member == "point") {
            if (arguments.size() != 2U) {
                failure = NativeFailure{450, "Wrong number of arguments"};
                return true;
            }
            const auto color = runtime(in).host->pixel(
                surface.window, to_twips_x(surface, number_at(0, 0.0)),
                to_twips_y(surface, number_at(1, 0.0)));
            result = Value{static_cast<Integer>(color.value_or(-1))};
            return true;
        }
        if (member == "paintpicture") {
            PictureObject* picture =
                arguments.empty() ? nullptr : picture_of(arguments[0].value);
            if (picture == nullptr || arguments.size() < 3U) {
                failure = NativeFailure{arguments.empty() ? 450 : 13,
                                        arguments.empty() ? "Wrong number of "
                                                            "arguments"
                                                          : "Type mismatch"};
                return true;
            }
            result = Value{Empty{}};
            if (picture->bytes.empty()) {
                return true;
            }
            const double x = to_twips_x(surface, number_at(1, 0.0));
            const double y = to_twips_y(surface, number_at(2, 0.0));
            const double natural_width = himetric_to_twips(picture->info.width);
            const double natural_height =
                himetric_to_twips(picture->info.height);
            const Scale scale = scale_of(surface);
            const auto width_of = [&](const std::size_t index,
                                      const double fallback) {
                return index < arguments.size() && !arguments[index].omitted &&
                               is_number(arguments[index].value)
                           ? as_double(arguments[index].value) *
                                 client_width(surface) / scale.width
                           : fallback;
            };
            const auto height_of = [&](const std::size_t index,
                                       const double fallback) {
                return index < arguments.size() && !arguments[index].omitted &&
                               is_number(arguments[index].value)
                           ? as_double(arguments[index].value) *
                                 client_height(surface) / scale.height
                           : fallback;
            };
            ui::DrawOp operation;
            operation.kind = "picture";
            operation.picture = host_picture(in, *picture);
            operation.x1 = x;
            operation.y1 = y;
            operation.x2 = width_of(3, natural_width);
            operation.y2 = height_of(4, natural_height);
            if (arguments.size() > 5U && !arguments[5].omitted) {
                operation.has_source = true;
                operation.source_x = width_of(5, 0.0);
                operation.source_y = height_of(6, 0.0);
                operation.source_width = width_of(7, natural_width);
                operation.source_height = height_of(8, natural_height);
            }
            draw_op(in, surface, operation);
            return true;
        }
        if (member == "scalex" || member == "scaley") {
            const double value = number_at(0, 0.0);
            const double from_mode = number_at(1, scale_mode_of(surface));
            const double to_mode = number_at(2, 1.0);
            const bool horizontal = member == "scalex";
            const auto unit = [&](const double mode) {
                if (mode == 0.0) {
                    const Scale scale = scale_of(surface);
                    return horizontal ? client_width(surface) / scale.width
                                      : client_height(surface) / scale.height;
                }
                return horizontal ? unit_x(static_cast<long>(mode))
                                  : unit_y(static_cast<long>(mode));
            };
            result = Value{
                static_cast<float>(value * unit(from_mode) / unit(to_mode))};
            return true;
        }
        return false;
    }

    [[nodiscard]] static double scale_mode_of(const ControlObject& object) {
        return number_of(object, "scalemode");
    }

    // A Boolean property's value, falling back to its design-time default.
    [[nodiscard]] static bool flag_of(const ControlObject& object,
                                      const std::string& key) {
        const auto found = object.values.find(key);
        if (found != object.values.end()) {
            if (const auto* value = std::get_if<bool>(&found->second)) {
                return *value;
            }
        }
        if (const auto fallback = property_default(object.class_key, key)) {
            if (const auto* value = std::get_if<bool>(&*fallback)) {
                return *value;
            }
        }
        return false;
    }

    [[nodiscard]] static Value* find_control(InstanceData& form,
                                             const std::string& name) {
        const auto found = form.fields.variables.find(name);
        return found == form.fields.variables.end() ? nullptr : &found->second;
    }

    // The ControlObject for `name` (and `index` of a control array).
    [[nodiscard]] static ControlObject* control_named(InstanceData& form,
                                                      const std::string& name,
                                                      const long index) {
        if (name.empty()) {
            return control_of(form);
        }
        Value* value = find_control(form, name);
        if (value == nullptr) {
            return nullptr;
        }
        if (auto* direct = control_of(*value)) {
            return direct;
        }
        const auto* holder = std::get_if<ObjectInstance>(value);
        if (holder == nullptr) {
            return nullptr;
        }
        auto* array = dynamic_cast<ControlArray*>(holder->data->native.get());
        if (array == nullptr) {
            return nullptr;
        }
        const auto element = array->elements.find(index);
        return element == array->elements.end() ? nullptr
                                                : control_of(element->second);
    }

    [[nodiscard]] static Value event_value(const ui::EventArgument& argument,
                                           const std::string& type) {
        double number = 0.0;
        std::string text;
        bool is_text = false;
        if (const auto* integer = std::get_if<long>(&argument)) {
            number = static_cast<double>(*integer);
        } else if (const auto* real = std::get_if<double>(&argument)) {
            number = *real;
        } else {
            text = std::get<std::string>(argument);
            is_text = true;
        }
        if (type == "String" || is_text) {
            return Value{is_text
                             ? text
                             : std::to_string(static_cast<long long>(number))};
        }
        if (type == "Integer") {
            return Value{static_cast<Int16>(number)};
        }
        if (type == "Long") {
            return Value{static_cast<Integer>(number)};
        }
        if (type == "Single") {
            return Value{static_cast<float>(number)};
        }
        if (type == "Boolean") {
            return Value{number != 0.0};
        }
        return Value{number};
    }

    // Raises `event` of `control` ("" for the form) by running its handler.
    [[nodiscard]] static bool raise(Interpreter& in, InstanceData& form,
                                    const std::string& control,
                                    const long index, const std::string& event,
                                    std::vector<Value>& arguments,
                                    const std::size_t offset) {
        const auto class_iterator = in.class_definitions_.find(form.class_name);
        if (class_iterator == in.class_definitions_.end() || !in.execute_) {
            return true;
        }
        const ControlObject* form_object = control_of(form);
        const std::string prefix =
            control.empty()
                ? (form_object != nullptr && form_object->class_key == "mdiform"
                       ? "mdiform"
                   : form_object != nullptr &&
                           form_object->class_key == "usercontrol"
                       ? "usercontrol"
                       : "form")
                : control;
        const std::string handler = prefix + "_" + event;
        const auto method = class_iterator->second.methods.find(handler);
        // Handlers of a `WithEvents` variable that holds this control.
        const auto to_sinks = [&]() {
            if (control.empty()) {
                return true;
            }
            ControlObject* source = control_named(form, control, index);
            const auto self = source != nullptr ? source->self.lock() : nullptr;
            if (self == nullptr || self->event_sinks.empty()) {
                return true;
            }
            std::vector<CallArgument> sink_call;
            for (auto& argument : arguments) {
                sink_call.push_back(
                    CallArgument{argument, &argument, false, {}});
            }
            return in.dispatch_event_to_sinks(*self, event, sink_call, offset);
        };
        if (method == class_iterator->second.methods.end()) {
            return to_sinks();
        }
        std::vector<CallArgument> call;
        if (index >= 0) {
            call.push_back(CallArgument{
                Value{static_cast<Int16>(index)}, nullptr, false, {}});
        }
        for (auto& argument : arguments) {
            call.push_back(CallArgument{argument, &argument, false, {}});
        }
        const auto keep = form.shared_from_this();
        return in.invoke_definition(method->second, handler, std::move(call),
                                    offset, class_iterator->second.source,
                                    keep.get())
                   .has_value() &&
               to_sinks();
    }

    // The event's arguments converted per the event table.
    [[nodiscard]] static std::vector<Value> table_arguments(
        const ControlObject& source, const std::string& event,
        const std::vector<ui::EventArgument>& raw) {
        std::vector<Value> arguments;
        if (const auto* row = ui::find_event(*source.cls, event)) {
            const auto parameters = ui::parameters_of(*row);
            for (std::size_t i = 0; i < parameters.size(); ++i) {
                arguments.push_back(
                    i < raw.size() ? event_value(raw[i], parameters[i].type)
                                   : default_for_type(parameters[i].type));
            }
        }
        return arguments;
    }

    // Raises an event with arguments converted per the event table.
    [[nodiscard]] static bool raise_table_event(
        Interpreter& in, ControlObject& source, const std::string& event,
        const std::vector<ui::EventArgument>& raw, const std::size_t offset) {
        const auto form = source.form.lock();
        if (form == nullptr) {
            return true;
        }
        auto arguments = table_arguments(source, event, raw);
        return raise(in, *form, source.is_form ? std::string{} : source.name,
                     source.index, lower_text(event), arguments, offset);
    }

    static void bind(Interpreter& in, ControlObject& object) {
        if (object.window == ui::kNoWindow) {
            return;
        }
        WindowBinding binding;
        binding.form = object.form;
        binding.control = object.is_form ? std::string{} : object.name;
        binding.index = object.index;
        runtime(in).bindings[object.window] = std::move(binding);
    }

    // The host window a control is a child of: its frame or picture box,
    // else the form.
    [[nodiscard]] static ui::WindowId parent_window(
        InstanceData& form, const ControlObject& form_object,
        const ControlObject& object) {
        if (!object.container.empty()) {
            const ControlObject* holder =
                control_named(form, object.container, -1);
            if (holder != nullptr && holder->window != ui::kNoWindow) {
                return holder->window;
            }
        }
        return form_object.window;
    }

    // Creates the host windows of a form and its controls and applies their
    // property values.
    // The host kind of a control: a multi-line text box carries its
    // MultiLine and ScrollBars settings, which fix the window's style.
    [[nodiscard]] static std::string window_kind(const ControlObject& object) {
        if (std::string_view{object.cls->name} == "TextBox" &&
            flag_of(object, "multiline")) {
            return "TextBox:" + std::to_string(static_cast<int>(
                                    number_of(object, "scrollbars")));
        }
        return object.cls->name;
    }
    [[nodiscard]] static bool create_windows(
        Interpreter& in, InstanceData& form, ControlObject& form_object,
        const ui::WindowId container = ui::kNoWindow) {
        auto& host = *runtime(in).host;
        ui::WindowId frame = container;
        if (is_mdi_child(form_object)) {
            // A child form lives inside the MDI form, which loads with it.
            if (const auto parent = mdi_parent(in)) {
                if (ensure_loaded(in, *parent, in.offset_)) {
                    if (const ControlObject* parent_object =
                            control_of(*parent)) {
                        frame = parent_object->window;
                    }
                }
            }
        }
        form_object.window = host.create_window(
            form_object.class_key == "mdiform"       ? "MDIForm"
            : form_object.class_key == "usercontrol" ? "UserControl"
                                                     : "Form",
            frame);
        bind(in, form_object);
        // Creation (z) order, so a container exists before its contents and
        // later controls sit above earlier ones.
        std::vector<ControlObject*> ordered;
        for_each_control(
            form, [&](ControlObject& object) { ordered.push_back(&object); });
        std::sort(ordered.begin(), ordered.end(),
                  [](const ControlObject* a, const ControlObject* b) {
                      return a->order != b->order ? a->order < b->order
                                                  : a->index < b->index;
                  });
        bool created = true;
        for (ControlObject* object : ordered) {
            if (object->is_user_control_host()) {
                created = attach_user_control(in, form, form_object, *object) &&
                          created;
                continue;
            }
            object->window = host.create_window(
                object->is_activex() ? "ActiveX:" + object->activex_clsid
                                     : window_kind(*object),
                parent_window(form, form_object, *object));
            bind(in, *object);
            mirror_all(in, *object);
            if (object->is_activex()) {
                created = attach_activex(in, form, *object) && created;
            }
        }
        mirror_all(in, form_object);
        return created;
    }

    // Pushes the check state or scroll range of a control to the host.
    static void push_state(Interpreter& in, ControlObject& object) {
        if (object.window == ui::kNoWindow) {
            return;
        }
        auto& host = *runtime(in).host;
        if (object.class_key == "checkbox" ||
            object.class_key == "optionbutton") {
            const auto found = object.values.find("value");
            long state = 0;
            if (found != object.values.end()) {
                if (const auto* flag = std::get_if<bool>(&found->second)) {
                    state = *flag ? 1 : 0;
                } else if (is_number(found->second)) {
                    state = static_cast<long>(as_double(found->second));
                }
            }
            host.set_value(object.window, state);
        } else if (object.class_key == "menu") {
            host.set_value(object.window, flag_of(object, "checked") ? 1 : 0);
        } else if (object.is_scroll()) {
            host.set_scroll(
                object.window, static_cast<long>(number_of(object, "min")),
                static_cast<long>(number_of(object, "max")),
                static_cast<long>(number_of(object, "value")),
                static_cast<long>(number_of(object, "smallchange")),
                static_cast<long>(number_of(object, "largechange")));
        }
    }

    // Pushes every stored property to the host.
    // A Label's colors and BackStyle, for the host that paints it.
    static void push_label_style(Interpreter& in, const ControlObject& label) {
        const auto color = [&](const char* key) {
            const auto found = label.values.find(key);
            return found == label.values.end() ? -1L : color_of(label, key);
        };
        const auto style = label.values.find("backstyle");
        const bool transparent =
            style != label.values.end() && as_double(style->second) == 0.0;
        runtime(in).host->set_label_style(label.window, color("forecolor"),
                                          color("backcolor"), transparent);
    }

    static void mirror_all(Interpreter& in, ControlObject& object) {
        auto& host = *runtime(in).host;
        const auto number_of = [&](const char* key) {
            const auto found = object.values.find(key);
            return found == object.values.end() ? 0.0
                                                : as_double(found->second);
        };
        for (const char* key : {"caption", "text"}) {
            const auto found = object.values.find(key);
            if (found != object.values.end() &&
                std::holds_alternative<std::string>(found->second)) {
                host.set_text(object.window,
                              std::get<std::string>(found->second));
            }
        }
        host.set_bounds(
            object.window,
            ui::Rect{
                number_of("left"), number_of("top"),
                object.is_form ? client_width(object) : number_of("width"),
                object.is_form ? client_height(object) : number_of("height")});
        if (object.class_key == "label") {
            push_label_style(in, object);
        }
        const auto flag = [&](const char* key, const bool fallback) {
            const auto found = object.values.find(key);
            if (found != object.values.end()) {
                if (const auto* value = std::get_if<bool>(&found->second)) {
                    return *value;
                }
            }
            return fallback;
        };
        host.set_enabled(object.window, flag("enabled", true));
        if (!object.is_form) {
            host.set_visible(object.window, flag("visible", true));
        }
        if (object.class_key == "timer") {
            apply_timer(in, object);
        }
        if (object.is_form && object.values.contains("backcolor")) {
            // A form's own BackColor paints its surface.
            ui::DrawOp clear = base_op(object, "cls");
            clear.color = color_of(object, "backcolor");
            draw_op(in, object, clear);
            paint_surface_picture(in, object);
        }
        if (object.class_key == "picturebox") {
            if (object.values.contains("borderstyle") &&
                !flag_of(object, "borderstyle")) {
                host.set_border(object.window, false);
            }
            if (object.values.contains("backcolor")) {
                ui::DrawOp clear = base_op(object, "cls");
                clear.color = color_of(object, "backcolor");
                draw_op(in, object, clear);
            }
            paint_surface_picture(in, object);
        }
        apply_button_picture(in, object);
        if (object.is_file_list() && object.items.empty()) {
            refresh_file_list(in, object);
        }
        if (object.is_list() && !object.is_file_list() &&
            !object.items.empty()) {
            sync_list(in, object);
        }
        push_state(in, object);
        comctl_push(in, object);
    }

    static void apply_timer(Interpreter& in, ControlObject& timer) {
        auto& host = *runtime(in).host;
        const auto form = timer.form.lock();
        const ControlObject* owner = form ? control_of(*form) : nullptr;
        const auto interval = timer.values.find("interval");
        const bool on = flag_of(timer, "enabled");
        const long milliseconds =
            interval != timer.values.end()
                ? static_cast<long>(as_double(interval->second))
                : 0;
        if (on && milliseconds > 0 && owner != nullptr && owner->loaded) {
            host.set_timer(timer.window, milliseconds);
        } else {
            host.kill_timer(timer.window);
        }
    }

    [[nodiscard]] static bool ensure_loaded(Interpreter& in, InstanceData& form,
                                            const std::size_t offset) {
        auto* object = control_of(form);
        if (object == nullptr || object->loaded || object->unloading) {
            return true;
        }
        object->loaded = true;
        if (!create_windows(in, form, *object)) {
            return false;
        }
        repaint_lightweight_surfaces(in, form);
        runtime(in).loaded.push_back(form.weak_from_this());
        std::vector<Value> none;
        return raise(in, form, "", -1, "load", none, offset);
    }

    // Loads the owning form before an object's first use.
    [[nodiscard]] static bool touch(Interpreter& in, ControlObject& self,
                                    NativeFailure& failure) {
        const auto form = self.form.lock();
        if (form == nullptr) {
            return true;
        }
        if (!ensure_loaded(in, *form, in.offset_)) {
            failure = reported();
            return false;
        }
        return true;
    }

    // Saves a PNG of the form's window (Win32 host with --screenshots).
    static void screenshot(Interpreter& in, const ControlObject& form,
                           const char* moment) {
        if (in.screenshot_directory_.empty() || form.window == ui::kNoWindow) {
            return;
        }
        std::error_code ignored;
        std::filesystem::create_directories(in.screenshot_directory_, ignored);
        const auto path = std::filesystem::path(in.screenshot_directory_) /
                          (form.display_name + "-" + moment + ".png");
        static_cast<void>(
            runtime(in).host->capture(form.window, path.string()));
    }

    // Sets keyboard focus, raising LostFocus and GotFocus.
    [[nodiscard]] static bool focus_control(Interpreter& in, InstanceData& form,
                                            const std::string& control,
                                            const long index,
                                            const std::size_t offset) {
        auto& host = *runtime(in).host;
        ControlObject* target = control_named(form, control, index);
        if (target == nullptr || target->window == ui::kNoWindow ||
            host.focus() == target->window ||
            (!target->is_form && !target->focusable())) {
            return true;
        }
        const auto previous = runtime(in).bindings.find(host.focus());
        // Leaving a control validates it when the new control asks for it.
        if (previous != runtime(in).bindings.end() &&
            flag_of(*target, "causesvalidation")) {
            if (const auto old_form = previous->second.form.lock()) {
                ControlObject* old =
                    control_named(*old_form, previous->second.control,
                                  previous->second.index);
                if (old != nullptr && !old->is_form &&
                    ui::find_event(*old->cls, "Validate") != nullptr) {
                    std::vector<Value> cancel{Value{false}};
                    if (!raise(in, *old_form, old->name, old->index, "validate",
                               cancel, offset)) {
                        return false;
                    }
                    const auto* flag = std::get_if<bool>(&cancel[0]);
                    if ((flag != nullptr && *flag) ||
                        (flag == nullptr &&
                         whole_value(cancel[0]).value_or(0) != 0)) {
                        return true;  // the control keeps the focus
                    }
                }
            }
        }
        host.set_focus(target->window);
        if (previous != runtime(in).bindings.end()) {
            if (const auto old_form = previous->second.form.lock()) {
                ControlObject* old =
                    control_named(*old_form, previous->second.control,
                                  previous->second.index);
                if (old != nullptr && !old->is_form &&
                    (!raise_table_event(in, *old, "LostFocus", {}, offset) ||
                     !bind_update(in, *old))) {
                    return false;
                }
            }
        }
        if (!target->is_form) {
            return raise_table_event(in, *target, "GotFocus", {}, offset);
        }
        return true;
    }

    [[nodiscard]] static bool show(Interpreter& in, InstanceData& form,
                                   const std::size_t offset) {
        if (!ensure_loaded(in, form, offset)) {
            return false;
        }
        auto* object = control_of(form);
        if (object == nullptr) {
            return true;
        }
        if (is_mdi_child(*object)) {
            if (const auto parent = mdi_parent(in)) {
                if (!show(in, *parent, offset)) {
                    return false;
                }
            }
        }
        object->shown = true;
        paint_surface_picture(in, *object);
        object->values["visible"] = Value{true};
        runtime(in).host->set_visible(object->window, true);
        std::vector<Value> none;
        if (!object->activated) {
            object->activated = true;
            // The first tab stop is the active control while Activate runs;
            // it receives GotFocus afterwards.
            const auto stops = tab_sequence(form);
            ControlObject* first = stops.empty() ? nullptr : stops.front();
            auto& host = *runtime(in).host;
            if (first != nullptr && first->window != ui::kNoWindow &&
                host.focus() == ui::kNoWindow) {
                host.set_focus(first->window);
            } else {
                first = nullptr;
            }
            // Datagrams a Load handler provoked have arrived by now.
            if (!ocx_poll_sockets(in)) {
                return false;
            }
            if (!raise(in, form, "", -1, "activate", none, offset)) {
                return false;
            }
            if (first != nullptr && host.focus() == first->window &&
                !raise_table_event(in, *first, "GotFocus", {}, offset)) {
                return false;
            }
            screenshot(in, *object, "shown");
        }
        return true;
    }

    [[nodiscard]] static bool hide(Interpreter& in, InstanceData& form,
                                   const std::size_t offset) {
        auto* object = control_of(form);
        if (object == nullptr || !object->loaded) {
            return true;
        }
        object->shown = false;
        object->activated = false;
        object->values["visible"] = Value{false};
        runtime(in).host->set_visible(object->window, false);
        std::vector<Value> none;
        return raise(in, form, "", -1, "deactivate", none, offset);
    }

    // Unloads a form: QueryUnload, Unload, then the windows go away.
    [[nodiscard]] static bool unload(Interpreter& in, InstanceData& form,
                                     const long mode,
                                     const std::size_t offset) {
        auto* object = control_of(form);
        if (object == nullptr || !object->loaded || object->unloading) {
            return true;
        }
        object->unloading = true;
        if (object->class_key == "mdiform") {
            screenshot(in, *object, "closing-with-children");
            auto children = mdi_children(in);
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                const ControlObject* child = control_of(**it);
                if (!unload(in, **it, 4, offset) ||
                    (child != nullptr && child->loaded)) {
                    object->unloading = false;  // a child refused
                    return !in.has_pending_error();
                }
            }
        }
        std::vector<Value> query{Value{Int16{0}},
                                 Value{static_cast<Int16>(mode)}};
        if (!raise(in, form, "", -1, "queryunload", query, offset)) {
            object->unloading = false;
            return false;
        }
        bool cancelled = whole_value(query[0]).value_or(0) != 0;
        if (!cancelled) {
            bool updated = true;
            for_each_control(form, [&](ControlObject& bound) {
                updated = updated && bind_update(in, bound);
            });
            if (!updated) {
                object->unloading = false;
                return false;
            }
            std::vector<Value> unload_arguments{Value{Int16{0}}};
            if (!raise(in, form, "", -1, "unload", unload_arguments, offset)) {
                object->unloading = false;
                return false;
            }
            cancelled = whole_value(unload_arguments[0]).value_or(0) != 0;
        }
        object->unloading = false;
        if (cancelled) {
            return true;
        }
        screenshot(in, *object, "closing");
        terminate_user_controls(in, form);
        destroy_windows(in, form, *object);
        std::vector<Value> none;
        return raise(in, form, "", -1, "terminate", none, offset);
    }

    static void destroy_windows(Interpreter& in, InstanceData& form,
                                ControlObject& object) {
        auto& state = runtime(in);
        state.host->destroy_window(object.window);
        std::erase_if(state.bindings, [&](const auto& entry) {
            return entry.second.form.lock().get() == &form;
        });
        std::erase_if(state.loaded, [&](const auto& entry) {
            const auto alive = entry.lock();
            return alive == nullptr || alive.get() == &form;
        });
        object.loaded = false;
        object.shown = false;
        object.activated = false;
        object.window = ui::kNoWindow;
        const auto reset = [](ControlObject& item) {
            item.values = item.design;
            item.window = ui::kNoWindow;
        };
        reset(object);
        for (auto& [name, value] : form.fields.variables) {
            if (auto* item = control_of(value)) {
                if (!item->is_form) {
                    reset(*item);
                }
            } else if (const auto* holder = std::get_if<ObjectInstance>(&value);
                       holder != nullptr && holder->data != nullptr) {
                if (auto* array = dynamic_cast<ControlArray*>(
                        holder->data->native.get())) {
                    for (auto& [at, element] : array->elements) {
                        if (auto* member = control_of(element)) {
                            reset(*member);
                        }
                    }
                }
            }
        }
    }

    [[nodiscard]] static bool call_method(Interpreter& in, ControlObject& self,
                                          const std::string& member,
                                          std::vector<CallArgument>& arguments,
                                          Value& result,
                                          NativeFailure& failure) {
        const auto form = self.form.lock();
        result = Value{Empty{}};
        if (member == "setfocus") {
            if (form == nullptr ||
                !focus_control(in, *form, self.is_form ? "" : self.name,
                               self.index, in.offset_)) {
                failure = reported();
            }
            return true;
        }
        if (member == "move") {
            static const char* const keys[] = {"left", "top", "width",
                                               "height"};
            for (std::size_t i = 0; i < arguments.size() && i < 4U; ++i) {
                if (arguments[i].omitted) {
                    continue;
                }
                std::vector<CallArgument> none;
                if (!self.put(in, keys[i], none, arguments[i].value, failure)) {
                    failure = NativeFailure{438,
                                            "Object doesn't support this "
                                            "property or method"};
                }
                if (failure.number != 0) {
                    return true;
                }
            }
            return true;
        }
        if (is_surface(self) &&
            surface_method(in, self, member, arguments, result, failure)) {
            return true;
        }
        if (self.is_list() && (member == "additem" || member == "removeitem" ||
                               member == "clear")) {
            return list_method(in, self, member, arguments, failure);
        }
        if (member == "linkexecute" || member == "linkpoke" ||
            member == "linkrequest" || member == "linksend") {
            // DDE is excluded from the product (REQ-0130 disposition).
            failure = NativeFailure{
                282, "No foreign application responded to a DDE initiate"};
            return true;
        }
        if (member == "validatecontrols" && form != nullptr) {
            // Validates the control that has the focus; a cancelled
            // validation raises 380.
            auto& state = runtime(in);
            const auto binding = state.bindings.find(state.host->focus());
            if (binding != state.bindings.end() &&
                binding->second.form.lock() == form) {
                ControlObject* focused = control_named(
                    *form, binding->second.control, binding->second.index);
                if (focused != nullptr && !focused->is_form &&
                    ui::find_event(*focused->cls, "Validate") != nullptr) {
                    std::vector<Value> cancel{Value{false}};
                    if (!raise(in, *form, focused->name, focused->index,
                               "validate", cancel, in.offset_)) {
                        failure = reported();
                    } else if ((std::get_if<bool>(&cancel[0]) != nullptr &&
                                std::get<bool>(cancel[0])) ||
                               whole_value(cancel[0]).value_or(0) != 0) {
                        failure = NativeFailure{380, "Invalid property value"};
                    }
                }
            }
            return true;
        }
        if (member == "popupmenu" && form != nullptr) {
            // The scripted `answer popupmenu <name>` picks an item.
            if (const auto chosen = in.form_dialog_answer("popupmenu")) {
                if (const ControlObject* item =
                        control_named(*form, lower_text(*chosen), -1)) {
                    click_later(in, *item);
                }
            }
            return true;
        }
        if (member == "arrange" && self.class_key == "mdiform") {
            const long mode = arguments.empty()
                                  ? 0
                                  : whole_value(arguments[0].value).value_or(0);
            runtime(in).host->arrange(self.window, mode);
            return true;
        }
        if (member == "printform" && self.is_form &&
            in.class_definitions_.contains("wfcprinter")) {
            // The form's image goes to the virtual printer as a marker line.
            const auto printer = in.find_variable("printer");
            const auto* held = printer.value != nullptr
                                   ? std::get_if<ObjectInstance>(printer.value)
                                   : nullptr;
            if (held != nullptr && held->data != nullptr) {
                auto& document = held->data->fields.variables["mdoc"];
                std::string spooled;
                if (const auto* text = std::get_if<std::string>(&document)) {
                    spooled = *text;
                }
                spooled +=
                    "[form " + self.display_name + " " +
                    std::to_string(static_cast<long>(client_width(self))) +
                    "x" +
                    std::to_string(static_cast<long>(client_height(self))) +
                    " twips]\n";
                document = Value{spooled};
            }
            return true;
        }
        if (member == "printform" || member == "arrange" ||
            member == "whatsthismode" || member == "showwhatsthis" ||
            member == "oledrag") {
            return true;
        }
        if (member == "drag") {
            // vbBeginDrag (1) starts a drag; vbEndDrag (0) and vbCancel (2)
            // end it. Drops arrive from scripted `drag` input.
            const long action =
                arguments.empty() || arguments[0].omitted
                    ? 1
                    : whole_value(arguments[0].value).value_or(1);
            runtime(in).drag_source = action == 1 ? self.window : ui::kNoWindow;
            if (action == 1) {
                runtime(in).host->begin_drag(self.window);
            }
            return true;
        }
        if (member == "refresh" || member == "zorder" || member == "cls" ||
            member == "setfocus") {
            return true;
        }
        if (self.is_form && member == "show") {
            const bool modal = !arguments.empty() && !arguments[0].omitted &&
                               whole_value(arguments[0].value).value_or(0) != 0;
            if (form == nullptr || !show(in, *form, in.offset_) ||
                (modal && !run_modal(in, *form))) {
                failure = reported();
            }
            return true;
        }
        if (self.is_form && member == "hide") {
            if (form == nullptr || !hide(in, *form, in.offset_)) {
                failure = reported();
            }
            return true;
        }
        return false;
    }

    // ----- Data binding (REQ-0131) --------------------------------------

    [[nodiscard]] static bool is_bindable(const ControlObject& object) {
        return !object.is_form &&
               (object.class_key == "textbox" || object.class_key == "label" ||
                object.class_key == "checkbox" ||
                object.class_key == "combobox");
    }

    [[nodiscard]] static const char* bound_property(
        const ControlObject& object) {
        if (object.class_key == "label") {
            return "caption";
        }
        return object.class_key == "checkbox" ? "value" : "text";
    }

    // The object a bound control reads from (DataSource, then DataMember);
    // `ok` is false when an error was raised. Empty without a binding.
    [[nodiscard]] static std::optional<Value> bound_source(
        Interpreter& in, ControlObject& object, bool& ok) {
        ok = true;
        const auto found = object.values.find("datasource");
        if (found == object.values.end() ||
            !std::holds_alternative<ObjectInstance>(found->second) ||
            text_value(object, "datafield").empty()) {
            return std::nullopt;
        }
        Value source = found->second;
        const std::string member = text_value(object, "datamember");
        if (!member.empty()) {
            std::vector<Value> arguments{source, Value{member},
                                         Value{Integer{2}}};
            auto next =
                in.evaluate_misc_function("callbyname", arguments, in.offset_);
            if (!next.has_value()) {
                ok = false;
                return std::nullopt;
            }
            source = std::move(*next);
            if (!std::holds_alternative<ObjectInstance>(source)) {
                static_cast<void>(in.raise_runtime(
                    91, "Object variable or With block variable not set",
                    in.offset_));
                ok = false;
                return std::nullopt;
            }
        }
        return source;
    }

    // Reads the bound field into the control (DataChanged becomes False).
    [[nodiscard]] static bool bind_refresh(Interpreter& in,
                                           ControlObject& object) {
        if (!is_bindable(object)) {
            return true;
        }
        bool ok = true;
        const auto source = bound_source(in, object, ok);
        if (!source.has_value()) {
            return ok;
        }
        std::vector<Value> arguments{
            *source, Value{text_value(object, "datafield")}, Value{Integer{2}}};
        auto field =
            in.evaluate_misc_function("callbyname", arguments, in.offset_);
        if (!field.has_value()) {
            return false;
        }
        const std::string property = bound_property(object);
        Value shown;
        if (std::holds_alternative<Null>(*field) ||
            std::holds_alternative<Empty>(*field)) {
            shown =
                property == "value" ? Value{Int16{0}} : Value{std::string{}};
        } else if (property == "value") {
            const bool on = whole_value(*field).value_or(0) != 0 ||
                            (std::holds_alternative<bool>(*field) &&
                             std::get<bool>(*field));
            shown = Value{Int16{static_cast<std::int16_t>(on ? 1 : 0)}};
        } else {
            shown = Value{Interpreter::render(*field)};
        }
        const bool changed = !(object.values[property] == shown);
        object.values[property] = shown;
        object.values["datachanged"] = Value{false};
        if (object.window != ui::kNoWindow) {
            auto& host = *runtime(in).host;
            if (property == "value") {
                push_state(in, object);
            } else {
                host.set_text(object.window, std::get<std::string>(shown));
            }
        }
        if (changed && property != "value" && object.class_key != "label") {
            object.bind_busy = true;
            const bool raised =
                raise_table_event(in, object, "Change", {}, in.offset_);
            object.bind_busy = false;
            object.values["datachanged"] = Value{false};
            return raised;
        }
        return true;
    }

    // Writes a pending edit (DataChanged) back to the bound field.
    [[nodiscard]] static bool bind_update(Interpreter& in,
                                          ControlObject& object) {
        if (!is_bindable(object) || !flag_of(object, "datachanged")) {
            return true;
        }
        bool ok = true;
        const auto source = bound_source(in, object, ok);
        if (!source.has_value()) {
            return ok;
        }
        const std::string property = bound_property(object);
        const Value current = object.values[property];
        std::vector<Value> arguments{*source,
                                     Value{text_value(object, "datafield")},
                                     Value{Integer{4}}, current};
        if (!in.evaluate_misc_function("callbyname", arguments, in.offset_)
                 .has_value()) {
            return false;
        }
        object.values["datachanged"] = Value{false};
        return true;
    }

    // ----- Drive, directory and file list boxes ------------------------

    [[nodiscard]] static std::string text_value(const ControlObject& object,
                                                const char* key) {
        const auto found = object.values.find(key);
        if (found != object.values.end()) {
            if (const auto* text = std::get_if<std::string>(&found->second)) {
                return *text;
            }
        }
        if (const auto fallback = property_default(object.class_key, key)) {
            if (const auto* text = std::get_if<std::string>(&*fallback)) {
                return *text;
            }
        }
        return {};
    }

    // Rebuilds the items of a drive, directory or file list box.
    static void refresh_file_list(Interpreter& in, ControlObject& list) {
        namespace fs = std::filesystem;
        std::error_code ignored;
        list.items.clear();
        list.display.clear();
        list.list_index = -1;
        if (list.class_key == "drivelistbox") {
            const std::string current = text_value(list, "drive");
            for (char letter = 'a'; letter <= 'z'; ++letter) {
                const std::string drive = std::string(1U, letter) + ":";
                if (fs::exists(drive + "\\", ignored)) {
                    if (drive == current) {
                        list.list_index = static_cast<long>(list.items.size());
                    }
                    list.items.push_back(drive);
                }
            }
        } else if (list.class_key == "dirlistbox") {
            const fs::path path = text_value(list, "path");
            std::vector<fs::path> chain;
            for (fs::path step = path;; step = step.parent_path()) {
                chain.insert(chain.begin(), step);
                if (step == step.parent_path() || step.empty()) {
                    break;
                }
            }
            std::size_t depth = 0;
            for (const auto& step : chain) {
                const std::string name = step.filename().string();
                list.items.push_back(directory_text(step));
                list.display.push_back(
                    std::string(depth * 2U, ' ') +
                    (name.empty() ? directory_text(step) : name));
                ++depth;
            }
            list.list_index = static_cast<long>(list.items.size()) - 1;
            std::vector<std::string> children;
            for (fs::directory_iterator it(path, ignored), end;
                 !ignored && it != end; it.increment(ignored)) {
                if (it->is_directory(ignored)) {
                    children.push_back(it->path().filename().string());
                }
            }
            std::sort(children.begin(), children.end(),
                      [](const std::string& a, const std::string& b) {
                          return fold_case(a) < fold_case(b);
                      });
            for (const auto& child : children) {
                list.items.push_back(directory_text(path / child));
                list.display.push_back(std::string(depth * 2U, ' ') + child);
            }
        } else if (list.class_key == "filelistbox") {
            const fs::path path = text_value(list, "path");
            std::vector<std::string> patterns;
            std::string pattern_text = text_value(list, "pattern");
            std::size_t start = 0;
            while (start <= pattern_text.size()) {
                auto end = pattern_text.find(';', start);
                if (end == std::string::npos) {
                    end = pattern_text.size();
                }
                std::string part = pattern_text.substr(start, end - start);
                const auto first = part.find_first_not_of(' ');
                const auto last = part.find_last_not_of(' ');
                if (first != std::string::npos) {
                    patterns.push_back(part.substr(first, last - first + 1U));
                }
                start = end + 1U;
            }
            if (patterns.empty()) {
                patterns.push_back("*.*");
            }
            std::vector<std::string> files;
            for (fs::directory_iterator it(path, ignored), end;
                 !ignored && it != end; it.increment(ignored)) {
                if (!it->is_regular_file(ignored)) {
                    continue;
                }
                const std::string name = it->path().filename().string();
                for (const auto& pattern : patterns) {
                    // "*.*" matches names without an extension, as in VB.
                    const std::string effective =
                        pattern == "*.*" ? "*" : pattern;
                    if (wildcard_match(effective, name)) {
                        files.push_back(name);
                        break;
                    }
                }
            }
            std::sort(files.begin(), files.end(),
                      [](const std::string& a, const std::string& b) {
                          return fold_case(a) < fold_case(b);
                      });
            const std::string chosen = fold_case(text_value(list, "filename"));
            for (const auto& name : files) {
                if (!chosen.empty() && fold_case(name) == chosen) {
                    list.list_index = static_cast<long>(list.items.size());
                }
                list.items.push_back(name);
            }
        }
        list.chosen.assign(list.items.size(), false);
        sync_list(in, list);
    }

    [[nodiscard]] static bool report_bind(Interpreter& in, ControlObject& self,
                                          NativeFailure& failure) {
        if (!bind_refresh(in, self)) {
            failure = reported();
        }
        return true;
    }

    // The post-assignment work of Path, Pattern, Drive and FileName.
    [[nodiscard]] static bool file_list_changed(Interpreter& in,
                                                ControlObject& self,
                                                const std::string& member,
                                                const Value& old_value,
                                                NativeFailure& failure) {
        namespace fs = std::filesystem;
        std::error_code ignored;
        const std::size_t offset = in.offset_;
        const auto revert = [&](const long number, const char* description) {
            self.values[member] = old_value;
            failure = NativeFailure{number, description};
            return true;
        };
        if (member == "path" && self.class_key != "drivelistbox") {
            const std::string requested = text_value(self, "path");
            if (!fs::is_directory(requested, ignored)) {
                return revert(76, "Path not found");
            }
            self.values["path"] = Value{directory_text(requested)};
            refresh_file_list(in, self);
            return raise_table_event(
                in, self,
                self.class_key == "filelistbox" ? "PathChange" : "Change", {},
                offset);
        }
        if (member == "pattern" && self.class_key == "filelistbox") {
            refresh_file_list(in, self);
            return raise_table_event(in, self, "PatternChange", {}, offset);
        }
        if (member == "filename" && self.class_key == "filelistbox") {
            refresh_file_list(in, self);
            return true;
        }
        if (member == "drive" && self.class_key == "drivelistbox") {
            const std::string text = text_value(self, "drive");
            if (text.empty() ||
                !std::isalpha(static_cast<unsigned char>(text.front()))) {
                return revert(68, "Device unavailable");
            }
            const std::string drive =
                std::string(1U, static_cast<char>(std::tolower(
                                    static_cast<unsigned char>(text[0])))) +
                ":";
            if (!fs::exists(drive + "\\", ignored)) {
                return revert(68, "Device unavailable");
            }
            self.values["drive"] = Value{drive};
            refresh_file_list(in, self);
            return raise_table_event(in, self, "Change", {}, offset);
        }
        return true;
    }

    // Mirrors a changed property to the host and raises its events.
    [[nodiscard]] static bool property_changed(Interpreter& in,
                                               ControlObject& self,
                                               const std::string& member,
                                               const Value& old_value,
                                               NativeFailure& failure) {
        auto& host = *runtime(in).host;
        const Value& now = self.values[member];
        const bool live = self.window != ui::kNoWindow;
        if (self.comctl != nullptr) {
            comctl_push(in, self);
        }
        if (is_lightweight(self) && live) {
            if (const auto owner = self.form.lock()) {
                double dx = 0.0;
                double dy = 0.0;
                if (ControlObject* surface =
                        surface_for(*owner, self, dx, dy)) {
                    repaint_surface(in, *owner, *surface);
                }
            }
            return true;
        }
        if (member == "value" || member == "checked" || member == "min" ||
            member == "max" || member == "smallchange" ||
            member == "largechange") {
            push_state(in, self);
        }
        if (!self.is_form &&
            (member == "datasource" || member == "datamember" ||
             member == "datafield")) {
            return report_bind(in, self, failure);
        }
        if (is_bindable(self) && !self.bind_busy &&
            member == bound_property(self) &&
            !text_value(self, "datafield").empty()) {
            self.values["datachanged"] = Value{true};
        }
        if (self.is_file_list() && !self.is_form &&
            (member == "path" || member == "pattern" || member == "drive" ||
             member == "filename")) {
            return file_list_changed(in, self, member, old_value, failure);
        }
        const std::size_t offset = in.offset_;
        const auto report = [&](const bool ok) {
            if (!ok) {
                failure = reported();
            }
            return ok;
        };
        if (member == "caption" || member == "text") {
            if (live && std::holds_alternative<std::string>(now)) {
                host.set_text(self.window, std::get<std::string>(now));
            }
            const bool changed = !(now == old_value);
            if (changed && !self.is_form &&
                (self.class_key == "textbox" || self.class_key == "label" ||
                 self.class_key == "combobox")) {
                return report(
                    raise_table_event(in, self, "Change", {}, offset));
            }
            return true;
        }
        if (live && self.class_key == "label" &&
            (member == "forecolor" || member == "backcolor" ||
             member == "backstyle")) {
            push_label_style(in, self);
            return true;
        }
        if (member == "borderstyle" && live && self.class_key == "picturebox") {
            host.set_border(self.window, flag_of(self, "borderstyle"));
            return true;
        }
        if (member == "backcolor" && live &&
            (self.is_form || self.class_key == "picturebox")) {
            // A new BackColor clears the surface to it (AutoRedraw off).
            if (const auto owner = self.form.lock()) {
                repaint_surface(in, *owner, self);
            } else {
                ui::DrawOp clear = base_op(self, "cls");
                clear.color = color_of(self, "backcolor");
                draw_op(in, self, clear);
                paint_surface_picture(in, self);
            }
            return true;
        }
        if (member == "enabled" && live) {
            host.set_enabled(self.window, std::get<bool>(now));
            if (self.class_key == "timer") {
                apply_timer(in, self);
            }
            return true;
        }
        if (member == "interval" && live && self.class_key == "timer") {
            apply_timer(in, self);
            return true;
        }
        if (is_picture_property(member) && live &&
            (self.is_form || self.class_key == "picturebox") &&
            member == "picture") {
            PictureObject* picture = picture_of(now);
            if (picture != nullptr && !picture->bytes.empty() &&
                self.class_key == "picturebox" && flag_of(self, "autosize")) {
                // AutoSize fits the box to the picture (plus the border).
                const double border = flag_of(self, "borderstyle") ? 60.0 : 0.0;
                self.values["width"] = Value{static_cast<float>(
                    himetric_to_twips(picture->info.width) + border)};
                self.values["height"] = Value{static_cast<float>(
                    himetric_to_twips(picture->info.height) + border)};
                host.set_bounds(
                    self.window,
                    ui::Rect{number_of(self, "left"), number_of(self, "top"),
                             number_of(self, "width"),
                             number_of(self, "height")});
            }
            ui::DrawOp clear = base_op(self, "cls");
            clear.color = color_of(self, "backcolor");
            draw_op(in, self, clear);
            paint_surface_picture(in, self);
            return true;
        }
        if (member == "picture" && live && !self.is_form) {
            apply_button_picture(in, self);
        }
        if (member == "visible" && self.is_form) {
            const auto form = self.form.lock();
            if (form != nullptr && std::get<bool>(now)) {
                return report(show(in, *form, offset));
            }
            if (form != nullptr) {
                return report(hide(in, *form, offset));
            }
            return true;
        }
        if (member == "visible" && live) {
            host.set_visible(self.window, std::get<bool>(now));
            return true;
        }
        if ((member == "left" || member == "top" || member == "width" ||
             member == "height") &&
            live) {
            mirror_all(in, self);
            if (self.is_user_control_host() &&
                (member == "width" || member == "height")) {
                return report(resize_user_control(in, self));
            }
            return true;
        }
        if (member == "value" && self.is_scroll() && !(now == old_value)) {
            return report(raise_table_event(in, self, "Change", {}, offset));
        }
        if (member == "value" && !self.is_form &&
            (self.class_key == "checkbox" || self.class_key == "optionbutton" ||
             self.class_key == "commandbutton") &&
            !(now == old_value)) {
            return report(raise_table_event(in, self, "Click", {}, offset));
        }
        return true;
    }

    // Delivers a host event to the matching handler.
    [[nodiscard]] static bool dispatch(Interpreter& in,
                                       const ui::UiEvent& event) {
        auto& state = runtime(in);
        const auto binding = state.bindings.find(event.window);
        if (binding == state.bindings.end()) {
            return true;
        }
        const auto form = binding->second.form.lock();
        if (form == nullptr) {
            return true;
        }
        ControlObject* source = control_named(*form, binding->second.control,
                                              binding->second.index);
        if (source == nullptr) {
            return true;
        }
        const std::string name = lower_text(event.name);
        if (name == "close") {
            return !source->is_form || unload(in, *form, 0, in.offset_);
        }
        if (name == "change" && state.headless == nullptr &&
            (source->class_key == "textbox" ||
             source->class_key == "combobox")) {
            // An echo of a change the program made is not a user edit.
            const std::string typed = state.host->text(source->window);
            const auto known = source->values.find("text");
            if (known != source->values.end() &&
                std::holds_alternative<std::string>(known->second) &&
                std::get<std::string>(known->second) == typed) {
                return true;
            }
            source->values["text"] = Value{typed};
            if (is_bindable(*source) &&
                !text_value(*source, "datafield").empty()) {
                source->values["datachanged"] = Value{true};
            }
        }
        const bool input = name == "click" || name == "dblclick" ||
                           name == "keydown" || name == "keyup" ||
                           name == "keypress" || name == "mousedown" ||
                           name == "mouseup" || name == "mousemove";
        if (input && !source->is_form) {
            const auto enabled = source->values.find("enabled");
            if (enabled != source->values.end() &&
                std::holds_alternative<bool>(enabled->second) &&
                !std::get<bool>(enabled->second)) {
                return true;
            }
            if (name == "click" && !focus_control(in, *form, source->name,
                                                  source->index, in.offset_)) {
                return false;
            }
        }
        if (name == "dblclick" && source->class_key == "dirlistbox" &&
            source->list_index >= 0 &&
            static_cast<std::size_t>(source->list_index) <
                source->items.size()) {
            // Double-clicking a directory makes it the current one.
            const Value previous = source->values["path"];
            source->values["path"] = Value{
                source->items[static_cast<std::size_t>(source->list_index)]};
            NativeFailure ignored;
            if (!file_list_changed(in, *source, "path", previous, ignored)) {
                return false;
            }
        }
        if (name == "userscroll" || name == "userscrollby") {
            if (source->is_scroll() && !event.arguments.empty()) {
                const double amount = std::get<double>(event.arguments[0]);
                const double base =
                    name == "userscroll" ? 0.0 : number_of(*source, "value");
                if (!focus_control(in, *form, source->name, source->index,
                                   in.offset_)) {
                    return false;
                }
                return scroll_to(in, *source, base + amount,
                                 name == "userscroll");
            }
            return true;
        }
        if (name == "userselect") {
            if (source->is_list() && !event.arguments.empty()) {
                const long index = std::get<long>(event.arguments[0]);
                if (index >= 0 &&
                    static_cast<std::size_t>(index) < source->items.size()) {
                    if (!focus_control(in, *form, source->name, source->index,
                                       in.offset_)) {
                        return false;
                    }
                    if (flag_of(*source, "multiselect")) {
                        source->chosen[static_cast<std::size_t>(index)] =
                            !source->chosen[static_cast<std::size_t>(index)];
                    }
                    const long previous = source->list_index;
                    if (!select_item(in, *source, index, true)) {
                        return false;
                    }
                    return previous != index ||
                           raise_table_event(in, *source, "Click", {},
                                             in.offset_);
                }
            }
            return true;
        }
        if (name == "click" && !source->is_form) {
            if (source->class_key == "checkbox") {
                const auto current = source->values.find("value");
                const long now =
                    current == source->values.end()
                        ? 0
                        : static_cast<long>(as_double(current->second));
                source->values["value"] = Value{Int16{now == 1 ? 0 : 1}};
            } else if (source->class_key == "optionbutton") {
                source->values["value"] = Value{true};
                for_each_control(*form, [&](ControlObject& other) {
                    if (&other != source && other.class_key == "optionbutton" &&
                        other.container == source->container) {
                        other.values["value"] = Value{false};
                        push_state(in, other);
                    }
                });
            }
            push_state(in, *source);
        }
        // With KeyPreview the form sees key events first and can swallow
        // them by zeroing the key code.
        if ((name == "keydown" || name == "keyup" || name == "keypress") &&
            !source->is_form) {
            ControlObject* owner = control_of(*form);
            if (owner != nullptr && flag_of(*owner, "keypreview")) {
                auto preview =
                    table_arguments(*owner, event.name, event.arguments);
                if (!raise(in, *form, "", -1, name, preview, in.offset_)) {
                    return false;
                }
                if (!preview.empty() &&
                    whole_value(preview[0]).value_or(0) == 0) {
                    return true;
                }
            }
        }
        if (name == "comctlinput" && source->comctl != nullptr &&
            event.arguments.size() == 2U) {
            // User input on a common control in the Win32 host.
            const std::string action =
                std::holds_alternative<std::string>(event.arguments[0])
                    ? std::get<std::string>(event.arguments[0])
                    : std::string{};
            const std::string rest =
                std::holds_alternative<std::string>(event.arguments[1])
                    ? std::get<std::string>(event.arguments[1])
                    : std::string{};
            ComctlSupport::from_host(*source, true);
            const bool ok = ComctlSupport::user_input(in, *form, *source,
                                                      lower_text(action), rest);
            ComctlSupport::from_host(*source, false);
            return ok;
        }
        if (name == "dropon" && event.arguments.size() == 3U) {
            // The end of a mouse drag over `source` (the drop target).
            const auto from = state.bindings.find(
                static_cast<ui::WindowId>(std::get<long>(event.arguments[0])));
            state.drag_source = ui::kNoWindow;
            if (from == state.bindings.end()) {
                return true;
            }
            Target dragged;
            dragged.form = from->second.form.lock();
            dragged.control = from->second.control;
            dragged.index = from->second.index;
            Target target;
            target.form = form;
            target.control = source->is_form ? std::string{} : source->name;
            target.index = source->index;
            if (dragged.form == nullptr) {
                return true;
            }
            const ControlObject* scale_owner =
                is_surface(*source) ? source : control_of(*form);
            return drop_on(in, dragged, target,
                           from_twips_x(*scale_owner,
                                        std::get<double>(event.arguments[1])),
                           from_twips_y(*scale_owner,
                                        std::get<double>(event.arguments[2])));
        }
        std::vector<ui::EventArgument> raw_arguments = event.arguments;
        if ((name == "mousedown" || name == "mouseup" || name == "mousemove") &&
            raw_arguments.size() == 4U) {
            // The host reports twips; the program sees its scale units.
            const ControlObject* scale_owner =
                is_surface(*source) ? source : control_of(*form);
            if (scale_owner != nullptr) {
                raw_arguments[2] = from_twips_x(
                    *scale_owner, std::get<double>(raw_arguments[2]));
                raw_arguments[3] = from_twips_y(
                    *scale_owner, std::get<double>(raw_arguments[3]));
            }
            if (name == "mousedown" && !source->is_form &&
                number_of(*source, "dragmode") == 1.0) {
                // DragMode = Automatic: the press starts a drag.
                state.drag_source = source->window;
                state.host->begin_drag(source->window);
            }
        }
        auto arguments = table_arguments(*source, event.name, raw_arguments);
        if (!raise(in, *form, source->is_form ? std::string{} : source->name,
                   source->index, name, arguments, in.offset_)) {
            return false;
        }
        if (name == "keypress" && !arguments.empty() &&
            state.headless != nullptr && !source->is_form &&
            (source->class_key == "textbox" ||
             source->class_key == "combobox")) {
            return default_typing(in, *source,
                                  whole_value(arguments[0]).value_or(0));
        }
        if (name == "keydown" && !arguments.empty()) {
            const long code = whole_value(arguments[0]).value_or(0);
            const long shift = arguments.size() > 1U
                                   ? whole_value(arguments[1]).value_or(0)
                                   : 0;
            if (code != 0) {
                return key_default(in, *form, *source, code, shift);
            }
        }
        return true;
    }

    static void loaded_forms(Interpreter& in, std::vector<Value>& items) {
        if (!in.forms_) {
            return;
        }
        for (const auto& weak : in.forms_->loaded) {
            if (const auto form = weak.lock()) {
                items.push_back(Value{ObjectInstance{form}});
            }
        }
    }

    [[nodiscard]] static std::shared_ptr<InstanceData> find_form(
        Interpreter& in, const std::string& name) {
        if (in.forms_ != nullptr) {
            for (const auto& weak : in.forms_->loaded) {
                if (const auto form = weak.lock()) {
                    const auto* object = control_of(*form);
                    if (object != nullptr && lower_text(object->name) == name) {
                        return form;
                    }
                }
            }
        }
        const auto variable = in.find_variable_raw(name);
        if (variable.value != nullptr) {
            if (const auto* holder =
                    std::get_if<ObjectInstance>(variable.value)) {
                return holder->data;
            }
        }
        return nullptr;
    }

    // `target` is `Form.Control`, `Form.Control(3)`, `Control` or `Form`.
    struct Target {
        std::shared_ptr<InstanceData> form;
        std::string control;
        long index{-1};
    };

    [[nodiscard]] static bool resolve_target(Interpreter& in,
                                             const std::string& text,
                                             Target& target) {
        std::string form_name;
        std::string control = lower_text(text);
        const auto dot = control.find('.');
        if (dot != std::string::npos) {
            form_name = control.substr(0, dot);
            control = control.substr(dot + 1U);
        }
        const auto paren = control.find('(');
        if (paren != std::string::npos) {
            target.index = std::atol(control.c_str() + paren + 1U);
            control = control.substr(0, paren);
        }
        if (!form_name.empty()) {
            target.form = find_form(in, form_name);
        } else {
            // A bare name: a form, else a control of a loaded form.
            target.form = find_form(in, control);
            if (target.form != nullptr) {
                control.clear();
            } else if (in.forms_) {
                for (const auto& weak : in.forms_->loaded) {
                    const auto form = weak.lock();
                    if (form != nullptr &&
                        control_named(*form, control, target.index) !=
                            nullptr) {
                        target.form = form;
                        break;
                    }
                }
            }
        }
        target.control = control;
        return target.form != nullptr;
    }

    // Raises DragOver then DragDrop on `destination` with `source` as the
    // dragged control (the scripted equivalent of a mouse drag).
    [[nodiscard]] static bool drop_on(Interpreter& in, const Target& source,
                                      const Target& destination, const double x,
                                      const double y) {
        ControlObject* dragged =
            control_named(*source.form, source.control, source.index);
        if (dragged == nullptr) {
            return true;
        }
        runtime(in).drag_source = dragged->window;
        const Value source_value = control_value(*source.form, *dragged);
        for (const char* event : {"dragover", "dragdrop"}) {
            std::vector<Value> arguments{source_value,
                                         Value{static_cast<float>(x)},
                                         Value{static_cast<float>(y)}};
            if (std::string_view(event) == "dragover") {
                arguments.push_back(Value{Int16{0}});  // vbEnter
            }
            if (!raise(in, *destination.form, destination.control,
                       destination.index, event, arguments, in.offset_)) {
                return false;
            }
        }
        runtime(in).drag_source = ui::kNoWindow;
        return true;
    }

    // The scripted equivalent of an OLE drag: OLEStartDrag on the source
    // fills a DataObject, then OLEDragOver and OLEDragDrop run on the target
    // when its OLEDropMode is manual, and OLECompleteDrag ends it.
    [[nodiscard]] static bool ole_drag(Interpreter& in, const Target& source,
                                       const Target& destination,
                                       const double x, const double y) {
        ControlObject* from =
            control_named(*source.form, source.control, source.index);
        ControlObject* to = control_named(
            *destination.form, destination.control, destination.index);
        if (from == nullptr || to == nullptr ||
            !in.class_definitions_.contains("wfcdataobject")) {
            return true;
        }
        auto data = in.instantiate_class("wfcdataobject", in.offset_);
        if (!data.has_value()) {
            return false;
        }
        // vbDropEffectCopy (1) | vbDropEffectMove (2) are allowed.
        std::vector<Value> start{*data, Value{Integer{3}}};
        if (!raise(in, *source.form, from->is_form ? std::string{} : from->name,
                   from->index, "olestartdrag", start, in.offset_)) {
            return false;
        }
        long effect = 3;
        if (number_of(*to, "oledropmode") == 1.0) {
            for (const char* name : {"oledragover", "oledragdrop"}) {
                std::vector<Value> arguments{*data, Value{Integer{effect}}};
                arguments.push_back(Value{Int16{1}});  // Button
                arguments.push_back(Value{Int16{0}});  // Shift
                arguments.push_back(Value{static_cast<float>(x)});
                arguments.push_back(Value{static_cast<float>(y)});
                if (std::string_view(name) == "oledragover") {
                    arguments.push_back(Value{Int16{0}});  // vbEnter
                }
                if (!raise(in, *destination.form,
                           to->is_form ? std::string{} : to->name, to->index,
                           name, arguments, in.offset_)) {
                    return false;
                }
                effect = whole_value(arguments[1]).value_or(effect);
            }
        } else {
            effect = 0;  // vbDropEffectNone: nothing accepts the data
        }
        std::vector<Value> done{Value{Integer{effect}}};
        return raise(in, *source.form,
                     from->is_form ? std::string{} : from->name, from->index,
                     "olecompletedrag", done, in.offset_);
    }

    // One line of the scripted input.
    [[nodiscard]] static bool run_script_line(Interpreter& in,
                                              const std::string& line) {
        std::istringstream words(line);
        std::string command;
        words >> command;
        command = lower_text(command);
        if (command.empty() || command.front() == '#' ||
            command.front() == '\'') {
            return true;
        }
        std::string target_text;
        words >> target_text;
        std::string rest;
        std::getline(words, rest);
        if (!rest.empty() && rest.front() == ' ') {
            rest.erase(0, 1);
        }
        auto& state = runtime(in);
        if (command == "trace") {
            // Print the drawing operations since the last `trace`.
            if (state.headless != nullptr) {
                for (const auto& drawn : state.headless->take_drawing()) {
                    if (in.has_output_line_ && !in.output_line_open_) {
                        in.output_.push_back('\n');
                    }
                    in.output_ += drawn;
                    in.has_output_line_ = true;
                    in.output_line_open_ = false;
                }
            }
            return true;
        }
        if (command == "answer") {
            state.answers.emplace_back(lower_text(target_text), rest);
            return true;
        }
        if (command == "wait" || command == "waitclock") {
            // `wait <ms>` lets events and sockets run for that long;
            // `waitclock <ms since 1970>` until that wall-clock time, so
            // programs that share a virtual LAN can act in step.
            const bool absolute = command == "waitclock";
            const long long amount = std::atoll(target_text.c_str());
            if (state.headless != nullptr && !absolute && !net::virtual_lan()) {
                state.headless->advance_time(static_cast<long>(amount));
                return ocx_poll_sockets(in);
            }
            const auto until =
                absolute ? std::chrono::system_clock::
                               time_point{std::chrono::milliseconds(amount)}
                         : std::chrono::system_clock::now() +
                               std::chrono::milliseconds(amount);
            while (std::chrono::system_clock::now() < until) {
                if (!ocx_poll_sockets(in)) {
                    return false;
                }
                if (state.headless != nullptr) {
                    state.headless->advance_time(5);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                } else if (const auto event = state.host->next_event(false)) {
                    if (!dispatch(in, *event)) {
                        return false;
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }
            return true;
        }
        if (command == "screenshot") {
            // screenshot <form> [moment]: a PNG of the form (with
            // --screenshots).
            Target shot;
            if (resolve_target(in, target_text, shot) && shot.form != nullptr) {
                if (const ControlObject* form_object = control_of(*shot.form)) {
                    screenshot(in, *form_object,
                               rest.empty() ? "script" : rest.c_str());
                }
            }
            return true;
        }
        if (command == "pump") {
            // pump <ms>: let the Winsock controls run for that long.
            const auto until =
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(std::atol(target_text.c_str()));
            while (std::chrono::steady_clock::now() < until) {
                if (!ocx_poll_sockets(in)) {
                    return false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            return true;
        }
        if (command == "ui") {
            // ui <target> <action> [arguments]: a scripted action on a
            // common control (select a tab, click a button, ...).
            Target ui_target;
            if (!resolve_target(in, target_text, ui_target)) {
                return true;
            }
            ControlObject* ui_object = control_named(
                *ui_target.form, ui_target.control, ui_target.index);
            if (ui_object == nullptr || ui_object->comctl == nullptr) {
                return true;
            }
            std::istringstream parts(rest);
            std::string action;
            parts >> action;
            std::string tail;
            std::getline(parts, tail);
            if (!tail.empty() && tail.front() == ' ') {
                tail.erase(0, 1);
            }
            return ComctlSupport::user_input(in, *ui_target.form, *ui_object,
                                             lower_text(action), tail);
        }
        if (command == "oledrag") {
            // oledrag <source> <destination> [x y]
            std::istringstream tail(rest);
            std::string destination_text;
            double x = 0;
            double y = 0;
            tail >> destination_text >> x >> y;
            Target source;
            Target destination;
            if (!resolve_target(in, target_text, source) ||
                !resolve_target(in, destination_text, destination)) {
                return true;
            }
            return ole_drag(in, source, destination, x, y);
        }
        if (command == "drag") {
            // drag <source> <destination> [x y]
            std::istringstream tail(rest);
            std::string destination_text;
            double x = 0;
            double y = 0;
            tail >> destination_text >> x >> y;
            Target source;
            Target destination;
            if (!resolve_target(in, target_text, source) ||
                !resolve_target(in, destination_text, destination)) {
                return true;
            }
            return drop_on(in, source, destination, x, y);
        }
        if (command == "press") {
            long code = 0;
            long shift = 0;
            long ascii = 0;
            if (!parse_key(target_text, code, shift, ascii)) {
                return true;
            }
            const ui::WindowId at = state.host->focus();
            const ui::WindowId window =
                at != ui::kNoWindow
                    ? at
                    : (state.loaded.empty() || !state.loaded.front().lock()
                           ? ui::kNoWindow
                           : control_of(*state.loaded.front().lock())->window);
            if (window == ui::kNoWindow) {
                return true;
            }
            state.host->post_event(
                ui::UiEvent{window, "KeyDown", {code, shift}});
            if (ascii != 0) {
                state.host->post_event(
                    ui::UiEvent{window, "KeyPress", {ascii}});
            }
            state.host->post_event(ui::UiEvent{window, "KeyUp", {code, shift}});
            return true;
        }
        Target target;
        if (!resolve_target(in, target_text, target)) {
            return true;  // input for a window that is gone is dropped
        }
        ControlObject* object =
            control_named(*target.form, target.control, target.index);
        if (object == nullptr || object->window == ui::kNoWindow) {
            return true;
        }
        if (command == "click") {
            state.host->post_event(ui::UiEvent{object->window, "Click", {}});
        } else if (command == "dblclick") {
            state.host->post_event(ui::UiEvent{object->window, "Click", {}});
            state.host->post_event(ui::UiEvent{object->window, "DblClick", {}});
        } else if ((command == "scroll" || command == "scrollby") &&
                   object->is_scroll()) {
            state.host->post_event(
                ui::UiEvent{object->window,
                            command == "scroll" ? "UserScroll" : "UserScrollBy",
                            {static_cast<double>(std::atof(rest.c_str()))}});
        } else if (command == "select" && object->is_list()) {
            const long index = std::atol(rest.c_str());
            state.host->post_event(
                ui::UiEvent{object->window, "UserSelect", {index}});
        } else if (command == "mousedown" || command == "mouseup" ||
                   command == "mousemove") {
            // mouse<kind> <target> [button x y]: twips inside the target.
            std::istringstream numbers(rest);
            long button = 1;
            double x = 0;
            double y = 0;
            numbers >> button >> x >> y;
            if (command == "mouseup" && state.drag_source != ui::kNoWindow) {
                state.host->post_event(
                    ui::UiEvent{object->window,
                                "DropOn",
                                {static_cast<long>(state.drag_source), x, y}});
            } else {
                state.host->post_event(
                    ui::UiEvent{object->window,
                                command == "mousedown" ? "MouseDown"
                                : command == "mouseup" ? "MouseUp"
                                                       : "MouseMove",
                                {button, 0L, x, y}});
            }
        } else if (command == "focus") {
            return focus_control(in, *target.form, target.control, target.index,
                                 in.offset_);
        } else if (command == "set" || command == "type") {
            if (command == "set") {
                const Value text{rest};
                object->values["text"] = text;
                if (is_bindable(*object) &&
                    !text_value(*object, "datafield").empty()) {
                    object->values["datachanged"] = Value{true};
                }
                state.host->set_text(object->window, rest);
                state.host->post_event(
                    ui::UiEvent{object->window, "Change", {}});
            } else {
                for (const char c : rest) {
                    const long code =
                        std::toupper(static_cast<unsigned char>(c));
                    state.host->post_event(
                        ui::UiEvent{object->window, "KeyDown", {code, 0L}});
                    state.host->post_event(ui::UiEvent{
                        object->window,
                        "KeyPress",
                        {static_cast<long>(static_cast<unsigned char>(c))}});
                    state.host->post_event(
                        ui::UiEvent{object->window, "KeyUp", {code, 0L}});
                }
            }
        } else if (command == "close") {
            return unload(in, *target.form, 0, in.offset_);
        }
        return true;
    }

    // Runs input until the script is exhausted; then closes every form.
    static void load_script(Interpreter& in) {
        auto& state = runtime(in);
        if (!state.script_loaded) {
            state.script_loaded = true;
            if (!in.ui_script_.empty()) {
                std::ifstream stream(in.ui_script_);
                std::string line;
                while (std::getline(stream, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    state.script.push_back(line);
                }
            }
        }
    }

    // Handles one queued event or one script line. `progressed` is false
    // when there was nothing to do; the result is false on a handler error.
    [[nodiscard]] static bool step(Interpreter& in, bool& progressed) {
        auto& state = runtime(in);
        load_script(in);
        progressed = true;
        if (state.ended) {
            progressed = false;
            return true;
        }
        // A scripted headless run polls sockets in `pump`, `wait` and DoEvents
        // only, so a script's order of events stays deterministic.
        if (state.headless == nullptr && in.exit_after_ms_ > 0 &&
            !state.exit_armed) {
            state.exit_armed = true;
            state.host->wake_timer(1, in.exit_after_ms_);
        }
        if (state.exit_requested) {
            progressed = false;
            return true;
        }
        bool ok = state.headless != nullptr || ocx_poll_sockets(in);
        if (!ok) {
            // A Winsock handler failed.
        } else if (const auto event = state.host->next_event(
                       state.headless == nullptr && !state.host->exhausted() &&
                       state.script_next >= state.script.size())) {
            if (event->name == "WfcExit") {
                state.exit_requested = true;
                state.host->wake_timer(1, 0);
                ok = true;
            } else if (event->name == "WfcWake") {
                ok = true;  // the poll at the top of the step has run
            } else {
                ok = dispatch(in, *event);
            }
        } else if (deliver_pending_key(in)) {
            ok = true;
        } else if (state.script_next < state.script.size()) {
            ok = run_script_line(in, state.script[state.script_next++]);
        } else {
            progressed = false;
            return true;
        }
        if (!ok && in.end_requested_) {
            // `End` in a handler stops the program without unloading forms.
            in.error_ = wfc::Evaluation{};
            state.ended = true;
            progressed = false;
            return true;
        }
        return ok;
    }

    // `Form.Show vbModal`: runs input until the form is hidden or unloaded.
    // When the input ends the user "closes" the form.
    [[nodiscard]] static bool run_modal(Interpreter& in, InstanceData& form) {
        auto* object = control_of(form);
        while (object != nullptr && object->loaded && object->shown) {
            bool progressed = false;
            if (!step(in, progressed)) {
                return false;
            }
            if (runtime(in).ended) {
                in.end_requested_ = true;  // keep ending the program
                return false;
            }
            if (!progressed) {
                return unload(in, form, 0, in.offset_);
            }
        }
        return true;
    }

    [[nodiscard]] static bool run_loop(Interpreter& in) {
        auto& state = runtime(in);
        while (true) {
            bool progressed = false;
            if (!step(in, progressed)) {
                return false;
            }
            if (!progressed) {
                break;
            }
        }
        if (state.ended) {
            return true;  // End: the forms just disappear
        }
        // The user "closes" what is left.
        auto remaining = state.loaded;
        for (auto it = remaining.rbegin(); it != remaining.rend(); ++it) {
            if (const auto form = it->lock()) {
                if (!unload(in, *form, 0, in.offset_)) {
                    if (in.end_requested_) {
                        // `End` in an Unload handler: the program just ends.
                        in.error_ = wfc::Evaluation{};
                        state.ended = true;
                        return true;
                    }
                    return false;
                }
            }
        }
        return true;
    }
};

namespace {

bool coerce_property(Interpreter& in, Value& value, const std::string& type,
                     NativeFailure& failure) {
    static_cast<void>(in);
    const auto mismatch = [&] {
        failure = NativeFailure{13, "Type mismatch"};
        return false;
    };
    if (type == "String") {
        if (is_object_reference(value) ||
            std::holds_alternative<ArrayValue>(value)) {
            return mismatch();
        }
        if (!std::holds_alternative<std::string>(value)) {
            value = Value{FormSupport::text_of(value)};
        }
        return true;
    }
    if (type == "Boolean") {
        if (const auto* flag = std::get_if<bool>(&value)) {
            static_cast<void>(flag);
            return true;
        }
        if (is_number(value)) {
            value = Value{as_double(value) != 0.0};
            return true;
        }
        return mismatch();
    }
    if (type == "Integer" || type == "Long" || type == "Single" ||
        type == "Double") {
        if (std::holds_alternative<bool>(value)) {
            value = Value{std::get<bool>(value) ? -1.0 : 0.0};
        } else if (const auto* text = std::get_if<std::string>(&value)) {
            char* end = nullptr;
            const double parsed = std::strtod(text->c_str(), &end);
            if (end == text->c_str() || *end != '\0') {
                return mismatch();
            }
            value = Value{parsed};
        } else if (!is_number(value)) {
            return mismatch();
        }
        const double number = as_double(value);
        if (type == "Integer") {
            const double rounded = std::nearbyint(number);
            if (rounded < -32768.0 || rounded > 32767.0) {
                failure = NativeFailure{6, "Overflow"};
                return false;
            }
            value = Value{static_cast<Int16>(rounded)};
        } else if (type == "Long") {
            const double rounded = std::nearbyint(number);
            if (rounded < -2147483648.0 || rounded > 2147483647.0) {
                failure = NativeFailure{6, "Overflow"};
                return false;
            }
            value = Value{static_cast<Integer>(rounded)};
        } else if (type == "Single") {
            value = Value{static_cast<float>(number)};
        } else {
            value = Value{number};
        }
        return true;
    }
    return true;  // Variant, Picture, Font, Object, ...: stored as given
}

}  // namespace

namespace {

// `Control.Font`: a view of the control's font properties.
class FontObject final : public NativeObject {
public:
    explicit FontObject(std::shared_ptr<InstanceData> owner_instance)
        : owner(std::move(owner_instance)) {}
    [[nodiscard]] std::string type_name() const override { return "Font"; }
    [[nodiscard]] bool has_member(const std::string& member) const override {
        return key_for(member) != nullptr || member == "_default" ||
               member == "hfont" || member == "clone" || member == "isequal";
    }
    [[nodiscard]] bool get(Interpreter& in, const std::string& member_name,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) override {
        const std::string member =
            member_name == "_default" ? "name" : member_name;
        const char* key = key_for(member);
        ControlObject* object = control_of(*owner);
        if (object == nullptr) {
            return false;
        }
        if (member == "hfont") {
            // A stable nonzero handle for the font's current attributes.
            std::size_t hash = 17;
            for (const char* part :
                 {"fontname", "fontsize", "fontbold", "fontitalic",
                  "fontunderline", "fontstrikethru", "fontweight",
                  "fontcharset"}) {
                const auto found = object->values.find(part);
                hash = hash * 31U +
                       (found == object->values.end()
                            ? 0U
                            : std::hash<std::string>{}(
                                  FormSupport::render_value(found->second)));
            }
            result = Value{static_cast<Integer>(0x100 + (hash % 0x7000))};
            return true;
        }
        if (member == "clone") {
            result = clone();
            return true;
        }
        if (member == "isequal") {
            const FontObject* other = nullptr;
            if (arguments.size() == 1U &&
                std::holds_alternative<ObjectInstance>(arguments[0].value)) {
                other = dynamic_cast<const FontObject*>(
                    std::get<ObjectInstance>(arguments[0].value)
                        .data->native.get());
            }
            if (other == nullptr) {
                failure = NativeFailure{13, "Type mismatch"};
                return true;
            }
            bool same = true;
            for (const char* part :
                 {"fontname", "fontsize", "fontbold", "fontitalic",
                  "fontunderline", "fontstrikethru", "fontweight",
                  "fontcharset"}) {
                Value mine;
                Value its;
                std::vector<CallArgument> none;
                NativeFailure ignored;
                const std::string name = std::string(part).substr(4);
                static_cast<void>(get(in, name, none, mine, ignored));
                static_cast<void>(const_cast<FontObject*>(other)->get(
                    in, name, none, its, ignored));
                same = same && mine == its;
            }
            result = Value{same};
            return true;
        }
        if (key == nullptr) {
            return false;
        }
        if (member == "weight") {
            const auto found = object->values.find("fontweight");
            result =
                found != object->values.end()
                    ? found->second
                    : Value{static_cast<Int16>(
                          FormSupport::flag_of(*object, "fontbold") ? 700
                                                                    : 400)};
            return true;
        }
        const auto found = object->values.find(key);
        if (found != object->values.end()) {
            result = found->second;
        } else if (const auto fallback =
                       property_default(object->class_key, key)) {
            result = *fallback;
        } else {
            result = member == "name"      ? Value{std::string{}}
                     : member == "size"    ? Value{0.0F}
                     : member == "charset" ? Value{Int16{0}}
                                           : Value{false};
        }
        return true;
    }
    [[nodiscard]] bool put(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>&, const Value& value,
                           NativeFailure& failure) override {
        const char* key = key_for(member);
        ControlObject* object = control_of(*owner);
        if (key == nullptr || object == nullptr) {
            return false;
        }
        Value converted = value;
        const std::string type = member == "name"   ? "String"
                                 : member == "size" ? "Single"
                                 : member == "weight" || member == "charset"
                                     ? "Integer"
                                     : "Boolean";
        if (!coerce_property(in, converted, type, failure)) {
            return true;
        }
        if (member == "weight") {
            object->values["fontweight"] = converted;
            converted = Value{as_double(converted) >= 700.0};
        } else if (member == "bold") {
            object->values["fontweight"] = Value{
                static_cast<Int16>(std::holds_alternative<bool>(converted) &&
                                           std::get<bool>(converted)
                                       ? 700
                                       : 400)};
        }
        const Value before =
            object->values.contains(key) ? object->values[key] : Value{Empty{}};
        object->values[key] = std::move(converted);
        if (standalone && !(before == object->values[key])) {
            // FontEvents.FontChanged(PropertyName)
            const auto self_instance = self.lock();
            if (self_instance != nullptr &&
                !self_instance->event_sinks.empty()) {
                std::string label = member;
                label[0] = static_cast<char>(
                    std::toupper(static_cast<unsigned char>(label[0])));
                std::vector<Value> argument{Value{label}};
                std::vector<CallArgument> call{
                    CallArgument{argument[0], &argument[0], false, {}}};
                if (!FormSupport::notify_sinks(in, *self_instance,
                                               "fontchanged", call)) {
                    failure = NativeFailure{kErrorAlreadyReported, {}};
                }
            }
        }
        return true;
    }
    // A copy that is its own object (`Clone`).
    [[nodiscard]] Value clone() const {
        Value copy = make_standalone_font();
        const ControlObject* from = control_of(*owner);
        auto* view = dynamic_cast<FontObject*>(
            std::get<ObjectInstance>(copy).data->native.get());
        ControlObject* to = control_of(*view->owner);
        if (from != nullptr && to != nullptr) {
            for (const char* part :
                 {"fontname", "fontsize", "fontbold", "fontitalic",
                  "fontunderline", "fontstrikethru", "fontweight",
                  "fontcharset"}) {
                const auto found = from->values.find(part);
                if (found != from->values.end()) {
                    to->values[part] = found->second;
                }
            }
        }
        return copy;
    }
    std::shared_ptr<InstanceData> owner;
    std::weak_ptr<InstanceData> self;
    bool standalone{false};

private:
    [[nodiscard]] static const char* key_for(const std::string& member) {
        if (member == "name") {
            return "fontname";
        }
        if (member == "size") {
            return "fontsize";
        }
        if (member == "bold" || member == "weight") {
            return "fontbold";
        }
        if (member == "italic") {
            return "fontitalic";
        }
        if (member == "underline") {
            return "fontunderline";
        }
        if (member == "strikethrough") {
            return "fontstrikethru";
        }
        if (member == "charset") {
            return "fontcharset";
        }
        return nullptr;
    }
};

}  // namespace

Value make_standalone_font() {
    // A font that belongs to no control: its attributes live in a hidden
    // label so FontObject works unchanged.
    auto holder = std::make_shared<InstanceData>();
    holder->class_name = "wfcnative";
    auto hidden = std::make_shared<ControlObject>(
        ui::find_control_class("Label"), "font", -1L, false);
    hidden->self = holder;
    holder->native = hidden;
    auto view = std::make_shared<FontObject>(holder);
    view->standalone = true;
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfcnative";
    data->native = view;
    view->self = data;
    return Value{ObjectInstance{std::move(data)}};
}

bool ControlObject::get(Interpreter& in, const std::string& member,
                        std::vector<CallArgument>& arguments, Value& result,
                        NativeFailure& failure) {
    if (!FormSupport::touch(in, *this, failure)) {
        return true;
    }
    if (is_activex() && FormSupport::activex_forwards(*this, member)) {
        return FormSupport::activex_get(in, *this, member, arguments, result,
                                        failure);
    }
    if (is_activex() && member == "object") {
        result = inner;
        return true;
    }
    if (comctl != nullptr &&
        ComctlSupport::get(in, *this, member, arguments, result, failure)) {
        return true;
    }
    if (class_key == "ole" &&
        FormSupport::ole_member(in, *this, member, arguments, result,
                                failure)) {
        return true;
    }
    if (member == "name") {
        result = Value{display_name};
        return true;
    }
    if (member == "hdc" && FormSupport::is_surface(*this) &&
        window != ui::kNoWindow) {
        // The surface's device context, for GDI calls through `Declare`.
        result = Value{static_cast<Integer>(
            FormSupport::runtime(in).host->surface_dc(window))};
        return true;
    }
    if (member == "index") {
        if (index < 0) {
            failure = NativeFailure{343, "Object not an array"};
            return true;
        }
        result = Value{static_cast<Int16>(index)};
        return true;
    }
    if (member == "_default") {
        const auto property = default_property_of(class_key);
        if (property.empty()) {
            return false;
        }
        return get(in, property, arguments, result, failure);
    }
    if (member == "font") {
        if (const auto owner = self.lock()) {
            auto data = std::make_shared<InstanceData>();
            data->class_name = "wfcnative";
            data->native = std::make_shared<FontObject>(owner);
            result = Value{ObjectInstance{std::move(data)}};
            return true;
        }
    }
    if (FormSupport::form_member(in, *this, member, result)) {
        // `Controls("name")`: the arguments index the returned collection.
        if (!arguments.empty()) {
            if (const auto* holder = std::get_if<ObjectInstance>(&result);
                holder != nullptr && holder->data->native != nullptr) {
                Value indexed;
                const auto keep = holder->data;
                if (keep->native->get(in, "_default", arguments, indexed,
                                      failure)) {
                    result = indexed;
                }
            }
        }
        return true;
    }
    if (FormSupport::is_surface(*this) &&
        FormSupport::surface_get(*this, member, result)) {
        return true;
    }
    if (is_list() &&
        FormSupport::list_get(in, *this, member, arguments, result, failure)) {
        return true;
    }
    if (const auto* getter = ui::find_member(*cls, member, 'g')) {
        const auto found = values.find(member);
        if (found != values.end()) {
            result = found->second;
        } else if (const auto fallback = property_default(class_key, member)) {
            result = *fallback;
        } else if (getter->type == std::string("Picture")) {
            result = make_empty_picture_value();
        } else {
            result = default_for_type(getter->type);
        }
        if (FormSupport::is_picture_property(member) &&
            std::holds_alternative<Nothing>(result)) {
            result = make_empty_picture_value();
        }
        return true;
    }
    if (ui::find_member(*cls, member, 'm') != nullptr) {
        return FormSupport::call_method(in, *this, member, arguments, result,
                                        failure);
    }
    return false;
}

bool ControlObject::put(Interpreter& in, const std::string& member,
                        std::vector<CallArgument>& arguments,
                        const Value& value, NativeFailure& failure) {
    if (!FormSupport::touch(in, *this, failure)) {
        return true;
    }
    if (is_activex() && FormSupport::activex_forwards(*this, member)) {
        return FormSupport::activex_put(in, *this, member, arguments, value,
                                        failure);
    }
    if (comctl != nullptr &&
        ComctlSupport::put(in, *this, member, arguments, value, failure)) {
        comctl_push(in, *this);
        return true;
    }
    static_cast<void>(arguments);
    if (member == "name" || member == "index") {
        failure = NativeFailure{383, "Property is read-only"};
        return true;
    }
    if (member == "font") {
        // `Set a.Font = b.Font`: copy the font properties.
        const auto* source = std::get_if<ObjectInstance>(&value);
        const auto* view =
            source != nullptr && source->data != nullptr
                ? dynamic_cast<FontObject*>(source->data->native.get())
                : nullptr;
        const ControlObject* from =
            view != nullptr ? control_of(*view->owner) : nullptr;
        if (from == nullptr) {
            failure = NativeFailure{13, "Type mismatch"};
            return true;
        }
        for (const char* key :
             {"fontname", "fontsize", "fontbold", "fontitalic", "fontunderline",
              "fontstrikethru", "fontweight", "fontcharset"}) {
            const auto found = from->values.find(key);
            if (found != from->values.end()) {
                values[key] = found->second;
            }
        }
        return true;
    }
    std::string wanted = member;
    if (member == "_default") {
        wanted = default_property_of(class_key);
    }
    if (FormSupport::is_surface(*this) &&
        FormSupport::surface_put(*this, wanted, value, failure)) {
        return true;
    }
    if (is_list() &&
        FormSupport::list_put(in, *this, wanted, arguments, value, failure)) {
        return true;
    }
    const auto* setter = ui::find_member(*cls, wanted, 'l');
    if (setter == nullptr) {
        setter = ui::find_member(*cls, wanted, 's');
    }
    if (setter == nullptr) {
        if (ui::find_member(*cls, wanted, 'g') != nullptr) {
            failure = NativeFailure{383, "Property is read-only"};
            return true;
        }
        return false;
    }
    Value converted = value;
    const std::string setter_type = setter->type;
    const bool object_type = setter_type == "Picture" ||
                             setter_type == "DataSource" ||
                             setter_type == "Font" || setter_type == "Object" ||
                             setter_type == "Control" ||
                             setter_type == "Form" || setter_type == "Variant";
    if (is_object_reference(converted) && !object_type &&
        !FormSupport::resolve(in, converted)) {
        failure = FormSupport::reported();
        return true;
    }
    if (setter_type == "Picture") {
        if (const ControlObject* source = control_of(converted);
            source != nullptr && picture_of(converted) == nullptr) {
            // `a.Picture = b` (no Set): b stands for its default property.
            const auto from = default_property_of(source->class_key);
            if (const auto held = source->values.find(from);
                !from.empty() && held != source->values.end()) {
                converted = held->second;
            } else if (!from.empty()) {
                converted = make_empty_picture_value();
            }
        }
        if (std::holds_alternative<Nothing>(converted)) {
            converted = make_empty_picture_value();
        } else if (picture_of(converted) == nullptr) {
            failure = NativeFailure{13, "Type mismatch"};
            return true;
        }
    } else if (!coerce_property(in, converted, setter_type, failure)) {
        return true;
    }
    if (is_scroll() && wanted == "value") {
        const double low = std::min(FormSupport::number_of(*this, "min"),
                                    FormSupport::number_of(*this, "max"));
        const double high = std::max(FormSupport::number_of(*this, "min"),
                                     FormSupport::number_of(*this, "max"));
        const double wanted_value = as_double(converted);
        if (wanted_value < low || wanted_value > high) {
            failure = NativeFailure{380, "Invalid property value"};
            return true;
        }
    }
    Value previous = default_for_type(setter->type);
    if (const auto found = values.find(wanted); found != values.end()) {
        previous = found->second;
    } else if (const auto fallback = property_default(class_key, wanted)) {
        previous = *fallback;
    }
    values[wanted] = std::move(converted);
    static_cast<void>(
        FormSupport::property_changed(in, *this, wanted, previous, failure));
    return true;
}

// ---------------------------------------------------------------------------
// Interpreter members

bool Interpreter::native_owns(InstanceData& instance,
                              const std::string& member) {
    if (instance.native == nullptr) {
        return false;
    }
    if (instance.class_name == "wfcnative") {
        return true;
    }
    const auto class_iterator = class_definitions_.find(instance.class_name);
    if (class_iterator != class_definitions_.end()) {
        const auto& definition = class_iterator->second;
        if (definition.methods.contains(member) ||
            definition.property_get.contains(member) ||
            definition.property_let.contains(member) ||
            definition.property_set.contains(member) ||
            definition.fields.contains(member)) {
            return false;
        }
    }
    return instance.native->has_member(member);
}

std::optional<Value> Interpreter::native_get_member(
    InstanceData& instance, const std::string& name,
    const std::size_t member_offset) {
    auto arguments = parse_call_argument_list();
    if (!arguments.has_value()) {
        return std::nullopt;
    }
    return native_get_member_with(instance, name, *arguments, member_offset);
}

std::optional<Value> Interpreter::native_get_member_with(
    InstanceData& instance, const std::string& name,
    std::vector<CallArgument>& arguments, const std::size_t member_offset) {
    if (!execute_) {
        return Value{Empty{}};
    }
    const auto keep = instance.shared_from_this();
    Value result{Empty{}};
    NativeFailure failure;
    const std::size_t saved_offset = offset_;
    offset_ = member_offset;
    const bool known =
        keep->native->get(*this, name, arguments, result, failure);
    offset_ = saved_offset;
    if (!known) {
        static_cast<void>(
            raise_runtime(438, "Object doesn't support this property or method",
                          member_offset));
        return std::nullopt;
    }
    if (failure.number == kErrorAlreadyReported) {
        return std::nullopt;
    }
    if (failure.number != 0) {
        static_cast<void>(
            raise_runtime(failure.number, failure.description, member_offset));
        return std::nullopt;
    }
    return result;
}

bool Interpreter::native_member_statement(const Value& base,
                                          const std::string& name,
                                          const std::size_t member_offset) {
    InstanceData& instance = *std::get<ObjectInstance>(base).data;
    if (name == "line" || name == "circle" || name == "pset" ||
        name == "scale") {
        const ControlObject* surface = control_of(instance);
        if (surface != nullptr && FormSupport::is_surface(*surface)) {
            return parse_surface_statement(instance, name, member_offset);
        }
    }
    if (name == "print") {
        const ControlObject* surface = control_of(instance);
        if (surface != nullptr && FormSupport::is_surface(*surface)) {
            print_surface_ = &instance;
            return parse_print_statement();
        }
    }
    skip_horizontal_whitespace();
    std::vector<CallArgument> arguments;
    bool parenthesized = false;
    if (!at_end() && current() == '(') {
        auto parsed = parse_call_argument_list();
        if (!parsed.has_value()) {
            return false;
        }
        arguments = std::move(*parsed);
        parenthesized = true;
        skip_horizontal_whitespace();
    }
    // `form.Control(i).Prop = x` / `form.Child.Prop = x`: read, then go on.
    if (!at_end() && current() == '.') {
        Value child{Nothing{}};
        if (execute_) {
            auto read = native_get_member_with(instance, name, arguments,
                                               member_offset);
            if (!read.has_value()) {
                return false;
            }
            child = std::move(*read);
        }
        const std::string temp_name =
            "wfcchain" + std::to_string(chain_counter_++);
        current_scope().variables.insert_or_assign(
            temp_name, std::holds_alternative<ObjectInstance>(child)
                           ? child
                           : Value{Nothing{}});
        const bool chained_ok =
            parse_assignment_or_array_element(temp_name, '\0');
        current_scope().variables.erase(temp_name);
        return chained_ok;
    }
    if (!at_end() && current() == '=') {
        advance();
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (!execute_) {
            return true;
        }
        const auto keep = instance.shared_from_this();
        NativeFailure failure;
        const std::size_t saved_offset = offset_;
        offset_ = member_offset;
        const bool known =
            keep->native->put(*this, name, arguments, *value, failure);
        offset_ = saved_offset;
        if (!known) {
            static_cast<void>(raise_runtime(
                438, "Object doesn't support this property or method",
                member_offset));
            return false;
        }
        if (failure.number == kErrorAlreadyReported) {
            return false;
        }
        if (failure.number != 0) {
            static_cast<void>(raise_runtime(failure.number, failure.description,
                                            member_offset));
            return false;
        }
        return true;
    }
    // A method call statement.
    if (!parenthesized && !at_statement_end()) {
        bare_call_arguments_ = true;
        auto parsed = parse_call_argument_list();
        bare_call_arguments_ = false;
        if (!parsed.has_value()) {
            return false;
        }
        arguments = std::move(*parsed);
    }
    return native_get_member_with(instance, name, arguments, member_offset)
        .has_value();
}

bool Interpreter::native_set_member(const Value& base, const std::string& name,
                                    const std::size_t member_offset) {
    InstanceData& instance = *std::get<ObjectInstance>(base).data;
    skip_horizontal_whitespace();
    std::vector<CallArgument> arguments;
    if (!at_end() && current() == '(') {
        auto parsed = parse_call_argument_list();
        if (!parsed.has_value()) {
            return false;
        }
        arguments = std::move(*parsed);
        skip_horizontal_whitespace();
    }
    if (!consume('=')) {
        set_error("WFC0014", "expected assignment operator", offset_);
        return false;
    }
    skip_horizontal_whitespace();
    auto value = parse_expression();
    if (!value.has_value()) {
        return false;
    }
    if (!execute_) {
        return true;
    }
    const auto keep = instance.shared_from_this();
    NativeFailure failure;
    const std::size_t saved_offset = offset_;
    offset_ = member_offset;
    const bool known =
        keep->native->put(*this, name, arguments, *value, failure);
    offset_ = saved_offset;
    if (!known) {
        static_cast<void>(
            raise_runtime(438, "Object doesn't support this property or method",
                          member_offset));
        return false;
    }
    if (failure.number == kErrorAlreadyReported) {
        return false;
    }
    if (failure.number != 0) {
        static_cast<void>(
            raise_runtime(failure.number, failure.description, member_offset));
        return false;
    }
    return true;
}

std::optional<Value> Interpreter::native_implicit_member(
    const std::string& name) {
    auto* const instance = current_instance();
    if (instance == nullptr || instance->native == nullptr ||
        instance->class_name == "wfcnative" || !native_owns(*instance, name)) {
        return std::nullopt;
    }
    return Value{ObjectInstance{instance->shared_from_this()}};
}

bool Interpreter::create_form_instance(
    const std::shared_ptr<InstanceData>& instance, const ClassDef& def,
    const std::size_t offset) {
    const ui::FrmObject& root = def.form->root;
    const std::string root_class =
        root.type.rfind("VB.", 0) == 0 ? root.type.substr(3) : root.type;
    const auto* form_class = ui::find_control_class(root_class);
    if (form_class == nullptr) {
        static_cast<void>(raise_runtime(
            429, "ActiveX component can't create object", offset));
        return false;
    }
    const std::string frx_directory = def.form_directory;
    const auto convert = [&](const ui::FrmProperty& source,
                             const ui::ControlClass& owner,
                             std::string property) -> std::optional<Value> {
        property = lower_text(property);
        std::string raw = source.value;
        if (source.in_resource) {
            std::string bytes;
            const std::string path =
                (frx_directory.empty() ? std::string{} : frx_directory + "/") +
                source.resource_file;
            if (ui::read_frx_record(path, source.resource_offset, bytes)) {
                raw = bytes;
            } else {
                return std::nullopt;
            }
        }
        if (FormSupport::is_picture_property(property)) {
            PictureInfo info;
            if (!raw.empty() && read_picture_header(raw, info)) {
                return make_picture_value(raw, info);
            }
            return std::nullopt;
        }
        const auto* row = ui::find_member(owner, property, 'l');
        if (row == nullptr) {
            row = ui::find_member(owner, property, 'g');
        }
        const std::string type = row != nullptr ? row->type : "Variant";
        Value value;
        if (source.is_string || source.in_resource) {
            value = Value{raw};
        } else if (!raw.empty() && raw[0] == '&') {
            value = Value{static_cast<Integer>(
                std::strtoul(raw.c_str() + 2, nullptr, 16))};
        } else {
            value = Value{std::strtod(raw.c_str(), nullptr)};
        }
        NativeFailure ignored;
        if (!coerce_property(*this, value, type, ignored)) {
            return std::nullopt;
        }
        return value;
    };
    const auto apply = [&](ControlObject& object, const ui::FrmObject& source) {
        for (const auto& property : source.properties) {
            const std::string key = lower_text(property.name);
            if (!property.nested.empty()) {
                // A Font group: Name, Size, Weight, Italic, ...
                for (const auto& part : property.nested) {
                    const std::string part_key = lower_text(part.name);
                    const std::string target =
                        part_key == "name"            ? "fontname"
                        : part_key == "size"          ? "fontsize"
                        : part_key == "weight"        ? "fontbold"
                        : part_key == "italic"        ? "fontitalic"
                        : part_key == "underline"     ? "fontunderline"
                        : part_key == "strikethrough" ? "fontstrikethru"
                                                      : std::string{};
                    if (target.empty()) {
                        continue;
                    }
                    if (part_key == "weight") {
                        // Weight 700 and up is bold.
                        object.values[target] = Value{
                            std::strtod(part.value.c_str(), nullptr) >= 700.0};
                    } else if (auto value =
                                   convert(part, *object.cls, target)) {
                        object.values[target] = std::move(*value);
                    }
                }
                continue;
            }
            if (key == "list" && property.in_resource && object.is_list()) {
                // A list record: a 16-bit count, then each item as a 16-bit
                // length and its text.
                std::string bytes;
                const std::string path =
                    (frx_directory.empty() ? std::string{}
                                           : frx_directory + "/") +
                    property.resource_file;
                if (!ui::read_frx_record(path, property.resource_offset,
                                         bytes) ||
                    bytes.size() < 2U) {
                    continue;
                }
                const auto read16 = [&](const std::size_t at) {
                    return static_cast<std::size_t>(
                        static_cast<unsigned char>(bytes[at]) |
                        (static_cast<unsigned char>(bytes[at + 1U]) << 8U));
                };
                const std::size_t count = read16(0);
                std::size_t at = 2;
                for (std::size_t i = 0; i < count && at + 2U <= bytes.size();
                     ++i) {
                    const std::size_t length = read16(at);
                    at += 2U;
                    if (at + length > bytes.size()) {
                        break;
                    }
                    object.items.push_back(bytes.substr(at, length));
                    at += length;
                }
                object.chosen.assign(object.items.size(), false);
                continue;
            }
            if (key == "index") {
                continue;  // consumed by the control-array logic
            }
            if (auto value = convert(property, *object.cls, key)) {
                object.values[key] = std::move(*value);
            }
        }
        // Form size: ClientWidth/ClientHeight stand for Width/Height.
        if (object.is_form) {
            static const std::pair<const char*, const char*> sizes[] = {
                {"clientwidth", "width"}, {"clientheight", "height"}};
            for (const auto& [from, to] : sizes) {
                if (object.values.contains(from) &&
                    !object.values.contains(to)) {
                    object.values[to] = object.values[from];
                }
            }
        }
        // A design-time scale that differs from the client area makes the
        // scale mode "user".
        if (FormSupport::is_surface(object) &&
            !object.values.contains("scalemode") &&
            (object.values.contains("scalewidth") ||
             object.values.contains("scaleheight"))) {
            const auto width = object.values.find("scalewidth");
            const auto height = object.values.find("scaleheight");
            const bool differs = (width != object.values.end() &&
                                  as_double(width->second) !=
                                      FormSupport::client_width(object)) ||
                                 (height != object.values.end() &&
                                  as_double(height->second) !=
                                      FormSupport::client_height(object));
            if (differs) {
                object.values["scalemode"] = Value{Int16{0}};
            }
        }
        object.design = object.values;
    };

    auto form_object =
        std::make_shared<ControlObject>(form_class, root.name, -1L, true);
    form_object->form = instance;
    form_object->self = instance;
    apply(*form_object, root);
    instance->native = form_object;

    // Controls, in file order; containers contribute their children.
    const auto native_class =
        [&](const std::string& type) -> const ui::ControlClass* {
        std::string leaf = type;
        if (leaf.rfind("VB.", 0) == 0) {
            leaf = leaf.substr(3);
        }
        if (const auto* intrinsic = ui::find_control_class(leaf)) {
            return intrinsic;
        }
        const auto dot = leaf.rfind('.');
        if (dot != std::string::npos) {
            return library_control_class(leaf.substr(0, dot),
                                         leaf.substr(dot + 1U));
        }
        return nullptr;
    };
    long next_order = 0;
    const auto make_control = [&](const ui::FrmObject& source,
                                  const ui::ControlClass& control_class,
                                  const long array_index,
                                  const ui::FrmObject& parent) {
        auto object = std::make_shared<ControlObject>(
            &control_class, source.name, array_index, false);
        object->form = instance;
        ComctlSupport::init(*object);
        object->order = next_order++;
        if (&control_class == &kExtenderClass) {
            const auto proxy = class_definitions_.find(lower_text(source.type));
            if (proxy != class_definitions_.end()) {
                object->activex_clsid = proxy->second.com_clsid;
                object->type_label = proxy->second.display_name;
            } else {
                const auto dot = source.type.rfind('.');
                const auto local = class_definitions_.find(lower_text(
                    dot == std::string::npos ? source.type
                                             : source.type.substr(dot + 1U)));
                if (local != class_definitions_.end()) {
                    object->type_label = local->second.display_name;
                }
            }
        }
        if (&parent != &root) {
            object->container = lower_text(parent.name);
        }
        apply(*object, source);
        comctl_load_persisted(*this, *object, source, frx_directory);
        auto data = std::make_shared<InstanceData>();
        data->class_name = "wfcnative";
        data->native = object;
        object->self = data;
        return Value{ObjectInstance{std::move(data)}};
    };
    std::map<std::string, std::shared_ptr<InstanceData>> arrays;
    const auto add_controls = [&](const auto& self,
                                  const ui::FrmObject& parent) -> void {
        for (const auto& child : parent.children) {
            const auto* control_class = native_class(child.type);
            std::string clsid;
            std::string user_control;
            if (control_class == nullptr) {
                // A UserControl of the project (`Library.Class`), or an
                // ActiveX control of a referenced type library.
                const auto dot = child.type.rfind('.');
                const std::string leaf = lower_text(
                    dot == std::string::npos ? child.type
                                             : child.type.substr(dot + 1U));
                const auto proxy =
                    class_definitions_.find(lower_text(child.type));
                const auto local = class_definitions_.find(leaf);
                if (proxy != class_definitions_.end() &&
                    !proxy->second.com_clsid.empty()) {
                    control_class = &kExtenderClass;
                    clsid = proxy->second.com_clsid;
                } else if (local != class_definitions_.end() &&
                           local->second.form != nullptr &&
                           local->second.form->root.type == "VB.UserControl") {
                    control_class = &kExtenderClass;
                    user_control = leaf;
                } else {
                    continue;
                }
            }
            const std::string key = lower_text(child.name);
            const auto* index_property = child.find("Index");
            if (index_property != nullptr) {
                const long array_index =
                    std::strtol(index_property->value.c_str(), nullptr, 10);
                auto found = arrays.find(key);
                if (found == arrays.end()) {
                    auto data = std::make_shared<InstanceData>();
                    data->class_name = "wfcnative";
                    data->native = std::make_shared<ControlArray>();
                    found = arrays.emplace(key, data).first;
                    instance->fields.variables[key] =
                        Value{ObjectInstance{data}};
                    instance->fields.object_variables.insert(key);
                }
                dynamic_cast<ControlArray*>(found->second->native.get())
                    ->elements[array_index] =
                    make_control(child, *control_class, array_index, parent);
                if (!user_control.empty()) {
                    control_of(
                        dynamic_cast<ControlArray*>(found->second->native.get())
                            ->elements[array_index])
                        ->usercontrol_class = user_control;
                }
            } else {
                instance->fields.variables[key] =
                    make_control(child, *control_class, -1L, parent);
                if (!user_control.empty()) {
                    control_of(instance->fields.variables[key])
                        ->usercontrol_class = user_control;
                }
                instance->fields.object_variables.insert(key);
            }
            self(self, child);
        }
    };
    add_controls(add_controls, root);
    // Form_Initialize / UserControl_Initialize: the instance now exists.
    static std::set<const InstanceData*> initializing;
    if (initializing.insert(instance.get()).second) {
        std::vector<Value> none;
        const bool ok = FormSupport::raise(*this, *instance, "", -1,
                                           "initialize", none, offset);
        initializing.erase(instance.get());
        return ok;
    }
    return true;
}

bool Interpreter::parse_load_unload_statement(const bool unload) {
    skip_horizontal_whitespace();
    const auto offset = offset_;
    auto identifier = parse_identifier(nullptr);
    if (!identifier.has_value()) {
        set_error("WFC0011", "expected a form name", offset);
        return false;
    }
    {
        // `Load Control(index)` / `Unload Control(index)`: a control array
        // element.
        const auto after_name = offset_;
        skip_horizontal_whitespace();
        if (!at_end() && current() == '(') {
            const auto variable = find_variable(*identifier);
            const auto* holder =
                variable.value != nullptr
                    ? std::get_if<ObjectInstance>(variable.value)
                    : nullptr;
            auto* array =
                holder != nullptr && holder->data != nullptr
                    ? dynamic_cast<ControlArray*>(holder->data->native.get())
                    : nullptr;
            if (array != nullptr) {
                advance();
                skip_horizontal_whitespace();
                auto index_value = parse_expression();
                if (!index_value.has_value()) {
                    return false;
                }
                skip_horizontal_whitespace();
                if (!consume(')')) {
                    set_error("WFC0005", "expected closing parenthesis",
                              offset_);
                    return false;
                }
                if (!execute_) {
                    return true;
                }
                const auto wanted = whole_value(*index_value);
                if (!wanted.has_value()) {
                    set_error("WFC0073", "Type mismatch", offset);
                    return false;
                }
                const auto keep = holder->data;
                return FormSupport::array_element(*this, *array, *wanted,
                                                  unload, offset);
            }
        }
        offset_ = after_name;
    }
    Value target{Nothing{}};
    if (*identifier == "me") {
        auto me = me_value(offset);
        if (!me.has_value()) {
            return false;
        }
        target = std::move(*me);
    } else {
        const auto variable = find_variable(*identifier);
        if (variable.value == nullptr) {
            set_error("WFC0015", "undeclared variable", offset);
            return false;
        }
        target = *variable.value;
    }
    if (!execute_) {
        return true;
    }
    const auto* holder = std::get_if<ObjectInstance>(&target);
    if (holder == nullptr || holder->data == nullptr ||
        control_of(*holder->data) == nullptr ||
        !control_of(*holder->data)->is_form) {
        static_cast<void>(raise_runtime(
            91, "Object variable or With block variable not set", offset));
        return false;
    }
    const auto keep = holder->data;
    return unload ? FormSupport::unload(*this, *keep, 1, offset)
                  : FormSupport::ensure_loaded(*this, *keep, offset);
}

namespace {

// The global `Forms` collection.
class FormsCollection final : public NativeObject {
public:
    [[nodiscard]] std::string type_name() const override { return "Object"; }
    [[nodiscard]] bool has_member(const std::string& member) const override {
        return member == "count" || member == "item" || member == "_default";
    }
    [[nodiscard]] bool get(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) override {
        std::vector<Value> forms;
        static_cast<void>(enumerate(in, forms));
        if (member == "count") {
            result = Value{static_cast<Integer>(forms.size())};
            return true;
        }
        if (member == "item" || member == "_default") {
            const auto wanted = arguments.size() == 1U
                                    ? whole_value(arguments[0].value)
                                    : std::nullopt;
            if (!wanted.has_value() || *wanted < 0 ||
                static_cast<std::size_t>(*wanted) >= forms.size()) {
                failure = NativeFailure{9, "Subscript out of range"};
                return true;
            }
            result = forms[static_cast<std::size_t>(*wanted)];
            return true;
        }
        return false;
    }
    [[nodiscard]] bool put(Interpreter&, const std::string&,
                           std::vector<CallArgument>&, const Value&,
                           NativeFailure&) override {
        return false;
    }
    [[nodiscard]] bool enumerate(Interpreter& in,
                                 std::vector<Value>& items) override {
        FormSupport::loaded_forms(in, items);
        return true;
    }
};

}  // namespace

namespace {

// The `Controls` collection of a form.
class ControlsCollection final : public NativeObject {
public:
    explicit ControlsCollection(std::weak_ptr<InstanceData> owner)
        : form(std::move(owner)) {}
    [[nodiscard]] std::string type_name() const override { return "Controls"; }
    [[nodiscard]] bool has_member(const std::string& member) const override {
        return member == "count" || member == "item" || member == "_default" ||
               member == "add" || member == "remove";
    }
    [[nodiscard]] bool get(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) override {
        if (member == "add" || member == "remove") {
            return FormSupport::controls_edit(in, form.lock(), member,
                                              arguments, result, failure);
        }
        std::vector<Value> controls;
        static_cast<void>(enumerate(in, controls));
        if (member == "count") {
            result = Value{static_cast<Integer>(controls.size())};
            return true;
        }
        if (member != "item" && member != "_default") {
            return false;
        }
        if (arguments.size() != 1U) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        if (const auto* name = std::get_if<std::string>(&arguments[0].value)) {
            const std::string wanted = lower_text(*name);
            for (const auto& entry : controls) {
                const ControlObject* object = control_of(entry);
                if (object != nullptr && object->name == wanted) {
                    result = entry;
                    return true;
                }
            }
            failure = NativeFailure{343, "Object not found"};
            return true;
        }
        const auto position = whole_value(arguments[0].value);
        if (!position.has_value() || *position < 0 ||
            static_cast<std::size_t>(*position) >= controls.size()) {
            failure = NativeFailure{9, "Subscript out of range"};
            return true;
        }
        result = controls[static_cast<std::size_t>(*position)];
        return true;
    }
    [[nodiscard]] bool put(Interpreter&, const std::string&,
                           std::vector<CallArgument>&, const Value&,
                           NativeFailure&) override {
        return false;
    }
    [[nodiscard]] bool enumerate(Interpreter& in,
                                 std::vector<Value>& items) override {
        static_cast<void>(in);
        const auto owner = form.lock();
        if (owner == nullptr) {
            return true;
        }
        // Creation order; array elements follow their array's position.
        std::vector<std::pair<long, Value>> ordered;
        FormSupport::for_each_control(*owner, [&](ControlObject& object) {
            const Value* held = nullptr;
            const auto direct = owner->fields.variables.find(object.name);
            if (direct != owner->fields.variables.end() && object.index < 0) {
                held = &direct->second;
            } else if (direct != owner->fields.variables.end()) {
                const auto* holder =
                    std::get_if<ObjectInstance>(&direct->second);
                if (holder != nullptr) {
                    if (auto* array = dynamic_cast<ControlArray*>(
                            holder->data->native.get())) {
                        const auto element = array->elements.find(object.index);
                        if (element != array->elements.end()) {
                            held = &element->second;
                        }
                    }
                }
            }
            if (held != nullptr) {
                ordered.emplace_back(object.order * 100000L + object.index + 1,
                                     *held);
            }
        });
        std::sort(ordered.begin(), ordered.end(),
                  [](const auto& left, const auto& right) {
                      return left.first < right.first;
                  });
        for (auto& entry : ordered) {
            items.push_back(std::move(entry.second));
        }
        return true;
    }
    std::weak_ptr<InstanceData> form;
};

}  // namespace

Value FormSupport::control_value(InstanceData& form,
                                 const ControlObject& object) {
    const auto direct = form.fields.variables.find(object.name);
    if (direct == form.fields.variables.end()) {
        return Value{Nothing{}};
    }
    if (object.index < 0) {
        return direct->second;
    }
    if (const auto* holder = std::get_if<ObjectInstance>(&direct->second)) {
        if (auto* array =
                dynamic_cast<ControlArray*>(holder->data->native.get())) {
            const auto element = array->elements.find(object.index);
            if (element != array->elements.end()) {
                return element->second;
            }
        }
    }
    return Value{Nothing{}};
}

bool FormSupport::array_element(Interpreter& in, ControlArray& array,
                                const long index, const bool unload,
                                const std::size_t offset) {
    ControlObject* first = array.elements.empty()
                               ? nullptr
                               : control_of(array.elements.begin()->second);
    const auto fail = [&](const Integer number, const std::string& text) {
        static_cast<void>(in.raise_runtime(number, text, offset));
        return false;
    };
    if (first == nullptr) {
        return fail(340, "Control array element doesn't exist");
    }
    const auto owner = first->form.lock();
    if (owner == nullptr) {
        return fail(340, "Control array element doesn't exist");
    }
    const auto existing = array.elements.find(index);
    if (unload) {
        if (existing == array.elements.end()) {
            return fail(340, "Control array element '" + std::to_string(index) +
                                 "' doesn't exist");
        }
        ControlObject* object = control_of(existing->second);
        if (object == nullptr || !object->dynamic) {
            return fail(361, "Can't load or unload this object");
        }
        auto& state = runtime(in);
        if (object->window != ui::kNoWindow) {
            state.bindings.erase(object->window);
            state.host->destroy_window(object->window);
        }
        array.elements.erase(existing);
        return true;
    }
    if (existing != array.elements.end()) {
        return fail(360, "Object already loaded");
    }
    auto object = std::make_shared<ControlObject>(
        first->cls, first->display_name, index, false);
    object->form = first->form;
    object->container = first->container;
    object->values = first->design;
    object->design = first->design;
    object->values["visible"] = Value{false};
    object->design["visible"] = Value{false};
    object->order = first->order;
    object->activex_clsid = first->activex_clsid;
    object->dynamic = true;
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfcnative";
    data->native = object;
    object->self = data;
    array.elements[index] = Value{ObjectInstance{std::move(data)}};
    const ControlObject* owner_object = control_of(*owner);
    if (owner_object != nullptr && owner_object->loaded) {
        object->window = runtime(in).host->create_window(
            object->is_activex() ? "ActiveX:" + object->activex_clsid
                                 : window_kind(*object),
            parent_window(*owner, *owner_object, *object));
        bind(in, *object);
        mirror_all(in, *object);
        if (object->is_activex() && !attach_activex(in, *owner, *object)) {
            return false;
        }
    }
    return true;
}

// Controls.Add(ProgID, Name[, Container]) and Controls.Remove(Name).
bool FormSupport::controls_edit(Interpreter& in,
                                const std::shared_ptr<InstanceData>& form,
                                const std::string& member,
                                std::vector<CallArgument>& arguments,
                                Value& result, NativeFailure& failure) {
    result = Value{Empty{}};
    if (form == nullptr) {
        failure = NativeFailure{91,
                                "Object variable or With block variable "
                                "not set"};
        return true;
    }
    const auto text_of = [&](const std::size_t index) {
        return index < arguments.size() && std::holds_alternative<std::string>(
                                               arguments[index].value)
                   ? std::get<std::string>(arguments[index].value)
                   : std::string{};
    };
    const std::string name = lower_text(text_of(member == "add" ? 1U : 0U));
    auto& runtime_state = runtime(in);
    const ControlObject* owner = control_of(*form);
    if (member == "remove") {
        const auto held = form->fields.variables.find(name);
        ControlObject* object = held == form->fields.variables.end()
                                    ? nullptr
                                    : control_of(held->second);
        if (object == nullptr || !object->dynamic || object->index >= 0) {
            failure = NativeFailure{361, "Can't load or unload this object"};
            return true;
        }
        if (object->window != ui::kNoWindow) {
            runtime_state.bindings.erase(object->window);
            runtime_state.host->destroy_window(object->window);
        }
        form->fields.variables.erase(held);
        form->fields.object_variables.erase(name);
        return true;
    }
    if (arguments.size() < 2U || arguments.size() > 3U) {
        failure = NativeFailure{450, "Wrong number of arguments"};
        return true;
    }
    std::string progid = lower_text(text_of(0));
    if (progid.rfind("vb.", 0) == 0) {
        progid = progid.substr(3);
    }
    static const std::set<std::string> addable = {
        "picturebox",    "label",      "textbox",      "frame",
        "commandbutton", "checkbox",   "optionbutton", "combobox",
        "listbox",       "hscrollbar", "vscrollbar",   "timer",
        "drivelistbox",  "dirlistbox", "filelistbox",  "shape",
        "line",          "image"};
    const ui::ControlClass* control_class =
        addable.contains(progid) ? ui::find_control_class(progid) : nullptr;
    std::string clsid;
    if (control_class == nullptr) {
        // An ActiveX control by ProgID or CLSID.
        clsid = in.com_progid_to_clsid(text_of(0));
        if (!clsid.empty()) {
            control_class = &kExtenderClass;
        }
    }
    if (control_class == nullptr) {
        failure = NativeFailure{711,
                                "Invalid class string. Looking for "
                                "object with ProgID: " +
                                    text_of(0)};
        return true;
    }
    if (name.empty() || form->fields.variables.contains(name)) {
        failure =
            NativeFailure{727, "There is already a control with the name '" +
                                   text_of(1) + "'"};
        return true;
    }
    auto object =
        std::make_shared<ControlObject>(control_class, text_of(1), -1L, false);
    object->form = form;
    object->activex_clsid = clsid;
    long order = 0;
    for_each_control(*form, [&](ControlObject& other) {
        order = std::max(order, other.order + 1);
    });
    object->order = order;
    object->dynamic = true;
    if (arguments.size() == 3U) {
        if (const ControlObject* holder = control_of(arguments[2].value)) {
            object->container = holder->name;
        }
    }
    object->values["visible"] = Value{false};
    object->design = object->values;
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfcnative";
    data->native = object;
    object->self = data;
    result = Value{ObjectInstance{data}};
    form->fields.variables[name] = result;
    form->fields.object_variables.insert(name);
    if (owner != nullptr && owner->loaded) {
        object->window = runtime_state.host->create_window(
            object->is_activex() ? "ActiveX:" + object->activex_clsid
                                 : window_kind(*object),
            parent_window(*form, *owner, *object));
        bind(in, *object);
        mirror_all(in, *object);
        if (object->is_activex() && !attach_activex(in, *form, *object)) {
            failure = reported();
            runtime_state.bindings.erase(object->window);
            runtime_state.host->destroy_window(object->window);
            form->fields.variables.erase(name);
            form->fields.object_variables.erase(name);
            return true;
        }
    }
    return true;
}

bool FormSupport::form_member(Interpreter& in, ControlObject& self,
                              const std::string& member, Value& result) {
    const auto form = self.form.lock();
    if (form == nullptr) {
        return false;
    }
    if (self.class_key == "usercontrol" && member == "extender") {
        result = self.extender;
        return true;
    }
    if (self.class_key == "usercontrol" && member == "ambient") {
        if (auto made = in.instantiate_class("wfcambient", in.offset_)) {
            result = std::move(*made);
        } else {
            result = Value{Nothing{}};
        }
        return true;
    }
    if (self.class_key == "usercontrol" && member == "propertychanged") {
        result = Value{Empty{}};  // there is no designer to notify
        return true;
    }
    if (self.is_form && member == "controls") {
        auto data = std::make_shared<InstanceData>();
        data->class_name = "wfcnative";
        data->native = std::make_shared<ControlsCollection>(form);
        result = Value{ObjectInstance{std::move(data)}};
        return true;
    }
    if (self.is_form && self.class_key == "mdiform" && member == "activeform") {
        result = Value{Nothing{}};
        const auto children = mdi_children(in);
        const auto& state = runtime(in);
        const auto binding = state.bindings.find(state.host->focus());
        std::shared_ptr<InstanceData> active;
        if (binding != state.bindings.end()) {
            const auto owner = binding->second.form.lock();
            const ControlObject* owner_object =
                owner ? control_of(*owner) : nullptr;
            if (owner_object != nullptr && is_mdi_child(*owner_object)) {
                active = owner;
            }
        }
        if (active == nullptr) {
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                const ControlObject* child = control_of(**it);
                if (child != nullptr && child->shown) {
                    active = *it;
                    break;
                }
            }
        }
        if (active != nullptr) {
            result = Value{ObjectInstance{active}};
        }
        return true;
    }
    if (self.is_form && member == "activecontrol") {
        result = Value{Nothing{}};
        const auto& state = runtime(in);
        const auto binding = state.bindings.find(state.host->focus());
        if (binding != state.bindings.end() &&
            binding->second.form.lock() == form &&
            !binding->second.control.empty()) {
            if (const ControlObject* focused = control_named(
                    *form, binding->second.control, binding->second.index)) {
                result = control_value(*form, *focused);
            }
        }
        return true;
    }
    if (!self.is_form && member == "parent") {
        result = Value{ObjectInstance{form}};
        return true;
    }
    if (!self.is_form && member == "container") {
        result = Value{ObjectInstance{form}};
        if (!self.container.empty()) {
            if (const ControlObject* holder =
                    control_named(*form, self.container, -1)) {
                result = control_value(*form, *holder);
            }
        }
        return true;
    }
    return false;
}

std::string lower_native_type(const InstanceData& instance) {
    return instance.native != nullptr ? lower_text(instance.native->type_name())
                                      : std::string{};
}

void Interpreter::register_native_types() {
    std::set<std::string> mentioned;
    const auto collect = [&](const std::string_view text) {
        std::string word;
        for (const char c : text) {
            if (is_identifier_part(c)) {
                word.push_back(ascii_lower(c));
            } else if (!word.empty()) {
                mentioned.insert(std::move(word));
                word.clear();
            }
        }
        if (!word.empty()) {
            mentioned.insert(std::move(word));
        }
    };
    collect(source_);
    std::set<std::string> user_classes;
    for (const auto& module : class_sources_) {
        collect(module.source);
        user_classes.insert(lower_text(module.name));
    }
    std::vector<std::string> names = {
        "control",           "form",    "mdiform", "stdpicture",   "picture",
        "vbcontrolextender", "stdfont", "font",    "ipicturedisp", "ifontdisp"};
    for (const auto& control_class : ui::control_classes()) {
        names.push_back(lower_text(control_class.name));
    }
    // The MSComctlLib controls, collections and value objects accept any
    // control or item object in an `As` clause.
    std::set<std::string> common_types;
    for (auto& name : comctl_type_names()) {
        common_types.insert(name);
        names.push_back(name);
    }
    for (const auto& name : names) {
        if (!mentioned.contains(name) || user_classes.contains(name) ||
            class_definitions_.contains(name)) {
            continue;
        }
        ClassDef proxy;
        proxy.display_name = name;
        proxy.native_type = common_types.contains(name) ? "control" : name;
        if (common_types.contains(name)) {
            const std::string library = name == "winsock" ? "mswinsocklib."
                                        : name == "commondialog"
                                            ? "mscomdlg."
                                            : "mscomctllib.";
            class_definitions_.emplace(library + name, proxy);
        }
        class_definitions_.emplace(name, std::move(proxy));
    }
}

void Interpreter::form_refresh_surfaces() {
    if (forms_ && forms_->host) {
        forms_->host->refresh_surfaces();
    }
}

Value Interpreter::form_active(const int what) {
    if (!forms_) {
        return Value{Nothing{}};
    }
    const auto& state = *forms_;
    std::shared_ptr<InstanceData> form;
    const ControlObject* control = nullptr;
    const auto binding = state.bindings.find(state.host->focus());
    if (binding != state.bindings.end()) {
        form = binding->second.form.lock();
        if (form != nullptr && !binding->second.control.empty()) {
            control = FormSupport::control_named(*form, binding->second.control,
                                                 binding->second.index);
        }
    }
    if (form == nullptr) {
        for (auto it = state.loaded.rbegin(); it != state.loaded.rend(); ++it) {
            const auto candidate = it->lock();
            const ControlObject* object =
                candidate ? control_of(*candidate) : nullptr;
            if (object != nullptr && object->shown) {
                form = candidate;
                break;
            }
        }
    }
    if (form == nullptr) {
        return Value{Nothing{}};
    }
    if (what == 0) {
        return Value{ObjectInstance{form}};
    }
    return control != nullptr ? FormSupport::control_value(*form, *control)
                              : Value{Nothing{}};
}

std::shared_ptr<NativeObject> make_forms_collection() {
    return std::make_shared<FormsCollection>();
}

namespace {

// The host window a dialog belongs to: the focused form, else the first.
ui::WindowId dialog_owner(Interpreter& in, FormRuntime& state) {
    static_cast<void>(in);
    const auto binding = state.bindings.find(state.host->focus());
    if (binding != state.bindings.end()) {
        if (const auto form = binding->second.form.lock()) {
            if (const ControlObject* object = control_of(*form)) {
                return object->window;
            }
        }
    }
    for (const auto& weak : state.loaded) {
        if (const auto form = weak.lock()) {
            if (const ControlObject* object = control_of(*form)) {
                return object->window;
            }
        }
    }
    return ui::kNoWindow;
}

}  // namespace

bool Interpreter::parse_sendkeys_statement(const std::size_t offset) {
    skip_horizontal_whitespace();
    auto keys = parse_expression();
    if (!keys.has_value()) {
        return false;
    }
    skip_horizontal_whitespace();
    bool wait = false;
    if (consume(',')) {
        skip_horizontal_whitespace();
        auto flag = parse_expression();
        if (!flag.has_value()) {
            return false;
        }
        wait = std::holds_alternative<bool>(*flag)
                   ? std::get<bool>(*flag)
                   : (is_number(*flag) && as_double(*flag) != 0.0);
    }
    if (!execute_) {
        return true;
    }
    const auto* text = std::get_if<std::string>(&*keys);
    if (text == nullptr) {
        set_error("WFC0073", "Type mismatch", offset);
        return false;
    }
    std::vector<FormSupport::Keystroke> strokes;
    if (!FormSupport::parse_sendkeys(*text, strokes)) {
        static_cast<void>(
            raise_runtime(5, "Invalid procedure call or argument", offset));
        return false;
    }
    if (!forms_ && ui_mode_ != "win32") {
        return true;  // no forms: nothing receives the keys
    }
    if (!FormSupport::send_keys(*this, strokes)) {
        return false;
    }
    if (wait) {
        return pump_forms();
    }
    return true;
}

std::optional<long> Interpreter::ui_message_box(const std::string& text,
                                                const std::string& title,
                                                const long style) {
    if (ui_mode_ != "win32" || !ui_script_.empty()) {
        return std::nullopt;
    }
    auto& state = FormSupport::runtime(*this);
    if (state.headless != nullptr) {
        return std::nullopt;
    }
    const auto found = app_properties_.find("title");
    return state.host->message_box(
        dialog_owner(*this, state), text,
        !title.empty() ? title
                       : (found != app_properties_.end() ? found->second
                                                         : std::string{"WFC"}),
        style);
}

std::optional<std::string> Interpreter::ui_input_box(const std::string& prompt,
                                                     const std::string& title,
                                                     const std::string& initial,
                                                     bool& handled) {
    handled = false;
    if (ui_mode_ != "win32" || !ui_script_.empty()) {
        return std::nullopt;
    }
    auto& state = FormSupport::runtime(*this);
    if (state.headless != nullptr) {
        return std::nullopt;
    }
    const auto found = app_properties_.find("title");
    bool supported = false;
    auto answer = state.host->input_box(
        dialog_owner(*this, state), prompt,
        !title.empty() ? title
                       : (found != app_properties_.end() ? found->second
                                                         : std::string{"WFC"}),
        initial, supported);
    handled = supported;
    return answer;
}

std::optional<std::string> Interpreter::form_dialog_answer(
    const std::string& kind) {
    if (!forms_ || forms_->answers.empty() ||
        forms_->answers.front().first != kind) {
        return std::nullopt;
    }
    std::string text = std::move(forms_->answers.front().second);
    forms_->answers.pop_front();
    return text;
}

InstanceData* Interpreter::implicit_form_surface() {
    auto* const instance = current_instance();
    if (instance == nullptr || instance->native == nullptr ||
        instance->class_name == "wfcnative") {
        return nullptr;
    }
    const ControlObject* object = control_of(*instance);
    return object != nullptr && object->is_form ? instance : nullptr;
}

bool Interpreter::form_print(InstanceData& surface, const std::string& text,
                             const bool newline) {
    ControlObject* object = control_of(surface);
    if (object == nullptr || !FormSupport::is_surface(*object)) {
        static_cast<void>(raise_runtime(
            438, "Object doesn't support this property or method", offset_));
        return false;
    }
    return FormSupport::surface_print(*this, *object, text, newline);
}

bool Interpreter::parse_graphics_point(double& x, double& y) {
    skip_horizontal_whitespace();
    if (!consume('(')) {
        set_error("WFC0005", "expected '('", offset_);
        return false;
    }
    const auto read = [&](double& out) {
        skip_horizontal_whitespace();
        auto value = parse_expression();
        if (!value.has_value()) {
            return false;
        }
        if (!resolve_default_value(*value, offset_)) {
            return false;
        }
        if (!execute_) {
            out = 0.0;
            return true;
        }
        if (!is_number(*value)) {
            set_error("WFC0073", "Type mismatch", offset_);
            return false;
        }
        out = as_double(*value);
        return true;
    };
    if (!read(x)) {
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume(',')) {
        set_error("WFC0005", "expected ','", offset_);
        return false;
    }
    if (!read(y)) {
        return false;
    }
    skip_horizontal_whitespace();
    if (!consume(')')) {
        set_error("WFC0005", "expected closing parenthesis", offset_);
        return false;
    }
    return true;
}

bool Interpreter::parse_surface_statement(InstanceData& surface_instance,
                                          const std::string& keyword,
                                          const std::size_t offset) {
    ControlObject* surface = control_of(surface_instance);
    if (surface == nullptr || !FormSupport::is_surface(*surface)) {
        static_cast<void>(raise_runtime(
            438, "Object doesn't support this property or method", offset));
        return false;
    }
    const auto keep = surface_instance.shared_from_this();
    skip_horizontal_whitespace();
    const auto numeric = [&](double& out, const bool required) {
        skip_horizontal_whitespace();
        if (!required && (at_statement_end() || current() == ',')) {
            return true;  // omitted
        }
        auto value = parse_expression();
        if (!value.has_value() || !resolve_default_value(*value, offset_)) {
            return false;
        }
        if (!execute_) {
            return true;
        }
        if (!is_number(*value)) {
            set_error("WFC0073", "Type mismatch", offset_);
            return false;
        }
        out = as_double(*value);
        return true;
    };
    double current_x = FormSupport::number_of(*surface, "currentx");
    double current_y = FormSupport::number_of(*surface, "currenty");
    if (keyword == "cls") {
        if (!execute_) {
            return true;
        }
        std::vector<CallArgument> none;
        Value ignored;
        NativeFailure failure;
        return FormSupport::surface_method(*this, *surface, "cls", none,
                                           ignored, failure);
    }
    if (keyword == "scale") {
        if (at_statement_end()) {
            if (execute_) {
                surface->values["scalemode"] = Value{Int16{1}};
                surface->values["scaleleft"] = Value{0.0F};
                surface->values["scaletop"] = Value{0.0F};
            }
            return true;
        }
        double x1 = 0.0;
        double y1 = 0.0;
        double x2 = 0.0;
        double y2 = 0.0;
        if (!parse_graphics_point(x1, y1)) {
            return false;
        }
        skip_horizontal_whitespace();
        if (!consume('-')) {
            set_error("WFC0005", "expected '-'", offset_);
            return false;
        }
        if (!parse_graphics_point(x2, y2)) {
            return false;
        }
        if (execute_) {
            if (x2 == x1 || y2 == y1) {
                static_cast<void>(
                    raise_runtime(380, "Invalid property value", offset));
                return false;
            }
            surface->values["scalemode"] = Value{Int16{0}};
            surface->values["scaleleft"] = Value{static_cast<float>(x1)};
            surface->values["scaletop"] = Value{static_cast<float>(y1)};
            surface->values["scalewidth"] = Value{static_cast<float>(x2 - x1)};
            surface->values["scaleheight"] = Value{static_cast<float>(y2 - y1)};
        }
        return true;
    }
    if (keyword == "pset") {
        const bool step = consume_keyword("step");
        double x = 0.0;
        double y = 0.0;
        if (!parse_graphics_point(x, y)) {
            return false;
        }
        double color = FormSupport::number_of(*surface, "forecolor");
        skip_horizontal_whitespace();
        if (consume(',') && !numeric(color, false)) {
            return false;
        }
        if (!execute_) {
            return true;
        }
        if (step) {
            x += current_x;
            y += current_y;
        }
        ui::DrawOp operation = FormSupport::base_op(*surface, "pset");
        operation.x1 = FormSupport::to_twips_x(*surface, x);
        operation.y1 = FormSupport::to_twips_y(*surface, y);
        operation.color = FormSupport::resolve_color(static_cast<long>(color));
        FormSupport::draw_op(*this, *surface, operation);
        surface->values["currentx"] = Value{static_cast<float>(x)};
        surface->values["currenty"] = Value{static_cast<float>(y)};
        return true;
    }
    if (keyword == "line") {
        bool step_start = consume_keyword("step");
        double x1 = current_x;
        double y1 = current_y;
        skip_horizontal_whitespace();
        if (!at_end() && current() == '(') {
            if (!parse_graphics_point(x1, y1)) {
                return false;
            }
            if (step_start) {
                x1 += current_x;
                y1 += current_y;
            }
        }
        skip_horizontal_whitespace();
        if (!consume('-')) {
            set_error("WFC0005", "expected '-'", offset_);
            return false;
        }
        skip_horizontal_whitespace();
        const bool step_end = consume_keyword("step");
        double x2 = 0.0;
        double y2 = 0.0;
        if (!parse_graphics_point(x2, y2)) {
            return false;
        }
        if (step_end) {
            x2 += x1;
            y2 += y1;
        }
        double color = FormSupport::number_of(*surface, "forecolor");
        bool box = false;
        bool filled = false;
        skip_horizontal_whitespace();
        if (consume(',')) {
            if (!numeric(color, false)) {
                return false;
            }
            skip_horizontal_whitespace();
            if (consume(',')) {
                skip_horizontal_whitespace();
                const auto style = parse_identifier(nullptr);
                if (!style.has_value() || (*style != "b" && *style != "bf")) {
                    set_error("WFC0011", "expected B or BF", offset_);
                    return false;
                }
                box = true;
                filled = *style == "bf";
            }
        }
        if (!execute_) {
            return true;
        }
        ui::DrawOp operation = FormSupport::base_op(*surface, "line");
        operation.x1 = FormSupport::to_twips_x(*surface, x1);
        operation.y1 = FormSupport::to_twips_y(*surface, y1);
        operation.x2 = FormSupport::to_twips_x(*surface, x2);
        operation.y2 = FormSupport::to_twips_y(*surface, y2);
        operation.color = FormSupport::resolve_color(static_cast<long>(color));
        operation.box = box;
        operation.filled = filled;
        FormSupport::draw_op(*this, *surface, operation);
        surface->values["currentx"] = Value{static_cast<float>(x2)};
        surface->values["currenty"] = Value{static_cast<float>(y2)};
        return true;
    }
    if (keyword == "circle") {
        const bool step = consume_keyword("step");
        double x = 0.0;
        double y = 0.0;
        if (!parse_graphics_point(x, y)) {
            return false;
        }
        if (step) {
            x += current_x;
            y += current_y;
        }
        skip_horizontal_whitespace();
        if (!consume(',')) {
            set_error("WFC0005", "expected ','", offset_);
            return false;
        }
        double radius = 0.0;
        if (!numeric(radius, true)) {
            return false;
        }
        double color = FormSupport::number_of(*surface, "forecolor");
        double start = 0.0;
        double end = 0.0;
        double aspect = 1.0;
        bool has_start = false;
        bool has_end = false;
        skip_horizontal_whitespace();
        if (consume(',')) {
            if (!numeric(color, false)) {
                return false;
            }
            skip_horizontal_whitespace();
            if (consume(',')) {
                skip_horizontal_whitespace();
                has_start = !(at_statement_end() || current() == ',');
                if (!numeric(start, false)) {
                    return false;
                }
                skip_horizontal_whitespace();
                if (consume(',')) {
                    skip_horizontal_whitespace();
                    has_end = !(at_statement_end() || current() == ',');
                    if (!numeric(end, false)) {
                        return false;
                    }
                    skip_horizontal_whitespace();
                    if (consume(',') && !numeric(aspect, false)) {
                        return false;
                    }
                }
            }
        }
        if (!execute_) {
            return true;
        }
        if (radius < 0.0) {
            static_cast<void>(
                raise_runtime(5, "Invalid procedure call or argument", offset));
            return false;
        }
        ui::DrawOp operation = FormSupport::base_op(*surface, "circle");
        operation.x1 = FormSupport::to_twips_x(*surface, x);
        operation.y1 = FormSupport::to_twips_y(*surface, y);
        const auto scale = FormSupport::scale_of(*surface);
        operation.radius =
            radius * FormSupport::client_width(*surface) / scale.width;
        operation.color = FormSupport::resolve_color(static_cast<long>(color));
        operation.arc = has_start || has_end;
        operation.start = start;
        operation.end = end;
        operation.aspect = aspect;
        FormSupport::draw_op(*this, *surface, operation);
        surface->values["currentx"] = Value{static_cast<float>(x)};
        surface->values["currenty"] = Value{static_cast<float>(y)};
        return true;
    }
    static_cast<void>(keep);
    set_error("WFC0010", "expected statement", offset);
    return false;
}

bool Interpreter::pump_forms() {
    if (!forms_ || !execute_) {
        return true;
    }
    if (!ocx_poll_sockets(*this)) {
        return false;
    }
    for (;;) {
        if (const auto event = forms_->host->next_event(false)) {
            if (!FormSupport::dispatch(*this, *event)) {
                return false;
            }
        } else if (!FormSupport::deliver_pending_key(*this)) {
            break;
        }
    }
    return true;
}

bool Interpreter::run_forms_at_exit() {
    if (!startup_form_.empty() && !forms_) {
        const auto variable = find_variable(lower_text(startup_form_));
        if (variable.value != nullptr) {
            if (const auto* holder =
                    std::get_if<ObjectInstance>(variable.value)) {
                if (!FormSupport::show(*this, *holder->data, offset_)) {
                    return false;
                }
            }
        }
    }
    if (!forms_) {
        return true;
    }
    if (end_requested_ && !forms_->ended) {
        return true;  // End in the main program: nothing is unloaded
    }
    return FormSupport::run_loop(*this);
}

#include "comctl.inc"
#include "ocx.inc"

}  // namespace wfc::detail
