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

auto uncomressed_data = decompress_data(embed_me_data, embed_me_size);
std::string embedded_string(
    reinterpret_cast<char*>(uncomressed_data.data()), uncomressed_data.size()
);

// this does not need anything else since by compiling it already tests if the embedding works
TEST_CASE("Embedding string data", "[embedding]") {
    SECTION("Embed text file") {
        INFO("embedded data(uncompressed):" << embedded_string);
        INFO("embedded data(compressed):" << embed_me_data);

        REQUIRE(embedded_string == "gabagool");
    }
}