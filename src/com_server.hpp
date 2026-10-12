// COM server support (MP-0005, ADR-0004): which classes of a project a WFC
// server exposes, their ProgIDs and class ids, and per-user registration.
// Internal to the WFC runtime; not part of the public API.

#ifndef WFC_COM_SERVER_HPP
#define WFC_COM_SERVER_HPP

#include <string>
#include <vector>

#include "wfc/evaluator.hpp"

namespace wfc {

struct ServedClass {
    std::string class_name;  // as written in the project
    std::string progid;      // Project.Class
    std::string clsid;       // {GUID}, derived from the ProgID
};

// The classes marked `Attribute VB_Exposed = True` and
// `Attribute VB_Creatable = True`. `project_name` is the project's `Name`.
[[nodiscard]] std::vector<ServedClass> served_classes(
    const std::string& project_name,
    const std::vector<ClassModuleSource>& classes);

// The class id every WFC server derives from a ProgID (stable across runs).
[[nodiscard]] std::string clsid_for_progid(const std::string& progid);

// Writes (or removes) the per-user registration under HKCU\Software\Classes:
// the ProgID, the CLSID and a LocalServer32 command that starts
// `wfc <project> --server`.
[[nodiscard]] bool register_server(const std::vector<ServedClass>& classes,
                                   const std::string& executable,
                                   const std::string& project_path,
                                   std::string& error);
[[nodiscard]] bool unregister_server(const std::vector<ServedClass>& classes,
                                     std::string& error);

}  // namespace wfc

#endif
