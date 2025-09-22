#include <qk/qk_filepath.h>
#include <catch2/catch_test_macros.hpp>

using namespace qk::filepath;

TEST_CASE("Filepath clean function", "[filepath]") {
    SECTION("Basic path cleaning") {
        REQUIRE(clean("path/to/../file") == from_slash("path/file"));
        REQUIRE(clean("path/./file") == from_slash("path/file"));
        REQUIRE(
            clean(string("path") + SEPARATOR + "" + SEPARATOR + "file") == from_slash("path/file")
        );
    }

    SECTION("Rooted paths") {
        REQUIRE(clean("/path/to/../file") == from_slash("/path/file"));
        REQUIRE(clean("/.") == from_slash("/"));
        REQUIRE(clean("/..") == from_slash("/"));
    }

    SECTION("Additional edge cases") {
        REQUIRE(clean(from_slash("path\\to/file")) == from_slash("path/to/file"));
        REQUIRE(clean("a/b/../../c") == from_slash("c"));
        REQUIRE(clean("C:foo/..") == from_slash("C:."));
        REQUIRE(clean("\\\\?\\C:\\foo\\..") == from_slash("\\\\?\\C:\\"));
    }

    SECTION("Empty or dot paths") {
        REQUIRE(clean("") == ".");
        REQUIRE(clean(".") == ".");
        REQUIRE(clean("..") == "..");
    }

#ifdef _WIN32
    SECTION("Windows volume name handling") {
        REQUIRE(clean("C:\\path\\to\\..\\file") == "C:\\path\\file");
        REQUIRE(clean("C:\\") == "C:\\");
        REQUIRE(clean("\\\\server\\share\\path\\..\\file") == "\\\\server\\share\\file");
    }
#endif
}

TEST_CASE("Filepath split function", "[filepath]") {
    string dir, file;

    SECTION("Basic path splitting") {
        split(from_slash("path/to/file.txt"), &dir, &file);
        REQUIRE(dir == from_slash("path/to/"));
        REQUIRE(file == "file.txt");
    }

    SECTION("Rooted path splitting") {
        split(from_slash("/file.txt"), &dir, &file);
        REQUIRE(dir == from_slash("/"));
        REQUIRE(file == "file.txt");
    }

    SECTION("No directory") {
        split("file.txt", &dir, &file);
        REQUIRE(dir == "");
        REQUIRE(file == "file.txt");
    }

    SECTION("Edge cases") {
        split("", &dir, &file);
        REQUIRE(dir == "");
        REQUIRE(file == "");
        split(from_slash("path/"), &dir, &file);
        REQUIRE(dir == from_slash("path/"));
        REQUIRE(file == "");
        split("C:", &dir, &file);
        REQUIRE(dir == "C:");
        REQUIRE(file == "");
        split("\\\\server\\share", &dir, &file);
        REQUIRE(dir == "\\\\server\\share");
        REQUIRE(file == "");
    }

#ifdef _WIN32
    SECTION("Windows volume name") {
        split("C:\\path\\file.txt", &dir, &file);
        REQUIRE(dir == "C:\\path\\");
        REQUIRE(file == "file.txt");
    }
#endif
}

TEST_CASE("Filepath ext function", "[filepath]") {
    SECTION("Basic extension extraction") {
        REQUIRE(ext("file.txt") == ".txt");
        REQUIRE(ext("file.tar.gz") == ".gz");
        REQUIRE(ext("file") == "");
    }

    SECTION("Additional cases") {
        REQUIRE(ext(".gitignore") == ".gitignore");
        REQUIRE(ext("file..txt") == ".txt");
        REQUIRE(ext(from_slash("path/.")) == ".");
    }

    SECTION("Path with directories") { REQUIRE(ext(from_slash("path/to/file.txt")) == ".txt"); }

    SECTION("No extension") { REQUIRE(ext(from_slash("path/to/file")) == ""); }
}

TEST_CASE("Filepath base function", "[filepath]") {
    SECTION("Basic base extraction") {
        REQUIRE(base(from_slash("path/to/file.txt")) == "file.txt");
        REQUIRE(base("file.txt") == "file.txt");
        REQUIRE(base("") == ".");
    }

    SECTION("Rooted paths") {
        REQUIRE(base(from_slash("/path/to/file.txt")) == "file.txt");
#ifdef _WIN32
        REQUIRE(base("C:\\path\\to\\file.txt") == "file.txt");
#endif
    }

    SECTION("Trailing separators") { REQUIRE(base(from_slash("path/to//")) == "to"); }

#ifdef _WIN32
    SECTION("Windows edge cases") { REQUIRE(base("C:") == string({SEPARATOR})); }
#endif
}

TEST_CASE("Filepath dir function", "[filepath]") {
    SECTION("Basic directory extraction") {
        REQUIRE(dir(from_slash("path/to/file.txt")) == from_slash("path/to"));
        REQUIRE(dir("file.txt") == ".");
    }

    SECTION("Rooted paths") {
        REQUIRE(dir(from_slash("/path/to/file.txt")) == from_slash("/path/to"));
#ifdef _WIN32
        REQUIRE(dir("C:\\path\\to\\file.txt") == "C:\\path\\to");
#endif
    }

    SECTION("Edge cases") {
        REQUIRE(dir("") == ".");
        REQUIRE(dir("C:") == "C:.");
        REQUIRE(dir("\\\\server\\share\\file") == "\\\\server\\share\\");
        REQUIRE(dir(from_slash("path/")) == from_slash("path"));
    }
}

TEST_CASE("Filepath slash conversion", "[filepath]") {
    SECTION("To slash") {
        REQUIRE(to_slash(string("path") + SEPARATOR + "to" + SEPARATOR + "file") == "path/to/file");
        REQUIRE(to_slash("path/to/file") == "path/to/file");
        REQUIRE(to_slash("") == "");
        REQUIRE(to_slash(string(1, SEPARATOR)) == "/");
    }

    SECTION("From slash") {
        REQUIRE(
            from_slash("path/to/file") == string("path") + SEPARATOR + "to" + SEPARATOR + "file"
        );
        REQUIRE(from_slash("") == "");
    }
}

TEST_CASE("Filepath volume name", "[filepath]") {
    SECTION("Non-Windows volume name") { REQUIRE(volume_name(from_slash("path/to/file")) == ""); }

#ifdef _WIN32
    SECTION("Windows volume name") {
        REQUIRE(volume_name("C:\\path\\to\\file") == "C:");
        REQUIRE(volume_name("\\\\server\\share\\file") == "\\\\server\\share");
    }

    SECTION("Windows edge cases") {
        REQUIRE(volume_name("\\\\?\\C:\\file") == "\\\\?\\C:");
        REQUIRE(volume_name("\\\\server") == "\\\\server");
        REQUIRE(volume_name("C") == "");
    }
#endif
}