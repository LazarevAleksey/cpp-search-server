// в качестве заготовки кода используйте последнюю версию своей поисковой системы
#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> duplicate;
    auto st = search_server.RetDoc().begin();
    auto end = search_server.RetDoc().end();

    for (; st != end; st++) {
        auto in = st->second;  // множество слов 1-го документа
        auto tt = st;
        for (auto strt = (++tt); strt != end; strt++) {
            if (in == strt->second) {
                duplicate.insert(strt->first);
            }
        }

    }
    //for (auto i : duplicate) {
//    std::cerr << "Found duplicate document id " << i << std::endl;
//}
    for (const int i : duplicate) {
        search_server.RemoveDocument(i);
    }
}
