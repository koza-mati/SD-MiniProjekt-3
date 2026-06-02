# SD-MiniProjekt-3

Projekt z kursu Struktury Danych: tablice mieszające w C++. Program udostępnia cztery implementacje tablicy mieszającej, menu konsolowe oraz moduł pomiarów wydajności zapisywanych do plików CSV i TXT.

## Cel projektu

Celem projektu jest:

- implementacja tablic mieszających przechowujących pary `key-value`,
- porównanie czterech sposobów rozwiązywania kolizji,
- przygotowanie jednej implementacji wykorzystującej drzewo AVL,
- obsługa podstawowych operacji przez wspólny interfejs,
- wykonanie pomiarów czasu działania operacji dla różnych rozmiarów danych,
- zapis i odczyt stanu tablicy z plików CSV.

Zaimplementowane operacje:

- `insert(key, value)` - dodaje nowy element albo aktualizuje wartość dla istniejącego klucza,
- `remove(key)` - usuwa element o podanym kluczu,
- `find(key, value)` - wyszukuje element po kluczu i zwraca jego wartość przez referencję,
- `returnSize()` - zwraca liczbę elementów,
- `clear()` - usuwa wszystkie elementy,
- `saveToCSV()` i `loadFromCSV()` - zapis i odczyt danych,
- `generateRandom()` - wypełnienie struktury losowymi danymi z poziomu menu.

## Technologie i ograniczenia

Projekt jest napisany w C++17. Wykorzystuje standardową bibliotekę C++ m.in. do strumieni plikowych, pomiaru czasu, generowania liczb losowych, `std::string`, `std::chrono`, `std::random_device`, `std::mt19937` i `std::uniform_int_distribution`.

Rdzeń struktur danych jest zaimplementowany samodzielnie. Program nie korzysta z gotowych tablic mieszających, takich jak `std::unordered_map`, ani z gotowych drzew, takich jak `std::map` albo `std::set`. Tablice, wpisy, próbkowanie oraz drzewa AVL są zaimplementowane ręcznie z użyciem wskaźników i dynamicznej alokacji pamięci.

Zarówno klucze, jak i wartości są liczbami całkowitymi typu `int`.

## Struktura projektu

Katalog `include/` zawiera:

- `hash_tables.hpp` - wspólny interfejs `IHashTable` oraz deklaracje czterech implementacji tablic mieszających,
- `benchmark.hpp` - deklaracje funkcji benchmarków i funkcji `benchmarkSeed(size, attempt)`.

Katalog `src/` zawiera:

- `main.cpp` - punkt startowy programu, menu konsolowe, obsługa CSV z poziomu programu i generowanie danych losowych,
- `hash_tables.cpp` - implementacje tablic mieszających: adresowanie liniowe, adresowanie kwadratowe, kubełki AVL i cuckoo hashing,
- `benchmark.cpp` - benchmarki operacji `insert` oraz `remove`, zapis wyników do CSV i TXT.

Pliki w katalogu głównym:

- `Makefile` - prosty plik budowania projektu,
- `README.md` - opis projektu,
- `miniprojekt3.exe` - przykładowy artefakt po kompilacji, nie jest wymagany do oceny kodu.

## Wspólny model danych

Dane w tablicach mieszających są reprezentowane przez parę:

- `key` - klucz całkowity, po którym wykonywane jest mieszanie i wyszukiwanie,
- `value` - wartość całkowita przypisana do klucza.

Wspólny interfejs `IHashTable` pozwala obsługiwać wszystkie implementacje w taki sam sposób. Dzięki temu menu i benchmarki nie muszą znać szczegółów konkretnej struktury. Każda tablica implementuje te same metody: `insert`, `remove`, `find`, `returnSize`, `clear`, `saveToCSV`, `loadFromCSV` oraz `name`.

## Funkcja mieszająca i pojemność tablicy

We wszystkich implementacjach indeks początkowy jest wyznaczany przez funkcję:

```text
abs(key) % capacity
```

Pojemność `capacity` oznacza liczbę kubełków lub komórek w aktualnej tablicy. Nie jest tym samym co liczba elementów. Liczbę zapisanych par `key-value` przechowuje `currentSize`, a zwraca ją metoda `returnSize()`.

Pojemność jest zwiększana automatycznie przez `rehash`, gdy tablica zaczyna być zbyt wypełniona. Nowa pojemność jest dobierana jako kolejna liczba pierwsza, co zmniejsza ryzyko niekorzystnego rozkładania się kluczy.

## Implementacja: adresowanie liniowe

`OpenAddressingHashTable` w trybie `Linear` implementuje tablicę mieszającą z adresowaniem otwartym i próbkowaniem liniowym.

Przy kolizji sprawdzane są kolejne komórki:

```text
(hash(key) + i) % capacity
```

Komórka może mieć jeden z trzech stanów:

- `Empty` - komórka nigdy nie była użyta,
- `Occupied` - komórka przechowuje aktywny element,
- `Deleted` - komórka zawierała element, ale został on usunięty.

Stan `Deleted` jest potrzebny, ponieważ przy adresowaniu otwartym usunięcie elementu nie może przerwać łańcucha próbkowania. Gdyby komórka po usunięciu była traktowana jak całkowicie pusta, wyszukiwanie niektórych elementów mogłoby zakończyć się za wcześnie.

Działanie operacji:

- `insert` szuka miejsca liniowo, dodaje nowy klucz albo aktualizuje istniejący,
- `remove` znajduje komórkę i oznacza ją jako `Deleted`,
- `find` przechodzi po sekwencji próbkowania do znalezienia klucza albo pustej komórki,
- `clear` ustawia wszystkie komórki jako `Empty`.

Złożoność średnia:

- `insert` - `O(1)`,
- `remove` - `O(1)`,
- `find` - `O(1)`,
- `returnSize` - `O(1)`,
- `saveToCSV` - `O(n + capacity)`,
- `loadFromCSV` - średnio `O(n)`.

W najgorszym przypadku operacje mogą mieć złożoność `O(n)`, gdy wystąpi wiele kolizji.

## Implementacja: adresowanie kwadratowe

`OpenAddressingHashTable` w trybie `Quadratic` implementuje tablicę mieszającą z adresowaniem otwartym i próbkowaniem kwadratowym.

Przy kolizji sprawdzane są komórki według wzoru:

```text
(hash(key) + i + i * i) % capacity
```

Próbkowanie kwadratowe ogranicza tworzenie długich ciągłych bloków zajętych komórek, które często pojawiają się przy próbkowaniu liniowym. Program utrzymuje niższe wypełnienie tej tablicy, aby sekwencja próbkowania miała większą szansę znaleźć wolną komórkę.

Działanie operacji jest takie samo jak w wariancie liniowym:

- `insert` dodaje lub aktualizuje element,
- `remove` oznacza komórkę jako `Deleted`,
- `find` wyszukuje element po sekwencji próbkowania,
- `rehash` tworzy większą tablicę i wstawia wszystkie aktywne elementy od nowa.

Złożoność średnia:

- `insert` - `O(1)`,
- `remove` - `O(1)`,
- `find` - `O(1)`,
- `returnSize` - `O(1)`,
- `saveToCSV` - `O(n + capacity)`,
- `loadFromCSV` - średnio `O(n)`.

W najgorszym przypadku operacje mogą mieć złożoność `O(n)`.

## Implementacja: kubełki z drzewami AVL

`AVLHashTable` implementuje tablicę mieszającą z osobnymi kubełkami. Każdy kubełek jest niezależnym drzewem AVL. Oznacza to, że funkcja mieszająca wybiera kubełek, a kolizje w tym samym kubełku są obsługiwane przez zbalansowane drzewo binarne.

Każdy węzeł AVL przechowuje:

- `key` - klucz elementu,
- `value` - wartość elementu,
- `height` - wysokość węzła potrzebna do balansowania,
- `left` i `right` - wskaźniki na lewe i prawe poddrzewo.

Drzewo AVL pilnuje różnicy wysokości lewego i prawego poddrzewa. Po operacjach `insert` i `remove` wykonywane są rotacje:

- rotacja w prawo,
- rotacja w lewo,
- rotacja lewo-prawo,
- rotacja prawo-lewo.

Dzięki temu pojedynczy kubełek nie degeneruje się do listy nawet wtedy, gdy wiele kluczy trafi do tego samego indeksu.

Działanie operacji:

- `insert` wybiera kubełek i wstawia element do drzewa AVL,
- `remove` usuwa węzeł z drzewa AVL i przywraca balans,
- `find` wyszukuje klucz w drzewie wybranego kubełka,
- `clear` zwalnia wszystkie węzły we wszystkich kubełkach,
- `rehash` tworzy nową tablicę kubełków i przenosi elementy do nowych drzew.

Złożoność:

- `insert` - średnio `O(log k)` w kubełku, gdzie `k` to liczba elementów w danym kubełku,
- `remove` - średnio `O(log k)`,
- `find` - średnio `O(log k)`,
- `returnSize` - `O(1)`,
- `saveToCSV` - `O(n)`,
- `loadFromCSV` - średnio `O(n log k)`.

Przy dobrym rozkładzie funkcji mieszającej `k` jest małe, więc operacje są bardzo szybkie. Przy dużej liczbie kolizji AVL nadal utrzymuje uporządkowaną i zbalansowaną strukturę.

## Implementacja: cuckoo hashing

`CuckooHashTable` implementuje tablicę mieszającą w schemacie _cuckoo hashing_. Jest to wariant adresowania otwartego o odmiennej zasadzie działania niż próbkowanie liniowe i kwadratowe: zamiast jednej tablicy i sekwencji próbkowania używane są dwie tablice oraz dwie niezależne funkcje mieszające.

Każdy klucz ma dokładnie dwie dozwolone pozycje:

- `hashFirst(key)` - pozycja w pierwszej tablicy,
- `hashSecond(key)` - pozycja w drugiej tablicy.

Dzięki temu operacje `find` i `remove` sprawdzają co najwyżej dwie komórki, niezależnie od wypełnienia struktury, co daje gwarantowany koszt `O(1)` w najgorszym przypadku.

Działanie operacji:

- `insert` umieszcza element w pierwszej tablicy. Jeśli komórka jest zajęta, dotychczasowy element zostaje "wyrzucony" (ang. _kick_) i przeniesiony na swoją alternatywną pozycję w drugiej tablicy. Proces przenoszenia powtarza się naprzemiennie między tablicami aż do znalezienia wolnej komórki,
- jeżeli liczba przeniesień przekroczy ustalony limit (`maxKicks`, rosnący logarytmicznie z pojemnością), oznacza to powstanie cyklu i wykonywany jest `rehash` z większą pojemnością oraz ponowne wstawienie elementu,
- `remove` sprawdza obie pozycje klucza i zwalnia właściwą komórkę,
- `find` sprawdza obie pozycje klucza.

Aby utrzymać skuteczność przenoszeń, struktura jest powiększana przez `rehash`, gdy łączne wypełnienie obu tablic zbliża się do połowy ich pojemności.

Złożoność średnia:

- `insert` - zamortyzowane `O(1)`,
- `remove` - `O(1)` w najgorszym przypadku,
- `find` - `O(1)` w najgorszym przypadku,
- `returnSize` - `O(1)`,
- `saveToCSV` - `O(capacity)`,
- `loadFromCSV` - średnio `O(n)`.

## Menu programu

Po uruchomieniu programu dostępne jest menu główne:

1. Tablica mieszająca - adresowanie liniowe.
2. Tablica mieszająca - adresowanie kwadratowe.
3. Tablica mieszająca - łańcuchowanie drzewami AVL.
4. Tablica mieszająca - cuckoo hashing.
5. Badania wydajnościowe i zapis CSV.
0. Wyjście.

W menu konkretnej struktury można:

- dodać albo zaktualizować element,
- usunąć element po kluczu,
- wyszukać element po kluczu,
- sprawdzić rozmiar,
- zapisać aktualny stan do CSV,
- wczytać dane z pliku CSV,
- wygenerować losowe dane,
- wyczyścić strukturę.

Po operacjach modyfikujących program automatycznie zapisuje stan pomocniczy:

- `hash_liniowa_autosave.csv` dla adresowania liniowego,
- `hash_kwadratowa_autosave.csv` dla adresowania kwadratowego,
- `hash_avl_autosave.csv` dla tablicy z kubełkami AVL,
- `hash_cuckoo_autosave.csv` dla cuckoo hashing.

Ręczny zapis z menu tworzy:

- `hash_liniowa.csv`,
- `hash_kwadratowa.csv`,
- `hash_avl.csv`,
- `hash_cuckoo.csv`.

## Generowanie danych

Funkcja `generateRandom()` używana w menu czyści aktualną strukturę i tworzy podaną liczbę nowych elementów.

Klucze i wartości są losowane z zakresu:

```text
1 .. count * 10 + 100
```

Do generowania używane są `std::random_device`, `std::mt19937` i `std::uniform_int_distribution`.

Ponieważ klucz w tablicy mieszającej musi być unikalny, program losuje kolejne pary do momentu, aż struktura osiągnie zadany rozmiar. Jeśli wylosowany klucz już istnieje, operacja `insert` aktualizuje wartość, ale rozmiar się nie zwiększa, więc losowanie trwa dalej.

## Zapis i odczyt CSV

Wszystkie implementacje zapisują pliki w formacie:

```csv
key,value
10,500
7,120
15,450
```

Metoda `loadFromCSV()` odczytuje kolejne rekordy `key,value`, a następnie odbudowuje strukturę przez `insert`. Oznacza to, że dane po wczytaniu są poprawne logicznie, ale fizyczne rozmieszczenie elementów w tablicy może być inne niż przed zapisem, ponieważ zależy od aktualnej pojemności tablicy i procesu `rehash`.

## Badania wydajnościowe

Benchmarki są uruchamiane z menu głównego przez opcję `Badania wydajnościowe i zapis CSV`.

Mierzone operacje:

- `insert`,
- `remove`.

Rozmiary struktur używane w benchmarkach:

```text
10000, 20000, 40000, 80000, 100000, 160000, 320000, 640000
```

Dla każdego rozmiaru każda operacja jest mierzona 100 razy. Przed pojedynczym pomiarem tworzona jest nowa struktura wypełniona tym samym zestawem danych dla danej próby. Dane testowe są generowane deterministycznie na podstawie funkcji `benchmarkSeed(size, attempt)`.

Pliki wynikowe:

- `pomiary.txt` - zbiorcze zestawienie wyników,
- `benchmark_liniowa.csv` - wyniki dla adresowania liniowego,
- `benchmark_kwadratowa.csv` - wyniki dla adresowania kwadratowego,
- `benchmark_avl.csv` - wyniki dla tablicy z kubełkami AVL,
- `benchmark_cuckoo.csv` - wyniki dla cuckoo hashing,
- `seedy_100000.txt` - lista seedów dla rozmiaru 100000.

Format plików CSV z benchmarkami:

```csv
Operation,Size,AverageTime_ns
insert,10000,123
remove,10000,456
```

## Kompilacja

Przykładowa komenda kompilacji:

```bash
g++ -std=c++17 -Wall -Wextra -O2 src/main.cpp src/hash_tables.cpp src/benchmark.cpp -o miniprojekt3.exe
```

W środowisku MSYS2/MinGW można użyć:

```bash
/mingw64/bin/g++ -std=c++17 -Wall -Wextra -O2 src/main.cpp src/hash_tables.cpp src/benchmark.cpp -o miniprojekt3.exe
```

Jeśli dostępny jest `make`, projekt można zbudować tak:

```bash
make
```

## Uruchomienie

Windows:

```bash
miniprojekt3.exe
```

Git Bash / MSYS / podobne środowisko:

```bash
./miniprojekt3.exe
```

## Podsumowanie

Projekt realizuje cztery warianty tablic mieszających: adresowanie liniowe, adresowanie kwadratowe, tablice z kubełkami opartymi o drzewa AVL oraz cuckoo hashing. Zawiera menu konsolowe, losowe generowanie danych, zapis i odczyt CSV oraz benchmarki porównujące koszty operacji `insert` i `remove` dla dużych zestawów danych.
