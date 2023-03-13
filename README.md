# Поисковый сервер
Консольное приложение с поддержкой многопоточности для поиска текстовых документов с учетом минус и стоп-слов, ранжированием по релевантности (TF-IDF) и пагинатором страниц для удобного вывода результатов поиска.

### Реализованый функционал
- Парсер строк. Разбиение строки по словам.
- Обработка стоп-слов. Не учитываются поисковой системой при подсчете TF-IDF и не влияют на результаты поиска.
- Обработка минус-слов. Документы, содержащие минус-слова не попадают в результаты поиска.
- Создание и обработка очереди запросов.
- Ранжирование вывода документов по рейтингу и релевантности.
- Пагинатор страниц для удобного вывода большого количества найденных документов.
- Удаление дубликатов документов.
- Поддержка параллельных алгоритмов execution библиотеки STL. Для избежания состояния гонки разработан контейнер ConcurrentMap, который является потокобезопасной версией unordered_map для ключей-целых чисел.

### Использованные технологии
- C++ 17
- STL
- CMake 3.25.2

### Содержание файлов
- `search_server.h | .cpp` класс SearchServer, функционал поисковой системы
- `concurrent_map.h` реализация потокобезопасного контейнера ConcurrentMap
- `document.h` структуры Document и enum-класс DocumentStatus
- `log_duration.h` профилировщик, позволяющий измерять время выполнения программы целиком или отдельных функций
- `paginator.h` реализация функционала пагинатора страниц
- `process_queries.h | cpp` реализация очереди запросов
- `read_input_functions.h | .cpp` функции чтения и обработки входного потока
- `string_processing.h | .cpp` вспомогательные функции обработки текста
- `text_example_functions.h | .cpp` функции с примерами использования и бенчмарки
- `main.cpp` точки входа для запуска функций примеров работы программы и бенчмарков
### Руководство по использованию
1. Для начала работы с поисковой системой необходимо создать экземпляр класса SearchServer, используя один из следующих конструкторов, задать стоп слова:

1.1. Шаблонный конструктор, принимающий массив строк
```C++
template <typename StringContainer>
explicit SearchServer(const StringContainer& stop_words);
```
Пример использования:
```C++
std::vector<std::string> stop_words {"and"s, "with"s};
SearchServer search_server(stop_words);
```
1.2. Конструктор, принимающий строку
```C++
explicit SearchServer(const std::string& stop_words);
```
Пример использования:
```C++
SearchServer search_server("and with"s);
```
2. Для добавления документа в базу данных необходимо вызвать метод AddDocuments класса SearchServer и передать ему id документа, текстовый документ, статус документа (ACTUAL, IRRELEVANT, BANNED, REMOVED,) и массив рейтинговых оценок.
```C++
void AddDocument(int document_id,
                 std::string_view document,
		 DocumentStatus status,
		 const std::vector<int>& ratings);
```
Пример использования:
```C++
int id = 0;
for (
    const string& text : {
        "funny pet and nasty rat"s,
        "funny pet with curly hair"s,
        "funny pet and not very nasty rat"s,
        "pet with rat and rat and rat"s,
        "nasty rat with curly hair"s,
    }) {
    search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
}
```
3. Отправка поискового запроса и варианты использования сервера.

Вариант 1. Метод MatchDocument.

К серверу можно обратиться, чтобы узнать количество совпадений слов из поискового запроса в каждом из документов. Если совпадений не было найдено в документе, то id такого документа в результаты выведено не будет. Если в документе найдено хоть одно минус-слово, то будет выведена информация о том, что в документе найдено 0 совпадений и id такого документа.

Пример использования (на сервер уже добавлены документы из примера выше):
```C++
const string query = "curly and funny -not"s;
{
    const auto [words, status] = search_server.MatchDocument(query, 1);
    cout << words.size() << " words for document 1"s << endl;
    // 1 words for document 1
}
{
    const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
    cout << words.size() << " words for document 2"s << endl;
    // 2 words for document 2
}
{
    const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
    cout << words.size() << " words for document 3"s << endl;
    // 0 words for document 3
}
```
Вывод:
```
1 words for document 1
2 words for document 2
0 words for document 3
```

Вариант 2. Метод FindTopDocuments.
Метод FindTopDocuments имеет перегрузки для разных вариантов вызова. На вход он может получить:
- строку запроса;
- строку запроса + статус документа;
- строку запроса + статус документа + функцию-пердикат;
- все вышеперечисленные варианты с политиками выполнения (execution policy) расширения Parallelism STL.

Метод возвращает вектор документов (структура Document), отсортированный по убыванию рейтинга (поле rating), а если документы имеют одинаковый рейтинг, то по убыванию актуальности документа (поле relevance).

Поля структуры Document:
```C++
struct Document {
    ...
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};
```

Пример использования:
```C++
SearchServer search_server("and with"s);
int id = 0;
for (
    const string& text : {
        "white cat and yellow hat"s,
        "curly cat curly tail"s,
        "nasty dog with big eyes"s,
        "nasty pigeon john"s,
    }) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
}
search_server.AddDocument(++id, "curly brown dog"s, DocumentStatus::BANNED, {2, 3, 4, 5});

std::string query = "curly nasty cat"s;
cout << "ACTUAL by default:"s << endl;
// последовательная версия
for (const Document& document :
    search_server.FindTopDocuments(query)) {
    PrintDocument(document);
}
cout << "BANNED:"s << endl;
// последовательная версия
for (const Document& document :
    search_server.FindTopDocuments(execution::seq,
                                   query,
                                   DocumentStatus::BANNED)) {
    PrintDocument(document);
}
cout << "Even ids:"s << endl;
// параллельная версия
for (const Document& document :
    search_server.FindTopDocuments(execution::par,
                                   query,
                                   [](int document_id, DocumentStatus status, int rating) {
                                       return document_id % 2 == 0;
                                   })) {
    PrintDocument(document);
}
```
Вывод:
```
ACTUAL by default:
{ document_id = 2, relevance = 0.687218, rating = 1 }
{ document_id = 4, relevance = 0.30543, rating = 1 }
{ document_id = 1, relevance = 0.229073, rating = 1 }
{ document_id = 3, relevance = 0.229073, rating = 1 }
BANNED:
{ document_id = 5, relevance = 0.30543, rating = 3 }
Even ids:
{ document_id = 2, relevance = 0.687218, rating = 1 }
{ document_id = 4, relevance = 0.30543, rating = 1 }
```
