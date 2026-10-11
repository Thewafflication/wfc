// Interpreter: forms and intrinsic controls (REQ-0289). A form is a class
// module with a predeclared instance whose controls are fields; controls are
// native objects whose properties come from the generated type-library
// tables. Windows live in a UiHost (headless by default). Internal to the WFC
// evaluator; not part of the public API.

#include "forms.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "../ui/control_tables.hpp"

namespace wfc::detail {
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
[[nodiscard]] std::optional<Value> property_default(
    const std::string& class_name, const std::string& property) {
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
        class_name == "vscrollbar") {
        return "value";
    }
    return {};
}

constexpr Integer kErrorAlreadyReported = -1;

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
    std::vector<long> item_data;
    std::vector<bool> chosen;  // MultiSelect selection flags
    long list_index{-1};
    long new_index{-1};
    [[nodiscard]] bool is_list() const {
        return class_key == "listbox" || class_key == "combobox";
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
    [[nodiscard]] bool put(Interpreter&, const std::string&,
                           std::vector<CallArgument>&, const Value&,
                           NativeFailure&) override {
        return false;
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

// Everything that needs Interpreter internals; a friend of Interpreter.
struct FormSupport {
    [[nodiscard]] static std::string text_of(const Value& value) {
        return Interpreter::render(value);
    }

    static FormRuntime& runtime(Interpreter& in) {
        if (!in.forms_) {
            in.forms_ = std::make_shared<FormRuntime>();
            auto host = std::make_unique<ui::HeadlessHost>();
            in.forms_->headless = host.get();
            in.forms_->host = std::move(host);
        }
        return *in.forms_;
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
        host.set_items(list.window, list.items);
        host.set_selection(list.window, list.list_index);
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
                       : "form")
                : control;
        const std::string handler = prefix + "_" + event;
        const auto method = class_iterator->second.methods.find(handler);
        if (method == class_iterator->second.methods.end()) {
            return true;
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
        return in
            .invoke_definition(method->second, handler, std::move(call), offset,
                               class_iterator->second.source, keep.get())
            .has_value();
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

    // Creates the host windows of a form and its controls and applies their
    // property values.
    static void create_windows(Interpreter& in, InstanceData& form,
                               ControlObject& form_object) {
        auto& host = *runtime(in).host;
        form_object.window = host.create_window("Form", ui::kNoWindow);
        bind(in, form_object);
        const auto create_one = [&](ControlObject& object) {
            const ui::WindowId parent = form_object.window;
            object.window = host.create_window(object.cls->name, parent);
            bind(in, object);
            mirror_all(in, object);
        };
        for (auto& [name, value] : form.fields.variables) {
            if (auto* object = control_of(value)) {
                if (!object->is_form) {
                    create_one(*object);
                }
            } else if (const auto* holder = std::get_if<ObjectInstance>(&value);
                       holder != nullptr && holder->data != nullptr) {
                if (auto* array = dynamic_cast<ControlArray*>(
                        holder->data->native.get())) {
                    for (auto& [position, element] : array->elements) {
                        if (auto* member = control_of(element)) {
                            create_one(*member);
                        }
                    }
                }
            }
        }
        mirror_all(in, form_object);
    }

    // Pushes every stored property to the host.
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
        host.set_bounds(object.window,
                        ui::Rect{number_of("left"), number_of("top"),
                                 number_of("width"), number_of("height")});
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
        create_windows(in, form, *object);
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

    // Sets keyboard focus, raising LostFocus and GotFocus.
    [[nodiscard]] static bool focus_control(Interpreter& in, InstanceData& form,
                                            const std::string& control,
                                            const long index,
                                            const std::size_t offset) {
        auto& host = *runtime(in).host;
        ControlObject* target = control_named(form, control, index);
        if (target == nullptr || target->window == ui::kNoWindow ||
            host.focus() == target->window) {
            return true;
        }
        const auto previous = runtime(in).bindings.find(host.focus());
        host.set_focus(target->window);
        if (previous != runtime(in).bindings.end()) {
            if (const auto old_form = previous->second.form.lock()) {
                ControlObject* old =
                    control_named(*old_form, previous->second.control,
                                  previous->second.index);
                if (old != nullptr && !old->is_form &&
                    !raise_table_event(in, *old, "LostFocus", {}, offset)) {
                    return false;
                }
            }
        }
        if (!target->is_form) {
            return raise_table_event(in, *target, "GotFocus", {}, offset);
        }
        return true;
    }

    static void focus_first(Interpreter& in, InstanceData& form,
                            const std::size_t offset) {
        ControlObject* best = nullptr;
        long best_order = 0;
        long position = 0;
        const auto consider = [&](ControlObject* object) {
            ++position;
            if (object == nullptr || object->is_form) {
                return;
            }
            const auto stop = object->values.find("tabstop");
            const auto enabled = object->values.find("enabled");
            const auto visible = object->values.find("visible");
            const auto flag = [](const auto& it, const auto& end) {
                return it == end || !std::holds_alternative<bool>(it->second) ||
                       std::get<bool>(it->second);
            };
            if (object->class_key == "label" || object->class_key == "timer" ||
                object->class_key == "frame" || object->class_key == "shape" ||
                object->class_key == "line" || object->class_key == "image" ||
                !flag(stop, object->values.end()) ||
                !flag(enabled, object->values.end()) ||
                !flag(visible, object->values.end())) {
                return;
            }
            const auto order = object->values.find("tabindex");
            const long key = order != object->values.end()
                                 ? static_cast<long>(as_double(order->second))
                                 : position;
            if (best == nullptr || key < best_order) {
                best = object;
                best_order = key;
            }
        };
        for (auto& [name, value] : form.fields.variables) {
            if (auto* object = control_of(value)) {
                consider(object);
            } else if (const auto* holder = std::get_if<ObjectInstance>(&value);
                       holder != nullptr && holder->data != nullptr) {
                if (auto* array = dynamic_cast<ControlArray*>(
                        holder->data->native.get())) {
                    for (auto& [at, element] : array->elements) {
                        consider(control_of(element));
                    }
                }
            }
        }
        if (best != nullptr) {
            static_cast<void>(
                focus_control(in, form, best->name, best->index, offset));
        }
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
        object->shown = true;
        object->values["visible"] = Value{true};
        runtime(in).host->set_visible(object->window, true);
        std::vector<Value> none;
        if (!object->activated) {
            object->activated = true;
            if (!raise(in, form, "", -1, "activate", none, offset)) {
                return false;
            }
            focus_first(in, form, offset);
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
        std::vector<Value> query{Value{Int16{0}},
                                 Value{static_cast<Int16>(mode)}};
        if (!raise(in, form, "", -1, "queryunload", query, offset)) {
            object->unloading = false;
            return false;
        }
        bool cancelled = whole_value(query[0]).value_or(0) != 0;
        if (!cancelled) {
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
        if (self.is_list() && (member == "additem" || member == "removeitem" ||
                               member == "clear")) {
            return list_method(in, self, member, arguments, failure);
        }
        if (member == "refresh" || member == "zorder" || member == "cls" ||
            member == "drag" || member == "setfocus") {
            return true;
        }
        if (self.is_form && member == "show") {
            if (form == nullptr || !show(in, *form, in.offset_)) {
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

    // Mirrors a changed property to the host and raises its events.
    [[nodiscard]] static bool property_changed(Interpreter& in,
                                               ControlObject& self,
                                               const std::string& member,
                                               const Value& old_value,
                                               NativeFailure& failure) {
        auto& host = *runtime(in).host;
        const Value& now = self.values[member];
        const bool live = self.window != ui::kNoWindow;
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
                    }
                });
            }
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
        return raise_table_event(in, *source, event.name, event.arguments,
                                 in.offset_);
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
        if (command == "wait") {
            if (state.headless != nullptr) {
                state.headless->advance_time(std::atol(target_text.c_str()));
            }
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
        } else if (command == "focus") {
            return focus_control(in, *target.form, target.control, target.index,
                                 in.offset_);
        } else if (command == "set" || command == "type") {
            if (command == "set") {
                const Value text{rest};
                object->values["text"] = text;
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
                    const auto previous = object->values.find("text");
                    std::string text =
                        previous != object->values.end() &&
                                std::holds_alternative<std::string>(
                                    previous->second)
                            ? std::get<std::string>(previous->second)
                            : std::string{};
                    text.push_back(c);
                    object->values["text"] = Value{text};
                    state.host->set_text(object->window, text);
                    state.host->post_event(
                        ui::UiEvent{object->window, "Change", {}});
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
    [[nodiscard]] static bool run_loop(Interpreter& in) {
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
        while (true) {
            if (const auto event = state.host->next_event(false)) {
                if (!dispatch(in, *event)) {
                    return false;
                }
                continue;
            }
            if (state.script_next < state.script.size()) {
                if (!run_script_line(in, state.script[state.script_next++])) {
                    return false;
                }
                continue;
            }
            break;
        }
        // The user "closes" what is left.
        auto remaining = state.loaded;
        for (auto it = remaining.rbegin(); it != remaining.rend(); ++it) {
            if (const auto form = it->lock()) {
                if (!unload(in, *form, 0, in.offset_)) {
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

bool ControlObject::get(Interpreter& in, const std::string& member,
                        std::vector<CallArgument>& arguments, Value& result,
                        NativeFailure& failure) {
    if (!FormSupport::touch(in, *this, failure)) {
        return true;
    }
    if (member == "name") {
        result = Value{display_name};
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
        } else {
            result = default_for_type(getter->type);
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
    static_cast<void>(arguments);
    if (!FormSupport::touch(in, *this, failure)) {
        return true;
    }
    if (member == "name" || member == "index") {
        failure = NativeFailure{383, "Property is read-only"};
        return true;
    }
    std::string wanted = member;
    if (member == "_default") {
        wanted = default_property_of(class_key);
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
    if (!coerce_property(in, converted, setter->type, failure)) {
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
        if (!resolve_default_value(*value, member_offset)) {
            return false;
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
                    if (auto value = convert(part, *object.cls, target)) {
                        if (part_key == "weight") {
                            object.values[target] =
                                Value{as_double(*value) >= 700.0};
                        } else {
                            object.values[target] = std::move(*value);
                        }
                    }
                }
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
        object.design = object.values;
    };

    auto form_object =
        std::make_shared<ControlObject>(form_class, root.name, -1L, true);
    form_object->form = instance;
    apply(*form_object, root);
    instance->native = form_object;

    // Controls, in file order; containers contribute their children.
    const auto native_class =
        [&](const std::string& type) -> const ui::ControlClass* {
        std::string leaf = type;
        if (leaf.rfind("VB.", 0) == 0) {
            leaf = leaf.substr(3);
        }
        return ui::find_control_class(leaf);
    };
    const auto make_control =
        [&](const ui::FrmObject& source, const ui::ControlClass& control_class,
            const long array_index, const ui::FrmObject& parent) {
            auto object = std::make_shared<ControlObject>(
                &control_class, source.name, array_index, false);
            object->form = instance;
            if (&parent != &root) {
                object->container = lower_text(parent.name);
            }
            apply(*object, source);
            auto data = std::make_shared<InstanceData>();
            data->class_name = "wfcnative";
            data->native = object;
            return Value{ObjectInstance{std::move(data)}};
        };
    std::map<std::string, std::shared_ptr<InstanceData>> arrays;
    const auto add_controls = [&](const auto& self,
                                  const ui::FrmObject& parent) -> void {
        for (const auto& child : parent.children) {
            const auto* control_class = native_class(child.type);
            if (control_class == nullptr) {
                continue;  // not an intrinsic control (ActiveX: MP-0005/6)
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
            } else {
                instance->fields.variables[key] =
                    make_control(child, *control_class, -1L, parent);
                instance->fields.object_variables.insert(key);
            }
            self(self, child);
        }
    };
    add_controls(add_controls, root);
    static_cast<void>(offset);
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

std::shared_ptr<NativeObject> make_forms_collection() {
    return std::make_shared<FormsCollection>();
}

bool Interpreter::pump_forms() {
    if (!forms_ || !execute_) {
        return true;
    }
    while (const auto event = forms_->host->next_event(false)) {
        if (!FormSupport::dispatch(*this, *event)) {
            return false;
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
    return FormSupport::run_loop(*this);
}

}  // namespace wfc::detail
