/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
*/

#include "dirb.hpp"

namespace dirb
{
    void http_worker(std::queue<std::string> &url_queue, std::mutex &queue_mtx, std::atomic_bool &do_quit, dirb_options dirb_opts)
    {
        httplib::Client cli(dirb_opts.base_url.c_str());
        // cli.set_ca_cert_path(CA_CERT_FILE);
        cli.enable_server_certificate_verification(dirb_opts.verify_certs);
        if (!dirb_opts.bearer_token.empty())
        {
            cli.set_bearer_token_auth(dirb_opts.bearer_token.c_str());
        }
        if (!dirb_opts.username.empty() && !dirb_opts.password.empty())
        {
            cli.set_basic_auth(dirb_opts.username.c_str(), dirb_opts.password.c_str());
        }
        cli.set_follow_location(dirb_opts.follow_redirects);
        cli.set_compress(true);
        cli.set_default_headers(dirb_opts.headers);
        while (!do_quit)
        {
            std::string url;
            {
                std::lock_guard<std::mutex> lock(queue_mtx);
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
                    std::lock_guard<std::mutex> lock(queue_mtx);
                    for (auto const &v : dirb_opts.probe_variations)
                    {
                        url_queue.push(url + v);
                    }
                }
                dirb_opts.log(ss.str());
            }
            else
            {
                std::stringstream ss;
                ss << (-1) << ';' << '"' << url << '"' << ';' << ';' << ';' << ';' << res.error();
                dirb_opts.error(ss.str());
                std::lock_guard<std::mutex> lock(queue_mtx);
                url_queue.push(url);
            }
        }
    }
}
