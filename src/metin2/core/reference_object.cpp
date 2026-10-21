#include "reference_object.h"
#include <cassert>

namespace metin2::inline core {

    reference_object::reference_object() : ref_count_(0), destructed_(false) {
    }

    reference_object::~reference_object() = default;

    void reference_object::add_reference() {
        if (ref_count_ == 0)
            on_construct();

        ++ref_count_;
    }

    auto reference_object::reference_count() -> int {
        return ref_count_;
    }

    void reference_object::add_reference_only() {
        ++ref_count_;
    }

    void reference_object::release() {
        if (ref_count_ > 1) {
            --ref_count_;
            return;
        }

        assert(destructed_ == false);
        assert(ref_count_ >= 0);
        ref_count_ = 0;
        on_self_destruct();
    }

    void reference_object::on_construct() {
        destructed_ = false;
    }

    void reference_object::on_self_destruct() {
        destructed_ = true;
        delete this;
    }

    auto reference_object::can_destroy() -> bool {
        if (ref_count_ > 0)
            return false;

        return true;
    }
}
