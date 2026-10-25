#include "resource.h"

#include <metin2/effects/effect_instance.h>
#include <metin2/effects/effect_mesh.h>
#include <metin2/graphics/attribute_data.h>
#include <metin2/graphics/grp_expanded_image_instance.h>
#include <metin2/graphics/grp_mark_instance.h>
#include <metin2/graphics/grp_sub_image.h>
#include <metin2/graphics/grp_text.h>
#include <metin2/graphics/grp_text_instance.h>
#include <metin2/models/thing.h>
#include <metin2/models/thing_instance.h>
// The world headers are not self-contained yet and depend on this order.
#include <metin2/world/actors/actor_instance.h>
#include <metin2/world/actors/game_type.h>
#include <metin2/world/actors/race_data.h>
#include <metin2/world/actors/race_motion_data.h>
#include <metin2/world/actors/weapon_trace.h>
#include <metin2/world/flying/fly_trace.h>
#include <metin2/world/flying/flying_data.h>
#include <metin2/world/flying/flying_instance.h>
#include <metin2/world/items/item_data.h>
#include <metin2/world/maps/area.h>
#include <metin2/world/maps/map_type.h>

namespace metin2::inline script {

    static auto new_image(const char *file_name) -> resource * {
        return new graphic_image(file_name);
    }

    static auto new_sub_image(const char *file_name) -> resource * {
        return new graphic_sub_image(file_name);
    }

    static auto new_text(const char *file_name) -> resource * {
        return new graphic_text(file_name);
    }

    static auto new_thing(const char *file_name) -> resource * {
        return new graphic_thing(file_name);
    }

    static auto new_effect_mesh(const char *file_name) -> resource * {
        return new effect_mesh(file_name);
    }

    static auto new_attribute_data(const char *file_name) -> resource * {
        return new attribute_data(file_name);
    }

    void python_resource::destroy() {
        flying_instance::destroy_system();
        actor_instance::destroy_system();
        area::destroy_system();
        graphic_expanded_image_instance::destroy_system();
        graphic_image_instance::destroy_system();
        graphic_mark_instance::destroy_system();
        graphic_thing_instance::destroy_system();
        model_instance::destroy_system();
        graphic_text_instance::destroy_system();
        effect_instance::destroy_system();
        weapon_trace::destroy_system();
        fly_trace::destroy_system();

        get_resource_manager().destroy_deleting_list();

        flying_data::destroy_system();
        item_data::destroy_system();
        effect_data::destroy_system();
        effect_mesh::effect_mesh_data::destroy_system();
        race_data::destroy_system();
        race_types::destroy_system();
        race_motion_data::destroy_system();

        get_resource_manager().destroy();
    }

    python_resource::python_resource() {
        get_resource_manager().register_asset_new_function_pointer("SubImage", new_sub_image);
        get_resource_manager().register_resource_new_function_pointer("dds", new_image);
        get_resource_manager().register_resource_new_function_pointer("jpg", new_image);
        get_resource_manager().register_resource_new_function_pointer("tga", new_image);
        get_resource_manager().register_resource_new_function_pointer("bmp", new_image);
        get_resource_manager().register_resource_new_function_pointer("png", new_image);
        get_resource_manager().register_resource_new_function_pointer("fnt", new_text);
        get_resource_manager().register_resource_new_function_pointer("gr2", new_thing);
        get_resource_manager().register_resource_new_function_pointer("mde", new_effect_mesh);
        get_resource_manager().register_resource_new_function_pointer("mdatr", new_attribute_data);
    }

    python_resource::~python_resource() = default;
}
