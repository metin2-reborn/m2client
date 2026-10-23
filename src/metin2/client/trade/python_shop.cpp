#include "python_shop.h"
#include <metin2/log/log.h>

#include <metin2/client/protocol/python_network_stream.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline trade {

    void python_shop::set_tab_coin_type(uint8_t tab_idx, uint8_t coin_type) {
        if (tab_idx >= tab_count_) {
            metin2::log::error("Shop tab {} out of range (must be below {})", tab_idx, SHOP_TAB_COUNT_MAX);
            return;
        }
        shoptabs_[tab_idx].coin_type = coin_type;
    }

    auto python_shop::tab_coin_type(uint8_t tab_idx) -> uint8_t {
        if (tab_idx >= tab_count_) {
            metin2::log::error("Shop tab {} out of range (must be below {})", tab_idx, SHOP_TAB_COUNT_MAX);
            return 0xff;
        }
        return shoptabs_[tab_idx].coin_type;
    }

    void python_shop::set_tab_name(uint8_t tab_idx, const char *name) {
        if (tab_idx >= tab_count_) {
            metin2::log::error("Shop tab {} out of range (must be below {})", tab_idx, SHOP_TAB_COUNT_MAX);
            return;
        }
        shoptabs_[tab_idx].name_value = name;
    }

    auto python_shop::tab_name(uint8_t tab_idx) -> const char * {
        if (tab_idx >= tab_count_) {
            metin2::log::error("Shop tab {} out of range (must be below {})", tab_idx, SHOP_TAB_COUNT_MAX);
            return nullptr;
        }

        return shoptabs_[tab_idx].name_value.c_str();
    }

    void python_shop::set_item_data(uint32_t index, const shop_item_data &shop_item_data) {
        uint8_t tab_idx = index / SHOP_HOST_ITEM_MAX_NUM;
        uint32_t slot_pos = index % SHOP_HOST_ITEM_MAX_NUM;

        set_item_data(tab_idx, slot_pos, shop_item_data);
    }

    auto python_shop::get_item_data(uint32_t index, const shop_item_data **item_data) -> bool {
        uint8_t tab_idx = index / SHOP_HOST_ITEM_MAX_NUM;
        uint32_t slot_pos = index % SHOP_HOST_ITEM_MAX_NUM;

        return get_item_data(tab_idx, slot_pos, item_data);
    }

    void python_shop::set_item_data(uint8_t tab_idx, uint32_t slot_pos, const shop_item_data &shop_item_data) {
        if (tab_idx >= SHOP_TAB_COUNT_MAX || slot_pos >= SHOP_HOST_ITEM_MAX_NUM) {
            metin2::log::error("Shop tab {0} or slot {2} out of range (tab must be below {1}, slot below {3})", tab_idx, SHOP_TAB_COUNT_MAX, slot_pos, SHOP_HOST_ITEM_MAX_NUM);
            return;
        }

        shoptabs_[tab_idx].items[slot_pos] = shop_item_data;
    }

    auto python_shop::get_item_data(uint8_t tab_idx, uint32_t slot_pos, const shop_item_data **item_data) -> bool {
        if (tab_idx >= SHOP_TAB_COUNT_MAX || slot_pos >= SHOP_HOST_ITEM_MAX_NUM) {
            metin2::log::error("Shop tab {0} or slot {2} out of range (tab must be below {1}, slot below {3})", tab_idx, SHOP_TAB_COUNT_MAX, slot_pos, SHOP_HOST_ITEM_MAX_NUM);
            return false;
        }

        *item_data = &shoptabs_[tab_idx].items[slot_pos];

        return true;
    }

    void python_shop::clear_private_shop_stock() {
        private_shop_item_stock_value_.clear();
    }
    void python_shop::add_private_shop_item_stock(item_pos item_pos, uint8_t display_pos, uint32_t price) {
        del_private_shop_item_stock(item_pos);

        shop_item_table selling_item;
        selling_item.vnum = 0;
        selling_item.count = 0;
        selling_item.pos = item_pos;
        selling_item.price = price;
        selling_item.display_pos = display_pos;
        private_shop_item_stock_value_.insert(std::make_pair(item_pos, selling_item));
    }
    void python_shop::del_private_shop_item_stock(item_pos item_pos) {
        if (private_shop_item_stock_value_.end() == private_shop_item_stock_value_.find(item_pos))
            return;

        private_shop_item_stock_value_.erase(item_pos);
    }
    auto python_shop::private_shop_item_price(item_pos item_pos) -> int {
        auto itor = private_shop_item_stock_value_.find(item_pos);

        if (private_shop_item_stock_value_.end() == itor)
            return 0;

        shop_item_table &shop_item_table = itor->second;
        return shop_item_table.price;
    }
    struct item_stock_sort_func {
        auto operator()(shop_item_table &left, shop_item_table &right) -> bool {
            return left.display_pos < right.display_pos;
        }
    };
    void python_shop::build_private_shop(const char *name) {
        std::vector<shop_item_table> item_stock;
        item_stock.reserve(private_shop_item_stock_value_.size());

        auto itor = private_shop_item_stock_value_.begin();
        for (; itor != private_shop_item_stock_value_.end(); ++itor) {
            item_stock.push_back(itor->second);
        }

        std::sort(item_stock.begin(), item_stock.end(), item_stock_sort_func());

        get_python_network_stream().send_build_private_shop_packet(name, item_stock);
    }

    void python_shop::open(bool is_private_shop, bool is_main_private_shop) {
        is_shoping_ = true;
        is_private_shop_ = is_private_shop;
        is_main_player_private_shop_ = is_main_private_shop;
    }

    void python_shop::close() {
        is_shoping_ = false;
        is_private_shop_ = false;
        is_main_player_private_shop_ = false;
    }

    auto python_shop::is_open() -> bool {
        return is_shoping_;
    }

    auto python_shop::is_private_shop() -> bool {
        return is_private_shop_;
    }

    auto python_shop::is_main_player_private_shop() -> bool {
        return is_main_player_private_shop_;
    }

    void python_shop::clear() {
        is_shoping_ = false;
        is_private_shop_ = false;
        is_main_player_private_shop_ = false;
        clear_private_shop_stock();
        tab_count_ = 1;

        for (int i = 0; i < SHOP_TAB_COUNT_MAX; i++)
            memset(shoptabs_[i].items, 0, sizeof(shop_item_data) * SHOP_HOST_ITEM_MAX_NUM);
    }

    python_shop::python_shop(void) {
        clear();
    }

    python_shop::~python_shop(void) = default;

    auto shop_open(PyObject *self, PyObject *args) -> PyObject * {
        int is_private_shop = 0, is_main_private_shop = 0;
        if (!PyArg_ParseTuple(args, "|ii", &is_private_shop, &is_main_private_shop)) {
            return nullptr;
        }

        python_shop &shop = get_python_shop();
        shop.open(is_private_shop, is_main_private_shop);
        Py_RETURN_NONE;
    }

    auto shop_close(PyObject *self, PyObject *args) -> PyObject * {
        python_shop &shop = get_python_shop();
        shop.close();
        Py_RETURN_NONE;
    }

    auto shop_is_open(PyObject *self, PyObject *args) -> PyObject * {
        python_shop &shop = get_python_shop();
        return Py_BuildValue("i", shop.is_open());
    }

    auto shop_is_prviate_shop(PyObject *self, PyObject *args) -> PyObject * {
        python_shop &shop = get_python_shop();
        return Py_BuildValue("i", shop.is_private_shop());
    }

    auto shop_is_main_player_private_shop(PyObject *self, PyObject *args) -> PyObject * {
        python_shop &shop = get_python_shop();
        return Py_BuildValue("i", shop.is_main_player_private_shop());
    }

    auto shop_get_item_id(PyObject *self, PyObject *args) -> PyObject * {
        int pos;
        if (!PyArg_ParseTuple(args, "i", &pos)) {
            return nullptr;
        }

        const shop_item_data *item_data;
        if (get_python_shop().get_item_data(pos, &item_data))
            return Py_BuildValue("i", item_data->vnum);

        return Py_BuildValue("i", 0);
    }

    auto shop_get_item_count(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        const shop_item_data *item_data;
        if (get_python_shop().get_item_data(index, &item_data))
            return Py_BuildValue("i", item_data->count);

        return Py_BuildValue("i", 0);
    }

    auto shop_get_item_price(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        const shop_item_data *item_data;
        if (get_python_shop().get_item_data(index, &item_data))
            return Py_BuildValue("i", item_data->price);

        return Py_BuildValue("i", 0);
    }

    auto shop_get_item_metin_socket(PyObject *self, PyObject *args) -> PyObject * {
        int index, metin_socket_index;
        if (!PyArg_ParseTuple(args, "ii", &index, &metin_socket_index)) {
            return nullptr;
        }

        const shop_item_data *item_data;
        if (get_python_shop().get_item_data(index, &item_data))
            return Py_BuildValue("i", item_data->sockets[metin_socket_index]);

        return Py_BuildValue("i", 0);
    }

    auto shop_get_item_attribute(PyObject *self, PyObject *args) -> PyObject * {
        int index, attr_slot_index;
        if (!PyArg_ParseTuple(args, "ii", &index, &attr_slot_index)) {
            return nullptr;
        }

        if (attr_slot_index >= 0 && attr_slot_index < ITEM_ATTRIBUTE_SLOT_MAX_NUM) {
            const shop_item_data *item_data;
            if (get_python_shop().get_item_data(index, &item_data))
                return Py_BuildValue("ii", item_data->attr[attr_slot_index].type_value, item_data->attr[attr_slot_index].value);
        }

        return Py_BuildValue("ii", 0, 0);
    }

    auto shop_clear_private_shop_stock(PyObject *self, PyObject *args) -> PyObject * {
        get_python_shop().clear_private_shop_stock();
        Py_RETURN_NONE;
    }
    auto shop_add_private_shop_item_stock(PyObject *self, PyObject *args) -> PyObject * {
        uint8_t item_window_type;
        uint16_t item_slot_index;
        int display_slot_index, price;

        if (!PyArg_ParseTuple(args, "bHii", &item_window_type, &item_slot_index, &display_slot_index, &price)) {
            return nullptr;
        }

        get_python_shop().add_private_shop_item_stock(item_pos(item_window_type, item_slot_index), display_slot_index, price);
        Py_RETURN_NONE;
    }
    auto shop_del_private_shop_item_stock(PyObject *self, PyObject *args) -> PyObject * {
        uint8_t item_window_type;
        uint16_t item_slot_index;

        if (!PyArg_ParseTuple(args, "bH", &item_window_type, &item_slot_index)) {
            return nullptr;
        }

        get_python_shop().del_private_shop_item_stock(item_pos(item_window_type, item_slot_index));
        Py_RETURN_NONE;
    }
    auto shop_get_private_shop_item_price(PyObject *self, PyObject *args) -> PyObject * {
        uint8_t item_window_type;
        uint16_t item_slot_index;

        if (!PyArg_ParseTuple(args, "bH", &item_window_type, &item_slot_index)) {
            return nullptr;
        }

        int value = get_python_shop().private_shop_item_price(item_pos(item_window_type, item_slot_index));
        return Py_BuildValue("i", value);
    }
    auto shop_build_private_shop(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!PyArg_ParseTuple(args, "s", &name)) {
            return nullptr;
        }

        get_python_shop().build_private_shop(name);
        Py_RETURN_NONE;
    }

    auto shop_get_tab_count(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_shop().tab_count());
    }

    auto shop_get_tab_name(PyObject *self, PyObject *args) -> PyObject * {
        uint8_t tab_idx;
        if (!PyArg_ParseTuple(args, "b", &tab_idx)) {
            return nullptr;
        }

        return Py_BuildValue("s", get_python_shop().tab_name(tab_idx));
    }

    auto shop_get_tab_coin_type(PyObject *self, PyObject *args) -> PyObject * {
        uint8_t tab_idx;
        if (!PyArg_ParseTuple(args, "b", &tab_idx)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_shop().tab_coin_type(tab_idx));
    }

    void init_trade_shop_module() {
        static PyMethodDef s_methods[] =
            {
                {"Open", shop_open, METH_VARARGS},
                {"Close", shop_close, METH_VARARGS},
                {"IsOpen", shop_is_open, METH_VARARGS},
                {"IsPrivateShop", shop_is_prviate_shop, METH_VARARGS},
                {"IsMainPlayerPrivateShop", shop_is_main_player_private_shop, METH_VARARGS},
                {"GetItemID", shop_get_item_id, METH_VARARGS},
                {"GetItemCount", shop_get_item_count, METH_VARARGS},
                {"GetItemPrice", shop_get_item_price, METH_VARARGS},
                {"GetItemMetinSocket", shop_get_item_metin_socket, METH_VARARGS},
                {"GetItemAttribute", shop_get_item_attribute, METH_VARARGS},
                {"GetTabCount", shop_get_tab_count, METH_VARARGS},
                {"GetTabName", shop_get_tab_name, METH_VARARGS},
                {"GetTabCoinType", shop_get_tab_coin_type, METH_VARARGS},

                {"ClearPrivateShopStock", shop_clear_private_shop_stock, METH_VARARGS},
                {"AddPrivateShopItemStock", shop_add_private_shop_item_stock, METH_VARARGS},
                {"DelPrivateShopItemStock", shop_del_private_shop_item_stock, METH_VARARGS},
                {"GetPrivateShopItemPrice", shop_get_private_shop_item_price, METH_VARARGS},
                {"BuildPrivateShop", shop_build_private_shop, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };
        PyObject *module = init_python_module("metin2.trade.shop", s_methods);

        PyModule_AddIntConstant(module, "SHOP_SLOT_COUNT", SHOP_HOST_ITEM_MAX_NUM);
        PyModule_AddIntConstant(module, "SHOP_COIN_TYPE_GOLD", SHOP_COIN_TYPE_GOLD);
        PyModule_AddIntConstant(module, "SHOP_COIN_TYPE_SECONDARY_COIN", SHOP_COIN_TYPE_SECONDARY_COIN);
    }
}
