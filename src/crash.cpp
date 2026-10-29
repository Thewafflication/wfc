#include "crash.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <dbghelp.h>

#include <cstdio>

#pragma comment(lib, "dbghelp.lib")

namespace wfc {
namespace {

const char* describe(const DWORD code) {
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            return "access violation";
        case EXCEPTION_STACK_OVERFLOW:
            return "stack overflow";
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            return "illegal instruction";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            return "integer division by zero";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return "array bounds exceeded";
        default:
            return "exception";
    }
}

// Where an address is: "module+0xoffset", and the function name when the
// symbols are at hand.
void print_frame(const HANDLE process, const DWORD64 address) {
    IMAGEHLP_MODULE64 module{};
    module.SizeOfStruct = sizeof(module);
    const char* name = "?";
    DWORD64 base = 0;
    if (SymGetModuleInfo64(process, address, &module)) {
        name = module.ModuleName;
        base = module.BaseOfImage;
    }
    char buffer[sizeof(SYMBOL_INFO) + 256] = {};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = 255;
    DWORD64 displacement = 0;
    if (SymFromAddr(process, address, &displacement, symbol)) {
        std::fprintf(stderr, "  %s+0x%llx  %s+0x%llx\n", name,
                     static_cast<unsigned long long>(address - base),
                     symbol->Name,
                     static_cast<unsigned long long>(displacement));
    } else {
        std::fprintf(stderr, "  %s+0x%llx\n", name,
                     static_cast<unsigned long long>(address - base));
    }
}

LONG WINAPI report(EXCEPTION_POINTERS* const exception) {
    const DWORD code = exception->ExceptionRecord->ExceptionCode;
    const auto address =
        reinterpret_cast<DWORD64>(exception->ExceptionRecord->ExceptionAddress);
    std::fprintf(stderr,
                 "wfc: internal error: %s (0x%08lx) at 0x%llx; this is a bug "
                 "in wfc, please report it with the program that caused it\n",
                 describe(code), static_cast<unsigned long>(code),
                 static_cast<unsigned long long>(address));
    if (GetEnvironmentVariableA("WFC_CRASH_STACK", nullptr, 0) != 0) {
        const HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
        SymInitialize(process, nullptr, TRUE);
        print_frame(process, address);
#if defined(_M_X64)
        CONTEXT context = *exception->ContextRecord;
        STACKFRAME64 frame{};
        frame.AddrPC.Offset = context.Rip;
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrStack.Offset = context.Rsp;
        frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode =
            AddrModeFlat;
        for (int depth = 0; depth < 40; ++depth) {
            if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process,
                             GetCurrentThread(), &frame, &context, nullptr,
                             SymFunctionTableAccess64, SymGetModuleBase64,
                             nullptr) ||
                frame.AddrPC.Offset == 0) {
                break;
            }
            print_frame(process, frame.AddrPC.Offset);
        }
#endif
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

// With WFC_CRASH_STACK the report is made at the first chance, before any
// frame based handling: a fault on an unusual stack still gets reported.
LONG WINAPI first_chance(EXCEPTION_POINTERS* const exception) {
    if (exception->ExceptionRecord->ExceptionCode ==
        EXCEPTION_ACCESS_VIOLATION) {
        report(exception);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void install_crash_report() {
    if (GetEnvironmentVariableA("WFC_CRASH_STACK", nullptr, 0) != 0) {
        AddVectoredExceptionHandler(1, &first_chance);
    }
    SetUnhandledExceptionFilter(&report);
    if (GetEnvironmentVariableA("WFC_CRASH_TEST", nullptr, 0) != 0) {
        *static_cast<volatile int*>(nullptr) = 1;
    }
}

}  // namespace wfc

#else

namespace wfc {

void install_crash_report() {}

}  // namespace wfc

#endif
