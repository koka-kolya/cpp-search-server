#include "search_server.h"

#include <cmath>

SearchServer::SearchServer(std::string_view stop_words)
        : SearchServer(SplitIntoWords(stop_words))
{
}
SearchServer::SearchServer(const std::string& stop_words)
        : SearchServer(std::string_view(stop_words))
{
}

void SearchServer::AddDocument(int document_id,
							   std::string_view document,
							   DocumentStatus status,
							   const std::vector<int>& ratings) {
    using namespace std;
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }

    storage_.emplace_back(document);

    const auto words = SplitIntoWordsNoStop(storage_.back());

    const double inv_word_count = 1.0 / words.size();
    for (std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document>
SearchServer::FindTopDocuments(const std::string_view raw_query,
							   DocumentStatus status) const {
	return FindTopDocuments(raw_query,
							[status](int document_id, DocumentStatus document_status, int rating) {
								return document_status == status;
						});
}

std::vector<Document>
SearchServer::FindTopDocuments(const SEQ&,
							   std::string_view raw_query,
							   DocumentStatus status) const {
	return FindTopDocuments(std::execution::seq,
							raw_query,
							[status](int document_id, DocumentStatus document_status, int rating) {
								return document_status == status;
						});
}

std::vector<Document>
SearchServer::FindTopDocuments(const PAR&,
							   std::string_view raw_query,
							   DocumentStatus status) const {
	return FindTopDocuments(std::execution::par,
							raw_query,
							[status](int document_id, DocumentStatus document_status, int rating) {
								return document_status == status;
						});
}

std::vector<Document>
SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}
std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const std::map<std::string_view , double>&
SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view , double> empty_word_freq;

    if (document_to_word_freqs_.count(document_id) == 0) {
        return empty_word_freq;
    }

    return document_to_word_freqs_.at(document_id);
}

MatchedDocuments SearchServer::MatchDocument(std::string_view raw_query,
											 int document_id) const {
	return MatchDocument(std::execution::seq, raw_query, document_id);
}

MatchedDocuments SearchServer::MatchDocument(const SEQ&,
											 std::string_view raw_query,
											 int document_id) const {

    const auto query = ParseQuery(raw_query);

	bool is_minus_consist =
			std::any_of(query.minus_words.begin(), query.minus_words.end(),
						[this, document_id](std::string_view minus_word){
							return  document_to_word_freqs_.at(document_id).count(minus_word)
									&& word_to_document_freqs_.count(minus_word);
					});

	if (is_minus_consist) {
		return { std::vector<std::string_view> {}, // возвращаем пустой вектор в случае наличия минус-слов
		         documents_.at(document_id).status };
	}

    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

MatchedDocuments
SearchServer::MatchDocument(const PAR&, std::string_view raw_query, int document_id) const {
    // явно заявляем, что сортировка не нужна
	if (raw_query.empty()) {
		return {};
	}
    const auto query = ParseQuery(raw_query, false);
    std::vector<std::string_view> matched_words {}; // задаём пустой вектор умышленно

	bool is_minus_consist =
			std::any_of(std::execution::par,
						query.minus_words.begin(), query.minus_words.end(),
						[this, document_id](std::string_view minus_word){
							return  document_to_word_freqs_.at(document_id).count(minus_word)
									&& word_to_document_freqs_.count(minus_word);
					});

    if (is_minus_consist) {
        return { matched_words, // возвращаем пустой вектор в случае наличия минус-слов
				 documents_.at(document_id).status };
    }

    // если минус-слов не нашлось,
    // делаем ресайз размером с вектор плюс-слов и заполняем его
    // распарсенными плюс-словами, которые есть в word_to_document_freqs и по id -- в document_to_word_freqs_
    matched_words.resize(query.plus_words.size());
	std::copy_if(std::execution::par,
				 query.plus_words.begin(), query.plus_words.end(),
				 matched_words.begin(),
				 [this, document_id] (std::string_view plus_word) {
					return word_to_document_freqs_.count(plus_word)
						   && document_to_word_freqs_.at(document_id).count(plus_word);
            });

    // приводим вектор в порядок std::sort --> std::unique --> erase
    std::sort(std::execution::par,
              matched_words.begin(), matched_words.end());

    auto last = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());

    if (matched_words.begin()->empty()) {
        matched_words.erase(matched_words.begin());
    }

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view>
SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    using namespace std;
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord
SearchServer::ParseQueryWord(std::string_view text) const {
    using namespace std::string_literals;
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw std::invalid_argument("Query word is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query
SearchServer::ParseQuery(std::string_view text, bool is_need_sort) const {
    Query result;
    std::vector<std::string_view> split_result = SplitIntoWords(text);

    // !!! подготовка контейнера ТОЛЬКО для непараллельной обработки
    if (is_need_sort) {
        std::sort(split_result.begin(), split_result.end());
        auto last = std::unique(split_result.begin(), split_result.end());
        split_result.erase(last, split_result.end());
    }

	for (std::string_view word : split_result) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
            if (query_word.is_minus) {
				result.minus_words.push_back(query_word.data);
			} else {
				result.plus_words.push_back(query_word.data);
			}
		}
    }

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void SearchServer::RemoveDocument(int document_id) {
    return RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const SEQ&, int document_id) {

    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }

    for (const auto& [word, _] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }

	document_ids_.erase(document_id);
	documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const PAR&, int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
		return;
	}

    const auto& word_freqs_ = document_to_word_freqs_.at(document_id);
    std::vector<std::string_view> words_(word_freqs_.size());
	std::transform(std::execution::par,
				   word_freqs_.begin(), word_freqs_.end(),
				   words_.begin(),
				   [] (const auto word_freq) {
						return word_freq.first;
				});
    std::for_each(std::execution::par,
                  words_.begin(), words_.end(),
                  [this, document_id](const std::string_view word){
                      word_to_document_freqs_.at(word).erase(document_id);
				});

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const SEQ&, std::string_view raw_query) const {
	return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const PAR&, std::string_view raw_query) const {
	return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}
