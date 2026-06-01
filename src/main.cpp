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
    OpenAddressingHashTable linearTable(OpenAddressingHashTable::Linear);
    OpenAddressingHashTable quadraticTable(OpenAddressingHashTable::Quadratic);
    AVLHashTable avlTable;

    bool running = true;
    while (running) {
        std::cout << "\nMenu glowne\n";
        std::cout << "1. Tablica mieszajaca - adresowanie liniowe\n";
        std::cout << "2. Tablica mieszajaca - adresowanie kwadratowe\n";
        std::cout << "3. Tablica mieszajaca - lancuchowanie drzewami AVL\n";
        std::cout << "4. Badania wydajnosciowe i zapis CSV\n";
        std::cout << "0. Wyjscie\n";

        int choice = readInt("Wybor: ");
        if (choice == 1) {
            tableMenu(linearTable, "hash_liniowa.csv", "hash_liniowa_autosave.csv");
        } else if (choice == 2) {
            tableMenu(quadraticTable, "hash_kwadratowa.csv", "hash_kwadratowa_autosave.csv");
        } else if (choice == 3) {
            tableMenu(avlTable, "hash_avl.csv", "hash_avl_autosave.csv");
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
