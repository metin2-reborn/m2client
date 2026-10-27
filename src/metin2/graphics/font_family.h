#pragma once

#include "resource.h"

#include <metin2/core/ref.h>

#include <string>
#include <string_view>
#include <vector>

namespace metin2::inline graphics {

    // A font shipped in the game packs: the .ttf file of every style and weight it comes in. TOML,
    // [asset] type "Font":
    //
    //   [normal]
    //   [[normal.weights]]
    //   weight = 400
    //   path = "ui:fonts/PTSans-Regular.ttf"
    //   [[normal.weights]]
    //   weight = 700
    //   path = "ui:fonts/PTSans-Bold.ttf"
    //   [italic]
    //   [[italic.weights]]
    //   ...
    //
    // normal is required, italic optional. Text asks for a style and a weight (CSS numbers, 400
    // regular, 700 bold) and gets the file of the closest weight the style has.
    class font_family : public resource {
      public:
        using ref = ref_ptr<font_family>;

        struct weight_file {
            int weight = 400;
            std::string path;
        };
        struct table {
            std::vector<weight_file> normal; // sorted by weight
            std::vector<weight_file> italic; // sorted by weight; empty without an [italic] table
        };

        static auto type() -> type_type;
        // Parses the asset text; errors are logged with the file name and line.
        static auto parse(std::string_view text, std::string_view name, table &out) -> bool;
        // The file closest to the weight in the style (a tie goes to the heavier one); italic falls
        // back to normal when the font has no italic. Empty for a table without files.
        static auto file_for(const table &files, bool italic, int weight) -> const std::string &;

        explicit font_family(const char *file_name);

        auto file_for(bool italic, int weight) const -> const std::string & { return file_for(files_, italic, weight); }

      protected:
        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

      private:
        table files_;
    };
}
