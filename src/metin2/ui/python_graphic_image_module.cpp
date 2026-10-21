#include <cstdint>
#include <metin2/graphics/grp_expanded_image_instance.h>
#include <metin2/graphics/grp_image.h>
#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/resource.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_utils.h>
#include "python_modules.h"

namespace metin2::inline ui {

    auto py_tuple_get_image_instance(PyObject *args, int pos, graphic_image_instance **ret_image_instance) -> bool {
        intptr_t handle;

        if (!py_tuple_get_integer(args, pos, &handle))
            return false;

        if (!handle)
            return false;

        *ret_image_instance = (graphic_image_instance *)handle;
        return true;
    }

    auto py_tuple_get_expanded_image_instance(PyObject *args, int pos, graphic_expanded_image_instance **ret_image_instance) -> bool {
        intptr_t handle;

        if (!py_tuple_get_integer(args, pos, &handle))
            return false;

        if (!handle)
            return false;

        if (!((graphic_image_instance *)handle)->is_type(graphic_expanded_image_instance::type()))
            return false;

        *ret_image_instance = (graphic_expanded_image_instance *)handle;

        return true;
    }

    auto grp_image_generate(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;

        if (!py_tuple_get_string(args, 0, &file_name))
            return py_bad_argument();

        if (!*file_name)
            return Py_BuildValue("i", 0);

        resource *resource = get_resource_manager().resource_pointer(file_name);

        if (!resource->is_type(graphic_image::type()))
            return py_build_exception("Resource is not an image (filename: %s)", file_name);

        graphic_image_instance *image_instance = graphic_image_instance::new_value();
        image_instance->set_image_pointer(static_cast<graphic_image *>(resource));

        if (image_instance->is_empty())
            return py_build_exception("Cannot load image (filename: %s)", file_name);

        return Py_BuildValue("n", (Py_ssize_t)image_instance);
    }

    auto grp_image_generate_from_handle(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle))
            return py_bad_argument();

        graphic_image_instance *image_instance = graphic_image_instance::new_value();
        image_instance->set_image_pointer((graphic_image *)handle);

        return Py_BuildValue("n", (Py_ssize_t)image_instance);
    }

    auto grp_image_delete(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;

        if (!py_tuple_get_image_instance(args, 0, &image_instance))
            return py_bad_argument();

        if (!image_instance)
            return py_build_none();

        graphic_image_instance::delete_value(image_instance);

        return py_build_none();
    }

    auto grp_image_render(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        if (!py_tuple_get_image_instance(args, 0, &image_instance))
            return py_bad_argument();

        image_instance->render();
        return py_build_none();
    }

    auto grp_set_image_position(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        if (!py_tuple_get_image_instance(args, 0, &image_instance))
            return py_bad_argument();

        float x;
        if (!py_tuple_get_float(args, 1, &x))
            return py_bad_argument();

        float y;
        if (!py_tuple_get_float(args, 2, &y))
            return py_bad_argument();

        image_instance->set_position(x, y);
        return py_build_none();
    }

    auto grp_set_image_diffuse_color(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        if (!py_tuple_get_image_instance(args, 0, &image_instance))
            return py_bad_argument();

        float r;
        if (!py_tuple_get_float(args, 1, &r))
            return py_bad_argument();
        float g;
        if (!py_tuple_get_float(args, 2, &g))
            return py_bad_argument();
        float b;
        if (!py_tuple_get_float(args, 3, &b))
            return py_bad_argument();
        float a;
        if (!py_tuple_get_float(args, 4, &a))
            return py_bad_argument();

        image_instance->set_diffuse_color(r, g, b, a);
        return py_build_none();
    }

    auto grp_get_width(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;

        if (!py_tuple_get_image_instance(args, 0, &image_instance))
            return py_bad_argument();

        if (image_instance->is_empty())
            return py_build_exception("Image is empty");

        return Py_BuildValue("i", image_instance->width());
    }

    auto grp_get_height(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;

        if (!py_tuple_get_image_instance(args, 0, &image_instance))
            return py_bad_argument();

        if (image_instance->is_empty())
            return py_build_exception("Image is empty");

        return Py_BuildValue("i", image_instance->height());
    }

    void initgrp_image() {
        static PyMethodDef s_methods[] =
            {
                {"Render", grp_image_render, METH_VARARGS},
                {"SetPosition", grp_set_image_position, METH_VARARGS},
                {"Generate", grp_image_generate, METH_VARARGS},
                {"GenerateFromHandle", grp_image_generate_from_handle, METH_VARARGS},
                {"Delete", grp_image_delete, METH_VARARGS},
                {"SetDiffuseColor", grp_set_image_diffuse_color, METH_VARARGS},
                {"GetWidth", grp_get_width, METH_VARARGS},
                {"GetHeight", grp_get_height, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };

        Py_InitModule("grpImage", s_methods);
    }
}
