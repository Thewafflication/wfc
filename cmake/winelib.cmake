# Toolchain file for a Linux build with Wine's winegcc (Winelib):
#   cmake -S . -B out/winelib -DCMAKE_TOOLCHAIN_FILE=cmake/winelib.cmake
#         -DWFC_WINELIB_ARCH=x64 -DWFC_WARNINGS_AS_ERRORS=OFF
# WFC_WINELIB_ARCH is x64 (default), x86 (-m32, needs the 32-bit Wine
# development packages) or arm64 (an arm64 host). The programs are Linux
# programs linked against Wine's Windows libraries; run them directly.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER winegcc)
set(CMAKE_CXX_COMPILER wineg++)
# winegcc writes name.exe (a launcher script) and name.exe.so.
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(WFC_WINELIB ON CACHE BOOL "Build with Winelib" FORCE)
if(WFC_WINELIB_ARCH STREQUAL "x86")
  set(CMAKE_C_FLAGS_INIT "-m32")
  set(CMAKE_CXX_FLAGS_INIT "-m32")
endif()
# The Microsoft C runtime functions glibc lacks, and the named-union VARIANT
# of Wine's headers read as the nameless one WFC uses.
set(_wfc_compat "${CMAKE_CURRENT_LIST_DIR}/../src/winelib_compat.hpp")
cmake_path(NORMAL_PATH _wfc_compat)
string(
  APPEND CMAKE_CXX_FLAGS_INIT
  " -fPIC -D_GLIBCXX_EXTERN_TEMPLATE=0"
  " -D_FORCENAMELESSUNION -include ${_wfc_compat}"
)
