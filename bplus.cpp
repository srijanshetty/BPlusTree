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
#define DEBUG

#include <iostream>
#include <math.h>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <queue>
#include <vector>
#include <climits>
#include <algorithm>

using namespace std;

namespace BPlusTree {
    class Node {
        // Static data
        static int leafCount;

        // Type of leaf
        bool leaf;

        // LeafNode properties
        string leafFileName;

        public:

        // Bounds
        static int lowerBound;
        static int upperBound;

        // Keys and their associated children
        vector<double> keys;
        vector<Node *> children;

        // The parent of the Node
        Node *parent;

        // The next leaf
        Node *nextLeaf;
        Node *previousLeaf;

        // Basic initialization
        Node();

        // Check if leaf
        bool isLeaf() { return leaf; }

        // set to internalNode
        void setToInternalNode() { leaf = false; }

        // Return the size of keys
        int size() { return keys.size(); }

        // Initialize the for the tree
        static void initialize(int pageSize);

        // Return the position of a key in keys
        int getKeyPosition(double key);

        // Read all the keys from disk to memory
        void getKeysFromDisk();

        // Save keys to disk
        void saveKeysToDisk();

        // Insert object into disk
        void insertObject(double key);

        // Insert an internal node into the tree
        void insertNode(double key, Node *leftChild, Node *rightChild);

        // Split the current Leaf Node
        void splitLeaf();

        // Split the current internal Node
        void splitInternal();
    };

    // Initialize static variables
    int Node::lowerBound = 0;
    int Node::upperBound = 0;
    int Node::leafCount = 0;

    Node *bRoot = nullptr;

    Node::Node() {
        // Initially the parent is NULL
        parent = nullptr;
        nextLeaf = nullptr;
        previousLeaf = nullptr;

        // Initially every node is a leaf
        leaf = true;

        // Exit if the lowerBoundKey is not defined
        if (lowerBound == 0) {
            cout << "LowerKeyBound not defined";
            exit(1);
        }

        // LeafNode properties
        leafFileName = PREFIX + to_string(leafCount++);
    }

    void Node::initialize(int pageSize) {
        int nodeSize = sizeof(new Node());
        int keySize = sizeof(double);
        lowerBound = floor((pageSize - nodeSize) / (2 * (keySize + nodeSize)));

        lowerBound = 2;
        upperBound = 2 * lowerBound;
    }

    int Node::getKeyPosition(double key) {
        // If keys are empty, return
        if (keys.size() == 0) {
            return 0;
        }

        // Find the original position of the key
        if (key <= keys.front()) {
            return 0;
        }

        for (int i = 1; i < (int)keys.size(); ++i) {
            if (keys[i -1] < key && key <= keys[i]) {
                return i;
            }
        }

        return keys.size();
    }

    void Node::saveKeysToDisk() {
        // Create a binary file
        ofstream leafFile;
        leafFile.open(leafFileName, ios::binary|ios::out);

        // Read the key and enter it into keys
        for (auto key : keys) {
            leafFile.write((char *) &key, sizeof(key));
        }

        // Close the file
        leafFile.close();
    }

    void Node::getKeysFromDisk() {
        // Clear the exisitng keys
        keys.clear();

        // Create a binary file
        ifstream leafFile;
        leafFile.open(leafFileName, ios::binary|ios::in);

        // Read the key and enter it into keys
        double key;
        while (!leafFile.eof()) {
            leafFile.read((char *) &key, sizeof(key));

            /* Common Error in reading files */
            if (!leafFile) break;

            keys.push_back(key);
        }

        // sort the keys
        sort(keys.begin(), keys.end());

        // Close the file
        leafFile.close();
    }

    void Node::insertObject(double key) {
        ofstream leafFile;
        leafFile.open(leafFileName, ios::binary|ios::app);
        leafFile.write((char *) &key, sizeof(key));
        leafFile.close();

        // Load keys to avoid stale keys
        getKeysFromDisk();
    }

    void Node::insertNode(double key, Node *leftChild, Node *rightChild) {
        int position = getKeyPosition(key);

        // insert the new key to keys
        keys.insert(keys.begin() + position, key);

        // insert the newChild
        children.insert(children.begin() + position + 1, rightChild);

#ifdef DEBUG
        cout << endl;
        cout << "InsertNode : " << endl;
        cout << "Base Node : ";
        for (auto key : keys) {
            cout << key << " ";
        }
        cout << endl;

        // Print them out
        cout << "LeftNode : ";
        for (auto key : leftChild->keys) {
            cout << key << " ";
        }
        cout << endl;

        cout << "RightNode : ";
        for (auto key : rightChild->keys) {
            cout << key << " ";
        }
        cout << endl;
#endif

        // If this overflows, we move again upward
        if ((int)keys.size() > upperBound) {
            splitInternal();
        }
    }

    void Node::splitInternal() {
#ifdef DEBUG
        cout << endl;
        cout << "SplitInternal : " << endl;
        cout << "Base Node : ";
        for (auto key : keys) {
            cout << key << " ";
        }
        cout << endl;
#endif

        // Create a surrogate internal node
        Node *surrogateInternalNode = new Node();
        surrogateInternalNode->setToInternalNode();

        // Fix up the keys
        double startPoint = *(keys.begin() + lowerBound);
        for (auto key = keys.begin() + lowerBound + 1; key != keys.end(); ++key) {
            surrogateInternalNode->keys.push_back(*key);
        }
        keys.resize(lowerBound);

#ifdef DEBUG
        // Print them out
        cout << "First InternalNode : ";
        for (auto key : keys) {
            cout << key << " ";
        }
        cout << endl;

        cout << "Second InternalNode : ";
        for (auto key : surrogateInternalNode->keys) {
            cout << key << " ";
        }
        cout << endl;

        cout << "Split At " << startPoint << endl;
#endif

        // Fix up the pointers
        for (auto child = children.begin() + lowerBound + 1; child != children.end(); ++child) {
            surrogateInternalNode->children.push_back(*child);
            (*child)->parent = surrogateInternalNode;
        }
        children.resize(lowerBound + 1);

        if (parent != nullptr) {
            // Assign parents
            surrogateInternalNode->parent = parent;

            // Now we push up the splitting one level
            parent->insertNode(startPoint, this, surrogateInternalNode);
        } else {
            // Create a new parent node
            Node *newParent = new Node();
            newParent->setToInternalNode();

            // Assign parents
            surrogateInternalNode->parent = newParent;
            parent = newParent;

            // Insert the key into the keys
            newParent->keys.push_back(startPoint);

            // Insert the children
            newParent->children.push_back(this);
            newParent->children.push_back(surrogateInternalNode);

            // Reset the root node
            bRoot = newParent;
        }
    }

    void Node::splitLeaf() {
#ifdef DEBUG
        cout << endl;
        cout << "SplitLeaf : " << endl;
        cout << "Base Node : ";

        for (auto key : keys) {
            cout << key << " ";
        }
        cout << endl;
#endif

        // Create a surrogate leaf node
        Node *surrogateLeafNode = new Node();
        for (auto key = keys.begin() + lowerBound; key != keys.end(); ++key) {
            surrogateLeafNode->insertObject(*key);
        }

        // Resize the current leaf node and save keys to disk
        keys.resize(lowerBound);
        saveKeysToDisk();

#ifdef DEBUG
        // Print them out
        cout << "First Leaf : ";
        for (auto key : keys) {
            cout << key << " ";
        }
        cout << endl;

        cout << "Second Leaf : ";
        for (auto key : surrogateLeafNode->keys) {
            cout << key << " ";
        }
        cout << endl;
#endif

        // Link up the leaves
        Node *tempLeaf = nextLeaf;
        nextLeaf = surrogateLeafNode;
        surrogateLeafNode->nextLeaf = tempLeaf;

        if (tempLeaf != nullptr) {
            tempLeaf->previousLeaf = surrogateLeafNode;
        }
        surrogateLeafNode->previousLeaf = this;

        if (parent != nullptr) {
            // Assign parents
            surrogateLeafNode->parent = parent;

            // Now we push up the splitting one level
            parent->insertNode(surrogateLeafNode->keys.front(), this, surrogateLeafNode);
        } else {
            // Create a new parent node
            Node *newParent = new Node();
            newParent->setToInternalNode();

            // Assign parents
            surrogateLeafNode->parent = newParent;
            parent = newParent;

            // Insert the key into the keys
            newParent->keys.push_back(surrogateLeafNode->keys.front());

            // Insert the children
            newParent->children.push_back(this);
            newParent->children.push_back(surrogateLeafNode);

            // Reset the root node
            bRoot = newParent;
        }
    }

    // Initialize the BPlusTree
    void initialize(int pageSize) {
        // Compute the number of keys in each node
        Node::initialize(pageSize);

        // Intialize the root
        bRoot = new Node();
    }

    // Serialize the tree
    void serialize(Node *root) {
        // Prettify
        cout << endl << endl;

        queue< pair<Node *, char> > previousLevel;
        previousLevel.push(make_pair(root, 'N'));

        Node *iterator;
        char type;
        while (!previousLevel.empty()) {
            queue< pair<Node *, char> > nextLevel;

            while (!previousLevel.empty()) {
                // Get the front and pop
                iterator = previousLevel.front().first;
                type = previousLevel.front().second;
                previousLevel.pop();

                // If it a seperator, print and move ahead
                if (type == '|') {
                    cout << "|| ";
                    continue;
                }

                // Print all the keys
                for (auto key : iterator->keys) {
                    cout << key << " ";
                }

                // Enqueue all the children
                for (auto child : iterator->children) {
                    nextLevel.push(make_pair(child, 'N'));

                    // Insert a marker to indicate end of child
                    nextLevel.push(make_pair(nullptr, '|'));
                }
            }

            // Seperate different levels
            cout << endl << endl;
            previousLevel = nextLevel;
        }
    }

    // Insert a key into the BPlusTree
    void insert(Node *root, double key) {
        // If the root is a leaf, we can directly insert
        if (root->isLeaf()) {
            // Insert object
            root->insertObject(key);

            // Split if required
            if (root->size() > root->upperBound) {
                root->splitLeaf();
            }

#ifdef DEBUG
            // Serialize
            serialize(bRoot);
#endif
        } else {
            // We traverse the tree
            int position = root->getKeyPosition(key);

            // Recurse into the tree
            insert(root->children[position], key);
        }
    }

    // Point search in a BPlusTree
    void pointSearch(Node *root, double searchKey) {
        // If the root is a leaf, we can directly search
        if (root->isLeaf()) {
            root->getKeysFromDisk();

            // Print all nodes in the current leaf
            for (auto key : root->keys) {
                if (key == searchKey) {
                    cout << key << endl;
                }
            }

            // Check nextleaf for same node
            if (root->nextLeaf != nullptr && root->nextLeaf->keys.front() == searchKey) {
                pointSearch(root->nextLeaf, searchKey);
            }
        } else {
            // We traverse the tree
            int position = root->getKeyPosition(searchKey);

            // Recurse into the tree
            pointSearch(root->children[position], searchKey);
        }
    }
}

using namespace BPlusTree;

int main() {
    // Open the configuration file
    ifstream configFile;
    configFile.open("./bplustree.config");

    // Read in the pageSize from the configuration file
    int pageSize = 0;
    configFile >> pageSize;

    // Initialize the BPlusTree module
    initialize(pageSize);

    for (int i = 0; i < 10; ++i) {
        insert(bRoot, 140);
        insert(bRoot, 240);
        insert(bRoot, 340);
    }

    pointSearch(bRoot, 140);
    pointSearch(bRoot, 240);

    // Clean up on exit
    system("rm leaves/* && touch leaves/DUMMY");

    return 0;
}
