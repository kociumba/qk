#include "cli.h"

#ifdef QK_CLI

namespace qk::cli {

bool parse_range_part(sv s, double& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc();
}

bool is_string_constraint(const sv& c) { return c == "..."; }

bool is_enum_constraint(const sv& c) { return c.find('|') != sv::npos || !c.empty(); }

bool is_range_constraint(const sv& c) { return c.find("..") != sv::npos; }

bool is_array_constraint(const sv& c) {
    return c.size() >= 2 && c.front() == '[' && c.back() == ']';
}

bool is_path_constraint(const sv& c) { return c.starts_with("path"); }

bool is_valid_constraint(const sv& c) {
    return c.empty() || is_string_constraint(c) || is_enum_constraint(c) ||
           is_range_constraint(c) || is_path_constraint(c) || is_array_constraint(c);
}

bool constrained(const sv& constraint, const std::string& value) {
    if (constraint.empty()) return value.empty();  // boolean values exist or not, no value

    if (is_array_constraint(constraint)) {
        auto real_constraint = constraint.substr(1, constraint.size() - 2);
        std::vector values = split(value, ',');

        for (auto& val : values) {
            if (!constrained(real_constraint, std::string(val))) {
                return false;
            }
        }

        return true;
    }

    if (is_path_constraint(constraint)) {
        namespace fs = std::filesystem;
        auto parts = split(constraint, ':');
        if (parts.empty() || parts[0] != "path") return false;

        std::unordered_set<std::string> mods;
        for (size_t i = 1; i < parts.size(); ++i) {
            std::string mod(parts[i]);
            mods.insert(mod);
        }

        bool require_exists = mods.contains("exists");
        bool expect_file = mods.contains("file");
        bool expect_dir = mods.contains("dir");

        if (expect_file && expect_dir) return false;
        if (value.empty()) return false;

        fs::path p(value);
        bool path_exists = fs::exists(p);
        if (require_exists && !path_exists) return false;

        bool is_actual_file = path_exists && fs::is_regular_file(p);
        bool is_actual_dir = path_exists && fs::is_directory(p);

        if (path_exists) {
            if (expect_file && !is_actual_file) return false;
            if (expect_dir && !is_actual_dir) return false;
        }

        if (!path_exists) {
            if (expect_file || expect_dir) {
                auto parent = p.parent_path();
                bool parent_ok = parent.empty() || (fs::exists(parent) && fs::is_directory(parent));
                if (!parent_ok) return false;
            }
            if (expect_file) {
                if (!p.has_filename()) return false;
            }
        }

        return true;
    }

    if (is_range_constraint(constraint)) {
        bool inclusive = constraint.find("..=") != sv::npos;
        bool half_open = constraint.find("..<") != sv::npos;

        if (!inclusive && !half_open) return false;

        auto type_split = split(constraint, ':');
        sv range_str = type_split.size() == 2 ? type_split[1] : type_split[0];
        bool is_float = type_split.size() == 2 && type_split[0] == "float";
        (void)is_float;

        size_t op_pos = sv::npos;
        size_t op_len = 0;
        if (inclusive) {
            op_pos = range_str.find("..=");
            op_len = 3;
        } else if (half_open) {
            op_pos = range_str.find("..<");
            op_len = 3;
        }

        if (op_pos == sv::npos) return false;

        sv low_str = range_str.substr(0, op_pos);
        sv high_str = range_str.substr(op_pos + op_len);

        double low, high, val;
        if (!parse_range_part(low_str, low)) return false;
        if (!parse_range_part(high_str, high)) return false;
        if (!parse_range_part(value, val)) return false;

        if (inclusive) return val >= low && val <= high;
        if (half_open) return val >= low && val < high;
        return val >= low && val <= high;
    }

    if (is_enum_constraint(constraint)) {
        auto allowed = split(constraint, '|');
        for (auto& a : allowed) {
            if (value == a) return true;
        }
        return false;
    }

    if (is_string_constraint(constraint)) return !value.empty();

    return false;
}

bool constrained(const std::vector<sv>& constraints, const std::string& value) {
    for (auto& c : constraints)
        if (constrained(c, value)) return true;
    return false;
}

bool ParseResult::has(sv flag_name) const { return flags_present.contains(std::string(flag_name)); }

std::optional<std::string> ParseResult::get(sv flag_name) const {
    auto it = values.find(std::string(flag_name));
    if (it == values.end()) return std::nullopt;
    return it->second;
}

std::optional<std::vector<std::string>> ParseResult::get_array(sv flag_name) const {
    auto it = array_values.find(std::string(flag_name));
    if (it == array_values.end()) return std::nullopt;
    return it->second;
}

bool is_potential_command(const sv& tok, const std::unordered_map<sv, Command*>& commands) {
    return commands.contains(tok);
}

std::string format_constraint_hint(const std::vector<sv>& constraints) {
    if (constraints.empty()) return "";

    std::stringstream ss;
    ss << " (expected: ";
    bool first = true;
    for (auto& c : constraints) {
        if (!first) ss << " OR ";

        if (is_range_constraint(c)) {
            ss << "number in range " << c;
        } else if (is_enum_constraint(c)) {
            ss << "one of {" << c << "}";
        } else if (is_string_constraint(c)) {
            ss << "non-empty string";
        } else if (is_path_constraint(c)) {
            if (c == "path:file")
                ss << "file path";
            else if (c == "path:dir")
                ss << "directory path";
            else if (c.ends_with(":exists"))
                ss << "existing path";
            else
                ss << "path";
        } else if (is_array_constraint(c)) {
            auto inner = c.substr(1, c.size() - 2);
            ss << "comma-separated list of " << format_constraint_hint({inner});
        } else {
            ss << c;
        }
        first = false;
    }
    ss << ")";
    return ss.str();
}

bool parse(CLI& cli, int argc, char* argv[]) {
    if (argc <= 1) {
        cli.error_message = "No arguments provided. Use --help to see available options.";
        return false;
    }

    std::unordered_map<std::string_view, Flag*> global_flags;
    std::unordered_map<std::string_view, Command*> commands;

    for (auto& elem : cli.structure) {
        if (auto* f = std::get_if<Flag>(&elem)) {
            global_flags[f->name] = f;
            if (!f->short_name.empty()) global_flags[f->short_name] = f;
        } else if (auto* c = std::get_if<Command>(&elem)) {
            commands[c->name] = c;
        } else if (auto* g = std::get_if<Group>(&elem)) {
            for (auto& inner : g->elements)
                if (auto* c = std::get_if<Command>(&inner)) commands[c->name] = c;
        }
    }

    Command* current_cmd = nullptr;
    size_t positional_index = 0;

    for (int i = 1; i < argc; ++i) {
        std::string_view tok = argv[i];

        if (tok.starts_with("--") || tok.starts_with("-")) {
            bool is_long_flag = tok.starts_with("--");
            auto name = is_long_flag ? tok.substr(2) : tok.substr(1);
            std::string_view flag_name = name;
            std::string value;

            size_t sep_pos = name.find_first_of(":=");
            if (sep_pos != std::string_view::npos) {
                flag_name = name.substr(0, sep_pos);
                value = name.substr(sep_pos + 1);
            }

            Flag* flag = nullptr;
            if (current_cmd) {
                for (auto& cf : current_cmd->Flags) {
                    if (cf.name == flag_name || cf.short_name == flag_name) {
                        flag = &cf;
                        break;
                    }
                }
            }

            if (!flag) {
                auto it = global_flags.find(flag_name);
                if (it != global_flags.end()) {
                    flag = it->second;
                }
            }

            if (!flag) {
                std::stringstream err;
                err << "Unknown flag: " << (is_long_flag ? "--" : "-") << flag_name;
                std::vector<std::string> suggestions;
                auto check_similarity = [&](Flag* f) {
                    if (!f) return;
                    std::string fname(f->name);
                    std::string fshort(f->short_name);
                    std::string search(flag_name);

                    if (fname.find(search) != std::string::npos ||
                        search.find(fname) != std::string::npos) {
                        suggestions.push_back("--" + fname);
                    } else if (!fshort.empty() && fshort == search) {
                        suggestions.push_back("--" + fname + " (-" + fshort + ")");
                    }
                };
                for (auto& [_, f] : global_flags)
                    check_similarity(f);
                if (current_cmd) {
                    for (auto& cf : current_cmd->Flags) {
                        check_similarity(&cf);
                    }
                }
                if (!suggestions.empty()) {
                    err << "\n  Did you mean: " << suggestions[0];
                    for (size_t i = 1; i < std::min(suggestions.size(), size_t(3)); ++i) {
                        err << ", " << suggestions[i];
                    }
                    err << "?";
                }
                if (current_cmd) {
                    err << "\n  Context: inside command '" << current_cmd->name << "'";
                }

                cli.error_message = err.str();
                return false;
            }

            if (value.empty() && !flag->constraint.empty()) {
                if (i + 1 >= argc) {
                    cli.error_message = "missing value for flag " +
                                        std::string(is_long_flag ? "--" : "-") +
                                        std::string(flag_name);
                    return false;
                }

                std::string_view next_tok = argv[i + 1];
                if (next_tok.starts_with('-')) {
                    cli.error_message = "missing value for flag " +
                                        std::string(is_long_flag ? "--" : "-") +
                                        std::string(flag_name) + " (next arg is another flag)";
                    return false;
                }

                if (current_cmd == nullptr && is_potential_command(next_tok, commands)) {
                    cli.error_message = "flag " + std::string(is_long_flag ? "--" : "-") +
                                        std::string(flag_name) +
                                        " requires a value, but next arg '" +
                                        std::string(next_tok) + "' looks like a command. Use --" +
                                        std::string(flag_name) + "=value before the command.";
                    return false;
                }

                value = argv[++i];
            } else if (value.empty() && flag->constraint.empty()) {
                // Boolean flag
            } else if (!value.empty() && flag->constraint.empty()) {
                cli.error_message = "flag " + std::string(is_long_flag ? "--" : "-") +
                                    std::string(flag_name) + " does not take a value";
                return false;
            }

            if (!value.empty() && !constrained(flag->constraint, value)) {
                cli.error_message = "invalid value '" + std::string(value) + "' for flag " +
                                    std::string(is_long_flag ? "--" : "-") +
                                    std::string(flag_name) + " — does not satisfy constraint" +
                                    format_constraint_hint(flag->constraint);
                return false;
            }

            if (!value.empty()) {
                if (!flag->constraint.empty() && is_array_constraint(flag->constraint[0])) {
                    auto values = split(sv(value), ',');
                    std::vector<std::string> str_values;
                    for (auto v : values) {
                        str_values.emplace_back(v);
                    }
                    cli.result.array_values[std::string(flag->name)] = str_values;
                }
                cli.result.values[std::string(flag->name)] = value;
            }
            cli.result.flags_present.insert(std::string(flag->name));
        } else {
            if (current_cmd == nullptr) {
                auto it = commands.find(tok);
                if (it != commands.end()) {
                    current_cmd = it->second;
                    positional_index = 0;
                    continue;
                } else {
                    cli.error_message = "unexpected argument before command: " + std::string(tok);
                    return false;
                }
            } else {
                if (positional_index >= current_cmd->positional.size()) {
                    cli.error_message = "too many positional args for command " +
                                        std::string(current_cmd->name) + " (expected " +
                                        std::to_string(current_cmd->positional.size()) + ")";
                    return false;
                }

                auto& p = current_cmd->positional[positional_index++];
                if (!constrained(p.constraint, std::string(tok))) {
                    cli.error_message = "invalid value '" + std::string(tok) + "' for positional " +
                                        std::string(p.name) + " — does not satisfy constraint" +
                                        format_constraint_hint(p.constraint);
                    return false;
                }

                cli.result.values[std::string(p.name)] = std::string(tok);
            }
        }
    }

    auto apply_defaults = [&](const Flag& f) {
        if (f.default_value.empty()) return;
        if (cli.result.has(f.name)) return;

        std::string val(f.default_value);
        if (!f.constraint.empty() && !constrained(f.constraint, val)) {
            cli.error_message = "invalid default value '" + val + "' for flag --" +
                                std::string(f.name) + format_constraint_hint(f.constraint);
            return;
        }

        cli.result.values[std::string(f.name)] = val;
    };

    for (auto& elem : cli.structure) {
        if (auto* f = std::get_if<Flag>(&elem))
            apply_defaults(*f);
        else if (auto* c = std::get_if<Command>(&elem)) {
            for (auto& cf : c->Flags)
                apply_defaults(cf);
        } else if (auto* g = std::get_if<Group>(&elem)) {
            for (auto& inner : g->elements) {
                if (auto* c = std::get_if<Command>(&inner))
                    for (auto& cf : c->Flags)
                        apply_defaults(cf);
            }
        }
    }

    for (auto& elem : cli.structure)
        if (auto* f = std::get_if<Flag>(&elem))
            if (f->required && !cli.result.has(f->name)) {
                cli.error_message = "missing required flag --" + std::string(f->name);
                return false;
            }

    if (current_cmd)
        for (auto& p : current_cmd->positional)
            if (p.required && !cli.result.has(p.name)) {
                cli.error_message = "missing required positional " + std::string(p.name);
                return false;
            }

    return true;
}

std::string get_error(CLI& cli) { return cli.error_message.empty() ? "" : cli.error_message; }

std::string get_help(CLI& cli) {
    std::stringstream ss;
    ss << "Usage:\n";
    for (auto& elem : cli.structure) {
        if (auto* f = std::get_if<Flag>(&elem)) {
            ss << "  --" << f->name;
            if (!f->short_name.empty()) ss << ", -" << f->short_name;
            bool use_colon = !f->constraint.empty() && is_enum_constraint(f->constraint[0]);
            if (!f->constraint.empty()) {
                ss << (use_colon ? ":" : " ");
                bool first = true;
                for (auto& c : f->constraint) {
                    if (!first) ss << "|";
                    ss << c;
                    first = false;
                }
            }
            ss << "\t" << f->desc;
            if (f->required) ss << " (required)";
            ss << "\n";
        } else if (auto* c = std::get_if<Command>(&elem)) {
            ss << "  " << c->name << "\n";
            for (auto& f2 : c->Flags) {
                ss << "    --" << f2.name;
                if (!f2.short_name.empty()) ss << ", -" << f2.short_name;
                bool use_colon = !f2.constraint.empty() && is_enum_constraint(f2.constraint[0]);
                if (!f2.constraint.empty()) {
                    ss << (use_colon ? ":" : " ");
                    bool first = true;
                    for (auto& c : f2.constraint) {
                        if (!first) ss << "|";
                        ss << c;
                        first = false;
                    }
                }
                ss << "\t" << f2.desc;
                if (f2.required) ss << " (required)";
                ss << "\n";
            }
            for (auto& p : c->positional) {
                ss << "    " << p.name << "\t" << p.desc;
                if (p.required) ss << " (required)";
                ss << "\n";
            }
        } else if (auto* g = std::get_if<Group>(&elem)) {
            ss << "  [" << g->name << "]\n";
            for (auto& inner : g->elements) {
                if (auto* c = std::get_if<Command>(&inner)) {
                    ss << "    " << c->name << "\n";
                }
            }
        }
    }
    return ss.str();
}

}  // namespace qk::cli

#endif
