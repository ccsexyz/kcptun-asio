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
// Created by 理 傅 on 2017/1/1.
//

#ifndef KCP_INVERSION_TREE_H
#define KCP_INVERSION_TREE_H

#include "matrix.h"
#include <vector>

struct inversionNode {
    struct matrix m_matrix;
    std::vector<std::shared_ptr<inversionNode>> m_children;
    struct matrix getInvertedMatrix(std::vector<int> &invalidIndices,
                                    int parent);

    void insertInvertedMatrix(std::vector<int> &invalidIndices,
                              struct matrix &matrix, int shards, int parent);
};

class inversionTree {
public:
    // newInversionTree initializes a tree for storing inverted matrices.
    // Note that the m_root node is the identity matrix as it implies
    // there were no errors with the original data.
    static inversionTree newInversionTree(int dataShards, int parityShards);

    // GetInvertedMatrix returns the cached inverted matrix or nil if it
    // is not found in the tree keyed on the indices of invalid rows.
    matrix GetInvertedMatrix(std::vector<int> &invalidIndices);

    // InsertInvertedMatrix inserts a new inverted matrix into the tree
    // keyed by the indices of invalid rows.  The total number of shards
    // is required for creating the proper length lists of child nodes for
    // each node.
    int InsertInvertedMatrix(std::vector<int> &invalidIndices,
                             struct matrix &matrix, int shards);

private:
    inversionNode m_root;
};

#endif // KCP_INVERSION_TREE_H
