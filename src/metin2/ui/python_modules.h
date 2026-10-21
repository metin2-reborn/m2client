#pragma once

namespace metin2::inline ui {

    // Registration of the Python modules implemented in ui/ (called once at startup).
    void initgrp();       // grp       python_graphic_module.cpp
    void initgrp_image(); // grpImage  python_graphic_image_module.cpp
    void initgrp_text();  // grpText   python_graphic_text_module.cpp
    void initwnd_mgr();   // wndMgr    python_window_manager_module.cpp
}
