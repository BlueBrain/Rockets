/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
 *                          Raphael.Dumusc@epfl.ch
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

#include <iostream>
#include <string>

using namespace rockets;

void print_usage()
{
    std::cout << "Usage: rockets-http-request url [body]" << std::endl;
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

    if (argc < 2)
    {
        print_usage();
        return EXIT_FAILURE;
    }

    const auto uri = std::string(argv[1]);
    const auto body = argc == 3 ? std::string(argv[2]) : std::string();
    const auto method = body.empty() ? http::Method::GET : http::Method::POST;

    http::Client client;
    try
    {
        auto res = client.request(uri, method, body);
        while (!is_ready(res))
            client.process(250);

        const auto response = res.get();
        if (response.code != http::Code::OK)
            std::cout << "Code " << (int)response.code << ":" << std::endl;
        if (!response.body.empty())
            std::cout << response.body << std::endl;
        return EXIT_SUCCESS;
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
