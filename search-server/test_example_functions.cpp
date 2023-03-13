#include "test_example_functions.h"

void PrintDocument(const Document& document) {
	using namespace std;
	cout << "{ "s
		 << "document_id = "s << document.id << ", "s
		 << "relevance = "s << document.relevance << ", "s
		 << "rating = "s << document.rating << " }"s << endl;
}

void Example1() {
	using namespace std;

	cout << "Example 1"s << endl;
	SearchServer search_server("and with"s);

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
	cout << endl;
}

void Example2() {
	using namespace std;

	cout << "Example 2"s << endl;

	SearchServer search_server("and with"s);

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

	const string query = "curly and funny"s;

	auto report = [&search_server, &query] {
		std::cout << "seq - par" << std::endl;

		cout << search_server.GetDocumentCount() << " documents total, "s
			 << search_server.FindTopDocuments(std::execution::seq, query).size()
			 << " documents for query ["s << query << "]"s << endl;
		cout << search_server.GetDocumentCount() << " documents total, "s
			 << search_server.FindTopDocuments(std::execution::par, query).size()
			 << " documents for query ["s << query << "]"s << endl;
	};

	report();
	// однопоточная версия
	search_server.RemoveDocument(5);
	report();
	// однопоточная версия
	search_server.RemoveDocument(execution::seq, 1);
	report();
	// многопоточная версия
	search_server.RemoveDocument(execution::par, 2);
	report();

	// Проверка метода Erase для ConcurrentMap
	ConcurrentMap<int, int> some_map(5);
	some_map[0].ref_to_value = 2;
	some_map[1].ref_to_value = 3;
	some_map[2].ref_to_value = 4;

	some_map.Erase(0);

	auto some_builded_map = some_map.BuildOrdinaryMap();
	for (const auto& [id, freq] : some_builded_map) {
		std::cout << "{ " << id << " : " << freq << " }" << std::endl;
	}

	cout << endl;
}

void Example3() {
	using namespace std;

	cout << "Example 3"s << endl;

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
	cout << endl;
}

std::string
GenerateWord(std::mt19937& generator,
			 int max_length) {
	using namespace std;
	const int length = uniform_int_distribution(1, max_length)(generator);
	string word;
	word.reserve(length);
	for (int i = 0; i < length; ++i) {
		word.push_back(uniform_int_distribution('a', 'z')(generator));
	}
	return word;
}

std::vector<std::string>
GenerateDictionary(std::mt19937& generator,
				   int word_count,
				   int max_length) {
	std::vector<std::string> words;
	words.reserve(word_count);
	for (int i = 0; i < word_count; ++i) {
		words.push_back(GenerateWord(generator, max_length));
	}
	words.erase(unique(words.begin(), words.end()), words.end());
	return words;
}

std::string
GenerateQuery(std::mt19937& generator,
			  const std::vector<std::string>& dictionary,
			  int word_count,
			  double minus_prob) {
	std::string query;
	for (int i = 0; i < word_count; ++i) {
		if (!query.empty()) {
			query.push_back(' ');
		}
		if (std::uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
			query.push_back('-');
		}
		query += dictionary[std::uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
	}
	return query;
}

std::vector<std::string>
GenerateQueries(std::mt19937& generator,
				const std::vector<std::string>& dictionary,
				int query_count,
				int max_word_count) {
	std::vector<std::string> queries;
	queries.reserve(query_count);
	for (int i = 0; i < query_count; ++i) {
		queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
	}
	return queries;
}

void Benchmark1() {
	std::mt19937 generator;
	const auto dictionary = GenerateDictionary(generator, 1000, 10);
	const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
	SearchServer search_server(dictionary[0]);
	for (size_t i = 0; i < documents.size(); ++i) {
		search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
	}
	const auto queries = GenerateQueries(generator, dictionary, 100, 70);
	std::cout << "Benchmark 1"s << std::endl;
	Test("SEQ"s, search_server, queries, std::execution::seq);
	Test("PAR"s, search_server, queries, std::execution::par);

}

void Benchmark2() {
	std::mt19937 generator;
	const auto dictionary = GenerateDictionary(generator, 1000, 10);
	const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
	const std::string query = GenerateQuery(generator, dictionary, 500, 0.1);
	SearchServer search_server(dictionary[0]);
	for (size_t i = 0; i < documents.size(); ++i) {
		search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
	}
	std::cout << std::endl;
	std::cout << "Benchmark 2"s << std::endl;
	Test("SEQ"s, search_server, query, std::execution::seq);
	Test("PAR"s, search_server, query, std::execution::par);
	std::cout << std::endl;
}

