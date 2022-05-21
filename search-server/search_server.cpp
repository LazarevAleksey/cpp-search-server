//Вставьте сюда своё решение из урока «‎Очередь запросов».‎
#include "search_server.h"

//using namespace std;


    void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
        const std::vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw std::invalid_argument("Invalid document_id");
        }
        const auto words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        document_ids_.push_back(document_id);
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

    int SearchServer::GetDocumentId(int index) const {
        return document_ids_.at(index);
    }

    std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
        int document_id) const {
        const auto query = ParseQuery(raw_query);

        std::vector<std::string> matched_words;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const std::string& word : query.minus_words) {
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

// private:
    bool SearchServer::IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }


    std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("Word " + word + " is invalid");
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    // Existence required
    double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
        double res = GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size();
        double res_ret = log(res);
        // long res_ret = std::log(res);
        return res_ret;
    }

    void AddDocument(SearchServer& search_server, int document_id, const std::string& document,
        DocumentStatus status, const std::vector<int>& ratings) {
        try {
            search_server.AddDocument(document_id, document, status, ratings);
        }
        catch (const std::exception& e) {
            std::cout << "Error in adding document "<< document_id << ": " << e.what() << std::endl;
        }
    }

    void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
        std::cout << "Results for request: " << raw_query << std::endl;
        try {
            for (const Document& document : search_server.FindTopDocuments(raw_query)) {
                PrintDocument(document);
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error is seaching: "<< e.what() << std::endl;
        }
    }

    void MatchDocuments(const SearchServer& search_server, const std::string& query) {
        try {
            std::cout << "Matching for request: " << query << std::endl;
            const int document_count = search_server.GetDocumentCount();
            for (int index = 0; index < document_count; ++index) {
                const int document_id = search_server.GetDocumentId(index);
                const auto [words, status] = search_server.MatchDocument(query, document_id);
                PrintMatchDocumentResult(document_id, words, status);
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error in matchig request "<< query << ": " << e.what() << std::endl;
        }
    }
