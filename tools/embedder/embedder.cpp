#include <qk/qk_binary.h>
#include <filesystem>
#include <iostream>

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options] <input_file> [input_files...]\n"
              << "\nOptions:\n"
              << "  -o, --output <dir>     Output directory for object files (default: current "
                 "directory)\n"
              << "  -k, --keep-asm         Keep assembly files after assembling (default: delete)\n"
              << "  -f, --format <fmt>     Target format: elf, pe, macho (default: auto-detect)\n"
              << "  -a, --arch <arch>      Target architecture: x64, x32 (default: auto-detect)\n"
              << "  -h, --help             Show this help message\n"
              << "\nExample:\n"
              << "  " << prog_name << " -o build/ data.bin texture.png\n";
}

#ifdef _WIN32
std::string object = ".obj";
#else
std::string object = ".o";
#endif

int main(int argc, char* argv[]) {
    using namespace qk::embed;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::vector<std::string> input_files;
    std::string output_dir = ".";
    bool keep_asm = false;
    Target target = default_target;
    Arch arch = default_arch;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-k" || arg == "--keep-asm") {
            keep_asm = true;
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            output_dir = argv[++i];
        } else if ((arg == "-f" || arg == "--format") && i + 1 < argc) {
            std::string fmt = argv[++i];
            if (fmt == "elf")
                target = Target::ELF;
            else if (fmt == "pe")
                target = Target::PE;
            else if (fmt == "macho")
                target = Target::MACH_O;
            else {
                std::cerr << "Error: Unknown format '" << fmt << "'\n";
                return 1;
            }
        } else if ((arg == "-a" || arg == "--arch") && i + 1 < argc) {
            std::string a = argv[++i];
            if (a == "x64")
                arch = Arch::x64;
            else if (a == "x32")
                arch = Arch::x32;
            else {
                std::cerr << "Error: Unknown architecture '" << a << "'\n";
                return 1;
            }
        } else if (arg[0] != '-') {
            input_files.push_back(arg);
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input_files.empty()) {
        std::cerr << "Error: No input files specified\n";
        print_usage(argv[0]);
        return 1;
    }

    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
    }

    int failed_count = 0;

    for (const auto& input : input_files) {
        std::cout << "Processing: " << input << std::endl;

        if (!std::filesystem::exists(input)) {
            std::cerr << "  Error: File not found\n";
            failed_count++;
            continue;
        }

        Binary bin = make_object(input, target, arch);

        if (bin.data.empty()) {
            std::cerr << "  Error: Failed to read file\n";
            failed_count++;
            continue;
        }

        std::filesystem::path input_path(input);
        std::string base_name = input_path.stem().string();

        for (auto& c : base_name) {
            if (!std::isalnum(static_cast<unsigned char>(c))) {
                c = '_';
            }
        }

        bin.asm_path = output_dir + "/" + base_name + ".asm";
        std::string obj_path = output_dir + "/" + base_name + object;

        if (!bin.render()) {
            std::cerr << "  Error: Failed to render assembly\n";
            failed_count++;
            continue;
        }

        std::cout << "  Generated: " << bin.asm_path << std::endl;

        if (!bin.assemble()) {
            std::cerr << "  Error: Failed to assemble object file\n";

            if (std::filesystem::exists(bin.asm_path)) {
                std::filesystem::remove(bin.asm_path);
            }

            failed_count++;
            continue;
        }

        std::cout << "  Generated: " << obj_path << std::endl;

        if (!keep_asm && std::filesystem::exists(bin.asm_path)) {
            std::filesystem::remove(bin.asm_path);
            std::cout << "  Cleaned up assembly file\n";
        }

        std::cout << "  Success!\n";
    }

    int success_count = input_files.size() - failed_count;
    std::cout << "\nProcessed " << input_files.size() << " file(s): " << success_count
              << " succeeded, " << failed_count << " failed\n";

    return failed_count > 0 ? 1 : 0;
}