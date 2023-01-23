#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;

    int64_t end_pos = str.npos;

    while (!str.empty()) {
        str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
        int64_t space = str.find(' ') ? str.find(' ') : end_pos;
        if (str[0] == ' ' || str.empty()) return result;
        result.push_back(str.substr(0, space));
        str.remove_prefix(space);
        if (space == end_pos) return result;
    }
    return result;
}
