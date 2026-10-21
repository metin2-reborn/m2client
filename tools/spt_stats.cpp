// spt_stats: generate the trees of .spt files and print what comes out (vertex/index counts, leaf
// count, bounding box, collision objects, texture names).
//   build/bin/spt_stats <file.spt>...   (extract trees from the packs with: just grn-dump --extract <name> <out>)
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

#include <metin2/trees/spt_tree.h>

int main(int argc, char **argv) {
    const float direction[3] = {0, 1, -0.3f};
    int fails = 0;
    for (int i = 1; i < argc; ++i) {
        std::ifstream file(argv[i], std::ios::binary);
        const std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        auto tree = metin2::spt_tree::load(data);
        if (!tree) {
            printf("LOADFAIL %s: %s\n", argv[i], tree.error().c_str());
            ++fails;
            continue;
        }
        tree->face_leaves(direction);
        const float (&b)[6] = tree->bounds();
        const char *n = strrchr(argv[i], '/');
        printf("%-34s br v=%5zu i=%5zu  fr v=%5zu i=%5zu  leaves=%4zu  bbox w=%5.0f d=%5.0f h=%5.0f  coll=%zu  %s | %s\n", n ? n + 1 : argv[i],
            tree->branches().vertices.size(), tree->branches().strip.size(), tree->fronds().vertices.size(), tree->fronds().strip.size(),
            tree->leaves().size(), b[3] - b[0], b[4] - b[1], b[5] - b[2], tree->collision().size(), tree->bark_texture().c_str(),
            tree->composite_texture().empty() ? "-" : tree->composite_texture().c_str());
    }
    printf("fails %d\n", fails);
    return fails ? 1 : 0;
}
