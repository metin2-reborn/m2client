#include "motion.h"
#include <cassert>

namespace metin2::inline models {

    motion::motion() {
        initialize();
    }

    motion::~motion() {
        destroy();
    }

    auto motion::is_empty() -> bool {
        return pgrn_ani_ ? false : true;
    }

    void motion::destroy() {
        initialize();
    }

    void motion::initialize() {
        pgrn_ani_ = nullptr;
    }

    auto motion::bind_granny_animation(granny_animation *pgrn_ani) -> bool {
        assert(is_empty());

        pgrn_ani_ = pgrn_ani;
        return true;
    }

    auto motion::granny_animation_pointer() const -> granny_animation * {
        return pgrn_ani_;
    }

    auto motion::get_duration() const -> float {
        return pgrn_ani_->Duration;
    }
}
