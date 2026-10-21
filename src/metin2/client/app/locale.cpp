#include "locale.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace metin2::inline client::inline app {

    namespace {
        const char *service_name_europe = "EUROPE";

        // English is the only locale; scripts may still switch the data folder (app.ForceSetLocale).
        char multi_locale_path[264] = "locale/en";
        char multi_locale_name[256] = "en";
    }

    auto locale_service_get_last_exp(int level) -> unsigned {
        static const int guild_level_max = 20;

        static uint32_t international_guildexp_list[guild_level_max + 1] =
            {
                0,         // 0
                6000UL,    // 1
                18000UL,   // 2
                36000UL,   // 3
                64000UL,   // 4
                94000UL,   // 5
                130000UL,  // 6
                172000UL,  // 7
                220000UL,  // 8
                274000UL,  // 9
                334000UL,  // 10
                400000UL,  // 11
                600000UL,  // 12
                840000UL,  // 13
                1120000UL, // 14
                1440000UL, // 15
                1800000UL, // 16
                2600000UL, // 17
                3200000UL, // 18
                4000000UL, // 19
                16800000UL // 20
        };

        if (level < 0 || level > guild_level_max)
            return 0;

        return international_guildexp_list[level];
    }

    auto locale_service_get_skill_power(unsigned level) -> int {
        static const unsigned skill_power_num = 50;

        if (level >= skill_power_num)
            return 0;

        static unsigned international_skill_powers[skill_power_num] =
            {
                0,
                5,
                6,
                8,
                10,
                12,
                14,
                16,
                18,
                20,
                22,
                24,
                26,
                28,
                30,
                32,
                34,
                36,
                38,
                40,
                50, // master
                52,
                54,
                56,
                58,
                60,
                63,
                66,
                69,
                72,
                82, // grand_master
                85,
                88,
                91,
                94,
                98,
                102,
                106,
                110,
                115,
                125, // perfect_master
                125,
        };
        return international_skill_powers[level];
    }

    auto locale_service_get_name() -> const char * { return service_name_europe; }
    auto locale_service_get_locale_path() -> const char * { return multi_locale_path; }

    void locale_service_force_set_locale(const char *name, const char *locale_path) {
        snprintf(multi_locale_name, sizeof(multi_locale_name), "%s", name);
        snprintf(multi_locale_path, sizeof(multi_locale_path), "%s", locale_path);
    }
}
