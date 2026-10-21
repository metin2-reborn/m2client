// proto_convert: converts a locale's item_proto / mob_proto from the legacy format (names in the
// locale's code page, 25-byte name fields) to format v2 (UTF-8 names, 64-byte name fields).
//
//   proto_convert <locale dir> <code page> [--list-korean]   e.g. proto_convert locale/de cp1252
//
// Reads <dir>/item_proto and <dir>/mob_proto (v1), writes them back as v2. Every field other than
// the names is copied unchanged. The internal name (szName) is Korean CP949; the displayed name
// (szLocaleName) is in the locale's code page, unless it is untranslated Korean (EUC-KR).
#include <metin2/core/lzo.h>
#include <metin2/core/tea.h>
#include <metin2/core/utf8.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <iconv.h>

using namespace metin2;

namespace {
    uint32_t s_itemKey[4] = {173217, 72619434, 408587239, 27973291};
    uint32_t s_mobKey[4] = {4813894, 18955, 552631, 6822045};

    constexpr uint32_t FourCC(char a, char b, char c, char d) {
        return uint32_t(uint8_t(a)) | (uint32_t(uint8_t(b)) << 8) | (uint32_t(uint8_t(c)) << 16) | (uint32_t(uint8_t(d)) << 24);
    }

    constexpr std::size_t kOldName = 25;  // char[24 + 1]
    constexpr std::size_t kNewName = 64;  // char[63 + 1]
    constexpr std::size_t kOldItem = 156; // v1 TItemTable
    constexpr std::size_t kOldMob = 255;  // v1 TMobTable

    int g_truncated = 0;
    int g_korean = 0;
    bool g_listKorean = false;

    auto ReadFile(const std::string &path, std::vector<uint8_t> &out) -> bool {
        FILE *f = fopen(path.c_str(), "rb");
        if (!f)
            return false;
        fseek(f, 0, SEEK_END);
        out.resize(static_cast<std::size_t>(ftell(f)));
        fseek(f, 0, SEEK_SET);
        bool ok = fread(out.data(), 1, out.size(), f) == out.size();
        fclose(f);
        return ok;
    }

    // Decodes text in a legacy code page; bytes the code page does not define become U+FFFD.
    auto Decode(std::string_view in, const char *codePage) -> std::string {
        std::string out;
        iconv_t cd = iconv_open("UTF-8", codePage);
        for (std::size_t pos = 0; pos < in.size();) {
            std::size_t step = 1;
            if (static_cast<unsigned char>(in[pos]) < 0x80) {
                out += in[pos];
            } else {
                // try one, then two bytes (double-byte code pages)
                bool done = false;
                for (std::size_t len = 1; len <= 2 && pos + len <= in.size() && !done; ++len) {
                    char buf[8];
                    char *src = const_cast<char *>(in.data() + pos);
                    char *dst = buf;
                    std::size_t inLeft = len, outLeft = sizeof(buf);
                    iconv(cd, nullptr, nullptr, nullptr, nullptr);
                    if (iconv(cd, &src, &inLeft, &dst, &outLeft) != static_cast<std::size_t>(-1) && inLeft == 0) {
                        out.append(buf, dst);
                        step = len;
                        done = true;
                    }
                }
                if (!done)
                    out += "\xEF\xBF\xBD";
            }
            pos += step;
        }
        iconv_close(cd);
        return out;
    }

    // Untranslated Korean: every high byte pairs as KS X 1001 and the text holds a Hangul run
    // that is not glued to Latin letters (cp1250/1251 letters can pair up by accident).
    auto LooksKorean(std::string_view s) -> bool {
        bool high = false;
        for (std::size_t i = 0; i < s.size();) {
            auto c = static_cast<unsigned char>(s[i]);
            if (c < 0x80) {
                ++i;
                continue;
            }
            auto t = i + 1 < s.size() ? static_cast<unsigned char>(s[i + 1]) : 0;
            if (c < 0xA1 || c > 0xFE || t < 0xA1 || t > 0xFE)
                return false;
            high = true;
            i += 2;
        }
        if (!high)
            return false;
        std::wstring w = utf8::to_wide(Decode(s, "CP949"));
        bool run = false;
        for (std::size_t i = 0; i < w.size(); ++i) {
            bool hangul = w[i] >= 0xAC00 && w[i] <= 0xD7A3;
            if (hangul && i + 1 < w.size() && w[i + 1] >= 0xAC00 && w[i + 1] <= 0xD7A3)
                run = true;
            auto latin = [&](std::size_t k) { return k < w.size() && ((w[k] >= 'A' && w[k] <= 'Z') || (w[k] >= 'a' && w[k] <= 'z')); };
            if (hangul && (latin(i + 1) || (i > 0 && latin(i - 1))))
                return false;
        }
        return run;
    }

    // Converts a legacy name field to a UTF-8 v2 field.
    void ConvertName(const uint8_t *oldField, uint8_t *newField, const char *codePage, bool displayName, uint32_t vnum, const char *what) {
        std::size_t len = strnlen(reinterpret_cast<const char *>(oldField), kOldName); // may lack a NUL
        std::string_view name(reinterpret_cast<const char *>(oldField), len);
        const char *cp = codePage;
        if (!displayName || LooksKorean(name)) {
            if (displayName) {
                ++g_korean;
                if (g_listKorean)
                    printf("  %s %u: untranslated: %s\n", what, vnum, Decode(name, "CP949").c_str());
            }
            cp = "CP949";
        }
        std::string text = Decode(name, cp);
        std::size_t n = utf8::truncated_size(text, kNewName - 1);
        if (n < text.size()) {
            ++g_truncated;
            fprintf(stderr, "  %s %u: name truncated: %s\n", what, vnum, text.c_str());
        }
        memset(newField, 0, kNewName);
        memcpy(newField, text.data(), n);
    }

    auto decompress(const uint8_t *blob, uint32_t *key, std::vector<uint8_t> &out) -> bool {
        lz_object obj;
        if (!get_clzo().decompress(obj, blob, key))
            return false;
        out.assign(obj.buffer(), obj.buffer() + obj.buffer_size());
        return true;
    }

    // The "MCOZ" container the client decrypts and decompresses (see CLZObject::Decompress).
    auto Compress(const std::vector<uint8_t> &in, uint32_t *key) -> std::vector<uint8_t> {
        std::vector<uint8_t> plain(4 + in.size() + in.size() / 16 + 64 + 3);
        uint32_t fourcc = FourCC('M', 'C', 'O', 'Z');
        memcpy(plain.data(), &fourcc, 4);
        lzo_uint compressed = 0;
        std::vector<uint8_t> work(LZO1X_1_MEM_COMPRESS);
        if (lzo1x_1_compress(in.data(), in.size(), plain.data() + 4, &compressed, work.data()) != LZO_E_OK)
            return {};
        std::size_t plainSize = 4 + compressed;
        std::vector<uint8_t> encrypted(plainSize + 8);
        plain.resize(plainSize + 8, 0);
        int encryptedSize = tea_encrypt(reinterpret_cast<uint32_t *>(encrypted.data()), reinterpret_cast<const uint32_t *>(plain.data()), key, static_cast<int>(plainSize));

        std::vector<uint8_t> out(sizeof(lz_object::header) + encryptedSize);
        lz_object::header header_value{fourcc, static_cast<uint32_t>(encryptedSize), static_cast<uint32_t>(compressed), static_cast<uint32_t>(in.size())};
        memcpy(out.data(), &header_value, sizeof(header_value));
        memcpy(out.data() + sizeof(header_value), encrypted.data(), encryptedSize);
        return out;
    }

    // Decompresses what Compress produced and compares it with the records it came from.
    auto Verify(const std::vector<uint8_t> &blob, uint32_t *key, const std::vector<uint8_t> &records) -> bool {
        std::vector<uint8_t> back;
        if (!decompress(blob.data(), key, back) || back.size() < records.size() || memcmp(back.data(), records.data(), records.size()) != 0) {
            fprintf(stderr, "round trip check failed\n");
            return false;
        }
        return true;
    }

    void Put32(std::vector<uint8_t> &out, uint32_t v) {
        out.insert(out.end(), reinterpret_cast<uint8_t *>(&v), reinterpret_cast<uint8_t *>(&v) + 4);
    }

    auto Write(const std::string &path, const std::vector<uint8_t> &data) -> bool {
        FILE *f = fopen(path.c_str(), "wb");
        if (!f)
            return false;
        bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
        return fclose(f) == 0 && ok;
    }

    auto ConvertItems(const std::string &dir, const char *codePage) -> bool {
        std::string path = dir + "/item_proto";
        std::vector<uint8_t> file;
        if (!ReadFile(path, file) || file.size() < 20) {
            fprintf(stderr, "cannot read %s\n", path.c_str());
            return false;
        }
        uint32_t h[5];
        memcpy(h, file.data(), sizeof(h)); // fourcc, version, stride, elements, data size
        if (h[0] != FourCC('M', 'I', 'P', 'X') || h[1] != 1 || h[2] != kOldItem) {
            fprintf(stderr, "%s: not a v1 item_proto (version %u, stride %u)\n", path.c_str(), h[1], h[2]);
            return false;
        }
        std::vector<uint8_t> records;
        if (!decompress(file.data() + 20, s_itemKey, records) || records.size() < std::size_t(h[3]) * kOldItem)
            return false;

        constexpr std::size_t kNewItem = kOldItem + 2 * (kNewName - kOldName);
        std::vector<uint8_t> out(std::size_t(h[3]) * kNewItem);
        for (uint32_t i = 0; i < h[3]; ++i) {
            const uint8_t *o = records.data() + i * kOldItem;
            uint8_t *n = out.data() + i * kNewItem;
            uint32_t vnum;
            memcpy(&vnum, o, 4);
            memcpy(n, o, 8); // vnum, vnum range
            ConvertName(o + 8, n + 8, codePage, false, vnum, "item");
            ConvertName(o + 8 + kOldName, n + 8 + kNewName, codePage, true, vnum, "item");
            memcpy(n + 8 + 2 * kNewName, o + 8 + 2 * kOldName, kOldItem - 8 - 2 * kOldName);
        }

        std::vector<uint8_t> blob = Compress(out, s_itemKey);
        if (blob.empty() || !Verify(blob, s_itemKey, out))
            return false;
        for (uint32_t i = 0; i < h[3] && i < 3; ++i)
            printf("  item %u: %s\n", *reinterpret_cast<uint32_t *>(&out[i * kNewItem]), reinterpret_cast<char *>(&out[i * kNewItem + 8 + kNewName]));
        std::vector<uint8_t> result;
        Put32(result, FourCC('M', 'I', 'P', 'X'));
        Put32(result, 2);
        Put32(result, kNewItem);
        Put32(result, h[3]);
        Put32(result, static_cast<uint32_t>(blob.size()));
        result.insert(result.end(), blob.begin(), blob.end());
        printf("%s: %u items\n", path.c_str(), h[3]);
        return !blob.empty() && Write(path, result);
    }

    auto ConvertMobs(const std::string &dir, const char *codePage) -> bool {
        std::string path = dir + "/mob_proto";
        std::vector<uint8_t> file;
        if (!ReadFile(path, file) || file.size() < 12) {
            fprintf(stderr, "cannot read %s\n", path.c_str());
            return false;
        }
        uint32_t h[3];
        memcpy(h, file.data(), sizeof(h)); // fourcc, elements, data size
        if (h[0] != FourCC('M', 'M', 'P', 'T')) {
            fprintf(stderr, "%s: not a v1 mob_proto\n", path.c_str());
            return false;
        }
        std::vector<uint8_t> records;
        if (!decompress(file.data() + 12, s_mobKey, records) || records.size() < std::size_t(h[1]) * kOldMob)
            return false;

        constexpr std::size_t kNewMob = kOldMob + 2 * (kNewName - kOldName);
        std::vector<uint8_t> out(std::size_t(h[1]) * kNewMob);
        for (uint32_t i = 0; i < h[1]; ++i) {
            const uint8_t *o = records.data() + i * kOldMob;
            uint8_t *n = out.data() + i * kNewMob;
            uint32_t vnum;
            memcpy(&vnum, o, 4);
            memcpy(n, o, 4); // vnum
            ConvertName(o + 4, n + 4, codePage, false, vnum, "mob");
            ConvertName(o + 4 + kOldName, n + 4 + kNewName, codePage, true, vnum, "mob");
            memcpy(n + 4 + 2 * kNewName, o + 4 + 2 * kOldName, kOldMob - 4 - 2 * kOldName);
        }

        std::vector<uint8_t> blob = Compress(out, s_mobKey);
        if (blob.empty() || !Verify(blob, s_mobKey, out))
            return false;
        for (uint32_t i = 0; i < h[1] && i < 3; ++i)
            printf("  mob %u: %s\n", *reinterpret_cast<uint32_t *>(&out[i * kNewMob]), reinterpret_cast<char *>(&out[i * kNewMob + 4 + kNewName]));
        std::vector<uint8_t> result;
        Put32(result, FourCC('M', 'M', 'P', '2'));
        Put32(result, h[1]);
        Put32(result, static_cast<uint32_t>(blob.size()));
        result.insert(result.end(), blob.begin(), blob.end());
        printf("%s: %u mobs\n", path.c_str(), h[1]);
        return !blob.empty() && Write(path, result);
    }
}

int main(int argc, char **argv) {
    if (argc == 4 && !strcmp(argv[3], "--list-korean"))
        g_listKorean = true;
    else if (argc != 3) {
        fprintf(stderr, "usage: proto_convert <locale dir> <code page, e.g. cp1252> [--list-korean]\n");
        return 2;
    }
    static clzo lzo;
    bool ok = ConvertItems(argv[1], argv[2]);
    ok = ConvertMobs(argv[1], argv[2]) && ok;
    printf("%d untranslated Korean names, %d truncated\n", g_korean, g_truncated);
    return ok ? 0 : 1;
}
