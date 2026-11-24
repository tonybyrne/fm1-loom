/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
/* Compile the real browser sample path: SMK has empty break slots, FM-1 keeps its originals. */
#include <assert.h>
#include <string.h>
#include "../../app/dsp/sample.c"
int main(void)
{
    static const char *const names[] = {"classic", "heat", "choke", "ecstasy"};
    assert(smp_factory() == 16 && SMP_BREAK == 12);
    for (int i = 0; i < SMP_BREAK; i++)
        assert(smp_get(i) && smp_get(i)->n);
    for (int i = 0; i < 4; i++) {
        const smp_t *s = smp_get(SMP_BREAK + i);
        assert(!strcmp(smp_name(SMP_BREAK + i), names[i]));
#if BOARD_SMK37
        assert(!s);
#else
        assert(s && s->n);
#endif
    }
    assert(!strcmp(smp_name(16), "user 1"));
    return 0;
}
