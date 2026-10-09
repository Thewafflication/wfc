/// @file unhardened_fixture.cpp
/// @brief Deliberately unhardened image for the WSP-SEC-0016 negative test.
///
/// Built without wsp_enable_hardening(), so it lacks Control Flow Guard.
/// TC-WSP-SEC-0016-pe-hardening-rejects-unhardened passes only when
/// Test-PeHardening.ps1 rejects this image. It is never shipped.

/// @brief Does nothing.
/// @return Always 0.
int main() {
    return 0;
}
