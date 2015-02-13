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
#define SESSION_FILE "./.tree.session"

// Constants
#define TREE_PREFIX "leaves/leaf_"
#define OBJECT_FILE "objects/objectFile"
#define DEFAULT_LOCATION -1
// #define DEBUG_VERBOSE
// #define DEBUG_NORMAL

// Two modes of running the program, either time it or show output
#define OUTPUT
// #define TIME

#include <chrono>
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

    // Database objects
    class DBObject {
        private:
            double key;
            long fileIndex;
            string dataString;

        public:
            static long objectCount;

        public:
            DBObject(double _key, string _dataString) : key(_key), dataString(_dataString) {
                fileIndex = objectCount++;

                // Open a file and write the string to it
                ofstream ofile(OBJECT_FILE, ios::app);
                ofile << dataString << endl;
                ofile.close();
            }

            DBObject(double _key, long _fileIndex) : key(_key), fileIndex(_fileIndex) {
                // Open a file and read the dataString
                ifstream ifile(OBJECT_FILE);
                for (long i = 0; i < fileIndex + 1; ++i) {
                    getline(ifile, dataString);
                }
                ifile.close();
            }

            // Return the key of the object
            double getKey() { return key; }

            // Return the string
            string getDataString() { return dataString; }

            // Return the fileIndex
            long getFileIndex() { return fileIndex; }
    };

    long DBObject::objectCount = 0;

    class Node {
        public:
            static long fileCount;              // Count of all files
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
            vector<long> objectPointers;        // To store the object pointers

        public:
            // Basic initialization
            Node();

            // Given a fileIndex, read it
            Node(long _fileIndex);

            // Check if leaf
            bool isLeaf() { return leaf; }

            // Get the file name
            string getFileName() { return TREE_PREFIX + to_string(fileIndex); }

            // Get the fileIndex
            long getFileIndex() { return fileIndex; }

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
            void insertObject(DBObject object);

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
        } else {
            for (auto objectPointer : objectPointers) {
                memcpy(buffer + location, &objectPointer, sizeof(objectPointer));
                location += sizeof(objectPointer);
            }
        }

        // Create a binary file and write to memory
        ofstream nodeFile;
        nodeFile.open(getFileName(), ios::binary|ios::out);
        nodeFile.write(buffer, pageSize);
        nodeFile.close();
    }

    void Node::readFromDisk() {
        // Create a character buffer which will be written to disk
        long location = 0;
        char buffer[pageSize];

        // Open the binary file ane read into memory
        ifstream nodeFile;
        nodeFile.open(getFileName(), ios::binary|ios::in);
        nodeFile.read(buffer, pageSize);
        nodeFile.close();

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
        } else {
            objectPointers.clear();
            long objectPointer;
            for (long i = 0; i < numKeys; ++i) {
                memcpy((char *) &objectPointer, buffer + location, sizeof(objectPointer));
                location += sizeof(objectPointer);
                objectPointers.push_back(objectPointer);
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

    void Node::insertObject(DBObject object) {
        long position = getKeyPosition(object.getKey());

        // insert the new key to keys
        keys.insert(keys.begin() + position, object.getKey());

        // insert the object pointer to the end
        objectPointers.insert(objectPointers.begin() + position, object.getFileIndex());

        // Commit the new node back into memory
        commitToDisk();
    }

    void Node::serialize() {
        // Return if node is empty
        if (keys.size() == 0) {
            return;
        }

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

#ifdef DEBUG_VERBOSE
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

        // Update the root if the element was inserted in the root
        if (fileIndex == bRoot->getFileIndex()) {
            bRoot->readFromDisk();
        }
    }

    void Node::splitInternal() {
#ifdef DEBUG_VERBOSE
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

#ifdef DEBUG_VERBOSE
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
#ifdef DEBUG_VERBOSE
        cout << endl;
        cout << "SplitLeaf : " << endl;
        cout << "Base Node : ";

        for (auto key : keys) {
            cout << key << " ";
        }
        cout << endl;
#endif

        // Create a surrogate leaf node with the keys and object Pointers
        Node *surrogateLeafNode = new Node();
        for (long i = lowerBound; i < (long) keys.size(); ++i) {
            DBObject object = DBObject(keys[i], objectPointers[i]);
            surrogateLeafNode->insertObject(object);
        }

        // Resize the current leaf node and commit the node to disk
        keys.resize(lowerBound);
        objectPointers.resize(lowerBound);

#ifdef DEBUG_VERBOSE
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
    void insert(Node *root, DBObject object) {
        // If the root is a leaf, we can directly insert
        if (root->isLeaf()) {
            // Insert object
            root->insertObject(object);

            // Split if required
            if (root->size() > root->upperBound) {
                root->splitLeaf();
            }

#ifdef DEBUG_VERBOSE
            // Serialize
            bRoot->serialize();
#endif
        } else {
            // We traverse the tree
            long position = root->getKeyPosition(object.getKey());

            // Load the node from disk
            Node *nextRoot = new Node(root->childIndices[position]);

            // Recurse into the node
            insert(nextRoot, object);

            // Clean up
            delete nextRoot;
        }
    }

    // Point search in a BPlusTree
    void pointQuery(Node *root, double searchKey) {
        // If the root is a leaf, we can directly search
        if (root->isLeaf()) {
            // Print all nodes in the current leaf
            for (long i = 0; i < (long) root->keys.size(); ++i) {
                if (root->keys[i] == searchKey) {
#ifdef DEBUG_NORMAL
                    cout << root->keys[i] << " ";
#endif
#ifdef OUTPUT
                    cout << DBObject(root->keys[i], root->objectPointers[i]).getDataString() << endl;
#endif
                }
            }

            // Check nextleaf for same node
            if (root->nextLeafIndex != DEFAULT_LOCATION) {
                // Load up the nextLeaf from disk
                Node *tempNode = new Node(root->nextLeafIndex);

                // Check in the nextLeaf and delegate
                if (tempNode->keys.front() == searchKey) {
                    pointQuery(tempNode, searchKey);
                }

                delete tempNode;
            }
        } else {
            // We traverse the tree
            long position = root->getKeyPosition(searchKey);

            // Load the node from disk
            Node *nextRoot = new Node(root->childIndices[position]);

            // Recurse into the node
            pointQuery(nextRoot, searchKey);

            // Clean up
            delete nextRoot;
        }
    }

    // window search
    void windowQuery(Node *root, double lowerLimit, double upperLimit) {
        // If the root is a leaf, we can directly search
        if (root->isLeaf()) {
            // Print all nodes in the current leaf which satisfy the criteria
            for (long i = 0; i < (long) root->keys.size(); ++i) {
                if (root->keys[i] >= lowerLimit && root->keys[i] <= upperLimit) {
#ifdef DEBUG_NORMAL
                    cout << root->keys[i] << " ";
#endif
#ifdef OUTPUT
                    cout << DBObject(root->keys[i], root->objectPointers[i]).getDataString() << endl;
#endif
                }
            }

            // If the nextLeafNode is not null
            if (root->nextLeafIndex != DEFAULT_LOCATION) {
                Node *tempNode= new Node(root->nextLeafIndex);

                // Check for condition and recurse
                if (tempNode->keys.front() >= lowerLimit && tempNode->keys.front() <=upperLimit) {
                    windowQuery(tempNode, lowerLimit, upperLimit);
                }

                // Delete the tempNode
                delete tempNode;
            }
        } else {
            // We traverse the tree
            long position = root->getKeyPosition(lowerLimit);

            // Load the node from disk
            Node *nextRoot = new Node(root->childIndices[position]);

            // Recurse into the node
            windowQuery(nextRoot, lowerLimit, upperLimit);

            // Clean up
            delete nextRoot;
        }
    }

    //rangesearch
    void rangeQuery(Node *root, double center, double range) {
        double upperBound = center + range;
        double lowerBound = (center - range >= 0) ? center - range : 0;

        // Call windowQuery internally
        windowQuery(root, lowerBound, upperBound);
    }

    // kNN query
    void kNNQuery(Node *root, double center, long k) {
        // If the root is a leaf, we can directly search
        if (root->isLeaf()) {
            vector< pair<double, long> > answers;

            // We traverse the tree
            long position = root->getKeyPosition(center);

            // Get k keys from ahead
            long count = 0;
            for (long i = position; i < (long)root->keys.size(); ++i, ++count) {
                answers.push_back(make_pair(root->keys[i], root->objectPointers[i]));
            }

            // Now check for leaves in front
            long nextIndex = root->nextLeafIndex;
            while (count < k && nextIndex != DEFAULT_LOCATION) {
                Node tempNode = Node(nextIndex);

                for (long i = 0; i < (long) tempNode.keys.size(); ++i, ++ count) {
                    answers.push_back(make_pair(tempNode.keys[i], tempNode.objectPointers[i]));
                }

                // Update the nextIndex
                nextIndex = tempNode.nextLeafIndex;
            }

            // Get k keys from behind
            count = 0;
            for (long i = 0; i < (long) position; ++i, ++count) {
                answers.push_back(make_pair(root->keys[i], root->objectPointers[i]));
            }

            // Check for leaves behind
            long previousIndex = root->previousLeafIndex;
            while (count < k && previousIndex != DEFAULT_LOCATION) {
                Node tempNode = Node(previousIndex);

                for (long i = 0; i < (long) tempNode.keys.size(); ++i, ++ count) {
                    answers.push_back(make_pair(tempNode.keys[i], tempNode.objectPointers[i]));
                }

                // Update the nextIndex
                previousIndex = tempNode.previousLeafIndex;
            }

            // Sort the obtained answers
            sort(answers.begin(), answers.end(),
                    [&](pair<double, long> T1, pair<double, long> T2) {
                    return (abs(T1.first - center) < abs(T2.first - center));
                    });

            // Print the answers
            pair <double, long> answer;
            for (long i = 0; i < k && i < (long) answers.size(); ++i) {
                answer = answers[i];
#ifdef DEBUG_NORMAL
                cout << answer.first << " ";
#endif
#ifdef OUTPUT
                cout << DBObject(answer.first, answer.second).getDataString() << endl;
#endif

            }
        } else {
            // We traverse the tree
            long position = root->getKeyPosition(center);

            // Load the node from disk
            Node *nextRoot = new Node(root->childIndices[position]);

            // Recurse into the node
            kNNQuery(nextRoot, center, k);

            // Clean up
            delete nextRoot;
        }
    }

    void storeSession() {
        // Create a character buffer which will be written to disk
        long location = 0;
        char buffer[Node::pageSize];

        // Store root's fileIndex
        long fileIndex = bRoot->getFileIndex();
        memcpy(buffer + location, &fileIndex, sizeof(fileIndex));
        location += sizeof(fileIndex);

        // Store the fileCount
        memcpy(buffer + location, &Node::fileCount, sizeof(Node::fileCount));
        location += sizeof(Node::fileCount);

        // Store the objectCount for DBObject
        memcpy(buffer + location, &DBObject::objectCount, sizeof(DBObject::objectCount));
        location += sizeof(DBObject::objectCount);

        // Create a binary file and write to memory
        ofstream sessionFile;
        sessionFile.open(SESSION_FILE, ios::binary|ios::out);
        sessionFile.write(buffer, Node::pageSize);
        sessionFile.close();
    }

    void loadSession() {
        // Create a character buffer which will be written to disk
        long location = 0;
        char buffer[Node::pageSize];

        // Open the binary file ane read into memory
        ifstream sessionFile;
        sessionFile.open(SESSION_FILE, ios::binary|ios::in);
        sessionFile.read(buffer, Node::pageSize);
        sessionFile.close();

        // Retrieve the fileIndex
        long fileIndex;
        memcpy((char *) &fileIndex, buffer + location, sizeof(fileIndex));
        location += sizeof(fileIndex);

        // Retreive the fileCount
        long fileCount;
        memcpy((char *) &fileCount, buffer + location, sizeof(fileCount));
        location += sizeof(fileCount);

        // Retrieve the objectCount
        long objectCount;
        memcpy((char *) &objectCount, buffer + location, sizeof(objectCount));
        location += sizeof(objectCount);

        // Store the session variables
        Node::fileCount = fileCount;
        DBObject::objectCount = objectCount;

        delete bRoot;
        bRoot = new Node(fileIndex);
        bRoot->readFromDisk();
    }
}

using namespace BPlusTree;

void buildTree() {
    ifstream ifile;
    ifile.open("./assgn3_bplus_data.txt", ios::in);

    double key;
    string dataString;
    long count = 0;
    while (ifile >> key >> dataString) {
        if (count % 5000 == 0) {
#ifdef DEBUG_NORMAL
            cout << "Inserting " << count << endl;
#endif
        }

        // Insert the object into file
        insert(bRoot, DBObject(key, dataString));

        // Update the counter
        count++;
    }

    // Close the file
    ifile.close();
}

void processQuery() {
    ifstream ifile;
    ifile.open("./assgn3_bplus_querysample.txt", ios::in);

    long query;
    while (ifile >> query) {
        if (query == 0) {
            double key;
            string dataString;
            ifile >> key >> dataString;

#ifdef OUTPUT
            cout << endl << query << " " << key << " " << dataString << endl;
#endif
#ifdef TIME
            cout << query << " ";
            auto start = std::chrono::high_resolution_clock::now();
#endif
            // Insert into the database
            insert(bRoot, DBObject(key, dataString));
#ifdef TIME
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            cout << microseconds << endl;
#endif
        } else if (query == 1) {
            double key;
            ifile >> key;

#ifdef OUTPUT
            cout << endl << query << " " << key << endl;
#endif
#ifdef TIME
            cout << query << " ";
            auto start = std::chrono::high_resolution_clock::now();
#endif
            // pointQuery
            pointQuery(bRoot, key);
#ifdef TIME
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            cout << microseconds << endl;
#endif
        } else if (query == 2) {
            double key, range;
            ifile >> key >> range;

#ifdef OUTPUT
            cout << endl << query << " " << key << " " << range << endl;
#endif
#ifdef TIME
            cout << query << " ";
            auto start = std::chrono::high_resolution_clock::now();
#endif
            // rangeQuery
            rangeQuery(bRoot, key, range * 0.1);
#ifdef TIME
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            cout << microseconds << endl;
#endif
        } else if (query == 3) {
            double key;
            long k;
            ifile >> key >> k;

#ifdef OUTPUT
            cout << endl << query << " " << key << " " << k << endl;
#endif
#ifdef TIME
            cout << query << " ";
            auto start = std::chrono::high_resolution_clock::now();
#endif
            // kNNQuery
            kNNQuery(bRoot, key, k);
#ifdef TIME
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            cout << microseconds << endl;
#endif
        } else if (query == 4) {
            double lowerLimit;
            double upperLimit;
            ifile >> lowerLimit >> upperLimit;

#ifdef OUTPUT
            cout << endl << query << " " << lowerLimit << " " << upperLimit << endl;
#endif
#ifdef TIME
            cout << query << " ";
            auto start = std::chrono::high_resolution_clock::now();
#endif
            // windowQuery
            windowQuery(bRoot, lowerLimit, upperLimit);
#ifdef TIME
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            cout << microseconds << endl;
#endif
        }
    }

    // Close the file
    ifile.close();
}

int main() {
    // Initialize the BPlusTree module
    Node::initialize();

    // Create a new tree
    bRoot = new Node();

    // Load session or build a new tree
    ifstream sessionFile(SESSION_FILE);
    if (sessionFile.good()) {
        loadSession();
    } else {
        buildTree();
    }

    // Process queries
    processQuery();

    // Store the session
    storeSession();

    return 0;
}
