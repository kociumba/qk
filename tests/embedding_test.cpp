#include <qk/qk_binary.h>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <print>
#include <string>

using namespace qk::embed;

extern "C" {
extern const unsigned char embed_me_data[];
extern const uint64_t embed_me_size;
}

std::string embedded_string(reinterpret_cast<const char*>(embed_me_data), embed_me_size);

// this does not need anything else since by compiling it already tests if the embedding works
TEST_CASE("Test embedding string data", "[embedding]") {
    SECTION("Embed text file") {
        INFO("embedded data:" << embedded_string);

        REQUIRE(embedded_string == "gabagool");
    }
}