#pragma once

#include <cstddef>

namespace wfc::detail {

// Runs `entry(argument)` on a new thread whose stack reserves `stack_bytes`
// and waits for it to finish. Returns false, without running `entry`, when
// the thread cannot be created.
[[nodiscard]] bool run_on_thread_with_stack(
    void (*entry)(void*), void* argument, std::size_t stack_bytes);

}  // namespace wfc::detail
