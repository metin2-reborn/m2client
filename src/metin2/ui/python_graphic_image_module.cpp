#include <cstdint>
#include <metin2/graphics/grp_expanded_image_instance.h>
#include <metin2/graphics/grp_image.h>
#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/resource.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "python_modules.h"

namespace metin2::inline ui {

    constexpr auto image_instance_converter = &py_handle_converter<graphic_image_instance>;

    auto expanded_image_instance_converter(PyObject *obj, void *out) -> int {
        graphic_image_instance *image_instance;
        if (!py_handle_converter<graphic_image_instance>(obj, &image_instance)) {
            return 0;
        }

        if (!image_instance->is_type(graphic_expanded_image_instance::type())) {
            PyErr_SetString(PyExc_TypeError, "Image is not an expanded image");
            return 0;
        }

        *static_cast<graphic_expanded_image_instance **>(out) = static_cast<graphic_expanded_image_instance *>(image_instance);
        return 1;
    }

    auto grp_image_generate(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        if (!*file_name)
            return Py_BuildValue("i", 0);

        resource *resource = get_resource_manager().resource_pointer(file_name);

        if (!resource->is_type(graphic_image::type())) {
            PyErr_Format(PyExc_ValueError, "%s is not an image", file_name);
            return nullptr;
        }

        graphic_image_instance *image_instance = graphic_image_instance::new_value();
        image_instance->set_image_pointer(static_cast<graphic_image *>(resource));

        if (image_instance->is_empty()) {
            PyErr_Format(PyExc_IOError, "Cannot load image %s", file_name);
            return nullptr;
        }

        return Py_BuildValue("n", (Py_ssize_t)image_instance);
    }

    auto grp_image_generate_from_handle(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        if (!PyArg_ParseTuple(args, "n", &handle)) {
            return nullptr;
        }

        graphic_image_instance *image_instance = graphic_image_instance::new_value();
        image_instance->set_image_pointer((graphic_image *)handle);

        return Py_BuildValue("n", (Py_ssize_t)image_instance);
    }

    auto grp_image_delete(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        if (!PyArg_ParseTuple(args, "O&", image_instance_converter, &image_instance)) {
            return nullptr;
        }

        if (!image_instance)
            Py_RETURN_NONE;

        graphic_image_instance::delete_value(image_instance);

        Py_RETURN_NONE;
    }

    auto grp_image_render(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        if (!PyArg_ParseTuple(args, "O&", image_instance_converter, &image_instance)) {
            return nullptr;
        }

        image_instance->render();
        Py_RETURN_NONE;
    }

    auto grp_set_image_position(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        float x, y;

        if (!PyArg_ParseTuple(args, "O&ff", image_instance_converter, &image_instance, &x, &y)) {
            return nullptr;
        }

        image_instance->set_position(x, y);
        Py_RETURN_NONE;
    }

    auto grp_set_image_diffuse_color(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        float r, g, b, a;

        if (!PyArg_ParseTuple(args, "O&ffff", image_instance_converter, &image_instance, &r, &g, &b, &a)) {
            return nullptr;
        }

        image_instance->set_diffuse_color(r, g, b, a);
        Py_RETURN_NONE;
    }

    auto grp_get_width(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        if (!PyArg_ParseTuple(args, "O&", image_instance_converter, &image_instance)) {
            return nullptr;
        }

        if (image_instance->is_empty()) {
            PyErr_SetString(PyExc_IOError, "Image is empty");
            return nullptr;
        }

        return Py_BuildValue("i", image_instance->width());
    }

    auto grp_get_height(PyObject *self, PyObject *args) -> PyObject * {
        graphic_image_instance *image_instance;
        if (!PyArg_ParseTuple(args, "O&", image_instance_converter, &image_instance)) {
            return nullptr;
        }

        if (image_instance->is_empty()) {
            PyErr_SetString(PyExc_IOError, "Image is empty");
            return nullptr;
        }

        return Py_BuildValue("i", image_instance->height());
    }

    void init_graphics_image_module() {
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

        init_python_module("metin2.graphics.image", s_methods);
    }
}
