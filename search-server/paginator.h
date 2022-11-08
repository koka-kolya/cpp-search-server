#pragma once
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <utility>

#include "document.h"

template<typename Iterator>
class IteratorRange {
public:
	explicit IteratorRange(Iterator begin, Iterator end) {
		iterator_range_ = pair(begin, end);
	}
	Iterator begin() {
		return iterator_range_.first;
	}
	Iterator end() {
		return iterator_range_.second;
	}
	size_t size() {
		return distance(iterator_range_.first, iterator_range_.second);
	}
private:
	std::pair<Iterator, Iterator> iterator_range_;
};

template <typename Iterator>
class Paginator {
public:
	explicit Paginator(Iterator begin, Iterator end, size_t page_size) {
		if (page_size != 0) {
			for (auto it = begin; it < end; ) {
				if (distance(it, end) < page_size) {
					IteratorRange<Iterator> page(it, end - 1);
					pages_.push_back(page);
					it = end;
				} else {
					auto it_ps = it;
					advance(it_ps, page_size - 1);
					IteratorRange<Iterator> page(it, it_ps);
					pages_.push_back(page);
					advance(it, page_size);
				}
			}
		}
	}
	auto begin() const {
		return pages_.begin();
	}
	auto end() const {
		return pages_.end();
	}
	size_t size() const {
		return pages_.size();
	}
private:
	std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
	return Paginator(begin(c), end(c), page_size);
}

std::ostream& operator<<(std::ostream& output, Document document);

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, IteratorRange<Iterator> iterator_range) {
	using namespace std::string_literals;
	for (auto it = iterator_range.begin(); it <= iterator_range.end(); ++it) {
		output << *it;
	}
	return output;
}
