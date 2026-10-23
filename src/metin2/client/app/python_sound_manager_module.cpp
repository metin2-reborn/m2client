#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/sound/sound_manager.h>

namespace metin2::inline client::inline app {
    auto snd_play_sound(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        get_sound_manager().play_sound2d(file_name);
        Py_RETURN_NONE;
    }

    auto snd_play_sound3d(PyObject *self, PyObject *args) -> PyObject * {
        float x, y, z;
        char *path;

        if (!PyArg_ParseTuple(args, "fffs", &x, &y, &z, &path)) {
            return nullptr;
        }

        get_sound_manager().play_sound3d(x, y, z, path);
        Py_RETURN_NONE;
    }

    auto snd_fade_in_music(PyObject *self, PyObject *args) -> PyObject * {
        char *path;
        if (!PyArg_ParseTuple(args, "s", &path)) {
            return nullptr;
        }

        get_sound_manager().fade_in_music(path);
        Py_RETURN_NONE;
    }

    auto snd_fade_out_music(PyObject *self, PyObject *args) -> PyObject * {
        char *path;
        if (!PyArg_ParseTuple(args, "s", &path)) {
            return nullptr;
        }

        get_sound_manager().fade_out_music(path);
        Py_RETURN_NONE;
    }

    auto snd_fade_out_all_music(PyObject *self, PyObject *args) -> PyObject * {
        get_sound_manager().fade_out_all_music();
        Py_RETURN_NONE;
    }

    auto snd_fade_limit_out_music(PyObject *self, PyObject *args) -> PyObject * {
        char *path;
        float limit_volume;

        if (!PyArg_ParseTuple(args, "sf", &path, &limit_volume)) {
            return nullptr;
        }

        get_sound_manager().fade_limit_out_music(path, limit_volume);
        Py_RETURN_NONE;
    }

    auto snd_stop_all_sound(PyObject *self, PyObject *args) -> PyObject * {
        get_sound_manager().stop_all_sound3d();
        Py_RETURN_NONE;
    }

    auto snd_set_music_volume(PyObject *self, PyObject *args) -> PyObject * {
        float volume;
        if (!PyArg_ParseTuple(args, "f", &volume)) {
            return nullptr;
        }

        get_sound_manager().set_music_volume(volume);
        Py_RETURN_NONE;
    }

    auto snd_set_sound_volumef(PyObject *self, PyObject *args) -> PyObject * {
        float volume;
        if (!PyArg_ParseTuple(args, "f", &volume)) {
            return nullptr;
        }

        get_sound_manager().set_sound_volume_ratio(volume);
        Py_RETURN_NONE;
    }

    auto snd_set_sound_volume(PyObject *self, PyObject *args) -> PyObject * {
        int volume;
        if (!PyArg_ParseTuple(args, "i", &volume)) {
            return nullptr;
        }

        get_sound_manager().set_sound_volume_grade(volume);
        Py_RETURN_NONE;
    }

    auto snd_set_sound_scale(PyObject *self, PyObject *args) -> PyObject * {
        float scale;
        if (!PyArg_ParseTuple(args, "f", &scale)) {
            return nullptr;
        }

        get_sound_manager().set_sound_scale(scale);
        Py_RETURN_NONE;
    }

    auto snd_set_ambience_sound_scale(PyObject *self, PyObject *args) -> PyObject * {
        float scale;
        if (!PyArg_ParseTuple(args, "f", &scale)) {
            return nullptr;
        }

        get_sound_manager().set_ambience_sound_scale(scale);
        Py_RETURN_NONE;
    }

    void init_sound_module() {
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

        init_python_module("metin2.sound", methods);
    }
}
