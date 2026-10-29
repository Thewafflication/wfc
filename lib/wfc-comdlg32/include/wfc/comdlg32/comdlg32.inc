// Native replacement for the Common Dialog control (`comdlg32.ocx`, library
// MSComDlg; MP-0008, REQ-0304). Included once by forms.cpp after ocx_support.inc,
// inside namespace wfc::detail. Internal to the WFC evaluator; not part of the
// public API.

// ----- Common Dialog -------------------------------------------------------------

namespace {

[[nodiscard]] std::string file_title(const std::string& path) {
    const auto at = path.find_last_of("/\\");
    return at == std::string::npos ? path : path.substr(at + 1U);
}

// The user cancelled: an error when CancelError is set.
[[nodiscard]] bool dialog_cancelled(ControlObject& self, NativeFailure& failure) {
    if (property_flag(self, "cancelerror")) {
        failure = NativeFailure{kCancelled, "Cancel was selected"};
    }
    return true;
}

[[nodiscard]] bool show_file_dialog(Interpreter& in, ControlObject& self,
                                    const bool save, NativeFailure& failure) {
    ui::FileDialogRequest request;
    request.save = save;
    request.title = property_text(self, "dialogtitle");
    request.initial_directory = property_text(self, "initdir");
    request.file_name = property_text(self, "filename");
    request.filter = property_text(self, "filter");
    request.default_extension = property_text(self, "defaultext");
    request.filter_index = std::max(1L, property_whole(self, "filterindex"));
    request.flags = property_whole(self, "flags");
    bool supported = true;
    auto chosen = FormSupport::runtime(in).host->file_dialog(
        owner_window(self), request, supported);
    if (!supported) {
        // No dialogs: a scripted `answer open|save <path>` stands for the user.
        const auto answer = FormSupport::dialog_answer(in, save ? "save" : "open");
        if (answer.has_value() && lower_text(*answer) != "cancel") {
            chosen = ui::FileDialogResult{*answer, request.filter_index};
        }
    }
    if (!chosen.has_value() || chosen->file_name.empty()) {
        return dialog_cancelled(self, failure);
    }
    self.values["filename"] = Value{chosen->file_name};
    self.values["filetitle"] = Value{file_title(chosen->file_name)};
    self.values["filterindex"] =
        Value{static_cast<Int16>(chosen->filter_index)};
    return true;
}

[[nodiscard]] bool show_color_dialog(Interpreter& in, ControlObject& self,
                                     NativeFailure& failure) {
    bool supported = true;
    auto chosen = FormSupport::runtime(in).host->color_dialog(
        owner_window(self), property_whole(self, "color"),
        property_whole(self, "flags"), supported);
    if (!supported) {
        const auto answer = FormSupport::dialog_answer(in, "color");
        if (answer.has_value() && lower_text(*answer) != "cancel") {
            chosen = parse_number(*answer);
        }
    }
    if (!chosen.has_value()) {
        return dialog_cancelled(self, failure);
    }
    self.values["color"] = Value{static_cast<Integer>(*chosen)};
    return true;
}

[[nodiscard]] bool show_font_dialog(Interpreter& in, ControlObject& self,
                                    NativeFailure& failure) {
    constexpr long kScreenFonts = 0x1;
    constexpr long kPrinterFonts = 0x2;
    const long flags = property_whole(self, "flags");
    if ((flags & (kScreenFonts | kPrinterFonts)) == 0) {
        failure = NativeFailure{kNoFonts, "No fonts exist"};
        return true;
    }
    ui::FontDialogRequest request;
    request.name = property_text(self, "fontname");
    request.size = FormSupport::number_of(self, "fontsize");
    request.bold = property_flag(self, "fontbold");
    request.italic = property_flag(self, "fontitalic");
    request.strikethrough = property_flag(self, "fontstrikethru");
    request.underline = property_flag(self, "fontunderline");
    request.color = property_whole(self, "color");
    request.flags = flags;
    bool supported = true;
    auto chosen = FormSupport::runtime(in).host->font_dialog(
        owner_window(self), request, supported);
    if (!supported) {
        // `answer font Name,Size,Bold,Italic` (Bold and Italic are 0 or 1).
        const auto answer = FormSupport::dialog_answer(in, "font");
        if (answer.has_value() && lower_text(*answer) != "cancel") {
            ui::FontDialogRequest scripted = request;
            std::vector<std::string> parts;
            std::string part;
            std::istringstream stream(*answer);
            while (std::getline(stream, part, ',')) {
                parts.push_back(part);
            }
            if (!parts.empty()) {
                scripted.name = parts[0];
            }
            if (parts.size() > 1U) {
                scripted.size = std::atof(parts[1].c_str());
            }
            if (parts.size() > 2U) {
                scripted.bold = parse_number(parts[2]) != 0;
            }
            if (parts.size() > 3U) {
                scripted.italic = parse_number(parts[3]) != 0;
            }
            chosen = scripted;
        }
    }
    if (!chosen.has_value()) {
        return dialog_cancelled(self, failure);
    }
    self.values["fontname"] = Value{chosen->name};
    self.values["fontsize"] = Value{static_cast<float>(chosen->size)};
    self.values["fontbold"] = Value{chosen->bold};
    self.values["fontitalic"] = Value{chosen->italic};
    self.values["fontstrikethru"] = Value{chosen->strikethrough};
    self.values["fontunderline"] = Value{chosen->underline};
    self.values["color"] = Value{static_cast<Integer>(chosen->color)};
    return true;
}

[[nodiscard]] bool show_dialog(Interpreter& in, ControlObject& self,
                               const std::string& which,
                               NativeFailure& failure) {
    if (which == "showopen") {
        return show_file_dialog(in, self, false, failure);
    }
    if (which == "showsave") {
        return show_file_dialog(in, self, true, failure);
    }
    if (which == "showcolor") {
        return show_color_dialog(in, self, failure);
    }
    if (which == "showfont") {
        return show_font_dialog(in, self, failure);
    }
    if (which == "showprinter") {
        // There is no printer dialog yet: the user cancels it.
        return dialog_cancelled(self, failure);
    }
    return true;  // ShowHelp
}

}  // namespace

std::optional<Value> Interpreter::form_dialog(
    const std::vector<Value>& arguments, const std::size_t offset) {
    static_cast<void>(offset);
    const auto text = [&](const std::size_t at) {
        if (at < arguments.size()) {
            if (const auto* held = std::get_if<std::string>(&arguments[at])) {
                return *held;
            }
        }
        return std::string{};
    };
    const auto number = [&](const std::size_t at) {
        return at < arguments.size()
                   ? static_cast<long>(as_double(arguments[at]))
                   : 0L;
    };
    const auto flag = [&](const std::size_t at) {
        if (at < arguments.size()) {
            if (const auto* held = std::get_if<bool>(&arguments[at])) {
                return *held;
            }
            return as_double(arguments[at]) != 0.0;
        }
        return false;
    };
    if (!execute_) {
        return Value{std::string{}};
    }
    const std::string kind = lower_text(text(0));
    auto& host = *FormSupport::runtime(*this).host;
    const ui::WindowId owner = host.focus();
    bool accepted = false;
    std::string file_name = text(3);
    long filter_index = std::max(1L, number(6));
    long color = number(8);
    std::string font_name = text(9);
    double font_size = std::max(1.0, arguments.size() > 10U ? as_double(arguments[10]) : 8.0);
    bool bold = flag(11);
    bool italic = flag(12);
    bool supported = true;
    if (kind == "open" || kind == "save") {
        ui::FileDialogRequest request;
        request.save = kind == "save";
        request.title = text(1);
        request.initial_directory = text(2);
        request.file_name = file_name;
        request.filter = text(4);
        request.default_extension = text(5);
        request.filter_index = filter_index;
        request.flags = number(7);
        // A scripted answer is used without showing the real dialog.
        const auto answer = form_dialog_answer(kind);
        std::optional<ui::FileDialogResult> chosen;
        if (answer.has_value()) {
            supported = false;
        } else {
            chosen = host.file_dialog(owner, request, supported);
        }
        if (!supported) {
            if (answer.has_value() && lower_text(*answer) != "cancel") {
                chosen = ui::FileDialogResult{*answer, filter_index};
            }
        }
        if (chosen.has_value() && !chosen->file_name.empty()) {
            accepted = true;
            file_name = chosen->file_name;
            filter_index = chosen->filter_index;
        }
    } else if (kind == "color") {
        const auto answer = form_dialog_answer("color");
        std::optional<long> chosen;
        if (answer.has_value()) {
            supported = false;
        } else {
            chosen = host.color_dialog(owner, color, number(7), supported);
        }
        if (!supported) {
            if (answer.has_value() && lower_text(*answer) != "cancel") {
                chosen = static_cast<long>(std::atof(answer->c_str()));
            }
        }
        if (chosen.has_value()) {
            accepted = true;
            color = *chosen;
        }
    } else if (kind == "font") {
        ui::FontDialogRequest request;
        request.name = font_name;
        request.size = font_size;
        request.bold = bold;
        request.italic = italic;
        request.color = color;
        request.flags = 0x1;  // screen fonts
        const auto answer = form_dialog_answer("font");
        std::optional<ui::FontDialogRequest> chosen;
        if (answer.has_value()) {
            supported = false;
        } else {
            chosen = host.font_dialog(owner, request, supported);
        }
        if (!supported) {
            if (answer.has_value() && lower_text(*answer) != "cancel") {
                ui::FontDialogRequest scripted = request;
                std::istringstream stream(*answer);
                std::string part;
                int position = 0;
                while (std::getline(stream, part, ',')) {
                    if (position == 0) {
                        scripted.name = part;
                    } else if (position == 1) {
                        scripted.size = std::atof(part.c_str());
                    } else if (position == 2) {
                        scripted.bold = std::atof(part.c_str()) != 0.0;
                    } else if (position == 3) {
                        scripted.italic = std::atof(part.c_str()) != 0.0;
                    }
                    ++position;
                }
                chosen = scripted;
            }
        }
        if (chosen.has_value()) {
            accepted = true;
            font_name = chosen->name;
            font_size = chosen->size;
            bold = chosen->bold;
            italic = chosen->italic;
            color = chosen->color;
        }
    }
    const char separator = '\x01';
    std::string result = accepted ? "1" : "0";
    result += separator + file_name;
    result += separator + std::to_string(filter_index);
    result += separator + std::to_string(color);
    result += separator + font_name;
    result += separator + std::to_string(font_size);
    result += separator + std::string(bold ? "1" : "0");
    result += separator + std::string(italic ? "1" : "0");
    return Value{std::move(result)};
}
