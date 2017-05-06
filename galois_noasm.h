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

#ifndef KCP_GALOIS_NOASM_H
#define KCP_GALOIS_NOASM_H

#include "galois.h"
#include "matrix.h"

#ifdef __cplusplus
extern "C" {
#endif
void galMulSlice(byte c, row_type in, row_type out);
void galMulSliceXor(byte c, row_type in, row_type out);

#ifdef __cplusplus
}
#endif

#endif // KCP_GALOIS_NOASM_H
