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

/* Structure of file
   -----------------
   fileIndex
   leaf
   parent
   nextLeaf
   previousLeaf
   keySize
   key1
   key2
   ...
   keyn
   child1
   child2
   ...
   child (n+1)
   ------------------
   */


// Configuration parameters
#define CONFIG_FILE "./bplustree.config"

// Constants
#define PREFIX "leaves/leaf_"
#define DEFAULT_LOCATION -1
// #define DEBUG

#include <iostream>
#include <math.h>
#include <fstream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <queue>
#include <vector>
#include <limits>
#include <algorithm>

using namespace std;

namespace BPlusTree {
    // A generic compare function for pairs of numbers
    template<typename T>
        class compare {
            public:
                bool operator() (pair<T, double> T1, pair<T, double> T2) {
                    return T1.second > T2.second;
                }
        };

    class Node {
        private:
            static long fileCount;              // Count of all files

        public:
            static long lowerBound;
            static long upperBound;
            static long pageSize;

        private:
            long fileIndex;                     // Name of file to store contents
            bool leaf;                          // Type of leaf

        public:
            long parentIndex;
            long nextLeafIndex;
            long previousLeafIndex;
            double keyType;                     // Dummy to indicate container base
            vector<double> keys;
            vector<long> childIndices;          // FileIndices of the children

            Node *parent;
            Node *nextLeaf;
            Node *previousLeaf;
            vector<Node *> children;

        public:
            // Basic initialization
            Node();

            // Given a fileIndex, read it
            Node(long _fileIndex);

            // Check if leaf
            bool isLeaf() { return leaf; }

            // Get the file name
            string getFileName() { return PREFIX + to_string(fileIndex); }

            // set to internalNode
            void setToInternalNode() { leaf = false; }

            // Return the size of keys
            long size() { return keys.size(); }

            // Initialize the for the tree
            static void initialize();

            // Return the position of a key in keys
            long getKeyPosition(double key);

            // Commit node to disk
            void commitToDisk();

            // Read from the disk into memory
            void readFromDisk();

            // Print node information
            void printNode();

            // Serialize the subtree
            void serialize();

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
    long Node::lowerBound = 0;
    long Node::upperBound = 0;
    long Node::pageSize = 0;
    long Node::fileCount = 0;

    Node *bRoot = nullptr;

    Node::Node() {
        // Initially the parent is NULL
        parent = nullptr;
        nextLeaf = nullptr;
        previousLeaf = nullptr;

        // Initially all the fileNames are DEFAULT_LOCATION
        parentIndex = DEFAULT_LOCATION;
        nextLeafIndex = DEFAULT_LOCATION;
        previousLeafIndex = DEFAULT_LOCATION;

        // Initially every node is a leaf
        leaf = true;

        // Exit if the lowerBoundKey is not defined
        if (lowerBound == 0) {
            cout << "LowerKeyBound not defined";
            exit(1);
        }

        // LeafNode properties
        fileIndex = fileCount++;
    }

    Node::Node(long _fileIndex) {
        // Reset the fileCount
        fileCount = _fileIndex + 10000;

        // Exit if the lowerBoundKey is not defined
        if (lowerBound == 0) {
            cout << "LowerKeyBound not defined";
            exit(1);
        }

        // Load the current node from disk
        fileIndex = _fileIndex;
        readFromDisk();
    }

    void Node::initialize() {
        // Read in the pageSize from the configuration file
        ifstream configFile;
        configFile.open(CONFIG_FILE);
        configFile >> pageSize;

        // Save some place in the file for the header
        long headerSize = sizeof(fileIndex)
            + sizeof(leaf)
            + sizeof(parentIndex)
            + sizeof(nextLeafIndex)
            + sizeof(previousLeafIndex);
        pageSize = pageSize - headerSize;

        // Compute parameters
        long nodeSize = sizeof(fileIndex);
        long keySize = sizeof(keyType);
        lowerBound = floor((pageSize - nodeSize) / (2 * (keySize + nodeSize)));

        // TODO : Change this back to default
        lowerBound = 5;
        upperBound = 2 * lowerBound;
        pageSize = pageSize + headerSize;
    }

    long Node::getKeyPosition(double key) {
        // If keys are empty, return
        if (keys.size() == 0) {
            return 0;
        }

        // Find the original position of the key
        if (key <= keys.front()) {
            return 0;
        }

        for (long i = 1; i < (long)keys.size(); ++i) {
            if (keys[i -1] < key && key <= keys[i]) {
                return i;
            }
        }

        return keys.size();
    }

    void Node::commitToDisk() {
        // Create a character buffer which will be written to disk
        long location = 0;
        char buffer[pageSize];

        // Store the fileIndex
        memcpy(buffer + location, &fileIndex, sizeof(fileIndex));
        location += sizeof(fileIndex);

        // Add the leaf to memory
        memcpy(buffer + location, &leaf, sizeof(leaf));
        location += sizeof(leaf);

        // Add parent to memory
        memcpy(buffer + location, &parentIndex, sizeof(parentIndex));
        location += sizeof(parentIndex);

        // Add the previous leaf node
        memcpy(buffer + location, &previousLeafIndex, sizeof(nextLeafIndex));
        location += sizeof(nextLeafIndex);

        // Add the next leaf node
        memcpy(buffer + location, &nextLeafIndex, sizeof(nextLeafIndex));
        location += sizeof(nextLeafIndex);

        // Store the number of keys
        long numKeys = keys.size();
        memcpy(buffer + location, &numKeys, sizeof(numKeys));
        location += sizeof(numKeys);

        // Add the keys to memory
        for (auto key : keys) {
            memcpy(buffer + location, &key, sizeof(key));
            location += sizeof(key);
        }

        // Add the child pointers to memory
        if (!leaf) {
            // for (auto childIndex : childIndices) {
                // memcpy(buffer + location, &childIndex, sizeof(childIndex));
                // location += sizeof(childIndex);
            // }
        }

        // Create a binary file and write to memory
        ofstream leafFile;
        leafFile.open(getFileName(), ios::binary|ios::out);
        leafFile.write(buffer, pageSize);
        leafFile.close();
    }

    void Node::readFromDisk() {
        // Create a character buffer which will be written to disk
        long location = 0;
        char buffer[pageSize];

        // Open the binary file ane read into memory
        ifstream leafFile;
        leafFile.open(getFileName(), ios::binary|ios::in);
        leafFile.read(buffer, pageSize);
        leafFile.close();

        // Retrieve the fileIndex
        memcpy((char *) &fileIndex, buffer + location, sizeof(fileIndex));
        location += sizeof(fileIndex);

        // Retreive the type of node
        memcpy((char *) &leaf, buffer + location, sizeof(leaf));
        location += sizeof(leaf);

        // Retrieve the parentIndex
        memcpy((char *) &parentIndex, buffer + location, sizeof(parentIndex));
        location += sizeof(parentIndex);

        // Retrieve the previousLeafIndex
        memcpy((char *) &previousLeafIndex, buffer + location, sizeof(previousLeafIndex));
        location += sizeof(previousLeafIndex);

        // Retrieve the nextLeafIndex
        memcpy((char *) &nextLeafIndex, buffer + location, sizeof(nextLeafIndex));
        location += sizeof(nextLeafIndex);

        // Retrieve the number of keys
        long numKeys;
        memcpy((char *) &numKeys, buffer + location, sizeof(numKeys));
        location += sizeof(numKeys);

        // Retrieve the keys
        keys.clear();
        double key;
        for (long i = 0; i < numKeys; ++i) {
            memcpy((char *) &key, buffer + location, sizeof(key));
            location += sizeof(key);
            keys.push_back(key);
        }

        // Retrieve childPointers
        if (!leaf) {
            childIndices.clear();
            // long childIndex;
            // for (long i = 0; i < numKeys + 1; ++i) {
                // memcpy((char *) &childIndex, buffer + location, sizeof(childIndex));
                // location += sizeof(childIndex);
            // childIndices.push_back(childIndex);
            // }
        }
    }

    void Node::printNode() {
        cout << fileIndex << endl;
        cout << leaf << endl;
        cout << parentIndex << endl;
        cout << previousLeafIndex << endl;
        cout << nextLeafIndex << endl;
        cout << keys.size() << endl;

        // Print keys
        for (auto key : keys) {
            cout << key << endl;
        }

        // Print the name of the child
        for (auto childIndex : childIndices) {
            cout << childIndex << endl;
        }
    }

    void Node::serialize() {
        // Prettify
        cout << endl << endl;

        queue< pair<Node *, char> > previousLevel;
        previousLevel.push(make_pair(this, 'N'));

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


    void Node::insertObject(double key) {
        long position = getKeyPosition(key);

        // insert the new key to keys
        keys.insert(keys.begin() + position, key);

        // Commit the new node back into memory
        commitToDisk();
    }

    void Node::insertNode(double key, Node *leftChild, Node *rightChild) {
        long position = getKeyPosition(key);

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
        if ((long)keys.size() > upperBound) {
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
        commitToDisk();

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
            bRoot->serialize();
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
            root->readFromDisk();

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
            long position = root->getKeyPosition(searchKey);

            // Recurse into the tree
            pointSearch(root->children[position], searchKey);
        }
    }

    // window search
    void windowSearch(Node *root, double lowerLimit, double upperLimit) {
        // If the root is a leaf, we can directly search
        if (root->isLeaf()) {
            root->readFromDisk();

            // Print all nodes in the current leaf which satisfy the criteria
            for (auto key : root->keys) {
                if (key >=lowerLimit && key <= upperLimit) {
                    cout << key << endl;
                }
            }

            // Check nextleaf for the condition
            if (root->nextLeaf != nullptr
                    && root->nextLeaf->keys.front() >= lowerLimit
                    && root->nextLeaf->keys.front() <= upperLimit) {
                windowSearch(root->nextLeaf, lowerLimit, upperLimit);
            }
        } else {
            // We traverse the tree
            int position = root->getKeyPosition(lowerLimit);

            // Recurse into the tree
            windowSearch(root->children[position], lowerLimit, upperLimit);
        }

    }

    //rangesearch
    void rangeSearch(Node *root, double center, double range) {
        double upperBound = center + range;
        double lowerBound = (center - range >= 0) ? center - range : 0;

        // Call windowSearch internally
        windowSearch(root, lowerBound, upperBound);
    }

    // kNN query
    void kNNsearch(Node *root, double center, long k) {
        // A priority_queue to keep the elements ordered
        priority_queue< pair<Node*, double>, vector< pair<Node*, double> >, compare<Node *> > q;

        // Insert the root into the queue
        q.push(make_pair(root, 0));

        // Vector to store the answers
        vector<double> answers;

        while(!q.empty() && (long)answers.size() <= k) {
            Node *currentNode = q.top().first;
            q.pop();

            if (currentNode->isLeaf()) {
                // All the keys for a leaf node are answers
                for (auto key : currentNode->keys) {
                    answers.push_back(key);
                }
            } else {
                if (currentNode->keys.size() >= 1) {
                    double distance;
                    vector<double> distances;

                    // For the first key
                    if (center < currentNode->keys.front()) {
                        distances.push_back(0);
                    } else {
                        distances.push_back(abs(center - currentNode->keys.front()));
                    }

                    // For middle keys
                    for (long i = 1; i < (long)currentNode->keys.size(); ++i) {
                        if (center < currentNode->keys[i - 1]) {
                            distance = abs(center - currentNode->keys[i - 1]);
                        } else if (center > currentNode->keys[i]) {
                            distance = abs(center - currentNode->keys[i]);
                        } else {
                            distance = 0;
                        }

                        distances.push_back(distance);
                    }

                    // For the last key
                    if (currentNode->keys.size() > 1) {
                        if (center > currentNode->keys.back()) {
                            distances.push_back(0);
                        } else {
                            distances.push_back(abs(center - currentNode->keys.back()));
                        }
                    }

                    // Now we push the children along with the computed distances
                    for (long i = 0; i < (long)distances.size(); ++i) {
                        q.push(make_pair(currentNode->children[i], distances[i]));
                    }
                }
            }
        }

        // Sort the obtained answers
        sort(answers.begin(), answers.end(), [&](double T1, double T2) { return (abs(T1 - center) < abs(T2 - center)); });

        // Print the answers
        for (long i = 0; i < k; ++i) {
            cout << answers[i] << endl;
        }
    }
}

using namespace BPlusTree;

int main() {
    // Initialize the BPlusTree module
    Node::initialize();

    // Create a new tree
    bRoot = new Node();

    for (long i = 0; i < 40; ++i) {
        insert(bRoot, 2 * i);
    }

    for (long i = 40; i > 0; --i) {
        insert(bRoot, 3 * i);
    }

    bRoot->serialize();
    // windowSearch(bRoot, 0 , 10);
    // rangeSearch(bRoot, 0 , 5);
    // kNNsearch(bRoot, 2, 3);

    // Clean up on exit
    // system("rm leaves/* && touch leaves/DUMMY");

    return 0;
}
