/* Copyright (C) John W. Ratcliff, 2001.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) John W. Ratcliff, 2001"
 */
#pragma once

#include <memory>
#include <vector>

#include <cassert>
#include <cstdint>

#include "frustum.h"
#include "pool.h"
#include "sphere.h"
#include "vector.h"

namespace metin2::inline geometry {

    enum sphere_pack_flag {

        SPF_SUPERSPHERE = (1 << 0), // a supersphere, allocated and deleted by the factory
        SPF_ROOT_TREE = (1 << 1),
        SPF_LEAF_TREE = (1 << 2),
        SPF_ROOTNODE = (1 << 3),
        SPF_RECOMPUTE = (1 << 4), // bounding sphere must be recomputed
        SPF_INTEGRATE = (1 << 5), // must be reintegrated into the tree
        // Frame-to-frame view frustum status; callbacks happen only when it changes.
        SPF_HIDDEN = (1 << 6),
        SPF_PARTIAL = (1 << 7),
        SPF_INSIDE = (1 << 8)
    };

    class sphere_pack_factory;
    class sphere_pack;

    // Callbacks for ray tracing, range testing and frustum culling.
    class sphere_pack_callback {
      public:
        sphere_pack_callback() = default;
        virtual ~sphere_pack_callback() = default;

        virtual void visibility_callback(const frustum &f,
            sphere_pack *sphere, // leaf node sphere
            view_state state)    // its new state
        {};

        virtual void ray_trace_callback(const vector3d &p1,
            const vector3d &dir,
            float distance,
            const vector3d &sect, // intersection location
            sphere_pack *sphere) {};

        virtual void range_test_callback(const vector3d &searchpos,
            float distance, // squared
            sphere_pack *sphere,
            view_state state) // VS_PARTIAL if the sphere straddles the range
        {};

        virtual void point_test2d_callback(const vector3d &searchpos,
            sphere_pack *sphere,
            view_state state) // VS_PARTIAL if the sphere straddles the range
        {};

      private:
    };

    class sphere_pack : public sphere {
      public:
        sphere_pack(void) {
            m_user_data_ = nullptr;
            m_factory_ = nullptr;
            m_next_ = nullptr;
            m_previous_ = nullptr;
            m_parent_ = nullptr;
            m_next_sibling_ = nullptr;
            m_prev_sibling_ = nullptr;
            m_children_ = nullptr;
            m_child_count_ = 0;
            m_fifo1_ = nullptr;
            m_fifo2_ = nullptr;
            set_radius(0);
            m_center_.set(0, 0, 0);
            m_flags_ = 0;
            m_binding_distance_ = 0;
        };
        ~sphere_pack() override = default;

        void init(sphere_pack_factory *factory,
            const vector3d &pos,
            float radius,
            void *userdata) {
            m_user_data_ = userdata;
            m_parent_ = nullptr;
            m_next_sibling_ = nullptr;
            m_prev_sibling_ = nullptr;
            m_flags_ = 0;
            m_fifo1_ = nullptr;
            m_fifo2_ = nullptr;
            m_factory_ = factory;
            m_center_ = pos;
            set_radius(radius);
        };

        void set_sphere_pack_flag(sphere_pack_flag flag) { m_flags_ |= flag; };
        void clear_sphere_pack_flag(sphere_pack_flag flag) { m_flags_ &= ~flag; };
        auto has_sphere_pack_flag(sphere_pack_flag flag) const -> bool {
            if (m_flags_ & flag) return true;
            return false;
        };

        void set_parent(sphere_pack *pack) { m_parent_ = pack; };
        auto parent(void) const -> sphere_pack * { return m_parent_; };

        inline void new_pos(const vector3d &pos);
        inline void new_pos_radius(const vector3d &pos, float radius);

        void unlink(void) {
            if (m_fifo1_) // still queued in fifo1: clear our slot
            {
                *m_fifo1_ = nullptr;
                m_fifo1_ = nullptr;
            }

            if (m_fifo2_) {
                *m_fifo2_ = nullptr;
                m_fifo2_ = nullptr;
            }

            if (m_parent_) m_parent_->lost_child(this);

            assert(!m_children_);

            m_parent_ = nullptr;
        }

        void add_child(sphere_pack *pack) {

            sphere_pack *my_child = m_children_;
            m_children_ = pack;

            pack->set_next_sibling(my_child);
            pack->set_prev_sibling(nullptr);
            pack->set_parent(this);

            if (my_child) my_child->set_prev_sibling(pack);

            m_child_count_++;
        }

        void set_next_sibling(sphere_pack *child) { m_next_sibling_ = child; }
        void set_prev_sibling(sphere_pack *child) { m_prev_sibling_ = child; }

        auto next_sibling(void) const -> sphere_pack * {
            return m_next_sibling_;
        }
        auto prev_sibling(void) const -> sphere_pack * {
            return m_prev_sibling_;
        }
        auto children(void) const -> sphere_pack * { return m_children_; }

        auto next(void) const -> sphere_pack * { return m_next_; };
        auto previous(void) const -> sphere_pack * { return m_previous_; };

        void set_next(sphere_pack *pack) { m_next_ = pack; };
        void set_previous(sphere_pack *pack) { m_previous_ = pack; };

        auto user_data(void) const -> void * { return m_user_data_; };
        void set_user_data(void *data) { m_user_data_ = data; };

        auto distance_squared(const sphere_pack *pack) const -> float { return m_center_.distance_sq(pack->m_center_); };

        void lost_child(sphere_pack *pack);

        auto pos(void) const -> const vector3d & { return m_center_; };

        inline auto recompute(float gravy) -> bool;

        auto child_count(void) const -> int { return m_child_count_; };

        void set_fifo1(sphere_pack **fifo) {
            m_fifo1_ = fifo;
        };

        void set_fifo2(sphere_pack **fifo) {
            m_fifo2_ = fifo;
        };

        void compute_binding_distance(sphere_pack *parent) {
            m_binding_distance_ = parent->radius() - radius();
            if (m_binding_distance_ <= 0)
                m_binding_distance_ = 0;
            else
                m_binding_distance_ *= m_binding_distance_;
        }

        void visibility_test(const frustum &f,
            sphere_pack_callback *callback,
            view_state state);

        void ray_trace(const vector3d &p1,
            const vector3d &dir,
            float distance,
            sphere_pack_callback *callback);

        void range_test(const vector3d &p,
            float distance,
            sphere_pack_callback *callback,
            view_state state);

        void point_test2d(const vector3d &p,
            sphere_pack_callback *callback,
            view_state state);

        void reset(void);

      private:
        sphere_pack *m_next_;
        sphere_pack *m_previous_; // m_next_/m_previous_ link the pool's free and used lists

        sphere_pack *m_parent_;
        sphere_pack *m_children_;

        sphere_pack *m_next_sibling_;
        sphere_pack *m_prev_sibling_;

        sphere_pack **m_fifo1_; // our slot in fifo1, cleared on unlink
        sphere_pack **m_fifo2_;

        int32_t m_flags_;
        int32_t m_child_count_;

        float m_binding_distance_;

        void *m_user_data_;

        sphere_pack_factory *m_factory_;
    };

    class sphere_pack_fifo {
      public:
        sphere_pack_fifo(int fifosize) {
            assert(fifosize > 0);
            m_count_ = 0;
            m_sp_ = 0;
            m_bottom_ = 0;
            m_fifo_size_ = fifosize;
            m_fifo_.resize(m_fifo_size_);
        };

        virtual ~sphere_pack_fifo(void) {
        };

        auto push(sphere_pack *sphere) -> sphere_pack ** {
            assert(m_count_ < m_fifo_size_ && "sphere fifo overflow");
            m_count_++;
            sphere_pack **ret = &m_fifo_[m_sp_];
            m_fifo_[m_sp_] = sphere;
            m_sp_++;
            if (m_sp_ == m_fifo_size_) m_sp_ = 0;
            return ret;
        };

        auto pop(void) -> sphere_pack * {
            while (m_sp_ != m_bottom_) {
                m_count_--;
                sphere_pack *ret = m_fifo_[m_bottom_];
                m_bottom_++;
                if (m_bottom_ == m_fifo_size_) m_bottom_ = 0;
                if (ret) return ret;
            }
            return nullptr;
        }

        auto count(void) const -> int { return m_count_; };

      private:
        int m_count_;
        int m_sp_;
        int m_bottom_;
        int m_fifo_size_;
        std::vector<sphere_pack *> m_fifo_;
    };

    class sphere_pack_factory : public sphere_pack_callback {
      public:
        sphere_pack_factory(int maxspheres,
            float rootsize,
            float leafsize,
            float gravy);

        ~sphere_pack_factory(void) override;

        void process(void);

        auto add_sphere(const vector3d &pos,
            float radius,
            void *userdata,
            int flags = SPF_LEAF_TREE) -> sphere_pack *;

        void add_integrate(sphere_pack *pack);
        void add_recompute(sphere_pack *recompute); // queues a supersphere for rebalancing

        void integrate(sphere_pack *pack, sphere_pack *supersphere, float node_size);

        void remove(sphere_pack *pack);

        void frustum_test(const frustum &f, sphere_pack_callback *callback);

        void ray_trace(const vector3d &p1, // source
            const vector3d &p2,            // ray vector (direction times length), not an end point
            sphere_pack_callback *callback);

        void range_test(const vector3d &center, float radius, sphere_pack_callback *callback);
        void point_test2d(const vector3d &center, sphere_pack_callback *callback);

        void ray_trace_callback(const vector3d &p1,
            const vector3d &dir,
            float distance,
            const vector3d &sect,
            sphere_pack *sphere) override;

        void range_test_callback(const vector3d &p, float distance, sphere_pack *sphere, view_state state) override;
        void point_test2d_callback(const vector3d &p, sphere_pack *sphere, view_state state) override;

        void visibility_callback(const frustum &f, sphere_pack *sphere, view_state state) override;

        void reset(void);

      private:
        sphere_pack *m_root_;
        sphere_pack *m_leaf_;
        sphere_pack_callback *m_callback_;

        pool<sphere_pack> m_spheres_;

        std::unique_ptr<sphere_pack_fifo> m_integrate_;
        std::unique_ptr<sphere_pack_fifo> m_recompute_;

        float m_max_root_size_;
        float m_max_leaf_size_;
        float m_super_sphere_gravy_; // extra radius added to superspheres so members can move a little
    };

    void sphere_pack::new_pos(const vector3d &pos) {
        m_center_ = pos;

        if (m_parent_ && !has_sphere_pack_flag(SPF_INTEGRATE)) {
            float dist = distance_squared(m_parent_);

            if (dist >= m_binding_distance_) // squared, like dist
            {
                // Moved out of the parent supersphere: rebalance the parent and reintegrate this sphere.
                m_factory_->add_recompute(m_parent_);

                unlink();
                m_factory_->add_integrate(this);
            }
        }
    }

    void sphere_pack::new_pos_radius(const vector3d &pos, float radius) {
        m_center_ = pos;

        if (m_parent_ && !has_sphere_pack_flag(SPF_INTEGRATE)) {
            if (radius != this->radius()) {
                set_radius(radius);
                compute_binding_distance(m_parent_);
            }

            m_factory_->add_recompute(m_parent_);

            float dist = distance_squared(m_parent_);

            if (dist >= m_binding_distance_) {
                unlink();
                m_factory_->add_integrate(this);
            }
        }
    }
}
