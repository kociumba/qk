#include <qk/qk_cli.h>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

using namespace qk::cli;

TEST_CASE("CLI parsing") {
    SECTION("Basic flag parsing") {
        CLI cli({
            Flag{.name = "verbose", .short_name = "v", .desc = "Enable verbose output"},
            Flag{
                .name = "output",
                .short_name = "o",
                .desc = "Output file",
                .constraint = {"path:file"}
            },
        });

        const char* argv[] = {"program", "--verbose", "-o", "test.txt"};
        REQUIRE(parse(cli, 4, const_cast<char**>(argv)));
        REQUIRE(cli.has("verbose"));
        REQUIRE(cli.get("output") == "test.txt");
        REQUIRE_FALSE(cli.get("nonexistent").has_value());
    }

    SECTION("Flag with constraints") {
        CLI cli({
            Flag{.name = "count", .desc = "Number of iterations", .constraint = {"1..=10"}},
            Flag{.name = "mode", .desc = "Operation mode", .constraint = {"fast|slow"}},
        });

        SECTION("Valid range constraint") {
            const char* argv[] = {"program", "--count", "5"};
            REQUIRE(parse(cli, 3, const_cast<char**>(argv)));
            REQUIRE(cli.get("count") == "5");
        }

        SECTION("Invalid range constraint") {
            const char* argv[] = {"program", "--count", "15"};
            REQUIRE_FALSE(parse(cli, 3, const_cast<char**>(argv)));
            INFO("Error message for invalid range: " << get_error(cli));
            REQUIRE(!get_error(cli).empty());
        }

        SECTION("Valid enum constraint") {
            const char* argv[] = {"program", "--mode:fast"};
            REQUIRE(parse(cli, 2, const_cast<char**>(argv)));
            REQUIRE(cli.get("mode") == "fast");
        }

        SECTION("Invalid enum constraint") {
            const char* argv[] = {"program", "--mode:medium"};
            REQUIRE_FALSE(parse(cli, 2, const_cast<char**>(argv)));
            INFO("Error message for invalid enum: " << get_error(cli));
            REQUIRE(!get_error(cli).empty());
        }
    }

    SECTION("Array constraint") {
        CLI cli({
            Flag{.name = "numbers", .desc = "List of numbers", .constraint = {"[1..=10]"}},
        });

        const char* argv[] = {"program", "--numbers", "1,5,10"};
        REQUIRE(parse(cli, 3, const_cast<char**>(argv)));
        auto numbers = cli.get_array("numbers");
        REQUIRE(numbers.has_value());
        REQUIRE(numbers->size() == 3);
        REQUIRE((*numbers)[0] == "1");
        REQUIRE((*numbers)[1] == "5");
        REQUIRE((*numbers)[2] == "10");
    }

    SECTION("Command with positional arguments") {
        CLI cli({
            Command{
                .name = "run",
                .Flags = {Flag{.name = "force", .desc = "Force execution"}},
                .positional = {Positional{
                    .name = "input", .desc = "Input file", .constraint = {"path:file"}
                }},
            },
        });

        SECTION("Valid command with positional") {
            const char* argv[] = {"program", "run", "test.txt"};
            REQUIRE(parse(cli, 3, const_cast<char**>(argv)));
            REQUIRE(cli.get("input") == "test.txt");
        }

        SECTION("Missing positional argument") {
            const char* argv[] = {"program", "run"};
            REQUIRE_FALSE(parse(cli, 2, const_cast<char**>(argv)));
            INFO("Error message for missing positional: " << get_error(cli));
            REQUIRE(!get_error(cli).empty());
        }
    }

    SECTION("Required flags") {
        CLI cli({
            Flag{
                .name = "config",
                .desc = "Config file",
                .constraint = {"path:file"},
                .required = true
            },
        });

        SECTION("Missing required flag") {
            const char* argv[] = {"program"};
            REQUIRE_FALSE(parse(cli, 1, const_cast<char**>(argv)));
            INFO("Error message for missing required flag: " << get_error(cli));
            REQUIRE(!get_error(cli).empty());
        }

        SECTION("Provided required flag") {
            const char* argv[] = {"program", "--config", "config.json"};
            REQUIRE(parse(cli, 3, const_cast<char**>(argv)));
            REQUIRE(cli.get("config") == "config.json");
        }
    }

    SECTION("Default values") {
        CLI cli({
            Flag{
                .name = "level", .desc = "Log level", .constraint = {"1..=5"}, .default_value = "3"
            },
        });

        const char* argv[] = {"program"};
        REQUIRE(parse(cli, 1, const_cast<char**>(argv)));
        REQUIRE(cli.get("level") == "3");
    }

    SECTION("Invalid flag") {
        CLI cli({
            Flag{.name = "verbose", .desc = "Enable verbose output"},
        });

        const char* argv[] = {"program", "--invalid"};
        REQUIRE_FALSE(parse(cli, 2, const_cast<char**>(argv)));
        INFO("Error message for invalid flag: " << get_error(cli));
        REQUIRE(!get_error(cli).empty());
    }

    SECTION("Help output") {
        CLI cli({
            Flag{.name = "verbose", .short_name = "v", .desc = "Enable verbose output"},
            Command{
                .name = "run",
                .Flags = {Flag{.name = "force", .desc = "Force execution"}},
                .positional = {Positional{
                    .name = "input", .desc = "Input file", .constraint = {"path:file"}
                }},
            },
        });

        std::string help = get_help(cli);
        REQUIRE(!help.empty());
        REQUIRE(help.find("--verbose, -v\tEnable verbose output") != std::string::npos);
        REQUIRE(help.find("run") != std::string::npos);
        REQUIRE(help.find("--force\tForce execution") != std::string::npos);
        REQUIRE(help.find("input\tInput file") != std::string::npos);
    }

    SECTION("Path constraints") {
        CLI cli({
            Flag{.name = "file", .desc = "Input file", .constraint = {"path:file"}},
            Flag{.name = "dir", .desc = "Input directory", .constraint = {"path:dir"}},
        });

        SECTION("Valid file path") {
            const char* argv[] = {"program", "--file", "cli_tests.cpp"};
            REQUIRE(parse(cli, 3, const_cast<char**>(argv)));
            REQUIRE(cli.get("file") == "cli_tests.cpp");
        }

        SECTION("Invalid file path") {
            const char* argv[] = {"program", "--file", "nonexistent.txt"};
            REQUIRE_FALSE(parse(cli, 3, const_cast<char**>(argv)));
            INFO("Error message for invalid file path: " << get_error(cli));
            REQUIRE(!get_error(cli).empty());
        }
    }
}
