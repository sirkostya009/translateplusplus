#ifndef TRANSLATOR_HPP
#define TRANSLATOR_HPP

#include "json.hpp"

using json = nlohmann::json;

class Translator {
    struct case_insensitive_comparator {
        static bool compare(unsigned char c1, unsigned char c2) {
            return tolower(c1) < tolower(c2);
        }

        bool operator()(const std::string& s1, const std::string& s2) const {
            return std::lexicographical_compare(
                    s1.begin(), s1.end(),
                    s2.begin(), s2.end(),
                    &case_insensitive_comparator::compare
            );
        }
    };

    std::map<std::string, std::string, case_insensitive_comparator> dictionary;
public:
    void set_dictionary(const json&);

    std::string translate_sentence(std::string string);

    void translate_file(const std::string& source, const std::string& path);
};

#endif
