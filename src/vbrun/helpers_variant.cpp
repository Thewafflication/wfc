// REQ-0102: the type tests of the language runtime on Variants (VarType,
// IsEmpty, IsNull, IsArray, IsObject, IsError).
//
// They return a 16-bit result (the reference sets only AX): -1 for True.

#include <cstdint>

#include "vbrun_helpers.hpp"

namespace {

// The type of a Variant as the language sees it: the by-reference flag is
// dropped, and an object stands for the type of its default property.
std::uint16_t language_type(const VARIANT* const value) {
    const unsigned vt = value->vt & ~static_cast<unsigned>(VT_BYREF);
    if (vt != VT_DISPATCH) {
        return static_cast<std::uint16_t>(vt);
    }
    IDispatch* const object =
        (value->vt & VT_BYREF) != 0
            ? (value->ppdispVal != nullptr ? *value->ppdispVal : nullptr)
            : value->pdispVal;
    if (object == nullptr) {
        return static_cast<std::uint16_t>(vt);
    }
    DISPPARAMS none = {nullptr, nullptr, 0, 0};
    VARIANT held;
    VariantInit(&held);
    const HRESULT result = object->Invoke(
        DISPID_VALUE, IID_NULL, 0, DISPATCH_METHOD | DISPATCH_PROPERTYGET,
        &none, &held, nullptr, nullptr);
    if (FAILED(result)) {
        return static_cast<std::uint16_t>(vt);
    }
    const auto type = static_cast<std::uint16_t>(held.vt);
    VariantClear(&held);
    return type;
}

SHORT truth(const bool condition) {
    return condition ? static_cast<SHORT>(-1) : static_cast<SHORT>(0);
}

}  // namespace

extern "C" {

LONG WINAPI rtcVarType(const VARIANT* value) {
    return language_type(value);
}

SHORT WINAPI rtcIsEmpty(const VARIANT* value) {
    return truth(language_type(value) == VT_EMPTY);
}

SHORT WINAPI rtcIsNull(const VARIANT* value) {
    return truth(language_type(value) == VT_NULL);
}

SHORT WINAPI rtcIsArray(const VARIANT* value) {
    return truth((language_type(value) & VT_ARRAY) != 0);
}

SHORT WINAPI rtcIsError(const VARIANT* value) {
    return truth(language_type(value) == VT_ERROR);
}

// IsObject looks at the type itself (no default property is read).
SHORT WINAPI rtcIsObject(const VARIANT* value) {
    const unsigned vt = value->vt & ~static_cast<unsigned>(VT_BYREF);
    return truth(vt == VT_DISPATCH);
}

}  // extern "C"
