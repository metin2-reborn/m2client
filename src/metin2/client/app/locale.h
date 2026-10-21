#pragma once

namespace metin2::inline client::inline app {

    // The locale only picks the data folder (locale/en); all text is UTF-8.
    auto locale_service_get_name() -> const char *;
    auto locale_service_get_locale_path() -> const char *;

    void locale_service_force_set_locale(const char *name, const char *locale_path);
    auto locale_service_get_last_exp(int level) -> unsigned;
    auto locale_service_get_skill_power(unsigned level) -> int;
}
