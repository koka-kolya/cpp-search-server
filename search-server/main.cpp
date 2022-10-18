/*
 * Sprint 3. Framework and search server. With corrections after the 1st review.
 */
void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	// Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
	// находит нужный документ
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}
	// Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
	// возвращает пустой результат
	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT(server.FindTopDocuments("in"s).empty());
	}
}

void TestExcludeDocumentsConsistMinusWordsFromOutputResults() {
	const int id = 2;
	const string content = "cat in the big city"s;
	const vector<int> ratings = {1, 2, 3, 4, 5};
	{
		SearchServer server;
		server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
		const auto founded_docs_without_minus = server.FindTopDocuments("cat big"s);
		ASSERT_EQUAL_HINT(founded_docs_without_minus.size(), 1, "ERR: Document hasn't been added!"s);
	}
	{
		SearchServer server;
		server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
		const auto founded_docs_with_minus = server.FindTopDocuments("cat -big"s);
		ASSERT_EQUAL_HINT(founded_docs_with_minus.size(), 0, "ERR: Document consist stop word!"s);
	}
}

void TestMatchingDocuments() {
	const int doc_id = 2;
	const string content = "cat in the big city"s;
	const vector<int> ratings = {1, 2, 3, 4, 5};
	
	
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		vector<string> words {};
		DocumentStatus status;
		tie(words, status) = server.MatchDocument("cat big"s, 2);
		vector<string> expected_words {"big"s, "cat"s};
		ASSERT_EQUAL_HINT(words, expected_words, "ERR: No expected words in matched document!");
		int status_int = static_cast<int>(status);
		DocumentStatus expected_status = DocumentStatus::ACTUAL;
		int expected_status_int = static_cast<int>(expected_status);
		ASSERT_EQUAL_HINT(status_int, expected_status_int, "ERR: No expected status in matched document!"s);
	}
}

void TestCorrectCalculateRelevance () {
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
	search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
	const vector<Document>& doc = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
	{
		vector<double> out_relevances;
		for (const auto& [_, rel, __] : doc) {
			out_relevances.push_back(rel);
		}
		vector<double> expected_relevances {0.86643397569993164, 0.17328679513998632, 0.17328679513998632};
		ASSERT_EQUAL_HINT(out_relevances.size(), expected_relevances.size(), "ERR: Count of calculated relevances not equal expected count!");
		for (int i = 0; i < out_relevances.size(); ++i) {
			bool is_correct_relevance = (out_relevances[i] - expected_relevances[i]) <= 1e-6;	
			ASSERT_HINT(is_correct_relevance, "ERR: Value of calculate relevance not equal expected relevance!"s);
		}
	}
}

void TestCorrectCalculatedRating() {
	{
		SearchServer search_server;
		search_server.SetStopWords("и в на"s);
		search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
		search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
		search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
		search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
		const vector<Document>& doc = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
		{
			vector<int> out_ratings;
			for (const auto& [_, __, rating] : doc) {
				out_ratings.push_back(rating);
			}
			vector<int> expected_ratings {5, 2, -1};
			ASSERT_EQUAL_HINT(out_ratings, expected_ratings, "ERR: Calculate ratings not equal expected!");
		}
	}
	//2 способ, со сравнением вычисленного в тесте рейтинга добавляемого документа
	{
		const int id = 2;
		const string content = "cat in the city"s;
		const vector<int> ratings = {1, 2, 3, 4, 5};
		{
			SearchServer server;
			server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
			int expected_rating = 0;
			for (const int& rating : ratings) {
				expected_rating += rating;
			}
			expected_rating = expected_rating/ratings.size();
			const vector<Document>& search_result = server.FindTopDocuments("cat", DocumentStatus::ACTUAL);
			for (const auto& [_, __, rating] : search_result) {
				ASSERT_EQUAL_HINT(expected_rating, rating, "ERR: Calculated rating is incorrect!");
			}
		}
	}
}

void TestRelevanceSortedForFindedDocuments() {
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
	search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
	vector<double> out_relevance;
	const vector<Document>& doc = search_server.FindTopDocuments("пушистый ухоженный кот"s);
	{
		for (const auto& [_, rel, __] : doc) {
			out_relevance.push_back(rel);
		}
		for (int i = 0; i < out_relevance.size() - 1; ++i) {
			bool is_left_bigger = out_relevance[i] - out_relevance[i + 1] >= 0;
			ASSERT_HINT(is_left_bigger, "ERR: Output result not correctly sorted by relevance!");
		}
	}
}

void TestRatingSortedForFindedDocuments() {
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
	search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
	search_server.AddDocument(4, "ухоженный и певчий дрозд анатолий"s, DocumentStatus::ACTUAL, {9, 10, 8});
	vector<double> out_relevance;
	const vector<Document>& doc = search_server.FindTopDocuments("пушистый ухоженный кот"s);
	{
		vector<int> out_ratings;
		vector<double> out_relevances;
		for (const auto& [_, relevance, rate] : doc) {
			out_ratings.push_back(rate);
			out_relevances.push_back(relevance);
		}
		for (int i = 0; i < out_ratings.size() - 1; ++i) {
			bool is_correct_sorted = true;
			if (abs(out_relevances[i] - out_relevances[i + 1]) < 1e-6) {
				is_correct_sorted = out_ratings[i] - out_ratings[i + 1] >= 0;
			}
			ASSERT_HINT(is_correct_sorted, "ERR: Output result not correctly sorted by rating!");
		}
	}
}

void TestFindedByStatus() {
	const int id = 2;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3, 4, 5};
	
	{
		const DocumentStatus status = DocumentStatus::ACTUAL;
		SearchServer server;
		server.AddDocument(id, content, DocumentStatus::ACTUAL, ratings);
		const vector<Document> search_result = server.FindTopDocuments("cat"s, status);
		ASSERT_EQUAL_HINT(search_result.size(), 1, "ERR: Count of searched documents not equal by expected!"s);
		for (const Document& doc : search_result) {
			ASSERT_EQUAL_HINT(doc.id, id,  "Searching by status doesn't work properly!"s);
		}
		server.AddDocument(202, "dog in the village", DocumentStatus::BANNED, ratings);
		const vector<Document>& search_result2 = server.FindTopDocuments("cat dog"s, DocumentStatus::BANNED);
		ASSERT_EQUAL_HINT(search_result2.size(), 1, "ERR: Searching by status doesn't work properly!");
		for (const Document& doc : search_result2) {
			ASSERT_EQUAL_HINT(doc.id, 202,  "Searching by status doesn't work properly!"s);
		}
		
	}
}

void TestFindedByPredicateFilter() {
	SearchServer search_server;
	search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
	search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
	
	for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
		ASSERT_HINT(document.id % 2 == 0, "ERR: Searching with predicate doesn't work properly!");
	}
}
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestExcludeDocumentsConsistMinusWordsFromOutputResults);
	RUN_TEST(TestMatchingDocuments);
	RUN_TEST(TestCorrectCalculateRelevance);
	RUN_TEST(TestCorrectCalculatedRating);
	RUN_TEST(TestRelevanceSortedForFindedDocuments);
	RUN_TEST(TestRatingSortedForFindedDocuments);
	RUN_TEST(TestFindedByStatus);
	RUN_TEST(TestFindedByPredicateFilter);
}
