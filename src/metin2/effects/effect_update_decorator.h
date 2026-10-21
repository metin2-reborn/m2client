#pragma once

#include <metin2/core/pool.h>
#include <metin2/core/random.h>
#include "type.h"

namespace metin2::inline effects {
    class particle_instance;

    namespace update_decorators {
        class decorator_data {
          public:
            float time;
            float elapsed_time;
            particle_instance *instance;
            decorator_data(float time_value, float elapsed_time_value, particle_instance *instance_value)
                : time(time_value), elapsed_time(elapsed_time_value), instance(instance_value) {}
        };
        class base_decorator {
            friend class CParticleSystemData;

          public:
            base_decorator() : next_decorator_(nullptr), prev_decorator_(nullptr) {}
            virtual ~base_decorator() = default;

            void excute(const decorator_data &d) {
                base_decorator *pd = this;
                while (pd) {
                    base_decorator *next_decorator = pd->next_decorator_;
                    pd->excute_impl(d);
                    pd = next_decorator;
                }
            }
            auto add_chain_front(base_decorator *pd) -> base_decorator * {
                pd->next_decorator_ = this;
                prev_decorator_ = pd;
                return pd;
            }
            void delete_this() {
                if (next_decorator_)
                    next_decorator_->delete_this();
                delete this;
            }
            auto clone(particle_instance *first_instance, particle_instance *instance) -> base_decorator * {
                base_decorator *new_decorator = clone_impl(first_instance, instance);
                base_decorator *src = this;
                base_decorator *dest = new_decorator;
                while (src->next_decorator_) {
                    dest->next_decorator_ = src->next_decorator_->clone_impl(first_instance, instance);
                    dest->next_decorator_->prev_decorator_ = dest;

                    src = src->next_decorator_;
                    dest = dest->next_decorator_;
                }
                return new_decorator;
            }

          protected:
            virtual void excute_impl(const decorator_data &d) = 0;
            virtual auto clone_impl(particle_instance *first_instance, particle_instance *instance) -> base_decorator * = 0;
            void remove_me() {
                prev_decorator_->next_decorator_ = next_decorator_;
                next_decorator_->prev_decorator_ = prev_decorator_;
                delete this;
            }
            base_decorator *next_decorator_;
            base_decorator *prev_decorator_;
        };

        class header_decorator : public base_decorator, public pooled_object<header_decorator> {
          public:
            header_decorator() = default;
            ~header_decorator() override = default;

          protected:
            void excute_impl(const decorator_data &) override {}
            auto clone_impl(particle_instance *, particle_instance *) -> base_decorator * override { return new header_decorator; }
        };

        class null_decorator : public base_decorator, public pooled_object<null_decorator> {
          public:
            null_decorator() = default;
            ~null_decorator() override = default;

          protected:
            void excute_impl(const decorator_data &d) override {}
            auto clone_impl(particle_instance *, particle_instance *) -> base_decorator * override { return new null_decorator; }
        };

        template <class T>
        class time_event_decorator : public base_decorator, public pooled_object<time_event_decorator<T>> {
          public:
            using time_event_type = time_event<T>;
            using time_event_container_type = std::vector<time_event_type>;
            time_event_decorator(const time_event_container_type &time_event_container, T *value = 0)
                : it_start_(time_event_container.begin()),
                  it_end_(time_event_container.end()),
                  it_cur_(time_event_container.begin()),
                  it_next_(time_event_container.begin()),
                  data_value_(value) {
                if (it_start_ == it_end_)
                    *value = T();
                else
                    ++it_next_;
            }
            ~time_event_decorator() override = default;

          protected:
            time_event_decorator(time_event_decorator<T> &ted, particle_instance *first_instance, particle_instance *instance)
                : it_start_(ted.it_start_),
                  it_end_(ted.it_end_),
                  it_cur_(ted.it_cur_),
                  it_next_(ted.it_next_),
                  // The value lives inside the particle instance: same offset in the new instance.
                  data_value_((T *)((unsigned char *)instance + ((unsigned char *)ted.data_value_ - (unsigned char *)first_instance))) {
                if (it_start_ == it_end_)
                    *data_value_ = T();
            }
            auto clone_impl(particle_instance *first_instance, particle_instance *instance) -> base_decorator * override { return new time_event_decorator(*this, first_instance, instance); }
            void excute_impl(const decorator_data &d) override {
                if (it_start_ == it_end_) {
                    remove_me();
                } else if (it_cur_->time > d.time) {
                    *data_value_ = it_cur_->value;
                } else {
                    while (it_next_ != it_end_ && it_next_->time <= d.time)
                        ++it_cur_, ++it_next_;
                    if (it_next_ == it_end_) {
                        *data_value_ = it_cur_->value;

                        remove_me();
                    } else {
                        float length = it_next_->time - it_cur_->time;
                        *data_value_ = it_cur_->value * (1 - (d.time - it_cur_->time) / length);
                        *data_value_ += it_next_->value * ((d.time - it_cur_->time) / length);
                    }
                }
            }

            typename time_event_container_type::const_iterator it_start_;
            typename time_event_container_type::const_iterator it_end_;
            typename time_event_container_type::const_iterator it_cur_;
            typename time_event_container_type::const_iterator it_next_;
            T *data_value_;
        };

        using scale_value_decorator = time_event_decorator<float>;
        using color_value_decorator = time_event_decorator<float>;
        using color_all_decorator = time_event_decorator<dwordcolor>;
        using air_resistance_value_decorator = time_event_decorator<float>;
        using gravity_value_decorator = time_event_decorator<float>;
        using rotation_speed_value_decorator = time_event_decorator<float>;

        class texture_animation_cw_decorator : public base_decorator, public pooled_object<texture_animation_cw_decorator> {
          public:
            texture_animation_cw_decorator(float frame_time_value, uint32_t n, uint8_t *idx) : n_(n), last_frame_time_(frame_time_value), frame_time_(frame_time_value), idx_(idx) {}
            ~texture_animation_cw_decorator() override = default;

          protected:
            auto clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * override { return new texture_animation_cw_decorator(frame_time_, n_, (uint8_t *)((unsigned char *)pi + ((uint8_t *)idx_ - (uint8_t *)pfi))); }
            void excute_impl(const decorator_data &d) override {
                last_frame_time_ -= d.elapsed_time;
                while (last_frame_time_ < 0.0f) {
                    last_frame_time_ += frame_time_;
                    if (++(*idx_) >= n_)
                        *idx_ = 0;
                }
            }
            uint32_t n_;
            float last_frame_time_;
            float frame_time_;
            uint8_t *idx_;
        };
        class texture_animation_ccw_decorator : public base_decorator, public pooled_object<texture_animation_ccw_decorator> {
          public:
            texture_animation_ccw_decorator(float frame_time_value, uint8_t n, uint8_t *idx) : n_(n), last_frame_time_(frame_time_value), frame_time_(frame_time_value), idx_(idx) {}
            ~texture_animation_ccw_decorator() override = default;

          protected:
            auto clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * override { return new texture_animation_ccw_decorator(frame_time_, n_, (uint8_t *)((unsigned char *)pi + ((uint8_t *)idx_ - (uint8_t *)pfi))); }
            void excute_impl(const decorator_data &d) override {
                last_frame_time_ -= d.elapsed_time;
                while (last_frame_time_ < 0.0f) {
                    last_frame_time_ += frame_time_;

                    if (--(*idx_) >= n_ && n_ != 0) // idx_ is unsigned, so stepping below zero wraps to a value >= n_.
                        *idx_ = uint8_t(n_ - 1);
                }
            }
            uint8_t n_;
            float last_frame_time_;
            float frame_time_;
            uint8_t *idx_;
        };
        class texture_animation_random_decorator : public base_decorator, public pooled_object<texture_animation_random_decorator> {
          public:
            texture_animation_random_decorator(float frame_time_value, uint8_t n, uint8_t *idx) : n_(n), last_frame_time_(frame_time_value), frame_time_(frame_time_value), idx_(idx) {}
            ~texture_animation_random_decorator() override = default;

          protected:
            auto clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * override { return new texture_animation_random_decorator(frame_time_, n_, (uint8_t *)((unsigned char *)pi + ((uint8_t *)idx_ - (uint8_t *)pfi))); }
            void excute_impl(const decorator_data &d) override {
                last_frame_time_ -= d.elapsed_time;
                if (last_frame_time_ < 0.0f && n_ != 0) {
                    *idx_ = (uint8_t)metin2::random::number(0, n_ - 1);
                }
                while (last_frame_time_ < 0.0f)
                    last_frame_time_ += frame_time_;
            }
            uint8_t n_;
            float last_frame_time_;
            float frame_time_;
            uint8_t *idx_;
        };

        class air_resistance_decorator : public base_decorator, public pooled_object<air_resistance_decorator> {
          public:
            air_resistance_decorator() = default;
            ~air_resistance_decorator() override = default;

          protected:
            void excute_impl(const decorator_data &d) override;
            auto clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * override;
        };

        class gravity_decorator : public base_decorator, public pooled_object<gravity_decorator> {
          public:
            gravity_decorator() = default;
            ~gravity_decorator() override = default;

          protected:
            void excute_impl(const decorator_data &d) override;
            auto clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * override;
        };

        class rotation_decorator : public base_decorator, public pooled_object<rotation_decorator> {
          public:
            rotation_decorator() = default;
            ~rotation_decorator() override = default;

          protected:
            void excute_impl(const decorator_data &d) override;
            auto clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * override;
        };

    }
}
