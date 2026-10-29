// Member table of the native replacement for Winsock (`mswinsck.ocx`, library
// MSWinsockLib). Written by hand from the documented members of the control;
// there is no type-library evidence for it.

#include <wfc/mswinsck/mswinsck.hpp>

namespace wfc::ui {
namespace {

const MemberRow kWinsockMembers[] = {
    {"BytesReceived", 'g', "Long", 1, ""},
    {"LocalHostName", 'g', "String", 2, ""},
    {"LocalIP", 'g', "String", 3, ""},
    {"LocalPort", 'g', "Long", 4, ""},
    {"LocalPort", 'l', "Long", 4, ""},
    {"Protocol", 'g', "Long", 5, ""},
    {"Protocol", 'l', "Long", 5, ""},
    {"RemoteHost", 'g', "String", 6, ""},
    {"RemoteHost", 'l', "String", 6, ""},
    {"RemoteHostIP", 'g', "String", 7, ""},
    {"RemotePort", 'g', "Long", 8, ""},
    {"RemotePort", 'l', "Long", 8, ""},
    {"SocketHandle", 'g', "Long", 9, ""},
    {"State", 'g', "Long", 10, ""},
    {"Accept", 'm', "Void", 11, "requestID:Long"},
    {"Bind", 'm', "Void", 12, "LocalPort:Variant:o;LocalIP:Variant:o"},
    {"Close", 'm', "Void", 13, ""},
    {"Connect", 'm', "Void", 14, "RemoteHost:Variant:o;RemotePort:Variant:o"},
    {"GetData", 'm', "Void", 15,
     "data:Variant;type:Variant:o;maxLen:Variant:o"},
    {"Listen", 'm', "Void", 16, ""},
    {"PeekData", 'm', "Void", 17,
     "data:Variant;type:Variant:o;maxLen:Variant:o"},
    {"SendData", 'm', "Void", 18, "data:Variant"},
};

const MemberRow kWinsockEvents[] = {
    {"Close", 'm', "Void", 1, ""},
    {"Connect", 'm', "Void", 2, ""},
    {"ConnectionRequest", 'm', "Void", 3, "requestID:Long"},
    {"DataArrival", 'm', "Void", 4, "bytesTotal:Long"},
    {"Error", 'm', "Void", 5,
     "Number:Integer;Description:String;Scode:Long;Source:String;HelpFile:"
     "String;HelpContext:Long;CancelDisplay:Boolean"},
    {"SendComplete", 'm', "Void", 6, ""},
    {"SendProgress", 'm', "Void", 7, "bytesSent:Long;bytesRemaining:Long"},
};

const ControlClass kClasses[] = {
    {"Winsock", kWinsockMembers, sizeof(kWinsockMembers) / sizeof(MemberRow),
     kWinsockEvents, sizeof(kWinsockEvents) / sizeof(MemberRow)},
};

}  // namespace

std::span<const ControlClass> mswinsck_classes() {
    return kClasses;
}

}  // namespace wfc::ui
