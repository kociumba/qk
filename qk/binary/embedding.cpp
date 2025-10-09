#include "embedding.h"

#ifdef QK_EMBEDDING

namespace qk::embed {

SymbolCache default_symbol_cache = {};

Binary make_object(const std::string& name, Target format, Arch arch, const std::string& nasm) {
    std::ifstream file(name, std::ios::in | std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        return {};
    }

    auto file_size = file.tellg();

    if (file_size <= 0) {
        return {};
    }

    file.seekg(0, std::ios::beg);

    std::vector<std::byte> data(file_size);

    file.read(reinterpret_cast<char*>(data.data()), file_size);

    if (!file) {
        return {};
    }

    std::filesystem::path filepath(name);
    std::string base_name = filepath.stem().string();

    sanitize_symbol(base_name);

    std::string asm_path = filepath.parent_path().string();
    if (!asm_path.empty()) {
        asm_path += "/";
    }
    asm_path += base_name + ".asm";

    std::vector symbols = {base_name + "_data", base_name + "_size", base_name + "_end"};

    return {format, arch, symbols, std::move(data), nasm, asm_path};
}

bool Binary::render() {
    if (data.empty() || symbols.empty() || asm_path.empty()) {
        return false;
    }

    std::ofstream asm_file(asm_path);
    if (!asm_file.is_open()) {
        return false;
    }

#if defined(__APPLE__) && defined(__aarch64__)
    bool is_apple_arm = true;
#else
    bool is_apple_arm = false;
#endif

    bool use_gas_syntax = is_apple_arm;

    if (use_gas_syntax) {
        for (const auto& symbol : symbols) {
            asm_file << ".globl _" << symbol << "\n";
        }
        asm_file << "\n";
        asm_file << ".section __TEXT,__const\n\n";
    } else {
        asm_file << "BITS " << arch_map.at(arch) << "\n\n";
        for (const auto& symbol : symbols) {
            asm_file << "global " << symbol << "\n";
            // enables runtime symbol discovery on winapi
            if (format == Target::PE) {
                asm_file << "export " << symbol << "\n";
            }
        }
        asm_file << "\n";
        asm_file << "section .rodata\n\n";
    }

    asm_file << (use_gas_syntax ? "_" : "") << symbols[0] << ":\n";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) {
            asm_file << "    " << (use_gas_syntax ? ".byte " : "db ");
        }
        asm_file << "0x" << std::hex << std::setw(2) << std::setfill('0')
                 << (static_cast<unsigned int>(static_cast<unsigned char>(data[i])) & 0xFF);

        if (i + 1 < data.size() && (i + 1) % 16 != 0) {
            asm_file << ", ";
        } else {
            asm_file << "\n";
        }
    }

    if (symbols.size() > 2) {
        asm_file << (use_gas_syntax ? "_" : "") << symbols[2] << ":\n";
    }

    if (symbols.size() > 1) {
        if (use_gas_syntax) {
            asm_file << "\n    .align 3\n";
            asm_file << "\n_" << symbols[1] << ":\n";
            asm_file << "    .quad " << std::dec << data.size() << "\n";
        } else {
            asm_file << "\n" << symbols[1] << ":\n";
            asm_file << "    dq " << std::dec << data.size() << "\n";
        }
    }

    asm_file.close();
    return true;
}

bool Binary::assemble() {
    if (asm_path.empty() || nasm_path.empty()) {
        return false;
    }

    if (!std::filesystem::exists(asm_path)) {
        return false;
    }

    std::filesystem::path asm_filepath(asm_path);
    std::string obj_path = asm_filepath.parent_path().string();
    if (!obj_path.empty()) {
        obj_path += "/";
    }
    obj_path += asm_filepath.stem().string() + object_ext;

#if defined(__APPLE__) && defined(__aarch64__)
    std::string command =
        "clang -c -x assembler -arch arm64 -o \"" + obj_path + "\" \"" + asm_path + "\"";
#else
    std::string format_str(format_map.at(format));
    std::string arch_str(arch_map.at(arch));
    std::string command = nasm_path + " -f " + format_str + arch_str + " -o \"" + obj_path +
                          "\" \"" + asm_path + "\"";
#endif

    int result = std::system(command.c_str());

    return result == 0;
}

bool setup_cache(SymbolCache* cache) {
    if (!cache) {
        return false;
    }

#ifdef _WIN32
    cache->handle = GetModuleHandle(nullptr);
    if (!cache->handle) {
        return false;
    }
#else
    cache->handle = RTLD_DEFAULT;
#endif

    return true;
}

void* find_symbol(const std::string& name, SymbolCache* cache) {
    if (!cache) return nullptr;
    if (!cache->handle) setup_cache(cache);

#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress((HMODULE)cache->handle, name.c_str()));
#elif defined(__APPLE__)
    std::string prefixed = "_" + name;
    void* sym = dlsym(cache->handle, prefixed.c_str());
    if (!sym) {
        sym = dlsym(cache->handle, name.c_str());
    }
    return sym;
#else
    return dlsym(cache->handle, name.c_str());
#endif
}

Resource find_resource(const std::string& filename, SymbolCache* cache) {
    if (!cache) return {};
    if (!cache->handle) setup_cache(cache);

    auto cached_symbol = cache->file_to_symbol.find(filename);
    std::string base_name;

    if (cached_symbol != cache->file_to_symbol.end()) {
        base_name = cached_symbol->second;
        auto cached_resource = cache->symbol_to_resource.find(base_name);
        if (cached_resource != cache->symbol_to_resource.end()) {
            return cached_resource->second;
        }
    } else {
        base_name = filename_to_symbol(filename);
        cache->file_to_symbol[filename] = base_name;
    }

    std::string data_sym = base_name + "_data";
    std::string size_sym = base_name + "_size";
    std::string end_sym = base_name + "_end";

    const auto* data = static_cast<const unsigned char*>(find_symbol(data_sym, cache));
    const auto* size_ptr = static_cast<const uint64_t*>(find_symbol(size_sym, cache));
    const auto* data_end = static_cast<const unsigned char*>(find_symbol(end_sym, cache));

    Resource res;
    if (data && size_ptr && data_end) {
        res.data = data;
        res.size = *size_ptr;
        res.data_end = data_end;
        cache->symbol_to_resource[base_name] = res;
    }

    return res;
}

bool compress_object(Binary* bin, int level) {
    if (bin->data.empty()) return false;

    uLongf bound = compressBound(bin->data.size());
    std::vector<Bytef> buf(bound);
    auto err = compress2(
        buf.data(), &bound, reinterpret_cast<Bytef*>(bin->data.data()), bin->data.size(), level
    );
    if (err != Z_OK) {
        return false;
    }

    bin->data.resize(bound);
    std::memcpy(bin->data.data(), buf.data(), bound);

    return true;
}

std::vector<std::byte> decompress_data(const unsigned char* data, uint64_t size) {
    if (!data || !size) return {};

    z_stream stream = {};
    stream.next_in = const_cast<Bytef*>(data);
    stream.avail_in = static_cast<uInt>(size);
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    int ret = inflateInit2(&stream, MAX_WBITS);
    if (ret != Z_OK) return {};

    std::vector<std::byte> buf;
    buf.reserve(std::min(size * 2 + 1024, uint64_t(1ULL << 20)));

    Bytef out_byte;
    do {
        stream.next_out = &out_byte;
        stream.avail_out = 1;

        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_OK) {
            buf.push_back(static_cast<std::byte>(out_byte));
        } else if (ret == Z_STREAM_END) {
            if (stream.avail_out == 0) {
                buf.push_back(static_cast<std::byte>(out_byte));
            }
            break;
        } else if (ret != Z_BUF_ERROR) {
            (void)inflateEnd(&stream);
            return {};
        }
    } while (stream.avail_in > 0);

    stream.next_out = &out_byte;
    stream.avail_out = 1;
    ret = inflate(&stream, Z_FINISH);
    if (ret == Z_OK || ret == Z_STREAM_END) {
        if (stream.avail_out == 0) {
            buf.push_back(static_cast<std::byte>(out_byte));
        }
    } else if (ret != Z_BUF_ERROR) {
        (void)inflateEnd(&stream);
        return {};
    }

    (void)inflateEnd(&stream);

    return buf;
}

bool patch_macho_arm64(const std::filesystem::path& path) {
    std::fstream f(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!f.is_open()) return false;

    uint32_t magic = 0;
    f.read(reinterpret_cast<char*>(&magic), 4);

    if (magic != 0xFEEDFACF) {
        return false;
    }

    f.seekp(4, std::ios::beg);

    const uint32_t CPU_TYPE_ARM64 = 0x0100000C;
    const uint32_t CPU_SUBTYPE_ARM64_ALL = 0x00000000;

    f.write(reinterpret_cast<const char*>(&CPU_TYPE_ARM64), 4);
    f.write(reinterpret_cast<const char*>(&CPU_SUBTYPE_ARM64_ALL), 4);

    f.flush();
    return true;
}

}  // namespace qk::embed

#endif
