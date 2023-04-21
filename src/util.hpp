/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
 */

#ifndef __UTIL_CPP__
#define __UTIL_CPP__

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace util
{

    std::vector<std::string> split(const std::string &str, char delim)
    {
        std::vector<std::string> strings;
        size_t start;
        size_t end = 0;
        while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
        {
            end = str.find(delim, start);
            strings.push_back(str.substr(start, end - start));
        }
        return strings;
    }

    std::pair<std::string, std::string> unpair(const std::string &str, char delim)
    {
        std::pair<std::string, std::string> pair;
        auto index = str.find(delim);
        if (index != std::string::npos)
        {
            pair = std::make_pair(str.substr(0, str.size() - index),
                                  str.substr(index + 1, std::string::npos));
            while (pair.second.front() == ' ')
            {
                pair.second.erase(0, 1);
            }
        }
        return pair;
    }

    template <typename InputIteratorT, typename SeparatorT>
    std::string join(InputIteratorT input, SeparatorT separator)
    {
        std::ostringstream result;
        auto i = std::begin(input);
        if (i != std::end(input))
        {
            result << *i++;
        }
        while (i != std::end(input))
        {
            result << separator << *i++;
        }
        return result.str();
    }
}

#endif // __UTIL_CPP__
