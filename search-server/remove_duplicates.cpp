#include "remove_duplicates.h"
#include "log_duration.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
	//	LOG_DURATION("RemoveDuplicates"s);
	std::set<int> duplicate_id;
	std::map<std::set<string>, int> words_id;
	
	for (const int id : search_server) {
		std::set<string> words;
		for (const auto& [word, freqs] : search_server.GetWordFrequencies(id)) {
			words.insert(word);
		}
		// если такое множество уже есть, заносим id в дубликаты
		if (words_id.count(words) != 0) {
			duplicate_id.insert(id);
		} else {
			words_id[words] = id;
		}
	}
	//удаляем дубликаты по id
	for (const int id : duplicate_id) {
		cout << "Found duplicate document id "s << id << endl;
		search_server.RemoveDocument(id);
	}
}
