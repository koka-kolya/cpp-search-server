#include "remove_duplicates.h"
#include "log_duration.h"
#include <tuple>
#include <algorithm>
#include <iterator>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
//	LOG_DURATION("RemoveDuplicates"s);
	std::vector<int> duplicate_id;
	duplicate_id.reserve(search_server.GetDocumentCount());
	std::map<string, double> words_freq;
	std::map<int, std::string> id_words;
	
	for (const int id : search_server) {
		std::string doc;
		for (const auto& [word, freq] : search_server.GetWordFrequencies(id)) {
			doc += (word + " "s); //т.к. в словаре word упорядочены, то просто составляем строку из всех слов для документа для последующего сравнения
		}
		const auto it = find_if(id_words.begin(), id_words.end(), [&doc](const std::pair<int, std::string>& iw1) {
			return iw1.second == doc;
		});
		if (it->first < id && it != id_words.end()) { //если текущий id больше найденного id совпадения, не добавляем в id_words, а вносим в вектор для дубликатов id
			duplicate_id.push_back(id);
			continue;
		} else {
			id_words[id] = doc;
		}
	}
	
	for (const int id : duplicate_id) {
		cout << "Found duplicate document id "s << id << endl;
		search_server.RemoveDocument(id);
	}
}
   
