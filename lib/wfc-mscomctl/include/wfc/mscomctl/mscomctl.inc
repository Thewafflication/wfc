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
           {"highlighted", "Boolean", ""},
           {"selected", "Boolean", ""}},
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

// State of a Common Dialog or Winsock control (ocx.inc).
struct OcxState {
    virtual ~OcxState() = default;
};

struct ComctlState {
    std::shared_ptr<OcxState> ocx;
    long next_id{1};
    bool from_host{false};  // the change came from the host: do not echo it
    std::map<std::string, std::vector<std::shared_ptr<CItem>>> lists;
    std::vector<long> root_kids;  // tree: top-level node ids in order
};


std::vector<std::string> comctl_type_names() {
    std::vector<std::string> names;
    for (const auto& control : ui::comctl_classes()) {
        names.push_back(lower_text(std::string(control.name)));
    }
    for (const auto& control : ui::ocx_classes()) {
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
        base = ui::find_ocx_class(leaf);
    }
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

const ui::ControlClass* library_control_class(const std::string_view library,
                                              const std::string_view leaf) {
    const std::string name = lower_text(std::string(library));
    if (name == "mscomctllib") {
        return ui::find_comctl_class(leaf) != nullptr ? comctl_class(leaf)
                                                      : nullptr;
    }
    if (name == "mscomdlg") {
        return lower_text(std::string(leaf)) == "commondialog"
                   ? comctl_class(leaf)
                   : nullptr;
    }
    if (name == "mswinsocklib") {
        return lower_text(std::string(leaf)) == "winsock" ? comctl_class(leaf)
                                                          : nullptr;
    }
    return nullptr;
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
        {"commondialog",
         {{"cancelerror", Value{false}},
          {"color", Value{Integer{0}}},
          {"copies", small(1)},
          {"defaultext", Value{std::string{}}},
          {"dialogtitle", Value{std::string{}}},
          {"filename", Value{std::string{}}},
          {"filetitle", Value{std::string{}}},
          {"filter", Value{std::string{}}},
          {"filterindex", small(0)},
          {"flags", Value{Integer{0}}},
          {"fontbold", Value{false}},
          {"fontitalic", Value{false}},
          {"fontname", Value{std::string{}}},
          {"fontsize", Value{8.0F}},
          {"fontstrikethru", Value{false}},
          {"fontunderline", Value{false}},
          {"frompage", small(0)},
          {"helpcommand", small(0)},
          {"helpcontext", Value{Integer{0}}},
          {"helpfile", Value{std::string{}}},
          {"helpkey", Value{std::string{}}},
          {"initdir", Value{std::string{}}},
          {"max", small(0)},
          {"maxfilesize", small(260)},
          {"min", small(0)},
          {"orientation", small(1)},
          {"printerdefault", Value{true}},
          {"topage", small(0)},
          {"hdc", Value{Integer{0}}}}},
        {"winsock",
         {{"bytesreceived", Value{Integer{0}}},
          {"localport", Value{Integer{0}}},
          {"protocol", Value{Integer{0}}},
          {"remotehost", Value{std::string{}}},
          {"remotehostip", Value{std::string{}}},
          {"remoteport", Value{Integer{0}}},
          {"sockethandle", Value{Integer{0}}},
          {"state", Value{Integer{0}}}}},
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
constexpr Integer kPanelsFull = 35609;
constexpr Integer kImageListBound = 35615;

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
    static bool node_add(Interpreter& in, ControlObject& control,
                         std::vector<CallArgument>& arguments, Value& result,
                         NativeFailure& failure);
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
        if (ref.kind == "listimage" && control.values.contains("_bound") &&
            position != items.size()) {
            failure = NativeFailure{kImageListBound,
                                    "Images can only be appended to a bound ImageList"};
            return false;
        }
    }
    if (ref.kind == "panel" && items.size() >= 16U) {
        failure = NativeFailure{kPanelsFull, "A status bar holds at most sixteen panels"};
        return false;
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
        if (!arguments.empty()) {
            // `Button.ButtonMenus("key")`: the arguments index a collection.
            const auto* holder = std::get_if<ObjectInstance>(&result);
            if (holder != nullptr && holder->data != nullptr &&
                dynamic_cast<ComctlCollection*>(holder->data->native.get())) {
                Value indexed;
                if (holder->data->native->get(in, "item", arguments, indexed,
                                              failure)) {
                    result = indexed;
                }
            }
        }
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
        if (failure.number == 0) {
            ComctlHooks::changed(in, *control);
        }
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
    if (member == "add" && ref.kind == "node") {
        if (ComctlHooks::node_add(in, *control, arguments, result, failure)) {
            ComctlHooks::changed(in, *control);
        } else if (failure.number == 0) {
            failure = FormSupport::reported();
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
    if ((member == "remove" || member == "clear") && ref.kind == "listimage" &&
        control->values.contains("_bound")) {
        failure = NativeFailure{kImageListBound,
                                "Images cannot be removed from a bound ImageList"};
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

void ComctlSupport::from_host(ControlObject& control, const bool on) {
    if (control.comctl != nullptr) {
        control.comctl->from_host = on;
    }
}

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

[[nodiscard]] bool control_flag(ControlObject& control, const char* name) {
    const auto found = control.values.find(name);
    return found != control.values.end() &&
           std::holds_alternative<bool>(found->second) &&
           std::get<bool>(found->second);
}

}  // namespace

[[nodiscard]] bool ocx_get(Interpreter& in, ControlObject& self,
                          const std::string& member,
                          std::vector<CallArgument>& arguments, Value& result,
                          NativeFailure& failure);
[[nodiscard]] bool ocx_put(Interpreter& in, ControlObject& self,
                           const std::string& member, const Value& value,
                           NativeFailure& failure);
[[nodiscard]] bool comctl_control_get(Interpreter& in, ControlObject& self,
                                      const std::string& member,
                                      std::vector<CallArgument>& arguments,
                                      Value& result, NativeFailure& failure);
[[nodiscard]] bool comctl_control_put(Interpreter& in, ControlObject& self,
                                      const std::string& member,
                                      const Value& value,
                                      NativeFailure& failure);

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
        if (!control_flag(self, "selectrange") &&
            (member == "selstart" || member == "sellength")) {
            // Without a selection range SelStart follows Value.
            result = member == "selstart" ? self.values.contains("value")
                                                 ? self.values.at("value")
                                                 : Value{Integer{0}}
                                          : Value{Integer{0}};
            return true;
        }
        if (member == "getnumticks") {
            const double span = number_property(self, "max") -
                                number_property(self, "min");
            const double step =
                std::max(1.0, number_property(self, "tickfrequency"));
            result = Value{static_cast<Integer>(std::floor(span / step) + 1.0)};
            return true;
        }
        if (member == "clearsel") {
            self.values["selstart"] = self.values.contains("value")
                                          ? self.values.at("value")
                                          : Value{Integer{0}};
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
    return comctl_control_get(in, self, member, arguments, result, failure);
}

bool ComctlSupport::put(Interpreter& in, ControlObject& self,
                        const std::string& member_name,
                        std::vector<CallArgument>& arguments,
                        const Value& value, NativeFailure& failure) {
    static_cast<void>(arguments);
    const std::string member = member_name == "_value" ? "value" : member_name;
    if (member == "imagelist" || member == "icons" || member == "smallicons" ||
        member == "hotimagelist" || member == "disabledimagelist" ||
        member == "columnheadericons") {
        // Binding an ImageList to a control: its images can then only grow.
        if (ControlObject* list = control_of(value);
            list != nullptr && list->class_key == "imagelist") {
            list->values["_bound"] = Value{true};
        }
    }
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
    if (slider && (member == "selstart" || member == "sellength")) {
        Value converted = value;
        if (!coerce_row(in, self, member, converted, failure)) {
            return true;
        }
        const double wanted = as_double(converted);
        const double minimum = number_property(self, "min");
        const double maximum = number_property(self, "max");
        if (!control_flag(self, "selectrange")) {
            if (member == "selstart") {
                if (wanted < minimum || wanted > maximum) {
                    failure = NativeFailure{380, "Invalid property value"};
                    return true;
                }
                // SelStart and Value are coupled while SelectRange is false.
                const bool changed = wanted != number_property(self, "value");
                self.values["value"] = Value{static_cast<Integer>(wanted)};
                if (changed && !raise_event(in, self, "Change", {})) {
                    failure = FormSupport::reported();
                }
            }
            return true;  // SelLength has no effect in this mode
        }
        const double start =
            member == "selstart" ? wanted : number_property(self, "selstart");
        const double length =
            member == "sellength" ? wanted : number_property(self, "sellength");
        if (start < minimum || start > maximum || length < 0 ||
            start + length > maximum) {
            failure = NativeFailure{380, "Invalid property value"};
            return true;
        }
        self.values[member] = converted;
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
    return comctl_control_put(in, self, member, value, failure);
}



// ----- Per-kind behavior ------------------------------------------------------------

namespace {

// What every per-kind function receives.
struct HookCtx {
    Interpreter& in;
    ControlObject& control;
    ListRef ref;
    ItemPtr item;
};

[[nodiscard]] std::vector<ItemPtr>& top_list(ControlObject& control,
                                             const std::string& kind) {
    ListRef ref;
    ref.kind = kind;
    return list_of(control, ref);
}

[[nodiscard]] bool flag(const ItemPtr& item, const char* name) {
    const auto found = item->p.find(name);
    return found != item->p.end() && std::holds_alternative<bool>(found->second) &&
           std::get<bool>(found->second);
}

[[nodiscard]] double item_number(const ItemPtr& item, const char* name) {
    const auto found = item->p.find(name);
    return found != item->p.end() && is_number(found->second)
               ? as_double(found->second)
               : 0.0;
}

[[nodiscard]] Value item_value(HookCtx& ctx, const ItemPtr& item,
                               const ListRef& ref) {
    return wrap_item(ctx.ref.owner.lock(), ref, item);
}

[[nodiscard]] Value nothing_value() { return Value{Nothing{}}; }

// ----- TabStrip -----------------------------------------------------------------

struct TabRect {
    double left{0};
    double top{0};
    double width{0};
    double height{0};
};

// The rectangle of every tab, laid out left to right (wrapping to rows with
// MultiRow). An approximation of the OCX layout in twips.
[[nodiscard]] std::vector<TabRect> tab_layout(ControlObject& control) {
    std::vector<TabRect> rects;
    const double fixed_width = number_property(control, "tabfixedwidth");
    const double fixed_height = number_property(control, "tabfixedheight");
    const double minimum = number_property(control, "tabminwidth");
    const long width_style =
        static_cast<long>(number_property(control, "tabwidthstyle"));
    const double height = fixed_height > 0.0 ? fixed_height : 300.0;
    const double total = number_property(control, "width");
    const bool multi = control.values.contains("multirow") &&
                       std::holds_alternative<bool>(control.values.at("multirow")) &&
                       std::get<bool>(control.values.at("multirow"));
    double x = 0.0;
    double y = 0.0;
    for (const auto& tab : top_list(control, "tab")) {
        if (!flag(tab, "visible")) {
            rects.push_back({});
            continue;
        }
        const auto caption = tab->p.at("caption");
        const std::string text = std::holds_alternative<std::string>(caption)
                                     ? std::get<std::string>(caption)
                                     : std::string{};
        double width = width_style == 2 && fixed_width > 0.0
                           ? fixed_width
                           : std::max(minimum > 0.0 ? minimum : 600.0,
                                      120.0 + 95.0 * static_cast<double>(text.size()));
        if (multi && x > 0.0 && x + width > total && total > 0.0) {
            x = 0.0;
            y += height;
        }
        rects.push_back({x, y, width, height});
        x += width;
    }
    return rects;
}

[[nodiscard]] ItemPtr selected_tab(ControlObject& control) {
    for (const auto& tab : top_list(control, "tab")) {
        if (flag(tab, "selected")) {
            return tab;
        }
    }
    return nullptr;
}

// Selects `tab`; with `notify`, BeforeClick (cancellable) and Click run.
[[nodiscard]] bool select_tab(Interpreter& in, ControlObject& control,
                              const ItemPtr& tab, const bool notify,
                              NativeFailure& failure) {
    if (flag(tab, "selected")) {
        return true;
    }
    if (notify) {
        Value cancel{Int16{0}};
        const auto form = control.form.lock();
        std::vector<Value> arguments{cancel};
        if (form != nullptr &&
            !FormSupport::raise(in, *form, control.name, control.index,
                                "beforeclick", arguments,
                                FormSupport::offset_of(in))) {
            failure = FormSupport::reported();
            return false;
        }
        if (whole_value(arguments[0]).value_or(0) != 0) {
            // Cancelled: the selection stays but Click is still raised.
            if (!raise_event(in, control, "Click", {})) {
                failure = FormSupport::reported();
                return false;
            }
            return true;
        }
    }
    for (const auto& other : top_list(control, "tab")) {
        other->p["selected"] = Value{other == tab};
    }
    if (notify && !raise_event(in, control, "Click", {})) {
        failure = FormSupport::reported();
        return false;
    }
    return true;
}

[[nodiscard]] bool tab_get(HookCtx& ctx, const std::string& member,
                           Value& result) {
    const auto rects = tab_layout(ctx.control);
    const long position = position_of(top_list(ctx.control, "tab"), ctx.item);
    const TabRect rect = position >= 1 && static_cast<std::size_t>(position) <= rects.size()
                             ? rects[static_cast<std::size_t>(position) - 1U]
                             : TabRect{};
    if (member == "left") {
        result = Value{static_cast<float>(rect.left)};
    } else if (member == "top") {
        result = Value{static_cast<float>(rect.top)};
    } else if (member == "width") {
        result = Value{static_cast<float>(rect.width)};
    } else if (member == "height") {
        result = Value{static_cast<float>(rect.height)};
    } else {
        return false;
    }
    return true;
}

// ----- StatusBar ----------------------------------------------------------------

[[nodiscard]] bool panel_get(HookCtx& ctx, const std::string& member,
                             Value& result) {
    if (member == "left") {
        double x = 0.0;
        for (const auto& panel : top_list(ctx.control, "panel")) {
            if (panel == ctx.item) {
                break;
            }
            if (flag(panel, "visible")) {
                x += std::max(item_number(panel, "width"),
                              item_number(panel, "minwidth"));
            }
        }
        result = Value{static_cast<float>(x)};
        return true;
    }
    return false;
}

// ----- Toolbar ------------------------------------------------------------------

constexpr int kStyleDefault = 0;
constexpr int kStyleCheck = 1;
constexpr int kStyleGroup = 2;
constexpr int kStyleSeparator = 3;
constexpr int kStylePlaceholder = 4;
constexpr int kStyleDropdown = 5;

[[nodiscard]] int button_style(const ItemPtr& button) {
    return static_cast<int>(item_number(button, "style"));
}

// A word of a scripted action: a number, or else a key.
[[nodiscard]] Value selector_of(const std::string& text) {
    char* end = nullptr;
    const double number = std::strtod(text.c_str(), &end);
    if (end != text.c_str() && std::string(end).empty()) {
        return Value{number};
    }
    return Value{text};
}

// Position and size of every button in twips (wrapping with `Wrappable`
// starts a new row when the toolbar is full).
[[nodiscard]] std::vector<TabRect> button_layout(ControlObject& control) {
    std::vector<TabRect> rects;
    const double height = number_property(control, "buttonheight") > 0.0
                              ? number_property(control, "buttonheight")
                              : 330.0;
    const double total = number_property(control, "width");
    const double fixed = number_property(control, "buttonwidth");
    bool wrap = true;
    if (const auto found = control.values.find("wrappable");
        found != control.values.end() &&
        std::holds_alternative<bool>(found->second)) {
        wrap = std::get<bool>(found->second);
    }
    double x = 0.0;
    double y = 0.0;
    for (const auto& button : top_list(control, "button")) {
        if (!flag(button, "visible")) {
            rects.push_back({});
            continue;
        }
        const int style = button_style(button);
        const auto caption = button->p.at("caption");
        const std::size_t length =
            std::holds_alternative<std::string>(caption)
                ? std::get<std::string>(caption).size()
                : 0U;
        double width = 360.0;
        if (style == kStyleSeparator) {
            width = 120.0;
        } else if (style == kStylePlaceholder) {
            width = std::max(item_number(button, "width"), 120.0);
        } else if (item_number(button, "width") > 0.0) {
            width = item_number(button, "width");
        } else if (fixed > 0.0) {
            width = fixed;
        } else {
            width = std::max(360.0, 120.0 + 95.0 * static_cast<double>(length));
        }
        if (wrap && total > 0.0 && x > 0.0 && x + width > total) {
            x = 0.0;
            y += height;
        }
        rects.push_back({x, y, width, height});
        x += width;
    }
    return rects;
}

// Presses `button` of a button group and releases the others of the group.
void press_group(ControlObject& control, const ItemPtr& button) {
    auto& buttons = top_list(control, "button");
    const long position = position_of(buttons, button);
    if (position == 0) {
        return;
    }
    std::size_t first = static_cast<std::size_t>(position) - 1U;
    std::size_t last = first;
    while (first > 0 && button_style(buttons[first - 1U]) == kStyleGroup) {
        --first;
    }
    while (last + 1U < buttons.size() &&
           button_style(buttons[last + 1U]) == kStyleGroup) {
        ++last;
    }
    for (std::size_t i = first; i <= last; ++i) {
        buttons[i]->p["value"] =
            Value{static_cast<Int16>(buttons[i] == button ? 1 : 0)};
    }
}

[[nodiscard]] bool button_get(HookCtx& ctx, const std::string& member,
                              Value& result) {
    if (member == "buttonmenus") {
        ListRef ref;
        ref.kind = "buttonmenu";
        ref.parent = ctx.item;
        result = wrap_collection(ctx.ref.owner.lock(), ref);
        return true;
    }
    const auto rects = button_layout(ctx.control);
    const long position =
        position_of(top_list(ctx.control, "button"), ctx.item);
    TabRect rect;
    if (position >= 1 && static_cast<std::size_t>(position) <= rects.size()) {
        rect = rects[static_cast<std::size_t>(position) - 1U];
    }
    if (member == "left") {
        result = Value{static_cast<float>(rect.left)};
    } else if (member == "top") {
        result = Value{static_cast<float>(rect.top)};
    } else if (member == "height") {
        result = Value{static_cast<float>(rect.height)};
    } else if (member == "width") {
        result = Value{static_cast<float>(rect.width)};
    } else {
        return false;
    }
    return true;
}

// The user presses `button`.
[[nodiscard]] bool click_button(Interpreter& in, ControlObject& control,
                                const ItemPtr& button) {
    const int style = button_style(button);
    if (!flag(button, "enabled") || style == kStyleSeparator ||
        style == kStylePlaceholder) {
        return true;
    }
    if (style == kStyleCheck) {
        button->p["value"] = Value{
            static_cast<Int16>(item_number(button, "value") == 0.0 ? 1 : 0)};
    } else if (style == kStyleGroup) {
        press_group(control, button);
    }
    ListRef ref;
    ref.kind = "button";
    return raise_event(in, control, "ButtonClick",
                       {wrap_item(control.self.lock(), ref, button)});
}

// Saved toolbar layouts (SaveToolbar / RestoreToolbar), per process.
std::map<std::string, std::string>& saved_toolbars() {
    static std::map<std::string, std::string> saved;
    return saved;
}

[[nodiscard]] bool toolbar_member(ControlObject& self,
                                  const std::string& member,
                                  std::vector<CallArgument>& arguments,
                                  Value& result, NativeFailure& failure) {
    if (member == "customize") {
        result = Value{Empty{}};  // there is no customization dialog
        return true;
    }
    if (member != "savetoolbar" && member != "restoretoolbar") {
        return false;
    }
    if (arguments.size() != 3U) {
        failure = NativeFailure{450, "Wrong number of arguments"};
        return true;
    }
    std::string name;
    for (const auto& argument : arguments) {
        name += FormSupport::text_of(argument.value) + "/";
    }
    const std::string registry_key = FormSupport::text_of(arguments[0].value);
    const std::string registry_subkey = FormSupport::text_of(arguments[1].value);
    const std::string registry_value = FormSupport::text_of(arguments[2].value);
    result = Value{Empty{}};
    if (member == "savetoolbar") {
        std::string layout;
        for (const auto& button : top_list(self, "button")) {
            layout += FormSupport::text_of(button->p.at("key")) + "=" +
                      (flag(button, "visible") ? "1" : "0") + ";";
        }
        saved_toolbars()[name] = layout;
        // Also under HKEY_CURRENT_USER\<Key>\<Subkey>, value <Value>.
        static_cast<void>(wfc_registry_write(registry_key, registry_subkey,
                                             registry_value, layout));
        return true;
    }
    std::string stored;
    if (const auto from_registry =
            wfc_registry_read(registry_key, registry_subkey, registry_value)) {
        stored = *from_registry;
    } else if (const auto found = saved_toolbars().find(name);
               found != saved_toolbars().end()) {
        stored = found->second;
    } else {
        return true;
    }
    const std::string& layout = stored;
    std::size_t start = 0;
    while (start < layout.size()) {
        const auto end = layout.find(';', start);
        const std::string entry = layout.substr(start, end - start);
        const auto equals = entry.find('=');
        if (equals != std::string::npos) {
            for (const auto& button : top_list(self, "button")) {
                if (same_key(button->p.at("key"), entry.substr(0, equals))) {
                    button->p["visible"] =
                        Value{entry.substr(equals + 1U) == "1"};
                }
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1U;
    }
    return true;
}

[[nodiscard]] bool toolbar_input(Interpreter& in, ControlObject& self,
                                 const std::string& action,
                                 const std::string& rest) {
    NativeFailure ignored;
    if (action == "button" || action == "dropdown") {
        const ItemPtr button =
            find_item(top_list(self, "button"), selector_of(rest), ignored);
        if (button == nullptr) {
            return true;
        }
        if (action == "button") {
            return click_button(in, self, button);
        }
        if (button_style(button) == kStyleDropdown && flag(button, "enabled")) {
            ListRef ref;
            ref.kind = "button";
            return raise_event(in, self, "ButtonDropDown",
                               {wrap_item(self.self.lock(), ref, button)});
        }
        return true;
    }
    if (action == "menu") {
        std::istringstream words(rest);
        std::string button_text;
        std::string menu_text;
        words >> button_text >> menu_text;
        const ItemPtr button = find_item(top_list(self, "button"),
                                         selector_of(button_text), ignored);
        if (button == nullptr) {
            return true;
        }
        const ItemPtr menu =
            find_item(button->sub, selector_of(menu_text), ignored);
        if (menu == nullptr || !flag(menu, "enabled")) {
            return true;
        }
        ListRef ref;
        ref.kind = "buttonmenu";
        ref.parent = button;
        return raise_event(in, self, "ButtonMenuClick",
                           {wrap_item(self.self.lock(), ref, menu)});
    }
    if (action == "dblclick") {
        // A double-click opens the customization interface when allowed; with
        // no dialog only the events run.
        bool allowed = true;
        if (const auto found = self.values.find("allowcustomize");
            found != self.values.end() &&
            std::holds_alternative<bool>(found->second)) {
            allowed = std::get<bool>(found->second);
        }
        return raise_event(in, self, "DblClick", {}) &&
               (!allowed || raise_event(in, self, "Change", {}));
    }
    if (action == "customize") {
        return raise_event(in, self, "Change", {});
    }
    return true;
}

// ----- Label editing (TreeView and ListView) ---------------------------------------

// Starts an edit: BeforeLabelEdit(Cancel). `started` tells whether it was not
// cancelled; false return means an error was raised.
[[nodiscard]] bool begin_label_edit(Interpreter& in, ControlObject& control,
                                    bool& started) {
    started = false;
    const auto form = control.form.lock();
    if (form == nullptr) {
        return true;
    }
    std::vector<Value> before{Value{Int16{0}}};
    if (!FormSupport::raise(in, *form, control.name, control.index,
                            "beforelabeledit", before, FormSupport::offset_of(in))) {
        return false;
    }
    if (whole_value(before[0]).value_or(0) != 0) {
        return true;
    }
    control.values["_editing"] = Value{true};
    started = true;
    return true;
}

// Ends an edit: AfterLabelEdit(Cancel, NewString) with the text, or with
// Null when the user cancelled (`text` empty). `accepted` is the text to keep.
[[nodiscard]] bool end_label_edit(Interpreter& in, ControlObject& control,
                                  const std::optional<std::string>& text,
                                  std::optional<std::string>& accepted) {
    accepted.reset();
    control.values.erase("_editing");
    const auto form = control.form.lock();
    if (form == nullptr) {
        return true;
    }
    std::vector<Value> after{Value{Int16{0}},
                             text.has_value() ? Value{*text} : Value{std::string{}}};
    if (!FormSupport::raise(in, *form, control.name, control.index,
                            "afterlabeledit", after, FormSupport::offset_of(in))) {
        return false;
    }
    if (text.has_value() && whole_value(after[0]).value_or(0) == 0) {
        accepted = std::holds_alternative<std::string>(after[1])
                       ? std::get<std::string>(after[1])
                       : *text;
    }
    return true;
}

// ----- TreeView -----------------------------------------------------------------

[[nodiscard]] ItemPtr node_by_id(ControlObject& control, const long id) {
    for (const auto& node : top_list(control, "node")) {
        if (node->id == id) {
            return node;
        }
    }
    return nullptr;
}

[[nodiscard]] std::vector<long>& kids_of(ControlObject& control,
                                         const ItemPtr& parent) {
    return parent == nullptr ? state_of(control).root_kids : parent->kids;
}

[[nodiscard]] Value node_value(ControlObject& control, const ItemPtr& node) {
    if (node == nullptr) {
        return Value{Nothing{}};
    }
    ListRef ref;
    ref.kind = "node";
    return wrap_item(control.self.lock(), ref, node);
}

[[nodiscard]] std::string node_text(const ItemPtr& node) {
    const auto& text = node->p.at("text");
    return std::holds_alternative<std::string>(text) ? std::get<std::string>(text)
                                                     : std::string{};
}

void sort_kids(ControlObject& control, std::vector<long>& kids) {
    std::stable_sort(kids.begin(), kids.end(), [&](const long a, const long b) {
        const ItemPtr left = node_by_id(control, a);
        const ItemPtr right = node_by_id(control, b);
        return left != nullptr && right != nullptr &&
               lower_text(node_text(left)) < lower_text(node_text(right));
    });
}

[[nodiscard]] bool tree_sorted(ControlObject& control) {
    const auto found = control.values.find("sorted");
    return found != control.values.end() &&
           std::holds_alternative<bool>(found->second) &&
           std::get<bool>(found->second);
}

// Nodes in display order with their depth; children of collapsed nodes are
// left out.
void visible_rows(ControlObject& control, const ItemPtr& parent, const int depth,
                  std::vector<std::pair<ItemPtr, int>>& rows) {
    for (const long id : kids_of(control, parent)) {
        const ItemPtr node = node_by_id(control, id);
        if (node == nullptr) {
            continue;
        }
        rows.emplace_back(node, depth);
        if (flag(node, "expanded")) {
            visible_rows(control, node, depth + 1, rows);
        }
    }
}

[[nodiscard]] ItemPtr selected_node(ControlObject& control) {
    for (const auto& node : top_list(control, "node")) {
        if (flag(node, "selected")) {
            return node;
        }
    }
    return nullptr;
}

[[nodiscard]] ItemPtr node_parent(ControlObject& control, const ItemPtr& node) {
    return node->parent == 0 ? nullptr : node_by_id(control, node->parent);
}

void select_node(ControlObject& control, const ItemPtr& node) {
    for (const auto& other : top_list(control, "node")) {
        other->p["selected"] = Value{other == node};
    }
}

// Expands or collapses `node`, raising the event when the state changes.
[[nodiscard]] bool set_expanded(Interpreter& in, ControlObject& control,
                                const ItemPtr& node, const bool expand) {
    if (flag(node, "expanded") == expand) {
        return true;
    }
    node->p["expanded"] = Value{expand};
    return raise_event(in, control, expand ? "Expand" : "Collapse",
                       {node_value(control, node)});
}

// Nodes.Add(Relative, Relationship, Key, Text, Image, SelectedImage)
[[nodiscard]] bool add_node(Interpreter& in, ControlObject& control,
                            std::vector<CallArgument>& arguments, Value& result,
                            NativeFailure& failure) {
    const KindSpec& spec = spec_of("node");
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
    auto& nodes = top_list(control, "node");
    ItemPtr relative;
    long relationship = 2;  // tvwNext
    if (const auto found = given.find("relative"); found != given.end()) {
        if (const auto* holder = std::get_if<ObjectInstance>(&found->second);
            holder != nullptr && holder->data != nullptr) {
            const auto* object =
                dynamic_cast<ComctlItem*>(holder->data->native.get());
            if (object != nullptr && object->ref.kind == "node" &&
                position_of(nodes, object->item) != 0) {
                relative = object->item;
            }
            if (relative == nullptr) {
                failure = NativeFailure{kElementNotFound, "Element not found"};
                return false;
            }
        } else {
            relative = find_item(nodes, found->second, failure);
            if (relative == nullptr) {
                return false;
            }
        }
        if (const auto way = given.find("relationship"); way != given.end()) {
            if (!is_number(way->second)) {
                failure = NativeFailure{13, "Type mismatch"};
                return false;
            }
            relationship = static_cast<long>(std::nearbyint(as_double(way->second)));
            if (relationship < 0 || relationship > 4) {
                failure = NativeFailure{35607, "Invalid relationship"};
                return false;
            }
        }
    }
    auto created = std::make_shared<CItem>();
    created->id = state_of(control).next_id++;
    for (const auto& property : spec.props) {
        created->p[property.name] = initial_value(property);
    }
    for (auto& [name, value] : given) {
        if (name == "relative" || name == "relationship") {
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
    if (const auto* text = std::get_if<std::string>(&created->p.at("key"));
        text != nullptr && !text->empty() && key_in_use(nodes, *text, nullptr)) {
        failure = NativeFailure{kKeyNotUnique, "Key is not unique in collection"};
        return false;
    }
    // Place the node in the tree.
    ItemPtr parent;
    std::size_t at = 0;
    if (relative == nullptr) {
        parent = nullptr;
        at = kids_of(control, nullptr).size();
    } else if (relationship == 4) {  // tvwChild
        parent = relative;
        at = kids_of(control, parent).size();
    } else {
        parent = node_parent(control, relative);
        auto& siblings = kids_of(control, parent);
        const auto where =
            std::find(siblings.begin(), siblings.end(), relative->id);
        const std::size_t position = static_cast<std::size_t>(where - siblings.begin());
        if (relationship == 0) {
            at = 0;
        } else if (relationship == 1) {
            at = siblings.size();
        } else if (relationship == 2) {
            at = position + 1U;
        } else {
            at = position;
        }
    }
    created->parent = parent == nullptr ? 0 : parent->id;
    auto& siblings = kids_of(control, parent);
    siblings.insert(siblings.begin() + static_cast<std::ptrdiff_t>(at), created->id);
    nodes.push_back(created);
    if (tree_sorted(control) || (parent != nullptr && flag(parent, "sorted"))) {
        sort_kids(control, siblings);
    }
    ListRef ref;
    ref.kind = "node";
    result = wrap_item(control.self.lock(), ref, created);
    return true;
}

// Removes the descendants of `node` and detaches it from its parent.
void detach_node(ControlObject& control, const ItemPtr& node) {
    auto& nodes = top_list(control, "node");
    std::vector<ItemPtr> doomed;
    const auto collect = [&](const auto& self, const ItemPtr& from) -> void {
        for (const long id : from->kids) {
            if (const ItemPtr child = node_by_id(control, id)) {
                doomed.push_back(child);
                self(self, child);
            }
        }
    };
    collect(collect, node);
    for (const auto& child : doomed) {
        nodes.erase(std::remove(nodes.begin(), nodes.end(), child), nodes.end());
    }
    node->kids.clear();
    auto& siblings = kids_of(control, node_parent(control, node));
    siblings.erase(std::remove(siblings.begin(), siblings.end(), node->id),
                   siblings.end());
}

[[nodiscard]] std::string full_path(ControlObject& control, const ItemPtr& node) {
    std::string separator = "\\";
    if (const auto found = control.values.find("pathseparator");
        found != control.values.end() &&
        std::holds_alternative<std::string>(found->second)) {
        separator = std::get<std::string>(found->second);
    }
    std::vector<std::string> parts;
    for (ItemPtr at = node; at != nullptr; at = node_parent(control, at)) {
        parts.push_back(node_text(at));
    }
    std::string path;
    for (auto part = parts.rbegin(); part != parts.rend(); ++part) {
        if (!path.empty()) {
            path += separator;
        }
        path += *part;
    }
    return path;
}

[[nodiscard]] bool node_get(HookCtx& ctx, const std::string& member,
                            Value& result) {
    ControlObject& control = ctx.control;
    const ItemPtr& node = ctx.item;
    const ItemPtr parent = node_parent(control, node);
    auto& siblings = kids_of(control, parent);
    const auto where = std::find(siblings.begin(), siblings.end(), node->id);
    const bool known = where != siblings.end();
    if (member == "parent") {
        result = node_value(control, parent);
    } else if (member == "children") {
        result = Value{static_cast<Integer>(node->kids.size())};
    } else if (member == "child") {
        result = node_value(control, node->kids.empty()
                                         ? nullptr
                                         : node_by_id(control, node->kids.front()));
    } else if (member == "next") {
        result = node_value(control, known && where + 1 != siblings.end()
                                         ? node_by_id(control, *(where + 1))
                                         : nullptr);
    } else if (member == "previous") {
        result = node_value(control, known && where != siblings.begin()
                                         ? node_by_id(control, *(where - 1))
                                         : nullptr);
    } else if (member == "firstsibling") {
        result = node_value(control, siblings.empty() ? nullptr
                                                      : node_by_id(control, siblings.front()));
    } else if (member == "lastsibling") {
        result = node_value(control, siblings.empty() ? nullptr
                                                      : node_by_id(control, siblings.back()));
    } else if (member == "root") {
        ItemPtr at = node;
        while (node_parent(control, at) != nullptr) {
            at = node_parent(control, at);
        }
        result = node_value(control, at);
    } else if (member == "fullpath") {
        result = Value{full_path(control, node)};
    } else if (member == "ensurevisible") {
        bool changed = false;
        if (control.values.contains("scroll") &&
            std::holds_alternative<bool>(control.values.at("scroll")) &&
            !std::get<bool>(control.values.at("scroll"))) {
            result = Value{false};  // EnsureVisible does not work without Scroll
            return true;
        }
        for (ItemPtr at = parent; at != nullptr; at = node_parent(control, at)) {
            if (!flag(at, "expanded")) {
                at->p["expanded"] = Value{true};
                changed = true;
            }
        }
        result = Value{changed};
    } else {
        return false;
    }
    return true;
}

[[nodiscard]] bool node_put(HookCtx& ctx, const std::string& member,
                            const Value& value, NativeFailure& failure) {
    ControlObject& control = ctx.control;
    const ItemPtr& node = ctx.item;
    const auto truth = [&] {
        return std::holds_alternative<bool>(value)
                   ? std::get<bool>(value)
                   : is_number(value) && as_double(value) != 0.0;
    };
    if (member == "children" || member == "parent" || member == "child" ||
        member == "next" || member == "previous" || member == "firstsibling" ||
        member == "lastsibling" || member == "root" || member == "fullpath") {
        failure = NativeFailure{383, "Property is read-only"};
        return true;
    }
    if (member == "expanded") {
        if (!set_expanded(ctx.in, control, node, truth())) {
            failure = FormSupport::reported();
        }
        return true;
    }
    if (member == "selected") {
        if (truth()) {
            select_node(control, node);
        } else {
            node->p["selected"] = Value{false};
        }
        return true;
    }
    if (member == "sorted") {
        node->p["sorted"] = Value{truth()};
        if (truth()) {
            sort_kids(control, node->kids);
        }
        return true;
    }
    return false;
}

[[nodiscard]] bool tree_member(Interpreter& in, ControlObject& self,
                               const std::string& member,
                               std::vector<CallArgument>& arguments,
                               Value& result, NativeFailure& failure) {
    static_cast<void>(in);
    if (member == "selecteditem") {
        result = node_value(self, selected_node(self));
        return true;
    }
    if (member == "drophighlight") {
        const auto found = self.values.find("drophighlight");
        result = found == self.values.end() ? Value{Nothing{}} : found->second;
        return true;
    }
    if (member == "getvisiblecount") {
        std::vector<std::pair<ItemPtr, int>> rows;
        visible_rows(self, nullptr, 0, rows);
        const double height = number_property(self, "height");
        const std::size_t fits = height > 0.0 ? static_cast<std::size_t>(height / 240.0) : rows.size();
        result = Value{static_cast<Integer>(std::min(rows.size(), std::max<std::size_t>(fits, 1)))};
        return true;
    }
    if (member == "hittest") {
        if (arguments.size() != 2U) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const double x = as_double(arguments[0].value);
        const double y = as_double(arguments[1].value);
        std::vector<std::pair<ItemPtr, int>> rows;
        visible_rows(self, nullptr, 0, rows);
        const double indent = number_property(self, "indentation") > 0.0
                                  ? number_property(self, "indentation")
                                  : 285.0;
        result = Value{Nothing{}};
        const double row = std::floor(y / 240.0);
        if (y >= 0.0 && row >= 0.0 && static_cast<std::size_t>(row) < rows.size()) {
            const auto& [node, depth] = rows[static_cast<std::size_t>(row)];
            const double left = static_cast<double>(depth + 1) * indent;
            const double right = left + 120.0 + 95.0 * static_cast<double>(node_text(node).size());
            if (x >= left && x < right) {
                result = node_value(self, node);
            }
        }
        return true;
    }
    if (member == "startlabeledit") {
        bool started = false;
        if (!begin_label_edit(in, self, started)) {
            failure = FormSupport::reported();
        }
        result = Value{Empty{}};
        return true;
    }
    return false;
}

[[nodiscard]] bool tree_input(Interpreter& in, ControlObject& self,
                              const std::string& action, const std::string& rest) {
    NativeFailure ignored;
    auto& nodes = top_list(self, "node");
    const auto pick = [&](const std::string& text) {
        return find_item(nodes, selector_of(text), ignored);
    };
    if (action == "click") {
        const ItemPtr node = pick(rest);
        if (node == nullptr) {
            return true;
        }
        const ItemPtr previous = selected_node(self);
        const bool second_click = previous == node;
        select_node(self, node);
        if (self.values.contains("singlesel") &&
            std::holds_alternative<bool>(self.values.at("singlesel")) &&
            std::get<bool>(self.values.at("singlesel"))) {
            if (previous != nullptr && previous != node) {
                bool ancestor = false;
                for (ItemPtr at = node_parent(self, node); at != nullptr;
                     at = node_parent(self, at)) {
                    ancestor = ancestor || at == previous;
                }
                if (!ancestor && !set_expanded(in, self, previous, false)) {
                    return false;
                }
            }
            if (!set_expanded(in, self, node, true)) {
                return false;
            }
        }
        if (!(raise_event(in, self, "NodeClick", {node_value(self, node)}) &&
              raise_event(in, self, "Click", {}))) {
            return false;
        }
        if (second_click && number_property(self, "labeledit") != 1.0) {
            bool started = false;
            return begin_label_edit(in, self, started);
        }
        return true;
    }
    if (action == "expand" || action == "collapse") {
        const ItemPtr node = pick(rest);
        return node == nullptr || set_expanded(in, self, node, action == "expand");
    }
    if (action == "check") {
        const ItemPtr node = pick(rest);
        const bool boxes = self.values.contains("checkboxes") &&
                           std::holds_alternative<bool>(self.values.at("checkboxes")) &&
                           std::get<bool>(self.values.at("checkboxes"));
        if (node == nullptr || !boxes) {
            return true;
        }
        node->p["checked"] = Value{!flag(node, "checked")};
        return raise_event(in, self, "NodeCheck", {node_value(self, node)});
    }
    if (action == "edit" || action == "canceledit") {
        // edit <text> | canceledit: the end of an edit of the selected node.
        // Automatic editing starts on the second click, manual editing with
        // StartLabelEdit; a scripted edit that was not started starts here.
        const ItemPtr node = selected_node(self);
        if (node == nullptr) {
            return true;
        }
        if (!self.values.contains("_editing")) {
            if (number_property(self, "labeledit") == 1.0) {
                return true;
            }
            bool started = false;
            if (!begin_label_edit(in, self, started)) {
                return false;
            }
            if (!started) {
                return true;
            }
        }
        std::optional<std::string> accepted;
        if (!end_label_edit(in, self,
                            action == "edit" ? std::optional<std::string>(rest)
                                             : std::nullopt,
                            accepted)) {
            return false;
        }
        if (accepted.has_value()) {
            node->p["text"] = Value{*accepted};
        }
        return true;
    }
    if (action == "key") {
        std::vector<std::pair<ItemPtr, int>> rows;
        visible_rows(self, nullptr, 0, rows);
        const ItemPtr current = selected_node(self);
        std::size_t at = rows.size();
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].first == current) {
                at = i;
            }
        }
        const std::string key = lower_text(rest);
        if (rows.empty()) {
            return true;
        }
        if (at == rows.size()) {
            select_node(self, rows.front().first);
            return true;
        }
        if (key == "down") {
            select_node(self, rows[(at + 1U) % rows.size()].first);
        } else if (key == "up") {
            select_node(self, rows[(at + rows.size() - 1U) % rows.size()].first);
        } else if (key.size() == 1U && key != " ") {
            // Type-ahead: the next visible node starting with the character.
            for (std::size_t step = 1; step <= rows.size(); ++step) {
                const auto& candidate = rows[(at + step) % rows.size()].first;
                const std::string text = lower_text(node_text(candidate));
                if (!text.empty() && text.front() == key.front()) {
                    select_node(self, candidate);
                    break;
                }
            }
        } else if (key == "home") {
            select_node(self, rows.front().first);
        } else if (key == "end") {
            select_node(self, rows.back().first);
        } else if (key == "right") {
            if (!current->kids.empty() && !flag(current, "expanded")) {
                return set_expanded(in, self, current, true);
            }
            if (!current->kids.empty()) {
                select_node(self, node_by_id(self, current->kids.front()));
            }
        } else if (key == "left") {
            if (flag(current, "expanded")) {
                return set_expanded(in, self, current, false);
            }
            if (const ItemPtr parent = node_parent(self, current)) {
                select_node(self, parent);
            }
        } else if (key == "space") {
            const bool boxes = self.values.contains("checkboxes") &&
                               std::holds_alternative<bool>(self.values.at("checkboxes")) &&
                               std::get<bool>(self.values.at("checkboxes"));
            if (boxes) {
                current->p["checked"] = Value{!flag(current, "checked")};
                return raise_event(in, self, "NodeCheck", {node_value(self, current)});
            }
        }
        return true;
    }
    return true;
}

// ----- ListView -----------------------------------------------------------------

[[nodiscard]] Value list_item_value(ControlObject& control, const ItemPtr& item) {
    if (item == nullptr) {
        return Value{Nothing{}};
    }
    ListRef ref;
    ref.kind = "listitem";
    return wrap_item(control.self.lock(), ref, item);
}

[[nodiscard]] std::string text_of_prop(const ItemPtr& item, const char* name) {
    const auto found = item->p.find(name);
    return found != item->p.end() &&
                   std::holds_alternative<std::string>(found->second)
               ? std::get<std::string>(found->second)
               : std::string{};
}

// The text a list item sorts by: its own text (SortKey 0) or a sub item's.
[[nodiscard]] std::string sort_text(ControlObject& control, const ItemPtr& item) {
    const long key = static_cast<long>(number_property(control, "sortkey"));
    if (key <= 0) {
        return lower_text(text_of_prop(item, "text"));
    }
    if (static_cast<std::size_t>(key) <= item->sub.size()) {
        return lower_text(text_of_prop(item->sub[static_cast<std::size_t>(key) - 1U], "text"));
    }
    return {};
}

void sort_list_items(ControlObject& control) {
    auto& items = top_list(control, "listitem");
    const bool descending = number_property(control, "sortorder") == 1.0;
    std::stable_sort(items.begin(), items.end(),
                     [&](const ItemPtr& left, const ItemPtr& right) {
                         const std::string a = sort_text(control, left);
                         const std::string b = sort_text(control, right);
                         return descending ? b < a : a < b;
                     });
}

// Positions of the items for the current view (twips).
[[nodiscard]] std::vector<TabRect> list_layout(ControlObject& control) {
    std::vector<TabRect> rects;
    const long view = static_cast<long>(number_property(control, "view"));
    const double width = number_property(control, "width");
    const double height = number_property(control, "height");
    const auto& items = top_list(control, "listitem");
    double header = 0.0;
    double row_width = 0.0;
    if (view == 3) {
        header = control_flag(control, "hidecolumnheaders") ? 0.0 : 240.0;
        for (const auto& column : top_list(control, "columnheader")) {
            row_width += item_number(column, "width");
        }
    }
    const double cell_width = view == 0 ? 1200.0 : 1440.0;
    const double cell_height = view == 0 ? 1100.0 : 240.0;
    const long per_row = std::max<long>(1, static_cast<long>(width / cell_width));
    const long per_column = std::max<long>(1, static_cast<long>((height > 0 ? height : 2400.0) / cell_height));
    for (std::size_t i = 0; i < items.size(); ++i) {
        const long n = static_cast<long>(i);
        if (view == 3) {
            rects.push_back({0.0, header + 240.0 * static_cast<double>(n), row_width, 240.0});
        } else if (view == 2) {
            rects.push_back({cell_width * static_cast<double>(n / per_column),
                             cell_height * static_cast<double>(n % per_column),
                             cell_width, cell_height});
        } else {
            rects.push_back({cell_width * static_cast<double>(n % per_row),
                             cell_height * static_cast<double>(n / per_row),
                             cell_width, cell_height});
        }
    }
    return rects;
}

[[nodiscard]] ItemPtr first_selected_item(ControlObject& control) {
    for (const auto& item : top_list(control, "listitem")) {
        if (flag(item, "selected")) {
            return item;
        }
    }
    return nullptr;
}

void select_list_item(ControlObject& control, const ItemPtr& item, const bool extend) {
    for (const auto& other : top_list(control, "listitem")) {
        if (other == item) {
            other->p["selected"] = Value{extend ? !flag(other, "selected") : true};
        } else if (!extend) {
            other->p["selected"] = Value{false};
        }
    }
}

[[nodiscard]] bool listitem_get(HookCtx& ctx, const std::string& member,
                                std::vector<CallArgument>& arguments,
                                Value& result, NativeFailure& failure) {
    ControlObject& control = ctx.control;
    const ItemPtr& item = ctx.item;
    if (member == "subitems") {
        if (arguments.size() != 1U || !is_number(arguments[0].value)) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const long index = static_cast<long>(as_double(arguments[0].value));
        if (index < 0 || static_cast<std::size_t>(index) > item->sub.size()) {
            failure = NativeFailure{kIndexOutOfBounds, "Index out of bounds"};
            return true;
        }
        result = index == 0 ? item->p.at("text")
                            : item->sub[static_cast<std::size_t>(index) - 1U]->p.at("text");
        arguments.clear();
        return true;
    }
    if (member == "listsubitems") {
        ListRef ref;
        ref.kind = "listsubitem";
        ref.parent = item;
        result = wrap_collection(ctx.ref.owner.lock(), ref);
        return true;
    }
    if (member == "left" || member == "top" || member == "width" || member == "height") {
        const auto rects = list_layout(control);
        const long position = position_of(top_list(control, "listitem"), item);
        TabRect rect;
        if (position >= 1 && static_cast<std::size_t>(position) <= rects.size()) {
            rect = rects[static_cast<std::size_t>(position) - 1U];
        }
        result = Value{static_cast<float>(member == "left" ? rect.left
                                          : member == "top" ? rect.top
                                          : member == "width" ? rect.width
                                                              : rect.height)};
        return true;
    }
    if (member == "ensurevisible") {
        result = Value{true};
        return true;
    }
    if (member == "createdragimage") {
        result = Value{Nothing{}};
        return true;
    }
    return false;
}

[[nodiscard]] bool listitem_put(HookCtx& ctx, const std::string& member,
                                std::vector<CallArgument>& arguments,
                                const Value& value, NativeFailure& failure) {
    ControlObject& control = ctx.control;
    const ItemPtr& item = ctx.item;
    if (member == "subitems") {
        if (arguments.size() != 1U || !is_number(arguments[0].value)) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const long index = static_cast<long>(as_double(arguments[0].value));
        if (index < 0) {
            failure = NativeFailure{kIndexOutOfBounds, "Index out of bounds"};
            return true;
        }
        Value text = value;
        if (!coerce_property(ctx.in, text, "String", failure)) {
            return true;
        }
        if (index == 0) {
            item->p["text"] = text;
            return true;
        }
        while (item->sub.size() < static_cast<std::size_t>(index)) {
            auto created = std::make_shared<CItem>();
            created->id = state_of(control).next_id++;
            for (const auto& property : spec_of("listsubitem").props) {
                created->p[property.name] = initial_value(property);
            }
            item->sub.push_back(created);
        }
        item->sub[static_cast<std::size_t>(index) - 1U]->p["text"] = text;
        return true;
    }
    if (member == "left" || member == "top" || member == "width" || member == "height") {
        failure = NativeFailure{383, "Property is read-only"};
        return true;
    }
    if (member == "selected") {
        const bool on = std::holds_alternative<bool>(value)
                            ? std::get<bool>(value)
                            : is_number(value) && as_double(value) != 0.0;
        if (on) {
            if (!control_flag(control, "multiselect")) {
                select_list_item(control, item, false);
            }
            item->p["selected"] = Value{true};
        } else {
            item->p["selected"] = Value{false};
        }
        return true;
    }
    return false;
}

[[nodiscard]] bool column_get(HookCtx& ctx, const std::string& member,
                              Value& result) {
    const auto& columns = top_list(ctx.control, "columnheader");
    if (member == "left") {
        double x = 0.0;
        for (const auto& column : columns) {
            if (column == ctx.item) {
                break;
            }
            x += item_number(column, "width");
        }
        result = Value{static_cast<float>(x)};
        return true;
    }
    if (member == "position") {
        result = Value{static_cast<Integer>(position_of(columns, ctx.item))};
        return true;
    }
    if (member == "subitemindex") {
        result = Value{static_cast<Int16>(position_of(columns, ctx.item) - 1)};
        return true;
    }
    return false;
}

[[nodiscard]] bool list_member(Interpreter& in, ControlObject& self,
                               const std::string& member,
                               std::vector<CallArgument>& arguments,
                               Value& result, NativeFailure& failure) {
    static_cast<void>(in);
    if (member == "selecteditem") {
        result = list_item_value(self, first_selected_item(self));
        return true;
    }
    if (member == "getfirstvisible") {
        const auto& items = top_list(self, "listitem");
        result = list_item_value(self, items.empty() ? nullptr : items.front());
        return true;
    }
    if (member == "finditem") {
        // FindItem(String, Where, Index, Match)
        if (arguments.empty() || arguments.size() > 4U) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const std::string wanted = lower_text(FormSupport::text_of(arguments[0].value));
        const long where = arguments.size() > 1U && !arguments[1].omitted
                               ? static_cast<long>(as_double(arguments[1].value))
                               : 0;
        const bool partial = arguments.size() > 3U && !arguments[3].omitted &&
                             as_double(arguments[3].value) != 0.0;
        auto& items = top_list(self, "listitem");
        std::size_t start = 0;
        if (arguments.size() > 2U && !arguments[2].omitted) {
            const ItemPtr from = find_item(items, arguments[2].value, failure);
            if (from == nullptr) {
                return true;
            }
            start = static_cast<std::size_t>(position_of(items, from)) - 1U;
        }
        result = Value{Nothing{}};
        for (std::size_t i = start; i < items.size(); ++i) {
            std::vector<std::string> candidates;
            if (where == 0) {
                candidates.push_back(text_of_prop(items[i], "text"));
            } else if (where == 1) {
                for (const auto& sub : items[i]->sub) {
                    candidates.push_back(text_of_prop(sub, "text"));
                }
            } else {
                candidates.push_back(FormSupport::text_of(items[i]->p.at("tag")));
            }
            for (const auto& candidate : candidates) {
                const std::string text = lower_text(candidate);
                if (partial ? text.rfind(wanted, 0) == 0 : text == wanted) {
                    result = list_item_value(self, items[i]);
                    return true;
                }
            }
        }
        return true;
    }
    if (member == "hittest") {
        if (arguments.size() != 2U) {
            failure = NativeFailure{450, "Wrong number of arguments"};
            return true;
        }
        const double x = as_double(arguments[0].value);
        const double y = as_double(arguments[1].value);
        const auto rects = list_layout(self);
        const auto& items = top_list(self, "listitem");
        result = Value{Nothing{}};
        for (std::size_t i = 0; i < rects.size() && i < items.size(); ++i) {
            const TabRect& rect = rects[i];
            if (x >= rect.left && x < rect.left + rect.width && y >= rect.top &&
                y < rect.top + rect.height) {
                result = list_item_value(self, items[i]);
                break;
            }
        }
        return true;
    }
    if (member == "startlabeledit") {
        bool started = false;
        if (!begin_label_edit(in, self, started)) {
            failure = FormSupport::reported();
        }
        result = Value{Empty{}};
        return true;
    }
    return false;
}

[[nodiscard]] bool list_put(Interpreter& in, ControlObject& self,
                            const std::string& member, const Value& value,
                            NativeFailure& failure) {
    if (member == "selecteditem") {
        const auto* holder = std::get_if<ObjectInstance>(&value);
        const auto* object =
            holder != nullptr && holder->data != nullptr
                ? dynamic_cast<ComctlItem*>(holder->data->native.get())
                : nullptr;
        if (object == nullptr || object->ref.kind != "listitem" ||
            position_of(top_list(self, "listitem"), object->item) == 0) {
            if (std::holds_alternative<Nothing>(value)) {
                for (const auto& item : top_list(self, "listitem")) {
                    item->p["selected"] = Value{false};
                }
                return true;
            }
            failure = NativeFailure{13, "Type mismatch"};
            return true;
        }
        select_list_item(self, object->item, false);
        return true;
    }
    if (member == "view") {
        Value converted = value;
        if (!coerce_property(in, converted, "Integer", failure)) {
            return true;
        }
        const double number = as_double(converted);
        if (number < 0 || number > 3) {
            failure = NativeFailure{380, "Invalid property value"};
            return true;
        }
        self.values["view"] = converted;
        return true;
    }
    if (member == "sorted" || member == "sortkey" || member == "sortorder") {
        Value converted = value;
        if (!coerce_property(in, converted, member == "sorted" ? "Boolean" : "Integer",
                             failure)) {
            return true;
        }
        self.values[member] = member == "sorted" ? converted
                                                 : normalized(converted, "XConstants");
        if (control_flag(self, "sorted")) {
            sort_list_items(self);
        }
        return true;
    }
    return false;
}

[[nodiscard]] bool list_input(Interpreter& in, ControlObject& self,
                              const std::string& action, const std::string& rest) {
    NativeFailure ignored;
    auto& items = top_list(self, "listitem");
    const auto pick = [&](const std::string& text) {
        return find_item(items, selector_of(text), ignored);
    };
    if (action == "click" || action == "dblclick") {
        std::istringstream words(rest);
        std::string name;
        std::string modifier;
        words >> name >> modifier;
        const ItemPtr item = pick(name);
        if (item == nullptr) {
            return true;
        }
        const bool second_click = flag(item, "selected") && modifier != "ctrl";
        select_list_item(self, item, modifier == "ctrl" && control_flag(self, "multiselect"));
        bool ok = raise_event(in, self, "ItemClick", {list_item_value(self, item)}) &&
                  raise_event(in, self, "Click", {});
        if (ok && action == "dblclick") {
            ok = raise_event(in, self, "DblClick", {});
        }
        if (ok && action == "click" && second_click &&
            number_property(self, "labeledit") != 1.0) {
            bool started = false;
            ok = begin_label_edit(in, self, started);
        }
        return ok;
    }
    if (action == "check") {
        const ItemPtr item = pick(rest);
        if (item == nullptr || !control_flag(self, "checkboxes")) {
            return true;
        }
        item->p["checked"] = Value{!flag(item, "checked")};
        return raise_event(in, self, "ItemCheck", {list_item_value(self, item)});
    }
    if (action == "columnclick") {
        const ItemPtr column =
            find_item(top_list(self, "columnheader"), selector_of(rest), ignored);
        if (column == nullptr) {
            return true;
        }
        ListRef ref;
        ref.kind = "columnheader";
        return raise_event(in, self, "ColumnClick",
                           {wrap_item(self.self.lock(), ref, column)});
    }
    if (action == "edit" || action == "canceledit") {
        const ItemPtr item = first_selected_item(self);
        if (item == nullptr) {
            return true;
        }
        if (!self.values.contains("_editing")) {
            if (number_property(self, "labeledit") == 1.0) {
                return true;
            }
            bool started = false;
            if (!begin_label_edit(in, self, started)) {
                return false;
            }
            if (!started) {
                return true;
            }
        }
        std::optional<std::string> accepted;
        if (!end_label_edit(in, self,
                            action == "edit" ? std::optional<std::string>(rest)
                                             : std::nullopt,
                            accepted)) {
            return false;
        }
        if (accepted.has_value()) {
            item->p["text"] = Value{*accepted};
        }
        return true;
    }
    return true;
}

// ----- ImageCombo ---------------------------------------------------------------

[[nodiscard]] Value combo_item_value(ControlObject& control, const ItemPtr& item) {
    if (item == nullptr) {
        return Value{Nothing{}};
    }
    ListRef ref;
    ref.kind = "comboitem";
    return wrap_item(control.self.lock(), ref, item);
}

[[nodiscard]] ItemPtr selected_combo_item(ControlObject& control) {
    for (const auto& item : top_list(control, "comboitem")) {
        if (flag(item, "selected")) {
            return item;
        }
    }
    return nullptr;
}

[[nodiscard]] std::string combo_text(ControlObject& control) {
    const auto found = control.values.find("text");
    return found != control.values.end() &&
                   std::holds_alternative<std::string>(found->second)
               ? std::get<std::string>(found->second)
               : std::string{};
}

// Selects `item` (or nothing): the text follows; with `notify` Click and
// Change run.
[[nodiscard]] bool select_combo_item(Interpreter& in, ControlObject& control,
                                     const ItemPtr& item, const bool notify) {
    for (const auto& other : top_list(control, "comboitem")) {
        other->p["selected"] = Value{other == item};
    }
    if (item != nullptr) {
        control.values["text"] = item->p.at("text");
    }
    if (!notify) {
        return true;
    }
    return raise_event(in, control, "Click", {}) &&
           raise_event(in, control, "Change", {});
}

[[nodiscard]] bool combo_member(ControlObject& self, const std::string& member,
                                Value& result) {
    if (member == "selecteditem") {
        result = combo_item_value(self, selected_combo_item(self));
        return true;
    }
    if (member == "getfirstvisible") {
        const auto& items = top_list(self, "comboitem");
        result = combo_item_value(self, items.empty() ? nullptr : items.front());
        return true;
    }
    if (member == "text") {
        result = Value{combo_text(self)};
        return true;
    }
    if (member == "seltext") {
        const std::string text = combo_text(self);
        const std::size_t start = std::min<std::size_t>(
            static_cast<std::size_t>(std::max(0.0, number_property(self, "selstart"))),
            text.size());
        const std::size_t length = static_cast<std::size_t>(
            std::max(0.0, number_property(self, "sellength")));
        result = Value{text.substr(start, length)};
        return true;
    }
    return false;
}

[[nodiscard]] bool combo_put(Interpreter& in, ControlObject& self,
                             const std::string& member, const Value& value,
                             NativeFailure& failure) {
    if (member == "selecteditem") {
        const auto* holder = std::get_if<ObjectInstance>(&value);
        const auto* object =
            holder != nullptr && holder->data != nullptr
                ? dynamic_cast<ComctlItem*>(holder->data->native.get())
                : nullptr;
        if (object == nullptr || object->ref.kind != "comboitem" ||
            position_of(top_list(self, "comboitem"), object->item) == 0) {
            if (std::holds_alternative<Nothing>(value)) {
                return select_combo_item(in, self, nullptr, false);
            }
            failure = NativeFailure{13, "Type mismatch"};
            return true;
        }
        if (!select_combo_item(in, self, object->item, true)) {
            failure = FormSupport::reported();
        }
        return true;
    }
    if (member == "text") {
        Value converted = value;
        if (!coerce_property(in, converted, "String", failure)) {
            return true;
        }
        const bool changed = combo_text(self) != std::get<std::string>(converted);
        self.values["text"] = converted;
        // The selection follows the text when an item has exactly that text.
        ItemPtr match;
        for (const auto& item : top_list(self, "comboitem")) {
            if (lower_text(text_of_prop(item, "text")) ==
                lower_text(std::get<std::string>(converted))) {
                match = item;
                break;
            }
        }
        for (const auto& item : top_list(self, "comboitem")) {
            item->p["selected"] = Value{item == match};
        }
        if (changed && !raise_event(in, self, "Change", {})) {
            failure = FormSupport::reported();
        }
        return true;
    }
    if (member == "seltext") {
        Value converted = value;
        if (!coerce_property(in, converted, "String", failure)) {
            return true;
        }
        const std::string text = combo_text(self);
        const std::size_t start = std::min<std::size_t>(
            static_cast<std::size_t>(std::max(0.0, number_property(self, "selstart"))),
            text.size());
        const std::size_t length = std::min<std::size_t>(
            static_cast<std::size_t>(std::max(0.0, number_property(self, "sellength"))),
            text.size() - start);
        const std::string inserted = std::get<std::string>(converted);
        self.values["text"] = Value{text.substr(0, start) + inserted +
                                    text.substr(start + length)};
        self.values["sellength"] = Value{static_cast<Integer>(inserted.size())};
        if (!raise_event(in, self, "Change", {})) {
            failure = FormSupport::reported();
        }
        return true;
    }
    return false;
}

[[nodiscard]] bool combo_input(Interpreter& in, ControlObject& self,
                               const std::string& action, const std::string& rest) {
    NativeFailure ignored;
    if (action == "select") {
        const ItemPtr item =
            find_item(top_list(self, "comboitem"), selector_of(rest), ignored);
        return item == nullptr || select_combo_item(in, self, item, true);
    }
    if (action == "dropdown") {
        return raise_event(in, self, "Dropdown", {});
    }
    if (action == "type") {
        // The user types: ignored while the control is locked.
        if (control_flag(self, "locked")) {
            return true;
        }
        NativeFailure failure;
        static_cast<void>(combo_put(in, self, "text", Value{rest}, failure));
        return failure.number != FormSupport::reported().number;
    }
    return true;
}

// @@KINDS_MORE

}  // namespace

// ----- Hook dispatch ------------------------------------------------------------------

bool ComctlHooks::node_add(Interpreter& in, ControlObject& control,
                           std::vector<CallArgument>& arguments, Value& result,
                           NativeFailure& failure) {
    return add_node(in, control, arguments, result, failure);
}

void ComctlHooks::changed(Interpreter& in, ControlObject& control) {
    comctl_push(in, control);
    if (control.class_key == "imagelist") {
        // Controls that draw from this list show the new images.
        if (const auto form = control.form.lock()) {
            std::vector<ControlObject*> users;
            FormSupport::for_each_control(*form, [&](ControlObject& other) {
                if (other.comctl != nullptr && &other != &control) {
                    users.push_back(&other);
                }
            });
            for (ControlObject* other : users) {
                comctl_push(in, *other);
            }
        }
    }
}

bool ComctlHooks::before_remove(Interpreter& in, ControlObject& control,
                                const std::string& kind, const ItemPtr& item) {
    static_cast<void>(in);
    if (kind == "node") {
        detach_node(control, item);
        return true;
    }
    if (kind == "comboitem") {
        item->p["selected"] = Value{false};
        return true;
    }
    if (kind == "tab" && flag(item, "selected")) {
        // The selection moves to a neighbor (no events).
        auto& tabs = top_list(control, "tab");
        const long position = position_of(tabs, item);
        ItemPtr neighbor;
        if (position > 1) {
            neighbor = tabs[static_cast<std::size_t>(position) - 2U];
        } else if (tabs.size() > 1U) {
            neighbor = tabs[1];
        }
        item->p["selected"] = Value{false};
        if (neighbor != nullptr) {
            neighbor->p["selected"] = Value{true};
        }
    }
    return true;
}

bool ComctlHooks::after_add(Interpreter& in, ControlObject& control,
                            const std::string& kind, const ItemPtr& item) {
    static_cast<void>(in);
    if (kind == "tab" && top_list(control, "tab").size() == 1U) {
        item->p["selected"] = Value{true};  // the first tab is selected
    }
    if (kind == "listitem" && control_flag(control, "sorted")) {
        sort_list_items(control);
    }
    if (kind == "listimage" && control.class_key == "imagelist") {
        // The first image sets the size of the list (pixels at 96 dpi).
        if (top_list(control, kind).size() == 1U) {
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
    static_cast<void>(arguments);
    static_cast<void>(failure);
    HookCtx ctx{in, control, ref, item};
    if (kind == "node" && member != "ensurevisible" && !arguments.empty()) {
        return false;
    }
    if (kind == "tab") {
        return tab_get(ctx, member, result);
    }
    if (kind == "panel") {
        return panel_get(ctx, member, result);
    }
    if (kind == "button") {
        return button_get(ctx, member, result);
    }
    if (kind == "node") {
        return node_get(ctx, member, result);
    }
    if (kind == "listimage") {
        if (member == "draw") {
            result = Value{Empty{}};  // drawing needs a device context
            return true;
        }
        if (member == "extracticon") {
            result = item->p.at("picture");
            return true;
        }
    }
    if (kind == "listitem") {
        return listitem_get(ctx, member, arguments, result, failure);
    }
    if (kind == "columnheader") {
        return column_get(ctx, member, result);
    }
    return false;
}

bool ComctlHooks::item_put(Interpreter& in, ControlObject& control,
                           const std::string& kind, const ItemPtr& item,
                           const ListRef& ref, const std::string& member,
                           std::vector<CallArgument>& arguments,
                           const Value& value, NativeFailure& failure) {
    if (kind == "node") {
        HookCtx ctx{in, control, ref, item};
        return node_put(ctx, member, value, failure);
    }
    if (kind == "listitem") {
        HookCtx ctx{in, control, ref, item};
        return listitem_put(ctx, member, arguments, value, failure);
    }
    if (kind == "comboitem" && member == "selected") {
        const bool on = std::holds_alternative<bool>(value)
                            ? std::get<bool>(value)
                            : is_number(value) && as_double(value) != 0.0;
        if (on && !select_combo_item(in, control, item, true)) {
            failure = FormSupport::reported();
        } else if (!on) {
            item->p["selected"] = Value{false};
        }
        return true;
    }
    if (kind == "columnheader" &&
        (member == "left" || member == "subitemindex")) {
        failure = NativeFailure{383, "Property is read-only"};
        return true;
    }
    if (kind == "tab") {
        if (member == "left" || member == "top" || member == "width" ||
            member == "height") {
            failure = NativeFailure{383, "Property is read-only"};
            return true;
        }
        if (member == "selected") {
            const bool on = std::holds_alternative<bool>(value)
                                ? std::get<bool>(value)
                                : is_number(value) && as_double(value) != 0.0;
            if (on) {
                static_cast<void>(select_tab(in, control, item, true, failure));
            }
            return true;
        }
    }
    if (kind == "panel" && member == "left") {
        failure = NativeFailure{383, "Property is read-only"};
        return true;
    }
    if (kind == "button") {
        if (member == "left" || member == "top" || member == "height") {
            failure = NativeFailure{383, "Property is read-only"};
            return true;
        }
        if (member == "style" || member == "value") {
            Value converted = value;
            if (!coerce_property(in, converted, "Integer", failure)) {
                return true;
            }
            const double number = as_double(converted);
            if (number < 0 || number > (member == "style" ? 5 : 1)) {
                failure = NativeFailure{380, "Invalid property value"};
                return true;
            }
            item->p[member] = converted;
            if (member == "value" && number == 1.0 &&
                static_cast<int>(item_number(item, "style")) == kStyleGroup) {
                press_group(control, item);
            }
            return true;
        }
    }
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

// Control-level members (called before the generic table path).
bool comctl_control_get(Interpreter& in, ControlObject& self,
                        const std::string& member,
                        std::vector<CallArgument>& arguments, Value& result,
                        NativeFailure& failure) {
    if (ocx_get(in, self, member, arguments, result, failure)) {
        return true;
    }
    if (self.class_key == "toolbar" &&
        toolbar_member(self, member, arguments, result, failure)) {
        return true;
    }
    if (self.class_key == "treeview" &&
        tree_member(in, self, member, arguments, result, failure)) {
        return true;
    }
    if (self.class_key == "listview" &&
        list_member(in, self, member, arguments, result, failure)) {
        return true;
    }
    if (self.class_key == "imagecombo" && combo_member(self, member, result)) {
        return true;
    }
    if (self.class_key == "tabstrip") {
        if (member == "selecteditem") {
            const ItemPtr tab = selected_tab(self);
            if (tab == nullptr) {
                result = Value{Nothing{}};
            } else {
                ListRef ref;
                ref.kind = "tab";
                result = wrap_item(self.self.lock(), ref, tab);
            }
            return true;
        }
        const double width = number_property(self, "width");
        const double height = number_property(self, "height");
        const auto rects = tab_layout(self);
        double rows_height = 0.0;
        for (const auto& rect : rects) {
            rows_height = std::max(rows_height, rect.top + rect.height);
        }
        if (rows_height == 0.0) {
            rows_height = 300.0;
        }
        if (member == "clientleft") {
            result = Value{60.0F};
            return true;
        }
        if (member == "clienttop") {
            result = Value{static_cast<float>(rows_height + 30.0)};
            return true;
        }
        if (member == "clientwidth") {
            result = Value{static_cast<float>(std::max(0.0, width - 120.0))};
            return true;
        }
        if (member == "clientheight") {
            result = Value{static_cast<float>(
                std::max(0.0, height - rows_height - 90.0))};
            return true;
        }
        if (member == "hittest") {
            if (arguments.size() != 2U) {
                failure = NativeFailure{450, "Wrong number of arguments"};
                return true;
            }
            const double x = as_double(arguments[0].value);
            const double y = as_double(arguments[1].value);
            result = Value{Nothing{}};
            const auto& tabs = top_list(self, "tab");
            for (std::size_t i = 0; i < rects.size() && i < tabs.size(); ++i) {
                const TabRect& rect = rects[i];
                if (rect.width > 0.0 && x >= rect.left && x < rect.left + rect.width &&
                    y >= rect.top && y < rect.top + rect.height) {
                    ListRef ref;
                    ref.kind = "tab";
                    result = wrap_item(self.self.lock(), ref, tabs[i]);
                    break;
                }
            }
            return true;
        }
        if (member == "deselectall") {
            for (const auto& tab : top_list(self, "tab")) {
                tab->p["selected"] = Value{false};
            }
            result = Value{Empty{}};
            return true;
        }
    }
    return false;
}

bool comctl_control_put(Interpreter& in, ControlObject& self,
                        const std::string& member, const Value& value,
                        NativeFailure& failure) {
    if (ocx_put(in, self, member, value, failure)) {
        return true;
    }
    if (self.class_key == "listview" && list_put(in, self, member, value, failure)) {
        return true;
    }
    if (self.class_key == "imagecombo" &&
        combo_put(in, self, member, value, failure)) {
        return true;
    }
    if (self.class_key == "treeview") {
        if (member == "selecteditem" || member == "drophighlight") {
            const auto* holder = std::get_if<ObjectInstance>(&value);
            const auto* object =
                holder != nullptr && holder->data != nullptr
                    ? dynamic_cast<ComctlItem*>(holder->data->native.get())
                    : nullptr;
            if (object == nullptr || object->ref.kind != "node" ||
                position_of(top_list(self, "node"), object->item) == 0) {
                if (std::holds_alternative<Nothing>(value)) {
                    if (member == "selecteditem") {
                        for (const auto& node : top_list(self, "node")) {
                            node->p["selected"] = Value{false};
                        }
                    } else {
                        self.values.erase("drophighlight");
                    }
                    return true;
                }
                failure = NativeFailure{13, "Type mismatch"};
                return true;
            }
            if (member == "selecteditem") {
                select_node(self, object->item);
            } else {
                self.values["drophighlight"] = value;
            }
            return true;
        }
        if (member == "sorted") {
            Value converted = value;
            if (!coerce_property(in, converted, "Boolean", failure)) {
                return true;
            }
            self.values["sorted"] = converted;
            if (std::get<bool>(converted)) {
                sort_kids(self, state_of(self).root_kids);
                for (const auto& node : top_list(self, "node")) {
                    sort_kids(self, node->kids);
                }
            }
            return true;
        }
    }
    if (self.class_key == "tabstrip" && member == "selecteditem") {
        const auto* holder = std::get_if<ObjectInstance>(&value);
        const auto* item =
            holder != nullptr && holder->data != nullptr
                ? dynamic_cast<ComctlItem*>(holder->data->native.get())
                : nullptr;
        if (item == nullptr || item->ref.kind != "tab" ||
            position_of(top_list(self, "tab"), item->item) == 0) {
            failure = NativeFailure{13, "Type mismatch"};
            return true;
        }
        static_cast<void>(select_tab(in, self, item->item, true, failure));
        return true;
    }
    if (self.class_key == "tabstrip" &&
        (member == "clientleft" || member == "clienttop" ||
         member == "clientwidth" || member == "clientheight")) {
        failure = NativeFailure{383, "Property is read-only"};
        return true;
    }
    return false;
}

bool ComctlSupport::user_input(Interpreter& in, InstanceData& form,
                               ControlObject& self, const std::string& action,
                               const std::string& rest) {
    static_cast<void>(form);
    NativeFailure failure;
    const auto select_by = [&](const std::string& kind) -> ItemPtr {
        const auto& items = top_list(self, kind);
        Value selector{rest};
        char* end = nullptr;
        const double number = std::strtod(rest.c_str(), &end);
        if (end != rest.c_str() && *end == '\0') {
            selector = Value{number};
        }
        NativeFailure ignored;
        return find_item(items, selector, ignored);
    };
    if (self.class_key == "slider" && action == "value") {
        // The user drags the thumb: Scroll while moving, then Change.
        const double wanted = std::atof(rest.c_str());
        const double minimum = number_property(self, "min");
        const double maximum = number_property(self, "max");
        const double target = std::min(std::max(wanted, minimum), maximum);
        if (target != number_property(self, "value")) {
            self.values["value"] = Value{static_cast<Integer>(target)};
            return raise_event(in, self, "Scroll", {}) &&
                   raise_event(in, self, "Click", {}) &&
                   raise_event(in, self, "Change", {});
        }
        return true;
    }
    if (self.class_key == "tabstrip" && action == "select") {
        if (const ItemPtr tab = select_by("tab")) {
            return select_tab(in, self, tab, true, failure);
        }
        return true;
    }
    if (self.class_key == "toolbar") {
        return toolbar_input(in, self, action, rest);
    }
    if (self.class_key == "treeview") {
        return tree_input(in, self, action, rest);
    }
    if (self.class_key == "listview") {
        return list_input(in, self, action, rest);
    }
    if (self.class_key == "imagecombo") {
        return combo_input(in, self, action, rest);
    }
    if (self.class_key == "statusbar" &&
        (action == "panel" || action == "paneldbl")) {
        if (const ItemPtr panel = select_by("panel")) {
            ListRef ref;
            ref.kind = "panel";
            const Value wrapped = wrap_item(self.self.lock(), ref, panel);
            bool ok = raise_event(in, self, "PanelClick", {wrapped});
            if (ok && action == "paneldbl") {
                ok = raise_event(in, self, "PanelDblClick", {wrapped});
            }
            return ok;
        }
        return true;
    }
    return true;
}

// ----- Persisted state of the controls (the BeginProperty groups of a .frm) ----------

namespace {

[[nodiscard]] std::string group_word(const std::string& name) {
    std::string word;
    for (const char c : name) {
        if (c == ' ' || c == '\t' || c == '{' || c == '(') {
            break;
        }
        word.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return word;
}

// The value of one persisted property for a property of `spec`.
[[nodiscard]] Value persisted_value(const PropSpec& spec,
                                    const ui::FrmProperty& source,
                                    const std::string& frx_directory) {
    const std::string type = spec.type;
    if (source.in_resource) {
        std::string bytes;
        const std::string path =
            (frx_directory.empty() ? std::string{} : frx_directory + "/") +
            source.resource_file;
        PictureInfo info;
        if (ui::read_frx_record(path, source.resource_offset, bytes) &&
            read_picture_header(bytes, info)) {
            return make_picture_value(bytes, info);
        }
        return Value{Empty{}};
    }
    if (type == "String") {
        return Value{source.value};
    }
    const double number = std::strtod(source.value.c_str(), nullptr);
    if (type == "Boolean") {
        return Value{number != 0.0};
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
    // Variant: a number when it reads as one, else the text.
    char* end = nullptr;
    std::strtod(source.value.c_str(), &end);
    if (!source.is_string && end != source.value.c_str() && *end == 0) {
        return Value{static_cast<Integer>(number)};
    }
    return Value{source.value};
}

// Reads the entries of `group` into a new item of `kind` appended to `items`.
[[nodiscard]] ItemPtr persisted_item(ControlObject& control, const std::string& kind,
                                     const ui::FrmProperty& group,
                                     const std::string& frx_directory) {
    const KindSpec& spec = spec_of(kind);
    auto item = std::make_shared<CItem>();
    item->id = state_of(control).next_id++;
    for (const auto& property : spec.props) {
        item->p[property.name] = initial_value(property);
    }
    for (const auto& entry : group.nested) {
        if (!entry.nested.empty()) {
            continue;
        }
        std::string name = lower_text(entry.name);
        if (name.rfind("object.", 0) == 0) {
            name = name.substr(7);
        }
        if (name == "imageindex") {
            name = kind == "tab" ? "image" : "image";
        }
        if (name == "picture" && kind == "listimage") {
            name = "picture";
        }
        if (const PropSpec* property = find_property(spec, name)) {
            item->p[name] = persisted_value(*property, entry, frx_directory);
        }
    }
    return item;
}

}  // namespace

void comctl_load_persisted(Interpreter& in, ControlObject& control,
                           const ui::FrmObject& source,
                           const std::string& frx_directory) {
    if (control.comctl == nullptr) {
        return;
    }
    if (source.find("OleObjectBlob") != nullptr) {
        FormSupport::warn(
            in, std::string(kUnreadableStateCode) +
                    ": the persisted state of control '" + control.display_name +
                    "' (OleObjectBlob) is not read; its collections start empty");
    }
    static const std::map<std::string, std::pair<const char*, const char*>> groups = {
        {"tabs", {"tab", "tab"}},
        {"panels", {"panel", "panel"}},
        {"buttons", {"button", "button"}},
        {"images", {"listimage", "listimage"}},
        {"columnheaders", {"columnheader", "columnheader"}},
        {"comboitems", {"comboitem", "comboitem"}},
    };
    for (const auto& property : source.properties) {
        if (property.nested.empty()) {
            continue;
        }
        const std::string word = group_word(property.name);
        const auto known = groups.find(word);
        if (known != groups.end()) {
            const std::string kind = known->second.first;
            auto& items = top_list(control, kind);
            for (const auto& entry : property.nested) {
                if (entry.nested.empty()) {
                    continue;  // NumTabs and friends
                }
                const ItemPtr item = persisted_item(control, kind, entry, frx_directory);
                if (kind == "button") {
                    for (const auto& menus : entry.nested) {
                        if (group_word(menus.name) != "buttonmenus") {
                            continue;
                        }
                        for (const auto& menu : menus.nested) {
                            if (!menu.nested.empty()) {
                                item->sub.push_back(persisted_item(
                                    control, "buttonmenu", menu, frx_directory));
                            }
                        }
                    }
                }
                items.push_back(item);
            }
            if (kind == "tab" && !items.empty()) {
                items.front()->p["selected"] = Value{true};
            }
            if (kind == "listimage" && !items.empty()) {
                // The first image sets the size of the list.
                static_cast<void>(
                    ComctlHooks::after_add(in, control, kind, items.front()));
            }
            continue;
        }
        if (word == "columnheader") {  // `ColumnHeader(1)` groups of a ListView
            top_list(control, "columnheader")
                .push_back(persisted_item(control, "columnheader", property,
                                          frx_directory));
        }
    }
}

// ----- Snapshots for the host ---------------------------------------------------------

namespace {

[[nodiscard]] long long_of(const Value& value) {
    return is_number(value) ? static_cast<long>(std::nearbyint(as_double(value))) : 0;
}

[[nodiscard]] ui::CommonItem common_item(const ItemPtr& item, const char* text_name) {
    ui::CommonItem result;
    result.text = text_of_prop(item, text_name);
    result.key = text_of_prop(item, "key");
    result.selected = flag(item, "selected");
    result.checked = flag(item, "checked");
    result.bold = flag(item, "bold");
    result.expanded = flag(item, "expanded");
    result.visible = !item->p.contains("visible") || flag(item, "visible");
    result.enabled = !item->p.contains("enabled") || flag(item, "enabled");
    if (const auto found = item->p.find("image"); found != item->p.end()) {
        result.image = long_of(found->second) - 1;
    }
    if (const auto found = item->p.find("style"); found != item->p.end()) {
        result.style = long_of(found->second);
    }
    if (const auto found = item->p.find("value"); found != item->p.end()) {
        result.value = long_of(found->second);
    }
    if (const auto found = item->p.find("width"); found != item->p.end()) {
        result.width = long_of(found->second);
    }
    if (const auto found = item->p.find("alignment"); found != item->p.end()) {
        result.alignment = long_of(found->second);
    }
    return result;
}

}  // namespace

// The ImageList control a property of `control` refers to, or nullptr.
[[nodiscard]] ControlObject* linked_image_list(ControlObject& control,
                                               const char* property) {
    const auto found = control.values.find(property);
    if (found == control.values.end()) {
        return nullptr;
    }
    ControlObject* list = control_of(found->second);
    if (list == nullptr) {
        // A form file names the list: `ImageList = "ImageList1"`.
        if (const auto* name = std::get_if<std::string>(&found->second)) {
            if (const auto form = control.form.lock()) {
                list = FormSupport::control_named(*form, lower_text(*name), -1);
            }
        }
    }
    return list != nullptr && list->class_key == "imagelist" ? list : nullptr;
}

// The 0-based index of an image reference (a 1-based index or a key) in
// `list`, or -1.
[[nodiscard]] long image_index(ControlObject* list, const Value& reference) {
    if (list == nullptr || std::holds_alternative<Empty>(reference)) {
        return -1;
    }
    auto& images = state_of(*list).lists["listimages"];
    if (const auto* key = std::get_if<std::string>(&reference)) {
        for (std::size_t i = 0; i < images.size(); ++i) {
            if (same_key(images[i]->p.at("key"), *key)) {
                return static_cast<long>(i);
            }
        }
        return -1;
    }
    const long number = long_of(reference);
    return number >= 1 && static_cast<std::size_t>(number) <= images.size()
               ? number - 1
               : -1;
}

void fill_images(Interpreter& in, ControlObject* list,
                 std::vector<long>& ids, long& width, long& height) {
    if (list == nullptr) {
        return;
    }
    width = static_cast<long>(number_property(*list, "imagewidth"));
    height = static_cast<long>(number_property(*list, "imageheight"));
    for (const auto& image : state_of(*list).lists["listimages"]) {
        PictureObject* picture = picture_of(image->p.at("picture"));
        ids.push_back(picture != nullptr ? FormSupport::host_picture(in, *picture) : 0);
    }
}

ui::CommonSnapshot comctl_snapshot(Interpreter& in, ControlObject& control) {
    ui::CommonSnapshot snapshot;
    snapshot.kind = control.class_key;
    snapshot.min = static_cast<long>(number_property(control, "min"));
    snapshot.max = static_cast<long>(number_property(control, "max"));
    snapshot.value = static_cast<long>(number_property(control, "value"));
    snapshot.view = static_cast<long>(number_property(control, "view"));
    snapshot.style = static_cast<long>(number_property(control, "style"));
    snapshot.vertical = number_property(control, "orientation") == 1.0;
    snapshot.smooth = number_property(control, "scrolling") == 1.0;
    snapshot.checkboxes = control_flag(control, "checkboxes");
    // Grid lines and full-row selection only apply in report view.
    snapshot.gridlines = control_flag(control, "gridlines") &&
                         (control.class_key != "listview" || snapshot.view == 3);
    snapshot.full_row = control_flag(control, "fullrowselect") &&
                        (control.class_key != "listview" || snapshot.view == 3);
    snapshot.hide_headers = control_flag(control, "hidecolumnheaders");
    snapshot.large_change = static_cast<long>(number_property(control, "largechange"));
    snapshot.small_change = static_cast<long>(number_property(control, "smallchange"));
    snapshot.tick_frequency = static_cast<long>(number_property(control, "tickfrequency"));
    snapshot.tick_style = static_cast<long>(number_property(control, "tickstyle"));
    snapshot.sel_start = static_cast<long>(number_property(control, "selstart"));
    snapshot.sel_length = static_cast<long>(number_property(control, "sellength"));
    snapshot.select_range = control_flag(control, "selectrange");
    snapshot.locked = control_flag(control, "locked");
    const auto text = control.values.find("text");
    if (text != control.values.end() && std::holds_alternative<std::string>(text->second)) {
        snapshot.text = std::get<std::string>(text->second);
    }
    const auto simple = control.values.find("simpletext");
    if (simple != control.values.end() &&
        std::holds_alternative<std::string>(simple->second)) {
        snapshot.simple_text = std::get<std::string>(simple->second);
    }
    const auto& lists = state_of(control).lists;
    const auto list = [&](const char* name) -> const std::vector<ItemPtr>& {
        static const std::vector<ItemPtr> none;
        const auto found = lists.find(name);
        return found == lists.end() ? none : found->second;
    };
    const std::string& cls = control.class_key;
    ControlObject* images = linked_image_list(control, cls == "listview" ? "icons" : "imagelist");
    ControlObject* small_images = linked_image_list(control, "smallicons");
    fill_images(in, images, snapshot.images, snapshot.image_width, snapshot.image_height);
    fill_images(in, small_images, snapshot.small_images, snapshot.small_width,
                snapshot.small_height);
    if (cls == "tabstrip") {
        long at = 0;
        for (const auto& tab : list("tabs")) {
            snapshot.items.push_back(common_item(tab, "caption"));
            snapshot.items.back().image = image_index(images, tab->p.at("image"));
            if (flag(tab, "selected")) {
                snapshot.selected = at;
            }
            ++at;
        }
    } else if (cls == "statusbar") {
        for (const auto& panel : list("panels")) {
            snapshot.items.push_back(common_item(panel, "text"));
        }
    } else if (cls == "toolbar") {
        for (const auto& button : list("buttons")) {
            ui::CommonItem item = common_item(button, "caption");
            item.image = image_index(images, button->p.at("image"));
            for (const auto& menu : button->sub) {
                item.cells.push_back(text_of_prop(menu, "text"));
            }
            snapshot.items.push_back(std::move(item));
        }
    } else if (cls == "treeview") {
        // Pre-order, parents before children.
        std::map<long, long> index_of;
        const auto walk = [&](const auto& self, const std::vector<long>& kids,
                              const long parent) -> void {
            for (const long id : kids) {
                const ItemPtr node = node_by_id(control, id);
                if (node == nullptr) {
                    continue;
                }
                ui::CommonItem item = common_item(node, "text");
                item.image = image_index(images, node->p.at("image"));
                item.image2 = image_index(images, node->p.at("selectedimage"));
                item.parent = parent;
                item.index = position_of(top_list(control, "node"), node);
                const long here = static_cast<long>(snapshot.items.size());
                index_of[id] = here;
                if (flag(node, "selected")) {
                    snapshot.selected = here;
                }
                snapshot.items.push_back(std::move(item));
                self(self, node->kids, here);
            }
        };
        walk(walk, state_of(control).root_kids, -1);
    } else if (cls == "listview") {
        for (const auto& column : list("columnheaders")) {
            snapshot.columns.push_back(common_item(column, "text"));
        }
        long at = 0;
        for (const auto& entry : list("listitems")) {
            ui::CommonItem item = common_item(entry, "text");
            item.image = image_index(images, entry->p.at("icon"));
            item.image2 = image_index(small_images, entry->p.at("smallicon"));
            for (const auto& sub : entry->sub) {
                item.cells.push_back(text_of_prop(sub, "text"));
            }
            item.index = at + 1;
            if (flag(entry, "selected") && snapshot.selected < 0) {
                snapshot.selected = at;
            }
            snapshot.items.push_back(std::move(item));
            ++at;
        }
    } else if (cls == "imagecombo") {
        long at = 0;
        for (const auto& entry : list("comboitems")) {
            ui::CommonItem item = common_item(entry, "text");
            item.image = image_index(images, entry->p.at("image"));
            item.image2 = image_index(images, entry->p.at("selimage"));
            item.indent = long_of(entry->p.at("indentation"));
            snapshot.items.push_back(std::move(item));
            if (flag(entry, "selected")) {
                snapshot.selected = at;
            }
            ++at;
        }
    }
    return snapshot;
}

// Pushes the control's state to the host (a no-op without a window).
void comctl_push(Interpreter& in, ControlObject& control) {
    if (control.comctl == nullptr || control.window == ui::kNoWindow ||
        control.comctl->from_host || control.class_key == "commondialog" ||
        control.class_key == "winsock") {
        return;
    }
    FormSupport::runtime(in).host->sync_common(control.window, comctl_snapshot(in, control));
}
