#include "embedding.h"

#ifdef QK_EMBEDDING

namespace qk::embed {

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

    for (auto& c : base_name) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            c = '_';
        }
    }

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

    asm_file << "BITS " << arch_map.at(arch) << "\n\n";
    for (const auto& symbol : symbols) {
        asm_file << "global " << symbol << "\n";
    }
    asm_file << "\n";

    asm_file << "section .rodata\n\n";

    asm_file << symbols[0] << ":\n";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) {
            asm_file << "    db ";
        }
        asm_file << "0x" << std::hex << std::setfill('0') << std::setw(2)
                 << static_cast<int>(static_cast<unsigned char>(data[i]));

        if (i + 1 < data.size() && (i + 1) % 16 != 0) {
            asm_file << ", ";
        } else {
            asm_file << "\n";
        }
    }

    if (symbols.size() > 2) {
        asm_file << symbols[2] << ":\n";
    }

    if (symbols.size() > 1) {
        asm_file << "\n" << symbols[1] << ":\n";
        asm_file << "    dq " << std::dec << data.size() << "\n";
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

    std::string format_str(format_map.at(format));
    std::string arch_str(arch_map.at(arch));
    std::string command = nasm_path + " -f " + format_str + arch_str + " -o \"" + obj_path +
                          "\" \"" + asm_path + "\"";

    int result = std::system(command.c_str());

    return result == 0;
}

}  // namespace qk::embed

#endif
