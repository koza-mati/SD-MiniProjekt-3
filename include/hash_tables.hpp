#ifndef HASH_TABLES_HPP
#define HASH_TABLES_HPP

#include <fstream>
#include <string>

// Pojedynczy rekord słownika: para klucz-wartość (obie liczby całkowite).
struct HashRecord {
    int key;
    int value;
};

// Wspólny interfejs wszystkich wariantów tablicy mieszającej. Pozwala menu oraz
// modułowi badań operować na każdej implementacji w identyczny sposób.
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

// Tablica mieszająca z adresowaniem otwartym, w trybie próbkowania liniowego
// lub kwadratowego.
class OpenAddressingHashTable : public IHashTable {
public:
    enum ProbeMode {
        Linear,     // (hash + i) % capacity
        Quadratic   // (hash + i + i*i) % capacity
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
        Empty,      // komórka nigdy nie była użyta
        Occupied,   // komórka przechowuje aktywny element
        Deleted     // nagrobek: element usunięto, ale ścieżka próbkowania trwa
    };

    struct Entry {
        int key;
        int value;
        EntryState state;
    };

    ProbeMode probeMode;
    Entry* table;
    int capacity;          // liczba komórek (zawsze liczba pierwsza)
    int currentSize;       // liczba aktywnych elementów
    int deletedCount;      // liczba nagrobków

    int hash(int key) const;
    int probeIndex(int baseIndex, int attempt) const;
    bool shouldGrow() const;
    void allocateTable(int newCapacity);
    void rehash(int newCapacity);
    int findSlot(int key) const;
    int nextPrime(int value) const;
    bool isPrime(int value) const;
};

// Tablica mieszająca z łańcuchowaniem, w której każdy kubełek jest niezależnym,
// zbalansowanym drzewem AVL (kolizje obsługiwane w czasie O(log k)).
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
        int height;   // wysokość węzła, potrzebna do balansowania
        Node* left;
        Node* right;
    };

    Node** buckets;    // tablica korzeni drzew AVL (kubełków)
    int capacity;      // liczba kubełków (zawsze liczba pierwsza)
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

// Tablica mieszająca w schemacie cuckoo hashing: dwie tablice i dwie niezależne
// funkcje mieszające. Każdy klucz ma dwie dozwolone pozycje, więc find i remove
// mają gwarantowaną złożoność O(1) w najgorszym przypadku.
class CuckooHashTable : public IHashTable {
public:
    CuckooHashTable(int initialCapacity = 101);
    ~CuckooHashTable();

    bool insert(int key, int value);
    bool remove(int key);
    bool find(int key, int& value) const;
    int returnSize() const;
    void clear();
    bool saveToCSV(const std::string& fileName) const;
    bool loadFromCSV(const std::string& fileName);
    const char* name() const;

private:
    struct Slot {
        int key;
        int value;
        bool occupied;
    };

    Slot* first;       // tablica adresowana przez hashFirst
    Slot* second;      // tablica adresowana przez hashSecond
    int capacity;      // pojemność każdej z tablic (zawsze liczba pierwsza)
    int currentSize;

    int hashFirst(int key) const;
    int hashSecond(int key) const;
    bool shouldGrow() const;
    int maxKicks() const;   // limit przeniesień przed uznaniem wystąpienia cyklu
    bool updateExisting(int key, int value);
    void allocateTables(int newCapacity);
    void rehash(int newCapacity);
    int nextPrime(int value) const;
    bool isPrime(int value) const;
};

#endif
