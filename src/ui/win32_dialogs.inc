// Win32 common dialogs for the Common Dialog control (MP-0008): open and save
// file, colour and font. Included by win32_host.cpp inside namespace wfc::ui.
// Internal to the WFC runtime.

std::optional<FileDialogResult> Win32Host::file_dialog(
    const WindowId owner, const FileDialogRequest& request, bool& supported) {
    supported = true;
    const auto* entry = find(owner);
    // The filter is "Text|*.txt|All|*.*" in VB and double-null terminated here.
    std::wstring filter = widen(request.filter);
    std::replace(filter.begin(), filter.end(), L'|', L'\0');
    filter.push_back(L'\0');
    filter.push_back(L'\0');
    std::wstring buffer(32768, L'\0');
    const std::wstring name = widen(request.file_name);
    std::copy_n(name.begin(), std::min(name.size(), buffer.size() - 1U),
                buffer.begin());
    const std::wstring directory = widen(request.initial_directory);
    const std::wstring title = widen(request.title);
    const std::wstring extension = widen(request.default_extension);

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = entry != nullptr ? entry->hwnd : nullptr;
    dialog.lpstrFilter = request.filter.empty() ? nullptr : filter.c_str();
    dialog.nFilterIndex = static_cast<DWORD>(request.filter_index);
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrInitialDir = directory.empty() ? nullptr : directory.c_str();
    dialog.lpstrTitle = title.empty() ? nullptr : title.c_str();
    dialog.lpstrDefExt = extension.empty() ? nullptr : extension.c_str();
    dialog.Flags = static_cast<DWORD>(request.flags) | OFN_EXPLORER |
                   OFN_NOCHANGEDIR |
                   (request.save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    const BOOL accepted =
        request.save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
    if (accepted == FALSE) {
        return std::nullopt;
    }
    FileDialogResult result;
    result.file_name = narrow(std::wstring(buffer.c_str()));
    result.filter_index = static_cast<long>(dialog.nFilterIndex);
    return result;
}

std::optional<long> Win32Host::color_dialog(const WindowId owner,
                                            const long initial,
                                            const long flags,
                                            bool& supported) {
    supported = true;
    const auto* entry = find(owner);
    static COLORREF custom[16] = {};
    CHOOSECOLORW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = entry != nullptr ? entry->hwnd : nullptr;
    dialog.rgbResult = static_cast<COLORREF>(initial) & 0x00FFFFFFU;
    dialog.lpCustColors = custom;
    dialog.Flags = static_cast<DWORD>(flags) | CC_RGBINIT;
    if (ChooseColorW(&dialog) == FALSE) {
        return std::nullopt;
    }
    return static_cast<long>(dialog.rgbResult);
}

std::optional<FontDialogRequest> Win32Host::font_dialog(
    const WindowId owner, const FontDialogRequest& request, bool& supported) {
    supported = true;
    const auto* entry = find(owner);
    LOGFONTW face{};
    const std::wstring name = widen(request.name);
    std::copy_n(name.begin(), std::min<std::size_t>(name.size(), LF_FACESIZE - 1),
                face.lfFaceName);
    face.lfHeight = -static_cast<LONG>(request.size * 96.0 / 72.0);
    face.lfWeight = request.bold ? FW_BOLD : FW_NORMAL;
    face.lfItalic = request.italic ? TRUE : FALSE;
    face.lfUnderline = request.underline ? TRUE : FALSE;
    face.lfStrikeOut = request.strikethrough ? TRUE : FALSE;
    CHOOSEFONTW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = entry != nullptr ? entry->hwnd : nullptr;
    dialog.lpLogFont = &face;
    dialog.rgbColors = static_cast<COLORREF>(request.color) & 0x00FFFFFFU;
    dialog.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS |
                   static_cast<DWORD>(request.flags & ~0x1L);
    if (ChooseFontW(&dialog) == FALSE) {
        return std::nullopt;
    }
    FontDialogRequest result = request;
    result.name = narrow(std::wstring(face.lfFaceName));
    result.size = static_cast<double>(dialog.iPointSize) / 10.0;
    result.bold = face.lfWeight >= FW_BOLD;
    result.italic = face.lfItalic != FALSE;
    result.underline = face.lfUnderline != FALSE;
    result.strikethrough = face.lfStrikeOut != FALSE;
    result.color = static_cast<long>(dialog.rgbColors);
    return result;
}
