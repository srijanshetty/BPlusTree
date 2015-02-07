/*
 * Copyright (c) 2015 Srijan R Shetty <srijan.shetty+code@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define PREFIX "leaves/leaf_"

#include <iostream>
#include <math.h>
#include <fstream>
#include <string>

namespace BPlusTree {

    // Properties shared by both internal nodes and leaf nodes
    class BaseNode {
        protected:
        bool isLeaf;
        int size;

        // Static data
        static int lowerKeyBound;
        static int upperKeyBound;
        static int leafCount;

        public:
        BaseNode() {
            size = 0;
            isLeaf = true;
        }

        // Function to compute number of keys
        static void computeNumberOfKeys(int pageSize) {
            // Compute the parameters for the Bplus tree
            int nodeSize = sizeof(new BaseNode());
            int keySize = sizeof(double);
            lowerKeyBound = floor((pageSize - nodeSize) / (2 * (keySize + nodeSize)));
            upperKeyBound = 2 * lowerKeyBound;
        }
    };

    // Initialize static variables
    int BaseNode::lowerKeyBound = 0;
    int BaseNode::upperKeyBound = 0;
    int BaseNode::leafCount = 0;

    class InternalNode: public BaseNode {
        double *keys;
        InternalNode *children;

        public:
            InternalNode() : BaseNode() {
                isLeaf = false;
                keys = new double[upperKeyBound];
                children = new InternalNode[upperKeyBound + 1];
            }
    };

    class LeafNode: public BaseNode {
        std::string leafFileName;

        public:
            LeafNode() : BaseNode() {
                leafFileName = PREFIX + std::to_string(leafCount);

                // Create a new file
                std::ofstream leafFile;
                leafFile.open(leafFileName, std::ios::binary|std::ios::out);
                leafFile << "yo";
                std::cout << leafFileName;
            }
    };
}

