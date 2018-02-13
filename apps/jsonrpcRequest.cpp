/* Copyright (c) 2018, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
 *
 * This file is part of Rockets <https://github.com/BlueBrain/Rockets>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Blue Brain Project / EPFL nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <rockets/helpers.h>
#include <rockets/http/client.h>
#include <rockets/jsonrpc/client.h>
#include <rockets/jsonrpc/http.h>
#include <rockets/ws/client.h>

#include <iostream>
#include <string>

using namespace rockets;

void print_usage()
{
    std::cout << "Usage: rockets-jsonrpc-request [ws://]<url> <method> "
                 "<json-params> [ws-protocol]"
              << std::endl;
}

inline bool starts_with(const std::string& a, const std::string& b)
{
    return a.compare(0, b.length(), b) == 0;
}

jsonrpc::Response request_http(const std::string& url,
                               const std::string& method,
                               const std::string& params)
{
    http::Client httpClient;
    jsonrpc::HttpCommunicator communicator{httpClient, url};
    jsonrpc::Client<jsonrpc::HttpCommunicator> client{communicator};

    auto res = client.request(method, params);
    while (!is_ready(res))
        httpClient.process(250);
    return res.get();
}

jsonrpc::Response request_ws(const std::string& url, const std::string& method,
                             const std::string& params,
                             const std::string& wsProtocol)
{
    ws::Client wsClient;
    auto connection = wsClient.connect(url, wsProtocol);
    while (!is_ready(connection))
        wsClient.process(250);
    connection.get();

    jsonrpc::Client<ws::Client> client{wsClient};
    auto res = client.request(method, params);
    while (!is_ready(res))
        wsClient.process(250);
    return res.get();
}

jsonrpc::Response request(const std::string& url, const std::string& method,
                          const std::string& params,
                          const std::string& wsProtocol)
{
    if (starts_with(url, "ws://") || starts_with(url, "wss://"))
        return request_ws(url, method, params, wsProtocol);

    return request_http(url, method, params);
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--help")
        {
            print_usage();
            return EXIT_SUCCESS;
        }
    }
    if (argc < 4)
    {
        print_usage();
        return EXIT_FAILURE;
    }

    const auto url = std::string(argv[1]);
    const auto method = std::string(argv[2]);
    const auto params = std::string(argv[3]);
    const auto wsProtocol = (argc >= 5) ? std::string(argv[4]) : "rockets";

    try
    {
        const auto response = request(url, method, params, wsProtocol);
        if (response.isError())
        {
            std::cerr << "Error " << response.error.code << " - "
                      << response.error.message << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Response: " << response.result << std::endl;
        return EXIT_SUCCESS;
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
