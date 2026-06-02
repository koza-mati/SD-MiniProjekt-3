#include "../include/hash_tables.hpp"

#include <cstdlib>
#include <sstream>


// Konstruktor alokuje tablicę o pojemności będącej najbliższą liczbą pierwszą.
OpenAddressingHashTable::OpenAddressingHashTable(ProbeMode mode, int initialCapacity)
    : probeMode(mode), table(0), capacity(0), currentSize(0), deletedCount(0) {
    allocateTable(nextPrime(initialCapacity));
}

OpenAddressingHashTable::~OpenAddressingHashTable() {
    delete[] table;
}

// Wstawia nową parę klucz-wartość albo aktualizuje wartość istniejącego klucza.
// Zwraca true, gdy dodano nowy element, false, gdy nastąpiła aktualizacja.
bool OpenAddressingHashTable::insert(int key, int value) {
    // Powiększenie tablicy, zanim współczynnik wypełnienia stanie się zbyt duży.
    if (shouldGrow()) {
        rehash(nextPrime(capacity * 2 + 1));
    }

    int baseIndex = hash(key);
    int firstDeleted = -1;   // pierwszy napotkany nagrobek, w który można wstawić element

    for (int attempt = 0; attempt < capacity; ++attempt) {
        int index = probeIndex(baseIndex, attempt);
        // Klucz już istnieje - aktualizujemy wartość.
        if (table[index].state == Occupied && table[index].key == key) {
            table[index].value = value;
            return false;
        }
        // Zapamiętujemy pierwszy nagrobek na ścieżce próbkowania.
        if (table[index].state == Deleted && firstDeleted == -1) {
            firstDeleted = index;
        }
        // Pusta komórka oznacza koniec ścieżki - klucza nie ma w tablicy.
        if (table[index].state == Empty) {
            // Preferujemy ponowne wykorzystanie nagrobka, jeśli był wcześniej napotkany.
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

    // Cała tablica przejrzana - jeśli był nagrobek, wykorzystujemy go.
    if (firstDeleted != -1) {
        table[firstDeleted].key = key;
        table[firstDeleted].value = value;
        table[firstDeleted].state = Occupied;
        --deletedCount;
        ++currentSize;
        return true;
    }

    // Brak miejsca - powiększamy tablicę i ponawiamy wstawianie.
    rehash(nextPrime(capacity * 2 + 1));
    return insert(key, value);
}

// Usuwa parę o danym kluczu. Komórka jest oznaczana jako nagrobek (Deleted),
// aby nie przerwać ścieżki próbkowania innych kluczy.
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

// Czyści tablicę, ustawiając wszystkie komórki jako puste.
void OpenAddressingHashTable::clear() {
    for (int i = 0; i < capacity; ++i) {
        table[i].state = Empty;
    }
    currentSize = 0;
    deletedCount = 0;
}

// Zapisuje wyłącznie aktywne komórki w formacie key,value.
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

// Odczytuje rekordy z pliku i odbudowuje tablicę przez kolejne wstawienia.
bool OpenAddressingHashTable::loadFromCSV(const std::string& fileName) {
    std::ifstream file(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }

    clear();
    std::string line;
    std::getline(file, line);   // pomijamy nagłówek
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

// Funkcja mieszająca: wartość bezwzględna klucza modulo pojemność tablicy.
int OpenAddressingHashTable::hash(int key) const {
    long long normalized = key;
    if (normalized < 0) {
        normalized = -normalized;
    }
    return static_cast<int>(normalized % capacity);
}

// Wyznacza indeks komórki dla kolejnej próby zgodnie z trybem próbkowania.
int OpenAddressingHashTable::probeIndex(int baseIndex, int attempt) const {
    if (probeMode == Linear) {
        return (baseIndex + attempt) % capacity;
    }
    return (baseIndex + attempt + attempt * attempt) % capacity;
}

// Sprawdza, czy tablica powinna zostać powiększona. Pod uwagę brane są zarówno
// aktywne elementy, jak i nagrobki. Próbkowanie kwadratowe wymaga niższego
// wypełnienia, aby ścieżka próbkowania nie pomijała wolnych komórek.
bool OpenAddressingHashTable::shouldGrow() const {
    int used = currentSize + deletedCount;
    int limit = probeMode == Quadratic ? capacity / 2 : capacity * 7 / 10;
    return used + 1 >= limit;
}

// Alokuje nową tablicę o podanej pojemności i inicjalizuje wszystkie komórki.
void OpenAddressingHashTable::allocateTable(int newCapacity) {
    capacity = newCapacity;
    table = new Entry[capacity];
    for (int i = 0; i < capacity; ++i) {
        table[i].key = 0;
        table[i].value = 0;
        table[i].state = Empty;
    }
}

// Przebudowuje tablicę: tworzy większą tablicę i wstawia do niej ponownie
// wszystkie aktywne elementy, pomijając nagrobki.
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

// Wyszukuje indeks komórki przechowującej aktywny klucz lub -1, gdy go brak.
int OpenAddressingHashTable::findSlot(int key) const {
    int baseIndex = hash(key);
    for (int attempt = 0; attempt < capacity; ++attempt) {
        int index = probeIndex(baseIndex, attempt);
        // Pusta komórka oznacza, że klucz nie występuje dalej na ścieżce.
        if (table[index].state == Empty) {
            return -1;
        }
        if (table[index].state == Occupied && table[index].key == key) {
            return index;
        }
    }
    return -1;
}

// Zwraca najbliższą liczbę pierwszą większą lub równą podanej wartości.
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
//  Tablica mieszająca z łańcuchowaniem kubełkami w postaci drzew AVL
// ===========================================================================

// Konstruktor alokuje tablicę pustych kubełków o pojemności będącej liczbą pierwszą.
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

// Wstawia parę do drzewa AVL wybranego kubełka.
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

// Usuwa węzeł o danym kluczu z drzewa AVL odpowiedniego kubełka.
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

// Zwalnia wszystkie węzły we wszystkich kubełkach.
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
    std::getline(file, line);   // pomijamy nagłówek
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

// Funkcja mieszająca wybierająca kubełek dla danego klucza.
int AVLHashTable::hash(int key) const {
    long long normalized = key;
    if (normalized < 0) {
        normalized = -normalized;
    }
    return static_cast<int>(normalized % capacity);
}

// Wysokość węzła; dla pustego poddrzewa przyjmujemy 0.
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

// Rotacja w prawo - przywraca równowagę przy przeciążeniu lewego poddrzewa.
AVLHashTable::Node* AVLHashTable::rotateRight(Node* node) {
    Node* leftChild = node->left;
    Node* middle = leftChild->right;

    leftChild->right = node;
    node->left = middle;

    node->height = maxInt(height(node->left), height(node->right)) + 1;
    leftChild->height = maxInt(height(leftChild->left), height(leftChild->right)) + 1;
    return leftChild;
}

// Rotacja w lewo - przywraca równowagę przy przeciążeniu prawego poddrzewa.
AVLHashTable::Node* AVLHashTable::rotateLeft(Node* node) {
    Node* rightChild = node->right;
    Node* middle = rightChild->left;

    rightChild->left = node;
    node->right = middle;

    node->height = maxInt(height(node->left), height(node->right)) + 1;
    rightChild->height = maxInt(height(rightChild->left), height(rightChild->right)) + 1;
    return rightChild;
}

// Rekurencyjnie wstawia węzeł, a następnie przywraca równowagę drzewa AVL.
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
        // Klucz już istnieje - aktualizujemy wartość bez dodawania węzła.
        node->value = value;
        return node;
    }

    node->height = 1 + maxInt(height(node->left), height(node->right));
    int nodeBalance = balance(node);

    // Cztery przypadki niezrównoważenia i odpowiadające im rotacje.
    if (nodeBalance > 1 && key < node->left->key) {          // przypadek lewo-lewo
        return rotateRight(node);
    }
    if (nodeBalance < -1 && key > node->right->key) {        // przypadek prawo-prawo
        return rotateLeft(node);
    }
    if (nodeBalance > 1 && key > node->left->key) {          // przypadek lewo-prawo
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    if (nodeBalance < -1 && key < node->right->key) {        // przypadek prawo-lewo
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

// Rekurencyjnie usuwa węzeł i przywraca równowagę drzewa AVL.
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
        // Węzeł ma co najwyżej jedno dziecko - zastępujemy go tym dzieckiem.
        if (!node->left || !node->right) {
            Node* child = node->left ? node->left : node->right;
            delete node;
            return child;
        }

        // Węzeł ma dwoje dzieci - zastępujemy go następnikiem (najmniejszy klucz
        // w prawym poddrzewie), a następnie usuwamy następnik z prawego poddrzewa.
        Node* successor = minNode(node->right);
        node->key = successor->key;
        node->value = successor->value;
        bool ignored = false;
        node->right = removeNode(node->right, successor->key, ignored);
    }

    node->height = 1 + maxInt(height(node->left), height(node->right));
    int nodeBalance = balance(node);

    // Przywrócenie równowagi po usunięciu - cztery możliwe przypadki.
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

// Zwraca węzeł o najmniejszym kluczu w poddrzewie (skrajnie lewy węzeł).
AVLHashTable::Node* AVLHashTable::minNode(Node* node) const {
    Node* current = node;
    while (current && current->left) {
        current = current->left;
    }
    return current;
}

// Iteracyjne wyszukiwanie klucza w drzewie binarnym wyszukiwań.
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

// Rekurencyjnie zwalnia węzły poddrzewa (przejście postorder).
void AVLHashTable::clearNode(Node* node) {
    if (!node) {
        return;
    }
    clearNode(node->left);
    clearNode(node->right);
    delete node;
}

// Zapisuje węzły w porządku in-order (rosnąco według klucza).
void AVLHashTable::saveNode(Node* node, std::ofstream& file) const {
    if (!node) {
        return;
    }
    saveNode(node->left, file);
    file << node->key << "," << node->value << "\n";
    saveNode(node->right, file);
}

// Przenosi wszystkie elementy poddrzewa do innej tablicy mieszającej.
void AVLHashTable::collectAndInsert(Node* node, AVLHashTable& target) const {
    if (!node) {
        return;
    }
    collectAndInsert(node->left, target);
    target.insert(node->key, node->value);
    collectAndInsert(node->right, target);
}

// Przebudowa: tworzy nową tablicę kubełków i przenosi do niej wszystkie elementy,
// a następnie przejmuje jej zasoby (technika kopiowania i zamiany wskaźników).
void AVLHashTable::rehash(int newCapacity) {
    AVLHashTable newTable(newCapacity);
    for (int i = 0; i < capacity; ++i) {
        collectAndInsert(buckets[i], newTable);
    }

    // Zwalniamy stare drzewa i tablicę kubełków.
    for (int i = 0; i < capacity; ++i) {
        clearNode(buckets[i]);
    }
    delete[] buckets;

    // Przejmujemy zasoby tymczasowej tablicy.
    buckets = newTable.buckets;
    capacity = newTable.capacity;
    currentSize = newTable.currentSize;
    newTable.buckets = 0;
    newTable.capacity = 0;
    newTable.currentSize = 0;
}

// Próg powiększenia: średnio około czterech elementów na kubełek.
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
//  Tablica mieszająca w schemacie cuckoo hashing (dwie tablice, dwie funkcje)
// ===========================================================================

// Konstruktor alokuje obie tablice o pojemności będącej liczbą pierwszą.
CuckooHashTable::CuckooHashTable(int initialCapacity)
    : first(0), second(0), capacity(0), currentSize(0) {
    allocateTables(nextPrime(initialCapacity));
}

CuckooHashTable::~CuckooHashTable() {
    delete[] first;
    delete[] second;
}

// Wstawia parę klucz-wartość. Jeśli klucz już istnieje, aktualizuje wartość.
// W przeciwnym razie umieszcza element, w razie potrzeby przenosząc (wyrzucając)
// kolejne elementy między tablicami zgodnie z zasadą cuckoo hashing.
bool CuckooHashTable::insert(int key, int value) {
    // Aktualizacja istniejącego klucza nie zmienia rozmiaru struktury.
    if (updateExisting(key, value)) {
        return false;
    }

    // Utrzymujemy współczynnik wypełnienia poniżej połowy, aby przenoszenia działały.
    if (shouldGrow()) {
        rehash(nextPrime(capacity * 2 + 1));
    }

    int currentKey = key;
    int currentValue = value;
    int whichTable = 0;          // 0 - tablica first, 1 - tablica second
    int limit = maxKicks();

    for (int kick = 0; kick < limit; ++kick) {
        if (whichTable == 0) {
            int index = hashFirst(currentKey);
            // Wolna komórka - umieszczamy element i kończymy.
            if (!first[index].occupied) {
                first[index].key = currentKey;
                first[index].value = currentValue;
                first[index].occupied = true;
                ++currentSize;
                return true;
            }
            // Komórka zajęta - wyrzucamy dotychczasowy element i bierzemy go "do ręki".
            int evictedKey = first[index].key;
            int evictedValue = first[index].value;
            first[index].key = currentKey;
            first[index].value = currentValue;
            currentKey = evictedKey;
            currentValue = evictedValue;
            whichTable = 1;   // wyrzucony element trafia do drugiej tablicy
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
            whichTable = 0;   // wyrzucony element trafia do pierwszej tablicy
        }
    }

    // Przekroczenie limitu przeniesień oznacza cykl - powiększamy strukturę
    // i ponawiamy wstawianie trzymanego w ręku elementu.
    rehash(nextPrime(capacity * 2 + 1));
    return insert(currentKey, currentValue);
}

// Usuwa klucz, sprawdzając obie jego dozwolone pozycje.
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

// Wyszukuje klucz - wystarczy sprawdzić dwie pozycje, stąd koszt O(1).
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

// Czyści obie tablice, oznaczając wszystkie komórki jako wolne.
void CuckooHashTable::clear() {
    for (int i = 0; i < capacity; ++i) {
        first[i].occupied = false;
        second[i].occupied = false;
    }
    currentSize = 0;
}

// Zapisuje aktywne komórki z obu tablic w formacie key,value.
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
    std::getline(file, line);   // pomijamy nagłówek
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

// Pierwsza funkcja mieszająca - mieszanie multiplikatywne dające pozycję w tablicy first.
int CuckooHashTable::hashFirst(int key) const {
    unsigned int mixed = static_cast<unsigned int>(key);
    mixed *= 2654435761u;
    return static_cast<int>(mixed % static_cast<unsigned int>(capacity));
}

// Druga, niezależna funkcja mieszająca dająca pozycję w tablicy second.
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

// Limit przeniesień rosnący logarytmicznie z pojemnością. Jego przekroczenie
// traktujemy jako wystąpienie cyklu wymagającego przebudowy struktury.
int CuckooHashTable::maxKicks() const {
    int limit = 8;
    int value = capacity;
    while (value > 1) {
        value /= 2;
        ++limit;
    }
    return limit;
}

// Jeśli klucz znajduje się na jednej z dwóch dozwolonych pozycji, aktualizuje
// jego wartość i zwraca true; w przeciwnym razie zwraca false.
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

// Alokuje obie tablice o podanej pojemności i oznacza wszystkie komórki jako wolne.
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

// Przebudowuje strukturę: tworzy większe tablice i ponownie wstawia do nich
// wszystkie aktywne elementy z obu dotychczasowych tablic.
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
