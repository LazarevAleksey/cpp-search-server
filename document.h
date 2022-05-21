//Вставьте сюда своё решение из урока «‎Очередь запросов».‎
#pragma once
#include <vector>

//#include "search_server.h"


struct Document {
	Document(); 
	Document(int id, double relevance, int rating);
	        
	    int id=0;
	    double relevance = 0.0;
	    int rating = 0;
	};

// document.h
enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

std::ostream& operator<<(std::ostream& out, const Document& document);

void PrintDocument(const Document& document);

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);

