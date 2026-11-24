/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Tony Byrne */
/* Physical captions only: preset names, musical parameters and button names keep their meaning. */
static const char *const SMK_KNOB_LABEL[8] = {"K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8"};
static const char *smk_knob_label(const char *s)
{
    if (s_eq(s, "MASTER")) return "K1";
    if (s_eq(s, "SELECT")) return "K2";
    if (s_eq(s, "ALGO") || s_eq(s, "ALGORITHM")) return "K3";
    if (s_eq(s, "PRESET") || s_eq(s, "PRESETS")) return "K4";
    if (s_eq(s, "KNOBS")) return "K5-8";
    return s;
}
static void smk_master_hint(char *s, uint32_t capacity)
{
    static const char prefix[] = "MASTER volume|";
    uint32_t n = s_len(s), add = sizeof prefix - 1u;
    if (!n || n + add >= capacity) return;
    for (uint32_t i = n + 1u; i; i--) s[i - 1u + add] = s[i - 1u];
    for (uint32_t i = 0; i < add; i++) s[i] = prefix[i];
}
