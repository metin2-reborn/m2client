// Resource IDs of the cursors, the icon and the application string table (from the Windows resource script).
#pragma once

#include <cstdint>

namespace metin2::inline client::inline app {

#define IDS_APP_NAME             1
#define IDS_POSSESSIVE_MORPHENE  2
#define IDS_WARN_BAD_DRIVER      3
#define IDS_WARN_NO_TNL          4
#define IDI_METIN2               100
#define IDC_CURSOR_NORMAL        102
#define IDC_CURSOR_ATTACK        104
#define IDC_CURSOR_CHAIR         105
#define IDC_CURSOR_DOOR          106
#define IDC_CURSOR_NO            107
#define IDC_CURSOR_PICK          108
#define IDC_CURSOR_TALK          109
#define IDC_CURSOR_BUY           110
#define IDC_CURSOR_SELL          111
#define IDC_CURSOR_PICKUP        114
#define IDC_CURSOR_CAMERA_ROTATE 139
#define IDC_CURSOR_HSIZE         140
#define IDC_CURSOR_VSIZE         141
#define IDC_CURSOR_HVSIZE        142

    // English text for the string table IDs above (user_interface.cpp).
    auto application_string_table_get_stringz(uint32_t id) -> const char *;
}
