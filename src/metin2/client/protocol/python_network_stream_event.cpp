#include <metin2/script/python_utils.h>
#include "python_network_stream.h"

namespace metin2::inline client::inline protocol {

    void python_network_stream::on_remote_disconnect() {
        py_call_class_member_func(handler_, "set_login_phase", Py_BuildValue("()"));
    }

    void python_network_stream::on_disconnect() {
    }

    void python_network_stream::on_script_event_start(int skin, int index) {
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "open_quest_window", Py_BuildValue("(ii)", skin, index));
    }
}
