#include "Translator.hpp"

#include <regex>
#include <fstream>

auto word_regex = std::regex{R"(\w+|\s{2,}|\n|\r|\t)"};

void Translator::set_dictionary(const json& js) {
    dictionary = js;
}

std::string Translator::translate_sentence(std::string string) {
    auto result = std::string();
    result.reserve(string.size());

    for (auto match = std::smatch(); std::regex_search(string, match, word_regex); string = match.suffix()) {
        auto word = match.str();

        result += (dictionary.find(word) != dictionary.end() ? dictionary[word] : word);

        if (result[result.size() - 1] != '\n'
            || result[result.size() - 1] != '\r'
            || result[result.size() - 1] != '\t'
            || result[result.size() - 1] != ' ') {
            result += ' ';
        }
    }

    return result;
}

void Translator::translate_file(const std::string& source, const std::string& path) {
    auto in = std::ifstream(source);
    auto out = std::ofstream(path);
    auto line = std::string();

    while (std::getline(in, line)) {
        out << translate_sentence(line) << '\n';
    }
}
