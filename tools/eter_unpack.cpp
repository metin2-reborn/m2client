// eter_unpack: extracts the original client's EterPack archives (pack/*.eix + *.epk) into a
// directory, one folder per pack, keeping the paths stored in the index ("d:/ymir work/..."
// loses its drive). Entries of the plain, compressed and key-encrypted kinds are decoded; the
// Panama and hybrid-crypt kinds need keys the login server hands out at runtime and are listed
// in <out>/unsupported.txt instead. File names are written as stored (the Korean packs use CP949).
//
//   eter_unpack <out dir> <pack.eix | pack dir>...

#include <metin2/core/crc32.h>
#include <metin2/core/lzo.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace metin2;

namespace {
    constexpr uint32_t PACK_FOUR_CC = 0x444b5045; // "EPKD"
    constexpr uint32_t LZO_FOUR_CC = 0x5a4f434d;  // "MCOZ"
    constexpr uint32_t PACK_VERSION = 2;

    uint32_t index_key[4] = {45129401, 92367215, 681285731, 1710201};
    uint32_t security_key[4] = {78952482, 527348324, 1632942, 486274726};

    enum compressed_type : char {
        TYPE_NONE = 0,
        TYPE_COMPRESS = 1,
        TYPE_SECURITY = 2,
        TYPE_PANAMA = 3,
        TYPE_HYBRIDCRYPT = 4,
        TYPE_HYBRIDCRYPT_WITHSDB = 5,
    };

#pragma pack(push, 4)
    struct pack_index {
        int32_t id;
        char filename[161];
        uint32_t filename_crc;
        int32_t real_data_size;
        int32_t data_size;
        uint32_t data_crc;
        int32_t data_position;
        char compressed_type;
    };
#pragma pack(pop)
    static_assert(sizeof(pack_index) == 192, "TEterPackIndex is serialized into .eix files");

    struct stats {
        size_t files = 0;
        size_t unsupported = 0;
        size_t failed = 0;
        size_t crc_mismatch = 0;
        uint64_t bytes = 0;
    };

    auto read_file(const fs::path &path, std::vector<uint8_t> &out) -> bool {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return false;
        }
        in.seekg(0, std::ios::end);
        const auto size = static_cast<size_t>(in.tellg());
        in.seekg(0);
        out.resize(size);
        return size == 0 || bool(in.read(reinterpret_cast<char *>(out.data()), static_cast<std::streamsize>(size)));
    }

    // A compressed block ("MCOZ" container) decoded into out; false with the reason printed.
    auto decode_block(const uint8_t *data, size_t size, uint32_t *key, std::vector<uint8_t> &out) -> bool {
        if (size < sizeof(lz_object::header)) {
            return false;
        }
        lz_object object;
        if (!object.begin_decompress(data) || !object.decompress(key)) {
            return false;
        }
        out.assign(object.buffer(), object.buffer() + object.size());
        return true;
    }

    // The stored path without its drive, rejected when it could leave the output directory.
    auto stored_path(const pack_index &entry) -> std::string {
        std::string name(entry.filename, strnlen(entry.filename, sizeof(entry.filename)));
        std::ranges::replace(name, '\\', '/');
        if (name.size() > 2 && name[1] == ':' && name[2] == '/') {
            name.erase(0, 3);
        }
        while (!name.empty() && name.front() == '/') {
            name.erase(0, 1);
        }
        if (name.empty() || name.find("../") != std::string::npos || name.ends_with("/..") || name == "..") {
            return {};
        }
        return name;
    }

    auto unpack(const fs::path &index_path, const fs::path &out_dir, std::ofstream &unsupported, stats &total) -> bool {
        const std::string pack_name = index_path.stem().string();
        fs::path data_path = index_path;
        data_path.replace_extension(".epk");
        if (!fs::exists(data_path)) {
            // The data file may be spelt with other capitals than the index (Etc.epk / ETC.eix).
            for (const fs::directory_entry &entry : fs::directory_iterator(index_path.parent_path())) {
                std::string candidate = entry.path().filename().string();
                std::string wanted = data_path.filename().string();
                std::ranges::transform(candidate, candidate.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
                std::ranges::transform(wanted, wanted.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
                if (candidate == wanted) {
                    data_path = entry.path();
                    break;
                }
            }
        }

        std::vector<uint8_t> index_file;
        if (!read_file(index_path, index_file) || index_file.size() < 12) {
            fprintf(stderr, "%s: cannot read the index\n", pack_name.c_str());
            return false;
        }
        std::vector<uint8_t> index_plain;
        uint32_t four_cc = 0;
        memcpy(&four_cc, index_file.data(), 4);
        if (four_cc == PACK_FOUR_CC) {
            index_plain = index_file;
        } else if (four_cc == LZO_FOUR_CC) {
            if (!decode_block(index_file.data(), index_file.size(), index_key, index_plain)) {
                fprintf(stderr, "%s: the index does not decode (wrong key?)\n", pack_name.c_str());
                return false;
            }
        } else {
            fprintf(stderr, "%s: not an EterPack index\n", pack_name.c_str());
            return false;
        }
        if (index_plain.size() < 12) {
            fprintf(stderr, "%s: index too short\n", pack_name.c_str());
            return false;
        }
        uint32_t version = 0;
        int32_t count = 0;
        memcpy(&four_cc, index_plain.data(), 4);
        memcpy(&version, index_plain.data() + 4, 4);
        memcpy(&count, index_plain.data() + 8, 4);
        if (four_cc != PACK_FOUR_CC || version != PACK_VERSION || count < 0 || index_plain.size() < 12 + sizeof(pack_index) * size_t(count)) {
            fprintf(stderr, "%s: bad index header (version %u, %d entries, %zu bytes)\n", pack_name.c_str(), version, count, index_plain.size());
            return false;
        }

        std::vector<uint8_t> data_file;
        if (!read_file(data_path, data_file)) {
            fprintf(stderr, "%s: cannot read %s\n", pack_name.c_str(), data_path.string().c_str());
            return false;
        }

        stats pack;
        std::vector<uint8_t> plain;
        for (int32_t i = 0; i < count; ++i) {
            pack_index entry;
            memcpy(&entry, index_plain.data() + 12 + sizeof(pack_index) * size_t(i), sizeof(entry));
            if (!entry.filename_crc) {
                continue; // a free slot
            }
            const std::string name = stored_path(entry);
            if (name.empty()) {
                fprintf(stderr, "%s: skipping an entry with an unusable name\n", pack_name.c_str());
                ++pack.failed;
                continue;
            }
            if (entry.data_position < 0 || entry.data_size < 0 || uint64_t(entry.data_position) + uint64_t(entry.data_size) > data_file.size()) {
                fprintf(stderr, "%s: %s lies outside the data file\n", pack_name.c_str(), name.c_str());
                ++pack.failed;
                continue;
            }
            const uint8_t *data = data_file.data() + entry.data_position;
            const auto size = static_cast<size_t>(entry.data_size);
            const uint8_t *out = nullptr;
            size_t out_size = 0;
            switch (entry.compressed_type) {
            case TYPE_NONE:
                out = data;
                out_size = size;
                break;
            case TYPE_COMPRESS:
            case TYPE_SECURITY:
                if (!decode_block(data, size, entry.compressed_type == TYPE_SECURITY ? security_key : nullptr, plain)) {
                    fprintf(stderr, "%s: %s does not decompress\n", pack_name.c_str(), name.c_str());
                    ++pack.failed;
                    continue;
                }
                out = plain.data();
                out_size = plain.size();
                break;
            default:
                unsupported << pack_name << '\t' << int(entry.compressed_type) << '\t' << name << '\n';
                ++pack.unsupported;
                continue;
            }
            // The index CRC is over the stored bytes, and the original client only checks it for
            // the key-encrypted kinds; for the others the field is often stale.
            if (entry.compressed_type == TYPE_SECURITY && get_crc32(reinterpret_cast<const char *>(data), size) != entry.data_crc) {
                fprintf(stderr, "%s: %s has a CRC mismatch (written anyway)\n", pack_name.c_str(), name.c_str());
                ++pack.crc_mismatch;
            }
            const fs::path target = out_dir / pack_name / fs::path(name);
            std::error_code error;
            fs::create_directories(target.parent_path(), error);
            std::ofstream file(target, std::ios::binary);
            if (!file || !file.write(reinterpret_cast<const char *>(out), static_cast<std::streamsize>(out_size))) {
                fprintf(stderr, "%s: cannot write %s\n", pack_name.c_str(), target.string().c_str());
                ++pack.failed;
                continue;
            }
            ++pack.files;
            pack.bytes += out_size;
        }
        printf("%-36s %6zu files %8.1f MB", pack_name.c_str(), pack.files, double(pack.bytes) / (1024.0 * 1024.0));
        if (pack.unsupported) {
            printf("  %zu need server keys", pack.unsupported);
        }
        if (pack.crc_mismatch) {
            printf("  %zu crc mismatches", pack.crc_mismatch);
        }
        if (pack.failed) {
            printf("  %zu failed", pack.failed);
        }
        printf("\n");
        fflush(stdout);
        total.files += pack.files;
        total.bytes += pack.bytes;
        total.unsupported += pack.unsupported;
        total.crc_mismatch += pack.crc_mismatch;
        total.failed += pack.failed;
        return true;
    }
}

auto main(int argc, char **argv) -> int {
    if (argc < 3) {
        fprintf(stderr, "usage: eter_unpack <out dir> <pack.eix | pack dir>...\n");
        return 2;
    }
    const fs::path out_dir = argv[1];
    std::vector<fs::path> indices;
    for (int i = 2; i < argc; ++i) {
        const fs::path arg = argv[i];
        if (fs::is_directory(arg)) {
            for (const fs::directory_entry &entry : fs::directory_iterator(arg)) {
                std::string ext = entry.path().extension().string();
                std::ranges::transform(ext, ext.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
                if (ext == ".eix") {
                    indices.push_back(entry.path());
                }
            }
        } else {
            indices.push_back(arg);
        }
    }
    std::ranges::sort(indices);

    std::error_code error;
    fs::create_directories(out_dir, error);
    std::ofstream unsupported(out_dir / "unsupported.txt");
    stats total;
    int bad_packs = 0;
    for (const fs::path &index : indices) {
        if (!unpack(index, out_dir, unsupported, total)) {
            ++bad_packs;
        }
    }
    printf("unpacked %zu files (%.1f MB) from %zu packs; %zu need server keys, %zu crc mismatches, %zu failed, %d packs unreadable\n", total.files,
        double(total.bytes) / (1024.0 * 1024.0), indices.size() - size_t(bad_packs), total.unsupported, total.crc_mismatch, total.failed, bad_packs);
    return bad_packs || total.failed ? 1 : 0;
}
