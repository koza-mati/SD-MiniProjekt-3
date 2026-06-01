#ifndef HASH_TABLES_HPP
#define HASH_TABLES_HPP

#include <fstream>
#include <string>

struct HashRecord {
    int key;
    int value;
};

class IHashTable {
public:
    virtual ~IHashTable() {}
    virtual bool insert(int key, int value) = 0;
    virtual bool remove(int key) = 0;
    virtual bool find(int key, int& value) const = 0;
    virtual int returnSize() const = 0;
    virtual void clear() = 0;
    virtual bool saveToCSV(const std::string& fileName) const = 0;
    virtual bool loadFromCSV(const std::string& fileName) = 0;
    virtual const char* name() const = 0;
};

class OpenAddressingHashTable : public IHashTable {
public:
    enum ProbeMode {
        Linear,
        Quadratic
    };

    OpenAddressingHashTable(ProbeMode mode, int initialCapacity = 101);
    ~OpenAddressingHashTable();

    bool insert(int key, int value);
    bool remove(int key);
    bool find(int key, int& value) const;
    int returnSize() const;
    void clear();
    bool saveToCSV(const std::string& fileName) const;
    bool loadFromCSV(const std::string& fileName);
    const char* name() const;

private:
    enum EntryState {
        Empty,
        Occupied,
        Deleted
    };

    struct Entry {
        int key;
        int value;
        EntryState state;
    };

    ProbeMode probeMode;
    Entry* table;
    int capacity;
    int currentSize;
    int deletedCount;

    int hash(int key) const;
    int probeIndex(int baseIndex, int attempt) const;
    bool shouldGrow() const;
    void allocateTable(int newCapacity);
    void rehash(int newCapacity);
    int findSlot(int key) const;
    int nextPrime(int value) const;
    bool isPrime(int value) const;
};

class AVLHashTable : public IHashTable {
public:
    AVLHashTable(int initialCapacity = 101);
    ~AVLHashTable();

    bool insert(int key, int value);
    bool remove(int key);
    bool find(int key, int& value) const;
    int returnSize() const;
    void clear();
    bool saveToCSV(const std::string& fileName) const;
    bool loadFromCSV(const std::string& fileName);
    const char* name() const;

private:
    struct Node {
        int key;
        int value;
        int height;
        Node* left;
        Node* right;
    };

    Node** buckets;
    int capacity;
    int currentSize;

    int hash(int key) const;
    int height(Node* node) const;
    int balance(Node* node) const;
    int maxInt(int a, int b) const;
    Node* createNode(int key, int value);
    Node* rotateRight(Node* node);
    Node* rotateLeft(Node* node);
    Node* insertNode(Node* node, int key, int value, bool& added);
    Node* removeNode(Node* node, int key, bool& removed);
    Node* minNode(Node* node) const;
    bool findNode(Node* node, int key, int& value) const;
    void clearNode(Node* node);
    void saveNode(Node* node, std::ofstream& file) const;
    void collectAndInsert(Node* node, AVLHashTable& target) const;
    void rehash(int newCapacity);
    bool shouldGrow() const;
    int nextPrime(int value) const;
    bool isPrime(int value) const;
};

#endif
