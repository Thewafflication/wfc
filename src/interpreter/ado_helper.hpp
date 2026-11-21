// The native helper behind the built-in ADODB classes (WfcAdo): the VB source
// of WfcAdoConnection and WfcAdoRecordset calls it, and it calls wfc-adodb
// (ADODB over SQLite). Internal to the WFC evaluator.

#ifndef WFC_INTERPRETER_ADO_HELPER_HPP
#define WFC_INTERPRETER_ADO_HELPER_HPP

#include "vb_value.hpp"

#include <optional>
#include <string>
#include <vector>

namespace wfc::detail {

struct AdoOutcome {
    std::optional<Value> value;
    Integer error{0};  // a VB error number to raise when not 0
    std::string text;
};

// WfcAdo(op, handle, a, b): see ado_helper.cpp for the operations.
[[nodiscard]] AdoOutcome wfc_ado(const std::vector<Value>& arguments);

}  // namespace wfc::detail

#endif  // WFC_INTERPRETER_ADO_HELPER_HPP
