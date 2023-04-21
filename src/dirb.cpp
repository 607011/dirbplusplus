/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
 */

#include <algorithm>
#include <sstream>

#include "dirb.hpp"
#include "certs.hpp"

namespace dirb
{
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
    const std::vector<int> dirb_runner::DefaultStatusCodeFilter = {200, 204, 301, 302, 307, 308, 401, 403};

    void dirb_runner::log(std::string const &message)
    {
        const std::lock_guard<std::mutex> lock(output_mutex_);
        std::cout << message << std::endl;
    }

    void dirb_runner::error(std::string const &message)
    {
        const std::lock_guard<std::mutex> lock(output_mutex_);
        std::cerr << message << std::endl;
    }

    void dirb_runner::http_worker()
    {
        httplib::Client cli(base_url_.c_str());
        if (verify_certs_)
        {
            X509_STORE *cts = nullptr;
            {
                std::lock_guard<std::mutex> lock(output_mutex_);
                cts = util::read_certificates(cli.ssl_context(), std::cerr);
            }
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
            // TODO: implement all methods, i.e. HEAD, POST, OPTIONS ...
            if (httplib::Result res = cli.Get(url.c_str()))
            {
                std::stringstream ss;
                ss << res->status << ';'
                   << '"' << url << '"' << ';'
                   << '"' << (res->has_header("Content-Type") ? res->get_header_value("Content-Type") : "") << '"' << ';'
                   << (res->has_header("Content-Length") ? res->get_header_value("Content-Length") : 0) << ';'
                   << '"' << (res->has_header("Set-Cookie") ? res->get_header_value("Set-Cookie") : "") << '"' << ';';
                if (300 <= res->status && res->status < 400)
                {
                    if (res->has_header("Location"))
                    {
                        ss << res->get_header_value("Location");
                    }
                }
                else if (res->status == 200)
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    for (auto const &v : probe_variations_)
                    {
                        url_queue_.push(url + v);
                    }
                    if (verify_certs_)
                    {
                        if (auto result = cli.get_openssl_verify_result())
                        {
                            std::cout << "verify error: " << X509_verify_cert_error_string(result) << std::endl;
                        }
                    }
                }
                if (std::find(status_codes_.begin(), status_codes_.end(), res->status) != status_codes_.end())
                {
                    log(ss.str());
                }
            }
            else
            {
                std::stringstream ss;
                ss << (-1) << ';' << '"' << url << '"' << ';' << ';' << ';' << ';' << res.error();
                error(ss.str());
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    url_queue_.push(url);
                }
            }
        }
        return;
    }
}
