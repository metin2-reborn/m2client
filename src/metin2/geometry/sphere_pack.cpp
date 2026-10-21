#include "sphere_pack.h"
#include <cassert>
#include <cmath>
#include <metin2/log/log.h>

namespace metin2::inline geometry {
    /* Copyright (C) John W. Ratcliff, 2001.
     * All rights reserved worldwide.
     *
     * This software is provided "as is" without express or implied
     * warranties. You may freely copy and compile this source into
     * applications you distribute provided that the copyright text
     *  below is included in the resulting source code, for example:
     * "Portions Copyright (C) John W. Ratcliff, 2001"
     */

    sphere_pack_factory::sphere_pack_factory(int maxspheres, float rootsize, float leafsize, float gravy) {
        maxspheres *= 4; // room for both trees (root and leaf) and their superspheres
        m_max_root_size_ = rootsize;
        m_max_leaf_size_ = leafsize;
        m_super_sphere_gravy_ = gravy;
        m_integrate_ = std::make_unique<sphere_pack_fifo>(maxspheres);
        m_recompute_ = std::make_unique<sphere_pack_fifo>(maxspheres);

        m_spheres_.set(maxspheres);

        vector3d p(0, 0, 0);

        m_root_ = m_spheres_.free_link();
        m_root_->init(this, p, 6553600, nullptr);
        m_root_->set_sphere_pack_flag(sphere_pack_flag(SPF_SUPERSPHERE | SPF_ROOTNODE | SPF_ROOT_TREE));

        m_leaf_ = m_spheres_.free_link();
        m_leaf_->init(this, p, 1638400, nullptr);
        m_leaf_->set_sphere_pack_flag(sphere_pack_flag(SPF_SUPERSPHERE | SPF_ROOTNODE | SPF_LEAF_TREE));
    }

    sphere_pack_factory::~sphere_pack_factory(void) {
    }

    void sphere_pack_factory::process(void) {
        {
            // Spheres that left their parent queued it for rebalancing; a parent left empty is removed.
            int maxrecompute = m_recompute_->count();
            for (int i = 0; i < maxrecompute; ++i) {
                sphere_pack *pack = m_recompute_->pop();
                if (!pack) break;
                pack->set_fifo1(nullptr);
                bool kill = pack->recompute(m_super_sphere_gravy_);
                if (kill) remove(pack);
            }
        }

        {
            int maxintegrate = m_integrate_->count();

            for (int i = 0; i < maxintegrate; ++i) {
                sphere_pack *pack = m_integrate_->pop();
                if (!pack)
                    break;
                pack->set_fifo2(nullptr);

                if (pack->has_sphere_pack_flag(SPF_ROOT_TREE))
                    integrate(pack, m_root_, m_max_root_size_);
                else
                    integrate(pack, m_leaf_, m_max_leaf_size_);
            }
        }
    }

    auto sphere_pack_factory::add_sphere(const vector3d &pos,
        float radius,
        void *userdata,
        int flags) -> sphere_pack * {

        sphere_pack *pack = m_spheres_.free_link();

        assert(pack);

        if (pack) {
            if (flags & SPF_ROOT_TREE) {
                pack->init(this, pos, radius, userdata);
                pack->set_sphere_pack_flag(SPF_ROOT_TREE);
                add_integrate(pack);
            } else {
                pack->init(this, pos, radius, userdata);
                pack->set_sphere_pack_flag(SPF_LEAF_TREE);
                add_integrate(pack);
            }
        }

        return pack;
    }

    void sphere_pack_factory::add_integrate(sphere_pack *pack) {
        if (pack->has_sphere_pack_flag(SPF_ROOT_TREE))
            m_root_->add_child(pack);
        else
            m_leaf_->add_child(pack);

        pack->set_sphere_pack_flag(SPF_INTEGRATE);
        sphere_pack **fifo = m_integrate_->push(pack);
        pack->set_fifo2(fifo);
    }

    void sphere_pack_factory::add_recompute(sphere_pack *recompute) {
        if (!recompute->has_sphere_pack_flag(SPF_RECOMPUTE)) {
            if (recompute->child_count()) {
                recompute->set_sphere_pack_flag(SPF_RECOMPUTE);
                sphere_pack **fifo = m_recompute_->push(recompute);
                recompute->set_fifo1(fifo);
            } else {
                remove(recompute);
            }
        }
    }

    auto sphere_pack::recompute(float gravy) -> bool {
        if (!m_children_) return true; // empty: the caller removes it
        if (has_sphere_pack_flag(SPF_ROOTNODE)) return false;

        clear_sphere_pack_flag(SPF_RECOMPUTE);

        return false;
    }

    void sphere_pack::lost_child(sphere_pack *t) {
        assert(m_child_count_);
        assert(m_children_);

        sphere_pack *prev = t->prev_sibling();

        if (prev) {
            sphere_pack *next = t->next_sibling();
            prev->set_next_sibling(next);
            if (next) next->set_prev_sibling(prev);
        } else {
            sphere_pack *next = t->next_sibling();
            m_children_ = next;
            if (m_children_) m_children_->set_prev_sibling(nullptr);
        }

        m_child_count_--;

        if (!m_child_count_ && has_sphere_pack_flag(SPF_SUPERSPHERE)) {
            m_factory_->remove(this);
        }
    }

    void sphere_pack_factory::remove(sphere_pack *pack) {

        if (pack->has_sphere_pack_flag(SPF_ROOTNODE)) return; // the root nodes live as long as the factory

        if (pack->has_sphere_pack_flag(SPF_SUPERSPHERE) && pack->has_sphere_pack_flag(SPF_LEAF_TREE)) {
            auto *link = (sphere_pack *)pack->user_data();

            remove(link);
        }

        pack->unlink();

        m_spheres_.release(pack);
    }

    void sphere_pack_factory::integrate(sphere_pack *pack,
        sphere_pack *supersphere,
        float node_size) {
        sphere_pack *search = supersphere->children();

        sphere_pack *nearest1 = nullptr; // nearest supersphere that fully encloses pack
        float neardist1 = 1e38f;

        sphere_pack *nearest2 = nullptr; // supersphere that must grow the least to take pack
        float neardist2 = 1e38f;

        while (search) {
            if (search->has_sphere_pack_flag(SPF_SUPERSPHERE) && !search->has_sphere_pack_flag(SPF_ROOTNODE) && search->child_count()) {

                float dist = pack->distance_squared(search);

                if (nearest1) {
                    if (dist < neardist1) {

                        float d = sqrtf(dist) + pack->radius();

                        if (d <= search->radius()) {
                            neardist1 = dist;
                            nearest1 = search;
                        }
                    }
                } else {

                    float d = (sqrtf(dist) + pack->radius()) - search->radius();

                    if (d < neardist2) {
                        if (d < 0) {
                            neardist1 = dist;
                            nearest1 = search;
                        } else {
                            neardist2 = d;
                            nearest2 = search;
                        }
                    }
                }
            }
            search = search->next_sibling();
        }

        if (nearest1) {
            pack->unlink();
            nearest1->add_child(pack);
            pack->compute_binding_distance(nearest1);
            nearest1->recompute(m_super_sphere_gravy_);

            if (nearest1->has_sphere_pack_flag(SPF_LEAF_TREE)) {
                auto *link = (sphere_pack *)nearest1->user_data();
                link->new_pos_radius(nearest1->pos(), nearest1->radius());
            }

        } else {
            bool newsphere = true;

            if (nearest2) {
                float newsize = neardist2 + nearest2->radius() + m_super_sphere_gravy_;

                if (newsize <= node_size) {
                    pack->unlink();

                    nearest2->set_radius(newsize);
                    nearest2->add_child(pack);
                    nearest2->recompute(m_super_sphere_gravy_);
                    pack->compute_binding_distance(nearest2);

                    if (nearest2->has_sphere_pack_flag(SPF_LEAF_TREE)) {
                        auto *link = (sphere_pack *)nearest2->user_data();
                        link->new_pos_radius(nearest2->pos(), nearest2->radius());
                    }

                    newsphere = false;
                }
            }

            if (newsphere) {
                assert(supersphere->has_sphere_pack_flag(SPF_ROOTNODE));
                pack->unlink();

                sphere_pack *parent = m_spheres_.free_link();
                assert(parent);
                parent->init(this, pack->pos(), pack->radius() + m_super_sphere_gravy_, nullptr);

                if (supersphere->has_sphere_pack_flag(SPF_ROOT_TREE))
                    parent->set_sphere_pack_flag(SPF_ROOT_TREE);
                else
                    parent->set_sphere_pack_flag(SPF_LEAF_TREE);

                parent->set_sphere_pack_flag(SPF_SUPERSPHERE);
                parent->add_child(pack);

                supersphere->add_child(parent);

                parent->recompute(m_super_sphere_gravy_);
                pack->compute_binding_distance(parent);

                if (parent->has_sphere_pack_flag(SPF_LEAF_TREE)) {
                    // Every leaf tree supersphere is mirrored by a sphere in the root tree; user_data links the two.
                    sphere_pack *link = add_sphere(parent->pos(), parent->radius(), parent, SPF_ROOT_TREE);
                    parent->set_user_data(link);
                }
            }
        }

        pack->clear_sphere_pack_flag(SPF_INTEGRATE);
    }

    void sphere_pack_factory::frustum_test(const frustum &f, sphere_pack_callback *callback) {
        m_callback_ = callback;
        m_root_->visibility_test(f, this, VS_PARTIAL);
    }

    void sphere_pack::visibility_test(const frustum &f, sphere_pack_callback *callback, view_state state) {

        if (state == VS_PARTIAL) {
            state = f.view_volume_test(m_center_, radius());
        }

        if (has_sphere_pack_flag(SPF_SUPERSPHERE)) {

            if (state == VS_OUTSIDE) {
                if (has_sphere_pack_flag(SPF_HIDDEN)) return; // no state change
                clear_sphere_pack_flag(sphere_pack_flag(SPF_INSIDE | SPF_PARTIAL));
                set_sphere_pack_flag(SPF_HIDDEN);
            } else {
                if (state == VS_INSIDE) {
                    if (has_sphere_pack_flag(SPF_INSIDE)) return; // no state change
                    clear_sphere_pack_flag(sphere_pack_flag(SPF_PARTIAL | SPF_HIDDEN));
                    set_sphere_pack_flag(SPF_INSIDE);
                } else {
                    clear_sphere_pack_flag(sphere_pack_flag(SPF_HIDDEN | SPF_INSIDE));
                    set_sphere_pack_flag(SPF_PARTIAL);
                }
            }

            sphere_pack *pack = m_children_;

            while (pack) {
                pack->visibility_test(f, callback, state);
                pack = pack->next_sibling();
            }

        } else {
            switch (state) {
            case VS_INSIDE:
                if (!has_sphere_pack_flag(SPF_INSIDE)) {
                    clear_sphere_pack_flag(sphere_pack_flag(SPF_HIDDEN | SPF_PARTIAL));
                    set_sphere_pack_flag(SPF_INSIDE);
                    callback->visibility_callback(f, this, state);
                }
                break;
            case VS_OUTSIDE:
                if (!has_sphere_pack_flag(SPF_HIDDEN)) {
                    clear_sphere_pack_flag(sphere_pack_flag(SPF_INSIDE | SPF_PARTIAL));
                    set_sphere_pack_flag(SPF_HIDDEN);
                    callback->visibility_callback(f, this, state);
                }
                break;
            case VS_PARTIAL:
                {
                    clear_sphere_pack_flag(sphere_pack_flag(SPF_INSIDE | SPF_HIDDEN));
                    set_sphere_pack_flag(SPF_PARTIAL);
                    callback->visibility_callback(f, this, state);
                }
                break;
            }
        }
    }

    void sphere_pack_factory::ray_trace(const vector3d &p1,
        const vector3d &p2,
        sphere_pack_callback *callback) {
        vector3d dir = p2;
        float dist = dir.normalize();
        m_callback_ = callback;
        m_root_->ray_trace(p1, dir, dist, this);
    }

    void sphere_pack_factory::range_test(const vector3d &center, float radius, sphere_pack_callback *callback) {
        m_callback_ = callback;
        m_root_->range_test(center, radius, this, VS_PARTIAL);
    }

    void sphere_pack_factory::point_test2d(const vector3d &center, sphere_pack_callback *callback) {
        m_callback_ = callback;

        m_root_->point_test2d(center, this, VS_PARTIAL);
    }

    void sphere_pack::range_test(const vector3d &p,
        float distance,
        sphere_pack_callback *callback,
        view_state state) {

        if (state == VS_PARTIAL) {
            float d = p.distance(m_center_);
            if ((d - distance) > radius()) return;
            if ((radius() + d) < distance) state = VS_INSIDE;
        }

        if (has_sphere_pack_flag(SPF_SUPERSPHERE)) {
            sphere_pack *pack = m_children_;
            while (pack) {
                pack->range_test(p, distance, callback, state);
                pack = pack->next_sibling();
            }

        } else {
            callback->range_test_callback(p, distance, this, state);
        }
    }

    void sphere_pack::point_test2d(const vector3d &p,
        sphere_pack_callback *callback,
        view_state state) {
        if (state == VS_PARTIAL) {
            float dx = p.x - m_center_.x;
            float dy = p.y - m_center_.y;
            float dist_square = (dx * dx) + (dy * dy);

            if (dist_square > radius2()) return;
            if (radius2() < -dist_square) state = VS_INSIDE;
        }

        if (has_sphere_pack_flag(SPF_SUPERSPHERE)) {
            sphere_pack *pack = m_children_;
            while (pack) {
                pack->point_test2d(p, callback, state);
                pack = pack->next_sibling();
            }

        } else {
            callback->point_test2d_callback(p, this, state);
        }
    }

    void sphere_pack_factory::range_test_callback(const vector3d &p, float distance, sphere_pack *sphere, view_state state) {
        auto *link = (sphere_pack *)sphere->user_data();
        if (link) link->range_test(p, distance, m_callback_, state);
    };

    void sphere_pack_factory::point_test2d_callback(const vector3d &p, sphere_pack *sphere, view_state state) {
        auto *link = (sphere_pack *)sphere->user_data();
        if (link) link->point_test2d(p, m_callback_, state);
    };

    void sphere_pack::ray_trace(const vector3d &p1,
        const vector3d &dir,
        float distance,
        sphere_pack_callback *callback) {
        bool hit = false;

        if (has_sphere_pack_flag(SPF_SUPERSPHERE)) {

            hit = ray_intersection_in_front(p1, dir, nullptr);

            if (hit) {
                sphere_pack *pack = m_children_;

                while (pack) {
                    pack->ray_trace(p1, dir, distance, callback);
                    pack = pack->next_sibling();
                }
            }

        } else {
            vector3d sect;
            hit = ray_intersection(p1, dir, distance, &sect);
            if (hit) {
                callback->ray_trace_callback(p1, dir, distance, sect, this);
            }
        }
    }

    void sphere_pack_factory::ray_trace_callback(const vector3d &p1,
        const vector3d &dir,
        float distance,
        const vector3d & /*sect*/,
        sphere_pack *sphere) {
        auto *link = (sphere_pack *)sphere->user_data();
        if (link) link->ray_trace(p1, dir, distance, m_callback_);
    };

    void sphere_pack_factory::reset(void) {
        m_root_->reset();
        m_leaf_->reset();
    }

    void sphere_pack::reset(void) {
        clear_sphere_pack_flag(sphere_pack_flag(SPF_HIDDEN | SPF_PARTIAL | SPF_INSIDE));

        sphere_pack *pack = m_children_;
        while (pack) {
            pack->reset();
            pack = pack->next_sibling();
        }
    }

    void sphere_pack_factory::visibility_callback(const frustum &f, sphere_pack *sphere, view_state state) {
        auto *link = (sphere_pack *)sphere->user_data();
        if (link) link->visibility_test(f, m_callback_, state);
    }
}
