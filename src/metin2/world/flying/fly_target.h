#pragma once
#include <metin2/math/math.h>
#include <set>
#include <string>

namespace metin2::inline world::inline flying {

    class fly_target;

    class fly_targetable_object {
        friend class fly_target;

      public:
        fly_targetable_object() = default;
        virtual ~fly_targetable_object() = default;

        virtual auto on_get_fly_target_position() -> math::vector3 = 0;
        virtual void on_shoot_damage() = 0;

      protected:
        inline void clear_fly_targeter();

      private:
        std::set<fly_target *> fly_targeter_set_;
        inline void add_fly_targeter(fly_target *targeter) {
            fly_targeter_set_.insert(targeter);
        }
        inline void remove_fly_targeter(fly_target *targeter) {
            fly_targeter_set_.erase(targeter);
        }
    };

    class fly_target {
      public:
        enum type {
            TYPE_NONE,
            TYPE_OBJECT,
            TYPE_POSITION,
        };

      public:
        fly_target();
        fly_target(fly_targetable_object *fly_target);
        fly_target(const math::vector3 &fly_target_position);
        fly_target(const fly_target &rhs);
        virtual ~fly_target();

        void clear();
        auto is_object() -> bool;
        auto is_position() -> bool;
        auto is_valid_target() -> bool;
        void notify_target_clear();

        auto fly_target_position() const -> const math::vector3 &;
        auto get_type() -> type;

        auto get_fly_target() -> fly_targetable_object *;

        auto operator=(const fly_target &rhs) -> fly_target &;

      private:
        void initialize_impl();

      private:
        mutable math::vector3 fly_target_position_;
        fly_targetable_object *fly_target_value_;

        type type_value_;
    };

    inline void fly_targetable_object::clear_fly_targeter() {
        std::set<fly_target *>::iterator it;
        for (it = fly_targeter_set_.begin(); it != fly_targeter_set_.end(); ++it) {
            (*it)->notify_target_clear();
        }
        fly_targeter_set_.clear();
    }
}
