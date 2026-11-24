// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 Tony Byrne
import assert from "node:assert/strict";
import { sendBank } from "../../app/web/samples/loom_samples.js";
for (const [bankKiB, deviceKiB] of [[216,100],[100,216]]) {
  const img = new Uint8Array(2048);
  new DataView(img.buffer).setUint32(16, bankKiB * 1024, true);
  const calls = [];
  const link = { async req(cmd) {
    calls.push(cmd);
    assert.equal(cmd, 15, "mismatched upload must not erase or begin");
    return [2, deviceKiB & 127, deviceKiB >> 7, 0,0,0,0,0];
  } };
  await assert.rejects(sendBank(link, img), /capacities differ/);
  assert.deepEqual(calls, [15]);
}
console.log("SMK/FM-1 cross-bank uploads refused before erase");
