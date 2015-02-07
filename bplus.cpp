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
#include <stdlib.h>
#include <queue>
#include <vector>
#include <climits>

using namespace std;

namespace BPlusTree {

    class Node {
        bool isLeaf;
        vector<pair<double, Node> > keys;

        // LeafNode properties

        // Static data
        static int lowerKeyBound;
        static int upperKeyBound;
        static int leafCount;

        public:
        string leafFileName;
        Node() {
            // Initially every node is a leaf
            isLeaf = true;

            // Exit if the lowerBoundKey is not defined
            if (lowerKeyBound == 0) {
                cout << "LowerKeyBound not defined";
                exit(1);
            }

            // LeafNode properties
            leafFileName = PREFIX + to_string(leafCount++);
        }

        Node(double key) {
            // Call the main constructor
            Node();

            // Insert the key into the leaf node
            insertIntoLeaf(key);
        }

        // Function to compute number of keys
        static void computeNumberOfKeys(int pageSize) {
            // Compute the parameters for the Bplus tree
            int nodeSize = sizeof(new Node());
            int keySize = sizeof(double);
            lowerKeyBound = floor((pageSize - nodeSize) / (2 * (keySize + nodeSize)));
            upperKeyBound = 2 * lowerKeyBound;
        }

        // Insert a key into keys
        void insertIntoLeaf(double key) {
            // Write to memory in case of leaf
            if (isLeaf) {
                ofstream leafFile;
                leafFile.open(leafFileName, ios::binary|ios::app);
                leafFile.write((char *) &key, sizeof(key));
                leafFile.close();

                // keys.push(make_pair(key, NULL));
            }
        }

        // Read all the keys from file into memory
        void getKeys() {
            // Do nothing for internal node
            if (isLeaf) {
                ifstream leafFile;
                leafFile.open(leafFileName, ios::binary|ios::in);

                // Fix reading last line multiple times
                while (!leafFile.eof()) {
                    double y;
                    leafFile.read((char *) &y, sizeof(double));
                    cout << y << endl;
                }
            }
        }
    };

    // Initialize static variables
    int Node::lowerKeyBound = 0;
    int Node::upperKeyBound = 0;
    int Node::leafCount = 0;
}

