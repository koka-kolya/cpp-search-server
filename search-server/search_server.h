#pragma once

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

#include <algorithm>
#include <deque>
#include <execution>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using PAR = const std::execution::parallel_policy;
using SEQ = const std::execution::sequenced_policy;
using Documents = std::vector<Document>;
using MatchedDocuments = std::tuple<std::vector<std::string_view>, DocumentStatus>;
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ROUND_ERR = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
	explicit SearchServer(const std::string_view stop_words);
    explicit SearchServer(const std::string& stop_words);

	void AddDocument(int document_id,
					 std::string_view document,
					 DocumentStatus status,
					 const std::vector<int>& ratings);

    template <typename DocumentPredicate>
	Documents
	FindTopDocuments(std::string_view raw_query,
					 DocumentPredicate document_predicate) const;
	template <typename DocumentPredicate>
	Documents
	FindTopDocuments(const SEQ&,
					 std::string_view raw_query,
					 DocumentPredicate document_predicate) const;
	template <typename DocumentPredicate>
	Documents
	FindTopDocuments(const PAR&,
					 std::string_view raw_query,
					 DocumentPredicate document_predicate) const;

	Documents
	FindTopDocuments(std::string_view raw_query,
					 DocumentStatus status) const;
	Documents
	FindTopDocuments(const SEQ&,
					 std::string_view raw_query,
					 DocumentStatus status) const;
	Documents
	FindTopDocuments(const PAR&,
					 std::string_view raw_query,
					 DocumentStatus status) const;

	Documents
	FindTopDocuments(std::string_view raw_query) const;
	Documents
	FindTopDocuments(const SEQ&,
					 std::string_view raw_query) const;
	Documents
	FindTopDocuments(const PAR&,
					 std::string_view raw_query) const;

	std::set<int>::const_iterator begin() const;
	std::set<int>::const_iterator end() const;

	MatchedDocuments
    MatchDocument(std::string_view raw_query,
                  int document_id) const;
	MatchedDocuments
	MatchDocument(const SEQ&,
                  std::string_view raw_query,
                  int document_id) const;
	MatchedDocuments
	MatchDocument(const PAR&,
                  std::string_view raw_query,
                  int document_id) const;

	const std::map<std::string_view, double>&
	GetWordFrequencies(int document_id) const;

	int GetDocumentCount() const;

	void RemoveDocument(int document_id);
	void RemoveDocument(const SEQ&, int document_id);
	void RemoveDocument(const PAR&, int document_id);

private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::set<std::string, std::less<>> stop_words_;
    std::deque<std::string> storage_ {};
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
	std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    bool IsStopWord(std::string_view word) const;

	static bool
	IsValidWord(std::string_view word);

	std::vector<std::string_view>
	SplitIntoWordsNoStop(const std::string_view text) const;

	static int
	ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

	QueryWord
	ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

	Query
	ParseQuery(std::string_view text, bool is_need_sort = true) const;

	double
	ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename DocumentPredicate>
	Documents FindAllDocuments(const Query& query,
							   DocumentPredicate document_predicate) const;
	template <typename DocumentPredicate>
	Documents FindAllDocuments(const SEQ&,
							   const Query& query,
							   DocumentPredicate document_predicate) const;
	template <typename DocumentPredicate>
	Documents FindAllDocuments(const PAR&,
							   const Query& query,
							   DocumentPredicate document_predicate) const;

	std::vector<std::string_view>
	SortingMatchWordVector (std::vector<std::string_view>& matched_words) const;
};

/*
 *	Specifying SearchServer template methods
 *
 */
template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
	: stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    using namespace std;
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
Documents
SearchServer::FindTopDocuments(std::string_view raw_query,
							   DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate>
Documents
SearchServer::FindTopDocuments(const SEQ&,
							   std::string_view raw_query,
							   DocumentPredicate document_predicate) const {
	const auto query = ParseQuery(raw_query);

	auto matched_documents = FindAllDocuments(query, document_predicate);
	std::sort(std::execution::seq, matched_documents.begin(), matched_documents.end(),
	     [this](const Document& lhs, const Document& rhs) {
		     if (std::abs(lhs.relevance - rhs.relevance) < ROUND_ERR) {
			     return lhs.rating > rhs.rating;
		     } else {
			     return lhs.relevance > rhs.relevance;
		     }
	     });
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}
	return matched_documents;
}

template <typename DocumentPredicate>
Documents
SearchServer::FindTopDocuments(const PAR&,
							   std::string_view raw_query,
							   DocumentPredicate document_predicate) const {
	const auto query = ParseQuery(raw_query);

	auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);
    std::sort(std::execution::par,
              matched_documents.begin(), matched_documents.end(),
              [this](const Document& lhs, const Document& rhs) {
                    if (std::abs(lhs.relevance - rhs.relevance) < ROUND_ERR) {
                        return lhs.rating > rhs.rating;
                    } else {
                        return lhs.relevance > rhs.relevance;
                    }
               });
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}
	return matched_documents;
}

template <typename DocumentPredicate>
Documents
SearchServer::FindAllDocuments(const Query& query,
							   DocumentPredicate document_predicate) const {
	return FindAllDocuments(std::execution::seq, query, document_predicate);
}

template <typename DocumentPredicate>
Documents
SearchServer::FindAllDocuments(const SEQ&,
							   const Query& query,
							   DocumentPredicate document_predicate) const {

	std::map<int, double> document_to_relevance;
	for (std::string_view word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
		for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
			const auto& document_data = documents_.at(document_id);
			if (document_predicate(document_id, document_data.status, document_data.rating)) {
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}
		}
	}

	for (const std::string_view word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
			document_to_relevance.erase(document_id);
		}
	}

	Documents matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance) {
		matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
	}
	return matched_documents;
}

template <typename DocumentPredicate>
Documents
SearchServer::FindAllDocuments(const PAR&,
							   const Query& query,
							   DocumentPredicate document_predicate) const {

    ConcurrentMap<int, double> document_to_relevance(word_to_document_freqs_.size());

	std::for_each (std::execution::par,
				   query.plus_words.begin(), query.plus_words.end(),
				   [this, &document_predicate, &document_to_relevance]
				   (const std::string_view plus_word) {
						if (word_to_document_freqs_.count(plus_word) != 0) {
							const double inverse_document_freq
									= ComputeWordInverseDocumentFreq(plus_word);
							for (const auto [document_id, term_freq]
								 : word_to_document_freqs_.at(plus_word)) {
								const auto& document_data = documents_.at(document_id);
								if (document_predicate(document_id,
													   document_data.status,
													   document_data.rating)) {
									document_to_relevance[document_id].ref_to_value
											+= term_freq * inverse_document_freq;
								}
							}
						}
				});

    std::for_each(std::execution::par,
                  query.minus_words.begin(), query.minus_words.end(),
                  [this, &document_to_relevance](const std::string_view minus_word){
                    if (word_to_document_freqs_.count(minus_word) > 0) {
						for (const auto [document_id, _] : word_to_document_freqs_.at(minus_word)) {
							document_to_relevance.Erase(document_id);
                        }
                    }
				});

    auto document_to_relevance_output = std::move(document_to_relevance.BuildOrdinaryMap());
	Documents matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance_output) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating} );
    }

    return matched_documents;

}
