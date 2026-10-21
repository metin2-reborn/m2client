#pragma once

namespace metin2::inline core {

    class reference_object {
      public:
        reference_object();
        virtual ~reference_object();

        void add_reference();
        void add_reference_only();
        void release();

        auto reference_count() -> int;

        auto can_destroy() -> bool;

      protected:
        virtual void on_construct();
        virtual void on_self_destruct();

      private:
        int ref_count_;
        bool destructed_;
    };
}
