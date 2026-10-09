#include "large_stack.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <process.h>
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace wfc::detail {
namespace {

struct Launch {
    void (*entry)(void*);
    void* argument;
};

#if defined(_WIN32)
unsigned __stdcall thread_main(void* raw) {
    const auto* launch = static_cast<const Launch*>(raw);
    launch->entry(launch->argument);
    return 0;
}
#else
void* thread_main(void* raw) {
    const auto* launch = static_cast<const Launch*>(raw);
    launch->entry(launch->argument);
    return nullptr;
}
#endif

}  // namespace

bool run_on_thread_with_stack(void (*entry)(void*), void* argument,
                              const std::size_t stack_bytes) {
    Launch launch{entry, argument};
#if defined(_WIN32)
    const auto handle = reinterpret_cast<HANDLE>(
        _beginthreadex(nullptr, static_cast<unsigned>(stack_bytes), thread_main,
                       &launch, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr));
    if (handle == nullptr) {
        return false;
    }
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    return true;
#else
    pthread_attr_t attributes;
    if (pthread_attr_init(&attributes) != 0) {
        return false;
    }
    pthread_t thread;
    const bool created =
        pthread_attr_setstacksize(&attributes, stack_bytes) == 0 &&
        pthread_create(&thread, &attributes, thread_main, &launch) == 0;
    pthread_attr_destroy(&attributes);
    if (!created) {
        return false;
    }
    pthread_join(thread, nullptr);
    return true;
#endif
}

}  // namespace wfc::detail
