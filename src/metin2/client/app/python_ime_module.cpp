#include <metin2/script/python_utils.h>
#include "python_application.h"
#include "python_ime.h"

namespace metin2::inline client::inline app {

    auto ime_enable_capture_input(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().enable_capture_input();
        return py_build_none();
    }

    auto ime_disable_capture_input(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().disable_capture_input();
        return py_build_none();
    }

    auto ime_set_max(PyObject *self, PyObject *args) -> PyObject * {
        int max_value;
        if (!py_tuple_get_integer(args, 0, &max_value))
            return py_build_exception();

        get_python_ime().set_max(max_value);
        return py_build_none();
    }

    auto ime_set_user_max(PyObject *self, PyObject *args) -> PyObject * {
        int max_value;
        if (!py_tuple_get_integer(args, 0, &max_value))
            return py_build_exception();

        get_python_ime().set_user_max(max_value);
        return py_build_none();
    }

    auto ime_set_text(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        if (!py_tuple_get_string(args, 0, &text))
            return py_build_exception();

        get_python_ime().set_text(text, strlen(text));
        return py_build_none();
    }

    // The edit text as UTF-8.
    auto ime_get_text(PyObject *self, PyObject *args) -> PyObject * {
        std::string text;
        get_python_ime().get_text(text);
        return Py_BuildValue("s", text.c_str());
    }

    auto ime_enable_ime(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().enable_ime();
        return py_build_none();
    }

    auto ime_disable_ime(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().disable_ime();
        return py_build_none();
    }

    auto ime_set_number_mode(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().set_number_mode();
        return py_build_none();
    }

    auto ime_add_except_key(PyObject *self, PyObject *args) -> PyObject * {
        int key;
        if (!py_tuple_get_integer(args, 0, &key))
            return py_build_exception();

        get_python_ime().add_except_key(key);
        return py_build_none();
    }

    auto ime_clear_except_key(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().clear_except_key();
        return py_build_none();
    }

    auto ime_set_string_mode(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().set_string_mode();
        return py_build_none();
    }

    auto ime_move_left(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().move_left();

        return py_build_none();
    }
    auto ime_move_right(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().move_right();

        return py_build_none();
    }

    auto ime_move_home(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().move_home();

        return py_build_none();
    }
    auto ime_move_end(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().move_end();

        return py_build_none();
    }

    auto ime_set_cursor_position(PyObject *self, PyObject *args) -> PyObject * {
        int position;
        if (!py_tuple_get_integer(args, 0, &position))
            return py_build_exception();

        get_python_ime().set_cursor_position(position);

        return py_build_none();
    }

    auto ime_delete(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().delete_value();

        return py_build_none();
    }

    auto ime_paste_text_from_clip_board(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().paste_text_from_clip_board();
        return py_build_none();
    }

    auto ime_enable_paste(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag))
            return py_build_exception();

        get_python_ime().enable_paste(flag ? true : false);
        return py_build_none();
    }

    auto ime_paste_string(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        if (!py_tuple_get_string(args, 0, &text))
            return py_build_exception();
        get_python_ime().paste_string(text);
        return py_build_none();
    }

    auto ime_paste_backspace(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().on_char_input(U'\b');
        return py_build_none();
    }

    auto ime_paste_return(PyObject *self, PyObject *args) -> PyObject * {
        get_python_ime().on_char_input(U'\r');
        return py_build_none();
    }

    void initime() {
        static PyMethodDef s_methods[] =
            {

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

        Py_InitModule("ime", s_methods);
    }
}
