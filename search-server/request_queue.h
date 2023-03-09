#pragma once
#include <stack>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
            : search_server_(search_server)
    {
    }

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        int result_count = 0;
        int time = 0;
    };

    const SearchServer& search_server_;
    std::deque<QueryResult> requests_;
    QueryResult result_;
    int requests_no_result_ = 0;
	const static int kMinInDay = 1440;
};

/*
 *	Specifying RequestQueue template methods
 */

template <typename DocumentPredicate>
std::vector<Document>
RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    ++result_.time; //по условию задания - 1 запрос в минуту
	//получаем результаты запроса
	auto search_result = search_server_.FindTopDocuments(raw_query, document_predicate);
    result_.result_count = static_cast<int>(search_result.size());
    requests_.push_back(result_);
    if (0 == search_result.size()) {
        ++requests_no_result_;
		if (requests_.size() > kMinInDay) { // новые сутки
            --result_.result_count;
            requests_.pop_front();
        }
    }
	if (requests_.size() > kMinInDay) { // новые сутки
        result_.time = 0; // обнуляем счетчик времени - новые сутки
        if (requests_.front().result_count == 0) { // уменьшаем, если самый старый результат пустой
            --requests_no_result_;
        }
        --result_.result_count;
        requests_.pop_front(); // удаляем самый старый результат
    }
    return search_result;
}
