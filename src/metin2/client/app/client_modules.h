#pragma once

namespace metin2::inline client::inline app {
    void init_app_module();
    void init_settings_module();
    void init_ime_module();
    void init_sound_module();
    void init_effects_module();
    void init_files_module();
}

namespace metin2::inline client::inline background {
    void init_world_module();
    void init_world_minimap_module();
}

namespace metin2::inline client::inline characters {
    void init_characters_module();
    void init_characters_manager_module();
    void init_characters_nonplayer_module();
}

namespace metin2::inline client::inline guilds {
    void init_guild_module();
}

namespace metin2::inline client::inline inventory {
    void init_items_module();
}

namespace metin2::inline client::inline player {
    void init_player_module();
    void init_player_skills_module();
}

namespace metin2::inline client::inline protocol {
    void init_net_module();
    void init_net_server_state_module();
}

namespace metin2::inline client::inline quest {
    void init_quest_module();
    void init_quest_events_module();
}

namespace metin2::inline client::inline social {
    void init_social_chat_module();
    void init_social_messenger_module();
    void init_social_text_tail_module();
}

namespace metin2::inline client::inline trade {
    void init_trade_exchange_module();
    void init_trade_shop_module();
    void init_trade_safebox_module();
}
