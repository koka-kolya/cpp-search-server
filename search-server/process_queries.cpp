#include "process_queries.h"

std::vector<std::vector<Document>>
ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> output(queries.size());
	std::transform(std::execution::par,
				   queries.begin(), queries.end(),
				   output.begin(),
				   [&search_server] (const std::string& query) {
						return search_server.FindTopDocuments(query);
				});
    return output;
}

std::vector<Document>
ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<Document> output;
    for (const auto& documents : ProcessQueries(search_server, queries)) {
        output.insert(output.end(), documents.begin(), documents.end());
    }
    return output;
}


