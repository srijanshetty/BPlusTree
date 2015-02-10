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

/* Conventions
   1. Caller ensures the Node is loaded into memory.
   2. If a function modifies the Node, it saves it back to disk
   */



// Configuration parameters
#define CONFIG_FILE "./bplustree.config"

// Constants
#define PREFIX "leaves/leaf_"
#define DEFAULT_LOCATION -1
#define DEBUG

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
            void insertNode(double key, long leftChildIndex, long rightChildIndex);

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
        fileIndex = ++fileCount;
    }

    Node::Node(long _fileIndex) {
        // Reset the fileCount
        fileCount = fileCount + _fileIndex + 10000;

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
        lowerBound = 2;
        upperBound = 2 * lowerBound;
        pageSize = pageSize + headerSize;
    }

    long Node::getKeyPosition(double key) {
        // If keys are empty, return
        if (keys.size() == 0 || key <= keys.front()) {
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
            for (auto childIndex : childIndices) {
                memcpy(buffer + location, &childIndex, sizeof(childIndex));
                location += sizeof(childIndex);
            }
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
            long childIndex;
            for (long i = 0; i < numKeys + 1; ++i) {
                memcpy((char *) &childIndex, buffer + location, sizeof(childIndex));
                location += sizeof(childIndex);
            childIndices.push_back(childIndex);
            }
        }
    }

    void Node::printNode() {
        cout << endl << endl;

        cout << "File : " << fileIndex << endl;
        cout << "IsLeaf : " << leaf << endl;
        cout << "Parent : " << parentIndex << endl;
        cout << "PreviousLeaf : " << previousLeafIndex << endl;
        cout << "NextLeaf : " << nextLeafIndex << endl;

        // Print keys
        cout << "Keys : ";
        for (auto key : keys) {
            cout << key << " ";
        }
        cout << endl;

        // Print the name of the child
        cout << "Children : ";
        for (auto childIndex : childIndices) {
            cout << childIndex << " ";
        }
        cout << endl;
    }

    void Node::insertObject(double key) {
        long position = getKeyPosition(key);

        // insert the new key to keys
        keys.insert(keys.begin() + position, key);

        // Commit the new node back into memory
        commitToDisk();
    }

    void Node::serialize() {
        // Prettify
        cout << endl << endl;

        queue< pair<long, char> > previousLevel;
        previousLevel.push(make_pair(fileIndex, 'N'));

        long currentIndex;
        Node *iterator;
        char type;
        while (!previousLevel.empty()) {
            queue< pair<long, char> > nextLevel;

            while (!previousLevel.empty()) {
                // Get the front and pop
                currentIndex = previousLevel.front().first;
                iterator = new Node(currentIndex);
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
                for (auto childIndex : iterator->childIndices) {
                    nextLevel.push(make_pair(childIndex, 'N'));

                    // Insert a marker to indicate end of child
                    nextLevel.push(make_pair(DEFAULT_LOCATION, '|'));
                }

                // Delete allocated memory
                delete iterator;
            }

            // Seperate different levels
            cout << endl << endl;
            previousLevel = nextLevel;
        }
    }

    void Node::insertNode(double key, long leftChildIndex, long rightChildIndex) {
        // insert the new key to keys
        long position = getKeyPosition(key);
        keys.insert(keys.begin() + position, key);

        // insert the newChild
        childIndices.insert(childIndices.begin() + position + 1, rightChildIndex);

        // commit changes to disk
        commitToDisk();

#ifdef DEBUG
        cout << endl;
        cout << "InsertNode : " << endl;
        cout << "Base Node : ";
        for (auto key : keys) {
            cout << key << " ";
        }
        cout << endl;

        // Print them out
        Node *leftChild = new Node(leftChildIndex);
        cout << "LeftNode : ";
        for (auto key : leftChild->keys) {
            cout << key << " ";
        }
        cout << endl;
        delete leftChild;

        Node *rightChild = new Node(rightChildIndex);
        cout << "RightNode : ";
        for (auto key : rightChild->keys) {
            cout << key << " ";
        }
        cout << endl;
        delete rightChild;
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

        // Fix the keys of the new node
        double startPoint = *(keys.begin() + lowerBound);
        for (auto key = keys.begin() + lowerBound + 1; key != keys.end(); ++key) {
            surrogateInternalNode->keys.push_back(*key);
        }

        // Resize the keys of the current node
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

        // Partition children for the surrogateInternalNode
        for (auto childIndex = childIndices.begin() + lowerBound + 1; childIndex != childIndices.end(); ++childIndex) {
            surrogateInternalNode->childIndices.push_back(*childIndex);

            // Assign parent to the children nodes
            Node *tempChildNode = new Node(*childIndex);
            tempChildNode->parentIndex = surrogateInternalNode->fileIndex;
            tempChildNode->commitToDisk();
            delete tempChildNode;
        }

        // Fix children for the current node
        childIndices.resize(lowerBound + 1);

        // If the current node is not a root node
        if (parentIndex != DEFAULT_LOCATION) {
            // Assign parents
            surrogateInternalNode->parentIndex = parentIndex;
            surrogateInternalNode->commitToDisk();
            commitToDisk();

            // Now we push up the splitting one level
            Node *tempParent = new Node(parentIndex);
            tempParent->insertNode(startPoint, fileIndex, surrogateInternalNode->fileIndex);
            delete tempParent;
        } else {
            // Create a new parent node
            Node *newParent = new Node();
            newParent->setToInternalNode();

            // Assign parents
            surrogateInternalNode->parentIndex = newParent->fileIndex;
            parentIndex = newParent->fileIndex;

            // Insert the key into the keys
            newParent->keys.push_back(startPoint);

            // Insert the children
            newParent->childIndices.push_back(fileIndex);
            newParent->childIndices.push_back(surrogateInternalNode->fileIndex);

            // Commit changes to disk
            newParent->commitToDisk();
            commitToDisk();
            surrogateInternalNode->commitToDisk();

            // Clean up the previous root node
            delete bRoot;

            // Reset the root node
            bRoot = newParent;
        }

        // Clean the surrogateInternalNode
        delete surrogateInternalNode;
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

        // Resize the current leaf node and commit the node to disk
        keys.resize(lowerBound);

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
        long tempLeafIndex = nextLeafIndex;
        nextLeafIndex = surrogateLeafNode->fileIndex;
        surrogateLeafNode->nextLeafIndex = tempLeafIndex;

        // If the tempLeafIndex is not null we have to load it and set its
        // previous index
        if (tempLeafIndex != DEFAULT_LOCATION) {
            Node *tempLeaf = new Node(tempLeafIndex);
            tempLeaf->previousLeafIndex = surrogateLeafNode->fileIndex;
            tempLeaf->commitToDisk();
            delete tempLeaf;
        }

        surrogateLeafNode->previousLeafIndex = fileIndex;

        // Consider the case when the current node is not a root
        if (parentIndex != DEFAULT_LOCATION) {
            // Assign parents
            surrogateLeafNode->parentIndex = parentIndex;
            surrogateLeafNode->commitToDisk();
            commitToDisk();

            // Now we push up the splitting one level
            Node *tempParent = new Node(parentIndex);
            tempParent->insertNode(surrogateLeafNode->keys.front(), fileIndex, surrogateLeafNode->fileIndex);
            delete tempParent;
        } else {
            // Create a new parent node
            Node *newParent = new Node();
            newParent->setToInternalNode();

            // Assign parents
            surrogateLeafNode->parentIndex = newParent->fileIndex;
            parentIndex = newParent->fileIndex;

            // Insert the key into the keys
            newParent->keys.push_back(surrogateLeafNode->keys.front());

            // Insert the children
            newParent->childIndices.push_back(this->fileIndex);
            newParent->childIndices.push_back(surrogateLeafNode->fileIndex);

            // Commit to disk
            newParent->commitToDisk();
            surrogateLeafNode->commitToDisk();
            commitToDisk();

            // Clean up the root node
            delete bRoot;

            // Reset the root node
            bRoot = newParent;
        }

        // Clean up surrogateNode
        delete surrogateLeafNode;
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
            long position = root->getKeyPosition(key);

            // Load the node from disk
            Node *nextRoot = new Node(root->childIndices[position]);

            // Recurse into the node
            insert(nextRoot, key);

            // Clean up
            delete nextRoot;
        }
    }

    // Point search in a BPlusTree
    void pointSearch(Node *root, double searchKey) {
        // If the root is a leaf, we can directly search
        if (root->isLeaf()) {
            // Print all nodes in the current leaf
            for (auto key : root->keys) {
                if (key == searchKey) {
                    cout << key << endl;
                }
            }

            // Check nextleaf for same node
            if (root->nextLeafIndex != DEFAULT_LOCATION) {
                // Load up the nextLeaf from disk
                Node *tempLeaf = new Node(root->nextLeafIndex);

                // Check in the nextLeaf and delegate
                if (tempLeaf->keys.front() == searchKey) {
                    pointSearch(tempLeaf, searchKey);
                }

                delete tempLeaf;
            }
        } else {
            // We traverse the tree
            long position = root->getKeyPosition(searchKey);

            // Load the node from disk
            Node *nextRoot = new Node(root->childIndices[position]);

            // Recurse into the node
            pointSearch(nextRoot, searchKey);

            // Clean up
            delete nextRoot;
        }
    }

    // window search
//     void windowSearch(Node *root, double lowerLimit, double upperLimit) {
//         // If the root is a leaf, we can directly search
//         if (root->isLeaf()) {
//             root->readFromDisk();
//
//             // Print all nodes in the current leaf which satisfy the criteria
//             for (auto key : root->keys) {
//                 if (key >=lowerLimit && key <= upperLimit) {
//                     cout << key << endl;
//                 }
//             }
//
//             // Check nextleaf for the condition
//             if (root->nextLeaf != nullptr
//                     && root->nextLeaf->keys.front() >= lowerLimit
//                     && root->nextLeaf->keys.front() <= upperLimit) {
//                 windowSearch(root->nextLeaf, lowerLimit, upperLimit);
//             }
//         } else {
//             // We traverse the tree
//             int position = root->getKeyPosition(lowerLimit);
//
//             // Recurse into the tree
//             windowSearch(root->children[position], lowerLimit, upperLimit);
//         }
//
//     }
//
//     //rangesearch
//     void rangeSearch(Node *root, double center, double range) {
//         double upperBound = center + range;
//         double lowerBound = (center - range >= 0) ? center - range : 0;
//
//         // Call windowSearch internally
//         windowSearch(root, lowerBound, upperBound);
//     }
//
//     // kNN query
//     void kNNsearch(Node *root, double center, long k) {
//         // A priority_queue to keep the elements ordered
//         priority_queue< pair<Node*, double>, vector< pair<Node*, double> >, compare<Node *> > q;
//
//         // Insert the root into the queue
//         q.push(make_pair(root, 0));
//
//         // Vector to store the answers
//         vector<double> answers;
//
//         while(!q.empty() && (long)answers.size() <= k) {
//             Node *currentNode = q.top().first;
//             q.pop();
//
//             if (currentNode->isLeaf()) {
//                 // All the keys for a leaf node are answers
//                 for (auto key : currentNode->keys) {
//                     answers.push_back(key);
//                 }
//             } else {
//                 if (currentNode->keys.size() >= 1) {
//                     double distance;
//                     vector<double> distances;
//
//                     // For the first key
//                     if (center < currentNode->keys.front()) {
//                         distances.push_back(0);
//                     } else {
//                         distances.push_back(abs(center - currentNode->keys.front()));
//                     }
//
//                     // For middle keys
//                     for (long i = 1; i < (long)currentNode->keys.size(); ++i) {
//                         if (center < currentNode->keys[i - 1]) {
//                             distance = abs(center - currentNode->keys[i - 1]);
//                         } else if (center > currentNode->keys[i]) {
//                             distance = abs(center - currentNode->keys[i]);
//                         } else {
//                             distance = 0;
//                         }
//
//                         distances.push_back(distance);
//                     }
//
//                     // For the last key
//                     if (currentNode->keys.size() > 1) {
//                         if (center > currentNode->keys.back()) {
//                             distances.push_back(0);
//                         } else {
//                             distances.push_back(abs(center - currentNode->keys.back()));
//                         }
//                     }
//
//                     // Now we push the children along with the computed distances
//                     for (long i = 0; i < (long)distances.size(); ++i) {
//                         q.push(make_pair(currentNode->children[i], distances[i]));
//                     }
//                 }
//             }
//         }
//
//         // Sort the obtained answers
//         sort(answers.begin(), answers.end(), [&](double T1, double T2) { return (abs(T1 - center) < abs(T2 - center)); });
//
//         // Print the answers
//         for (long i = 0; i < k; ++i) {
//             cout << answers[i] << endl;
//         }
    // }
}

using namespace BPlusTree;

int main() {
    // Initialize the BPlusTree module
    Node::initialize();

    // Create a new tree
    bRoot = new Node();

    for (long i = 0; i < 40; ++i) {
        cout << "Insert" << 2 * i << endl;
        insert(bRoot, 2 * i);
        bRoot->readFromDisk();
    }

    bRoot->serialize();
    // windowSearch(bRoot, 0 , 10);
    // rangeSearch(bRoot, 0 , 5);
    // kNNsearch(bRoot, 2, 3);

    // Clean up on exit
    // system("rm leaves/* && touch leaves/DUMMY");

    delete bRoot;

    return 0;
}
