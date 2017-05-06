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

#ifndef KCP_MATRIX_H
#define KCP_MATRIX_H

#include "galois.h"
#include <memory>
#include <vector>

using row_type = std::shared_ptr<std::vector<byte>>;

struct matrix {
    // newMatrix returns a matrix of zeros.
    static matrix newMatrix(int rows, int cols);

    // IdentityMatrix returns an identity matrix of the given empty.
    static matrix identityMatrix(int size);

    // Create a Vandermonde matrix, which is guaranteed to have the
    // property that any subset of rows that forms a square matrix
    // is invertible.
    static matrix vandermonde(int rows, int cols);

    // Multiply multiplies this matrix (the one on the left) by another
    // matrix (the one on the right) and returns a new matrix with the result.
    matrix Multiply(matrix &right);

    // Augment returns the concatenation of this matrix and the matrix on the
    // right.
    matrix Augment(matrix &right);

    // Returns a part of this matrix. Data is copied.
    matrix SubMatrix(int rmin, int cmin, int rmax, int cmax);

    // IsSquare will return true if the matrix is square
    bool IsSquare();

    // SwapRows Exchanges two rows in the matrix.
    int SwapRows(int r1, int r2);

    // Invert returns the inverse of this matrix.
    // Returns ErrSingular when the matrix is singular and doesn't have an
    // inverse. The matrix must be square, otherwise ErrNotSquare is returned.
    matrix Invert();

    //  Gaussian elimination (also known as row reduction)
    int gaussianElimination();

    std::vector<row_type> data;
    int rows{0}, cols{0};

    inline byte &at(int row, int col) { return (*(data[row]))[col]; }

    inline bool empty() { return (rows == 0 || cols == 0); }
};

#endif // KCP_MATRIX_H
