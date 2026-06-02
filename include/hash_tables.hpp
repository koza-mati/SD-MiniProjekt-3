#ifndef HASH_TABLES_HPP
#define HASH_TABLES_HPP

#include <fstream>
#include <string>

// Pojedynczy rekord słownika: para klucz-wartość.
// Zarówno klucz, jak i wartość są liczbami całkowitymi.
struct HashRecord {
    int key;
    int value;
};

// Wspólny interfejs dla wszystkich wariantów tablicy mieszającej.
// Dzięki niemu menu oraz moduł badań wydajnościowych mogą operować na każdej
// implementacji w identyczny sposób, bez znajomości jej szczegółów.
class IHashTable {
public:
    virtual ~IHashTable() {}
    virtual bool insert(int key, int value) = 0;       // wstawia lub aktualizuje parę
    virtual bool remove(int key) = 0;                  // usuwa parę o danym kluczu
    virtual bool find(int key, int& value) const = 0;  // wyszukuje wartość po kluczu
    virtual int returnSize() const = 0;                // zwraca liczbę elementów
    virtual void clear() = 0;                          // usuwa wszystkie elementy
    virtual bool saveToCSV(const std::string& fileName) const = 0;
    virtual bool loadFromCSV(const std::string& fileName) = 0;
    virtual const char* name() const = 0;              // czytelna nazwa struktury
};

// Tablica mieszająca z adresowaniem otwartym.
// Obsługuje dwa tryby próbkowania: liniowe oraz kwadratowe.
class OpenAddressingHashTable : public IHashTable {
public:
    enum ProbeMode {
        Linear,     // próbkowanie liniowe: (hash + i) % capacity
        Quadratic   // próbkowanie kwadratowe: (hash + i + i*i) % capacity
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
    // Stan pojedynczej komórki tablicy.
    enum EntryState {
        Empty,      // komórka nigdy nie była użyta
        Occupied,   // komórka przechowuje aktywny element
        Deleted     // komórka zawierała element, który został usunięty (nagrobek)
    };

    // Pojedyncza komórka tablicy: para klucz-wartość wraz ze stanem.
    struct Entry {
        int key;
        int value;
        EntryState state;
    };

    ProbeMode probeMode;   // wybrany tryb próbkowania
    Entry* table;          // dynamicznie alokowana tablica komórek
    int capacity;          // liczba komórek tablicy (zawsze liczba pierwsza)
    int currentSize;       // liczba aktywnych elementów
    int deletedCount;      // liczba nagrobków (komórek w stanie Deleted)

    int hash(int key) const;                            // funkcja mieszająca: |key| % capacity
    int probeIndex(int baseIndex, int attempt) const;   // indeks dla kolejnej próby
    bool shouldGrow() const;                            // czy tablica wymaga powiększenia
    void allocateTable(int newCapacity);                // alokuje i inicjalizuje nową tablicę
    void rehash(int newCapacity);                       // przebudowuje tablicę o nowej pojemności
    int findSlot(int key) const;                        // wyszukuje komórkę aktywnego klucza
    int nextPrime(int value) const;                     // najbliższa liczba pierwsza >= value
    bool isPrime(int value) const;
};

// Tablica mieszająca z łańcuchowaniem (adresowanie zamknięte).
// Każdy kubełek jest niezależnym, zbalansowanym drzewem AVL, dzięki czemu
// kolizje w obrębie jednego kubełka są obsługiwane w czasie O(log k).
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
    // Węzeł drzewa AVL przechowywanego w pojedynczym kubełku.
    struct Node {
        int key;
        int value;
        int height;   // wysokość węzła, potrzebna do balansowania
        Node* left;
        Node* right;
    };

    Node** buckets;    // tablica wskaźników na korzenie drzew AVL (kubełki)
    int capacity;      // liczba kubełków (zawsze liczba pierwsza)
    int currentSize;   // łączna liczba elementów we wszystkich kubełkach

    int hash(int key) const;             // funkcja mieszająca wybierająca kubełek
    int height(Node* node) const;        // wysokość węzła (0 dla pustego)
    int balance(Node* node) const;       // współczynnik zrównoważenia węzła
    int maxInt(int a, int b) const;
    Node* createNode(int key, int value);
    Node* rotateRight(Node* node);       // rotacja w prawo
    Node* rotateLeft(Node* node);        // rotacja w lewo
    Node* insertNode(Node* node, int key, int value, bool& added);   // wstawianie z balansowaniem
    Node* removeNode(Node* node, int key, bool& removed);            // usuwanie z balansowaniem
    Node* minNode(Node* node) const;     // węzeł o najmniejszym kluczu (następnik)
    bool findNode(Node* node, int key, int& value) const;
    void clearNode(Node* node);          // rekurencyjne zwalnianie poddrzewa
    void saveNode(Node* node, std::ofstream& file) const;            // zapis in-order do pliku
    void collectAndInsert(Node* node, AVLHashTable& target) const;   // przeniesienie do nowej tablicy
    void rehash(int newCapacity);        // przebudowa tablicy kubełków
    bool shouldGrow() const;             // czy tablica wymaga powiększenia
    int nextPrime(int value) const;
    bool isPrime(int value) const;
};

// Tablica mieszająca w schemacie cuckoo hashing.
// Wykorzystuje dwie osobne tablice oraz dwie niezależne funkcje mieszające.
// Każdy klucz ma dokładnie dwie dozwolone pozycje, dzięki czemu operacje
// find i remove mają gwarantowaną złożoność O(1) w najgorszym przypadku.
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
    // Pojedyncza komórka jednej z dwóch tablic.
    struct Slot {
        int key;
        int value;
        bool occupied;   // czy komórka przechowuje aktywny element
    };

    Slot* first;       // pierwsza tablica, adresowana przez hashFirst
    Slot* second;      // druga tablica, adresowana przez hashSecond
    int capacity;      // pojemność każdej z tablic (zawsze liczba pierwsza)
    int currentSize;   // łączna liczba elementów w obu tablicach

    int hashFirst(int key) const;    // pierwsza funkcja mieszająca (pozycja w tablicy first)
    int hashSecond(int key) const;   // druga funkcja mieszająca (pozycja w tablicy second)
    bool shouldGrow() const;         // czy łączne wypełnienie zbliża się do połowy pojemności
    int maxKicks() const;            // limit przeniesień przed uznaniem wystąpienia cyklu
    bool updateExisting(int key, int value);   // aktualizuje wartość, jeśli klucz już istnieje
    void allocateTables(int newCapacity);      // alokuje i inicjalizuje obie tablice
    void rehash(int newCapacity);              // przebudowa obu tablic o nowej pojemności
    int nextPrime(int value) const;
    bool isPrime(int value) const;
};

#endif
