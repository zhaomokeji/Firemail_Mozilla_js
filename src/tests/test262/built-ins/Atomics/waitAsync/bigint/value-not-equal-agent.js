// |reftest| skip async -- Atomics.waitAsync is not supported
// Copyright (C) 2020 Rick Waldron. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.
/*---
esid: sec-atomics.waitasync
description: >
  Returns "not-equal" when value arg does not match an index in the typedArray
info: |
  Atomics.waitAsync( typedArray, index, value, timeout )

  1. Return DoWait(async, typedArray, index, value, timeout).

  DoWait ( mode, typedArray, index, value, timeout )

  16. Let w be ! AtomicLoad(typedArray, i).
  17. If v is not equal to w, then
    a. Perform LeaveCriticalSection(WL).
    b. If mode is sync, then
      i. Return the String "not-equal".
    c. Perform ! Call(capability.[[Resolve]], undefined, « "not-equal" »).
    d. Return promiseCapability.[[Promise]].

flags: [async]
includes: [atomicsHelper.js]
features: [Atomics.waitAsync, SharedArrayBuffer, TypedArray, Atomics, BigInt, arrow-function, async-functions]
---*/
assert.sameValue(typeof Atomics.waitAsync, 'function');
const RUNNING = 1;
const value = 42n;

$262.agent.start(`
  $262.agent.receiveBroadcast(function(sab) {
    const i64a = new BigInt64Array(sab);
    Atomics.add(i64a, ${RUNNING}, 1n);
    $262.agent.report(Atomics.store(i64a, 0, 42n));
    $262.agent.report(Atomics.waitAsync(i64a, 0, 0n).value);
    $262.agent.leaving();
  });
`);

const i64a = new BigInt64Array(new SharedArrayBuffer(BigInt64Array.BYTES_PER_ELEMENT * 4));

$262.agent.safeBroadcastAsync(i64a, RUNNING, 1n).then(async agentCount => {
  assert.sameValue(agentCount, 1n);

  assert.sameValue(
    await $262.agent.getReportAsync(),
    value.toString(),
    'Atomics.store(i64a, 0, 42n) returns 42'
  );

  assert.sameValue(
    await $262.agent.getReportAsync(),
    'not-equal',
    'Atomics.waitAsync(i64a, 0, 0n).value resolves to "not-equal"'
  );

  assert.sameValue(
    Atomics.notify(i64a, 0, 1),
    0,
    'Atomics.notify(i64a, 0, 1) returns 0 (nothing to notify)'
  );
}).then($DONE, $DONE);
