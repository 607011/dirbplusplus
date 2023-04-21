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

    class dirb_runner
    {
    public:
        dirb_runner() = delete;
        dirb_runner(std::string const &base_url, std::mutex &output_mutex, bool follow_redirects, httplib::Headers const &headers, std::string const &bearer_token, std::vector<std::string> const &probe_variations, std::string const &username, std::string const &password, std::string const &body, bool verify_certs, http::verb method, unsigned int version, std::queue<std::string> &url_queue, std::mutex &queue_mtx, std::atomic_bool &do_quit)
            : base_url(base_url), output_mutex(output_mutex), follow_redirects(follow_redirects), headers(headers), bearer_token(bearer_token), probe_variations(probe_variations), username(username), password(password), body(body), verify_certs(verify_certs), method(method), version(version), url_queue(url_queue), queue_mtx(queue_mtx), do_quit(do_quit)
        {
        }
        void http_worker();

    private:
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
        std::queue<std::string> &url_queue;
        std::mutex &queue_mtx;
        std::atomic_bool &do_quit;

        void log(std::string const &message) const;
        void error(std::string const &message) const;
    };

    
}
#endif // __DIRB_HPP__
