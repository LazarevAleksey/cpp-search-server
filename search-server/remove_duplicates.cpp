// в качестве заготовки кода используйте последнюю версию своей поисковой системы
#include "remove_duplicates.h"


template <typename Map>
bool CompareMap(Map const& lhs, Map const& rhs) {

    auto pred = [](auto a, auto b)
    { return a.first == b.first; };

    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(), rhs.begin(), pred);
}

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> duplicate;

    for (auto st = search_server.begin(); st != search_server.end(); st++) {
       auto mymap = search_server.GetWordFrequencies((st->first)); // по id получили словарь первого документа
       auto t = st;
       auto st2 = (++t);
       for (; st2 != search_server.end(); st2++) {
          auto mymap1 = search_server.GetWordFrequencies((st2->first));
           bool f = CompareMap(mymap, mymap1);
           if (f) {
               duplicate.insert(st2->first);
           }
       }
    }

    for (const int i : duplicate) {
        search_server.RemoveDocument(i);
    }
}
