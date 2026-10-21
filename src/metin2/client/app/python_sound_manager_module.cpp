#include <metin2/script/python_utils.h>
#include <metin2/sound/sound_manager.h>

namespace metin2::inline client::inline app {
    auto snd_play_sound(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        get_sound_manager().play_sound2d(file_name);
        return py_build_none();
    }

    auto snd_play_sound3d(PyObject *self, PyObject *args) -> PyObject * {
        float fx, fy, fz;
        if (!py_tuple_get_float(args, 0, &fx) ||
            !py_tuple_get_float(args, 1, &fy) ||
            !py_tuple_get_float(args, 2, &fz)) {
            return py_build_exception();
        }

        char *file_name;
        if (!py_tuple_get_string(args, 3, &file_name)) {
            return py_build_exception();
        }

        get_sound_manager().play_sound3d(fx, fy, fz, file_name);
        return py_build_none();
    }

    auto snd_fade_in_music(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        get_sound_manager().fade_in_music(file_name);
        return py_build_none();
    }

    auto snd_fade_out_music(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        get_sound_manager().fade_out_music(file_name);
        return py_build_none();
    }

    auto snd_fade_out_all_music(PyObject *self, PyObject *args) -> PyObject * {
        get_sound_manager().fade_out_all_music();
        return py_build_none();
    }

    auto snd_fade_limit_out_music(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name)) {
            return py_build_exception();
        }

        float limit_volume;
        if (!py_tuple_get_float(args, 1, &limit_volume)) {
            return py_build_exception();
        }

        get_sound_manager().fade_limit_out_music(file_name, limit_volume);
        return py_build_none();
    }

    auto snd_stop_all_sound(PyObject *self, PyObject *args) -> PyObject * {
        get_sound_manager().stop_all_sound3d();
        return py_build_none();
    }

    auto snd_set_music_volume(PyObject *self, PyObject *args) -> PyObject * {
        float volume;
        if (!py_tuple_get_float(args, 0, &volume)) {
            return py_build_exception();
        }

        get_sound_manager().set_music_volume(volume);
        return py_build_none();
    }

    auto snd_set_sound_volumef(PyObject *self, PyObject *args) -> PyObject * {
        float volume;
        if (!py_tuple_get_float(args, 0, &volume)) {
            return py_build_exception();
        }

        get_sound_manager().set_sound_volume_ratio(volume);
        return py_build_none();
    }

    auto snd_set_sound_volume(PyObject *self, PyObject *args) -> PyObject * {
        int volume;
        if (!py_tuple_get_integer(args, 0, &volume)) {
            return py_build_exception();
        }

        get_sound_manager().set_sound_volume_grade(volume);
        return py_build_none();
    }

    auto snd_set_sound_scale(PyObject *self, PyObject *args) -> PyObject * {
        float scale;
        if (!py_tuple_get_float(args, 0, &scale)) {
            return py_build_exception();
        }

        get_sound_manager().set_sound_scale(scale);
        return py_build_none();
    }

    auto snd_set_ambience_sound_scale(PyObject *self, PyObject *args) -> PyObject * {
        float scale;
        if (!py_tuple_get_float(args, 0, &scale)) {
            return py_build_exception();
        }

        get_sound_manager().set_ambience_sound_scale(scale);
        return py_build_none();
    }

    void initsnd() {
        static PyMethodDef methods[] = {
            {"PlaySound", snd_play_sound, METH_VARARGS},
            {"PlaySound3D", snd_play_sound3d, METH_VARARGS},
            {"FadeInMusic", snd_fade_in_music, METH_VARARGS},
            {"FadeOutMusic", snd_fade_out_music, METH_VARARGS},
            {"FadeOutAllMusic", snd_fade_out_all_music, METH_VARARGS},
            {"FadeLimitOutMusic", snd_fade_limit_out_music, METH_VARARGS},
            {"StopAllSound", snd_stop_all_sound, METH_VARARGS},

            {"SetMusicVolume", snd_set_music_volume, METH_VARARGS},
            {"SetSoundVolumef", snd_set_sound_volumef, METH_VARARGS},
            {"SetSoundVolume", snd_set_sound_volume, METH_VARARGS},
            {"SetSoundScale", snd_set_sound_scale, METH_VARARGS},
            {"SetAmbienceSoundScale", snd_set_ambience_sound_scale, METH_VARARGS},
            {nullptr, nullptr, 0, nullptr},
        };

        Py_InitModule("snd", methods);
    }
}
