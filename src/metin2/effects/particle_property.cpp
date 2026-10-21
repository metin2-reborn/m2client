#include "particle_property.h"
#include <metin2/gpu/types.h>
#include <metin2/graphics/resource_manager.h>

namespace metin2::inline effects {

    void particle_property::insert_texture(const char *file_name) {
        auto *image = (graphic_image *)get_resource_manager().resource_pointer(file_name);

        image_vector.push_back(image);
    }

    auto particle_property::set_texture(const char *file_name) -> bool {
        if (image_vector.size() > 1) {
            assert(false);
            return false;
        }
        image_vector.clear();
        insert_texture(file_name);
        return true;
    }

    void particle_property::clear() {
        rotation_type = 0;
        rotation_speed = 0.0f;
        rotation_random_starting_begin = 0;
        rotation_random_starting_end = 0;

        attach_flag = false;
        stretch_flag = false;

        src_blend_type = gpu::blend::SRC_ALPHA;
        dest_blend_type = gpu::blend::ONE;
        color_operation_type = gpu::texture_op::MODULATE;

        billboard_type = BILLBOARD_TYPE_NONE;

        tex_ani_type = TEXTURE_ANIMATION_TYPE_NONE;
        tex_ani_delay = 0.05f;
        tex_ani_random_start_frame_flag = false;

        time_event_gravity.clear();
        time_event_air_resistance.clear();

        time_event_scale_x.clear();
        time_event_scale_y.clear();
        time_event_color.clear();
        time_event_rotation.clear();

        image_vector.clear();
    }

    particle_property::particle_property() {
        clear();
        z_axis = math::vector3(0.0f, 0.0f, 1.0f);
    }
    particle_property::~particle_property() = default;

    auto particle_property::operator=(const particle_property &particle_property_value) -> particle_property & = default;
}
