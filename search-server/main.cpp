/*
 * Sprint 3. Framework and search server.
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
	const int doc_id = 2;
	const string content = "cat in the big city"s;
	const vector<int> ratings = {1, 2, 3, 4, 5};
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("cat -big"s);
		ASSERT_EQUAL_HINT(found_docs.size(), 0, "ERR: Document consist stop word!"s);
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
		tie(words, status) = server.MatchDocument("cat -big"s, 2);
		ASSERT_EQUAL_HINT(words.size(), 0, "ERR: Vector of words consist stop word!"s);
	}
}

void TestRelevanceSortedAndCorrectCalculatedRatingsOfFindedDocuments() {
	{
		SearchServer search_server;
		search_server.SetStopWords("и в на"s);
		search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
		search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
		search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
		search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
		const vector<Document>& doc = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
		{
			vector<double> out_rel;
			vector<int> out_rating;
			for (const auto& [id, rel, rate] : doc) {
				out_rel.push_back(rel);
				out_rating.push_back(rate);
			}
			vector<double> compare_rel {0.86643397569993164, 0.17328679513998632, 0.17328679513998632};
			ASSERT_EQUAL_HINT(compare_rel, out_rel, "ERR: Not sorted by relevance"s);
			vector<int> compare_rating = {5, 2, -1};
			ASSERT_EQUAL_HINT(compare_rating, out_rating, "ERR: Not correct calculated rating"s);
		}
	}
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestExcludeDocumentsConsistMinusWordsFromOutputResults);
	RUN_TEST(TestMatchingDocuments);
	RUN_TEST(TestRelevanceSortedAndCorrectCalculatedRatingsOfFindedDocuments);
}
