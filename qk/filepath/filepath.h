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

namespace qk::filepath {

using std::string;

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

string clean(const string& path);
void split(const string& path, string* dir, string* file);
string ext(const string& path);
string base(const string& path);
string dir(const string& path);
string to_slash(const string& path);
string from_slash(const string& path);
string volume_name(const string& path);

}  // namespace qk::filepath

#endif

#endif  // FILEPATH_H
