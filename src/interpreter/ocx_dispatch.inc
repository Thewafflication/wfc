// The entry points of the OCX controls, called from the common control code
// (mscomctl.inc). Included once by forms.cpp after the OCX libraries.

bool ocx_get(Interpreter& in, ControlObject& self, const std::string& member,
             std::vector<CallArgument>& arguments, Value& result,
             NativeFailure& failure) {
    if (self.class_key == "commondialog") {
        if (member.rfind("show", 0) == 0 && member != "show") {
            result = Value{Empty{}};
            static_cast<void>(show_dialog(in, self, member, failure));
            return true;
        }
        return false;
    }
    if (self.class_key == "winsock") {
        return winsock_get(in, self, member, arguments, result, failure);
    }
    return false;
}

bool ocx_put(Interpreter& in, ControlObject& self, const std::string& member,
             const Value& value, NativeFailure& failure) {
    if (self.class_key == "commondialog" && member == "action") {
        static const char* const kActions[] = {"",          "showopen",
                                               "showsave",  "showcolor",
                                               "showfont",  "showprinter",
                                               "showhelp"};
        const long action = static_cast<long>(as_double(value));
        if (action >= 1 && action <= 6) {
            static_cast<void>(show_dialog(in, self, kActions[action], failure));
        }
        return true;
    }
    if (self.class_key == "winsock") {
        return winsock_put(in, self, member, value, failure);
    }
    return false;
}
