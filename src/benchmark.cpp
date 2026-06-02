#include "../include/benchmark.hpp"

#include "../include/hash_tables.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>

static const int ATTEMPTS = 100;   // liczba prób (różnych ziaren) dla każdego rozmiaru
static const int SIZES_COUNT = 8;
// Rozmiary struktur badane w pomiarach.
static const int BENCHMARK_SIZES[SIZES_COUNT] = {10000, 20000, 40000, 80000, 100000, 160000, 320000, 640000};

// Deterministyczne ziarno zależne od rozmiaru i numeru próby. Dzięki temu każda
// próba korzysta z innego, lecz powtarzalnego zestawu danych (skrót FNV-1a).
unsigned int benchmarkSeed(int size, int attempt) {
    unsigned int seed = 2166136261u;
    seed ^= static_cast<unsigned int>(size);
    seed *= 16777619u;
    seed ^= static_cast<unsigned int>(attempt + 1) * 2654435761u;
    seed *= 16777619u;
    return seed;
}

// Generuje zestaw unikalnych kluczy 1..count z losowymi wartościami, a następnie
// tasuje rekordy (algorytm Fishera-Yatesa). Tasowanie zapewnia, że badane klucze
// nie znajdują się zawsze na początku struktury - przeciwdziała to pozornemu O(1).
static void generateData(HashRecord* data, int count, unsigned int seed) {
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> valueDistribution(1, count * 10 + 100);

    for (int i = 0; i < count; ++i) {
        data[i].key = i + 1;
        data[i].value = valueDistribution(generator);
    }

    for (int i = count - 1; i > 0; --i) {
        std::uniform_int_distribution<int> indexDistribution(0, i);
        int swapIndex = indexDistribution(generator);
        HashRecord temporary = data[i];
        data[i] = data[swapIndex];
        data[swapIndex] = temporary;
    }
}

// Tworzy tablicę mieszającą wskazanego typu.
static IHashTable* createTable(int tableType) {
    if (tableType == 0) {
        return new OpenAddressingHashTable(OpenAddressingHashTable::Linear);
    }
    if (tableType == 1) {
        return new OpenAddressingHashTable(OpenAddressingHashTable::Quadratic);
    }
    if (tableType == 2) {
        return new AVLHashTable();
    }
    return new CuckooHashTable();
}

// Nazwa pliku CSV z wynikami dla danego typu tablicy.
static const char* benchmarkFileName(int tableType) {
    if (tableType == 0) {
        return "benchmark_liniowa.csv";
    }
    if (tableType == 1) {
        return "benchmark_kwadratowa.csv";
    }
    if (tableType == 2) {
        return "benchmark_avl.csv";
    }
    return "benchmark_cuckoo.csv";
}

// Czytelna nazwa typu tablicy używana w zestawieniu zbiorczym.
static const char* benchmarkTableName(int tableType) {
    if (tableType == 0) {
        return "Adresowanie liniowe";
    }
    if (tableType == 1) {
        return "Adresowanie kwadratowe";
    }
    if (tableType == 2) {
        return "Lancuchowanie AVL";
    }
    return "Cuckoo hashing";
}

// Mierzy czas pojedynczej operacji insert na strukturze zbudowanej wcześniej
// z size elementów. Mierzona jest wyłącznie jedna operacja, aby struktura nie
// rozrastała się w trakcie pomiaru i nie zafałszowała wyniku.
static long long measureInsert(int tableType, HashRecord* data, int size) {
    IHashTable* table = createTable(tableType);
    for (int i = 0; i < size; ++i) {
        table->insert(data[i].key, data[i].value);
    }

    // Element data[size] ma unikalny klucz, więc jest to rzeczywiste wstawienie.
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    table->insert(data[size].key, data[size].value);
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    delete table;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

// Mierzy czas pojedynczej operacji remove na strukturze zbudowanej z size elementów.
// Usuwany jest klucz z losowej (po tasowaniu) pozycji, a nie zawsze ten sam.
static long long measureRemove(int tableType, HashRecord* data, int size) {
    IHashTable* table = createTable(tableType);
    for (int i = 0; i < size; ++i) {
        table->insert(data[i].key, data[i].value);
    }

    int keyToRemove = data[size / 2].key;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    table->remove(keyToRemove);
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    delete table;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}

// Zapisuje listę ziaren użytych dla rozmiaru 100000, co umożliwia odtworzenie pomiarów.
static void saveSeedsFor100000() {
    std::ofstream file("seedy_100000.txt");
    for (int attempt = 0; attempt < ATTEMPTS; ++attempt) {
        file << attempt << "," << benchmarkSeed(100000, attempt) << "\n";
    }
}

// Uruchamia pełen zestaw pomiarów dla wszystkich typów tablic i rozmiarów.
// Dla każdego rozmiaru każda operacja jest mierzona ATTEMPTS razy (różne ziarna),
// a wynik to średnia z tych prób.
void runBenchmarks() {
    std::ofstream summary("pomiary.txt");
    if (!summary.is_open()) {
        std::cout << "Nie mozna utworzyc pliku pomiary.txt\n";
        return;
    }

    saveSeedsFor100000();

    for (int tableType = 0; tableType < 4; ++tableType) {
        std::ofstream csv(benchmarkFileName(tableType));
        csv << "Operation,Size,AverageTime_ns\n";
        summary << benchmarkTableName(tableType) << "\n";

        for (int sizeIndex = 0; sizeIndex < SIZES_COUNT; ++sizeIndex) {
            int size = BENCHMARK_SIZES[sizeIndex];
            long long insertTotal = 0;
            long long removeTotal = 0;

            std::cout << "Benchmark: " << benchmarkTableName(tableType) << ", size=" << size << "\n";

            for (int attempt = 0; attempt < ATTEMPTS; ++attempt) {
                // Dla każdej próby budujemy strukturę od nowa z osobnego zestawu danych.
                HashRecord* data = new HashRecord[size + 1];
                generateData(data, size + 1, benchmarkSeed(size, attempt));
                insertTotal += measureInsert(tableType, data, size);
                removeTotal += measureRemove(tableType, data, size);
                delete[] data;
            }

            // Średni czas operacji w nanosekundach.
            long long insertAverage = insertTotal / ATTEMPTS;
            long long removeAverage = removeTotal / ATTEMPTS;

            csv << "insert," << size << "," << insertAverage << "\n";
            csv << "remove," << size << "," << removeAverage << "\n";
            summary << "size=" << size << ", insert=" << insertAverage
                    << " ns, remove=" << removeAverage << " ns\n";
        }

        summary << "\n";
    }

    std::cout << "Zapisano: pomiary.txt, benchmark_liniowa.csv, benchmark_kwadratowa.csv, benchmark_avl.csv, benchmark_cuckoo.csv, seedy_100000.txt\n";
}
