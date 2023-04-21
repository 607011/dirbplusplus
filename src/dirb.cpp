/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
 */

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

    void dirb_runner::log(std::string const &message)
    {
        const std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << message << std::endl;
    }

    void dirb_runner::error(std::string const &message)
    {
        const std::lock_guard<std::mutex> lock(output_mutex);
        std::cerr << message << std::endl;
    }

    void dirb_runner::http_worker()
    {
        httplib::Client cli(base_url.c_str());
        X509_STORE *cts = nullptr;
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            cts = util::read_certificates(cli.ssl_context(), std::cerr);
        }
        if (cts == nullptr)
        {
            return;
        }
        cli.set_ca_cert_store(cts);
        cli.enable_server_certificate_verification(verify_certs);
        if (!bearer_token.empty())
        {
            cli.set_bearer_token_auth(bearer_token.c_str());
        }
        if (!username.empty() && !password.empty())
        {
            cli.set_basic_auth(username.c_str(), password.c_str());
        }
        cli.set_follow_location(follow_redirects);
        cli.set_compress(true);
        cli.set_default_headers(headers);
        while (!do_quit)
        {
            std::string url;
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (url_queue.empty())
                {
                    return;
                }
                else
                {
                    url = url_queue.front();
                    url_queue.pop();
                }
            }
            if (url.empty())
            {
                std::cout << "empty";
                continue;
            }
            if (url.front() != '/')
            {
                url = '/' + url;
            }
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
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    for (auto const &v : probe_variations)
                    {
                        url_queue.push(url + v);
                    }
                    if (verify_certs)
                    {
                        if (auto result = cli.get_openssl_verify_result())
                        {
                            std::cout << "verify error: " << X509_verify_cert_error_string(result) << std::endl;
                        }
                    }
                }
                log(ss.str());
            }
            else
            {
                std::stringstream ss;
                ss << (-1) << ';' << '"' << url << '"' << ';' << ';' << ';' << ';' << res.error();
                error(ss.str());
                {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    url_queue.push(url);
                }
            }
        }
        return;
    }
}
