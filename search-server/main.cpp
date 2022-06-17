#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include "remove_duplicates.h"
#include "log_duration.h"

#include <locale.h>


using namespace std;

int main() {
    setlocale(LC_ALL, "Rus");
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });
   
    /*cerr << "Freqs: " << search_server.GetWordFrequencies(2).at("funny") << endl;
    cerr << "Freqs: " << search_server.GetWordFrequencies(99).at("funny") << endl;
    */
    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });
    
    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    {
       // LOG_DURATION("RemoveDuplicates");
        LOG_DURATION_STREAM("RemoveDuplicates", cout);
        //LogDuration guard("Long task", cout);
        RemoveDuplicates(search_server);
    }
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;

    {
       // LOG_DURATION("MatchDocuments");
        LOG_DURATION_STREAM("MatchDocuments", cout);
        MatchDocuments(search_server, "пушистый -пёс"s);
    }
    
    {
       // LOG_DURATION("FindTopDocuments");
        LOG_DURATION_STREAM("FindTopDocuments", cout);
        FindTopDocuments(search_server, "пушистый -кот"s);
    }
    
}

// Main with old lesson.
//int main() {
//    SearchServer search_server("and in at"s);
//    RequestQueue request_queue(search_server);
//
//    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
//    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
//    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
//    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
//    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
//
//    // 1439 запросов с нулевым результатом
//    for (int i = 0; i < 1439; ++i) {
//        request_queue.AddFindRequest("empty request"s);
//    }
//    // все еще 1439 запросов с нулевым результатом
//    request_queue.AddFindRequest("curly dog"s);
//    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
//    request_queue.AddFindRequest("big collar"s);
//    // первый запрос удален, 1437 запросов с нулевым результатом
//    request_queue.AddFindRequest("sparrow"s);
//    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
//}

