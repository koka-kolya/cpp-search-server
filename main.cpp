#include "document.h"
#include "log_duration.h"
#include "paginator.h"
#include "process_queries.h"
#include "read_input_functions.h"
#include "request_queue.h"
//#include "remove_duplicates.h"
#include "search_server.h"
#include "string_processing.h"

#include "test_example_functions.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <random>



///////////////////////////////
///////    BENCHMARK 1    /////
///////////////////////////////

//#include <execution>
//#include <iostream>
//#include <random>
//#include <string>
//#include <vector>

//using namespace std;

//string GenerateWord(mt19937& generator, int max_length) {
//    const int length = uniform_int_distribution(1, max_length)(generator);
//    string word;
//    word.reserve(length);
//    for (int i = 0; i < length; ++i) {
//        word.push_back(uniform_int_distribution('a', 'z')(generator));
//    }
//    return word;
//}

//vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
//    vector<string> words;
//    words.reserve(word_count);
//    for (int i = 0; i < word_count; ++i) {
//        words.push_back(GenerateWord(generator, max_length));
//    }
//    sort(words.begin(), words.end());
//    words.erase(unique(words.begin(), words.end()), words.end());
//    return words;
//}

//string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
//    string query;
//    for (int i = 0; i < word_count; ++i) {
//        if (!query.empty()) {
//            query.push_back(' ');
//        }
//        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
//            query.push_back('-');
//        }
//        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
//    }
//    return query;
//}

//vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
//    vector<string> queries;
//    queries.reserve(query_count);
//    for (int i = 0; i < query_count; ++i) {
//        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
//    }
//    return queries;
//}

//template <typename ExecutionPolicy>
//void Test(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
//    LOG_DURATION(mark);
//    const int document_count = search_server.GetDocumentCount();
//    int word_count = 0;
//    for (int id = 0; id < document_count; ++id) {
//        const auto [words, status] = search_server.MatchDocument(policy, query, id);
//        word_count += words.size();
//    }
//    cout << word_count << endl;
//}

//#define TEST(policy) Test(#policy, search_server, query, execution::policy)

//int main() {
////	TestNoStopWords();
//    mt19937 generator;

//    const auto dictionary = GenerateDictionary(generator, 1000, 10);
//    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

//    const string query = GenerateQuery(generator, dictionary, 500, 0.1);

//    SearchServer search_server(dictionary[0]);
//    for (size_t i = 0; i < documents.size(); ++i) {
//        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
//    }
////    TestSearchServer();
//    TEST(seq);
//    TEST(par);
//}

/////////////////////////////
////    BENCHMARK 2     /////
/////////////////////////////

#include <random>
#include <string>
#include <vector>
using namespace std;
string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}
vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}
string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}
vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}
template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}
#define TEST(policy) Test(#policy, search_server, queries, execution::policy)
int main() {
    mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 1000, 10);
    const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
    SearchServer search_server(dictionary[0]);
    for (size_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
    }
    const auto queries = GenerateQueries(generator, dictionary, 100, 70);
    TEST(seq);
    TEST(par);
}

/////////////////////////
////	EXAMPLE 1	/////
/////////////////////////

//#include <iostream>
//#include <string>
//#include <vector>

//using namespace std;

//int main() {
//    SearchServer search_server("and with"s);

//    int id = 0;
//    for (
//        const string& text : {
//            "funny pet and nasty rat"s,
//            "funny pet with curly hair"s,
//            "funny pet and not very nasty rat"s,
//            "pet with rat and rat and rat"s,
//            "nasty rat with curly hair"s,
//    }
//            ) {
//        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
//    }

//    const string query = "curly and funny -not"s;

//    {
//        const auto [words, status] = search_server.MatchDocument(query, 1);
//        cout << words.size() << " words for document 1"s << endl;
//        // 1 words for document 1
//    }

//    {
//        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
//        cout << words.size() << " words for document 2"s << endl;
//        // 2 words for document 2
//    }

//    {
//        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
//        cout << words.size() << " words for document 3"s << endl;
//        // 0 words for document 3
//    }

//    return 0;
//}


/////////////////////////
////	EXAMPLE 2	/////
/////////////////////////

//#include "search_server.h"

//#include <iostream>
//#include <string>
//#include <vector>

//using namespace std;

//int main() {
//    SearchServer search_server("and with"s);

//    int id = 0;
//    for (
//        const string& text : {
//            "funny pet and nasty rat"s,
//            "funny pet with curly hair"s,
//            "funny pet and not very nasty rat"s,
//            "pet with rat and rat and rat"s,
//            "nasty rat with curly hair"s,
//    }
//            ) {
//        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
//    }

//    const string query = "curly and funny"s;

//    auto report = [&search_server, &query] {
//        std::cout << "seq - par" << std::endl;
//        cout << search_server.GetDocumentCount() << " documents total, "s
//             << search_server.FindTopDocuments(std::execution::seq, query).size() << " documents for query ["s << query << "]"s << endl;
//        cout << search_server.GetDocumentCount() << " documents total, "s
//             << search_server.FindTopDocuments(std::execution::par, query).size() << " documents for query ["s << query << "]"s << endl;
//    };

//    report();
//    // однопоточная версия
//    search_server.RemoveDocument(5);
//    report();
//    // однопоточная версия
//    search_server.RemoveDocument(execution::seq, 1);
//    report();
//    // многопоточная версия
//    search_server.RemoveDocument(execution::par, 2);
//    report();

//    // Проверка метода Erase для ConcurrentMap

//    ConcurrentMap<int, int> some_map(5);
//    some_map[0].ref_to_value = 2;
//    some_map[1].ref_to_value = 3;
//    some_map[2].ref_to_value = 4;

//    some_map.Erase(0);

//    auto some_builded_map = some_map.BuildOrdinaryMap();
//    for (const auto& [id, freq] : some_builded_map) {
//        std::cout << "{ " << id << " : " << freq << " }" << std::endl;
//    }

//    return 0;
//}

///////////////////////////////
///////     EXAMPLE 3     /////
///////////////////////////////

//#include "process_queries.h"
//#include "search_server.h"
//#include <execution>
//#include <iostream>
//#include <string>
//#include <vector>
//using namespace std;
//void PrintDocument(const Document& document) {
//    cout << "{ "s
//         << "document_id = "s << document.id << ", "s
//         << "relevance = "s << document.relevance << ", "s
//         << "rating = "s << document.rating << " }"s << endl;
//}
//int main() {
//    SearchServer search_server("and with"s);
//    int id = 0;
//    for (
//        const string& text : {
//            "white cat and yellow hat"s,
//            "curly cat curly tail"s,
//            "nasty dog with big eyes"s,
//            "nasty pigeon john"s,
//        }
//    ) {
//        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
//    }
//    cout << "ACTUAL by default:"s << endl;
//    // последовательная версия
//    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
//        PrintDocument(document);
//    }
//    cout << "BANNED:"s << endl;
//    // последовательная версия
//    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
//        PrintDocument(document);
//    }
//    cout << "Even ids:"s << endl;
//    // параллельная версия
//    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
//        PrintDocument(document);
//    }
//    return 0;
//}
