/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
#define BOARD_SMK37 1
#define PLAT_HOST 1
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../app/app.h"
#include "../../app/dsp/sample.h"
#include "../../base/bsp/chip/fm1_flash_policy.h"
static uint8_t nor[0x100000], saved[0x100000];
static int fail_after = -1;
static unsigned writes;
static int cut(void) { writes++; if (!fail_after) return 1; if (fail_after > 0) fail_after--; return 0; }
static int st_read(uint32_t off, void *dst, uint32_t n)
{ assert(FL_READ_OK(off,n)); memcpy(dst,nor+off,n); return 0; }
static int erase(uint32_t off)
{ assert(FL_GUARD(off,4096)); if (cut()) return -1; memset(nor+off,255,4096); return 0; }
static int st_erase(uint32_t off) { assert(FL_STORE_OK(off,4096)); return erase(off); }
static int st_prog(uint32_t off, const void *p, uint32_t n)
{
    assert(FL_GUARD(off,n) && n <= 256 && (off & 255u) + n <= 256);
    if (cut()) return -1;
    for (uint32_t i=0;i<n;i++) nor[off+i] &= ((const uint8_t *)p)[i];
    return 0;
}
static int fl_erase4k(uint32_t off, uint32_t *took) { *took=0; return erase(off); }
static void audio_silence(void) {}
#include "../../base/device/storage.c"
#include "../../base/device/smk37/owner.c"
int main(void)
{
    assert(OBJ_NOBJ == 24 && APP_STORE_POOL_N == 20 && APP_STORE_POOL_OLD == 20);
    uint32_t address;
    assert(SMP_BANK_SIZE == 100u*1024u);
    assert(smp_reg_of(0,&address)==0 && address==0xB8000);
    assert(smp_reg_of(0x2FFF,&address)==0 && address==0xBAFFF);
    assert(smp_reg_of(0x3000,&address)==1 && address==0xD4000);
    assert(smp_reg_of(SMP_BANK_SIZE-1,&address)==1 && address==0xE9FFF);
    assert(smp_reg_of(SMP_BANK_SIZE,&address)==-1);
    for (unsigned o=0;o<OBJ_NOBJ;o++) {
        unsigned n=st_pooled(o) ? APP_STORE_POOL_N : 2;
        for (unsigned i=0;i<n;i++) {
            uint32_t a=st_sector(o,i);
            assert(FL_STORE_OK(a,4096) && !(a & 4095u) && a < 0xB8000 && a != 0xAF000);
            if (!st_pooled(o)) {
                for (unsigned j=0;j<20;j++) assert(a!=APP_STORE_POOL_ADDR(j));
                for (unsigned other=0;other<=o;other++) if (!st_pooled(other))
                    for (unsigned copy=0;copy<2;copy++)
                        if (other!=o || copy!=i) assert(a!=st_sector(other,copy));
            }
            for (int r=0;r<SMP_NREG;r++) assert(a+4096<=smp_reg_off(r) || a>=smp_reg_off(r)+smp_reg_size(r));
        }
    }
    for (unsigned i=0;i<20;i++) for (unsigned j=0;j<i;j++) assert(APP_STORE_POOL_ADDR(i)!=APP_STORE_POOL_ADDR(j));
    for (int n=0;n<=32;n++) {
        memset(nor,0x5A,sizeof nor); fail_after=n;
        (void)smk_owner_boot(); fail_after=-1; assert(!smk_owner_boot());
        for (uint32_t a=0x9D000;a<0xBB000;a++)
            if (a<0xAF000 || a>=0xB0000) assert(nor[a]==255);
        for (uint32_t a=0;a<sizeof nor;a++)
            if (!(a>=0x9D000 && a<0xBB000) && !(a>=0xBC000 && a<0xBD000)) assert(nor[a]==0x5A);
    }
    unsigned before=writes; assert(!smk_owner_boot() && writes==before);
    memcpy(nor+0xAF000,"SMK37DATA\0FELUCCA",16);
    assert(!smk_owner_boot() && !memcmp(nor+0xAF000,"SMK37DATA\0LOOM8\0\0",16));
    assert(!st_pool_boot() && st_pool_scan==20);
    uint32_t value=0x12345678, got;
    assert(!st_save(OBJ_PROJ0,&value,4)); memcpy(saved,nor,sizeof nor);
    for (int n=0;n<=3;n++) {
        memcpy(nor,saved,sizeof nor); fail_after=n; uint32_t next=0x87654321;
        (void)st_save(OBJ_PROJ0,&next,4); fail_after=-1;
        assert(st_load(OBJ_PROJ0,&got,4)==4 && (got==value || got==next));
    }
    memcpy(nor,saved,sizeof nor);
    for (unsigned i=1;i<20;i++) { value=i+100; assert(!st_save(OBJ_PROJ0+i,&value,4)); }
    assert(st_pool_room()==0 && st_save(OBJ_PROJ0+20,&value,4)==-8);
    for (unsigned i=1;i<20;i++) assert(st_load(OBJ_PROJ0+i,&got,4)==4 && got==i+100);
    memcpy(nor+0xEAFF8,"usrdata",8); fail_after=-1;
    assert(!smk_stock_release());
    for (uint32_t a=0xEA000;a<0xEB000;a++) assert(nor[a]==255);
    assert(nor[0xC1000]==0x5A && nor[0xF4000]==0x5A && nor[0xFF000]==0x5A);
    puts("SMK storage: all objects, split pool, power cuts, full-store refusal, owner tag and stock marker passed");
    return 0;
}
