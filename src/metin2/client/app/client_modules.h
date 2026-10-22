#pragma once

// The client's Python modules, all registered under the root package `metin2` (see
// script/python_module.h). The ui and script modules are declared in ui/python_modules.h and
// script/python_debug_module.h.
namespace metin2::inline client::inline app {
    void init_app_module();      // metin2.app
    void init_settings_module(); // metin2.settings
    void init_ime_module();      // metin2.ime
    void init_sound_module();    // metin2.sound
    void init_effects_module();  // metin2.effects
    void init_files_module();    // metin2.files
}
namespace metin2::inline client::inline background {
    void init_world_module();         // metin2.world
    void init_world_minimap_module(); // metin2.world.minimap
}
namespace metin2::inline client::inline characters {
    void init_characters_module();           // metin2.characters
    void init_characters_manager_module();   // metin2.characters.manager
    void init_characters_nonplayer_module(); // metin2.characters.nonplayer
}
namespace metin2::inline client::inline guilds {
    void init_guild_module(); // metin2.guild
}
namespace metin2::inline client::inline inventory {
    void init_items_module(); // metin2.items
}
namespace metin2::inline client::inline player {
    void init_player_module();        // metin2.player
    void init_player_skills_module(); // metin2.player.skills
}
namespace metin2::inline client::inline protocol {
    void init_net_module();              // metin2.net
    void init_net_server_state_module(); // metin2.net.server_state
}
namespace metin2::inline client::inline quest {
    void init_quest_module();        // metin2.quest
    void init_quest_events_module(); // metin2.quest.events
}
namespace metin2::inline client::inline social {
    void init_social_chat_module();      // metin2.social.chat
    void init_social_messenger_module(); // metin2.social.messenger
    void init_social_text_tail_module(); // metin2.social.text_tail
}
namespace metin2::inline client::inline trade {
    void init_trade_exchange_module(); // metin2.trade.exchange
    void init_trade_shop_module();     // metin2.trade.shop
    void init_trade_safebox_module();  // metin2.trade.safebox
}
