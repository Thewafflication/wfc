// Behavior tests of the VBRUN helpers (REQ-0100 to REQ-0104). Usage:
//   vbrun_helper_tests <wfcvbrun.dll> <family>
// Each family calls exports of the library by name with the argument lists of
// the reference runtime and checks the results and the effects the reference
// has (probed on MSVBVM60 6.00.9848 with scripts/Probe-Vbrun.ps1).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <oleauto.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

HMODULE g_library = nullptr;
int g_failures = 0;
int g_checks = 0;

void check(const bool condition, const char* const what) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::fprintf(stderr, "FAIL: %s\n", what);
    }
}

template <typename Function>
Function export_of(const char* const name) {
    const FARPROC proc = GetProcAddress(g_library, name);
    if (proc == nullptr) {
        std::fprintf(stderr, "FAIL: %s is not exported\n", name);
        ++g_failures;
    }
    return reinterpret_cast<Function>(proc);
}

// ----- REQ-0103 member and memory helpers ---------------------------------

// A counted object for the object helpers.
struct Counted final : IUnknown {
    long references = 1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(++references);
    }
    ULONG STDMETHODCALLTYPE Release() override {
        return static_cast<ULONG>(--references);
    }
};

void memory_family() {
    using Get1 = HRESULT(WINAPI*)(const std::uint8_t*, std::uint8_t*);
    using Get2 = HRESULT(WINAPI*)(const std::uint16_t*, std::uint16_t*);
    using Get4 = HRESULT(WINAPI*)(const std::uint32_t*, std::uint32_t*);
    using Put1 = HRESULT(WINAPI*)(std::uint8_t*, std::uint8_t);
    using Put2 = HRESULT(WINAPI*)(std::uint16_t*, std::uint16_t);
    using Put4 = HRESULT(WINAPI*)(std::uint32_t*, std::uint32_t);
    using Put8 = HRESULT(WINAPI*)(std::uint32_t*, std::uint32_t, std::uint32_t);
    const auto get1 = export_of<Get1>("GetMem1");
    const auto get2 = export_of<Get2>("GetMem2");
    const auto get4 = export_of<Get4>("GetMem4");
    const auto get8 = export_of<Get4>("GetMem8");
    const auto put1 = export_of<Put1>("PutMem1");
    const auto put2 = export_of<Put2>("PutMem2");
    const auto put4 = export_of<Put4>("PutMem4");
    const auto put8 = export_of<Put8>("PutMem8");
    const auto var_ptr = export_of<LONG_PTR(WINAPI*)(void*)>("VarPtr");
    if (!get1 || !get2 || !get4 || !get8 || !put1 || !put2 || !put4 || !put8 ||
        !var_ptr) {
        return;
    }
    std::uint8_t byte_in = 0xA5;
    std::uint8_t byte_out = 0;
    check(get1(&byte_in, &byte_out) == S_OK && byte_out == 0xA5, "GetMem1");
    std::uint16_t word_in = 0xBEEF;
    std::uint16_t word_out = 0;
    check(get2(&word_in, &word_out) == S_OK && word_out == 0xBEEF, "GetMem2");
    std::uint32_t long_in = 0xDEADBEEFU;
    std::uint32_t long_out = 0;
    check(get4(&long_in, &long_out) == S_OK && long_out == 0xDEADBEEFU,
          "GetMem4");
    const std::uint32_t wide_in[2] = {0x01234567U, 0x89ABCDEFU};
    std::uint32_t wide_out[2] = {0, 0};
    check(get8(wide_in, wide_out) == S_OK && wide_out[0] == 0x01234567U &&
              wide_out[1] == 0x89ABCDEFU,
          "GetMem8");

    std::uint8_t put_byte = 0;
    check(put1(&put_byte, 0x7F) == S_OK && put_byte == 0x7F, "PutMem1");
    std::uint16_t put_word = 0;
    check(put2(&put_word, 0x1234) == S_OK && put_word == 0x1234, "PutMem2");
    std::uint32_t put_long = 0;
    check(put4(&put_long, 0xCAFEF00DU) == S_OK && put_long == 0xCAFEF00DU,
          "PutMem4");
    std::uint32_t put_wide[2] = {0, 0};
    check(put8(put_wide, 0x11223344U, 0x55667788U) == S_OK &&
              put_wide[0] == 0x11223344U && put_wide[1] == 0x55667788U,
          "PutMem8");
    check(var_ptr(&put_long) == reinterpret_cast<LONG_PTR>(&put_long),
          "VarPtr");

    // Strings are copied by byte length (an odd length is kept).
    using GetStr = HRESULT(WINAPI*)(BSTR*, BSTR*);
    using PutStr = HRESULT(WINAPI*)(BSTR*, BSTR);
    const auto get_str = export_of<GetStr>("GetMemStr");
    const auto put_str = export_of<PutStr>("PutMemStr");
    if (get_str && put_str) {
        BSTR source = SysAllocString(L"hello");
        BSTR copy = nullptr;
        check(get_str(&source, &copy) == S_OK && copy != nullptr &&
                  copy != source && SysStringLen(copy) == 5U &&
                  std::wmemcmp(copy, L"hello", 5) == 0,
              "GetMemStr copies a String");
        SysFreeString(copy);
        SysFreeString(source);
        BSTR odd = SysAllocStringByteLen("abc", 3);
        BSTR odd_copy = nullptr;
        check(get_str(&odd, &odd_copy) == S_OK && odd_copy != nullptr &&
                  SysStringByteLen(odd_copy) == 3U &&
                  std::memcmp(odd_copy, "abc", 3) == 0,
              "GetMemStr keeps an odd byte count");
        SysFreeString(odd);
        SysFreeString(odd_copy);
        BSTR none = nullptr;
        BSTR got = SysAllocString(L"x");
        check(get_str(&none, &got) == S_OK && got == nullptr,
              "GetMemStr of an empty String gives an empty String");

        BSTR target = SysAllocString(L"old");
        BSTR value = SysAllocString(L"new value");
        check(put_str(&target, value) == S_OK && target != nullptr &&
                  target != value && SysStringLen(target) == 9U &&
                  std::wmemcmp(target, L"new value", 9) == 0,
              "PutMemStr replaces a String with a copy");
        check(put_str(&target, nullptr) == S_OK && target != nullptr &&
                  SysStringByteLen(target) == 0U,
              "PutMemStr of an empty String gives an allocated empty String");
        SysFreeString(target);
        SysFreeString(value);
    }

    // Objects are counted.
    using GetObj = HRESULT(WINAPI*)(IUnknown**, IUnknown**);
    using SetObj = HRESULT(WINAPI*)(IUnknown**, IUnknown*);
    const auto get_obj = export_of<GetObj>("GetMemObj");
    const auto set_obj = export_of<SetObj>("SetMemObj");
    if (get_obj && set_obj) {
        Counted first;
        Counted second;
        IUnknown* held = &first;
        IUnknown* taken = nullptr;
        check(get_obj(&held, &taken) == S_OK && taken == &first &&
                  first.references == 2,
              "GetMemObj counts the copy");
        check(set_obj(&held, &second) == S_OK && held == &second &&
                  second.references == 2 && first.references == 1,
              "SetMemObj counts the new object and lets the old one go");
        check(set_obj(&held, nullptr) == S_OK && held == nullptr &&
                  second.references == 1,
              "SetMemObj of Nothing");
        IUnknown* empty = nullptr;
        IUnknown* sink = &first;
        check(get_obj(&empty, &sink) == S_OK && sink == nullptr,
              "GetMemObj of Nothing");
    }

    // A Variant is copied.
    using GetVar = HRESULT(WINAPI*)(VARIANT*, VARIANT*);
    const auto get_var = export_of<GetVar>("GetMemVar");
    if (get_var) {
        VARIANT from;
        VariantInit(&from);
        from.vt = VT_BSTR;
        from.bstrVal = SysAllocString(L"variant text");
        VARIANT to;
        VariantInit(&to);
        check(get_var(&from, &to) == S_OK && to.vt == VT_BSTR &&
                  to.bstrVal != from.bstrVal &&
                  std::wcscmp(to.bstrVal, L"variant text") == 0,
              "GetMemVar copies a Variant");
        VariantClear(&from);
        VariantClear(&to);
    }
}

}  // namespace

int main(const int argument_count, const char* const arguments[]) {
    if (argument_count != 3) {
        std::fprintf(stderr, "usage: vbrun_helper_tests <dll> <family>\n");
        return 2;
    }
    g_library = LoadLibraryA(arguments[1]);
    if (g_library == nullptr) {
        std::fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
        return 2;
    }
    const std::string family = arguments[2];
    if (family == "memory") {
        memory_family();
    } else {
        std::fprintf(stderr, "unknown family %s\n", arguments[2]);
        return 2;
    }
    if (g_failures != 0) {
        std::fprintf(stderr, "%d of %d checks failed\n", g_failures, g_checks);
        return 1;
    }
    std::printf("%s ok: %d checks\n", arguments[2], g_checks);
    return 0;
}
