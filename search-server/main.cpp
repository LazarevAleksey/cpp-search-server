#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <iomanip>


using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ETALON_RELEVANCE = 1e-6;


vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{
                ComputeAverageRating(ratings),
                status
            });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < ETALON_RELEVANCE) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;

    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query,
            [status](int document_id, DocumentStatus status_pr, int rating) { return status_pr == status; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    /*int rating;
    DocumentStatus status;*/
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_; //хранит id документа, рейтинг и статус

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        // int tt= rating_sum / static_cast<int>(ratings.size());
        return rating_sum / static_cast<int>(ratings.size());
        //return tt;
    }

    //string data;
    //bool is_minus;
    //bool is_stop;
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1); //substr(1) - обрезает первый символ.
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    /*set<string> plus_words;
    set<string> minus_words;*/
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Predikat>
    vector<Document> FindAllDocuments(const Query& query, Predikat filter) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                auto doc_filter = documents_.at(document_id);
                if (filter(document_id, doc_filter.status, doc_filter.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
                });
        }
        return matched_documents;
    }
};

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
//template<typename Data>
//void Print(ostream& out, const Data& container) {
//    for (const Document& element : container) {
//        out << element;
//    }
//}

template<typename Element>
ostream& operator<<(ostream& out, const vector<Element>& v) {
    for (const auto& element : v) {
        out << element;
    }
    return out;
}


void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}
//#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(a) a()

// -------- Начало модульных тестов поисковой системы ----------
//void PrintDocument(const Document& document) {
//    cout << "{ "s
//        << "document_id = "s << document.id << ", "s
//        << "relevance = "s << document.relevance << ", "s
//        << "rating = "s << document.rating
//        << " }"s << endl;
//}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
void TestUnExcludeMinusWordIntoQuery() {
    //создаем документ
        //если в документе содержится минус слово, документ исключается из поиска
        //минус-слова исключают из результатов поиска документы, содержащие такие слова
    const int doc_id1 = 40;
    const int doc_id2 = 41;
    const int doc_id3 = 42;
    const int doc_id4 = 43;
    const string content1 = "cat in the city"s;
    const string content2 = "dog and cat is friend"s;
    const string content3 = "man walk in town"s;
    const string content4 = "fish slim ni Red sea"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        for (int i = 0; i < 4; i++) {
            auto current_status = static_cast<DocumentStatus>(i);
            SearchServer server;
            server.AddDocument(doc_id1, content1, current_status, ratings);
            server.AddDocument(doc_id2, content2, current_status, ratings);
            server.AddDocument(doc_id3, content3, current_status, ratings);
            server.AddDocument(doc_id4, content4, current_status, ratings);

            //Проверяем, что найденые документы без минус слов
            const vector<Document> found_docs = server.FindTopDocuments("cat -dog"s);
            if (i == 0) {
                //assert(found_docs.size() == 1);
                ASSERT_EQUAL(found_docs.size(), 1);
                //Проверяем, что найден документ не содержит минус слов
                const Document& doc0 = found_docs[0];
                //assert(doc0.id == doc_id1);
                ASSERT_EQUAL(doc0.id, doc_id1);
            }
            else {
                //assert(found_docs.size() == 0);
                ASSERT_EQUAL(found_docs.size(), 0);
            }
        }
    }
}
void TestSortReval() {
        const int doc_id1 = 0;
        const int doc_id2 = 1;
        const int doc_id3 = 2;
        const string content1 = "белый кот и модный ошейник"s;
        const string content2 = "пушистый кот пушистый хвост"s;
        const string content3 = "ухоженный пёс выразительные глаза"s;
    {
        
        const vector<int> ratings = { 1, 2, 3 };
        //const vector<int> ratings = {  };
        SearchServer server;
        for (unsigned i = 0; i < 4; i++) {
            auto current_status = static_cast<DocumentStatus>(i);

            server.AddDocument(doc_id1, content1, current_status, ratings);
            server.AddDocument(doc_id2, content2, current_status, ratings);
            server.AddDocument(doc_id3, content3, current_status, ratings);

            vector<Document> rev = server.FindTopDocuments("пушистый ухоженный кот"s, current_status);
            for (unsigned i = 0; i < rev.size(); i++) {
                if (i != rev.size() - 1) {
                    ASSERT(rev[i].relevance >= rev[i + 1].relevance);
                }
                cerr << rev[i].relevance << endl;
            }
        }
    }
    {
            /*0.650672
            0.274653
            0.081093*/
        const vector<int> ratings = { 1, 2, 3 };
        //const vector<double> et_rat = { 0.650672, 0.274653, 0.081093 };
        const vector<double> et_rat = { 0.650672, 0.274653, 0.081093 };
        SearchServer server;
        for (unsigned i = 0; i < 4; i++) {
            auto current_status = static_cast<DocumentStatus>(i);
            server.AddDocument(doc_id1, content1, current_status, ratings);
            server.AddDocument(doc_id2, content2, current_status, ratings);
            server.AddDocument(doc_id3, content3, current_status, ratings);
            vector<Document> rev = server.FindTopDocuments("пушистый ухоженный кот"s, current_status);
            for (unsigned i = 0; i < rev.size(); i++) {
                ASSERT(abs(rev[i].relevance - et_rat[i]) < ETALON_RELEVANCE);
            }
        }
    }
    {
        SearchServer search_server;
        search_server.SetStopWords("и в на"s);

        search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
        search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

        Document doc1 = { 1, 0.866434,  5 };
        Document doc2 = { 0, 0.173287, 2 };
        Document doc3 = { 2, 0.173287, -1 };
        const vector<Document> etalon1 = { doc1, doc2, doc3 };
   
        const vector <Document> etalon2 = { { 3, 0.231049, 9 } };
        const vector <Document> etalon3 = { {  0, 0.173287,  2 },
                                            { 2, 0.173287, -1 } };
        //cout << "ACTUAL by default:"s << endl;
       // for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        //const vector<Document> docc = search_server.FindTopDocuments("пушистый ухоженный кот"s);

        const vector<Document>& document = search_server.FindTopDocuments("пушистый ухоженный кот"s,
            [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(document, etalon1);
         
    }
}
void TestSortRating() {
    const int doc_id = 0;
    const string content = "cat in the city"s;
    const vector<int> ratings_positiv = { 1, 2, 3 };
    const vector<int> ratings_negativ = { -1, -2, -3 };
    const vector<int> ratings_mixed = { 1, -2, 3 };
    const vector<int> ratings_zero = {};
    const vector<int> ratings_etalon = { 2, -2, 0, 0 };
    const vector<vector<int>> rating = { ratings_positiv, ratings_negativ, ratings_mixed, ratings_zero };

    {
        SearchServer server;
        // Позитивный тест
        // Отсутствие оценок, ожидаемый рейтинг равен нулю.
        int cnt = 0;
        for (auto ratings : rating) {
            for (int i = 0; i < 4; i++) {
                auto current_status = static_cast<DocumentStatus>(i);
                server.AddDocument(doc_id + cnt, content, current_status, ratings);
                vector<Document> found_docs = server.FindTopDocuments("cat"s, current_status);
                sort(found_docs.begin(), found_docs.end(), [](Document& l, Document& r) { return l.id < r.id; });
                if (i == 0) {
                    ASSERT_EQUAL(found_docs[cnt].rating, ratings_etalon[cnt]);
                }
                else {
                    if (cnt == 0) {
                        ASSERT_EQUAL(found_docs.size(), 0);
                    }
                }
            }
            cnt++;
        }
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestUnExcludeMinusWordIntoQuery);
    RUN_TEST(TestSortReval);
    RUN_TEST(TestSortRating);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
    return 0;
}
