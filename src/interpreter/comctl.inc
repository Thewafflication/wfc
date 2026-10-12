// The MSComctlLib controls (MP-0006, ADR-0006): TabStrip, Toolbar, StatusBar,
// ProgressBar, TreeView, ListView, ImageList, Slider and ImageCombo, with their
// collections and value objects. Included once by forms.cpp, inside
// namespace wfc::detail, after FormSupport and ControlObject are defined.
// Internal to the WFC evaluator; not part of the public API.

// ----- Item specifications ---------------------------------------------------

namespace {

struct PropSpec {
    const char* name;
    const char* type;  // String, Long, Integer, Boolean, Single, Variant
    const char* initial;  // "" (empty / zero / False), "true", or a number
};

struct KindSpec {
    const char* kind;        // "tab"
    const char* item_name;   // "Tab" (TypeName)
    const char* collection;  // "Tabs"
    std::vector<const char*> add_parameters;  // positional, lower case
    std::vector<PropSpec> props;
    const char* default_property;
};

const std::map<std::string, KindSpec>& kind_specs() {
    static const std::map<std::string, KindSpec> specs = {
        {"tab",
         {"tab",
          "Tab",
          "Tabs",
          {"index", "key", "caption", "image"},
          {{"caption", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"image", "Variant", ""},
           {"tooltiptext", "String", ""},
           {"visible", "Boolean", "true"},
           {"highlighted", "Boolean", ""}},
          "caption"}},
        {"panel",
         {"panel",
          "Panel",
          "Panels",
          {"index", "key", "text", "style", "picture"},
          {{"text", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"style", "Integer", ""},
           {"alignment", "Integer", ""},
           {"bevel", "Integer", "1"},
           {"autosize", "Integer", ""},
           {"width", "Single", ""},
           {"minwidth", "Single", "960"},
           {"picture", "Variant", ""},
           {"tooltiptext", "String", ""},
           {"visible", "Boolean", "true"},
           {"enabled", "Boolean", "true"}},
          "text"}},
        {"button",
         {"button",
          "Button",
          "Buttons",
          {"index", "key", "caption", "style", "image"},
          {{"caption", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"style", "Integer", ""},
           {"value", "Integer", ""},
           {"image", "Variant", ""},
           {"tooltiptext", "String", ""},
           {"description", "String", ""},
           {"visible", "Boolean", "true"},
           {"enabled", "Boolean", "true"},
           {"mixedstate", "Boolean", ""},
           {"width", "Single", ""}},
          "caption"}},
        {"buttonmenu",
         {"buttonmenu",
          "ButtonMenu",
          "ButtonMenus",
          {"index", "key", "text"},
          {{"text", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"enabled", "Boolean", "true"},
           {"visible", "Boolean", "true"}},
          "text"}},
        {"node",
         {"node",
          "Node",
          "Nodes",
          {"relative", "relationship", "key", "text", "image",
           "selectedimage"},
          {{"text", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"image", "Variant", ""},
           {"selectedimage", "Variant", ""},
           {"expandedimage", "Variant", ""},
           {"expanded", "Boolean", ""},
           {"selected", "Boolean", ""},
           {"checked", "Boolean", ""},
           {"bold", "Boolean", ""},
           {"forecolor", "Long", ""},
           {"backcolor", "Long", ""},
           {"sorted", "Boolean", ""},
           {"visible", "Boolean", "true"}},
          "text"}},
        {"listitem",
         {"listitem",
          "ListItem",
          "ListItems",
          {"index", "key", "text", "icon", "smallicon"},
          {{"text", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"icon", "Variant", ""},
           {"smallicon", "Variant", ""},
           {"selected", "Boolean", ""},
           {"checked", "Boolean", ""},
           {"ghosted", "Boolean", ""},
           {"bold", "Boolean", ""},
           {"forecolor", "Long", ""},
           {"tooltiptext", "String", ""}},
          "text"}},
        {"listsubitem",
         {"listsubitem",
          "ListSubItem",
          "ListSubItems",
          {"index", "key", "text", "reporticon", "tooltiptext"},
          {{"text", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"reporticon", "Variant", ""},
           {"forecolor", "Long", ""},
           {"bold", "Boolean", ""},
           {"tooltiptext", "String", ""}},
          "text"}},
        {"columnheader",
         {"columnheader",
          "ColumnHeader",
          "ColumnHeaders",
          {"index", "key", "text", "width", "alignment", "icon"},
          {{"text", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"width", "Single", "1440"},
           {"alignment", "Integer", ""},
           {"icon", "Variant", ""},
           {"subitemindex", "Integer", ""}},
          "text"}},
        {"listimage",
         {"listimage",
          "ListImage",
          "ListImages",
          {"index", "key", "picture"},
          {{"key", "String", ""},
           {"tag", "Variant", ""},
           {"picture", "Variant", ""}},
          "picture"}},
        {"comboitem",
         {"comboitem",
          "ComboItem",
          "ComboItems",
          {"index", "key", "text", "image", "selimage", "indentation"},
          {{"text", "String", ""},
           {"key", "String", ""},
           {"tag", "Variant", ""},
           {"image", "Variant", ""},
           {"selimage", "Variant", ""},
           {"indentation", "Integer", ""},
           {"selected", "Boolean", ""}},
          "text"}},
    };
    return specs;
}

// Which collection property of a control holds which kind of item.
struct CollectionBinding {
    const char* control;     // class key
    const char* property;    // `Tabs`
    const char* kind;        // `tab`
};

const std::vector<CollectionBinding>& collection_bindings() {
    static const std::vector<CollectionBinding> bindings = {
        {"tabstrip", "tabs", "tab"},
        {"statusbar", "panels", "panel"},
        {"toolbar", "buttons", "button"},
        {"treeview", "nodes", "node"},
        {"listview", "listitems", "listitem"},
        {"listview", "columnheaders", "columnheader"},
        {"imagelist", "listimages", "listimage"},
        {"imagecombo", "comboitems", "comboitem"},
    };
    return bindings;
}

}  // namespace

// One element of a collection: a bag of properties with shared identity.
struct CItem {
    long id{0};
    std::map<std::string, Value> p;
    // Tree nodes: the ids of the children in order, and the parent id (0 =
    // top level).
    std::vector<long> kids;
    long parent{0};
    // Buttons: the drop-down menu items; list items: the sub items.
    std::vector<std::shared_ptr<CItem>> sub;
    // The object a program holds for this item (kept weakly).
    std::weak_ptr<InstanceData> wrapper;
};

struct ComctlState {
    long next_id{1};
    std::map<std::string, std::vector<std::shared_ptr<CItem>>> lists;
    std::vector<long> root_kids;  // tree: top-level node ids in order
};


std::vector<std::string> comctl_type_names() {
    std::vector<std::string> names;
    for (const auto& control : ui::comctl_classes()) {
        names.push_back(lower_text(std::string(control.name)));
    }
    for (const auto& [kind, spec] : kind_specs()) {
        names.push_back(lower_text(std::string(spec.item_name)));
        names.push_back(lower_text(std::string(spec.collection)));
    }
    return names;
}

// ----- Merged class tables -----------------------------------------------------

namespace {

struct MergedClass {
    std::vector<ui::MemberRow> members;
    std::vector<ui::MemberRow> events;
    ui::ControlClass cls{};
};

}  // namespace

const ui::ControlClass* comctl_class(const std::string_view leaf) {
    static std::map<std::string, std::unique_ptr<MergedClass>> merged;
    const ui::ControlClass* base = ui::find_comctl_class(leaf);
    if (base == nullptr) {
        return nullptr;
    }
    const std::string key = lower_text(std::string(base->name));
    const auto found = merged.find(key);
    if (found != merged.end()) {
        return &found->second->cls;
    }
    auto entry = std::make_unique<MergedClass>();
    const auto has = [](const std::vector<ui::MemberRow>& rows,
                        const ui::MemberRow& wanted) {
        for (const auto& row : rows) {
            if (lower_text(row.name) == lower_text(wanted.name) &&
                row.kind == wanted.kind) {
                return true;
            }
        }
        return false;
    };
    for (std::size_t i = 0; i < base->member_count; ++i) {
        entry->members.push_back(base->members[i]);
    }
    for (const auto& row : kExtenderMembers) {
        if (!has(entry->members, row)) {
            entry->members.push_back(row);
        }
    }
    for (std::size_t i = 0; i < base->event_count; ++i) {
        entry->events.push_back(base->events[i]);
    }
    for (const auto& row : kExtenderEvents) {
        if (!has(entry->events, row)) {
            entry->events.push_back(row);
        }
    }
    entry->cls = ui::ControlClass{base->name, entry->members.data(),
                                  entry->members.size(), entry->events.data(),
                                  entry->events.size()};
    const ui::ControlClass* result = &entry->cls;
    merged.emplace(key, std::move(entry));
    return result;
}

// ----- Defaults of the control properties -------------------------------------

namespace {

[[nodiscard]] Value small(const int number) {
    return Value{static_cast<Int16>(number)};
}

}  // namespace

std::optional<Value> comctl_default(const std::string& class_name,
                                    const std::string& property) {
    static const std::map<std::string, std::map<std::string, Value>> table = {
        {"progressbar",
         {{"min", Value{0.0F}},
          {"max", Value{100.0F}},
          {"value", Value{0.0F}},
          {"controldefault", Value{0.0F}},
          {"orientation", small(0)},
          {"scrolling", small(0)},
          {"appearance", small(1)},
          {"borderstyle", small(0)}}},
        {"slider",
         {{"min", Value{Integer{0}}},
          {"max", Value{Integer{10}}},
          {"value", Value{Integer{0}}},
          {"largechange", Value{Integer{5}}},
          {"smallchange", Value{Integer{1}}},
          {"tickfrequency", Value{Integer{1}}},
          {"tickstyle", small(0)},
          {"orientation", small(0)},
          {"selectrange", Value{false}},
          {"selstart", Value{Integer{0}}},
          {"sellength", Value{Integer{0}}},
          {"textposition", small(0)},
          {"text", Value{std::string{}}},
          {"borderstyle", small(0)}}},
        {"tabstrip",
         {{"style", small(0)},
          {"multirow", Value{false}},
          {"tabfixedwidth", small(0)},
          {"tabfixedheight", small(0)},
          {"tabminwidth", Value{Integer{0}}},
          {"tabwidthstyle", small(0)},
          {"showtips", Value{true}},
          {"hottracking", Value{false}},
          {"multiselect", Value{false}},
          {"placement", small(0)},
          {"separators", Value{false}},
          {"tabstyle", small(0)}}},
        {"imagelist",
         {{"imagewidth", small(16)},
          {"imageheight", small(16)},
          {"maskcolor", Value{Integer{12632256}}},
          {"usemaskcolor", Value{true}},
          {"backcolor", Value{Integer{-2147483633}}}}},
        {"toolbar",
         {{"style", small(0)},
          {"wrappable", Value{true}},
          {"showtips", Value{true}},
          {"allowcustomize", Value{true}},
          {"appearance", small(1)},
          {"borderstyle", small(0)},
          {"textalignment", small(0)},
          {"buttonheight", Value{0.0F}},
          {"buttonwidth", Value{0.0F}}}},
        {"statusbar",
         {{"style", small(0)},
          {"simpletext", Value{std::string{}}},
          {"showtips", Value{true}}}},
        {"treeview",
         {{"indentation", Value{0.0F}},
          {"labeledit", small(0)},
          {"linestyle", small(0)},
          {"pathseparator", Value{std::string{"\\"}}},
          {"sorted", Value{false}},
          {"style", small(7)},
          {"checkboxes", Value{false}},
          {"fullrowselect", Value{false}},
          {"hottracking", Value{false}},
          {"singlesel", Value{false}},
          {"hideselection", Value{true}},
          {"appearance", small(1)},
          {"borderstyle", small(0)}}},
        {"listview",
         {{"view", small(0)},
          {"labeledit", small(0)},
          {"sorted", Value{false}},
          {"sortkey", small(0)},
          {"sortorder", small(0)},
          {"multiselect", Value{false}},
          {"checkboxes", Value{false}},
          {"gridlines", Value{false}},
          {"fullrowselect", Value{false}},
          {"hideselection", Value{true}},
          {"hidecolumnheaders", Value{false}},
          {"labelwrap", Value{true}},
          {"arrange", small(0)},
          {"appearance", small(1)},
          {"borderstyle", small(0)}}},
        {"imagecombo",
         {{"text", Value{std::string{}}},
          {"locked", Value{false}},
          {"indentation", small(0)},
          {"sellength", Value{Integer{0}}},
          {"selstart", Value{Integer{0}}},
          {"seltext", Value{std::string{}}}}},
    };
    const auto found = table.find(class_name);
    if (found == table.end()) {
        return std::nullopt;
    }
    const auto value = found->second.find(property);
    if (value == found->second.end()) {
        return std::nullopt;
    }
    return value->second;
}

// ----- Collections and items --------------------------------------------------

namespace {

constexpr Integer kIndexOutOfBounds = 35600;
constexpr Integer kElementNotFound = 35601;
constexpr Integer kKeyNotUnique = 35602;
constexpr Integer kReadOnlyAfterUse = 35614;

using ItemPtr = std::shared_ptr<CItem>;

[[nodiscard]] const KindSpec& spec_of(const std::string& kind) {
    return kind_specs().at(kind);
}

// The lower-case name of the list that holds items of `kind` (top-level).
[[nodiscard]] std::string list_name_of(const std::string& kind) {
    return lower_text(std::string(spec_of(kind).collection));
}

[[nodiscard]] Value initial_value(const PropSpec& property) {
    const std::string type = property.type;
    const std::string initial = property.initial;
    if (type == "String") {
        return Value{initial};
    }
    if (type == "Boolean") {
        return Value{initial == "true"};
    }
    const double number = initial.empty() ? 0.0 : std::atof(initial.c_str());
    if (type == "Integer") {
        return Value{static_cast<Int16>(number)};
    }
    if (type == "Long") {
        return Value{static_cast<Integer>(number)};
    }
    if (type == "Single") {
        return Value{static_cast<float>(number)};
    }
    return Value{Empty{}};  // Variant
}

[[nodiscard]] const PropSpec* find_property(const KindSpec& spec,
                                            const std::string& name) {
    for (const auto& property : spec.props) {
        if (name == property.name) {
            return &property;
        }
    }
    return nullptr;
}

[[nodiscard]] bool same_key(const Value& left, const std::string& right) {
    const auto* text = std::get_if<std::string>(&left);
    return text != nullptr && !text->empty() &&
           lower_text(*text) == lower_text(right);
}

// A reference to one list of items: a top-level collection of a control, or
// the sub items of a button or list item.
struct ListRef {
    std::weak_ptr<InstanceData> owner;
    std::string kind;
    ItemPtr parent;  // set for button menus and list sub items
};

[[nodiscard]] ComctlState& state_of(ControlObject& control) {
    return *control.comctl;
}

[[nodiscard]] ControlObject* control_for(const std::weak_ptr<InstanceData>& owner) {
    const auto data = owner.lock();
    return data != nullptr ? control_of(*data) : nullptr;
}

[[nodiscard]] std::vector<ItemPtr>& list_of(ControlObject& control,
                                            const ListRef& ref) {
    if (ref.parent != nullptr) {
        return ref.parent->sub;
    }
    return state_of(control).lists[list_name_of(ref.kind)];
}

// The 1-based position of `item` in its list.
[[nodiscard]] long position_of(const std::vector<ItemPtr>& items,
                               const ItemPtr& item) {
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i] == item) {
            return static_cast<long>(i) + 1;
        }
    }
    return 0;
}

// Finds an item by index (number) or key (string).
[[nodiscard]] ItemPtr find_item(const std::vector<ItemPtr>& items,
                                const Value& selector,
                                NativeFailure& failure) {
    if (const auto* text = std::get_if<std::string>(&selector)) {
        for (const auto& item : items) {
            if (same_key(item->p.at("key"), *text)) {
                return item;
            }
        }
        failure = NativeFailure{kElementNotFound, "Element not found"};
        return nullptr;
    }
    if (!is_number(selector)) {
        failure = NativeFailure{13, "Type mismatch"};
        return nullptr;
    }
    const long index = static_cast<long>(std::nearbyint(as_double(selector)));
    if (index < 1 || static_cast<std::size_t>(index) > items.size()) {
        failure = NativeFailure{kIndexOutOfBounds, "Index out of bounds"};
        return nullptr;
    }
    return items[static_cast<std::size_t>(index) - 1U];
}

[[nodiscard]] bool key_in_use(const std::vector<ItemPtr>& items,
                              const std::string& key, const ItemPtr& except) {
    for (const auto& item : items) {
        if (item != except && same_key(item->p.at("key"), key)) {
            return true;
        }
    }
    return false;
}

}  // namespace

// What a mutation of a control's model needs to tell the host and the other
// parts of the control (selection fix-ups, host snapshots).
struct ComctlHooks {
    static void changed(Interpreter& in, ControlObject& control);
    static bool before_remove(Interpreter& in, ControlObject& control,
                              const std::string& kind, const ItemPtr& item);
    static bool after_add(Interpreter& in, ControlObject& control,
                          const std::string& kind, const ItemPtr& item);
    static bool item_get(Interpreter& in, ControlObject& control,
                         const std::string& kind, const ItemPtr& item,
                         const ListRef& ref, const std::string& member,
                         std::vector<CallArgument>& arguments, Value& result,
                         NativeFailure& failure);
    static bool item_put(Interpreter& in, ControlObject& control,
                         const std::string& kind, const ItemPtr& item,
                         const ListRef& ref, const std::string& member,
                         std::vector<CallArgument>& arguments,
                         const Value& value, NativeFailure& failure);
    static bool collection_get(Interpreter& in, ControlObject& control,
                               const ListRef& ref, const std::string& member,
                               std::vector<CallArgument>& arguments,
                               Value& result, NativeFailure& failure);
};

namespace {

[[nodiscard]] Value wrap_item(const std::shared_ptr<InstanceData>& owner,
                              const ListRef& ref, const ItemPtr& item);
[[nodiscard]] Value wrap_collection(const std::shared_ptr<InstanceData>& owner,
                                    const ListRef& ref);

class ComctlItem final : public NativeObject {
public:
    ComctlItem(ListRef where, ItemPtr element)
        : ref(std::move(where)), item(std::move(element)) {}
    [[nodiscard]] std::string type_name() const override {
        return spec_of(ref.kind).item_name;
    }
    [[nodiscard]] bool has_member(const std::string&) const override {
        return true;
    }
    [[nodiscard]] bool get(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) override;
    [[nodiscard]] bool put(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments,
                           const Value& value, NativeFailure& failure) override;
    ListRef ref;
    ItemPtr item;
    std::weak_ptr<InstanceData> self;
};

class ComctlCollection final : public NativeObject {
public:
    explicit ComctlCollection(ListRef where) : ref(std::move(where)) {}
    [[nodiscard]] std::string type_name() const override {
        return spec_of(ref.kind).collection;
    }
    [[nodiscard]] bool has_member(const std::string&) const override {
        return true;
    }
    [[nodiscard]] bool get(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) override;
    [[nodiscard]] bool put(Interpreter&, const std::string&,
                           std::vector<CallArgument>&, const Value&,
                           NativeFailure&) override {
        return false;
    }
    [[nodiscard]] bool enumerate(Interpreter& in,
                                 std::vector<Value>& items) override {
        static_cast<void>(in);
        const auto owner = ref.owner.lock();
        ControlObject* control = control_for(ref.owner);
        if (control == nullptr) {
            return true;
        }
        for (const auto& item : list_of(*control, ref)) {
            items.push_back(wrap_item(owner, ref, item));
        }
        return true;
    }
    ListRef ref;
};

[[nodiscard]] Value wrap_item(const std::shared_ptr<InstanceData>& owner,
                              const ListRef& ref, const ItemPtr& item) {
    // One wrapper per item while it lives, so `Is` works.
    if (auto existing = item->wrapper.lock()) {
        return Value{ObjectInstance{std::move(existing)}};
    }
    auto object = std::make_shared<ComctlItem>(ref, item);
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfcnative";
    data->native = object;
    object->self = data;
    ListRef stored = ref;
    stored.owner = owner;
    object->ref = stored;
    item->wrapper = data;
    return Value{ObjectInstance{std::move(data)}};
}

[[nodiscard]] Value wrap_collection(const std::shared_ptr<InstanceData>& owner,
                                    const ListRef& ref) {
    ListRef stored = ref;
    stored.owner = owner;
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfcnative";
    data->native = std::make_shared<ComctlCollection>(std::move(stored));
    return Value{ObjectInstance{std::move(data)}};
}

}  // namespace


// ----- Generic collection and item behavior ------------------------------------

namespace {

[[nodiscard]] Value normalized(const Value& value, const std::string& type) {
    if (type.size() > 9U && type.rfind("Constants") == type.size() - 9U &&
        is_number(value)) {
        return Value{static_cast<Int16>(std::nearbyint(as_double(value)))};
    }
    return value;
}

// Coerces `value` to the declared type of a table property.
[[nodiscard]] bool coerce_row(Interpreter& in, const ControlObject& control,
                              const std::string& member, Value& value,
                              NativeFailure& failure) {
    const auto* row = ui::find_member(*control.cls, member, 'l');
    if (row == nullptr) {
        row = ui::find_member(*control.cls, member, 's');
    }
    if (row == nullptr) {
        return true;
    }
    if (!coerce_property(in, value, row->type, failure)) {
        return false;
    }
    value = normalized(value, row->type);
    return true;
}

[[nodiscard]] bool coerce_prop(Interpreter& in, const PropSpec& property,
                               Value& value, NativeFailure& failure) {
    return coerce_property(in, value, property.type, failure);
}

// Inserts a new item built from the `Add` arguments of its collection.
[[nodiscard]] bool add_to_collection(Interpreter& in, ControlObject& control,
                                     const ListRef& ref,
                                     std::vector<CallArgument>& arguments,
                                     Value& result, NativeFailure& failure) {
    const KindSpec& spec = spec_of(ref.kind);
    std::map<std::string, Value> given;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i].omitted) {
            continue;
        }
        if (!arguments[i].name.empty()) {
            given[arguments[i].name] = arguments[i].value;
        } else if (i < spec.add_parameters.size()) {
            given[spec.add_parameters[i]] = arguments[i].value;
        } else {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return false;
        }
    }
    auto& items = list_of(control, ref);
    std::size_t position = items.size();
    if (const auto index = given.find("index"); index != given.end()) {
        if (!is_number(index->second)) {
            failure = NativeFailure{13, "Type mismatch"};
            return false;
        }
        const long wanted =
            static_cast<long>(std::nearbyint(as_double(index->second)));
        if (wanted < 1 || static_cast<std::size_t>(wanted) > items.size() + 1U) {
            failure = NativeFailure{kIndexOutOfBounds, "Index out of bounds"};
            return false;
        }
        position = static_cast<std::size_t>(wanted) - 1U;
    }
    auto created = std::make_shared<CItem>();
    created->id = state_of(control).next_id++;
    for (const auto& property : spec.props) {
        created->p[property.name] = initial_value(property);
    }
    for (auto& [name, value] : given) {
        if (name == "index" || name == "relative" || name == "relationship") {
            continue;
        }
        const PropSpec* property = find_property(spec, name);
        if (property == nullptr) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return false;
        }
        if (name == "key" && !std::holds_alternative<std::string>(value)) {
            failure = NativeFailure{13, "Type mismatch"};
            return false;
        }
        Value converted = value;
        if (!coerce_prop(in, *property, converted, failure)) {
            return false;
        }
        created->p[name] = std::move(converted);
    }
    const auto& key = created->p.at("key");
    if (const auto* text = std::get_if<std::string>(&key);
        text != nullptr && !text->empty() && key_in_use(items, *text, nullptr)) {
        failure = NativeFailure{kKeyNotUnique, "Key is not unique in collection"};
        return false;
    }
    if (ref.kind == "listimage") {
        const auto picture = created->p.find("picture");
        if (picture == created->p.end() || picture_of(picture->second) == nullptr) {
            failure = NativeFailure{481, "Invalid picture"};
            return false;
        }
    }
    items.insert(items.begin() + static_cast<std::ptrdiff_t>(position), created);
    if (!ComctlHooks::after_add(in, control, ref.kind, created)) {
        failure = FormSupport::reported();
        return true;
    }
    ComctlHooks::changed(in, control);
    result = wrap_item(ref.owner.lock(), ref, created);
    return true;
}

}  // namespace

bool ComctlItem::get(Interpreter& in, const std::string& member,
                     std::vector<CallArgument>& arguments, Value& result,
                     NativeFailure& failure) {
    ControlObject* control = control_for(ref.owner);
    if (control == nullptr) {
        failure = NativeFailure{91, "Object variable or With block variable not set"};
        return true;
    }
    const KindSpec& spec = spec_of(ref.kind);
    const std::string name =
        member == "_default" ? std::string(spec.default_property) : member;
    if (ComctlHooks::item_get(in, *control, ref.kind, item, ref, name, arguments,
                              result, failure)) {
        return true;
    }
    if (name == "index") {
        result = Value{static_cast<Integer>(
            position_of(list_of(*control, ref), item))};
        return true;
    }
    if (find_property(spec, name) != nullptr) {
        result = item->p.at(name);
        return true;
    }
    return false;
}

bool ComctlItem::put(Interpreter& in, const std::string& member,
                     std::vector<CallArgument>& arguments, const Value& value,
                     NativeFailure& failure) {
    ControlObject* control = control_for(ref.owner);
    if (control == nullptr) {
        failure = NativeFailure{91, "Object variable or With block variable not set"};
        return true;
    }
    const KindSpec& spec = spec_of(ref.kind);
    const std::string name =
        member == "_default" ? std::string(spec.default_property) : member;
    if (ComctlHooks::item_put(in, *control, ref.kind, item, ref, name, arguments,
                              value, failure)) {
        return true;
    }
    const PropSpec* property = find_property(spec, name);
    if (property == nullptr) {
        return false;
    }
    Value converted = value;
    if (!coerce_prop(in, *property, converted, failure)) {
        return true;
    }
    if (name == "key") {
        const auto* text = std::get_if<std::string>(&converted);
        if (text != nullptr && !text->empty() &&
            key_in_use(list_of(*control, ref), *text, item)) {
            failure = NativeFailure{kKeyNotUnique, "Key is not unique in collection"};
            return true;
        }
    }
    item->p[name] = std::move(converted);
    ComctlHooks::changed(in, *control);
    return true;
}

bool ComctlCollection::get(Interpreter& in, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) {
    ControlObject* control = control_for(ref.owner);
    if (control == nullptr) {
        failure = NativeFailure{91, "Object variable or With block variable not set"};
        return true;
    }
    auto& items = list_of(*control, ref);
    if (member == "count") {
        result = Value{static_cast<Integer>(items.size())};
        return true;
    }
    if (member == "item" || member == "_default") {
        if (arguments.size() != 1U || arguments[0].omitted) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const ItemPtr found = find_item(items, arguments[0].value, failure);
        if (found != nullptr) {
            result = wrap_item(ref.owner.lock(), ref, found);
        }
        return true;
    }
    if (member == "add") {
        if (!add_to_collection(in, *control, ref, arguments, result, failure) &&
            failure.number == 0) {
            failure = FormSupport::reported();
        }
        return true;
    }
    if (member == "remove") {
        if (arguments.size() != 1U) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const ItemPtr found = find_item(items, arguments[0].value, failure);
        if (found == nullptr) {
            return true;
        }
        if (!ComctlHooks::before_remove(in, *control, ref.kind, found)) {
            failure = FormSupport::reported();
            return true;
        }
        auto& current = list_of(*control, ref);
        current.erase(std::remove(current.begin(), current.end(), found),
                      current.end());
        ComctlHooks::changed(in, *control);
        result = Value{Empty{}};
        return true;
    }
    if (member == "clear") {
        for (const auto& item : std::vector<ItemPtr>(items)) {
            if (!ComctlHooks::before_remove(in, *control, ref.kind, item)) {
                failure = FormSupport::reported();
                return true;
            }
        }
        list_of(*control, ref).clear();
        if (ref.kind == "node") {
            state_of(*control).root_kids.clear();
        }
        ComctlHooks::changed(in, *control);
        result = Value{Empty{}};
        return true;
    }
    return ComctlHooks::collection_get(in, *control, ref, member, arguments,
                                       result, failure);
}

// ----- ComctlSupport ----------------------------------------------------------

void ComctlSupport::init(ControlObject& control) {
    if (control.cls != nullptr && !control.is_form &&
        comctl_class(control.cls->name) == control.cls) {
        control.comctl = std::make_shared<ComctlState>();
    }
}

namespace {

[[nodiscard]] long offset_now(Interpreter& in) {
    return static_cast<long>(FormSupport::offset_of(in));
}

// Raises `event` of `control` with evaluated arguments.
[[nodiscard]] bool raise_event(Interpreter& in, ControlObject& control,
                               const std::string& event,
                               std::vector<Value> arguments) {
    const auto form = control.form.lock();
    if (form == nullptr) {
        return true;
    }
    return FormSupport::raise(in, *form, control.name, control.index,
                              lower_text(event), arguments,
                              static_cast<std::size_t>(offset_now(in)));
}

[[nodiscard]] double number_property(ControlObject& control,
                                     const std::string& member) {
    return FormSupport::number_of(control, member);
}

}  // namespace

bool ComctlSupport::get(Interpreter& in, ControlObject& self,
                        const std::string& member,
                        std::vector<CallArgument>& arguments, Value& result,
                        NativeFailure& failure) {
    const auto owner = self.self.lock();
    for (const auto& binding : collection_bindings()) {
        if (self.class_key == binding.control && member == binding.property) {
            ListRef ref;
            ref.kind = binding.kind;
            result = wrap_collection(owner, ref);
            if (!arguments.empty()) {
                // `Tabs(1)`: the arguments index the collection.
                const auto* holder = std::get_if<ObjectInstance>(&result);
                Value indexed;
                if (holder->data->native->get(in, "item", arguments, indexed,
                                              failure)) {
                    result = indexed;
                }
            }
            return true;
        }
    }
    if (self.class_key == "slider") {
        if (member == "getnumticks") {
            const double span = number_property(self, "max") -
                                number_property(self, "min");
            const double step =
                std::max(1.0, number_property(self, "tickfrequency"));
            result = Value{static_cast<Integer>(std::floor(span / step) + 1.0)};
            return true;
        }
        if (member == "clearsel") {
            self.values["selstart"] = Value{Integer{0}};
            self.values["sellength"] = Value{Integer{0}};
            result = Value{Empty{}};
            return true;
        }
        if (member == "doclick") {
            if (!raise_event(in, self, "Click", {})) {
                failure = FormSupport::reported();
            }
            result = Value{Empty{}};
            return true;
        }
        if (member == "_value") {
            return get(in, self, "value", arguments, result, failure);
        }
    }
    if (self.class_key == "imagelist") {
        if (member == "himagelist") {
            result = Value{static_cast<Integer>(
                0x1000 + (reinterpret_cast<std::uintptr_t>(&self) & 0xFFF0))};
            return true;
        }
        if (member == "overlay") {
            // The composite of two images: the first stands for it here.
            if (arguments.size() != 2U) {
                failure = NativeFailure{450, "Wrong number of arguments"};
                return true;
            }
            ListRef ref;
            ref.kind = "listimage";
            auto& items = list_of(self, ref);
            const ItemPtr first = find_item(items, arguments[0].value, failure);
            const ItemPtr second = find_item(items, arguments[1].value, failure);
            if (first != nullptr && second != nullptr) {
                result = first->p.at("picture");
            }
            return true;
        }
    }
    return false;
}

bool ComctlSupport::put(Interpreter& in, ControlObject& self,
                        const std::string& member_name,
                        std::vector<CallArgument>& arguments,
                        const Value& value, NativeFailure& failure) {
    static_cast<void>(arguments);
    const std::string member = member_name == "_value" ? "value" : member_name;
    const bool progress = self.class_key == "progressbar";
    const bool slider = self.class_key == "slider";
    if ((progress || slider) &&
        (member == "value" || member == "min" || member == "max")) {
        Value converted = value;
        if (!coerce_row(in, self, member, converted, failure)) {
            return true;
        }
        double minimum = number_property(self, "min");
        double maximum = number_property(self, "max");
        double current = number_property(self, "value");
        const double wanted = as_double(converted);
        if (member == "min") {
            minimum = wanted;
        } else if (member == "max") {
            maximum = wanted;
        }
        if (minimum > maximum ||
            (member == "value" && (wanted < minimum || wanted > maximum))) {
            failure = NativeFailure{380, "Invalid property value"};
            return true;
        }
        const double before = current;
        self.values[member] = converted;
        if (member == "value") {
            current = wanted;
        }
        if (member != "value") {
            current = std::min(std::max(current, minimum), maximum);
            const auto* row = ui::find_member(*self.cls, "value", 'l');
            Value clamped{current};
            NativeFailure ignored;
            if (row != nullptr) {
                static_cast<void>(coerce_property(in, clamped, row->type, ignored));
            }
            self.values["value"] = clamped;
        }
        if (slider && current != before &&
            !raise_event(in, self, "Change", {})) {
            failure = FormSupport::reported();
        }
        return true;
    }
    if (self.class_key == "imagelist" &&
        (member == "imagewidth" || member == "imageheight")) {
        ListRef ref;
        ref.kind = "listimage";
        if (!list_of(self, ref).empty()) {
            failure = NativeFailure{kReadOnlyAfterUse,
                                    "Property is read-only after images are added"};
            return true;
        }
    }
    return false;
}


// ----- Per-kind hooks (extended control by control) -------------------------------

void ComctlHooks::changed(Interpreter& in, ControlObject& control) {
    static_cast<void>(in);
    static_cast<void>(control);
}

bool ComctlHooks::before_remove(Interpreter& in, ControlObject& control,
                                const std::string& kind, const ItemPtr& item) {
    static_cast<void>(in);
    static_cast<void>(control);
    static_cast<void>(kind);
    static_cast<void>(item);
    return true;
}

bool ComctlHooks::after_add(Interpreter& in, ControlObject& control,
                            const std::string& kind, const ItemPtr& item) {
    static_cast<void>(in);
    if (kind == "listimage" && control.class_key == "imagelist") {
        // The first image sets the size of the list (pixels at 96 dpi).
        ListRef ref;
        ref.kind = kind;
        if (list_of(control, ref).size() == 1U) {
            if (const PictureObject* picture = picture_of(item->p.at("picture"))) {
                const auto pixels = [](const long himetric) {
                    return static_cast<Int16>(
                        std::lround(static_cast<double>(himetric) * 96.0 / 2540.0));
                };
                control.values["imagewidth"] = Value{pixels(picture->info.width)};
                control.values["imageheight"] = Value{pixels(picture->info.height)};
            }
        }
    }
    return true;
}

bool ComctlHooks::item_get(Interpreter& in, ControlObject& control,
                           const std::string& kind, const ItemPtr& item,
                           const ListRef& ref, const std::string& member,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) {
    static_cast<void>(in);
    static_cast<void>(control);
    static_cast<void>(kind);
    static_cast<void>(item);
    static_cast<void>(ref);
    static_cast<void>(member);
    static_cast<void>(arguments);
    static_cast<void>(result);
    static_cast<void>(failure);
    return false;
}

bool ComctlHooks::item_put(Interpreter& in, ControlObject& control,
                           const std::string& kind, const ItemPtr& item,
                           const ListRef& ref, const std::string& member,
                           std::vector<CallArgument>& arguments,
                           const Value& value, NativeFailure& failure) {
    static_cast<void>(in);
    static_cast<void>(control);
    static_cast<void>(kind);
    static_cast<void>(item);
    static_cast<void>(ref);
    static_cast<void>(member);
    static_cast<void>(arguments);
    static_cast<void>(value);
    static_cast<void>(failure);
    return false;
}

bool ComctlHooks::collection_get(Interpreter& in, ControlObject& control,
                                 const ListRef& ref, const std::string& member,
                                 std::vector<CallArgument>& arguments,
                                 Value& result, NativeFailure& failure) {
    static_cast<void>(in);
    static_cast<void>(control);
    static_cast<void>(ref);
    static_cast<void>(member);
    static_cast<void>(arguments);
    static_cast<void>(result);
    static_cast<void>(failure);
    return false;
}

bool ComctlSupport::user_input(Interpreter& in, InstanceData& form,
                               ControlObject& self, const std::string& action,
                               const std::string& rest) {
    static_cast<void>(form);
    if (self.class_key == "slider" && action == "value") {
        // The user drags the thumb: Scroll while moving, then Change.
        const double wanted = std::atof(rest.c_str());
        const double minimum = number_property(self, "min");
        const double maximum = number_property(self, "max");
        const double target = std::min(std::max(wanted, minimum), maximum);
        if (target != number_property(self, "value")) {
            self.values["value"] = Value{static_cast<Integer>(target)};
            return raise_event(in, self, "Scroll", {}) &&
                   raise_event(in, self, "Change", {});
        }
    }
    return true;
}
