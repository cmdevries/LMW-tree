#ifndef NODE_H
#define NODE_H

#include "StdIncludes.h"

template <typename T>
class Node {
private:
    // In Leaf nodes the keys are the data.
    vector<T*> _keys;

    // Child nodes if this is an internal cluster node.
    vector<Node*> _children;

    bool _isLeaf;
    
public:

    Node() {
        _isLeaf = true;
    }
    
    ~Node() {
        for (size_t i = 0; i < size(); i++) {
            remove(i);
        }
    }

    bool isEmpty() {
        return _keys.empty();
    }

    bool isLeaf() {
        return _isLeaf;
    }

    int size() {
        return _keys.size();
    }

    T* getKey(int i) {
        return _keys[i];
    }

    vector<T*>& getKeys() {
        return _keys;
    }

    // pre: is not a leaf

    Node* getChild(int i) {
        return _children[i];
    }

    // pre: is not a leaf

    vector<Node*>& getChildren() {
        return _children;
    }

    void clearKeysAndChildren() {
        _children.clear();
        _keys.clear();
    }

    // pre: is leaf

    void add(T* key) {
        _keys.push_back(key);
    }

    void add(T* key, Node *node) {
        _keys.push_back(key);
        _children.push_back(node);
        _isLeaf = false;
    }

    void addAll(vector<T*> &keys) {
        _keys = keys;
    }

    void removeData(vector<T*>& data) {
        if (isLeaf()) {
            for (int i = 0; i < _keys.size(); i++) {
                data.push_back(_keys[i]);
            }
            _keys.clear();
        }
    }

    void removeData(vector<T*>& keys, vector<Node<T>*>& children) {
        std::copy(_keys.begin(), _keys.end(), std::back_inserter(keys));
        _keys.clear();
        std::copy(_children.begin(), _children.end(), std::back_inserter(children));
        _children.clear();
        _isLeaf = true;
    }

    void removeEmptyChildren() {


    }

    void remove(int i) {
        if (!_isLeaf) {
            // Free the memory for the centroid key
            delete _keys[i];
            _keys[i] = NULL;
            // Delete associated child node
            delete _children[i];
            _children[i] = NULL;
        }
    }

    // Use this to finalize individual removals.
    // No need to use this after calling removeAll()

    void finalizeRemovals() {

        // Remove NULL keys and corresponding NULL children
        int toRemove = 0;
        int sz = _keys.size();

        for (int i = 0; i < _keys.size(); i++) {
            if (_keys[i] == NULL) {
                toRemove++;
            } else {
                // Shuffle keys down 
                _keys[i - toRemove] = _keys[i];

                // Shuffle children down
                if (!_isLeaf) _children[i - toRemove] = _children[i];
            }
        }

        // Resize vector containers
        int newSize = sz - toRemove;

        if (toRemove > 0) {
            _keys.resize(newSize);
            if (!_isLeaf) _children.resize(newSize);
        }
    }

};


#endif	/* NODE_H */



