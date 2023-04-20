/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
*/

#ifndef __DIRB_HPP__
#define __DIRB_HPP__

#include <mutex>
#include <sstream>
#include <string>
#include <queue>
#include <vector>

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include <httplib.h>


namespace dirb
{

    namespace http
    {
        enum verb
        {
            del,
            get,
            head,
            options,
            patch,
            post,
            put,
        };
    }

    struct dirb_options
    {
        std::string base_url{};
        std::mutex &output_mutex;
        bool follow_redirects{false};
        httplib::Headers headers{};
        std::string bearer_token{};
        std::vector<std::string> probe_variations{};
        std::string username{};
        std::string password{};
        std::string body{};
        bool verify_certs{false};
        http::verb method{http::verb::get};
        unsigned int version{11};

        void log(std::string const &message) const
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << message << std::endl;
        }

        void error(std::string const &message) const
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cerr << message << std::endl;
        }
    };

    void http_worker(std::queue<std::string> &url_queue, std::mutex &queue_mtx, std::atomic_bool &do_quit, dirb_options dirb_opts);
}
#endif // __DIRB_HPP__
