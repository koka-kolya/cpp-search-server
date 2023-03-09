#pragma once
#include "log_duration.h"
#include "search_server.h"
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

void PrintDocument(const Document& document);
void Example1();
void Example2();
void Example3();


std::string GenerateWord(std::mt19937& generator, int max_length);

std::vector<std::string> GenerateDictionary(std::mt19937& generator,
											int word_count,
											int max_length);

std::string GenerateQuery(std::mt19937& generator,
						  const std::vector<std::string>& dictionary,
						  int word_count,
						  double minus_prob = 0);

std::vector<std::string> GenerateQueries(std::mt19937& generator,
										 const std::vector<std::string>& dictionary,
										 int query_count,
										 int max_word_count);

template <typename ExecutionPolicy>
void Test(std::string_view mark, const SearchServer& search_server,
		  const std::vector<std::string>& queries, ExecutionPolicy&& policy);
template <typename ExecutionPolicy>
void Test(std::string_view mark, SearchServer search_server,
		  const std::string& query, ExecutionPolicy&& policy);

void Benchmark1();
void Benchmark2();

template <typename ExecutionPolicy>
void Test(std::string_view mark, const SearchServer& search_server,
		  const std::vector<std::string>& queries, ExecutionPolicy&& policy) {
	LOG_DURATION(mark);
	double total_relevance = 0;
	for (const std::string_view query : queries) {
		for (const auto& document : search_server.FindTopDocuments(policy, query)) {
			total_relevance += document.relevance;
		}
	}
	std::cout << total_relevance << std::endl;
}

template <typename ExecutionPolicy>
void Test(std::string_view mark, SearchServer search_server,
		  const std::string& query, ExecutionPolicy&& policy) {
	LOG_DURATION(mark);
	const int document_count = search_server.GetDocumentCount();
	int word_count = 0;
	for (int id = 0; id < document_count; ++id) {
		const auto [words, status] = search_server.MatchDocument(policy, query, id);
		word_count += words.size();
	}
	std::cout << word_count << std::endl;
}
