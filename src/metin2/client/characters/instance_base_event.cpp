#include "instance_base.h"

namespace metin2::inline client::inline characters {

    auto instance_base::event_handler_ref() -> actor_instance::event_handler & {
        return graphic_thing_instance_value_.event_handler_ref_impl();
    }

    void instance_base::set_event_handler(actor_instance::event_handler *event_handler) {
        graphic_thing_instance_value_.set_event_handler(event_handler);
    }
}
