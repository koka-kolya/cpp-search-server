#include "document.h"

Document::Document(int id, double relevance, int rating)
: id(id)
, relevance(relevance)
, rating(rating) 
{
}

std::ostream& operator<<(std::ostream& output, Document document) {
	using namespace std::string_literals;
	output << "{ "s
	<< "document_id = "s << document.id << ", "s
	<< "relevance = "s << document.relevance << ", "s
	<< "rating = "s << document.rating << " }"s;
	return output;
}
