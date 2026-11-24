/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#define s_eq(a,b) (!strcmp((a),(b)))
#define s_len(s) ((uint32_t)strlen(s))
#include "../../app/smk37/knob_labels.h"
int main(void)
{
    assert(!strcmp(smk_knob_label("MASTER"), "K1"));
    assert(!strcmp(smk_knob_label("SELECT"), "K2"));
    assert(!strcmp(smk_knob_label("ALGO"), "K3"));
    assert(!strcmp(smk_knob_label("ALGORITHM"), "K3"));
    assert(!strcmp(smk_knob_label("PRESET"), "K4"));
    assert(!strcmp(smk_knob_label("KNOBS"), "K5-8"));
    const char *keep[] = {"HOME", "SEL", "EDIT", "GLO", "SAVE", "PLAY", "REC", "LFO", "PRESET SAVED", "my PRESET"};
    for (unsigned i = 0; i < sizeof keep / sizeof keep[0]; i++) assert(smk_knob_label(keep[i]) == keep[i]);
    for (unsigned i = 0; i < 8; i++) assert(SMK_KNOB_LABEL[i][0] == 'K' && SMK_KNOB_LABEL[i][1] == '1' + (int)i);
    char hint[64] = "SELECT page|ALGO track|PRESET sound";
    smk_master_hint(hint, sizeof hint);
    assert(!strcmp(hint, "MASTER volume|SELECT page|ALGO track|PRESET sound"));
    char small[12] = "ALGO track";
    smk_master_hint(small, sizeof small);
    assert(!strcmp(small, "ALGO track"));
    return 0;
}
