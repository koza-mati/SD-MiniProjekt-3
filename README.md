# SD-MiniProjekt-3

Projekt z kursu Struktury Danych: tablice mieszajace w C++. Program udostepnia trzy implementacje tablicy mieszajacej, menu konsolowe oraz modul pomiarow wydajnosci zapisywanych do plikow CSV i TXT.

## Cel projektu

Celem projektu jest:

- implementacja tablic mieszajacych przechowujacych pary `key-value`,
- porownanie trzech sposobow rozwiazywania kolizji,
- przygotowanie jednej implementacji wykorzystujacej drzewo AVL,
- obsluga podstawowych operacji przez wspolny interfejs,
- wykonanie pomiarow czasu dzialania operacji dla roznych rozmiarow danych,
- zapis i odczyt stanu tablicy z plikow CSV.

Zaimplementowane operacje:

- `insert(key, value)` - dodaje nowy element albo aktualizuje wartosc dla istniejacego klucza,
- `remove(key)` - usuwa element o podanym kluczu,
- `find(key, value)` - wyszukuje element po kluczu i zwraca jego wartosc przez referencje,
- `returnSize()` - zwraca liczbe elementow,
- `clear()` - usuwa wszystkie elementy,
- `saveToCSV()` i `loadFromCSV()` - zapis i odczyt danych,
- `generateRandom()` - wypelnienie struktury losowymi danymi z poziomu menu.

## Technologie i ograniczenia

Projekt jest napisany w C++17. Wykorzystuje standardowa biblioteke C++ m.in. do strumieni plikowych, pomiaru czasu, generowania liczb losowych, `std::string`, `std::chrono`, `std::random_device`, `std::mt19937` i `std::uniform_int_distribution`.

Rdzen struktur danych jest zaimplementowany samodzielnie. Program nie korzysta z gotowych tablic mieszajacych, takich jak `std::unordered_map`, ani z gotowych drzew, takich jak `std::map` albo `std::set`. Tablice, wpisy, probkowanie oraz drzewa AVL sa zaimplementowane recznie z uzyciem wskaznikow i dynamicznej alokacji pamieci.

Zarowno klucze, jak i wartosci sa liczbami calkowitymi typu `int`.

## Struktura projektu

Katalog `include/` zawiera:

- `hash_tables.hpp` - wspolny interfejs `IHashTable` oraz deklaracje trzech implementacji tablic mieszajacych,
- `benchmark.hpp` - deklaracje funkcji benchmarkow i funkcji `benchmarkSeed(size, attempt)`.

Katalog `src/` zawiera:

- `main.cpp` - punkt startowy programu, menu konsolowe, obsluga CSV z poziomu programu i generowanie danych losowych,
- `hash_tables.cpp` - implementacje tablic mieszajacych: adresowanie liniowe, adresowanie kwadratowe i kubelki AVL,
- `benchmark.cpp` - benchmarki operacji `insert` oraz `remove`, zapis wynikow do CSV i TXT.

Pliki w katalogu glownym:

- `Makefile` - prosty plik budowania projektu,
- `README.md` - opis projektu,
- `miniprojekt3.exe` - przykladowy artefakt po kompilacji, nie jest wymagany do oceny kodu.

## Wspolny model danych

Dane w tablicach mieszajacych sa reprezentowane przez pare:

- `key` - klucz calkowity, po ktorym wykonywane jest mieszanie i wyszukiwanie,
- `value` - wartosc calkowita przypisana do klucza.

Wspolny interfejs `IHashTable` pozwala obslugiwac wszystkie implementacje w taki sam sposob. Dzieki temu menu i benchmarki nie musza znac szczegolow konkretnej struktury. Kazda tablica implementuje te same metody: `insert`, `remove`, `find`, `returnSize`, `clear`, `saveToCSV`, `loadFromCSV` oraz `name`.

## Funkcja mieszajaca i pojemnosc tablicy

We wszystkich implementacjach indeks poczatkowy jest wyznaczany przez funkcje:

```text
abs(key) % capacity
```

Pojemnosc `capacity` oznacza liczbe kubelkow lub komorek w aktualnej tablicy. Nie jest tym samym co liczba elementow. Liczbe zapisanych par `key-value` przechowuje `currentSize`, a zwraca ja metoda `returnSize()`.

Pojemnosc jest zwiekszana automatycznie przez `rehash`, gdy tablica zaczyna byc zbyt wypelniona. Nowa pojemnosc jest dobierana jako kolejna liczba pierwsza, co zmniejsza ryzyko niekorzystnego rozkladania sie kluczy.

## Implementacja: adresowanie liniowe

`OpenAddressingHashTable` w trybie `Linear` implementuje tablice mieszajaca z adresowaniem otwartym i probkowaniem liniowym.

Przy kolizji sprawdzane sa kolejne komorki:

```text
(hash(key) + i) % capacity
```

Komorka moze miec jeden z trzech stanow:

- `Empty` - komorka nigdy nie byla uzyta,
- `Occupied` - komorka przechowuje aktywny element,
- `Deleted` - komorka zawierala element, ale zostal on usuniety.

Stan `Deleted` jest potrzebny, poniewaz przy adresowaniu otwartym usuniecie elementu nie moze przerwac lancucha probkowania. Gdyby komorka po usunieciu byla traktowana jak calkowicie pusta, wyszukiwanie niektorych elementow mogloby zakonczyc sie za wczesnie.

Dzialanie operacji:

- `insert` szuka miejsca liniowo, dodaje nowy klucz albo aktualizuje istniejacy,
- `remove` znajduje komorke i oznacza ja jako `Deleted`,
- `find` przechodzi po sekwencji probkowania do znalezienia klucza albo pustej komorki,
- `clear` ustawia wszystkie komorki jako `Empty`.

Zlozonosc srednia:

- `insert` - `O(1)`,
- `remove` - `O(1)`,
- `find` - `O(1)`,
- `returnSize` - `O(1)`,
- `saveToCSV` - `O(n + capacity)`,
- `loadFromCSV` - srednio `O(n)`.

W najgorszym przypadku operacje moga miec zlozonosc `O(n)`, gdy wystapi wiele kolizji.

## Implementacja: adresowanie kwadratowe

`OpenAddressingHashTable` w trybie `Quadratic` implementuje tablice mieszajaca z adresowaniem otwartym i probkowaniem kwadratowym.

Przy kolizji sprawdzane sa komorki wedlug wzoru:

```text
(hash(key) + i + i * i) % capacity
```

Probowanie kwadratowe ogranicza tworzenie dlugich ciaglych blokow zajetych komorek, ktore czesto pojawiaja sie przy probkowaniu liniowym. Program utrzymuje nizsze wypelnienie tej tablicy, aby sekwencja probkowania miala wieksza szanse znalezc wolna komorke.

Dzialanie operacji jest takie samo jak w wariancie liniowym:

- `insert` dodaje lub aktualizuje element,
- `remove` oznacza komorke jako `Deleted`,
- `find` wyszukuje element po sekwencji probkowania,
- `rehash` tworzy wieksza tablice i wstawia wszystkie aktywne elementy od nowa.

Zlozonosc srednia:

- `insert` - `O(1)`,
- `remove` - `O(1)`,
- `find` - `O(1)`,
- `returnSize` - `O(1)`,
- `saveToCSV` - `O(n + capacity)`,
- `loadFromCSV` - srednio `O(n)`.

W najgorszym przypadku operacje moga miec zlozonosc `O(n)`.

## Implementacja: kubelki z drzewami AVL

`AVLHashTable` implementuje tablice mieszajaca z osobnymi kubelkami. Kazdy kubelek jest niezaleznym drzewem AVL. Oznacza to, ze funkcja mieszajaca wybiera kubelek, a kolizje w tym samym kubelku sa obslugiwane przez zbalansowane drzewo binarne.

Kazdy wezel AVL przechowuje:

- `key` - klucz elementu,
- `value` - wartosc elementu,
- `height` - wysokosc wezla potrzebna do balansowania,
- `left` i `right` - wskazniki na lewe i prawe poddrzewo.

Drzewo AVL pilnuje roznicy wysokosci lewego i prawego poddrzewa. Po operacjach `insert` i `remove` wykonywane sa rotacje:

- rotacja w prawo,
- rotacja w lewo,
- rotacja lewo-prawo,
- rotacja prawo-lewo.

Dzieki temu pojedynczy kubelek nie degeneruje sie do listy nawet wtedy, gdy wiele kluczy trafi do tego samego indeksu.

Dzialanie operacji:

- `insert` wybiera kubelek i wstawia element do drzewa AVL,
- `remove` usuwa wezel z drzewa AVL i przywraca balans,
- `find` wyszukuje klucz w drzewie wybranego kubelka,
- `clear` zwalnia wszystkie wezly we wszystkich kubelkach,
- `rehash` tworzy nowa tablice kubelkow i przenosi elementy do nowych drzew.

Zlozonosc:

- `insert` - srednio `O(log k)` w kubelku, gdzie `k` to liczba elementow w danym kubelku,
- `remove` - srednio `O(log k)`,
- `find` - srednio `O(log k)`,
- `returnSize` - `O(1)`,
- `saveToCSV` - `O(n)`,
- `loadFromCSV` - srednio `O(n log k)`.

Przy dobrym rozkladzie funkcji mieszajacej `k` jest male, wiec operacje sa bardzo szybkie. Przy duzej liczbie kolizji AVL nadal utrzymuje uporzadkowana i zbalansowana strukture.

## Menu programu

Po uruchomieniu programu dostepne jest menu glowne:

1. Tablica mieszajaca - adresowanie liniowe.
2. Tablica mieszajaca - adresowanie kwadratowe.
3. Tablica mieszajaca - lancuchowanie drzewami AVL.
4. Badania wydajnosciowe i zapis CSV.
0. Wyjscie.

W menu konkretnej struktury mozna:

- dodac albo zaktualizowac element,
- usunac element po kluczu,
- wyszukac element po kluczu,
- sprawdzic rozmiar,
- zapisac aktualny stan do CSV,
- wczytac dane z pliku CSV,
- wygenerowac losowe dane,
- wyczyscic strukture.

Po operacjach modyfikujacych program automatycznie zapisuje stan pomocniczy:

- `hash_liniowa_autosave.csv` dla adresowania liniowego,
- `hash_kwadratowa_autosave.csv` dla adresowania kwadratowego,
- `hash_avl_autosave.csv` dla tablicy z kubelkami AVL.

Reczny zapis z menu tworzy:

- `hash_liniowa.csv`,
- `hash_kwadratowa.csv`,
- `hash_avl.csv`.

## Generowanie danych

Funkcja `generateRandom()` uzywana w menu czysci aktualna strukture i tworzy podana liczbe nowych elementow.

Klucze i wartosci sa losowane z zakresu:

```text
1 .. count * 10 + 100
```

Do generowania uzywane sa `std::random_device`, `std::mt19937` i `std::uniform_int_distribution`.

Poniewaz klucz w tablicy mieszajacej musi byc unikalny, program losuje kolejne pary do momentu, az struktura osiagnie zadany rozmiar. Jesli wylosowany klucz juz istnieje, operacja `insert` aktualizuje wartosc, ale rozmiar sie nie zwieksza, wiec losowanie trwa dalej.

## Zapis i odczyt CSV

Wszystkie implementacje zapisuja pliki w formacie:

```csv
key,value
10,500
7,120
15,450
```

Metoda `loadFromCSV()` odczytuje kolejne rekordy `key,value`, a nastepnie odbudowuje strukture przez `insert`. Oznacza to, ze dane po wczytaniu sa poprawne logicznie, ale fizyczne rozmieszczenie elementow w tablicy moze byc inne niz przed zapisem, poniewaz zalezy od aktualnej pojemnosci tablicy i procesu `rehash`.

## Badania wydajnosciowe

Benchmarki sa uruchamiane z menu glownego przez opcje `Badania wydajnosciowe i zapis CSV`.

Mierzone operacje:

- `insert`,
- `remove`.

Rozmiary struktur uzywane w benchmarkach:

```text
10000, 20000, 40000, 80000, 100000, 160000, 320000, 640000
```

Dla kazdego rozmiaru kazda operacja jest mierzona 100 razy. Przed pojedynczym pomiarem tworzona jest nowa struktura wypelniona tym samym zestawem danych dla danej proby. Dane testowe sa generowane deterministycznie na podstawie funkcji `benchmarkSeed(size, attempt)`.

Pliki wynikowe:

- `pomiary.txt` - zbiorcze zestawienie wynikow,
- `benchmark_liniowa.csv` - wyniki dla adresowania liniowego,
- `benchmark_kwadratowa.csv` - wyniki dla adresowania kwadratowego,
- `benchmark_avl.csv` - wyniki dla tablicy z kubelkami AVL,
- `seedy_100000.txt` - lista seedow dla rozmiaru 100000.

Format plikow CSV z benchmarkami:

```csv
Operation,Size,AverageTime_ns
insert,10000,123
remove,10000,456
```

## Kompilacja

Przykladowa komenda kompilacji:

```bash
g++ -std=c++17 -Wall -Wextra -O2 src/main.cpp src/hash_tables.cpp src/benchmark.cpp -o miniprojekt3.exe
```

W srodowisku MSYS2/MinGW mozna uzyc:

```bash
/mingw64/bin/g++ -std=c++17 -Wall -Wextra -O2 src/main.cpp src/hash_tables.cpp src/benchmark.cpp -o miniprojekt3.exe
```

Jesli dostepny jest `make`, projekt mozna zbudowac tak:

```bash
make
```

## Uruchomienie

Windows:

```bash
miniprojekt3.exe
```

Git Bash / MSYS / podobne srodowisko:

```bash
./miniprojekt3.exe
```

## Podsumowanie

Projekt realizuje trzy warianty tablic mieszajacych: adresowanie liniowe, adresowanie kwadratowe oraz tablice z kubelkami opartymi o drzewa AVL. Zawiera menu konsolowe, losowe generowanie danych, zapis i odczyt CSV oraz benchmarki porownujace koszty operacji `insert` i `remove` dla duzych zestawow danych.
