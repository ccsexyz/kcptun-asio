// The MIT License (MIT)

// Copyright (c) 2016 Daniel Fu

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// Created by 理 傅 on 2016/12/30.
//

#include "galois.h"
#include <stdexcept>

extern const int fieldSize;
extern byte mulTable[256][256];
extern const byte logTable[];
extern byte expTable[];

byte galAdd(byte a, byte b) { return a ^ b; }

byte galSub(byte a, byte b) { return a ^ b; }

byte galMultiply(byte a, byte b) { return mulTable[a][b]; }

byte galDivide(byte a, byte b) {
    if (a == 0) {
        return 0;
    }

    if (b == 0) {
        throw std::invalid_argument("Argument 'divisor' is 0");
    }

    int logA = logTable[a];
    int logB = logTable[b];
    int logResult = logA - logB;
    if (logResult < 0) {
        logResult += 255;
    }
    return expTable[logResult];
}

byte galExp(byte a, byte n) {
    if (n == 0) {
        return 1;
    }
    if (a == 0) {
        return 0;
    }

    int logA = logTable[a];
    int logResult = logA * n;
    while (logResult >= 255) {
        logResult -= 255;
    }
    return expTable[logResult];
}
