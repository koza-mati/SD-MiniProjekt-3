#include "../include/benchmark.hpp"
#include "../include/hash_tables.hpp"

#include <iostream>
#include <limits>
#include <random>
#include <string>

static void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Wczytuje liczbę całkowitą, ponawiając próbę przy błędnym wejściu.
static int readInt(const char* prompt) {
    int value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            return value;
        }
        std::cout << "Niepoprawna liczba. Sprobuj ponownie.\n";
        clearInput();
    }
}

static void autosave(IHashTable& table, const std::string& fileName) {
    if (!table.saveToCSV(fileName)) {
        std::cout << "Nie udalo sie wykonac autosave do pliku " << fileName << "\n";
    }
}

// Wypełnia strukturę losowymi parami (mt19937). Klucze muszą być unikalne, więc
// losujemy aż do osiągnięcia żądanego rozmiaru - powtórzony klucz powoduje jedynie
// aktualizację wartości i nie zwiększa rozmiaru.
static void generateRandom(IHashTable& table, int count) {
    table.clear();

    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<int> keyDistribution(1, count * 10 + 100);
    std::uniform_int_distribution<int> valueDistribution(1, count * 10 + 100);

    while (table.returnSize() < count) {
        table.insert(keyDistribution(generator), valueDistribution(generator));
    }
}

// Menu pojedynczej struktury. Po każdej operacji modyfikującej wykonywany jest
// automatyczny zapis stanu do pliku pomocniczego.
static void tableMenu(IHashTable& table, const std::string& manualFile, const std::string& autosaveFile) {
    bool running = true;
    while (running) {
        std::cout << "\n" << table.name() << "\n";
        std::cout << "1. Dodaj lub zaktualizuj element\n";
        std::cout << "2. Usun element po kluczu\n";
        std::cout << "3. Wyszukaj element po kluczu\n";
        std::cout << "4. Sprawdz rozmiar\n";
        std::cout << "5. Zapisz aktualny stan do CSV\n";
        std::cout << "6. Wczytaj dane z pliku CSV\n";
        std::cout << "7. Wygeneruj losowe dane\n";
        std::cout << "8. Wyczysc strukture\n";
        std::cout << "0. Powrot\n";

        int choice = readInt("Wybor: ");
        if (choice == 1) {
            int key = readInt("Klucz: ");
            int value = readInt("Wartosc: ");
            bool added = table.insert(key, value);
            autosave(table, autosaveFile);
            std::cout << (added ? "Dodano element.\n" : "Zaktualizowano istniejacy klucz.\n");
        } else if (choice == 2) {
            int key = readInt("Klucz do usuniecia: ");
            bool removed = table.remove(key);
            autosave(table, autosaveFile);
            std::cout << (removed ? "Usunieto element.\n" : "Nie znaleziono klucza.\n");
        } else if (choice == 3) {
            int key = readInt("Klucz do wyszukania: ");
            int value = 0;
            if (table.find(key, value)) {
                std::cout << "Znaleziono: key=" << key << ", value=" << value << "\n";
            } else {
                std::cout << "Nie znaleziono klucza.\n";
            }
        } else if (choice == 4) {
            std::cout << "Rozmiar: " << table.returnSize() << "\n";
        } else if (choice == 5) {
            std::cout << (table.saveToCSV(manualFile) ? "Zapisano CSV.\n" : "Blad zapisu CSV.\n");
        } else if (choice == 6) {
            std::string fileName;
            std::cout << "Nazwa pliku CSV: ";
            std::cin >> fileName;
            if (table.loadFromCSV(fileName)) {
                autosave(table, autosaveFile);
                std::cout << "Wczytano dane.\n";
            } else {
                std::cout << "Nie udalo sie wczytac pliku.\n";
            }
        } else if (choice == 7) {
            int count = readInt("Liczba elementow: ");
            if (count < 0) {
                std::cout << "Liczba elementow nie moze byc ujemna.\n";
            } else {
                generateRandom(table, count);
                autosave(table, autosaveFile);
                std::cout << "Wygenerowano losowe dane.\n";
            }
        } else if (choice == 8) {
            table.clear();
            autosave(table, autosaveFile);
            std::cout << "Wyczyszczono strukture.\n";
        } else if (choice == 0) {
            running = false;
        } else {
            std::cout << "Nieznana opcja.\n";
        }
    }
}

int main() {
    // Trzy warianty tablicy mieszającej obsługiwane przez wspólny interfejs:
    // adresowanie otwarte, łańcuchowanie drzewami AVL oraz cuckoo hashing.
    OpenAddressingHashTable openTable;
    AVLHashTable avlTable;
    CuckooHashTable cuckooTable;

    bool running = true;
    while (running) {
        std::cout << "\nMenu glowne\n";
        std::cout << "1. Tablica mieszajaca - adresowanie otwarte\n";
        std::cout << "2. Tablica mieszajaca - lancuchowanie drzewami AVL\n";
        std::cout << "3. Tablica mieszajaca - cuckoo hashing\n";
        std::cout << "4. Badania wydajnosciowe i zapis CSV\n";
        std::cout << "0. Wyjscie\n";

        int choice = readInt("Wybor: ");
        if (choice == 1) {
            tableMenu(openTable, "hash_otwarte.csv", "hash_otwarte_autosave.csv");
        } else if (choice == 2) {
            tableMenu(avlTable, "hash_avl.csv", "hash_avl_autosave.csv");
        } else if (choice == 3) {
            tableMenu(cuckooTable, "hash_cuckoo.csv", "hash_cuckoo_autosave.csv");
        } else if (choice == 4) {
            runBenchmarks();
        } else if (choice == 0) {
            running = false;
        } else {
            std::cout << "Nieznana opcja.\n";
        }
    }

    return 0;
}
