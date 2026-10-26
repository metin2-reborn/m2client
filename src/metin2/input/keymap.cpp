#include "keymap.h"
#include "keys.h"

#include <raylib.h>

#include <metin2/core/string_case.h>

#include <charconv>
#include <string>

namespace metin2::inline input {

    namespace {
        struct key_entry {
            int raylib_key;
            int dik;
            int vk;
            const char *name;
        };

        const key_entry key_table[] = {
            {KEY_ESCAPE,        DIK_ESCAPE,       0x1B, "ESCAPE"      },
            {KEY_ONE,           DIK_1,            '1',  "1"           },
            {KEY_TWO,           DIK_2,            '2',  "2"           },
            {KEY_THREE,         DIK_3,            '3',  "3"           },
            {KEY_FOUR,          DIK_4,            '4',  "4"           },
            {KEY_FIVE,          DIK_5,            '5',  "5"           },
            {KEY_SIX,           DIK_6,            '6',  "6"           },
            {KEY_SEVEN,         DIK_7,            '7',  "7"           },
            {KEY_EIGHT,         DIK_8,            '8',  "8"           },
            {KEY_NINE,          DIK_9,            '9',  "9"           },
            {KEY_ZERO,          DIK_0,            '0',  "0"           },
            {KEY_MINUS,         DIK_MINUS,        0xBD, "MINUS"       },
            {KEY_EQUAL,         DIK_EQUALS,       0xBB, "EQUALS"      },
            {KEY_BACKSPACE,     DIK_BACK,         0x08, "BACK"        },
            {KEY_TAB,           DIK_TAB,          0x09, "TAB"         },
            {KEY_Q,             DIK_Q,            'Q',  "Q"           },
            {KEY_W,             DIK_W,            'W',  "W"           },
            {KEY_E,             DIK_E,            'E',  "E"           },
            {KEY_R,             DIK_R,            'R',  "R"           },
            {KEY_T,             DIK_T,            'T',  "T"           },
            {KEY_Y,             DIK_Y,            'Y',  "Y"           },
            {KEY_U,             DIK_U,            'U',  "U"           },
            {KEY_I,             DIK_I,            'I',  "I"           },
            {KEY_O,             DIK_O,            'O',  "O"           },
            {KEY_P,             DIK_P,            'P',  "P"           },
            {KEY_LEFT_BRACKET,  DIK_LBRACKET,     0xDB, "LBRACKET"    },
            {KEY_RIGHT_BRACKET, DIK_RBRACKET,     0xDD, "RBRACKET"    },
            {KEY_ENTER,         DIK_RETURN,       0x0D, "RETURN"      },
            {KEY_LEFT_CONTROL,  DIK_LCONTROL,     0x11, "LCONTROL"    },
            {KEY_A,             DIK_A,            'A',  "A"           },
            {KEY_S,             DIK_S,            'S',  "S"           },
            {KEY_D,             DIK_D,            'D',  "D"           },
            {KEY_F,             DIK_F,            'F',  "F"           },
            {KEY_G,             DIK_G,            'G',  "G"           },
            {KEY_H,             DIK_H,            'H',  "H"           },
            {KEY_J,             DIK_J,            'J',  "J"           },
            {KEY_K,             DIK_K,            'K',  "K"           },
            {KEY_L,             DIK_L,            'L',  "L"           },
            {KEY_SEMICOLON,     DIK_SEMICOLON,    0xBA, "SEMICOLON"   },
            {KEY_APOSTROPHE,    DIK_APOSTROPHE,   0xDE, "APOSTROPHE"  },
            {KEY_GRAVE,         DIK_GRAVE,        0xC0, "GRAVE"       },
            {KEY_LEFT_SHIFT,    DIK_LSHIFT,       0x10, "LSHIFT"      },
            {KEY_BACKSLASH,     DIK_BACKSLASH,    0xDC, "BACKSLASH"   },
            {KEY_Z,             DIK_Z,            'Z',  "Z"           },
            {KEY_X,             DIK_X,            'X',  "X"           },
            {KEY_C,             DIK_C,            'C',  "C"           },
            {KEY_V,             DIK_V,            'V',  "V"           },
            {KEY_B,             DIK_B,            'B',  "B"           },
            {KEY_N,             DIK_N,            'N',  "N"           },
            {KEY_M,             DIK_M,            'M',  "M"           },
            {KEY_COMMA,         DIK_COMMA,        0xBC, "COMMA"       },
            {KEY_PERIOD,        DIK_PERIOD,       0xBE, "PERIOD"      },
            {KEY_SLASH,         DIK_SLASH,        0xBF, "SLASH"       },
            {KEY_RIGHT_SHIFT,   DIK_RSHIFT,       0x10, "RSHIFT"      },
            {KEY_KP_MULTIPLY,   DIK_MULTIPLY,     0x6A, "MULTIPLY"    },
            {KEY_LEFT_ALT,      DIK_LMENU,        0x12, "LMENU"       },
            {KEY_SPACE,         DIK_SPACE,        0x20, "SPACE"       },
            {KEY_CAPS_LOCK,     DIK_CAPITAL,      0x14, "CAPITAL"     },
            {KEY_F1,            DIK_F1,           0x70, "F1"          },
            {KEY_F2,            DIK_F2,           0x71, "F2"          },
            {KEY_F3,            DIK_F3,           0x72, "F3"          },
            {KEY_F4,            DIK_F4,           0x73, "F4"          },
            {KEY_F5,            DIK_F5,           0x74, "F5"          },
            {KEY_F6,            DIK_F6,           0x75, "F6"          },
            {KEY_F7,            DIK_F7,           0x76, "F7"          },
            {KEY_F8,            DIK_F8,           0x77, "F8"          },
            {KEY_F9,            DIK_F9,           0x78, "F9"          },
            {KEY_F10,           DIK_F10,          0x79, "F10"         },
            {KEY_NUM_LOCK,      DIK_NUMLOCK,      0x90, "NUMLOCK"     },
            {KEY_SCROLL_LOCK,   DIK_SCROLL,       0x91, "SCROLL"      },
            {KEY_KP_7,          DIK_NUMPAD7,      0x67, "NUMPAD7"     },
            {KEY_KP_8,          DIK_NUMPAD8,      0x68, "NUMPAD8"     },
            {KEY_KP_9,          DIK_NUMPAD9,      0x69, "NUMPAD9"     },
            {KEY_KP_SUBTRACT,   DIK_SUBTRACT,     0x6D, "SUBTRACT"    },
            {KEY_KP_4,          DIK_NUMPAD4,      0x64, "NUMPAD4"     },
            {KEY_KP_5,          DIK_NUMPAD5,      0x65, "NUMPAD5"     },
            {KEY_KP_6,          DIK_NUMPAD6,      0x66, "NUMPAD6"     },
            {KEY_KP_ADD,        DIK_ADD,          0x6B, "ADD"         },
            {KEY_KP_1,          DIK_NUMPAD1,      0x61, "NUMPAD1"     },
            {KEY_KP_2,          DIK_NUMPAD2,      0x62, "NUMPAD2"     },
            {KEY_KP_3,          DIK_NUMPAD3,      0x63, "NUMPAD3"     },
            {KEY_KP_0,          DIK_NUMPAD0,      0x60, "NUMPAD0"     },
            {KEY_KP_DECIMAL,    DIK_DECIMAL,      0x6E, "DECIMAL"     },
            {KEY_F11,           DIK_F11,          0x7A, "F11"         },
            {KEY_F12,           DIK_F12,          0x7B, "F12"         },
            {KEY_KP_EQUAL,      DIK_NUMPADEQUALS, 0xBB, "NUMPADEQUALS"},
            {KEY_KP_ENTER,      DIK_NUMPADENTER,  0x0D, "NUMPADENTER" },
            {KEY_RIGHT_CONTROL, DIK_RCONTROL,     0x11, "RCONTROL"    },
            {KEY_KP_DIVIDE,     DIK_DIVIDE,       0x6F, "DIVIDE"      },
            {KEY_PRINT_SCREEN,  DIK_SYSRQ,        0x2C, "SYSRQ"       },
            {KEY_RIGHT_ALT,     DIK_RMENU,        0x12, "RMENU"       },
            {KEY_PAUSE,         DIK_PAUSE,        0x13, "PAUSE"       },
            {KEY_HOME,          DIK_HOME,         0x24, "HOME"        },
            {KEY_UP,            DIK_UP,           0x26, "UP"          },
            {KEY_PAGE_UP,       DIK_PRIOR,        0x21, "PRIOR"       },
            {KEY_LEFT,          DIK_LEFT,         0x25, "LEFT"        },
            {KEY_RIGHT,         DIK_RIGHT,        0x27, "RIGHT"       },
            {KEY_END,           DIK_END,          0x23, "END"         },
            {KEY_DOWN,          DIK_DOWN,         0x28, "DOWN"        },
            {KEY_PAGE_DOWN,     DIK_NEXT,         0x22, "NEXT"        },
            {KEY_INSERT,        DIK_INSERT,       0x2D, "INSERT"      },
            {KEY_DELETE,        DIK_DELETE,       0x2E, "DELETE"      },
            {KEY_LEFT_SUPER,    DIK_LWIN,         0x5B, "LWIN"        },
            {KEY_RIGHT_SUPER,   DIK_RWIN,         0x5C, "RWIN"        },
            {KEY_KB_MENU,       DIK_APPS,         0x5D, "APPS"        },
        };

        struct key_alias {
            const char *name;
            int dik;
        };

        const key_alias key_aliases[] = {
            {"ESC",          DIK_ESCAPE  },
            {"BACKSPACE",    DIK_BACK    },
            {"LALT",         DIK_LMENU   },
            {"RALT",         DIK_RMENU   },
            {"CAPSLOCK",     DIK_CAPITAL },
            {"PGUP",         DIK_PRIOR   },
            {"PGDN",         DIK_NEXT    },
            {"UPARROW",      DIK_UP      },
            {"DOWNARROW",    DIK_DOWN    },
            {"LEFTARROW",    DIK_LEFT    },
            {"RIGHTARROW",   DIK_RIGHT   },
            {"ENTER",        DIK_RETURN  },
            {"NUMPADSTAR",   DIK_MULTIPLY},
            {"NUMPADMINUS",  DIK_SUBTRACT},
            {"NUMPADPLUS",   DIK_ADD     },
            {"NUMPADPERIOD", DIK_DECIMAL },
            {"NUMPADSLASH",  DIK_DIVIDE  },
        };
    }

    auto dik_to_raylib(int dik) -> int {
        for (const key_entry &entry : key_table)
            if (entry.dik == dik)
                return entry.raylib_key;
        return 0;
    }

    auto dik_to_vk(int dik) -> int {
        for (const key_entry &entry : key_table)
            if (entry.dik == dik)
                return entry.vk;
        return 0;
    }

    auto raylib_to_vk(int key) -> int {
        for (const key_entry &entry : key_table)
            if (entry.raylib_key == key)
                return entry.vk;
        return 0;
    }

    auto dik_from_name(std::string_view name) -> int {
        if (name.empty())
            return 0;

        int value = 0;
        if (name.starts_with("0x") || name.starts_with("0X")) {
            std::from_chars(name.data() + 2, name.data() + name.size(), value, 16);
            return value;
        }

        if (name.starts_with("DIK_"))
            name.remove_prefix(4);

        const std::string key(name);
        for (const key_entry &entry : key_table)
            if (iequals(entry.name, key.c_str()))
                return entry.dik;
        for (const key_alias &alias : key_aliases)
            if (iequals(alias.name, key.c_str()))
                return alias.dik;

        const auto [end, ec] = std::from_chars(name.data(), name.data() + name.size(), value);
        return ec == std::errc{} && end == name.data() + name.size() && value > 0 && value < 256 ? value : 0;
    }
}
