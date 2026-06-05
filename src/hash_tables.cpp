#include "../include/hash_tables.hpp"

#include <cstdlib>
#include <sstream>

// ===========================================================================
//  Adresowanie otwarte (próbkowanie liniowe i kwadratowe)
// ===========================================================================

OpenAddressingHashTable::OpenAddressingHashTable(ProbeMode mode, int initialCapacity)
    : probeMode(mode), table(0), capacity(0), currentSize(0), deletedCount(0) {
    allocateTable(nextPrime(initialCapacity));
}

OpenAddressingHashTable::~OpenAddressingHashTable() {
    delete[] table;
}

// Zwraca true, gdy dodano nowy element, false, gdy zaktualizowano istniejący klucz.
bool OpenAddressingHashTable::insert(int key, int value) {
    if (shouldGrow()) {
        rehash(nextPrime(capacity * 2 + 1));
    }

    int baseIndex = hash(key);
    int firstDeleted = -1;   // pierwszy nagrobek na ścieżce, w który można wstawić element

    for (int attempt = 0; attempt < capacity; ++attempt) {
        int index = probeIndex(baseIndex, attempt);
        if (table[index].state == Occupied && table[index].key == key) {
            table[index].value = value;
            return false;
        }
        if (table[index].state == Deleted && firstDeleted == -1) {
            firstDeleted = index;
        }
        // Pusta komórka kończy ścieżkę próbkowania - klucza nie ma w tablicy.
        if (table[index].state == Empty) {
            int target = firstDeleted != -1 ? firstDeleted : index;
            table[target].key = key;
            table[target].value = value;
            if (table[target].state == Deleted) {
                --deletedCount;
            }
            table[target].state = Occupied;
            ++currentSize;
            return true;
        }
    }

    if (firstDeleted != -1) {
        table[firstDeleted].key = key;
        table[firstDeleted].value = value;
        table[firstDeleted].state = Occupied;
        --deletedCount;
        ++currentSize;
        return true;
    }

    // Brak wolnego miejsca - powiększamy tablicę i ponawiamy wstawianie.
    rehash(nextPrime(capacity * 2 + 1));
    return insert(key, value);
}

// Komórka jest oznaczana jako nagrobek (Deleted), aby nie przerwać ścieżki
// próbkowania innych kluczy.
bool OpenAddressingHashTable::remove(int key) {
    int index = findSlot(key);
    if (index == -1) {
        return false;
    }

    table[index].state = Deleted;
    --currentSize;
    ++deletedCount;
    return true;
}

bool OpenAddressingHashTable::find(int key, int& value) const {
    int index = findSlot(key);
    if (index == -1) {
        return false;
    }

    value = table[index].value;
    return true;
}

int OpenAddressingHashTable::returnSize() const {
    return currentSize;
}

void OpenAddressingHashTable::clear() {
    for (int i = 0; i < capacity; ++i) {
        table[i].state = Empty;
    }
    currentSize = 0;
    deletedCount = 0;
}

bool OpenAddressingHashTable::saveToCSV(const std::string& fileName) const {
    std::ofstream file(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }

    file << "key,value\n";
    for (int i = 0; i < capacity; ++i) {
        if (table[i].state == Occupied) {
            file << table[i].key << "," << table[i].value << "\n";
        }
    }
    return true;
}

bool OpenAddressingHashTable::loadFromCSV(const std::string& fileName) {
    std::ifstream file(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }

    clear();
    std::string line;
    std::getline(file, line);   // nagłówek
    while (std::getline(file, line)) {
        std::stringstream stream(line);
        std::string keyText;
        std::string valueText;
        if (std::getline(stream, keyText, ',') && std::getline(stream, valueText, ',')) {
            insert(std::atoi(keyText.c_str()), std::atoi(valueText.c_str()));
        }
    }
    return true;
}

const char* OpenAddressingHashTable::name() const {
    return probeMode == Linear ? "Tablica mieszajaca - adresowanie liniowe"
                               : "Tablica mieszajaca - adresowanie kwadratowe";
}

int OpenAddressingHashTable::hash(int key) const {
    long long normalized = key;
    if (normalized < 0) {
        normalized = -normalized;
    }
    return static_cast<int>(normalized % capacity);
}

int OpenAddressingHashTable::probeIndex(int baseIndex, int attempt) const {
    if (probeMode == Linear) {
        return (baseIndex + attempt) % capacity;
    }
    return (baseIndex + attempt + attempt * attempt) % capacity;
}

// Pod uwagę brane są aktywne elementy i nagrobki. Próbkowanie kwadratowe wymaga
// niższego wypełnienia, aby sekwencja próbkowania nie pomijała wolnych komórek.
bool OpenAddressingHashTable::shouldGrow() const {
    int used = currentSize + deletedCount;
    int limit = probeMode == Quadratic ? capacity / 2 : capacity * 7 / 10;
    return used + 1 >= limit;
}

void OpenAddressingHashTable::allocateTable(int newCapacity) {
    capacity = newCapacity;
    table = new Entry[capacity];
    for (int i = 0; i < capacity; ++i) {
        table[i].key = 0;
        table[i].value = 0;
        table[i].state = Empty;
    }
}

// Tworzy większą tablicę i wstawia ponownie aktywne elementy, pomijając nagrobki.
void OpenAddressingHashTable::rehash(int newCapacity) {
    Entry* oldTable = table;
    int oldCapacity = capacity;

    table = 0;
    allocateTable(newCapacity);
    currentSize = 0;
    deletedCount = 0;

    for (int i = 0; i < oldCapacity; ++i) {
        if (oldTable[i].state == Occupied) {
            insert(oldTable[i].key, oldTable[i].value);
        }
    }

    delete[] oldTable;
}

int OpenAddressingHashTable::findSlot(int key) const {
    int baseIndex = hash(key);
    for (int attempt = 0; attempt < capacity; ++attempt) {
        int index = probeIndex(baseIndex, attempt);
        if (table[index].state == Empty) {   // koniec ścieżki - klucza nie ma
            return -1;
        }
        if (table[index].state == Occupied && table[index].key == key) {
            return index;
        }
    }
    return -1;
}

int OpenAddressingHashTable::nextPrime(int value) const {
    if (value < 2) {
        return 2;
    }
    while (!isPrime(value)) {
        ++value;
    }
    return value;
}

bool OpenAddressingHashTable::isPrime(int value) const {
    if (value < 2) {
        return false;
    }
    if (value == 2) {
        return true;
    }
    if (value % 2 == 0) {
        return false;
    }
    for (int i = 3; i * i <= value; i += 2) {
        if (value % i == 0) {
            return false;
        }
    }
    return true;
}

// ===========================================================================
//  Łańcuchowanie kubełkami w postaci drzew AVL
// ===========================================================================

AVLHashTable::AVLHashTable(int initialCapacity)
    : buckets(0), capacity(nextPrime(initialCapacity)), currentSize(0) {
    buckets = new Node*[capacity];
    for (int i = 0; i < capacity; ++i) {
        buckets[i] = 0;
    }
}

AVLHashTable::~AVLHashTable() {
    clear();
    delete[] buckets;
}

bool AVLHashTable::insert(int key, int value) {
    if (shouldGrow()) {
        rehash(nextPrime(capacity * 2 + 1));
    }

    bool added = false;
    int index = hash(key);
    buckets[index] = insertNode(buckets[index], key, value, added);
    if (added) {
        ++currentSize;
    }
    return added;
}

bool AVLHashTable::remove(int key) {
    bool removed = false;
    int index = hash(key);
    buckets[index] = removeNode(buckets[index], key, removed);
    if (removed) {
        --currentSize;
    }
    return removed;
}

bool AVLHashTable::find(int key, int& value) const {
    return findNode(buckets[hash(key)], key, value);
}

int AVLHashTable::returnSize() const {
    return currentSize;
}

void AVLHashTable::clear() {
    for (int i = 0; i < capacity; ++i) {
        clearNode(buckets[i]);
        buckets[i] = 0;
    }
    currentSize = 0;
}

bool AVLHashTable::saveToCSV(const std::string& fileName) const {
    std::ofstream file(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }

    file << "key,value\n";
    for (int i = 0; i < capacity; ++i) {
        saveNode(buckets[i], file);
    }
    return true;
}

bool AVLHashTable::loadFromCSV(const std::string& fileName) {
    std::ifstream file(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }

    clear();
    std::string line;
    std::getline(file, line);   // nagłówek
    while (std::getline(file, line)) {
        std::stringstream stream(line);
        std::string keyText;
        std::string valueText;
        if (std::getline(stream, keyText, ',') && std::getline(stream, valueText, ',')) {
            insert(std::atoi(keyText.c_str()), std::atoi(valueText.c_str()));
        }
    }
    return true;
}

const char* AVLHashTable::name() const {
    return "Tablica mieszajaca - lancuchowanie drzewami AVL";
}

int AVLHashTable::hash(int key) const {
    long long normalized = key;
    if (normalized < 0) {
        normalized = -normalized;
    }
    return static_cast<int>(normalized % capacity);
}

int AVLHashTable::height(Node* node) const {
    return node ? node->height : 0;
}

// Współczynnik zrównoważenia: różnica wysokości lewego i prawego poddrzewa.
int AVLHashTable::balance(Node* node) const {
    return node ? height(node->left) - height(node->right) : 0;
}

int AVLHashTable::maxInt(int a, int b) const {
    return a > b ? a : b;
}

AVLHashTable::Node* AVLHashTable::createNode(int key, int value) {
    Node* node = new Node;
    node->key = key;
    node->value = value;
    node->height = 1;
    node->left = 0;
    node->right = 0;
    return node;
}

AVLHashTable::Node* AVLHashTable::rotateRight(Node* node) {
    Node* leftChild = node->left;
    Node* middle = leftChild->right;

    leftChild->right = node;
    node->left = middle;

    node->height = maxInt(height(node->left), height(node->right)) + 1;
    leftChild->height = maxInt(height(leftChild->left), height(leftChild->right)) + 1;
    return leftChild;
}

AVLHashTable::Node* AVLHashTable::rotateLeft(Node* node) {
    Node* rightChild = node->right;
    Node* middle = rightChild->left;

    rightChild->left = node;
    node->right = middle;

    node->height = maxInt(height(node->left), height(node->right)) + 1;
    rightChild->height = maxInt(height(rightChild->left), height(rightChild->right)) + 1;
    return rightChild;
}

AVLHashTable::Node* AVLHashTable::insertNode(Node* node, int key, int value, bool& added) {
    if (!node) {
        added = true;
        return createNode(key, value);
    }

    if (key < node->key) {
        node->left = insertNode(node->left, key, value, added);
    } else if (key > node->key) {
        node->right = insertNode(node->right, key, value, added);
    } else {
        node->value = value;   // klucz już istnieje - tylko aktualizacja wartości
        return node;
    }

    node->height = 1 + maxInt(height(node->left), height(node->right));
    int nodeBalance = balance(node);

    // Cztery przypadki niezrównoważenia i odpowiadające im rotacje.
    if (nodeBalance > 1 && key < node->left->key) {
        return rotateRight(node);
    }
    if (nodeBalance < -1 && key > node->right->key) {
        return rotateLeft(node);
    }
    if (nodeBalance > 1 && key > node->left->key) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    if (nodeBalance < -1 && key < node->right->key) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

AVLHashTable::Node* AVLHashTable::removeNode(Node* node, int key, bool& removed) {
    if (!node) {
        return 0;
    }

    if (key < node->key) {
        node->left = removeNode(node->left, key, removed);
    } else if (key > node->key) {
        node->right = removeNode(node->right, key, removed);
    } else {
        removed = true;
        if (!node->left || !node->right) {
            Node* child = node->left ? node->left : node->right;
            delete node;
            return child;
        }

        // Węzeł z dwojgiem dzieci zastępujemy następnikiem (najmniejszy klucz
        // w prawym poddrzewie), który następnie usuwamy z prawego poddrzewa.
        Node* successor = minNode(node->right);
        node->key = successor->key;
        node->value = successor->value;
        bool ignored = false;
        node->right = removeNode(node->right, successor->key, ignored);
    }

    node->height = 1 + maxInt(height(node->left), height(node->right));
    int nodeBalance = balance(node);

    if (nodeBalance > 1 && balance(node->left) >= 0) {
        return rotateRight(node);
    }
    if (nodeBalance > 1 && balance(node->left) < 0) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    if (nodeBalance < -1 && balance(node->right) <= 0) {
        return rotateLeft(node);
    }
    if (nodeBalance < -1 && balance(node->right) > 0) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

AVLHashTable::Node* AVLHashTable::minNode(Node* node) const {
    Node* current = node;
    while (current && current->left) {
        current = current->left;
    }
    return current;
}

bool AVLHashTable::findNode(Node* node, int key, int& value) const {
    while (node) {
        if (key < node->key) {
            node = node->left;
        } else if (key > node->key) {
            node = node->right;
        } else {
            value = node->value;
            return true;
        }
    }
    return false;
}

void AVLHashTable::clearNode(Node* node) {
    if (!node) {
        return;
    }
    clearNode(node->left);
    clearNode(node->right);
    delete node;
}

void AVLHashTable::saveNode(Node* node, std::ofstream& file) const {
    if (!node) {
        return;
    }
    saveNode(node->left, file);
    file << node->key << "," << node->value << "\n";
    saveNode(node->right, file);
}

void AVLHashTable::collectAndInsert(Node* node, AVLHashTable& target) const {
    if (!node) {
        return;
    }
    collectAndInsert(node->left, target);
    target.insert(node->key, node->value);
    collectAndInsert(node->right, target);
}

// Przebudowa metodą kopiowania i zamiany: budujemy nową tablicę kubełków,
// przenosimy do niej elementy, po czym przejmujemy jej zasoby.
void AVLHashTable::rehash(int newCapacity) {
    AVLHashTable newTable(newCapacity);
    for (int i = 0; i < capacity; ++i) {
        collectAndInsert(buckets[i], newTable);
    }

    for (int i = 0; i < capacity; ++i) {
        clearNode(buckets[i]);
    }
    delete[] buckets;

    buckets = newTable.buckets;
    capacity = newTable.capacity;
    currentSize = newTable.currentSize;
    newTable.buckets = 0;
    newTable.capacity = 0;
    newTable.currentSize = 0;
}

// Powiększenie przy średnio około czterech elementach na kubełek.
bool AVLHashTable::shouldGrow() const {
    return currentSize > capacity * 4;
}

int AVLHashTable::nextPrime(int value) const {
    if (value < 2) {
        return 2;
    }
    while (!isPrime(value)) {
        ++value;
    }
    return value;
}

bool AVLHashTable::isPrime(int value) const {
    if (value < 2) {
        return false;
    }
    if (value == 2) {
        return true;
    }
    if (value % 2 == 0) {
        return false;
    }
    for (int i = 3; i * i <= value; i += 2) {
        if (value % i == 0) {
            return false;
        }
    }
    return true;
}

// ===========================================================================
//  Cuckoo hashing (dwie tablice, dwie funkcje mieszające)
// ===========================================================================

CuckooHashTable::CuckooHashTable(int initialCapacity)
    : first(0), second(0), capacity(0), currentSize(0) {
    allocateTables(nextPrime(initialCapacity));
}

CuckooHashTable::~CuckooHashTable() {
    delete[] first;
    delete[] second;
}

bool CuckooHashTable::insert(int key, int value) {
    if (updateExisting(key, value)) {
        return false;
    }

    int currentKey = key;
    int currentValue = value;

    // Pętla zamiast rekurencji - przy dużych zestawach danych rekurencyjne
    // ponawianie po rehash mogło przepełnić stos (szczególnie w buildzie Debug).
    while (true) {
        if (shouldGrow()) {
            rehash(nextPrime(capacity * 2 + 1));
        }

        int whichTable = 0;   // 0 - tablica first, 1 - tablica second
        int limit = maxKicks();

        // Zasada cuckoo: jeśli docelowa komórka jest zajęta, wyrzucamy jej element
        // i przenosimy go na alternatywną pozycję w drugiej tablicy, powtarzając
        // proces naprzemiennie aż do znalezienia wolnej komórki.
        for (int kick = 0; kick < limit; ++kick) {
            if (whichTable == 0) {
                int index = hashFirst(currentKey);
                if (!first[index].occupied) {
                    first[index].key = currentKey;
                    first[index].value = currentValue;
                    first[index].occupied = true;
                    ++currentSize;
                    return true;
                }
                int evictedKey = first[index].key;
                int evictedValue = first[index].value;
                first[index].key = currentKey;
                first[index].value = currentValue;
                currentKey = evictedKey;
                currentValue = evictedValue;
                whichTable = 1;
            } else {
                int index = hashSecond(currentKey);
                if (!second[index].occupied) {
                    second[index].key = currentKey;
                    second[index].value = currentValue;
                    second[index].occupied = true;
                    ++currentSize;
                    return true;
                }
                int evictedKey = second[index].key;
                int evictedValue = second[index].value;
                second[index].key = currentKey;
                second[index].value = currentValue;
                currentKey = evictedKey;
                currentValue = evictedValue;
                whichTable = 0;
            }
        }

        // Przekroczenie limitu przeniesień oznacza cykl - powiększamy strukturę
        // i ponawiamy wstawianie trzymanego "w ręku" elementu.
        rehash(nextPrime(capacity * 2 + 1));
    }
}

bool CuckooHashTable::remove(int key) {
    int indexFirst = hashFirst(key);
    if (first[indexFirst].occupied && first[indexFirst].key == key) {
        first[indexFirst].occupied = false;
        --currentSize;
        return true;
    }

    int indexSecond = hashSecond(key);
    if (second[indexSecond].occupied && second[indexSecond].key == key) {
        second[indexSecond].occupied = false;
        --currentSize;
        return true;
    }

    return false;
}

bool CuckooHashTable::find(int key, int& value) const {
    int indexFirst = hashFirst(key);
    if (first[indexFirst].occupied && first[indexFirst].key == key) {
        value = first[indexFirst].value;
        return true;
    }

    int indexSecond = hashSecond(key);
    if (second[indexSecond].occupied && second[indexSecond].key == key) {
        value = second[indexSecond].value;
        return true;
    }

    return false;
}

int CuckooHashTable::returnSize() const {
    return currentSize;
}

void CuckooHashTable::clear() {
    for (int i = 0; i < capacity; ++i) {
        first[i].occupied = false;
        second[i].occupied = false;
    }
    currentSize = 0;
}

bool CuckooHashTable::saveToCSV(const std::string& fileName) const {
    std::ofstream file(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }

    file << "key,value\n";
    for (int i = 0; i < capacity; ++i) {
        if (first[i].occupied) {
            file << first[i].key << "," << first[i].value << "\n";
        }
    }
    for (int i = 0; i < capacity; ++i) {
        if (second[i].occupied) {
            file << second[i].key << "," << second[i].value << "\n";
        }
    }
    return true;
}

bool CuckooHashTable::loadFromCSV(const std::string& fileName) {
    std::ifstream file(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }

    clear();
    std::string line;
    std::getline(file, line);   // nagłówek
    while (std::getline(file, line)) {
        std::stringstream stream(line);
        std::string keyText;
        std::string valueText;
        if (std::getline(stream, keyText, ',') && std::getline(stream, valueText, ',')) {
            insert(std::atoi(keyText.c_str()), std::atoi(valueText.c_str()));
        }
    }
    return true;
}

const char* CuckooHashTable::name() const {
    return "Tablica mieszajaca - cuckoo hashing";
}

// Dwie niezależne funkcje mieszające (mieszanie multiplikatywne) wyznaczające
// pozycje klucza w obu tablicach.
int CuckooHashTable::hashFirst(int key) const {
    unsigned int mixed = static_cast<unsigned int>(key);
    mixed *= 2654435761u;
    return static_cast<int>(mixed % static_cast<unsigned int>(capacity));
}

int CuckooHashTable::hashSecond(int key) const {
    unsigned int mixed = static_cast<unsigned int>(key);
    mixed ^= mixed >> 16;
    mixed *= 2246822519u;
    mixed ^= mixed >> 13;
    return static_cast<int>(mixed % static_cast<unsigned int>(capacity));
}

// Łączne wypełnienie obu tablic utrzymujemy poniżej połowy ich pojemności.
bool CuckooHashTable::shouldGrow() const {
    return currentSize + 1 >= capacity;
}

// Limit przeniesień rosnący logarytmicznie z pojemnością; jego przekroczenie
// traktujemy jako wystąpienie cyklu wymagającego przebudowy.
int CuckooHashTable::maxKicks() const {
    int limit = 8;
    int value = capacity;
    while (value > 1) {
        value /= 2;
        ++limit;
    }
    return limit;
}

bool CuckooHashTable::updateExisting(int key, int value) {
    int indexFirst = hashFirst(key);
    if (first[indexFirst].occupied && first[indexFirst].key == key) {
        first[indexFirst].value = value;
        return true;
    }

    int indexSecond = hashSecond(key);
    if (second[indexSecond].occupied && second[indexSecond].key == key) {
        second[indexSecond].value = value;
        return true;
    }

    return false;
}

void CuckooHashTable::allocateTables(int newCapacity) {
    capacity = newCapacity;
    first = new Slot[capacity];
    second = new Slot[capacity];
    for (int i = 0; i < capacity; ++i) {
        first[i].key = 0;
        first[i].value = 0;
        first[i].occupied = false;
        second[i].key = 0;
        second[i].value = 0;
        second[i].occupied = false;
    }
}

void CuckooHashTable::rehash(int newCapacity) {
    Slot* oldFirst = first;
    Slot* oldSecond = second;
    int oldCapacity = capacity;

    first = 0;
    second = 0;
    allocateTables(newCapacity);
    currentSize = 0;

    for (int i = 0; i < oldCapacity; ++i) {
        if (oldFirst[i].occupied) {
            insert(oldFirst[i].key, oldFirst[i].value);
        }
        if (oldSecond[i].occupied) {
            insert(oldSecond[i].key, oldSecond[i].value);
        }
    }

    delete[] oldFirst;
    delete[] oldSecond;
}

int CuckooHashTable::nextPrime(int value) const {
    if (value < 2) {
        return 2;
    }
    while (!isPrime(value)) {
        ++value;
    }
    return value;
}

bool CuckooHashTable::isPrime(int value) const {
    if (value < 2) {
        return false;
    }
    if (value == 2) {
        return true;
    }
    if (value % 2 == 0) {
        return false;
    }
    for (int i = 3; i * i <= value; i += 2) {
        if (value % i == 0) {
            return false;
        }
    }
    return true;
}
