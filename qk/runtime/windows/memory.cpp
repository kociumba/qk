#include "memory.h"

#ifdef QK_RUNTIME_UTILS

namespace qk::runtime::mem {

bool parse_signature(std::string_view sig, byte_vec& out) {
    out.clear();
    out.reserve(sig.size() / 2);

    for (size_t i = 0; i < sig.size();) {
        while (i < sig.size() && std::isspace(static_cast<unsigned char>(sig[i]))) {
            ++i;
        }
        if (i >= sig.size()) break;

        if (sig[i] == '?') {
            out.push_back(std::byte{WILDCARD_BYTE});
            while (i < sig.size() && sig[i] == '?')
                ++i;
            continue;
        }

        if (i + 1 >= sig.size()) return false;

        int hi = hex_to_nibble(sig[i]);
        int lo = hex_to_nibble(sig[i + 1]);
        if (hi < 0 || lo < 0) return false;

        out.push_back(std::byte((hi << 4) | lo));
        i += 2;
    }

    return true;
}

}  // namespace qk::runtime::mem

#endif
