'use strict';
// Copyright (c) 2012 Ecma International.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
es5id: 10.4.3-1-33gs
description: >
    Strict - checking 'this' from a global scope (FunctionDeclaration
    defined within an Anonymous FunctionExpression inside strict mode)
flags: [onlyStrict]
---*/

if (! ((function () {
    function f() {
        return typeof this;
    }
    return (f()==="undefined") && ((typeof this)==="undefined");
})())) {
    throw "'this' had incorrect value!";
}

reportCompare(0, 0);
