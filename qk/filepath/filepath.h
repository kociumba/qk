// these functions and their behaviour is mostly ported from the go standard lib path/filepath
#ifndef FILEPATH_H
#define FILEPATH_H

#ifdef QK_FILEPATH

#include <algorithm>
#include <cctype>
#include <ranges>
#include <sstream>
#include <string>
#include "../api.h"

/// implements a port of the go internal 'filepathlite' package, the implementation is tested
/// against the behaviour of the newest go version, to ensure algorithm parity
///
/// in the future this will also include select functions from 'path/filepath'
///
/// the functions also *mostly* preserve their original doc comments
namespace qk::filepath {

using std::string;

/// A lazybuf is a lazily constructed path buffer.
/// It supports append, reading previously appended bytes,
/// and retrieving the final string. It does not allocate a buffer
/// to hold the output until that output diverges from s.
struct lazybuf {
    string path;
    string buf;
    int w;
    string vol_and_path;
    int vol_len;
    bool using_buf;

    lazybuf(const std::string& p, const std::string& vol_path, int vlen)
        : path(p), buf(""), w(0), vol_and_path(vol_path), vol_len(vlen), using_buf(false) {}

    char index(int i) { return using_buf ? buf[i] : path[i]; }

    void append(char c) {
        if (!using_buf) {
            if (w < (int)path.length() && path[w] == c) {
                w++;
                return;
            }
            buf = path.substr(0, w);
            using_buf = true;
        }

        if (w >= (int)buf.length()) {
            buf.push_back(c);
        } else {
            buf[w] = c;
        }
        w++;
    }

    void prepend(const std::string& prefix) {
        if (!using_buf) {
            buf = path.substr(0, w);
            using_buf = true;
        }

        buf = prefix + buf;
        w += (int)prefix.length();
    }

    std::string _string() {
        if (!using_buf) {
            return vol_and_path.substr(0, vol_len + w);
        }
        return vol_and_path.substr(0, vol_len) + buf.substr(0, w);
    }
};

#ifdef _WIN32
#define SEPARATOR '\\'

inline bool is_path_sep(const char& c) { return c == '/' || c == '\\'; }
inline bool path_has_prefix_fold(const string& s, const string& prefix) {
    if (s.length() < prefix.length()) {
        return false;
    }

    for (int i = 0; i < prefix.length(); i++) {
        if (is_path_sep(prefix[i])) {
            if (!is_path_sep(s[i])) return false;
        } else if (std::toupper(s[i]) != std::toupper(prefix[i])) {
            return false;
        }
    }

    if (s.length() > prefix.length() && !is_path_sep(s[prefix.length()])) {
        return false;
    }

    return true;
}

inline int unc_len(const string& path, int prefix_len) {
    int count = 0;
    for (int i = prefix_len; i < path.length(); i++) {
        if (is_path_sep(path[i])) {
            count++;
            if (count == 2) {
                return i;
            }
        }
    }
    return (int)path.length();
}

inline bool cut_path(const string& path, string* out_before, string* out_after) {
    for (int i = 0; i < path.length(); i++) {
        if (is_path_sep(path[i])) {
            *out_before = path.substr(0, i);
            *out_after = path.substr(i + 1);
            return true;
        }
    }

    *out_before = path;
    *out_after = "";
    return false;
}

inline int volume_name_len(const string& path) {
    if (path.length() >= 2 && path[1] == ':') {
        return 2;
    }

    if (path.empty() || !is_path_sep(path[0])) {
        return 0;
    }

    if (path_has_prefix_fold(path, R"(\\.\UNC)")) {
        return unc_len(path, (int)string(R"(\\.\UNC\)").length());
    }

    if (path_has_prefix_fold(path, R"(\\.)") || path_has_prefix_fold(path, R"(\\?)") ||
        path_has_prefix_fold(path, R"(\??)")) {
        if (path.length() == 3) {
            return 3;
        }
        string before, after;
        bool ok = cut_path(path.substr(4), &before, &after);
        if (!ok) {
            return (int)path.length();
        }
        return int(path.length() - after.length() - 1);
    }

    if (path.length() >= 2 && is_path_sep(path[1])) {
        return unc_len(path, 2);
    }

    return 0;
}

inline void post_clean(lazybuf* out) {
    if (out->vol_len != 0 || out->buf.empty()) {
        return;
    }

    for (char c : out->buf) {
        if (is_path_sep(c)) {
            break;
        }
        if (c == ':') {
            out->prepend(string({'.', SEPARATOR}));
            return;
        }
    }

    if (out->buf.length() >= 3 && is_path_sep(out->buf[0]) && out->buf[1] == '?' &&
        out->buf[2] == '?') {
        out->prepend(string({SEPARATOR, '.'}));
    }
}
#else
#define SEPARATOR '/'

inline bool is_path_sep(const char& c) { return c == '/'; }

inline int volume_name_len(const string& path) { return 0; }

inline void post_clean(lazybuf* out) {}
#endif

/// clean returns the shortest path name equivalent to path
/// by purely lexical processing. It applies the following rules
/// iteratively until no further processing can be done:
///
///  1. Replace multiple [Separator] elements with a single one.
///  2. Eliminate each . path name element (the current directory).
///  3. Eliminate each inner .. path name element (the parent directory)
///     along with the non-.. element that precedes it.
///  4. Eliminate .. elements that begin a rooted path:
///     that is, replace "/.." by "/" at the beginning of a path,
///     assuming Separator is '/'.
///
/// The returned path ends in a slash only if it represents a root directory,
/// such as "/" on Unix or `C:\` on Windows.
///
/// Finally, any occurrences of slash are replaced by Separator.
///
/// If the result of this process is an empty string, clean
/// returns the string ".".
///
/// On Windows, clean does not modify the volume name other than to replace
/// occurrences of "/" with `\`.
/// For example, clean("//host/share/../x") returns `\\host\share\x`.
///
/// See also Rob Pike, “Lexical File Names in Plan 9 or
/// Getting Dot-Dot Right,”
/// https://9p.io/sys/doc/lexnames.html
string clean(const string& path);

/// split splits path immediately following the final [Separator],
/// separating it into a directory and file name component.
/// If there is no Separator in path, split returns an empty dir
/// and file set to path.
/// The returned values have the property that path = dir+file.
void split(const string& path, string* dir, string* file);

/// ext returns the file name extension used by path.
/// The extension is the suffix beginning at the final dot
/// in the final element of path; it is empty if there is
/// no dot.
string ext(const string& path);

/// base returns the last element of path.
/// Trailing path separators are removed before extracting the last element.
/// If the path is empty, base returns ".".
/// If the path consists entirely of separators, Base returns a single separator.
string base(const string& path);

/// dir returns all but the last element of path, typically the path's directory.
/// After dropping the final element, dir calls [clean] on the path and trailing
/// slashes are removed.
/// If the path is empty, dir returns ".".
/// If the path consists entirely of separators, dir returns a single separator.
/// The returned path does not end in a separator unless it is the root directory.
string dir(const string& path);

/// to_slash returns the result of replacing each separator character
/// in path with a slash ('/') character. Multiple separators are
/// replaced by multiple slashes.
string to_slash(const string& path);

/// from_slash returns the result of replacing each slash ('/') character
/// in path with a separator character. Multiple slashes are replaced
/// by multiple separators.
string from_slash(const string& path);

/// volume_name returns leading volume name.
/// Given "C:\foo\bar" it returns "C:" on Windows.
/// Given "\\host\share\foo" it returns "\\host\share".
/// On other platforms it returns "".
string volume_name(const string& path);

}  // namespace qk::filepath

#endif

#endif  // FILEPATH_H
