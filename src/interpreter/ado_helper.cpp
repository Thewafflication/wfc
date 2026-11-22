// WfcAdo: handles to wfc-adodb connections and recordsets for the built-in
// ADODB classes. Connection operations are 1-9, recordset operations 20-33.

#include "ado_helper.hpp"

#include <wfc/adodb/adodb.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>

namespace wfc::detail {
namespace {

struct Tables {
    std::map<Integer, std::shared_ptr<adodb::Connection>> connections;
    std::map<Integer, std::shared_ptr<adodb::Recordset>> recordsets;
    Integer next{1};
};

Tables& tables() {
    static Tables instance;
    return instance;
}

std::optional<std::int64_t> whole(const Value& value) {
    if (const auto* v = std::get_if<Integer>(&value)) {
        return *v;
    }
    if (const auto* v = std::get_if<Int16>(&value)) {
        return *v;
    }
    if (const auto* v = std::get_if<Byte>(&value)) {
        return *v;
    }
    return std::nullopt;
}

Value from_adodb(const adodb::Value& value) {
    switch (value.index()) {
        case 1: {
            const auto number = std::get<1>(value);
            if (number >= std::numeric_limits<Integer>::min() &&
                number <= std::numeric_limits<Integer>::max()) {
                return Value{static_cast<Integer>(number)};
            }
            return Value{static_cast<double>(number)};
        }
        case 2:
            return Value{std::get<2>(value)};
        case 3:
            return Value{std::get<3>(value)};
        case 4: {
            ArrayValue bytes{};
            for (const auto byte : std::get<4>(value)) {
                bytes.elements.emplace_back(static_cast<Byte>(byte));
            }
            bytes.element_type_index = Value{Byte{}}.index();
            return Value{std::move(bytes)};
        }
        default:
            return Value{Null{}};
    }
}

adodb::Value to_adodb(const Value& value) {
    if (const auto number = whole(value)) {
        return adodb::Value{*number};
    }
    if (const auto* v = std::get_if<bool>(&value)) {
        return adodb::Value{static_cast<std::int64_t>(*v ? -1 : 0)};
    }
    if (const auto* v = std::get_if<std::string>(&value)) {
        return adodb::Value{*v};
    }
    if (std::holds_alternative<Empty>(value) ||
        std::holds_alternative<Null>(value)) {
        return adodb::Value{};
    }
    if (const auto* v = std::get_if<DateValue>(&value)) {
        return adodb::Value{v->serial};
    }
    return adodb::Value{as_double(value)};
}

// The value of a field as the Visual Basic type its ADO data type names.
Value typed(const adodb::Value& stored, const int type) {
    if (stored.index() == 0 || stored.index() >= 3) {
        return from_adodb(stored);
    }
    double number = 0;
    if (const auto* whole_number = std::get_if<std::int64_t>(&stored)) {
        number = static_cast<double>(*whole_number);
    } else if (const auto* real = std::get_if<double>(&stored)) {
        number = *real;
    } else {
        return from_adodb(stored);  // text in a numeric field: as it is
    }
    switch (type) {
        case 2:
            return Value{static_cast<Int16>(number)};
        case 3:
            return Value{static_cast<Integer>(number)};
        case 4:
            return Value{static_cast<float>(number)};
        case 5:
            return Value{number};
        case 6:
            return Value{Currency{std::llround(number * 10000.0)}};
        case 7:
        case 133:
        case 134:
        case 135:
            return Value{DateValue{number}};
        case 11:
            return Value{number != 0.0};
        case 16:
        case 17:
            return Value{static_cast<Byte>(number)};
        default:
            return from_adodb(stored);
    }
}

// A value assigned to a field, in the form the recordset stores for its type.
adodb::Value coerced(const Value& value, const int type) {
    if (std::holds_alternative<Empty>(value) ||
        std::holds_alternative<Null>(value)) {
        return adodb::Value{};
    }
    const bool text_type = type == 200 || type == 201 || type == 202 ||
                           type == 203 || type == 8 || type == 129 ||
                           type == 130;
    if (text_type) {
        if (const auto* v = std::get_if<std::string>(&value)) {
            return adodb::Value{*v};
        }
        if (const auto* v = std::get_if<bool>(&value)) {
            return adodb::Value{std::string(*v ? "True" : "False")};
        }
        char buffer[40];
        if (const auto number = whole(value)) {
            return adodb::Value{std::to_string(*number)};
        }
        std::snprintf(buffer, sizeof(buffer), "%.15g", as_double(value));
        return adodb::Value{std::string(buffer)};
    }
    double number = 0;
    if (const auto* v = std::get_if<std::string>(&value)) {
        char* end = nullptr;
        number = std::strtod(v->c_str(), &end);
        if (end == v->c_str() || *end != '\0') {
            return adodb::Value{*v};  // not a number: kept as text
        }
    } else if (const auto* flag = std::get_if<bool>(&value)) {
        number = *flag ? -1.0 : 0.0;
    } else if (const auto* date = std::get_if<DateValue>(&value)) {
        number = date->serial;
    } else if (const auto whole_number = whole(value)) {
        number = static_cast<double>(*whole_number);
    } else {
        number = as_double(value);
    }
    switch (type) {
        case 2:
        case 3:
        case 16:
        case 17:
        case 20:
        case 11:
            return adodb::Value{static_cast<std::int64_t>(
                type == 11 && number != 0.0 ? -1 : std::llround(number))};
        default:
            return adodb::Value{number};
    }
}

AdoOutcome failed(const adodb::Connection& connection) {
    AdoOutcome outcome;
    if (!connection.errors().empty()) {
        const auto& last = connection.errors().back();
        outcome.error = static_cast<Integer>(last.number);
        outcome.text = last.description;
    } else {
        outcome.error = 5;
        outcome.text = "Invalid procedure call or argument";
    }
    return outcome;
}

AdoOutcome bad(const Integer error, std::string text) {
    AdoOutcome outcome;
    outcome.error = error;
    outcome.text = std::move(text);
    return outcome;
}

AdoOutcome ok(Value value) {
    AdoOutcome outcome;
    outcome.value = std::move(value);
    return outcome;
}

AdoOutcome closed() {
    return bad(3704, "Operation is not allowed when the object is closed.");
}

AdoOutcome no_record() {
    return bad(3021,
               "Either BOF or EOF is True, or the current record has been"
               " deleted. Requested operation requires a current record.");
}

}  // namespace

static AdoOutcome wfc_ado_core(const std::vector<Value>& arguments) {
    if (arguments.size() < 2U) {
        return bad(450,
                   "Wrong number of arguments or invalid property"
                   " assignment");
    }
    const auto op = whole(arguments[0]);
    const auto handle = whole(arguments[1]);
    if (!op || !handle) {
        return bad(13, "Type mismatch");
    }
    const auto text_at = [&](const std::size_t index) {
        const auto* text = index < arguments.size()
                               ? std::get_if<std::string>(&arguments[index])
                               : nullptr;
        return text != nullptr ? *text : std::string{};
    };
    const auto long_at = [&](const std::size_t index) {
        return index < arguments.size() ? whole(arguments[index]).value_or(0)
                                        : 0;
    };
    auto& all = tables();
    const auto key = static_cast<Integer>(*handle);

    if (*op == 1) {  // open a connection: WfcAdo(1, 0, connectionString)
        auto connection = std::make_shared<adodb::Connection>();
        if (!connection->open(text_at(2))) {
            return failed(*connection);
        }
        const Integer id = all.next++;
        all.connections[id] = std::move(connection);
        return ok(Value{id});
    }
    if (*op == 40) {  // a closed, disconnected recordset
        const Integer id = all.next++;
        all.recordsets[id] = adodb::Recordset::create();
        return ok(Value{id});
    }
    if (*op >= 2 && *op <= 9) {
        const auto found = all.connections.find(key);
        if (found == all.connections.end()) {
            return closed();
        }
        adodb::Connection& connection = *found->second;
        connection.clear_errors();
        switch (*op) {
            case 2:  // close
                all.connections.erase(found);
                return ok(Value{Integer{0}});
            case 3: {  // execute: WfcAdo(3, h, sql, params()) -> Array(rs, n)
                std::vector<adodb::Value> parameters;
                if (arguments.size() > 3U) {
                    if (const auto* list =
                            std::get_if<ArrayValue>(&arguments[3])) {
                        for (const auto& element : list->elements) {
                            parameters.push_back(to_adodb(element));
                        }
                    }
                }
                auto result =
                    long_at(4) != 0
                        ? connection.execute_updatable(text_at(2), parameters)
                        : connection.execute(text_at(2), parameters);
                if (!result.ok) {
                    return failed(connection);
                }
                Integer recordset = 0;
                if (result.recordset) {
                    recordset = all.next++;
                    all.recordsets[recordset] = std::move(result.recordset);
                }
                ArrayValue pair{};
                pair.elements.emplace_back(recordset);
                pair.elements.emplace_back(
                    static_cast<Integer>(result.records_affected));
                pair.is_variant_element = true;
                pair.element_type_index = Value{Empty{}}.index();
                return ok(Value{std::move(pair)});
            }
            case 4: {
                const int level = connection.begin_trans();
                return level == 0 ? failed(connection)
                                  : ok(Value{static_cast<Integer>(level)});
            }
            case 5:
                return connection.commit_trans() ? ok(Value{Integer{0}})
                                                 : failed(connection);
            case 6:
                return connection.rollback_trans() ? ok(Value{Integer{0}})
                                                   : failed(connection);
            case 7:
                return ok(
                    Value{static_cast<double>(connection.last_insert_id())});
            case 8:
                return ok(Value{connection.version()});
            default:
                break;
        }
        return bad(450,
                   "Wrong number of arguments or invalid property"
                   " assignment");
    }

    const auto found = all.recordsets.find(key);
    if (found == all.recordsets.end()) {
        return closed();
    }
    adodb::Recordset& set = *found->second;
    const auto field = [&]() -> std::optional<std::size_t> {
        const auto index = long_at(2);
        if (index < 0 || static_cast<std::size_t>(index) >= set.field_count()) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(index);
    };
    switch (*op) {
        case 20:
            return ok(Value{static_cast<Integer>(set.field_count())});
        case 21:
        case 22:
        case 23: {
            const auto index = field();
            if (!index) {
                return bad(3265,
                           "Item cannot be found in the collection"
                           " corresponding to the requested name or ordinal.");
            }
            if (*op == 21) {
                return ok(Value{set.field(*index).name});
            }
            if (*op == 22) {
                return ok(Value{static_cast<Integer>(set.field(*index).type)});
            }
            if (set.bof() || set.eof()) {
                return no_record();
            }
            return ok(typed(set.value(*index), set.field(*index).type));
        }
        case 24:
            set.move_first();
            return ok(Value{Integer{0}});
        case 25:
            set.move_last();
            return ok(Value{Integer{0}});
        case 26:
            return set.move_next() ? ok(Value{Integer{0}}) : no_record();
        case 27:
            return set.move_previous() ? ok(Value{Integer{0}}) : no_record();
        case 28:
            return ok(Value{set.bof()});
        case 29:
            return ok(Value{set.eof()});
        case 30:
            return ok(Value{static_cast<Integer>(set.record_count())});
        case 31:
            return ok(Value{static_cast<Integer>(set.absolute_position())});
        case 32:
            return ok(Value{static_cast<Integer>(set.field_index(text_at(2)))});
        case 34:  // Fields.Append(name, type, size)
            return set.append_field(text_at(2), static_cast<int>(long_at(3)),
                                    static_cast<long>(long_at(4)))
                       ? ok(Value{Integer{0}})
                       : bad(3367,
                             "Object or provider is not capable of"
                             " performing requested operation.");
        case 35:  // Open without a source
            return set.open_empty() ? ok(Value{Integer{0}})
                                    : bad(3705,
                                          "Operation is not allowed when the"
                                          " object is open.");
        case 36:
        case 37:
        case 38:
        case 39:
        case 41: {
            if (set.state() == adodb::adStateClosed) {
                return closed();
            }
            if (!set.editable()) {
                return bad(3251,
                           "Current Recordset does not support"
                           " updating. This may be a limitation of"
                           " the provider, or of the selected"
                           " locktype.");
            }
            bool done = false;
            if (*op == 36) {
                done = set.add_new();
            } else if (*op == 37) {
                done = set.update() || true;  // nothing pending is fine
            } else if (*op == 38) {
                if (set.bof() || set.eof()) {
                    return no_record();
                }
                done = set.delete_current();
            } else if (*op == 39) {
                done = set.cancel_update() || true;
            } else {
                const auto index = field();
                if (!index) {
                    return bad(3265,
                               "Item cannot be found in the"
                               " collection corresponding to the"
                               " requested name or ordinal.");
                }
                if (set.bof() || set.eof()) {
                    return no_record();
                }
                done = arguments.size() > 3U &&
                       set.set_value(*index, coerced(arguments[3],
                                                     set.field(*index).type));
            }
            return done ? ok(Value{Integer{0}})
                        : bad(3001,
                              "Arguments are of the wrong type, are"
                              " out of acceptable range, or are in"
                              " conflict with one another.");
        }
        case 42:
            if (set.state() == adodb::adStateClosed) {
                return closed();
            }
            return set.set_filter(text_at(2))
                       ? ok(Value{Integer{0}})
                       : bad(3001,
                             "Arguments are of the wrong type, are"
                             " out of acceptable range, or are in"
                             " conflict with one another.");
        case 43:  // Find(criteria, skip, direction)
            if (set.state() == adodb::adStateClosed) {
                return closed();
            }
            return set.find(text_at(2), static_cast<long>(long_at(3)),
                            long_at(4) != 2)
                       ? ok(Value{Integer{0}})
                       : bad(3001,
                             "Arguments are of the wrong type, are"
                             " out of acceptable range, or are in"
                             " conflict with one another.");
        case 44:
            return ok(Value{set.filter()});
        case 45:
            return ok(Value{static_cast<Integer>(set.state())});
        case 46:  // Sort = "field [ASC|DESC], ..."
            if (set.state() == adodb::adStateClosed) {
                return closed();
            }
            return set.set_sort(text_at(2))
                       ? ok(Value{Integer{0}})
                       : bad(3265,
                             "Item cannot be found in the collection"
                             " corresponding to the requested name or"
                             " ordinal.");
        case 47:
            return ok(Value{set.sort()});
        case 50:  // Bookmark
            if (set.state() == adodb::adStateClosed) {
                return closed();
            }
            return ok(Value{static_cast<Integer>(set.bookmark())});
        case 51:  // Bookmark = mark
            if (set.state() == adodb::adStateClosed) {
                return closed();
            }
            return set.goto_bookmark(static_cast<long>(long_at(2)))
                       ? ok(Value{Integer{0}})
                       : bad(3001,
                             "Arguments are of the wrong type, are out"
                             " of acceptable range, or are in conflict"
                             " with one another.");
        case 48:    // Field.Type
        case 49: {  // Field.DefinedSize
            const auto index = field();
            if (!index) {
                return bad(3265,
                           "Item cannot be found in the collection"
                           " corresponding to the requested name or"
                           " ordinal.");
            }
            const auto& info = set.field(*index);
            return ok(Value{static_cast<Integer>(
                *op == 48 ? info.type : info.defined_size)});
        }
        case 33:
            all.recordsets.erase(found);
            return ok(Value{Integer{0}});
        default:
            break;
    }
    return bad(450,
               "Wrong number of arguments or invalid property"
               " assignment");
}

// A write to a table behind an updatable recordset can fail in any call that
// flushes a pending row; the error surfaces as that call's error.
AdoOutcome wfc_ado(const std::vector<Value>& arguments) {
    auto outcome = wfc_ado_core(arguments);
    if (arguments.size() >= 2U) {
        const auto op = whole(arguments[0]);
        const auto handle = whole(arguments[1]);
        if (op && handle && *op >= 20 && *op != 40) {
            auto& all = tables();
            const auto found =
                all.recordsets.find(static_cast<Integer>(*handle));
            adodb::Error error;
            if (found != all.recordsets.end() &&
                found->second->take_error(error)) {
                return bad(static_cast<Integer>(error.number),
                           error.description);
            }
        }
    }
    return outcome;
}

}  // namespace wfc::detail
