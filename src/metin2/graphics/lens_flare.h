//	CLensFlare Class
//	(c) 2003 IDV, Inc.
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com

#pragma once

#include "grp_image_instance.h"
#include "grp_screen.h"

#include <float.h>
#include <metin2/gpu/types.h>
#include <string>
#include <vector>

namespace metin2::inline graphics {

    const float c_fHalfMaxBright = 0.45f;
    const float c_fDistanceScale = 1.0f;
    const int c_nDepthTestDimension = 15;

    class flare {
      public:
        void draw(float bright_scale, int width_value, int height, int x, int y);
        void init(std::string path);

        flare();
        virtual ~flare();

      private:
        struct flare_piece {
            flare_piece() : position_value(0.0f),
                            width(0.0f),
                            color_value(nullptr) {
            }
            graphic_image_instance image_instance;
            float position_value; // -1.0 = light location, 0.0 = center, 1.0 = far end of flare
            float width;          // height = width
            float *color_value;
        };

        std::vector<flare_piece *> v_flares_;
    };

    class lens_flare : public screen {
      public:
        lens_flare();
        ~lens_flare() override;

        void compute(const math::vector3 &light_direction); // Reads the camera from gpu::transform::VIEW, so call it after the camera is set up.

        void draw_before_flare();
        void draw_after_flare();
        void draw_flare();

        void set_main_flare(std::string sun_file, float sun_size);
        void initialize(std::string path);

        void set_flare_location(double x, double y);

        void set_visible(bool state) { flare_visible_ = state; }
        auto is_visible() -> bool { return flare_visible_; }

        void set_brightnesses(float before_bright, float after_bright);

        void adjust_brightness();

        void characterize_flare(bool enabled, bool show_main_flare, float max_brightness, const math::color &color);

      protected:
        auto interpolate(float start, float end, float percent) -> float;

      private:
        float flare_pos_[2], flare_win_pos_[2];
        float before_bright_, after_bright_;
        bool flare_visible_, draw_flare_, draw_bright_screen_;
        float sun_size_;
        flare flare_value_;
        bool enabled_;
        bool show_main_flare_;
        float max_brightness_;
        float color_value_[4];

        graphic_image_instance sun_flare_image_instance_;

        // How much of the sun the scene leaves uncovered: a small probe quad at the sun's screen
        // position is drawn once without and once with the depth test inside two occlusion queries,
        // and their ratio is read back the next frame. Two probes alternate so nothing waits on the GPU.
        struct occlusion_probe {
            uint32_t total_query = 0;
            uint32_t visible_query = 0;
            bool pending = false;
        };
        occlusion_probe probes_[2];
        int probe_index_ = 0;
        float visibility_ = 1.0f;

        void measure_occlusion();
        void draw_occlusion_probe();
        void clamp_brightness();
    };
}
