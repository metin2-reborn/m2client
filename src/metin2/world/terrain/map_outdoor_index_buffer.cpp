#include "map_outdoor.h"

#include <metin2/log/log.h>

namespace metin2::inline world::inline terrain {
    void map_outdoor::set_index_buffer() {
        void *indices;
        int32_t x, y;

        uint32_t index_num = TERRAIN_PATCHSIZE * TERRAIN_PATCHSIZE * 4;

        uint16_t count[TERRAINPATCH_LODMAX], count2[TERRAINPATCH_LODMAX];
        uint8_t uci;
        for (uci = 0; uci < TERRAINPATCH_LODMAX; ++uci) {
            pwa_indices_[uci] = new uint16_t[index_num];
            memset(pwa_indices_[uci], 0, sizeof(uint16_t) * index_num);
            count[uci] = 0;
            count2[uci] = 0;

            if (!index_buffer_[uci].create(index_num, gpu::format::INDEX16)) {
                metin2::log::error("Cannot create terrain index buffer");
            }
        }

        uint8_t num_line_warp = TERRAIN_PATCHSIZE + 1;

        for (y = 0; y < TERRAIN_PATCHSIZE; y++) {
            if (y % 2 == 0) {
                pwa_indices_[0][(count[0])++] = count2[0];
                pwa_indices_[0][(count[0])++] = count2[0] + num_line_warp;
            } else {
                pwa_indices_[0][(count[0])++] = count2[0] + num_line_warp;
                pwa_indices_[0][(count[0])++] = count2[0];
            }

            for (x = 0; x < TERRAIN_PATCHSIZE; x++) {
                if (y % 2 == 0) {
                    pwa_indices_[0][(count[0])++] = (uint16_t)(count2[0] + 1);
                    pwa_indices_[0][(count[0])++] = (uint16_t)(count2[0] + num_line_warp + 1);
                    count2[0] += (short)1;
                } else {
                    pwa_indices_[0][(count[0])++] = (uint16_t)(count2[0] + num_line_warp - 1);
                    pwa_indices_[0][(count[0])++] = (uint16_t)(count2[0] - 1);
                    count2[0] -= (short)1;
                }

                if (0 == x % 2) {
                    if (0 == y) {
                        if (0 == x) {
                            add_lvl1tl(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        } else if ((TERRAIN_PATCHSIZE - 2) == x) {
                            add_lvl1tr(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        } else {
                            add_lvl1t(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        }
                    } else if ((TERRAIN_PATCHSIZE - 2) == y) {
                        if (0 == x) {
                            add_lvl1bl(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        } else if ((TERRAIN_PATCHSIZE - 2) == x) {
                            add_lvl1br(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        } else {
                            add_lvl1b(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        }
                    } else if (0 == y % 2) {
                        if (0 == x) {
                            add_lvl1l(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        } else if ((TERRAIN_PATCHSIZE - 2) == x) {
                            add_lvl1r(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        } else {
                            add_lvl1m(pwa_indices_[1], count[1], count2[1], num_line_warp);
                        }
                    }

                    count2[1] += 2;
                }

                if (0 == x % 4) {
                    if (0 == y) {
                        if (0 == x) {
                            add_lvl2tl(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        } else if ((TERRAIN_PATCHSIZE - 4) == x) {
                            add_lvl2tr(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        } else {
                            add_lvl2t(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        }
                    } else if ((TERRAIN_PATCHSIZE - 4) == y) {
                        if (0 == x) {
                            add_lvl2bl(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        } else if ((TERRAIN_PATCHSIZE - 4) == x) {
                            add_lvl2br(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        } else {
                            add_lvl2b(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        }
                    } else if (0 == y % 4) {
                        if (0 == x) {
                            add_lvl2l(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        } else if ((TERRAIN_PATCHSIZE - 4) == x) {
                            add_lvl2r(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        } else {
                            add_lvl2m(pwa_indices_[2], count[2], count2[2], num_line_warp);
                        }
                    }

                    count2[2] += 4;
                }
            }

            if (y < TERRAIN_PATCHSIZE - 1) {
                pwa_indices_[0][(count[0])++] = (uint16_t)(count2[0] + num_line_warp);
                pwa_indices_[0][(count[0])++] = (uint16_t)(count2[0] + num_line_warp);

                count2[0] += num_line_warp;
                if (0 == y % 2) {
                    count2[1] += 2;
                }

                if (0 == y % 4) {
                    count2[2] += 4;
                }
            }
        }

        for (uci = 0; uci < TERRAINPATCH_LODMAX; ++uci) {
            num_indices_[uci] = count[uci];
            if (!index_buffer_[uci].lock((void **)&indices)) {
                metin2::log::error("Cannot unlock terrain index buffer");
            }

            memcpy(indices, pwa_indices_[uci], count[uci] * sizeof(uint16_t));
            index_buffer_[uci].unlock();
            delete[] pwa_indices_[uci];
            pwa_indices_[uci] = nullptr;
        }
    }

    void map_outdoor::add_lvl1tl(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + 2;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
        indices[rw_count++] = rw_cur_count + 1;
        indices[rw_count++] = rw_cur_count;
    }

    void map_outdoor::add_lvl1t(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 1;
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
    }

    void map_outdoor::add_lvl1tr(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 2;
        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 1;
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
    }

    void map_outdoor::add_lvl1l(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
    }

    void map_outdoor::add_lvl1r(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 2;
        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
    }

    void map_outdoor::add_lvl1bl(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
    }

    void map_outdoor::add_lvl1b(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
    }

    void map_outdoor::add_lvl1br(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 1;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 2;
        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp + 1;
    }

    void map_outdoor::add_lvl1m(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + 2;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + 2;
    }

    void map_outdoor::add_lvl2tl(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        add_lvl1tl(indices, rw_count, rw_cur_count, ruc_num_line_warp);
        add_lvl1t(indices, rw_count, rw_cur_count + 2, ruc_num_line_warp);
        add_lvl1l(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2, ruc_num_line_warp);
        add_lvl1m(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2 + 2, ruc_num_line_warp);
    }

    void map_outdoor::add_lvl2t(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        add_lvl1t(indices, rw_count, rw_cur_count, ruc_num_line_warp);
        add_lvl1t(indices, rw_count, rw_cur_count + 2, ruc_num_line_warp);

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4 + 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4 + 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
    }

    void map_outdoor::add_lvl2tr(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        add_lvl1t(indices, rw_count, rw_cur_count, ruc_num_line_warp);
        add_lvl1tr(indices, rw_count, rw_cur_count + 2, ruc_num_line_warp);
        add_lvl1m(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2, ruc_num_line_warp);
        add_lvl1r(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2 + 2, ruc_num_line_warp);
    }

    void map_outdoor::add_lvl2l(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        add_lvl1l(indices, rw_count, rw_cur_count, ruc_num_line_warp);
        add_lvl1l(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2, ruc_num_line_warp);

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + 4;

        indices[rw_count++] = rw_cur_count + 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4 + 4;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4 + 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4 + 2;
    }

    void map_outdoor::add_lvl2r(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        add_lvl1r(indices, rw_count, rw_cur_count + 2, ruc_num_line_warp);
        add_lvl1r(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2 + 2, ruc_num_line_warp);

        indices[rw_count++] = rw_cur_count + 2;
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;

        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4 + 2;
    }

    void map_outdoor::add_lvl2bl(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        add_lvl1l(indices, rw_count, rw_cur_count, ruc_num_line_warp);
        add_lvl1m(indices, rw_count, rw_cur_count + 2, ruc_num_line_warp);
        add_lvl1bl(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2, ruc_num_line_warp);
        add_lvl1b(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2 + 2, ruc_num_line_warp);
    }

    void map_outdoor::add_lvl2b(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count;

        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + 4;

        indices[rw_count++] = rw_cur_count + 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 2;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 2 + 4;

        add_lvl1b(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2, ruc_num_line_warp);
        add_lvl1b(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2 + 2, ruc_num_line_warp);
    }

    void map_outdoor::add_lvl2br(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        add_lvl1m(indices, rw_count, rw_cur_count, ruc_num_line_warp);
        add_lvl1r(indices, rw_count, rw_cur_count + 2, ruc_num_line_warp);
        add_lvl1b(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2, ruc_num_line_warp);
        add_lvl1br(indices, rw_count, rw_cur_count + ruc_num_line_warp * 2 + 2, ruc_num_line_warp);
    }

    void map_outdoor::add_lvl2m(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp) {
        indices[rw_count++] = rw_cur_count;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4;
        indices[rw_count++] = rw_cur_count + 4;

        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4;
        indices[rw_count++] = rw_cur_count + ruc_num_line_warp * 4 + 4;
        indices[rw_count++] = rw_cur_count + 4;
    }
}
