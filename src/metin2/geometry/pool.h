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

#include <cassert>

namespace metin2::inline geometry {

    // Fixed pool of items for fast allocation; Type must provide next/previous links.
    template <class Type>
    class pool {
      public:
        pool(void) {
            m_max_items_ = 0;
            m_head_ = nullptr;
            m_free_ = nullptr;
            m_data_ = nullptr;
            m_current_ = nullptr;
            m_free_count_ = 0;
            m_used_count_ = 0;
        };

        ~pool(void) {
            if (m_data_)
                delete[] m_data_;
        };

        void set(int maxitems) {
            assert(maxitems > 0);
            if (m_data_)
                delete[] m_data_;
            m_max_items_ = maxitems;
            m_data_ = new Type[m_max_items_];
            m_free_ = m_data_;
            m_head_ = nullptr;
            int loop_value = (m_max_items_ - 1);
            for (int i = 0; i < loop_value; i++) {
                m_data_[i].set_next(&m_data_[i + 1]);
                if (i == 0)
                    m_data_[i].set_previous(nullptr);
                else
                    m_data_[i].set_previous(&m_data_[i - 1]);
            }

            m_data_[loop_value].set_next(nullptr);
            m_data_[loop_value].set_previous(&m_data_[loop_value - 1]);
            m_current_ = nullptr;
            m_free_count_ = maxitems;
            m_used_count_ = 0;
        };

        void release(Type *t) {

            if (t == m_current_) m_current_ = t->next();

            Type *prev = t->previous();

            if (prev) {
                Type *next = t->next();
                prev->set_next(next);
                if (next) next->set_previous(prev);
            } else {
                Type *next = t->next();
                m_head_ = next;
                if (m_head_) m_head_->set_previous(nullptr);
            }

            Type *temp = m_free_;
            m_free_ = t;
            t->set_previous(nullptr);
            t->set_next(temp);

            m_used_count_--;
            m_free_count_++;
        };

        auto free_link(void) -> Type * {
            if (!m_free_) return nullptr;
            Type *ret = m_free_;
            m_free_ = ret->next();
            Type *temp = m_head_;
            m_head_ = ret;
            if (temp) temp->set_previous(ret);
            m_head_->set_next(temp);
            m_head_->set_previous(nullptr);
            m_used_count_++;
            m_free_count_--;
            return ret;
        };

      private:
        int m_max_items_;
        Type *m_current_;
        Type *m_data_;
        Type *m_head_; // head of the used list
        Type *m_free_; // head of the free list
        int m_used_count_;
        int m_free_count_;
    };
}
