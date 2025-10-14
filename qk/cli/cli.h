#ifndef CLI_H
#define CLI_H

#ifdef QK_CLI

#include <charconv>
#include <concepts>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include "../api.h"

#ifdef QK_HAS_REFLECTION
#include <reflect>
#endif

namespace qk::cli {

using sv = std::string_view;

QK_API inline std::vector<sv> split(sv s, char delim) {
    std::vector<sv> result;
    size_t start = 0;
    while (true) {
        size_t pos = s.find(delim, start);
        if (pos == sv::npos) {
            result.emplace_back(s.substr(start));
            break;
        }
        result.emplace_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return result;
}

bool parse_range_part(sv s, double& out);

// validation
//
// general idea for the validation is to have a string notation of constraints, that is then
// enforced on parsed values e.g: constraint = "1..10", value has to be within the range 1 to 10
//
// notation:
//      - boolean values are represented by empty strings, they either exist or not
//      - numeric ranges are represented by range notation like: 1..=10 or 69..<420 (concept for
//      more specific notation: int:10..=99, float:3.7..<6.9), range notation from odin, ..= for
//      inclusive, ..< for half open
//      - string values are represented using ... notation, and accept any valid string values
//      - enum-like values are represented by strings separated using | e.g: speed|size, that
//      are compared against the value
//      - path value needs to be an existent path on the system, notation not yet finalised
//      (concepts like path:file or path:dir for specifics)
//      - all the above can be wrapped in [constraint] to signify an array of comma separated
//      values that need to fit that constraint
//
// for flexibility we allow more than one constraint to be applied to a value, but constraints
// will have to take some logical priority when validating, so for example applying both
// path:file and
// ... to a value will have no effect since a path is always more specific than a string value

// this needs to be comp time, so that constraints can report errors using static_assert, I hope
// this will be possible
QK_API bool is_valid_constraint(const sv& c);

QK_API bool is_range_constraint(const sv& c);
QK_API bool is_enum_constraint(const sv& c);
QK_API bool is_string_constraint(const sv& c);
QK_API bool is_path_constraint(const sv& c);
QK_API bool is_array_constraint(const sv& c);

#ifdef QK_HAS_REFLECTION
template <typename T>
    requires std::is_enum_v<T>
QK_API std::string enum_constraint() {
    std::stringstream ss;
    for (auto&& [value, name] : reflect::enumerators<T>) {
        ss << name << "|";
    }

    auto s = ss.str();
    if (!s.empty()) s.pop_back();
    return s;
}
#endif

QK_API bool constrained(const sv& constraint, const std::string& value);
QK_API bool constrained(const std::vector<sv>& constraints, const std::string& value);

// cli structure
//
// core idea is to build the structure using initializer lists and struct instances, then parse the
// cli input based on that definition, generating help printables and error messages in the process
//
// flags are defined and validated using the above constraint system, how the user retrieves the
// parsed values is undecided yet
//
// the goal is to provide an api that can be used like this:
//
//      if (!parse(cli, arg, argv) {
//          std::println("{}", get_error(cli));
//          std::println("{}", get_help(cli));
//      }

struct Flag;
struct Group;
struct Command;
struct Positional;

using CliElement = std::variant<Group, Command>;

struct QK_API Group {
    std::string_view name;
    std::vector<CliElement> elements;
};

struct QK_API Command {
    std::string_view name;
    std::vector<Flag> Flags;
    std::vector<Positional> positional;
};

struct Positional {
    sv name;
    sv desc;
    std::vector<sv> constraint;
    bool required = true;
};

struct QK_API Flag {
    sv name;
    sv short_name;
    sv desc;
    std::vector<sv> constraint;
    bool required = false;
    sv default_value;
};

// allows global flags
using TopLevelElement = std::variant<Group, Command, Flag>;

struct QK_API ParseResult {
    std::unordered_map<std::string, std::string> values;
    std::unordered_map<std::string, std::vector<std::string>> array_values;
    std::unordered_set<std::string> flags_present;

    [[nodiscard]] bool has(sv flag_name) const;

    [[nodiscard]] std::optional<std::string> get(sv flag_name) const;
    [[nodiscard]] std::optional<std::vector<std::string>> get_array(sv flag_name) const;

    template <typename T>
    std::optional<T> get_as(sv flag_name) const {
        auto val = get(flag_name);
        if (!val) return std::nullopt;
        T out{};
        std::stringstream ss(*val);
        ss >> out;
        if (ss.fail()) return std::nullopt;
        return out;
    }
};

struct QK_API CLI {
    std::vector<TopLevelElement> structure;
    ParseResult result;
    std::string error_message;

    CLI(const std::initializer_list<TopLevelElement>& structure) : structure(structure) {}

    // simple method forwarding to make it simpler to use
    [[nodiscard]] bool has(sv flag_name) const { return result.has(flag_name); }

    [[nodiscard]] std::optional<std::string> get(sv flag_name) const {
        return result.get(flag_name);
    }
    [[nodiscard]] std::optional<std::vector<std::string>> get_array(sv flag_name) const {
        return result.get_array(flag_name);
    }

    template <typename T>
    std::optional<T> get_as(sv flag_name) const {
        return result.get_as<T>(flag_name);
    }
};

QK_API bool parse(CLI& cli, int argc, char* argv[]);

QK_API std::string get_error(CLI& cli);

QK_API std::string get_help(CLI& cli);

}  // namespace qk::cli

#endif

#endif  // CLI_H
