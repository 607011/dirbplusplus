/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
*/

#ifndef __UTIL_CPP__
#define __UTIL_CPP__

#include <string>
#include <vector>
#include <utility>

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

}

#endif // __UTIL_CPP__
