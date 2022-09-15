/*
 * Sprint 1. Search-server. TF-IDF sort.
 */
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
//Return inputed string with spaces
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
/*
 * Return inputed number
 */
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
/*
 * Recieves text and split it to single words, clear from spaces, and return vector of words
 */
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}
//struct for matched documents
struct Document {
    int id;
    double relevance;
};
//struct for query-words
struct Query {
    set<string> minus_words;
    set<string> plus_words;
};

class SearchServer {
    
public:
    /*
     * Recieves input a string with stop-words, split to single words and insert its to container for stop-words
     */
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    /*
     * Recieves from input documents ID and text, parse text to words, calculate TF-index for single word and insert its to container {word, {ID, TF}}
     */
    void AddDocument(int document_id, const string& document) {
        ++document_count_;
//        int match_word_count = 0;
        const vector<string> words = SplitIntoWordsNoStop(document);
        double word_tf = 0.0;
        if(!document.empty()) {
            for (const string& word : words) {
                map<int, double> docs_id_tf;
                word_tf = static_cast<double>(count(words.begin(), words.end(), word))/words.size();
                documents_to_freqs_[word].insert({document_id, word_tf});
            }
        }
    }
    /*
     * Receives input query, sorting for descending relevance, and resize to max value output
     */
    vector<Document> FindTopDocuments(const string& raw_query) const {
        vector<Document> matched_documents;
        if(!raw_query.empty()){
            matched_documents = FindAllDocuments(raw_query);
            sort(matched_documents.begin(), matched_documents.end(),
                 [](const Document& lhs, const Document& rhs) {
                     return lhs.relevance > rhs.relevance;
                 });
            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
        }
        return matched_documents;
    }

private:
    map<string, map<int, double>> documents_to_freqs_; //{word, {ID, TF}}, где word - is document's word, ID - id's document what consist this word TF - the ratio of the number of repetitions to the number of words
    set<string> stop_words_; // container of stop-word
    int document_count_ = 0; // count documents on the server (calculate by the number of hits AddDocument)
    /*
     * Check if word is stop-word
     */
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    /*
     * Check if document contains minus-word
     */
    bool IsMinusWord(const string& word) const {
        Query query_words;
        return count(query_words.minus_words.begin(), query_words.minus_words.end(), word) > 0;
    }
    /*
     * Clear text from stop-words
     */
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }
    /*
     * Check if word contains "-"
     */
    static bool IsMinus(const string& word) {
        return count_if(word.begin(), word.end(), [](const char& ch){return ch == '-';}) > 0;
    }
    /*
     * Parse text and allocate to plus- and minus-words containers
     */
    Query ParseQuery (const string& text) const {
        Query query_words;
        vector<string> query_words_v = SplitIntoWords(text);
        for(const string& word : query_words_v) {
            if (IsMinus(word)) {
                if (!IsStopWord(word.substr(1))) {
                    query_words.minus_words.insert(word.substr(1));
                }
            } else {
                if (!IsStopWord(word)) {
                    query_words.plus_words.insert(word);
                }
            }
        }
        return query_words;
    }
    /*
     * Parse raw_query from input, calculate relevance, clear minus-words consist documents
     */
    vector<Document> FindAllDocuments(const string& raw_query) const {
        vector<Document> matched_documents;
        Query query_words = ParseQuery(raw_query);
        set<string> minus_words = query_words.minus_words;
        set<string> plus_words = query_words.plus_words;
        double idf = 0.0;
        double idf_tf = 0.0;
        map<int, double> relevance;
        set<int> num_doc_for_del;
        
        //Loop for plus word
        for (const string& plus_word : plus_words) {
            if (!documents_to_freqs_.count(plus_word)) { //if document don't consist plus-word, next document
                continue;
            }
            // Loop for doc what consist plus_word (get id and tf)
            // Calculate idf; idf*tf, add up to relevance
            for (const auto& [id, tf] : documents_to_freqs_.at(plus_word)) {
                idf = log(1.0*document_count_/documents_to_freqs_.at(plus_word).size());
                idf_tf = idf*tf;
                relevance[id] += idf_tf;
            }
        }
        
        // Erase docs with minus words from relevance
        for (const string& minus_word : minus_words) {
            if (!documents_to_freqs_.count(minus_word)) {
                continue;
            }
            for (const auto& [id, tf] : documents_to_freqs_.at(minus_word)) {
                relevance.erase(id);
            }
        }
        
        // Add matched_documents without docs what consist minus-word
        for (const auto& [id, rel] : relevance) {
            matched_documents.push_back({id, rel});
        }
        return matched_documents;
    }
};
//Create object of class
SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
