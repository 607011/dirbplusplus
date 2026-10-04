/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023-2026 Oliver Lau <oliver@ersatzworld.net>
 */

#include <algorithm>
#include <sstream>

#include "dirb.hpp"
#include "certs.hpp"

namespace dirb
{
    namespace
    {
        std::string safe_header_value(httplib::Response const &response, std::string const &header_name)
        {
            if (!response.has_header(header_name))
            {
                return {};
            }
            auto value = response.get_header_value(header_name);
            return value.empty() ? std::string{} : value;
        }
    }

    namespace util
    {
        X509_STORE *read_certificates(SSL_CTX *ssl_ctx, std::ostream &err)
        {
            BIO *cbio = BIO_new_mem_buf(reinterpret_cast<void *>(cacert_pem), static_cast<int>(cacert_pem_len));
            if (cbio == nullptr)
            {
                err << "\u001b[31;1mERROR:\u001b[0m CA certificates cannot be read (file: " << __FILE__ << ", line:" << __LINE__ << ")" << std::endl;
                return nullptr;
            }
            X509_STORE *cts = SSL_CTX_get_cert_store(ssl_ctx);
            if (cts == nullptr)
            {
                err << "\u001b[31;1mERROR:\u001b[0m X.509 store cannot be created (file: " << __FILE__ << ", line:" << __LINE__ << ")" << std::endl;
                return nullptr;
            };
            STACK_OF(X509_INFO) *inf = PEM_X509_INFO_read_bio(cbio, nullptr, nullptr, nullptr);
            if (inf == nullptr)
            {
                err << "\u001b[31;1mERROR:\u001b[0m X.509 info cannot be created (file: " << __FILE__ << ", line:" << __LINE__ << ")" << std::endl;
                BIO_free(cbio);
                return nullptr;
            }
            for (int i = 0; i < sk_X509_INFO_num(inf); i++)
            {
                X509_INFO *itmp = sk_X509_INFO_value(inf, i);
                if (itmp->x509)
                {
                    X509_STORE_add_cert(cts, itmp->x509);
                }
                if (itmp->crl)
                {
                    X509_STORE_add_crl(cts, itmp->crl);
                }
            }
            sk_X509_INFO_pop_free(inf, X509_INFO_free);
            BIO_free(cbio);
            return cts;
        }
    }

    const std::string dirb_runner::DefaultUserAgent = std::string(PROJECT_NAME) + "/" + PROJECT_VERSION;
    const std::unordered_map<int, bool> dirb_runner::DefaultStatusCodeFilter = {
        {200, true},
        {204, true},
        {301, true},
        {302, true},
        {307, true},
        {308, true},
        {401, true},
        {403, true}};

    void dirb_runner::enqueue_log_message(std::string const &message, bool is_error)
    {
        std::string entry = is_error ? "[ERROR] " + message : message;
        {
            std::lock_guard<std::mutex> lock(log_queue_mutex_);
            log_queue_.push(std::move(entry));
        }
        log_cv_.notify_one();
    }

    void dirb_runner::log(std::string const &message)
    {
        enqueue_log_message(message, false);
    }

    void dirb_runner::error(std::string const &message)
    {
        enqueue_log_message(message, true);
    }

    void dirb_runner::stop_logger()
    {
        logger_stop_.store(true);
        log_cv_.notify_all();
    }

    void dirb_runner::logger_worker()
    {
        while (true)
        {
            std::string message;
            {
                std::unique_lock<std::mutex> lock(log_queue_mutex_);
                log_cv_.wait(lock, [this]
                             { return logger_stop_.load() || !log_queue_.empty(); });
                if (logger_stop_.load() && log_queue_.empty())
                {
                    break;
                }
                message = std::move(log_queue_.front());
                log_queue_.pop();
            }

            if (message.rfind("[ERROR] ", 0) == 0)
            {
                std::cerr << message.substr(8) << std::endl;
            }
            else
            {
                std::cout << message << std::endl;
            }
        }
    }

    httplib::Result dirb_runner::send_request(httplib::Client &cli, std::string const &url) const
    {
        std::string content_type = "application/x-www-form-urlencoded";
        auto header_it = headers_.find("Content-Type");
        if (header_it != headers_.end())
        {
            content_type = header_it->second;
        }

        switch (method_)
        {
        case http::verb::get:
            return cli.Get(url.c_str());
        case http::verb::head:
            return cli.Head(url.c_str());
        case http::verb::post:
            if (body_.empty())
            {
                return cli.Post(url.c_str());
            }
            return cli.Post(url.c_str(), body_.c_str(), body_.size(), content_type);
        case http::verb::put:
            if (body_.empty())
            {
                return cli.Put(url.c_str());
            }
            return cli.Put(url.c_str(), body_.c_str(), body_.size(), content_type);
        case http::verb::patch:
            if (body_.empty())
            {
                return cli.Patch(url.c_str());
            }
            return cli.Patch(url.c_str(), body_.c_str(), body_.size(), content_type);
        case http::verb::del:
            if (body_.empty())
            {
                return cli.Delete(url.c_str());
            }
            return cli.Delete(url.c_str(), body_.c_str(), body_.size(), content_type);
        case http::verb::options:
            return cli.Options(url.c_str());
        default:
            return cli.Get(url.c_str());
        }
    }

    void dirb_runner::http_worker()
    {
        httplib::Client cli(base_url_.c_str());
        if (verify_certs_)
        {
            X509_STORE *cts = nullptr;
            cts = util::read_certificates(cli.ssl_context(), std::cerr);
            if (cts == nullptr)
            {
                return;
            }
            cli.set_ca_cert_store(cts);
        }
        cli.enable_server_certificate_verification(verify_certs_);
        if (!bearer_token_.empty())
        {
            cli.set_bearer_token_auth(bearer_token_.c_str());
        }
        if (!username_.empty() && !password_.empty())
        {
            cli.set_basic_auth(username_.c_str(), password_.c_str());
        }
        cli.set_follow_location(follow_redirects_);
        cli.set_compress(true);
        cli.set_default_headers(headers_);
        while (!do_quit_)
        {
            std::string url;
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (url_queue_.empty())
                {
                    return;
                }
                else
                {
                    url = url_queue_.front();
                    url_queue_.pop();
                }
            }
            if (url.empty())
            {
                continue;
            }
            if (url.front() != '/')
            {
                url = '/' + url;
            }
            if (httplib::Result res = send_request(cli, url))
            {
                std::stringstream ss;
                auto const &response = *res;
                ss << response.status << ';'
                   << '"' << url << '"' << ';'
                   << '"' << safe_header_value(response, "Content-Type") << '"' << ';'
                   << safe_header_value(response, "Content-Length") << ';'
                   << '"' << safe_header_value(response, "Set-Cookie") << '"' << ';';
                if (300 <= response.status && response.status < 400)
                {
                    if (response.has_header("Location"))
                    {
                        ss << safe_header_value(response, "Location");
                    }
                }
                else if (response.status == 200)
                {
                    for (auto const &v : probe_variations_)
                    {
                        enqueue_url(url + v);
                    }
                    if (verify_certs_)
                    {
                        if (auto result = cli.get_openssl_verify_result())
                        {
                            log(std::string("verify error: ") + X509_verify_cert_error_string(result));
                        }
                    }
                }
                if (status_codes_.contains(response.status))
                {
                    log(ss.str());
                }
            }
            else
            {
                std::stringstream ss;
                ss << (-1) << ';' << '"' << url << '"' << ';' << ';' << ';' << ';' << res.error();
                error(ss.str());
                enqueue_url(url);
            }
        }
        return;
    }
}
