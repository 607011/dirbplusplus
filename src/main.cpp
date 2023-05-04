/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <mutex>
#include <numeric>
#include <thread>
#include <string>
#include <vector>

#include <getopt.hpp>

#include "timer.hpp"
#include "util.hpp"
#include "dirb.hpp"

#include "certs.hpp"

namespace chrono = std::chrono;
namespace http = dirb::http;

#ifndef PROJECT_NAME
#define PROJECT_NAME "dirb++"
#endif
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown"
#endif

namespace
{
    constexpr std::size_t DefaultNumThreads = 40U;

    void about()
    {
        std::cout
            << PROJECT_NAME << "++ " << PROJECT_VERSION << " - Fast, multithreaded version of the original Dirb.\n\n"
            << "Copyright (c) 2023 Oliver Lau\n\n";
    }

    void license()
    {
        std::cout
            << "Permission is hereby granted, free of charge, to any person obtaining\n"
               "a copy of this software and associated documentation files (the \"Soft-\n"
               "ware\"), to deal in the Software without restriction, including without\n"
               "limitation the rights to use, copy, modify, merge, publish, distribute,\n"
               "sublicense, and/or sell copies of the Software, and to permit persons\n"
               "to whom the Software is furnished to do so, subject to the following\n"
               "conditions:\n\n"
               "The above copyright notice and this permission notice shall be included\n"
               "in all copies or substantial portions of the Software.\n\n"
               "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
               "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
               "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
               "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY\n"
               "CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,\n"
               "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFT-\n"
               "WARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n";
    }

    void brief_usage()
    {
        std::cout
            << "USAGE: " << PROJECT_NAME << " [options] base_url\n"
            << "\n"
            << "See `" << PROJECT_NAME << " --help` for options\n";
    }

    void usage()
    {
        std::cout
            << "\n"
               "USAGE: "
            << PROJECT_NAME
            << " [options] base_url\n"
               "\n"
               "  base_url\n"
               "\n"
               "       The base URL used for all URL queries,\n"
               "       e.g. `http://example.com` or `https://example.com`\n"
               "       (no trailing slash!)\n"
               "\n"
               "OPTIONS:\n"
               "\n"
               "  -w FILENAME [--word-list ...]\n"
               "    Add word list file\n"
               "\n"
               "  -v [--verbose]\n"
               "    Increase verbosity of output (only applies to standard output mode)\n"
               "\n"
               "  -t N [--threads N]\n"
               "    Run in N threads (default: "
            << DefaultNumThreads << "\n"
            << "\n"
               "  -p USERNAME:PASSWORD [--credentials ...]\n"
               "    Enable basic authentication with USERNAME and PASSWORD\n"
               "\n"
               "  -b TOKEN [--bearer-token ...]\n"
               "    Use a bearer token to authenticate, e.g. a JWT\n"
               "\n"
               "  --cookie COOKIE\n"
               "    Send Cookie header with each request\n"
               "\n"
               "  -i CODELIST [--include ...] \n"
               "    Only include HTTP status codes in CODELIST.\n"
               "    CODELIST is a comma-separated list of status codes.\n"
               "    Default: ";
        std::vector<int> keys;
        keys.reserve(dirb::dirb_runner::DefaultStatusCodeFilter.size());
        std::transform(
            dirb::dirb_runner::DefaultStatusCodeFilter.begin(),
            dirb::dirb_runner::DefaultStatusCodeFilter.end(),
            std::back_inserter(keys),
            [](auto pair)
            { return pair.first; });
        std::cout
            << util::join(keys, ',')
            << "\n\n"
               "  -m VERB [--method ...] **NOT IMPLEMENTED YET**\n"
               "    HTTP request method to use; default is GET.\n"
               "    VERB is one of GET, OPTIONS, HEAD, PUT, PATCH, POST, DELETE\n"
               "    (case-insensitive)\n"
               "\n"
               "  --body BODY **NOT IMPLEMENTED YET**\n"
               "    Append BODY to each request; only applies to POST requests.\n"
               "\n"
               "  --content-type TYPE\n"
               "    Send TYPE in Content-Type header with each request\n"
               "\n"
               "  --user-agent USERAGENT\n"
               "    Send USERAGENT in User-Agent header,\n"
               "    default: \""
            << dirb::dirb_runner::DefaultUserAgent << "\"\n"
            << "\n"
               "  -X EXT1,EXT2,...EXTn [--probe-extensions ...]\n"
               "    Do not only check the path itself, but also try every\n"
               "    path by adding these extensions, delimited by comma. E.g.:\n"
               "      -X .jsp,.php,.phpx,.xhtml\n"
               "\n"
               "  -V EXT1,EXT2,...EXTn [--probe-variations ...]\n"
               "    If a path is found, check these variations by appending them\n"
               "    to the path, delimited by comma. E.g.:\n"
               "      -V _,_admin\n"
               "\n"
               "  -f [--follow-redirects]\n"
               "    Follow 301 and 302 redirects to their final destination\n"
               "\n"
               "  --verify-certs\n"
               "    Enable verification of CA certificates\n"
               "    (only applies to HTTPS requests)\n"
               "\n"
               "  --license\n"
               "    Display license\n"
               "\n";
    }
}

int main(int argc, char *argv[])
{
    std::size_t num_threads{DefaultNumThreads};
    std::vector<std::string> word_list_filenames{};
    dirb::dirb_runner dirb_runner{};
    std::string user_agent{dirb::dirb_runner::DefaultUserAgent};
    std::vector<std::string> probe_extensions{};
    int verbosity{0};
    using argparser = argparser::argparser;
    argparser opt{argc, argv};
    opt
        .reg({"-f", "--follow-redirects"}, argparser::no_argument,
             [&dirb_runner](std::string const &)
             { dirb_runner.set_follow_redirects(true); })
        .reg({"-v", "--verbose"}, argparser::no_argument,
             [&verbosity](std::string const &)
             { ++verbosity; })
        .reg({"-t", "--threads"}, argparser::required_argument,
             [&num_threads](std::string const &val)
             { num_threads = static_cast<unsigned int>(std::stoi(val)); })
        .reg({"-H", "--header"}, argparser::required_argument,
             [&dirb_runner](std::string const &val)
             { dirb_runner.add_header(util::unpair(val, ':')); })
        .reg({"-X", "--probe-extensions"}, argparser::required_argument,
             [&probe_extensions](std::string const &val)
             { probe_extensions = util::split(val, ','); })
        .reg({"-V", "--probe-variations"}, argparser::required_argument,
             [&dirb_runner](std::string const &val)
             { dirb_runner.set_probe_variations(util::split(val, ',')); })
        .reg({"-w", "--word-list"}, argparser::required_argument,
             [&word_list_filenames](std::string const &val)
             { word_list_filenames.push_back(val); })
        .reg({"-i", "--include"}, argparser::required_argument,
             [&dirb_runner](std::string const &val)
             {
                 std::unordered_map<int, bool> codes;
                 for (auto code : util::split(val, ','))
                 {
                     codes.emplace(std::stoi(code), true);
                 }
                 dirb_runner.set_status_code_filter(codes);
             })
        .reg({"-p", "--credentials"}, argparser::required_argument,
             [&dirb_runner](std::string const &val)
             {
                 auto const &cred = util::unpair(val, ':');
                 dirb_runner.set_username(cred.first);
                 dirb_runner.set_password(cred.second);
             })
        .reg({"-b", "--bearer-token"}, argparser::required_argument,
             [&dirb_runner](std::string const &val)
             { dirb_runner.set_bearer_token(val); })
        .reg({"--cookie"}, argparser::required_argument,
             [&dirb_runner](std::string const &val)
             { dirb_runner.add_header("Cookie", val); })
        .reg({"--user-agent"}, argparser::required_argument,
             [&user_agent](std::string const &val)
             { user_agent = val; })
        .reg({"--body"}, argparser::required_argument,
             [&dirb_runner](std::string const &val)
             { dirb_runner.set_body(val); })
        .reg({"--verify-certs"}, argparser::no_argument,
             [&dirb_runner](std::string const &)
             { dirb_runner.set_verify_certs(true); })
        .reg({"--content-type"}, argparser::required_argument,
             [&dirb_runner](std::string const &val)
             { dirb_runner.add_header(std::make_pair("Content-Type", val)); })
        .reg({"-m", "--method"}, argparser::required_argument,
             [&dirb_runner](std::string val)
             {
                 std::transform(val.begin(), val.end(), val.begin(), [](int c)
                                { return std::toupper(c); });
                 if (val == "GET")
                 {
                     dirb_runner.set_method(http::verb::get);
                 }
                 else if (val == "HEAD")
                 {
                     dirb_runner.set_method(http::verb::head);
                 }
                 else if (val == "POST")
                 {
                     dirb_runner.set_method(http::verb::post);
                 }
                 else if (val == "PATCH")
                 {
                     dirb_runner.set_method(http::verb::patch);
                 }
                 else if (val == "OPTIONS")
                 {
                     dirb_runner.set_method(http::verb::options);
                 }
                 else if (val == "PUT")
                 {
                     dirb_runner.set_method(http::verb::put);
                 }
                 else if (val == "DELETE")
                 {
                     dirb_runner.set_method(http::verb::del);
                 }
                 else
                 {
                     std::cerr << "\u001b[31;1mERROR:\u001b[0m Invalid method '" << val << "'.\n";
                     exit(EXIT_FAILURE);
                 }
             })
        .reg({"-?", "--help"}, argparser::no_argument,
             [](std::string const &)
             {
                 about();
                 usage();
                 exit(EXIT_SUCCESS);
             })
        .reg({"--license"}, argparser::no_argument,
             [](std::string const &)
             {
                 about();
                 license();
                 exit(EXIT_SUCCESS);
             })
        .pos([&dirb_runner](std::string const &val)
             { dirb_runner.set_base_url(val); });
    try
    {
        opt();
    }
    catch (::argparser::argument_required_exception const &e)
    {
        std::cerr << e.what() << '\n';
    }

    if (!dirb_runner.has_base_url())
    {
        about();
        brief_usage();
        return EXIT_FAILURE;
    }
    dirb_runner.add_header("User-Agent", user_agent);

    if (verbosity > 1)
    {
        std::cout << "Reading word list" << (word_list_filenames.size() == 1 ? "" : "s") << " ... " << std::flush;
    }
    for (std::string const &word_list_filename : word_list_filenames)
    {
        std::ifstream is(word_list_filename);
        std::string line;
        while (std::getline(is, line))
        {
            dirb_runner.add_to_queue(line);
            for (auto const &ext : probe_extensions)
            {
                dirb_runner.add_to_queue(line + ext);
            }
        }
    }
    num_threads = std::min(num_threads, dirb_runner.url_queue_size());
    if (verbosity > 0)
    {
        std::cout << "Read " << dirb_runner.url_queue_size() << " URLs." << std::endl;
        std::cout << "Starting " << num_threads << " worker threads ..." << std::endl;
    }
    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    timer t;
    for (std::size_t i = 0; i < num_threads; ++i)
    {
        workers.emplace_back(&dirb::dirb_runner::http_worker, &dirb_runner);
    }
    for (auto &worker : workers)
    {
        worker.join();
    }
    if (verbosity > 0)
    {
        std::cout << "Elapsed time: "
                  << chrono::duration_cast<chrono::milliseconds>(t.elapsed()).count() << " ms"
                  << std::endl;
    }
    return EXIT_SUCCESS;
}
