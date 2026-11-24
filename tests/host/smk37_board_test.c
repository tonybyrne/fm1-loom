/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define fm1_ticks original_ticks
#include "fm1_time.h"
#undef fm1_ticks
#include "fm1_gpio.h"
#undef FM1_PR
static uint32_t ports[8][8], regs[10], ticks, masked;
#define FM1_PR(p,r) ports[p][(r)/4]
static uint32_t fm1_ticks(void) { return ticks; }
static uint32_t irq_save(void) { uint32_t old = masked; masked = 1; return old; }
static void irq_restore(uint32_t old) { masked = old; }
static uint32_t *smk_reg(uint32_t a)
{
    const uint32_t addr[] = {0x11E00,0x11E04,0x11E0C,0x11E10,0x51020,0x16A0C,0x13100,0x13104,0x11900};
    for (unsigned i = 0; i < 9; i++) if (a == addr[i]) return regs + i;
    abort();
}
#define SMK_REG(a) (*smk_reg(a))
#include "../../base/bsp/boards/smk37/fm1_input.h"
static void contact(uint8_t code, int on)
{
    uint8_t mask = (uint8_t)(1u << (code & 15));
    if (on) fm1_in.raw[code >> 4] |= mask; else fm1_in.raw[code >> 4] &= (uint8_t)~mask;
}
static void frame(void) { ticks += 38400; smk_frame(); }
int main(void)
{
    uint8_t used[256] = {0};
    for (unsigned k = 0; k < 37; k++) for (unsigned c = 0; c < 2; c++) {
        uint8_t code = SMK_KEYC[k][c]; assert((code & 15) < 5 && !used[code]); used[code] = 1;
    }
    for (unsigned i = 0; i < 18; i++) {
        uint8_t code = SMK_BTNC[i]; assert(((code & 15) == 5 || (code & 15) == 6) && !used[code]); used[code] = 1;
    }
    for (unsigned i = 0; i < 8; i++) for (unsigned c = 0; c < 2; c++) {
        uint8_t code = SMK_ENCC[i][c]; assert((code & 15) == 7 && !used[code]); used[code] = 1;
    }
    for (unsigned p = 0; p < 8; p++) for (unsigned r = 0; r < 8; r++) ports[p][r] = UINT32_MAX;
    fm1_input_init();
    assert(ports[FM1_PB][FM1_DIR/4] == UINT32_MAX);
    assert((ports[FM1_PH][FM1_DIR/4] & ((1u<<6)|(1u<<8)|(1u<<9))) == ((1u<<6)|(1u<<8)|(1u<<9)));
    assert(ports[FM1_PA][FM1_DIR/4] & (1u << 9));
    assert((ports[FM1_PC][FM1_DIR/4] & (1u<<9)) && !(ports[FM1_PC][FM1_DIE/4] & (1u<<9)));
    uint8_t trim[0x22C] = {0}, before[0x22C];
    const uint16_t points[2][5] = {{50,550,720,850,950},{5,640,800,920,980}};
    for (unsigned i = 0; i < 53; i++) for (unsigned j = 0; j < 5; j++) {
        uint16_t v = points[i >= 16][j]; trim[8+i*10+j*2] = (uint8_t)v; trim[9+i*10+j*2] = (uint8_t)(v >> 8);
    }
    memcpy(trim + 0x224, "trimdat", 8); memcpy(before, trim, sizeof trim);
    assert(smk_trim_load(trim));
    assert(smk_velocity(16, 970) > smk_velocity(16, 700));
    contact(SMK_KEYC[5][0], 1); frame();
    assert(!fm1_in.notes);
    contact(SMK_KEYC[5][1], 1); frame();
    assert(fm1_in.notes == 1 && fm1_in.key_vel[5] > 0 && fm1_in.key_vel[5] <= 127);
    contact(SMK_KEYC[5][0], 0); contact(SMK_KEYC[5][1], 0);
    frame(); frame(); assert(fm1_in.notes == 1); frame(); assert(!fm1_in.notes);
    contact(SMK_BTNC[17], 1); frame(); assert(!(fm1_in.buttons & (1u << 17)));
    frame(); assert(fm1_in.buttons & (1u << 17));
    contact(SMK_BTNC[17], 0); frame(); frame(); frame(); assert(fm1_in.buttons & (1u << 17));
    frame(); assert(!(fm1_in.buttons & (1u << 17)));
    fm1_in.enc_last[0] = 0; fm1_in.enc_hist[0] = 0x38;
    const uint8_t cycle[] = {1,3,2,0};
    for (unsigned round = 0; round < 2; round++) {
        for (unsigned j = 0; j < 4; j++) {
            contact(SMK_ENCC[0][0], cycle[j] & 1); contact(SMK_ENCC[0][1], cycle[j] & 2); frame(); frame();
        }
        masked = 1; assert(fm1_enc_take(0) == -1 && masked == 1); masked = 0;
    }
    smk_key_event(0, 90); smk_key_event(0, 0);
    assert(smk_extkey_take() == 90 && smk_extkey_take() == 0 && smk_extkey_take() == -1);
    smk_pad_event(0, 80); assert(fm1_in.pnotes == (1u << SMK_WHITE[0]));
    smk_pad_event(0, 0); assert(!fm1_in.pnotes);
    assert(!memcmp(trim, before, sizeof trim));
    trim[0x224] = 'x'; assert(!smk_trim_load(trim) && !smk_trim);
    assert(smk_velocity(16, 1000) == 127);
    puts("SMK input: safe pins, read-only trim, contacts, debounce, detents and event routing passed");
    return 0;
}
