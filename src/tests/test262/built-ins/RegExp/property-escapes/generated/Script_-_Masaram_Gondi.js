// Copyright 2020 Mathias Bynens. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
author: Mathias Bynens
description: >
  Unicode property escapes for `Script=Masaram_Gondi`
info: |
  Generated by https://github.com/mathiasbynens/unicode-property-escapes-tests
  Unicode v13.0.0
esid: sec-static-semantics-unicodematchproperty-p
features: [regexp-unicode-property-escapes]
includes: [regExpUtils.js]
---*/

const matchSymbols = buildString({
  loneCodePoints: [
    0x011D3A
  ],
  ranges: [
    [0x011D00, 0x011D06],
    [0x011D08, 0x011D09],
    [0x011D0B, 0x011D36],
    [0x011D3C, 0x011D3D],
    [0x011D3F, 0x011D47],
    [0x011D50, 0x011D59]
  ]
});
testPropertyEscapes(
  /^\p{Script=Masaram_Gondi}+$/u,
  matchSymbols,
  "\\p{Script=Masaram_Gondi}"
);
testPropertyEscapes(
  /^\p{Script=Gonm}+$/u,
  matchSymbols,
  "\\p{Script=Gonm}"
);
testPropertyEscapes(
  /^\p{sc=Masaram_Gondi}+$/u,
  matchSymbols,
  "\\p{sc=Masaram_Gondi}"
);
testPropertyEscapes(
  /^\p{sc=Gonm}+$/u,
  matchSymbols,
  "\\p{sc=Gonm}"
);

const nonMatchSymbols = buildString({
  loneCodePoints: [
    0x011D07,
    0x011D0A,
    0x011D3B,
    0x011D3E
  ],
  ranges: [
    [0x00DC00, 0x00DFFF],
    [0x000000, 0x00DBFF],
    [0x00E000, 0x011CFF],
    [0x011D37, 0x011D39],
    [0x011D48, 0x011D4F],
    [0x011D5A, 0x10FFFF]
  ]
});
testPropertyEscapes(
  /^\P{Script=Masaram_Gondi}+$/u,
  nonMatchSymbols,
  "\\P{Script=Masaram_Gondi}"
);
testPropertyEscapes(
  /^\P{Script=Gonm}+$/u,
  nonMatchSymbols,
  "\\P{Script=Gonm}"
);
testPropertyEscapes(
  /^\P{sc=Masaram_Gondi}+$/u,
  nonMatchSymbols,
  "\\P{sc=Masaram_Gondi}"
);
testPropertyEscapes(
  /^\P{sc=Gonm}+$/u,
  nonMatchSymbols,
  "\\P{sc=Gonm}"
);

reportCompare(0, 0);
