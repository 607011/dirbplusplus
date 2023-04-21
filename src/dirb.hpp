/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
 */

#ifndef __DIRB_HPP__
#define __DIRB_HPP__

#include <atomic>
#include <mutex>
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

    class dirb_runner final
    {
    public:
        dirb_runner(){};
        dirb_runner(dirb_runner const &) = delete;
        dirb_runner(dirb_runner &&) = delete;
        inline void set_headers(httplib::Headers const &headers)
        {
            this->headers_ = headers;
        }
        inline void add_header(std::string const &header, std::string const &value)
        {
            headers_.emplace(header, value);
        }
        inline void add_header(std::pair<std::string, std::string> const &hv)
        {
            headers_.emplace(hv);
        }
        inline bool has_base_url() const
        {
            return !base_url_.empty();
        }
        inline void set_base_url(std::string const &base_url)
        {
            this->base_url_ = base_url;
        }
        inline void set_username(std::string const &username)
        {
            this->username_ = username;
        }
        inline void set_password(std::string const &password)
        {
            this->password_ = password;
        }
        inline void set_body(std::string const &body)
        {
            this->body_ = body;
        }
        inline void set_bearer_token(std::string const &bearer_token)
        {
            this->bearer_token_ = bearer_token;
        }
        inline void set_method(http::verb method)
        {
            this->method_ = method;
        }
        inline void set_verify_certs(bool verify_certs)
        {
            this->verify_certs_ = verify_certs;
        }
        inline void set_follow_redirects(bool follow_redirects)
        {
            this->follow_redirects_ = follow_redirects;
        }
        inline void set_probe_variations(std::vector<std::string> const &probe_variations)
        {
            this->probe_variations_ = probe_variations;
        }
        inline void set_url_queue(std::queue<std::string> const &url_queue)
        {
            this->url_queue_ = url_queue;
        }
        inline void add_to_queue(std::string const &url)
        {
            url_queue_.emplace(url);
        }
        inline size_t url_queue_size() const
        {
            return url_queue_.size();
        }

        void http_worker();

        static const std::string DefaultUserAgent;

    private:
        std::string base_url_{};
        std::mutex output_mutex_;
        bool follow_redirects_{false};
        httplib::Headers headers_{};
        std::string bearer_token_{};
        std::vector<std::string> probe_variations_{};
        std::string username_{};
        std::string password_{};
        std::string body_{};
        bool verify_certs_{false};
        http::verb method_{http::verb::get};
        std::queue<std::string> url_queue_;
        std::mutex queue_mutex_;
        std::atomic_bool do_quit_{false};

        void log(std::string const &message);
        void error(std::string const &message);
    };

}
#endif // __DIRB_HPP__
