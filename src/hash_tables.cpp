#include "../include/hash_tables.hpp"

#include <cstdlib>
#include <sstream>

OpenAddressingHashTable::OpenAddressingHashTable(ProbeMode mode, int initialCapacity)
    : probeMode(mode), table(0), capacity(0), currentSize(0), deletedCount(0) {
    allocateTable(nextPrime(initialCapacity));
}

OpenAddressingHashTable::~OpenAddressingHashTable() {
    delete[] table;
}

bool OpenAddressingHashTable::insert(int key, int value) {
    if (shouldGrow()) {
        rehash(nextPrime(capacity * 2 + 1));
    }

    int baseIndex = hash(key);
    int firstDeleted = -1;

    for (int attempt = 0; attempt < capacity; ++attempt) {
        int index = probeIndex(baseIndex, attempt);
        if (table[index].state == Occupied && table[index].key == key) {
            table[index].value = value;
            return false;
        }
        if (table[index].state == Deleted && firstDeleted == -1) {
            firstDeleted = index;
        }
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

    rehash(nextPrime(capacity * 2 + 1));
    return insert(key, value);
}

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
    std::getline(file, line);
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
        if (table[index].state == Empty) {
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
    std::getline(file, line);
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
        node->value = value;
        return node;
    }

    node->height = 1 + maxInt(height(node->left), height(node->right));
    int nodeBalance = balance(node);

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
