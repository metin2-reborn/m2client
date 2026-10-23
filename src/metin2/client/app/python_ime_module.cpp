#include "python_ime.h"

#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline app {
    auto ime_enable_capture_input(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().enable_capture_input();
        Py_RETURN_NONE;
    }

    auto ime_disable_capture_input(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().disable_capture_input();
        Py_RETURN_NONE;
    }

    auto ime_set_max(PyObject *self, PyObject *args) -> PyObject * {
        int max_value;
        if (!PyArg_ParseTuple(args, "i", &max_value)) {
            return nullptr;
        }

        get_python_ime().set_max(max_value);
        Py_RETURN_NONE;
    }

    auto ime_set_user_max(PyObject *self, PyObject *args) -> PyObject * {
        int max_value;
        if (!PyArg_ParseTuple(args, "i", &max_value)) {
            return nullptr;
        }

        get_python_ime().set_user_max(max_value);
        Py_RETURN_NONE;
    }

    auto ime_set_text(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        if (!PyArg_ParseTuple(args, "s", &text)) {
            return nullptr;
        }

        get_python_ime().set_text(text, strlen(text));
        Py_RETURN_NONE;
    }

    auto ime_get_text(PyObject *self, PyObject *args) -> PyObject * {
        std::string text;
        get_python_ime().get_text(text);
        return Py_BuildValue("s", text.c_str());
    }

    auto ime_enable_ime(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().enable_ime();
        Py_RETURN_NONE;
    }

    auto ime_disable_ime(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().disable_ime();
        Py_RETURN_NONE;
    }

    auto ime_set_number_mode(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().set_number_mode();
        Py_RETURN_NONE;
    }

    auto ime_add_except_key(PyObject *self, PyObject *args) -> PyObject * {
        int key;
        if (!PyArg_ParseTuple(args, "i", &key)) {
            return nullptr;
        }

        get_python_ime().add_except_key(key);
        Py_RETURN_NONE;
    }

    auto ime_clear_except_key(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().clear_except_key();
        Py_RETURN_NONE;
    }

    auto ime_set_string_mode(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().set_string_mode();
        Py_RETURN_NONE;
    }

    auto ime_move_left(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().move_left();
        Py_RETURN_NONE;
    }

    auto ime_move_right(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().move_right();
        Py_RETURN_NONE;
    }

    auto ime_move_home(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().move_home();
        Py_RETURN_NONE;
    }
    auto ime_move_end(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().move_end();
        Py_RETURN_NONE;
    }

    auto ime_set_cursor_position(PyObject *self, PyObject *args) -> PyObject * {
        int position;
        if (!PyArg_ParseTuple(args, "i", &position)) {
            return nullptr;
        }

        get_python_ime().set_cursor_position(position);

        Py_RETURN_NONE;
    }

    auto ime_delete(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().delete_value();
        Py_RETURN_NONE;
    }

    auto ime_paste_text_from_clip_board(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().paste_text_from_clip_board();
        Py_RETURN_NONE;
    }

    auto ime_enable_paste(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        get_python_ime().enable_paste(flag ? true : false);
        Py_RETURN_NONE;
    }

    auto ime_paste_string(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        if (!PyArg_ParseTuple(args, "s", &text)) {
            return nullptr;
        }

        get_python_ime().paste_string(text);
        Py_RETURN_NONE;
    }

    auto ime_paste_backspace(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().on_char_input(U'\b');
        Py_RETURN_NONE;
    }

    auto ime_paste_return(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().on_char_input(U'\r');
        Py_RETURN_NONE;
    }

    void init_ime_module() {
        static PyMethodDef methods[] = {
            {"EnableCaptureInput", ime_enable_capture_input, METH_VARARGS},
            {"DisableCaptureInput", ime_disable_capture_input, METH_VARARGS},
            {"SetMax", ime_set_max, METH_VARARGS},
            {"SetUserMax", ime_set_user_max, METH_VARARGS},
            {"SetText", ime_set_text, METH_VARARGS},
            {"GetText", ime_get_text, METH_VARARGS},
            {"EnableIME", ime_enable_ime, METH_VARARGS},
            {"DisableIME", ime_disable_ime, METH_VARARGS},

            {"SetNumberMode", ime_set_number_mode, METH_VARARGS},
            {"SetStringMode", ime_set_string_mode, METH_VARARGS},
            {"AddExceptKey", ime_add_except_key, METH_VARARGS},
            {"ClearExceptKey", ime_clear_except_key, METH_VARARGS},

            {"MoveLeft", ime_move_left, METH_VARARGS},
            {"MoveRight", ime_move_right, METH_VARARGS},
            {"MoveHome", ime_move_home, METH_VARARGS},
            {"MoveEnd", ime_move_end, METH_VARARGS},
            {"SetCursorPosition", ime_set_cursor_position, METH_VARARGS},
            {"Delete", ime_delete, METH_VARARGS},
            {"PasteString", ime_paste_string, METH_VARARGS},
            {"PasteBackspace", ime_paste_backspace, METH_VARARGS},
            {"PasteReturn", ime_paste_return, METH_VARARGS},
            {"PasteTextFromClipBoard", ime_paste_text_from_clip_board, METH_VARARGS},
            {"EnablePaste", ime_enable_paste, METH_VARARGS},

            {nullptr, nullptr, 0, nullptr},
        };

        init_python_module("metin2.ime", methods);
    }
}
