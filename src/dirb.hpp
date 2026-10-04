/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023-2026 Oliver Lau <oliver@ersatzworld.net>
 */

#ifndef __DIRB_HPP__
#define __DIRB_HPP__

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>
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
            std::lock_guard<std::mutex> lock(queue_mutex_);
            this->url_queue_ = url_queue;
            this->seen_urls_.clear();
            auto queue_copy = this->url_queue_;
            while (!queue_copy.empty())
            {
                this->seen_urls_.insert(queue_copy.front());
                queue_copy.pop();
            }
        }
        inline bool enqueue_url(std::string const &url)
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (seen_urls_.contains(url))
            {
                return false;
            }
            seen_urls_.insert(url);
            url_queue_.emplace(url);
            return true;
        }
        inline void add_to_queue(std::string const &url)
        {
            enqueue_url(url);
        }
        inline size_t url_queue_size() const
        {
            return url_queue_.size();
        }
        inline void set_status_code_filter(std::unordered_map<int, bool> const &codes)
        {
            this->status_codes_ = codes;
        }

        void http_worker();
        void logger_worker();
        void stop_logger();
        httplib::Result send_request(httplib::Client &cli, std::string const &url) const;

        static const std::string DefaultUserAgent;
        static const std::unordered_map<int, bool> DefaultStatusCodeFilter;

    private:
        std::string base_url_{};
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
        std::unordered_set<std::string> seen_urls_;
        std::mutex queue_mutex_;
        std::queue<std::string> log_queue_;
        std::mutex log_queue_mutex_;
        std::condition_variable log_cv_;
        std::atomic_bool do_quit_{false};
        std::atomic_bool logger_stop_{false};
        std::unordered_map<int, bool> status_codes_{DefaultStatusCodeFilter};

        void enqueue_log_message(std::string const &message, bool is_error);
        void log(std::string const &message);
        void error(std::string const &message);
    };

}
#endif // __DIRB_HPP__
