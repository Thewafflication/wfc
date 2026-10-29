#pragma once

// A crash of wfc itself (an access violation in the runtime or in generated
// code) ends with a message that says so, instead of silently. Setting
// WFC_CRASH_STACK adds the symbolized call stack.

namespace wfc {

void install_crash_report();

}  // namespace wfc
