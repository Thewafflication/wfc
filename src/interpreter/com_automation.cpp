// Interpreter: late-bound COM Automation client (REQ-0286). `CreateObject` and
// `GetObject` reach real COM servers through `IDispatch`; member reads, calls,
// property writes, default members and `For Each` work on the resulting
// objects. Internal to the WFC evaluator; not part of the public API.

#include <algorithm>
#include <cctype>

#include "interpreter.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <objbase.h>
#include <ocidl.h>
#include <oleauto.h>
#endif

namespace wfc::detail {

#ifndef _WIN32

ComObject::~ComObject() = default;

std::optional<Value> Interpreter::com_create_object(const std::string&,
                                                    const std::size_t offset) {
    static_cast<void>(
        raise_runtime(429, "ActiveX component can't create object", offset));
    return std::nullopt;
}

std::optional<Value> Interpreter::com_get_object(const std::string&,
                                                 const std::string&,
                                                 const std::size_t offset) {
    static_cast<void>(
        raise_runtime(429, "ActiveX component can't create object", offset));
    return std::nullopt;
}

std::optional<Value> Interpreter::com_get_member(InstanceData&,
                                                 const std::string&,
                                                 const std::size_t offset) {
    static_cast<void>(raise_runtime(
        438, "Object doesn't support this property or method", offset));
    return std::nullopt;
}

bool Interpreter::com_member_statement(const Value&, const std::string&,
                                       const std::size_t offset) {
    static_cast<void>(raise_runtime(
        438, "Object doesn't support this property or method", offset));
    return false;
}

bool Interpreter::com_set_member(const Value&, const std::string&,
                                 const std::size_t offset) {
    static_cast<void>(raise_runtime(
        438, "Object doesn't support this property or method", offset));
    return false;
}

std::optional<Value> Interpreter::com_default_member(InstanceData&,
                                                     const std::size_t offset) {
    static_cast<void>(raise_runtime(
        438, "Object doesn't support this property or method", offset));
    return std::nullopt;
}

std::optional<ArrayValue> Interpreter::com_enumerate(InstanceData&,
                                                     const std::size_t offset) {
    static_cast<void>(raise_runtime(
        438, "Object doesn't support this property or method", offset));
    return std::nullopt;
}

std::string Interpreter::com_type_name(InstanceData&) {
    return "Object";
}

#else

namespace {

// COM is initialized (single-threaded apartment) once per interpreter thread
// the first time an Automation call needs it, and torn down with the thread.
struct ComApartment {
    ComApartment() {
        const HRESULT result =
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        initialized = SUCCEEDED(result);
    }
    ~ComApartment() {
        if (initialized) {
            CoUninitialize();
        }
    }
    bool initialized{};
};

void ensure_apartment() {
    thread_local const ComApartment apartment;
    static_cast<void>(apartment);
}

[[nodiscard]] std::wstring widen(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        wide.data(), length);
    return wide;
}

[[nodiscard]] std::string narrow(const wchar_t* text, const std::size_t count) {
    if (text == nullptr || count == 0U) {
        return {};
    }
    const int length =
        WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(count), nullptr,
                            0, nullptr, nullptr);
    std::string narrow_text(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(count),
                        narrow_text.data(), length, nullptr, nullptr);
    return narrow_text;
}

[[nodiscard]] std::string narrow(const BSTR text) {
    return text == nullptr ? std::string{} : narrow(text, SysStringLen(text));
}

[[nodiscard]] IDispatch* dispatch_of(const ComObject& object) {
    return static_cast<IDispatch*>(object.dispatch);
}

// Wraps `dispatch` (AddRef'd here) as an interpreter object value.
[[nodiscard]] Value wrap_dispatch(IDispatch* dispatch) {
    if (dispatch == nullptr) {
        return Value{Nothing{}};
    }
    dispatch->AddRef();
    auto data = std::make_shared<InstanceData>();
    data->class_name = "wfccom";
    data->com = std::make_shared<ComObject>(dispatch);
    return Value{ObjectInstance{std::move(data)}};
}

[[nodiscard]] Value variant_to_value(const VARIANT& variant);

[[nodiscard]] Value safearray_to_value(SAFEARRAY* array) {
    if (array == nullptr) {
        return Value{Empty{}};
    }
    VARTYPE element_type = VT_VARIANT;
    SafeArrayGetVartype(array, &element_type);
    const UINT dimensions = SafeArrayGetDim(array);
    ArrayValue result{};
    result.is_variant_element = true;
    result.element_type_index = Value{Empty{}}.index();
    std::vector<std::pair<LONG, LONG>> bounds;
    std::size_t total = 1U;
    for (UINT d = 1; d <= dimensions; ++d) {
        LONG low = 0;
        LONG high = -1;
        SafeArrayGetLBound(array, d, &low);
        SafeArrayGetUBound(array, d, &high);
        bounds.emplace_back(low, high);
        total *= high >= low ? static_cast<std::size_t>(high - low + 1) : 0U;
    }
    // SAFEARRAY API index 0 is the right-most dimension (see
    // SafeArrayGetElement). Walk the VB-order (row-major) positions and map
    // each to an API index.
    std::vector<LONG> position(dimensions);
    for (UINT d = 0; d < dimensions; ++d) {
        position[d] = bounds[d].first;
    }
    result.elements.reserve(total);
    for (std::size_t n = 0; n < total; ++n) {
        std::vector<LONG> api_index(dimensions);
        for (UINT d = 0; d < dimensions; ++d) {
            api_index[dimensions - 1U - d] = position[d];
        }
        VARIANT element;
        VariantInit(&element);
        if (element_type == VT_VARIANT) {
            SafeArrayGetElement(array, api_index.data(), &element);
        } else if (element_type == VT_DISPATCH || element_type == VT_UNKNOWN) {
            IUnknown* unknown = nullptr;
            SafeArrayGetElement(array, api_index.data(), &unknown);
            if (unknown != nullptr) {
                IDispatch* dispatch = nullptr;
                unknown->QueryInterface(IID_IDispatch,
                                        reinterpret_cast<void**>(&dispatch));
                unknown->Release();
                element.vt = VT_DISPATCH;
                element.pdispVal = dispatch;
            }
        } else if (element_type == VT_BSTR) {
            BSTR text = nullptr;
            SafeArrayGetElement(array, api_index.data(), &text);
            element.vt = VT_BSTR;
            element.bstrVal = text;
        } else if (element_type != VT_DECIMAL) {
            SafeArrayGetElement(array, api_index.data(), &element.llVal);
            element.vt = element_type;
        }
        result.elements.push_back(variant_to_value(element));
        VariantClear(&element);
        for (UINT d = dimensions; d-- > 0U;) {
            if (position[d] < bounds[d].second) {
                ++position[d];
                break;
            }
            position[d] = bounds[d].first;
        }
    }
    if (dimensions == 1U) {
        result.lower_bound = bounds[0].first;
    } else {
        for (const auto& [low, high] : bounds) {
            result.dimensions.emplace_back(low, high);
        }
    }
    return Value{std::move(result)};
}

[[nodiscard]] Value variant_to_value(const VARIANT& variant) {
    if ((variant.vt & VT_ARRAY) != 0) {
        SAFEARRAY* array =
            (variant.vt & VT_BYREF) != 0 ? *variant.pparray : variant.parray;
        return safearray_to_value(array);
    }
    const bool by_reference = (variant.vt & VT_BYREF) != 0;
    switch (variant.vt & VT_TYPEMASK) {
        case VT_EMPTY:
            return Value{Empty{}};
        case VT_NULL:
            return Value{Null{}};
        case VT_I2:
            return Value{by_reference ? *variant.piVal : variant.iVal};
        case VT_I4:
        case VT_INT:
            return Value{static_cast<Integer>(by_reference ? *variant.plVal
                                                           : variant.lVal)};
        case VT_R4:
            return Value{by_reference ? *variant.pfltVal : variant.fltVal};
        case VT_R8:
            return Value{by_reference ? *variant.pdblVal : variant.dblVal};
        case VT_CY:
            return Value{Currency{
                (by_reference ? *variant.pcyVal : variant.cyVal).int64}};
        case VT_DATE:
            return Value{
                DateValue{by_reference ? *variant.pdate : variant.date}};
        case VT_BSTR:
            return Value{
                narrow(by_reference ? *variant.pbstrVal : variant.bstrVal)};
        case VT_BOOL:
            return Value{(by_reference ? *variant.pboolVal : variant.boolVal) !=
                         VARIANT_FALSE};
        case VT_ERROR:
            return Value{ErrorValue{static_cast<std::int32_t>(
                by_reference ? *variant.pscode : variant.scode)}};
        case VT_UI1:
            return Value{Byte{by_reference ? *variant.pbVal : variant.bVal}};
        case VT_I1:
            return Value{static_cast<Int16>(by_reference ? *variant.pcVal
                                                         : variant.cVal)};
        case VT_UI2:
            return Value{static_cast<Integer>(by_reference ? *variant.puiVal
                                                           : variant.uiVal)};
        case VT_UI4:
        case VT_UINT: {
            const ULONG number = by_reference ? *variant.pulVal : variant.ulVal;
            if (number <= 0x7FFFFFFFUL) {
                return Value{static_cast<Integer>(number)};
            }
            return Value{static_cast<double>(number)};
        }
        case VT_I8:
            return Value{static_cast<double>(by_reference ? *variant.pllVal
                                                          : variant.llVal)};
        case VT_UI8:
            return Value{static_cast<double>(by_reference ? *variant.pullVal
                                                          : variant.ullVal)};
        case VT_DECIMAL: {
            double number = 0.0;
            VarR8FromDec(by_reference ? variant.pdecVal : &variant.decVal,
                         &number);
            return Value{number};
        }
        case VT_DISPATCH: {
            IDispatch* dispatch =
                by_reference ? *variant.ppdispVal : variant.pdispVal;
            return wrap_dispatch(dispatch);
        }
        case VT_UNKNOWN: {
            IUnknown* unknown =
                by_reference ? *variant.ppunkVal : variant.punkVal;
            if (unknown == nullptr) {
                return Value{Nothing{}};
            }
            IDispatch* dispatch = nullptr;
            if (SUCCEEDED(unknown->QueryInterface(
                    IID_IDispatch, reinterpret_cast<void**>(&dispatch)))) {
                Value wrapped = wrap_dispatch(dispatch);
                dispatch->Release();
                return wrapped;
            }
            return Value{Nothing{}};
        }
        case VT_VARIANT:
            if (by_reference && variant.pvarVal != nullptr) {
                return variant_to_value(*variant.pvarVal);
            }
            return Value{Empty{}};
        default:
            return Value{Empty{}};
    }
}

// Converts `value` into `out` (which must be VT_EMPTY). Returns false when the
// value has no Automation representation (an object that is not a COM object).
[[nodiscard]] bool value_to_variant(const Value& value, VARIANT& out) {
    VariantInit(&out);
    if (std::holds_alternative<Empty>(value)) {
        return true;
    }
    if (std::holds_alternative<Null>(value)) {
        out.vt = VT_NULL;
        return true;
    }
    if (const auto* number = std::get_if<Integer>(&value)) {
        out.vt = VT_I4;
        out.lVal = *number;
        return true;
    }
    if (const auto* number = std::get_if<Int16>(&value)) {
        out.vt = VT_I2;
        out.iVal = *number;
        return true;
    }
    if (const auto* number = std::get_if<Byte>(&value)) {
        out.vt = VT_UI1;
        out.bVal = *number;
        return true;
    }
    if (const auto* flag = std::get_if<bool>(&value)) {
        out.vt = VT_BOOL;
        out.boolVal = *flag ? VARIANT_TRUE : VARIANT_FALSE;
        return true;
    }
    if (const auto* number = std::get_if<double>(&value)) {
        out.vt = VT_R8;
        out.dblVal = *number;
        return true;
    }
    if (const auto* number = std::get_if<float>(&value)) {
        out.vt = VT_R4;
        out.fltVal = *number;
        return true;
    }
    if (const auto* money = std::get_if<Currency>(&value)) {
        out.vt = VT_CY;
        out.cyVal.int64 = money->scaled;
        return true;
    }
    if (const auto* decimal = std::get_if<Decimal>(&value)) {
        out.vt = VT_R8;
        out.dblVal = as_double(Value{*decimal});
        return true;
    }
    if (const auto* date = std::get_if<DateValue>(&value)) {
        out.vt = VT_DATE;
        out.date = date->serial;
        return true;
    }
    if (const auto* error = std::get_if<ErrorValue>(&value)) {
        out.vt = VT_ERROR;
        out.scode = error->code;
        return true;
    }
    if (const auto* text = std::get_if<std::string>(&value)) {
        const auto wide = widen(*text);
        out.vt = VT_BSTR;
        out.bstrVal =
            SysAllocStringLen(wide.data(), static_cast<UINT>(wide.size()));
        return true;
    }
    if (std::holds_alternative<Nothing>(value)) {
        out.vt = VT_DISPATCH;
        out.pdispVal = nullptr;
        return true;
    }
    if (const auto* object = std::get_if<ObjectInstance>(&value)) {
        if (object->data == nullptr || object->data->com == nullptr) {
            return false;
        }
        auto* const dispatch = dispatch_of(*object->data->com);
        dispatch->AddRef();
        out.vt = VT_DISPATCH;
        out.pdispVal = dispatch;
        return true;
    }
    if (const auto* array = std::get_if<ArrayValue>(&value)) {
        const std::size_t dimensions =
            array->dimensions.empty() ? 1U : array->dimensions.size();
        std::vector<SAFEARRAYBOUND> bounds(dimensions);
        std::vector<LONG> lows(dimensions);
        if (array->dimensions.empty()) {
            bounds[0].cElements = static_cast<ULONG>(array->elements.size());
            bounds[0].lLbound = array->lower_bound;
            lows[0] = array->lower_bound;
        } else {
            // API bound 0 is the right-most dimension.
            for (std::size_t d = 0; d < dimensions; ++d) {
                const auto& [low, high] = array->dimensions[d];
                bounds[dimensions - 1U - d].lLbound = low;
                bounds[dimensions - 1U - d].cElements =
                    high >= low ? static_cast<ULONG>(high - low + 1) : 0UL;
                lows[d] = low;
            }
        }
        SAFEARRAY* created = SafeArrayCreate(
            VT_VARIANT, static_cast<UINT>(dimensions), bounds.data());
        if (created == nullptr) {
            return false;
        }
        std::vector<LONG> position = lows;
        for (const auto& element : array->elements) {
            VARIANT converted;
            if (!value_to_variant(element, converted)) {
                VariantClear(&converted);
                SafeArrayDestroy(created);
                return false;
            }
            std::vector<LONG> api_index(dimensions);
            for (std::size_t d = 0; d < dimensions; ++d) {
                api_index[dimensions - 1U - d] = position[d];
            }
            SafeArrayPutElement(created, api_index.data(), &converted);
            VariantClear(&converted);
            for (std::size_t d = dimensions; d-- > 0U;) {
                const LONG high =
                    array->dimensions.empty()
                        ? lows[0] + static_cast<LONG>(array->elements.size()) -
                              1
                        : array->dimensions[d].second;
                if (position[d] < high) {
                    ++position[d];
                    break;
                }
                position[d] = lows[d];
            }
        }
        out.vt = VT_ARRAY | VT_VARIANT;
        out.parray = created;
        return true;
    }
    return false;
}

struct ComFailure {
    Integer number{};
    std::string description;
    std::string source;
    std::string help_file;
    Integer help_context{};
};

[[nodiscard]] ComFailure failure_for(const HRESULT result,
                                     const EXCEPINFO& exception,
                                     const UINT bad_argument) {
    static_cast<void>(bad_argument);
    ComFailure failure;
    switch (result) {
        case DISP_E_MEMBERNOTFOUND:
        case DISP_E_UNKNOWNNAME:
            failure.number = 438;
            failure.description =
                "Object doesn't support this property or method";
            break;
        case DISP_E_TYPEMISMATCH:
            failure.number = 13;
            failure.description = "Type mismatch";
            break;
        case DISP_E_BADPARAMCOUNT:
            failure.number = 450;
            failure.description =
                "Wrong number of arguments or invalid "
                "property assignment";
            break;
        case DISP_E_PARAMNOTOPTIONAL:
            failure.number = 449;
            failure.description = "Argument not optional";
            break;
        case DISP_E_BADVARTYPE:
            failure.number = 458;
            failure.description =
                "Variable uses an Automation type not supported in Visual "
                "Basic";
            break;
        case DISP_E_OVERFLOW:
            failure.number = 6;
            failure.description = "Overflow";
            break;
        case DISP_E_EXCEPTION: {
            const HRESULT code = exception.scode != 0
                                     ? exception.scode
                                     : static_cast<HRESULT>(exception.wCode);
            failure.number = static_cast<Integer>(code);
            failure.description = narrow(exception.bstrDescription);
            failure.source = narrow(exception.bstrSource);
            failure.help_file = narrow(exception.bstrHelpFile);
            failure.help_context =
                static_cast<Integer>(exception.dwHelpContext);
            if (failure.description.empty()) {
                failure.description = "Automation error";
            }
            break;
        }
        default:
            failure.number = static_cast<Integer>(result);
            failure.description = "Automation error";
            break;
    }
    return failure;
}

}  // namespace

ComObject::~ComObject() {
    if (dispatch != nullptr) {
        static_cast<IDispatch*>(dispatch)->Release();
    }
}

namespace {

[[nodiscard]] bool ids_for(ComObject& object, const std::string& member,
                           const std::vector<CallArgument>& arguments,
                           std::vector<DISPID>& ids, HRESULT& result) {
    IDispatch* const dispatch = dispatch_of(object);
    std::vector<std::wstring> names;
    names.push_back(widen(member));
    for (const auto& argument : arguments) {
        if (!argument.name.empty()) {
            names.push_back(widen(argument.name));
        }
    }
    std::vector<LPOLESTR> pointers;
    pointers.reserve(names.size());
    for (auto& name : names) {
        pointers.push_back(name.data());
    }
    ids.assign(names.size(), DISPID_UNKNOWN);
    if (names.size() == 1U) {
        const auto cached = object.dispids.find(member);
        if (cached != object.dispids.end()) {
            ids[0] = cached->second;
            result = S_OK;
            return true;
        }
    }
    result = dispatch->GetIDsOfNames(IID_NULL, pointers.data(),
                                     static_cast<UINT>(pointers.size()),
                                     LOCALE_USER_DEFAULT, ids.data());
    if (FAILED(result)) {
        return false;
    }
    if (names.size() == 1U) {
        object.dispids.emplace(member, ids[0]);
    }
    return true;
}

// Runs one Invoke; on success `result` is the converted return value.
struct InvokeOutcome {
    bool ok{};
    Value result{Empty{}};
    ComFailure failure;
};

[[nodiscard]] InvokeOutcome invoke_member(InstanceData& instance,
                                          const std::string& member,
                                          const WORD flags,
                                          std::vector<CallArgument>& arguments,
                                          const Value* put_value) {
    InvokeOutcome outcome;
    ensure_apartment();
    ComObject& object = *instance.com;
    std::vector<DISPID> ids;
    HRESULT status = S_OK;
    DISPID member_id = DISPID_VALUE;
    std::vector<DISPID> named_ids;
    if (!member.empty()) {
        if (!ids_for(object, member, arguments, ids, status)) {
            outcome.failure = failure_for(
                status == DISP_E_UNKNOWNNAME ? DISP_E_MEMBERNOTFOUND : status,
                EXCEPINFO{}, 0);
            return outcome;
        }
        member_id = ids[0];
        for (std::size_t i = 1; i < ids.size(); ++i) {
            named_ids.push_back(ids[i]);
        }
    }

    // IDispatch wants arguments right to left; named arguments come first
    // in that array, in the order of their DISPIDs.
    std::vector<std::size_t> positional;
    std::vector<std::size_t> named;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        (arguments[i].name.empty() ? positional : named).push_back(i);
    }
    const bool is_put =
        (flags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) != 0;
    std::vector<VARIANT> storage(arguments.size() + (is_put ? 1U : 0U));
    std::vector<bool> by_reference(storage.size(), false);
    std::vector<VARIANT> reference_copies(storage.size());
    for (auto& v : storage) {
        VariantInit(&v);
    }
    for (auto& v : reference_copies) {
        VariantInit(&v);
    }
    const auto cleanup = [&] {
        for (auto& v : storage) {
            VariantClear(&v);
        }
        for (auto& v : reference_copies) {
            VariantClear(&v);
        }
    };
    std::size_t slot = 0;
    const auto place = [&](const CallArgument& argument) -> bool {
        VARIANT& target = storage[slot];
        if (argument.omitted) {
            target.vt = VT_ERROR;
            target.scode = DISP_E_PARAMNOTFOUND;
        } else if (argument.byref_target != nullptr && !is_put) {
            // A variable argument is passed by reference so an [out] or
            // [in, out] parameter can write back.
            if (!value_to_variant(*argument.byref_target,
                                  reference_copies[slot])) {
                return false;
            }
            by_reference[slot] = true;
            target.vt = VT_BYREF | VT_VARIANT;
            target.pvarVal = &reference_copies[slot];
        } else if (!value_to_variant(argument.value, target)) {
            return false;
        }
        ++slot;
        return true;
    };
    bool converted = true;
    std::vector<const CallArgument*> in_order;
    for (std::size_t k = named.size(); k-- > 0U;) {
        in_order.push_back(&arguments[named[k]]);
    }
    for (std::size_t k = positional.size(); k-- > 0U;) {
        in_order.push_back(&arguments[positional[k]]);
    }
    std::vector<std::size_t> slot_of_argument(arguments.size());
    for (const CallArgument* argument : in_order) {
        slot_of_argument[static_cast<std::size_t>(argument -
                                                  arguments.data())] = slot;
        if (!place(*argument)) {
            converted = false;
            break;
        }
    }
    if (converted && is_put) {
        // The assigned value is the right-most (first in source order after
        // the index arguments) element; it sits at the end of the array.
        CallArgument value_argument{*put_value, nullptr, false, {}};
        // Reuse `place` with a by-value argument.
        if (!place(value_argument)) {
            converted = false;
        }
    }
    if (!converted) {
        cleanup();
        outcome.failure = failure_for(DISP_E_TYPEMISMATCH, EXCEPINFO{}, 0);
        return outcome;
    }

    DISPPARAMS params{};
    params.cArgs = static_cast<UINT>(slot);
    params.rgvarg = slot == 0U ? nullptr : storage.data();
    DISPID put_id = DISPID_PROPERTYPUT;
    std::vector<DISPID> all_named;
    // Named arguments occupy the first `named.size()` array slots.
    for (std::size_t k = named.size(); k-- > 0U;) {
        all_named.push_back(named_ids[k]);
    }
    if (is_put) {
        // For a put, the value is the last array slot only when there are no
        // named arguments; the property-put DISPID names it.
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &put_id;
        // IDispatch requires the named (value) argument first in the array.
        // Rotate: move the final slot (the value) to the front.
        std::rotate(storage.begin(), storage.begin() + (slot - 1U),
                    storage.begin() + slot);
        std::rotate(by_reference.begin(), by_reference.begin() + (slot - 1U),
                    by_reference.begin() + slot);
        std::rotate(reference_copies.begin(),
                    reference_copies.begin() + (slot - 1U),
                    reference_copies.begin() + slot);
    } else if (!all_named.empty()) {
        params.cNamedArgs = static_cast<UINT>(all_named.size());
        params.rgdispidNamedArgs = all_named.data();
    }

    VARIANT returned;
    VariantInit(&returned);
    EXCEPINFO exception{};
    UINT bad_argument = 0;
    status = dispatch_of(object)->Invoke(
        member_id, IID_NULL, LOCALE_USER_DEFAULT, flags, &params,
        is_put ? nullptr : &returned, &exception, &bad_argument);
    if (status == DISP_E_MEMBERNOTFOUND && !is_put &&
        (flags & DISPATCH_PROPERTYGET) != 0 && (flags & DISPATCH_METHOD) != 0) {
        // Some servers accept only one of the two kinds.
        status = dispatch_of(object)->Invoke(
            member_id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params,
            &returned, &exception, &bad_argument);
    }
    if (FAILED(status)) {
        outcome.failure = failure_for(status, exception, bad_argument);
        SysFreeString(exception.bstrSource);
        SysFreeString(exception.bstrDescription);
        SysFreeString(exception.bstrHelpFile);
        cleanup();
        VariantClear(&returned);
        return outcome;
    }
    // Copy ByRef results back to the caller's variables.
    if (!is_put) {
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            const std::size_t at = slot_of_argument[i];
            if (by_reference[at] && arguments[i].byref_target != nullptr) {
                // Write back an [out] result without changing the declared
                // type of a typed variable.
                Value updated = variant_to_value(reference_copies[at]);
                Value& target = *arguments[i].byref_target;
                if (updated.index() == target.index() ||
                    std::holds_alternative<Empty>(target)) {
                    target = std::move(updated);
                }
            }
        }
    }
    outcome.ok = true;
    outcome.result = is_put ? Value{Empty{}} : variant_to_value(returned);
    cleanup();
    VariantClear(&returned);
    return outcome;
}

}  // namespace

std::optional<Value> Interpreter::com_create_object(const std::string& progid,
                                                    const std::size_t offset) {
    ensure_apartment();
    CLSID class_id{};
    const auto wide = widen(progid);
    HRESULT status = CLSIDFromProgID(wide.c_str(), &class_id);
    if (FAILED(status)) {
        // A class id written as {GUID} is accepted too.
        status = CLSIDFromString(wide.c_str(), &class_id);
    }
    IDispatch* dispatch = nullptr;
    if (SUCCEEDED(status)) {
        status = CoCreateInstance(
            class_id, nullptr, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
            IID_IDispatch, reinterpret_cast<void**>(&dispatch));
    }
    if (FAILED(status) || dispatch == nullptr) {
        static_cast<void>(raise_runtime(
            429, "ActiveX component can't create object", offset));
        return std::nullopt;
    }
    Value wrapped = wrap_dispatch(dispatch);
    dispatch->Release();
    return wrapped;
}

std::optional<Value> Interpreter::com_get_object(const std::string& path,
                                                 const std::string& progid,
                                                 const std::size_t offset) {
    ensure_apartment();
    IDispatch* dispatch = nullptr;
    HRESULT status = E_FAIL;
    if (path.empty() && !progid.empty()) {
        CLSID class_id{};
        const auto wide = widen(progid);
        status = CLSIDFromProgID(wide.c_str(), &class_id);
        if (SUCCEEDED(status)) {
            IUnknown* unknown = nullptr;
            status = GetActiveObject(class_id, nullptr, &unknown);
            if (SUCCEEDED(status) && unknown != nullptr) {
                status = unknown->QueryInterface(
                    IID_IDispatch, reinterpret_cast<void**>(&dispatch));
                unknown->Release();
            }
        }
        if (FAILED(status)) {
            static_cast<void>(raise_runtime(
                429, "ActiveX component can't create object", offset));
            return std::nullopt;
        }
    } else if (!path.empty()) {
        const auto wide = widen(path);
        status = CoGetObject(wide.c_str(), nullptr, IID_IDispatch,
                             reinterpret_cast<void**>(&dispatch));
        if (FAILED(status) || dispatch == nullptr) {
            static_cast<void>(raise_runtime(
                432,
                "File name or class name not found during Automation "
                "operation",
                offset));
            return std::nullopt;
        }
    } else {
        static_cast<void>(
            raise_runtime(5, "Invalid procedure call or argument", offset));
        return std::nullopt;
    }
    Value wrapped = wrap_dispatch(dispatch);
    dispatch->Release();
    return wrapped;
}

std::optional<Value> Interpreter::com_get_member(InstanceData& instance,
                                                 const std::string& name,
                                                 const std::size_t offset) {
    auto arguments = parse_call_argument_list();
    if (!arguments.has_value()) {
        return std::nullopt;
    }
    if (!execute_) {
        return Value{Empty{}};
    }
    auto outcome =
        invoke_member(instance, name, DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                      *arguments, nullptr);
    if (!outcome.ok) {
        static_cast<void>(raise_runtime(outcome.failure.number,
                                        outcome.failure.description, offset));
        err_source_ = outcome.failure.source;
        err_help_file_ = outcome.failure.help_file;
        err_help_context_ = outcome.failure.help_context;
        return std::nullopt;
    }
    return std::move(outcome.result);
}

std::optional<Value> Interpreter::com_default_member(InstanceData& instance,
                                                     const std::size_t offset) {
    return com_get_member(instance, std::string{}, offset);
}

bool Interpreter::com_member_statement(const Value& base,
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
    const auto report = [&](const InvokeOutcome& outcome) {
        static_cast<void>(raise_runtime(outcome.failure.number,
                                        outcome.failure.description,
                                        member_offset));
        err_source_ = outcome.failure.source;
        err_help_file_ = outcome.failure.help_file;
        err_help_context_ = outcome.failure.help_context;
        return false;
    };
    // `obj.Child.Member = x` / `obj.Items(1).Name = x`: read, then continue
    // the statement against the result.
    if (!at_end() && current() == '.') {
        Value child{Nothing{}};
        if (execute_) {
            auto outcome = invoke_member(instance, name,
                                         DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                                         arguments, nullptr);
            if (!outcome.ok) {
                return report(outcome);
            }
            child = std::move(outcome.result);
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
        // An object on the right of a plain `=` is a Let of its default value
        // in VB6; a property that takes an object needs `Set`.
        auto outcome = invoke_member(instance, name, DISPATCH_PROPERTYPUT,
                                     arguments, &*value);
        if (!outcome.ok) {
            return report(outcome);
        }
        return true;
    }
    // A method call statement: `obj.Method`, `obj.Method(args)` or
    // `obj.Method arg, arg`.
    if (!parenthesized && !at_statement_end()) {
        bare_call_arguments_ = true;
        auto parsed = parse_call_argument_list();
        bare_call_arguments_ = false;
        if (!parsed.has_value()) {
            return false;
        }
        arguments = std::move(*parsed);
    }
    if (!execute_) {
        return true;
    }
    auto outcome =
        invoke_member(instance, name, DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                      arguments, nullptr);
    if (!outcome.ok) {
        return report(outcome);
    }
    return true;
}

bool Interpreter::com_set_member(const Value& base, const std::string& name,
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
    auto outcome = invoke_member(instance, name, DISPATCH_PROPERTYPUTREF,
                                 arguments, &*value);
    if (!outcome.ok) {
        static_cast<void>(raise_runtime(outcome.failure.number,
                                        outcome.failure.description,
                                        member_offset));
        err_source_ = outcome.failure.source;
        return false;
    }
    return true;
}

std::optional<ArrayValue> Interpreter::com_enumerate(InstanceData& instance,
                                                     const std::size_t offset) {
    ensure_apartment();
    std::vector<CallArgument> none;
    VARIANT returned;
    VariantInit(&returned);
    DISPPARAMS params{};
    EXCEPINFO exception{};
    UINT bad_argument = 0;
    const HRESULT status =
        dispatch_of(*instance.com)
            ->Invoke(DISPID_NEWENUM, IID_NULL, LOCALE_USER_DEFAULT,
                     DISPATCH_METHOD | DISPATCH_PROPERTYGET, &params, &returned,
                     &exception, &bad_argument);
    if (FAILED(status)) {
        const auto failure = failure_for(
            status == DISP_E_MEMBERNOTFOUND ? DISP_E_TYPEMISMATCH : status,
            exception, bad_argument);
        static_cast<void>(
            raise_runtime(failure.number, "Object not a collection", offset));
        return std::nullopt;
    }
    IEnumVARIANT* enumerator = nullptr;
    if (returned.vt == VT_UNKNOWN && returned.punkVal != nullptr) {
        returned.punkVal->QueryInterface(IID_IEnumVARIANT,
                                         reinterpret_cast<void**>(&enumerator));
    } else if (returned.vt == VT_DISPATCH && returned.pdispVal != nullptr) {
        returned.pdispVal->QueryInterface(
            IID_IEnumVARIANT, reinterpret_cast<void**>(&enumerator));
    }
    VariantClear(&returned);
    if (enumerator == nullptr) {
        static_cast<void>(
            raise_runtime(438, "Object not a collection", offset));
        return std::nullopt;
    }
    ArrayValue result{};
    result.is_variant_element = true;
    result.element_type_index = Value{Empty{}}.index();
    while (true) {
        VARIANT element;
        VariantInit(&element);
        ULONG fetched = 0;
        const HRESULT next = enumerator->Next(1, &element, &fetched);
        if (next != S_OK || fetched == 0) {
            VariantClear(&element);
            break;
        }
        result.elements.push_back(variant_to_value(element));
        VariantClear(&element);
    }
    enumerator->Release();
    return result;
}

std::string Interpreter::com_type_name(InstanceData& instance) {
    ensure_apartment();
    ITypeInfo* info = nullptr;
    // The coclass name when the server publishes one (as VB reports
    // `FileSystemObject`), else the dispatch interface's own name.
    IProvideClassInfo* provider = nullptr;
    if (SUCCEEDED(dispatch_of(*instance.com)
                      ->QueryInterface(__uuidof(IProvideClassInfo),
                                       reinterpret_cast<void**>(&provider))) &&
        provider != nullptr) {
        provider->GetClassInfo(&info);
        provider->Release();
    }
    if (info == nullptr) {
        dispatch_of(*instance.com)->GetTypeInfo(0, LOCALE_USER_DEFAULT, &info);
    }
    std::string name = "Object";
    if (info != nullptr) {
        BSTR type_name = nullptr;
        if (SUCCEEDED(info->GetDocumentation(MEMBERID_NIL, &type_name, nullptr,
                                             nullptr, nullptr)) &&
            type_name != nullptr) {
            name = narrow(type_name);
        }
        SysFreeString(type_name);
        info->Release();
    }
    return name;
}

#endif  // _WIN32

}  // namespace wfc::detail
