#include "filepath.h"

#ifdef QK_FILEPATH

namespace qk::filepath {

string clean(const string& path) {
    auto original_path = path;
    auto vol_len = volume_name_len(path);
    auto path_c = path.substr(vol_len);

    if (path_c == "") {
        if (vol_len > 1 && is_path_sep(original_path[0]) && is_path_sep(original_path[1])) {
            return from_slash(original_path);
        }
        return original_path + ".";
    }

    auto rooted = is_path_sep(path_c[0]);
    auto n = path_c.length();
    std::stringstream out;
    int r = 0, w = 0, dotdot = 0;
    if (rooted) {
        out << SEPARATOR;
        r = 1, w = 0, dotdot = 1;
    }

    while (r < n) {
        if (is_path_sep(path_c[r])) {
            r++;
        } else if (path_c[r] == '.' && (r + 1 == n || is_path_sep(path_c[r + 1]))) {
            r++;
        } else if (path_c[r] == '.' && path_c[r + 1] == '.' &&
                   (r + 2 == n || is_path_sep(path_c[r + 2]))) {
            r += 2;
            if (w > dotdot) {
                string current = out.str();
                while (w > dotdot && !is_path_sep(current[w - 1])) {
                    w--;
                }
                if (w > dotdot) {
                    w--;
                }
                out.str("");
                out << current.substr(0, w);
            } else if (!rooted) {
                if (w > 0) {
                    out << SEPARATOR;
                    w++;
                }
                out << "..";
                w += 2;
                dotdot = w;
            }
        } else {
            if ((rooted && w != 1) || (!rooted && w != 0)) {
                out << SEPARATOR;
                w++;
            }
            for (; r < n && !is_path_sep(path_c[r]); r++) {
                out << path_c[r];
                w++;
            }
        }
    }

    if (w == 0) {
        out << '.';
        w++;
    }

    post_clean(out, vol_len);
    return from_slash(out.str());
}

void split(const string& path, string* dir, string* file) {
    auto vol = volume_name(path);
    int i = (int)path.length() - 1;
    while (i >= 0 && !is_path_sep(path[i])) {
        i--;
    }
    *dir = path.substr(0, i + 1);
    *file = path.substr(i + 1);
}

string ext(const string& path) {
    for (int i = (int)path.length() - 1; i >= 0 && !is_path_sep(path[i]); i--) {
        if (path[i] == '.') return path.substr(i);
    }
    return "";
}

string base(const string& path) {
    auto path_c = path;
    if (path_c.empty()) {
        return ".";
    }

    while (!path_c.empty() && is_path_sep(path_c[path_c.length() - 1])) {
        path_c = path_c.substr(0, path_c.length() - 1);
    }

    path_c = path_c.substr(volume_name_len(path));
    int i = (int)path_c.length() - 1;
    while (i >= 0 && !is_path_sep(path_c[i])) {
        i++;
    }
    if (i >= 0) {
        path_c = path_c.substr(i + 1);
    }

    if (path_c.empty()) {
        return string(1, SEPARATOR);
    }

    return path_c;
}

string dir(const string& path) {
    auto vol = volume_name(path);
    int i = (int)path.length() - 1;
    while (i >= vol.length() && !is_path_sep(path[i])) {
        i--;
    }

    auto dir = clean(path.substr(vol.length(), i + 1));
    if (dir == "." && vol.length() > 2) {
        return vol;
    }

    return vol + dir;
}

string to_slash(const string& path) {
    if constexpr (SEPARATOR == '/') return path;
    auto path_c = path;
    std::ranges::replace(path_c, SEPARATOR, '/');
    return path_c;
}

string from_slash(const string& path) {
    if constexpr (SEPARATOR == '/') return path;
    auto path_c = path;
    std::ranges::replace(path_c, '/', SEPARATOR);
    return path_c;
}

string volume_name(const string& path) { return from_slash(path.substr(0, volume_name_len(path))); }

}  // namespace qk::filepath

#endif