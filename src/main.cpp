/*
 * Dirb++ - Fast, multithreaded version of the original Dirb
 * Copyright (c) 2023 Oliver Lau <oliver.lau@gmail.com>
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

#include "timer.hpp"
#include "util.hpp"
#include "dirb.hpp"

#include "certs.hpp"

#ifdef WIN32
#include <Windows.h>
#define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#endif

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
    constexpr size_t DefaultNumThreads = 40U;

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
               "  -m VERB [--method ...] **NOT IMPLEMENTED YET**\n"
               "    HTTP request method to use; default is GET.\n"
               "    VERB is one of GET, OPTIONS, HEAD, PUT, PATCH, POST, DELETE\n"
               "    (case-insensitive)\n"
               "\n"
               "  --body BODY **NOT IMPLEMENTED YET**\n"
               "    Append BODY to each request; only applies to POST requests.\n"
               "\n"
               "  --content-type TYPE\n"
               "    Send TYPE in Content-Tpe header with each request\n"
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
    size_t num_threads = DefaultNumThreads;
    std::vector<std::string> word_list_filenames{};
    dirb::dirb_runner dirb_runner;
    std::string user_agent;
    std::vector<std::string> probe_extensions{};
    int verbosity = 0;

    while (true)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"follow-redirects", no_argument, 0, 'f'},
            {"verbose", no_argument, 0, 'v'},
            {"threads", required_argument, 0, 't'},
            {"header", required_argument, 0, 'H'},
            {"probe-extensions", required_argument, 0, 'X'},
            {"probe-variations", required_argument, 0, 'V'},
            {"word-list", required_argument, 0, 'w'},
            {"credentials", required_argument, 0, 'p'},
            {"bearer-token", required_argument, 0, 'b'},
            {"cookie", required_argument, 0, 0},
            {"user-agent", required_argument, 0, 0},
            {"body", required_argument, 0, 0},
            {"content-type", required_argument, 0, 0},
            {"method", required_argument, 0, 'm'},
            {"verify-certs", no_argument, 0, 0},
            {"help", no_argument, 0, '?'},
            {"license", no_argument, 0, 0},
            {0, 0, 0, 0}};
        int c = getopt_long(argc, argv, "?b:fp:H:m:t:vV:w:X:", long_options, &option_index);
        if (c == -1)
        {
            break;
        }
        switch (c)
        {
        case 0:
            if (strcmp(long_options[option_index].name, "cookie") == 0)
            {
                dirb_runner.add_header("Cookie", optarg);
            }
            else if (strcmp(long_options[option_index].name, "user-agent") == 0)
            {
                user_agent = optarg;
            }
            else if (strcmp(long_options[option_index].name, "body") == 0)
            {
                dirb_runner.set_body(optarg);
            }
            else if (strcmp(long_options[option_index].name, "verify-certs") == 0)
            {
                dirb_runner.set_verify_certs(true);
            }
            else if (strcmp(long_options[option_index].name, "license") == 0)
            {
                about();
                license();
                return EXIT_SUCCESS;
            }
            break;
        case 'b':
            dirb_runner.set_bearer_token(optarg);
            break;
        case 'm':
            if (strncasecmp(optarg, "GET", 3) == 0)
            {
                dirb_runner.set_method(http::verb::get);
            }
            else if (strncasecmp(optarg, "HEAD", 4) == 0)
            {
                dirb_runner.set_method(http::verb::head);
            }
            else if (strncasecmp(optarg, "POST", 4) == 0)
            {
                dirb_runner.set_method(http::verb::post);
            }
            else if (strncasecmp(optarg, "PATCH", 5) == 0)
            {
                dirb_runner.set_method(http::verb::patch);
            }
            else if (strncasecmp(optarg, "OPTIONS", 7) == 0)
            {
                dirb_runner.set_method(http::verb::options);
            }
            else if (strncasecmp(optarg, "PUT", 3) == 0)
            {
                dirb_runner.set_method(http::verb::put);
            }
            else if (strncasecmp(optarg, "DELETE", 6) == 0)
            {
                dirb_runner.set_method(http::verb::del);
            }
            else
            {
                std::cerr << "\u001b[31;1mERROR:\u001b[0m Invalid method '" << optarg << "'.\n";
                return EXIT_FAILURE;
            }
            break;
        case 'f':
            dirb_runner.set_follow_redirects(true);
            break;
        case 'p':
        {
            auto const &cred = util::unpair(optarg, ':');
            dirb_runner.set_username(cred.first);
            dirb_runner.set_password(cred.second);
            break;
        }
        case 't':
            num_threads = static_cast<unsigned int>(atoi(optarg));
            break;
        case 'X':
        {
            std::cout << optarg << std::endl;
            probe_extensions = util::split(optarg, ',');
            break;
        }
        case 'V':
            dirb_runner.set_probe_variations(util::split(optarg, ','));
            break;
        case 'H':
            dirb_runner.add_header(util::unpair(optarg, ':'));
            break;
        case 'v':
            ++verbosity;
            break;
        case 'w':
            word_list_filenames.push_back(optarg);
            break;
        case '?':
            about();
            usage();
            return EXIT_SUCCESS;
        default:
            break;
        }
    }
    if (optind < argc)
    {
        dirb_runner.set_base_url(argv[optind++]);
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
    for (size_t i = 0; i < num_threads; ++i)
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
