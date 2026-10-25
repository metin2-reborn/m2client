#include "check.h"

#include <metin2/core/file_name.h>
#include <metin2/effects/effect_data.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/grp_sub_image.h>
#include <metin2/sound/type.h>
#include <metin2/world/actors/race_data.h>
#include <metin2/world/actors/race_motion_data.h>
#include <metin2/world/flying/flying_data.h>
#include <metin2/world/maps/area.h>
#include <metin2/world/maps/map_config.h>
#include <metin2/world/maps/map_util.h>
#include <metin2/world/maps/property.h>

#include <string>

using namespace metin2;

TEST(canonical_name_maps_the_folders_that_left_ymir_work) {
    CHECK_EQ(eter_file_manager::canonical_name("d:\\Ymir Work\\pc\\warrior\\Hair.DDS"), std::string("character:pc/warrior/hair.png"));
    CHECK_EQ(eter_file_manager::canonical_name("D:/ymir work/monster2/wolf/wolf.gr2"), std::string("character:monster2/wolf/wolf.gr2"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/effect/etc/dropitem/dropitem.mse"), std::string("effect:etc/dropitem/dropitem.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("ymir work/zone/b/obj/house.gr2"), std::string("zone:b/obj/house.gr2"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/pc/warrior/wait.msa"), std::string("character:pc/warrior/wait.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/special/fog.tga"), std::string("special:fog.tga"));
    // The new names map to themselves.
    CHECK_EQ(eter_file_manager::canonical_name("character:pc/warrior/hair.png"), std::string("character:pc/warrior/hair.png"));
    CHECK_EQ(eter_file_manager::canonical_name("Effect:etc/A.dds"), std::string("effect:etc/a.png"));
}

TEST(canonical_name_keeps_other_names) {
    // Names that stayed in the system archive, and other archives, keep their name (lower case, '/').
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/pcx/a.dds"), std::string("d:/ymir work/pcx/a.dds"));
    CHECK_EQ(eter_file_manager::canonical_name("textures:terrainmaps/a.dds"), std::string("textures:terrainmaps/a.dds"));
    CHECK_EQ(eter_file_manager::canonical_name("season3/a.tga"), std::string("season3/a.tga"));
    // Scripts are TOML everywhere.
    CHECK_EQ(eter_file_manager::canonical_name("Warrior_M.msm"), std::string("warrior_m.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("locale:ui/Login.sub"), std::string("locale:ui/login.toml"));
    // Fly scripts are x.fly.toml, next to the x.toml effect of the same name.
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/effect/monster/bou_20_1_throw.MSF"), std::string("effect:monster/bou_20_1_throw.fly.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("Arrow.msf"), std::string("arrow.fly.toml"));
}

TEST(canonical_name_maps_the_folders_that_left_the_system_archive) {
    CHECK_EQ(eter_file_manager::canonical_name("d:\\ymir work\\ui\\Game\\a.sub"), std::string("ui:game/a.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("D:/Ymir Work/UI/pattern/a.tga"), std::string("ui:pattern/a.png"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/uiloading/base/a.jpg"), std::string("ui:loading/base/a.jpg"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/guild/facility/jedan.msm"), std::string("guild:facility/jedan.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/tree/B1_Sassafras.spt"), std::string("tree:b1_sassafras.spt"));
    CHECK_EQ(eter_file_manager::canonical_name("season1\\monster\\x\\wait.msa"), std::string("season1:monster/x/wait.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("season2/npc/x/x.dds"), std::string("season2:npc/x/x.dds"));
    // The icons are textures:icon and PNG; their hair textures stayed DDS.
    CHECK_EQ(eter_file_manager::canonical_name("icon/item/00010.tga"), std::string("textures:icon/item/00010.png"));
    CHECK_EQ(eter_file_manager::canonical_name("icon:item/00010.png"), std::string("textures:icon/item/00010.png"));
    CHECK_EQ(eter_file_manager::canonical_name("icon:hair/45164.dds"), std::string("textures:icon/hair/45164.dds"));
    // The ui images and the quest item images are PNG; other item TGAs stayed.
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/ui/public.dds"), std::string("ui:public.png"));
    CHECK_EQ(eter_file_manager::canonical_name("ui:pattern/board_line_left.tga"), std::string("ui:pattern/board_line_left.png"));
    CHECK_EQ(eter_file_manager::canonical_name("item:quest/73001.tga"), std::string("item:quest/73001.png"));
    CHECK_EQ(eter_file_manager::canonical_name("item:weapon/a.tga"), std::string("item:weapon/a.tga"));
    CHECK_EQ(eter_file_manager::canonical_name("textures:terrainmaps/a.tga"), std::string("textures:terrainmaps/a.tga"));
    CHECK_EQ(eter_file_manager::canonical_name("environment:A1.msenv"), std::string("environment:a1.toml"));
}

TEST(parse_path_resolves_old_names_to_the_new_archives) {
    const auto pc = eter_file_manager::parse_path("d:\\ymir work\\pc\\warrior\\a.dds");
    CHECK_EQ(pc.archive_value, std::string("character"));
    CHECK_EQ(pc.path, std::string("pc/warrior/a.png"));
    const auto zone = eter_file_manager::parse_path("d:/ymir work/zone/a.gr2");
    CHECK_EQ(zone.archive_value, std::string("zone"));
    CHECK_EQ(zone.path, std::string("a.gr2"));
    const auto ui = eter_file_manager::parse_path("d:/ymir work/ui/a.dds");
    CHECK_EQ(ui.archive_value, std::string("ui"));
    CHECK_EQ(ui.path, std::string("a.png"));
    const auto season = eter_file_manager::parse_path("season1/npc/x/x.gr2");
    CHECK_EQ(season.archive_value, std::string("season1"));
    CHECK_EQ(season.path, std::string("npc/x/x.gr2"));
    const auto other = eter_file_manager::parse_path("ymir work/missing/a.dds");
    CHECK_EQ(other.archive_value, std::string("system"));
    CHECK_EQ(other.path, std::string("ymir work/missing/a.dds"));
}

TEST(sound_script_name_follows_the_old_sound_folders) {
    CHECK_EQ(eter_file_manager::sound_script_name("character:pc/warrior/wait.toml"), std::string("sound:pc/warrior/wait.toml"));
    CHECK_EQ(eter_file_manager::sound_script_name("d:/ymir work/pc/warrior/wait.msa"), std::string("sound:pc/warrior/wait.toml"));
    CHECK_EQ(eter_file_manager::sound_script_name("effect:etc/levelup_1/levelup.toml"), std::string("sound:effect/etc/levelup_1/levelup.toml"));
    CHECK(eter_file_manager::sound_script_name("season1/monster/x/wait.toml").empty());
    CHECK(eter_file_manager::sound_script_name("system:x/wait.toml").empty());
}

TEST(asset_toml_checks_the_type) {
    CHECK(parse_asset_toml("[asset]\ntype = \"Motion\"\n", "a.toml", "Motion").has_value());
    CHECK(!parse_asset_toml("[asset]\ntype = \"Effect\"\n", "a.toml", "Motion").has_value());
    CHECK(!parse_asset_toml("[motion]\nfile = \"a.gr2\"\n", "a.toml", "Motion").has_value());
    CHECK(!parse_asset_toml("MotionFileName \"a.gr2\"\n", "a.msa", "Motion").has_value());
}

TEST(asset_table_reads_required_and_optional_values) {
    const auto table = toml::parse("a = 1.5\nb = 2\nc = true\nd = [1, 2, 3]\ne = \"Y\"\nf = 300\n");
    asset_table t(table, "t.toml");
    float a = 0.0f;
    int b = 0;
    bool c = false;
    math::vector3 d;
    int e = 0;
    float missing = 7.0f;
    CHECK(t.required("a", a));
    CHECK(t.required("b", b));
    CHECK(t.required("c", c));
    CHECK(t.required("d", d));
    constexpr std::string_view names[] = {"X", "Y"};
    CHECK(t.required_enum("e", names, e));
    CHECK(t.optional("missing", missing));
    CHECK_EQ(a, 1.5f);
    CHECK_EQ(b, 2);
    CHECK(c);
    CHECK_EQ(d.z, 3.0f);
    CHECK_EQ(e, 1);
    CHECK_EQ(missing, 7.0f);
    CHECK(t.finish()); // f is unknown: a warning only

    asset_table bad(table, "t.toml");
    uint8_t f = 0;
    CHECK(!bad.required("f", f)); // out of range
    CHECK(!bad.required("nothing", a));
    CHECK(!bad.finish());
}

namespace {
    constexpr const char *motion_text = R"([asset]
type = "Motion"

[motion]
file = "character:pc/warrior/attack.gr2"
duration = 1.5
accumulation = [0.0, -100.0, 0.0]

[combo]
pre_input_time = 0.5
direct_input_time = 0.75
input_limit_time = 1.0

[attack]
type = "Snipe"
hitting_type = "Great"
stiffen_time = 0.1
invisible_time = 0.2
external_force = 30.0
motion_type = "Combo"

[[attack.hit]]
start_time = 0.25
end_time = 0.5
bone = "equip_right_hand"
weapon_length = 150.0
positions = [
    [0.3, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0],
    [0.25, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0],
]

[loop]
count = 2
start_time = 0.1
end_time = 0.9

[[event]]
type = "Effect"
time = 0.5
effect = "d:/ymir work/pc/warrior/effect/a.mse"
position = [0.0, 30.0, 0.0]
bone = "Bip01"
attach = true
follow = true

[[event]]
type = "SpecialAttack"
time = 0.6
duration = 0.2
attack_type = "Splash"
hitting_type = "Good"
stiffen_time = 0.0
invisible_time = 0.3
external_force = 0.0
collision_type = "Splash"
spheres = [{ radius = 130.0, position = [0.0, -90.0, 120.0] }]

[[event]]
type = "ScreenWaving"
time = 0.7
duration = 0.3
power = 5

[[event]]
type = "Warp"
time = 1.0
)";
}

TEST(motion_reads_every_part) {
    race_motion_data m;
    CHECK(m.load(motion_text, "attack.toml"));
    CHECK_EQ(std::string(m.motion_file_name()), std::string("character:pc/warrior/attack.gr2"));
    CHECK_EQ(m.motion_duration(), 1.5f);
    CHECK(m.is_combo_input_time_data());
    CHECK_EQ(m.next_combo_time(), 0.75f);
    CHECK(m.is_loop_motion());
    CHECK_EQ(m.get_loop_count(), 2);

    CHECK(m.is_attacking_motion());
    const auto *attack = m.motion_attack_data_pointer();
    CHECK_EQ(attack->attack_type_value, int{race_types::ATTACK_TYPE_SNIPE});
    CHECK_EQ(attack->hitting_type, int{race_types::HIT_TYPE_GREAT});
    CHECK_EQ(attack->motion_type_value, int{race_types::MOTION_TYPE_COMBO});
    CHECK_EQ(attack->hit_limit_count, 0);
    CHECK_EQ(attack->hit_data_container_value.size(), std::size_t{1});
    const auto &hit = attack->hit_data_container_value[0];
    CHECK_EQ(hit.bone_name, std::string("equip_right_hand"));
    CHECK_EQ(hit.hit_position.size(), std::size_t{2});
    CHECK_EQ(hit.hit_position.begin()->first, 0.25f); // ordered by time
    CHECK_EQ(hit.hit_position.rbegin()->second.position_value.z, 6.0f);

    CHECK_EQ(m.motion_event_data_count(), uint32_t{4});
    CHECK(m.has_splash_motion_event());
    const race_motion_data::motion_event_data *event = nullptr;
    CHECK(m.motion_event_data_pointer(0, &event));
    const auto *effect = dynamic_cast<const race_motion_data::motion_effect_event_data *>(event);
    CHECK(effect != nullptr);
    // Old effect names are read as their new name.
    CHECK_EQ(effect->effect_file_name, std::string("character:pc/warrior/effect/a.toml"));
    CHECK(effect->is_attaching && effect->is_following && !effect->is_independent);
    CHECK(m.motion_event_data_pointer(1, &event));
    const auto *special = dynamic_cast<const race_motion_data::motion_attacking_event_data *>(event);
    CHECK(special != nullptr);
    CHECK(special->is_enable_hit_process);
    CHECK_EQ(special->collision_data_value.sphere_data_vector.size(), std::size_t{1});
    CHECK_EQ(special->collision_data_value.sphere_data_vector[0].attribute().radius, 130.0f);
    CHECK_EQ(m.get_event_type(2), int{race_motion_data::MOTION_EVENT_TYPE_SCREEN_WAVING});
    CHECK_EQ(m.get_event_type(3), int{race_motion_data::MOTION_EVENT_TYPE_WARP});
}

TEST(motion_rejects_bad_scripts) {
    race_motion_data no_motion;
    CHECK(!no_motion.load("[asset]\ntype = \"Motion\"\n", "a.toml"));
    race_motion_data no_duration;
    CHECK(!no_duration.load("[asset]\ntype = \"Motion\"\n[motion]\nfile = \"a.gr2\"\n", "a.toml"));
    race_motion_data bad_event;
    CHECK(!bad_event.load("[asset]\ntype = \"Motion\"\n[motion]\nfile = \"a.gr2\"\nduration = 1.0\n[[event]]\ntype = \"Dance\"\ntime = 0.0\n", "a.toml"));
    race_motion_data bad_enum;
    CHECK(!bad_enum.load("[asset]\ntype = \"Motion\"\n[motion]\nfile = \"a.gr2\"\nduration = 1.0\n[attack]\nhitting_type = \"Huge\"\nstiffen_time = 0.0\n"
                         "invisible_time = 0.0\nexternal_force = 0.0\nmotion_type = \"Normal\"\n",
        "a.toml"));
    race_motion_data effect;
    CHECK(!effect.load("[asset]\ntype = \"Effect\"\n", "a.toml"));
}

namespace {
    constexpr const char *effect_text = R"([asset]
type = "Effect"

[bounding_sphere]
radius = 50.0
center = [0.0, 0.0, 10.0]

[[particle]]
start_time = 0.1
path = [
    { time = 0.0, mode = "Bezier", position = [0.0, 0.0, 10.0], control = [1.0, 2.0, 3.0] },
    { time = 1.0, mode = "Direct", position = [0.0, 0.0, 20.0] },
]

[particle.emitter]
max_emission_count = 2
cycle_length = 1.5
cycle_loop = true
shape = "Ellipse"
radius = 20.0
velocity_events = []
emission_rate_events = [[0.0, 5.0], [0.3, 5.0]]
life_time_events = [[0.0, 0.5]]
particle_size_x_events = [[0.0, 12.0]]
particle_size_y_events = [[0.0, 12.0]]

[particle.sprite]
src_blend = 5
dest_blend = 2
billboard = "All"
rotation = "RandomDirection"
rotation_speed = 100.0
rotation_random_start = [0, 360]
stretch = false
texture_animation = "None"
texture_animation_delay = 0.03
gravity_events = [[0.0, 400.0]]
scale_x_events = [[0.0, 0.0], [1.0, 1.0]]
scale_y_events = [[0.0, 0.0], [1.0, 1.0]]
red_events = [[0.0, 1.0]]
green_events = [[0.0, 0.0]]
blue_events = [[0.0, 0.0]]
alpha_events = [[0.0, 1.0], [1.0, 0.0]]
rotation_events = [[0.0, 0.0]]
textures = []

[[mesh]]
mesh = "impact1.mde"
loop = false
frame_delay = 0.02

[[mesh.element]]
billboard = "Y"
blending = true
src_blend = 5
dest_blend = 2
texture_loop = true
texture_frame_delay = 0.02
color_factor = [0.5, 0.25, 1.0, 1.0]

[[light]]
ambient = [0.1, 0.1, 0.1, 1.0]
diffuse = [1.0, 0.5, 0.0, 1.0]
max_range = 300.0
attenuation = [0.0, 0.01, 0.0]
range_events = [[0.0, 100.0], [1.0, 300.0]]
)";
}

TEST(effect_reads_particles_meshes_and_lights) {
    effect_data e;
    CHECK(e.load(effect_text, "effect:etc/test/test.toml"));
    CHECK_EQ(e.bounding_sphere_radius(), 50.0f);
    CHECK_EQ(e.bounding_sphere_position().z, 10.0f);
    CHECK_EQ(e.particle_count(), uint32_t{1});
    CHECK_EQ(e.mesh_count(), uint32_t{1});
    CHECK_EQ(e.light_count(), uint32_t{1});

    particle_system_data *p = e.particle_pointer(0);
    CHECK_EQ(p->start_time(), 0.1f);
    math::vector3 position;
    p->get_position(0.5f, position);
    CHECK_EQ(position.x, 0.5f); // halfway along the curve that starts at the first entry
    const emitter_property *emitter = p->emitter_property_pointer();
    CHECK_EQ(emitter->max_emission_count, uint32_t{2});
    CHECK_EQ(emitter->emitter_shape, uint8_t{emitter_property::EMITTER_SHAPE_ELLIPSE});
    CHECK(emitter->time_event_emitting_velocity.empty());               // given empty
    CHECK_EQ(emitter->time_event_emitting_size.size(), std::size_t{1}); // missing: 0
    CHECK_EQ(emitter->time_event_emission_count_per_second.size(), std::size_t{2});
    const particle_property *sprite = p->particle_property_pointer();
    CHECK_EQ(sprite->rotation_type, uint8_t{particle_property::ROTATION_TYPE_RANDOM_DIRECTION});
    CHECK_EQ(sprite->rotation_random_starting_end, uint16_t{360});
    CHECK_EQ(sprite->time_event_gravity.size(), std::size_t{1});
    CHECK(sprite->time_event_air_resistance.empty());
    CHECK_EQ(sprite->time_event_color.size(), std::size_t{2}); // the times of all four channels

    effect_mesh_script *mesh = e.mesh_pointer(0);
    // A name without an archive is next to the script.
    CHECK_EQ(std::string(mesh->mesh_file_name()), std::string("effect:etc/test/impact1.mde"));
    CHECK_EQ(mesh->billboard_type(0), int{MESH_BILLBOARD_TYPE_Y});
    math::color color;
    CHECK(mesh->color_factor(0, &color));
    CHECK_EQ(color.g, 0.25f);
}

TEST(effect_rejects_bad_scripts) {
    effect_data no_sprite;
    CHECK(!no_sprite.load("[asset]\ntype = \"Effect\"\n[[particle]]\n[particle.emitter]\nmax_emission_count = 1\n", "a.toml"));
    effect_data bad_mode;
    CHECK(!bad_mode.load("[asset]\ntype = \"Effect\"\n[[mesh]]\nmesh = \"a.mde\"\nloop = false\nframe_delay = 0.1\n"
                         "path = [{ time = 0.0, mode = \"Spiral\", position = [0.0, 0.0, 0.0] }]\n",
        "a.toml"));
    effect_data motion;
    CHECK(!motion.load("[asset]\ntype = \"Motion\"\n", "a.toml"));
}

TEST(sound_script_reads_the_sounds_in_order) {
    sound_types::sound_data_vector sounds;
    CHECK(sound_types::load_sound_script("[asset]\ntype = \"SoundScript\"\n"
                                         "[[sound]]\ntime = 0.5\nfile = \"sound:pc/a.wav\"\n"
                                         "[[sound]]\ntime = 0.25\nfile = \"sound:common/b.wav\"\n",
        "a.toml", sounds));
    CHECK_EQ(sounds.size(), std::size_t{2});
    CHECK_EQ(sounds[0].time, 0.5f);
    CHECK_EQ(sounds[0].sound_file_name, std::string("sound:pc/a.wav"));
    CHECK_EQ(sounds[1].sound_file_name, std::string("sound:common/b.wav"));

    CHECK(sound_types::load_sound_script("[asset]\ntype = \"SoundScript\"\n", "a.toml", sounds));
    CHECK(sounds.empty());
}

TEST(sound_script_rejects_bad_scripts) {
    sound_types::sound_data_vector sounds;
    CHECK(!sound_types::load_sound_script("[asset]\ntype = \"Motion\"\n", "a.toml", sounds));
    CHECK(!sound_types::load_sound_script("ScriptType CharacterSoundInformation\n", "a.mss", sounds));
    CHECK(!sound_types::load_sound_script("[asset]\ntype = \"SoundScript\"\n[[sound]]\ntime = 0.5\n", "a.toml", sounds));
    CHECK(!sound_types::load_sound_script("[asset]\ntype = \"SoundScript\"\n[[sound]]\ntime = \"x\"\nfile = \"a.wav\"\n", "a.toml", sounds));
}

namespace {
    constexpr std::string_view kPropertyHeader = "[asset]\ntype = \"Property\"\n[property]\nid = 3000000000\nname = \"House\"\n";

    auto load_property(std::string_view kind) -> std::optional<property> {
        property p("property:a.toml");
        if (!p.load(std::string(kPropertyHeader) + std::string(kind))) {
            return std::nullopt;
        }
        return p;
    }
}

TEST(property_reads_every_kind) {
    const auto building = load_property("[building]\nfile = \"zone:b/house.gr2\"\nshadow = true\n");
    CHECK(building.has_value());
    CHECK_EQ(building->crc(), uint32_t{3000000000});
    CHECK_EQ(building->name(), std::string("House"));
    CHECK_EQ(building->type(), property_types::PROPERTY_TYPE_BUILDING);
    CHECK_EQ(building->building().file_name, std::string("zone:b/house.gr2"));
    CHECK_EQ(building->building().attribute_data_file_name, std::string("zone:b/house.mdatr"));
    CHECK(building->building().is_shadow_flag);

    const auto tree = load_property("[tree]\nfile = \"tree:a.spt\"\n");
    CHECK(tree.has_value());
    CHECK_EQ(tree->type(), property_types::PROPERTY_TYPE_TREE);
    CHECK_EQ(tree->tree().file_name, std::string("tree:a.spt"));

    const auto effect = load_property("[effect]\nfile = \"effect:background/a.toml\"\n");
    CHECK(effect.has_value());
    CHECK_EQ(effect->type(), property_types::PROPERTY_TYPE_EFFECT);
    CHECK_EQ(effect->effect().file_name, std::string("effect:background/a.toml"));

    const auto block = load_property("[dungeon_block]\nfile = \"zone:dungeon/a.gr2\"\n");
    CHECK(block.has_value());
    CHECK_EQ(block->type(), property_types::PROPERTY_TYPE_DUNGEON_BLOCK);
    CHECK_EQ(block->dungeon_block().attribute_data_file_name, std::string("zone:dungeon/a.mdatr"));

    const auto ambience = load_property("[ambience]\nplay_type = \"Step\"\ninterval = 2.0\ninterval_variation = 0.5\n"
                                        "sounds = [\"sound:ambience/a.wav\", \"sound:ambience/b.wav\"]\n");
    CHECK(ambience.has_value());
    CHECK_EQ(ambience->type(), property_types::PROPERTY_TYPE_AMBIENCE);
    CHECK_EQ(ambience->ambience().play_type, property_types::AMBIENCE_PLAY_TYPE_STEP);
    CHECK_EQ(ambience->ambience().play_interval, 2.0f);
    CHECK_EQ(ambience->ambience().max_volume_area_percentage, 0.0f); // missing: 0
    CHECK_EQ(ambience->ambience().ambience_sound_vector.size(), std::size_t{2});
}

TEST(property_rejects_bad_scripts) {
    CHECK(!load_property("").has_value());
    CHECK(!load_property("[tree]\nfile = \"a.spt\"\n[effect]\nfile = \"a.toml\"\n").has_value());
    CHECK(!load_property("[building]\n").has_value());
    CHECK(!load_property("[ambience]\nplay_type = \"LOOP\"\ninterval = 0.0\ninterval_variation = 0.0\nsounds = []\n").has_value());

    property no_id("property:a.toml");
    CHECK(!no_id.load("[asset]\ntype = \"Property\"\n[property]\nname = \"a\"\n[tree]\nfile = \"a.spt\"\n"));
    property old("property:a.prb");
    CHECK(!old.load("YPRT\r\n1430790742\r\npropertytype\t\"Tree\"\r\n"));
}

TEST(sub_image_reads_the_image_and_rect) {
    std::string image;
    int rect[4] = {};
    CHECK(graphic_sub_image::load("[asset]\ntype = \"SubImage\"\n[sub_image]\nimage = \"ui:public/public.dds\"\nrect = [0, 37, 376, 74]\n", "a.toml", image, rect));
    CHECK_EQ(image, std::string("ui:public/public.dds"));
    CHECK_EQ(rect[1], 37);
    CHECK_EQ(rect[2], 376);
}

TEST(sub_image_rejects_bad_scripts) {
    std::string image;
    int rect[4] = {};
    CHECK(!graphic_sub_image::load("title subImage\nversion 2.0\nimage \"a.dds\"\n", "a.sub", image, rect));
    CHECK(!graphic_sub_image::load("[asset]\ntype = \"Race\"\n", "a.toml", image, rect));
    CHECK(!graphic_sub_image::load("[asset]\ntype = \"SubImage\"\n[sub_image]\nimage = \"a.dds\"\nrect = [0, 0, 1]\n", "a.toml", image, rect));
    CHECK(!graphic_sub_image::load("[asset]\ntype = \"SubImage\"\n[sub_image]\nimage = \"a.dds\"\nrect = [0, 0, 1.5, 2]\n", "a.toml", image, rect));
    CHECK(!graphic_sub_image::load("[asset]\ntype = \"SubImage\"\n[sub_image]\nrect = [0, 0, 1, 2]\n", "a.toml", image, rect));
}

namespace {
    constexpr std::string_view kRaceText = R"([asset]
type = "Race"

[race]
model = "character:pc/warrior/warrior.gr2"
smoke_bone = "ore"

[[shape]]
index = 3
model = "character:pc/warrior/nahan.gr2"
skins = [["character:pc/warrior/nahan.png", "character:pc/warrior/jaho.png"]]

[[hair]]
index = 1
model = "character:pc/warrior/hair/hair_1.gr2"
skin = ["character:pc/warrior/hair/hair_1.png", "character:pc/warrior/hair/hair_1_red.png"]

[[attach]]
type = "Collision"
attaching = true
model_index = 0
bone = "Bip01"
collision_type = "Defending"
spheres = [{ radius = 130.0, position = [0.0, -20.0, 80.0] }, { radius = 50.0, position = [0.0, 0.0, 0.0] }]

[[attach]]
type = "Effect"
attaching = false
model_index = 1
bone = ""
effect = "effect:etc/a.toml"
position = [1.0, 2.0, 3.0]

[[motion]]
name = "Wait"
file = "character:monster/wolf/00.toml"
percent = 70

[[motion]]
name = "Skill2"
file = "character:monster/wolf/41.toml"
percent = 100
)";

    struct race_view : race_data {
        using race_data::attaching_data_vector_value_;
        using race_data::base_model_file_name_;
        using race_data::smoke_bone_name_;
    };
}

TEST(race_reads_every_part) {
    race_view race;
    CHECK(race.load(kRaceText, "a.toml"));
    CHECK_EQ(race.base_model_file_name_, std::string("character:pc/warrior/warrior.gr2"));
    CHECK_EQ(race.smoke_bone_name_, std::string("ore"));

    const race_data::shape *shape = race.find_shape(3);
    CHECK(shape != nullptr);
    CHECK_EQ(shape->model_file_name, std::string("character:pc/warrior/nahan.gr2"));
    CHECK_EQ(shape->vct_skin.size(), std::size_t{1});
    CHECK_EQ(shape->vct_skin[0].dst_file_name, std::string("character:pc/warrior/jaho.png"));

    const race_data::hair *hair = race.find_hair(1);
    CHECK(hair != nullptr);
    CHECK_EQ(hair->model_file_name, std::string("character:pc/warrior/hair/hair_1.gr2"));

    CHECK_EQ(race.attaching_data_vector_value_.size(), std::size_t{2});
    const race_types::attaching_data &collision = race.attaching_data_vector_value_[0];
    CHECK_EQ(collision.type_value, uint32_t{race_types::ATTACHING_DATA_TYPE_COLLISION_DATA});
    CHECK(collision.is_attaching);
    CHECK_EQ(collision.collision_data_value->collision_type_value, int{race_types::COLLISION_TYPE_DEFENDING});
    CHECK_EQ(collision.collision_data_value->sphere_data_vector.size(), std::size_t{2});
    const race_types::attaching_data &effect = race.attaching_data_vector_value_[1];
    CHECK_EQ(effect.attaching_model_index, uint32_t{1});
    CHECK_EQ(effect.effect_data_value->file_name, std::string("effect:etc/a.toml"));
    CHECK_EQ(effect.effect_data_value->position_value.z, 3.0f);
    CHECK_EQ(effect.effect_data_value->rotation.x, 0.0f);

    CHECK_EQ(race.motion_list().size(), std::size_t{2});
    CHECK_EQ(race.motion_list()[0].index, uint16_t{race_motion_data::NAME_WAIT});
    CHECK_EQ(race.motion_list()[0].file_name, std::string("character:monster/wolf/00.toml"));
    CHECK_EQ(race.motion_list()[0].percentage, uint8_t{70});
    CHECK_EQ(race.motion_list()[1].index, uint16_t{race_motion_data::NAME_SKILL + 122});
}

TEST(race_rejects_bad_scripts) {
    race_data no_race;
    CHECK(!no_race.load("[asset]\ntype = \"Race\"\n", "a.toml"));
    race_data bad_attach;
    CHECK(!bad_attach.load("[asset]\ntype = \"Race\"\n[race]\n[[attach]]\ntype = \"Light\"\nattaching = true\nmodel_index = 0\nbone = \"\"\n", "a.toml"));
    race_data bad_collision;
    CHECK(!bad_collision.load("[asset]\ntype = \"Race\"\n[race]\n[[attach]]\ntype = \"Collision\"\nattaching = true\nmodel_index = 0\nbone = \"\"\n"
                              "collision_type = \"Hard\"\nspheres = []\n",
        "a.toml"));
    race_data shape_without_model;
    CHECK(!shape_without_model.load("[asset]\ntype = \"Race\"\n[race]\n[[shape]]\nindex = 0\n", "a.toml"));
    const std::string motion = "[asset]\ntype = \"Race\"\n[race]\n[[motion]]\nfile = \"a.toml\"\npercent = 100\n";
    race_data unknown_motion;
    CHECK(!unknown_motion.load(motion + "name = \"Fly\"\n", "a.toml"));
    race_data motion_without_name;
    CHECK(!motion_without_name.load(motion, "a.toml"));
    race_data big_percent;
    CHECK(!big_percent.load("[asset]\ntype = \"Race\"\n[race]\n[[motion]]\nname = \"Wait\"\nfile = \"a.toml\"\npercent = 300\n", "a.toml"));
    race_data old;
    CHECK(!old.load("ScriptType RaceDataScript\nBaseModelFileName \"a.gr2\"\n", "a.msm"));
}

namespace {
    constexpr std::string_view kEnvironmentText = R"([asset]
type = "Environment"

[directional_light]
direction = [0.25, 0.5, -0.75]

[directional_light.background]
enable = true
diffuse = [0.5, 0.5, 0.6, 1.0]
ambient = [0.1, 0.1, 0.1, 1.0]

[directional_light.character]
enable = false
diffuse = [0.7, 0.7, 0.7, 1.0]
ambient = [0.2, 0.2, 0.2, 1.0]

[material]
diffuse = [1.0, 1.0, 1.0, 1.0]

[fog]
enable = true
near = 5000.0
far = 20000.0
color = [0.1, 0.1, 0.2, 1.0]

[filter]
enable = false
color = [0.3, 0.3, 0.5, 0.0]
alpha_src = 1
alpha_dest = 2

[sky_box]
scale = [3500.0, 3500.0, 3500.0]
gradient_upper = 1
gradient_lower = 1
cloud_scale = [100000.0, 100000.0]
cloud_height = 20000.0
cloud_speed = [0.002, 0.003]
cloud_texture = "environment:clouds.png"
cloud_color = { first = [1.0, 1.0, 1.0, 1.0], second = [0.5, 0.5, 0.5, 0.0] }
gradient = [{ first = [0.1, 0.2, 0.3, 1.0], second = [0.4, 0.5, 0.6, 1.0] }, { first = [0.7, 0.8, 0.9, 1.0], second = [1.0, 1.0, 1.0, 1.0] }]
faces = { front = "environment:skybox/front.png", bottom = "" }

[lens_flare]
enable = false
main_flare_size = 0.3
)";
}

TEST(environment_reads_every_table) {
    environment_data env;
    environment_init(env);
    CHECK(environment_parse(env, kEnvironmentText, "a.toml"));
    CHECK_EQ(env.dir_lights[ENV_DIRLIGHT_CHARACTER].direction.y, 0.5f);
    CHECK(env.dir_lights_enable[ENV_DIRLIGHT_BACKGROUND]);
    CHECK(!env.dir_lights_enable[ENV_DIRLIGHT_CHARACTER]);
    CHECK_EQ(env.dir_lights[ENV_DIRLIGHT_BACKGROUND].diffuse.b, 0.6f);
    CHECK_EQ(env.material_value.ambient.r, 0.8f); // missing: the default stays
    CHECK(env.fog_enable);
    CHECK_EQ(env.fog_far_distance, 20000.0f);
    CHECK_EQ(env.filtering_alpha_dest, uint8_t{2});
    CHECK_EQ(env.cloud_speed.y, 0.003f);
    CHECK_EQ(env.cloud_texture_scale.x, 4.0f); // default
    CHECK_EQ(env.cloud_gradient_color.second_color.a, 0.0f);
    CHECK_EQ(env.sky_box_gradient_color_vector.size(), std::size_t{2});
    CHECK_EQ(env.sky_box_gradient_color_vector[1].first_color.g, 0.8f);
    CHECK_EQ(env.sky_box_face_file_name[0], std::string("environment:skybox/front.png"));
    CHECK_EQ(env.main_flare_size, 0.3f);
}

TEST(environment_rejects_bad_scripts) {
    environment_data env;
    environment_init(env);
    CHECK(!environment_parse(env, "[asset]\ntype = \"Race\"\n", "a.toml"));
    CHECK(!environment_parse(env, "ScriptType EnvrionmentData\n", "a.msenv"));
    CHECK(!environment_parse(env, "[asset]\ntype = \"Environment\"\n[fog]\ncolor = [1.0, 1.0, 1.0]\n", "a.toml"));
    CHECK(!environment_parse(env, "[asset]\ntype = \"Environment\"\n[sky_box]\ngradient_upper = 2\ngradient = []\n", "a.toml"));
    CHECK(!environment_parse(env, "[asset]\ntype = \"Environment\"\n[sky_box]\ncloud_scale = [1.0]\n", "a.toml"));
}

namespace {
    struct fly_view : flying_data {
        using flying_data::accel_;
        using flying_data::bomb_effect_name_;
        using flying_data::homing_max_angle_;
        using flying_data::init_vel_;
        using flying_data::is_homing_;
        using flying_data::range_;
    };
}

TEST(fly_reads_the_flight_and_attachments) {
    fly_view fly;
    CHECK(fly.load(R"([asset]
type = "Fly"

[fly]
initial_velocity = 4000.0
bomb_effect = "character:monster2/ch_bowman/blow.toml"
homing = true
range = 5000.0
acceleration = [0.0, -100.0, 0.0]

[[attach]]
type = "Effect"
fly_type = "Sine"
file = "character:monster2/ch_bowman/arrow.toml"
period = 0.2
tail = { color = 0x66E1E1E1, length = 0.18, size = 1.0 }

[[attach]]
type = "None"
fly_type = "Line"
)",
        "a.fly.toml"));
    CHECK_EQ(fly.init_vel_, 4000.0f);
    CHECK_EQ(fly.range_, 5000.0f);
    CHECK(fly.is_homing_);
    CHECK_EQ(fly.homing_max_angle_, 3.0f); // missing: the default stays
    CHECK_EQ(fly.accel_.y, -100.0f);
    CHECK_EQ(fly.bomb_effect_name_, std::string("character:monster2/ch_bowman/blow.toml"));
    CHECK_EQ(fly.attach_data_count(), 2);
    const flying_data::flying_attach_data &arrow = fly.attach_data_reference(0);
    CHECK_EQ(arrow.type_value, int{flying_data::FLY_ATTACH_EFFECT});
    CHECK_EQ(arrow.fly_type, int{flying_data::FLY_ATTACH_TYPE_SINE});
    CHECK_EQ(arrow.period, 0.2f);
    CHECK(arrow.has_tail);
    CHECK_EQ(arrow.tail_color, 0x66E1E1E1u);
    CHECK(!arrow.rect_shape);
    CHECK(!fly.attach_data_reference(1).has_tail);
    CHECK(fly.attach_data_reference(1).filename.empty());
}

TEST(fly_rejects_bad_scripts) {
    const std::string head = "[asset]\ntype = \"Fly\"\n[fly]\ninitial_velocity = 1.0\n";
    flying_data good;
    CHECK(good.load(head + "range = 1.0\n", "a.fly.toml"));
    flying_data no_range;
    CHECK(!no_range.load(head, "a.fly.toml"));
    flying_data effect;
    CHECK(!effect.load("[asset]\ntype = \"Effect\"\n[fly]\ninitial_velocity = 1.0\nrange = 1.0\n", "a.fly.toml"));
    flying_data bad_fly_type;
    CHECK(!bad_fly_type.load(head + "range = 1.0\n[[attach]]\ntype = \"Effect\"\nfly_type = \"Spiral\"\n", "a.fly.toml"));
    flying_data short_tail;
    CHECK(!short_tail.load(head + "range = 1.0\n[[attach]]\ntype = \"Effect\"\nfly_type = \"Line\"\ntail = { color = 1 }\n", "a.fly.toml"));
}

TEST(map_config_reads_the_map_and_terrain) {
    map_config config;
    CHECK(load_map_config(R"([asset]
type = "Map"

[map]
type = "Outdoor"

[terrain]
size = [4, 5]
base_position = [409600, 896000]
view_radius = 128
height_scale = 0.5
texture_set = "texturesets:metin2_a1.toml"
environment = "A1.toml"
visible = false
)",
        "map.toml", config));
    CHECK(!config.indoor);
    CHECK(config.parent.empty());
    CHECK(config.has_terrain);
    CHECK_EQ(config.size_y, 5);
    CHECK(config.has_base_position);
    CHECK_EQ(config.base_x, 409600u);
    CHECK_EQ(config.view_radius, 128);
    CHECK_EQ(config.height_scale, 0.5f);
    CHECK_EQ(config.texture_set, std::string("texturesets:metin2_a1.toml"));
    CHECK_EQ(config.environment, std::string("A1.toml"));
    CHECK(!config.terrain_visible);

    map_config copy;
    CHECK(load_map_config("[asset]\ntype = \"Map\"\n[map]\ntype = \"Indoor\"\nparent = \"metin2_map_a1\"\n", "map.toml", copy));
    CHECK(copy.indoor);
    CHECK_EQ(copy.parent, std::string("metin2_map_a1"));
    CHECK(!copy.has_terrain);
}

TEST(map_config_rejects_bad_scripts) {
    map_config config;
    CHECK(!load_map_config("[asset]\ntype = \"Area\"\n[map]\ntype = \"Outdoor\"\n", "map.toml", config));
    CHECK(!load_map_config("[asset]\ntype = \"Map\"\n", "map.toml", config));
    CHECK(!load_map_config("[asset]\ntype = \"Map\"\n[map]\ntype = \"Cave\"\n", "map.toml", config));
    const std::string terrain = "[asset]\ntype = \"Map\"\n[map]\ntype = \"Outdoor\"\n[terrain]\nheight_scale = 0.5\ntexture_set = \"a.toml\"\n";
    CHECK(load_map_config(terrain + "size = [1, 1]\nview_radius = 1\n", "map.toml", config));
    CHECK(!load_map_config(terrain + "size = [1]\nview_radius = 1\n", "map.toml", config));
    CHECK(!load_map_config(terrain + "size = [1, 1]\nview_radius = 0\n", "map.toml", config));
    CHECK(!load_map_config(terrain + "size = [1, 1]\nview_radius = 1\nbase_position = [-1, 0]\n", "map.toml", config));
}

TEST(area_reads_objects_and_ambiences) {
    area::object_data_vector objects;
    area::object_data_vector ambiences;
    CHECK(area::load_config(R"([asset]
type = "Area"

[[object]]
property = 569394331
position = [20801.75, -13522.1, 17875.5]
rotation = [0, 15, 180]
height_bias = -40.0

[[object]]
property = 3
position = [1.0, 2.0, 3.0]
rotation = [0, 0, 0]
portals = [1, 255]

[[ambience]]
property = 7
position = [4.0, 5.0, 6.0]
range = 1000
max_volume_area_percentage = 0.5
)",
        "area.toml", objects, ambiences));
    CHECK_EQ(objects.size(), std::size_t{2});
    CHECK_EQ(objects[0].crc, 569394331u);
    CHECK_EQ(objects[0].position_value.z, 17875.5f);
    CHECK_EQ(objects[0].pitch, 15.0f);
    CHECK_EQ(objects[0].roll, 180.0f);
    CHECK_EQ(objects[0].height_bias, -40.0f);
    CHECK_EQ(objects[0].portal_id[0], uint8_t{0});
    CHECK_EQ(objects[1].height_bias, 0.0f);
    CHECK_EQ(objects[1].portal_id[1], uint8_t{255});
    CHECK_EQ(ambiences.size(), std::size_t{1});
    CHECK_EQ(ambiences[0].range, 1000u);
    CHECK_EQ(ambiences[0].max_volume_area_percentage, 0.5f);

    objects.clear();
    CHECK(area::load_config("[asset]\ntype = \"Area\"\n", "area.toml", objects, ambiences));
    CHECK(objects.empty());
}

TEST(area_rejects_bad_scripts) {
    area::object_data_vector objects;
    area::object_data_vector ambiences;
    const std::string head = "[asset]\ntype = \"Area\"\n";
    CHECK(!area::load_config("[asset]\ntype = \"Map\"\n", "area.toml", objects, ambiences));
    CHECK(!area::load_config(head + "[[object]]\nproperty = 1\nposition = [1.0, 2.0, 3.0]\n", "area.toml", objects, ambiences));
    CHECK(!area::load_config(head + "[[object]]\nproperty = 1\nposition = [1.0, 2.0]\nrotation = [0, 0, 0]\n", "area.toml", objects, ambiences));
    CHECK(!area::load_config(head + "[[object]]\nproperty = 1\nposition = [1.0, 2.0, 3.0]\nrotation = [0, 0, 0]\nportals = [256]\n", "area.toml", objects, ambiences));
    CHECK(!area::load_config(head + "[[ambience]]\nproperty = 1\nposition = [1.0, 2.0, 3.0]\n", "area.toml", objects, ambiences));
}

TEST(file_name_path_counts_the_archive_as_a_folder) {
    std::string tree = "tree:b1_oak.spt";
    CHECK_EQ(file_name_helper::path(tree), std::string("tree:"));
    std::string old = "d:/ymir work/tree/b1_oak.spt";
    CHECK_EQ(file_name_helper::path(old), std::string("d:/ymir work/tree/"));
    std::string nested = "zone:b/obj/house.gr2";
    CHECK_EQ(file_name_helper::path(nested), std::string("zone:b/obj/"));
    std::string bare = "house.gr2";
    CHECK(file_name_helper::path(bare).empty());
}
