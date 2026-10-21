// grn_dump: load every .gr2 of an archive through the Granny runtime and report what the assets
// actually use (vertex layouts, curve formats, failures). Run from the game directory: files are
// read through the client's file manager (data/<Archive>.zip and data/<Archive>/); writes nothing
// except an --extract target.
//
//   grn_dump [--archive <name>] [--limit N] [--verbose] [--file <name>] [--list <substring>] [--extract <name> <out-file>]
//   --archive picks the archive to scan (default System).
//   --list prints every file of the archive whose path contains <substring> (case-insensitive).
//   --extract writes one file, looked up exactly as the client does ("d:/...", "Archive:path").
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <granny.h>
#include <map>
#include <metin2/core/mapped_file.h>
#include <metin2/files/eter_file_manager.h>
#include <set>
#include <string>
#include <strings.h>
#include <vector>

using namespace metin2;

static std::map<std::string, int> g_logMessages;
static int g_verbose = 0;
static std::string g_currentFile;

static void LogCallback(granny_log_message_type type, granny_log_message_origin, const char *, granny_int32x, const char *msg, void *) {
    std::string key = std::string(type == GrannyErrorLogMessage ? "error: " : type == GrannyWarningLogMessage ? "warning: "
                                                                                                              : "note: ") +
                      (msg ? msg : "");
    ++g_logMessages[key];
    if (g_verbose) fprintf(stderr, "  [%s] %s\n", g_currentFile.c_str(), key.c_str());
}

static std::string TypeSignature(const granny_data_type_definition *t) {
    std::string s;
    if (!t) return "(null)";
    for (; t->Type != GrannyEndMember; ++t) {
        if (!s.empty()) s += ",";
        s += t->Name ? t->Name : "?";
        char buf[32];
        snprintf(buf, sizeof(buf), ":%d", (int)t->Type);
        s += buf;
        if (t->ArrayWidth) {
            snprintf(buf, sizeof(buf), "[%d]", t->ArrayWidth);
            s += buf;
        }
    }
    return s;
}

int main(int argc, char **argv) {
    int limit = 0;
    const char *archive = "System";
    const char *only = nullptr;
    const char *list = nullptr;
    const char *extractName = nullptr;
    const char *extractOut = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--limit") && i + 1 < argc)
            limit = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--verbose"))
            g_verbose = 1;
        else if (!strcmp(argv[i], "--archive") && i + 1 < argc)
            archive = argv[++i];
        else if (!strcmp(argv[i], "--file") && i + 1 < argc)
            only = argv[++i];
        else if (!strcmp(argv[i], "--list") && i + 1 < argc)
            list = argv[++i];
        else if (!strcmp(argv[i], "--extract") && i + 2 < argc) {
            extractName = argv[++i];
            extractOut = argv[++i];
        }
    }
    eter_file_manager &files_ = get_file_manager();
    if (extractName) {
        mapped_file file;
        const void *data = nullptr;
        if (!files_.get(file, extractName, &data)) {
            fprintf(stderr, "not found: %s\n", extractName);
            return 1;
        }
        FILE *fp = fopen(extractOut, "wb");
        if (!fp) {
            perror(extractOut);
            return 1;
        }
        fwrite(data, 1, file.size(), fp);
        fclose(fp);
        printf("extracted %s (%u bytes) to %s\n", extractName, (unsigned)file.size(), extractOut);
        return 0;
    }
    if (list) {
        std::vector<std::string> found;
        files_.for_each(archive, "", [&](const std::string &name) { if (strcasestr(name.c_str(), list)) found.push_back(name); });
        std::sort(found.begin(), found.end());
        for (const std::string &name : found)
            printf("%s\n", name.c_str());
        return 0;
    }
    granny_log_callback cb = {LogCallback, nullptr};
    GrannySetLogCallback(&cb);

    int files = 0, ok = 0, failed = 0, models = 0, meshes = 0, anims = 0, skeletons = 0, textures = 0;
    std::map<std::string, int> vertexTypes, curveTypes, extendedData, sections;
    std::set<std::string> failedFiles;
    std::vector<std::string> names;
    files_.for_each(archive, "", [&](const std::string &fn) {
        size_t n = fn.size();
        if (n < 4 || strcasecmp(fn.c_str() + n - 4, ".gr2") != 0) return;
        if (only && strcasecmp(fn.c_str(), only) != 0) return;
        names.push_back(fn);
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const std::string &fn : names) {
        if (limit && files >= limit) break;
        ++files;
        g_currentFile = fn;
        mapped_file mapped;
        const void *data = nullptr;
        std::string assetName = std::string(archive) + ":" + fn;
        if (!files_.get(mapped, assetName.c_str(), &data) || !data) {
            ++failed;
            failedFiles.insert(fn + " (read failed)");
            continue;
        }
        std::vector<uint8_t> copy((const uint8_t *)data, (const uint8_t *)data + mapped.size());
        granny_file *file = GrannyReadEntireFileFromMemory((granny_int32x)copy.size(), copy.data());
        granny_file_info *info = file ? GrannyGetFileInfo(file) : nullptr;
        if (!info) {
            ++failed;
            failedFiles.insert(fn);
            if (file) GrannyFreeFile(file);
            continue;
        }
        ++ok;
        models += info->ModelCount;
        meshes += info->MeshCount;
        anims += info->AnimationCount;
        skeletons += info->SkeletonCount;
        textures += info->TextureCount;
        for (int i = 0; i < info->MeshCount; ++i) {
            granny_mesh *m = info->Meshes[i];
            if (m && m->PrimaryVertexData) ++vertexTypes[TypeSignature(m->PrimaryVertexData->VertexType)];
            if (m && m->MaterialBindingCount)
                for (int b = 0; b < m->MaterialBindingCount; ++b) {
                    granny_material *mat = m->MaterialBindings[b].Material;
                    if (mat && mat->ExtendedData.Type) ++extendedData[TypeSignature(mat->ExtendedData.Type)];
                }
        }
        for (int i = 0; i < info->AnimationCount; ++i) {
            granny_animation *a = info->Animations[i];
            if (!a) continue;
            for (int g = 0; g < a->TrackGroupCount; ++g) {
                granny_track_group *tg = a->TrackGroups[g];
                if (!tg) continue;
                for (int t = 0; t < tg->TransformTrackCount; ++t) {
                    granny_transform_track &tt = tg->TransformTracks[t];
                    ++curveTypes["orient " + TypeSignature(tt.OrientationCurve.CurveData.Type)];
                    ++curveTypes["pos " + TypeSignature(tt.PositionCurve.CurveData.Type)];
                    ++curveTypes["scale " + TypeSignature(tt.ScaleShearCurve.CurveData.Type)];
                }
                char flags[32];
                snprintf(flags, sizeof(flags), "trackgroup flags 0x%x", (unsigned)tg->Flags);
                ++sections[flags];
            }
        }
        if (g_verbose) printf("%s: models=%d meshes=%d anims=%d skeletons=%d\n", fn.c_str(), info->ModelCount, info->MeshCount, info->AnimationCount, info->SkeletonCount);
        GrannyFreeFile(file);
    }
    printf("files=%d ok=%d failed=%d models=%d meshes=%d animations=%d skeletons=%d textures=%d\n", files, ok, failed, models, meshes, anims, skeletons, textures);
    printf("\nvertex layouts:\n");
    for (auto &kv : vertexTypes)
        printf("  %6d  %s\n", kv.second, kv.first.c_str());
    printf("\ncurve data types:\n");
    for (auto &kv : curveTypes)
        printf("  %6d  %s\n", kv.second, kv.first.c_str());
    printf("\nmaterial extended data:\n");
    for (auto &kv : extendedData)
        printf("  %6d  %s\n", kv.second, kv.first.c_str());
    printf("\ntrack group flags:\n");
    for (auto &kv : sections)
        printf("  %6d  %s\n", kv.second, kv.first.c_str());
    printf("\nlog messages:\n");
    for (auto &kv : g_logMessages)
        printf("  %6d  %s\n", kv.second, kv.first.c_str());
    if (!failedFiles.empty()) {
        printf("\nfailed files (%zu):\n", failedFiles.size());
        int shown = 0;
        for (const std::string &f : failedFiles) {
            if (shown++ >= 50) {
                printf("  ...\n");
                break;
            }
            printf("  %s\n", f.c_str());
        }
    }
    return failed ? 2 : 0;
}
