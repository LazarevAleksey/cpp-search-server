// в качестве заготовки кода используйте последнюю версию своей поисковой системы
#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server) {
	LOG_DURATION_STREAM("RemoveDuplicates", cout);
	set<int> duplicate;
	map<set<std::string>, int> dmap;
	set<string> temp_set;

	for (auto st = search_server.begin(); st != search_server.end(); st++) {
		int doc_id = st->first;
		auto mymap = search_server.GetWordFrequencies(doc_id); // по id получили словарь первого документа
		temp_set.clear();
		for (auto ss = mymap.begin(); ss != mymap.end(); ss++) {
			temp_set.insert(ss->first);
		}
		if (st == search_server.begin()) {
			dmap[temp_set] = doc_id;
		}
		else {
			bool r = !dmap.count(temp_set) > 0;
			if (r) {
				dmap[temp_set] = doc_id;
			}
			else {
				duplicate.insert(doc_id);
			}
		}
	}

	for (const int i : duplicate) {
		search_server.RemoveDocument(i);
	}
}
