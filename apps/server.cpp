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

#include <rockets/jsonrpc/http.h>
#include <rockets/jsonrpc/server.h>
#include <rockets/server.h>

#include "rockets/json.hpp"

#include <cstdio> // std::snprintf
#include <iostream>
#include <signal.h> // ctrl-c handler

using namespace rockets;

const std::string htmlPage{
    R"HTMLCONTENT(<html>
    <head>
        <meta charset="UTF-8">
        <title>Rockets Websocket Echo Client</title>
    </head>
    <body>
        <h1>Rockets Websocket Echo Client</h1>
        <p>
            <button onClick="initWebSocket();">Connect</button>
            <button onClick="stopWebSocket();">Disconnect</button>
            <button onClick="checkSocket();">State</button>
        </p>
        <p>
            <textarea id="debugTextArea"
                      style="width:400px;height:200px;"></textarea>
        </p>
        <p>
            <input type="text" id="inputText"
                   onkeydown="if(event.keyCode==13)sendMessage();"/>
            <button onClick="sendMessage();">Send</button>
        </p>
        <script type="text/javascript">
            var wsUri = "ws://%s";
            var wsProtocol = "%s";
            var websocket = null;
            var debugTextArea = document.getElementById("debugTextArea");
            var jsonrpc = %s;
            var requestId = 0;
            function debug(message) {
                debugTextArea.value += message + "\n";
                debugTextArea.scrollTop = debugTextArea.scrollHeight;
            }
            function jsonrpcEchoRequest(params) {
                var obj = {jsonrpc: '2.0', method: 'echo', id: requestId++};
                obj['params'] = {message: params}
                return JSON.stringify(obj);
            }
            function sendMessage() {
                var msg = document.getElementById("inputText").value;
                document.getElementById("inputText").value = "";
                if (websocket != null)
                {
                    if (jsonrpc)
                        websocket.send(jsonrpcEchoRequest(msg));
                    else
                        websocket.send(msg);
                    debug('=> "' + msg + '"');
                }
            }
            function initWebSocket() {
                try {
                    if (typeof MozWebSocket == 'function')
                        WebSocket = MozWebSocket;
                    if (websocket && websocket.readyState == 1)
                        websocket.close();
                    websocket = new WebSocket(wsUri, wsProtocol);
                    websocket.onopen = function (evt) {
                        debug("CONNECTED");
                    };
                    websocket.onclose = function (evt) {
                        debug("DISCONNECTED");
                    };
                    websocket.onmessage = function (event) {
                        if (jsonrpc) {
                            var obj = JSON.parse(event.data);
                            if (obj.hasOwnProperty('error')) {
                                var err = obj['error'];
                                var msg = err['code'] + ' - ' + err['message'];
                            }
                            else if (obj.hasOwnProperty('result')) {
                                var msg = obj['result'];
                            }
                        } else {
                            var msg = event.data;
                        }
                        debug('<= "' + msg + '"');
                    };
                    websocket.onerror = function (evt) {
                        debug('ERROR: ' + evt.data);
                    };
                } catch (exception) {
                    debug('ERROR: ' + exception);
                }
            }
            function stopWebSocket() {
                if (websocket)
                    websocket.close();
            }
            function getAsString(readyState) {
                switch (readyState) {
                    case 0: return "CONNECTING";
                    case 1: return "OPEN";
                    case 2: return "CLOSING";
                    case 3: return "CLOSED";
                    default: return "UNKNOW";
                }
            }
            function checkSocket() {
                if (websocket != null) {
                    var stateStr = getAsString(websocket.readyState);
                    debug("WebSocket state: " + stateStr);
                } else {
                    debug("WebSocket is null");
                }
            }
        </script>
    </body>
</html>
)HTMLCONTENT"};

bool running = true;

void ctrlCHandler(int)
{
    std::cout << "Shutting down..." << std::endl;
    running = false;
}

void setupCtrlCHandler()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = ctrlCHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, 0);
}

template <typename... Args>
std::string format(const std::string& input, Args... args)
{
    const auto size = std::snprintf(nullptr, 0, input.c_str(), args...);
    std::string output(size, '\0');
    std::snprintf(&output[0], size + 1, input.c_str(), args...);
    return output;
}

template <typename Container>
bool contains(const Container& c, const std::string& value)
{
    return std::find(c.begin(), c.end(), value) != c.end();
}

template <typename Container>
void remove(Container& c, const std::string& value)
{
    c.erase(std::remove(c.begin(), c.end(), value), c.end());
}

void print_usage()
{
    std::cout << "Usage: rockets-server [interface:port] [ws-protocol]"
              << std::endl
              << "Options: --jsonrpc - use JSON-RPC 2.0 protocol" << std::endl;
}

int main(int argc, char** argv)
{
    setupCtrlCHandler();

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);

    if (contains(args, "--help"))
    {
        print_usage();
        return EXIT_SUCCESS;
    }

    const auto useJsonRpc = contains(args, "--jsonrpc");
    remove(args, "--jsonrpc");
    const auto interface = (args.size() >= 1) ? args[0] : ":8888";
    const auto wsProtocol = (args.size() >= 2) ? args[1] : "rockets";

    try
    {
        Server server{interface, wsProtocol};

        const auto uri = server.getURI();
        const auto page = format(htmlPage, uri.c_str(), wsProtocol.c_str(),
                                 useJsonRpc ? "true" : "false");

        server.handle(http::Method::GET, "", [&page](const http::Request&) {
            return http::make_ready_response(http::Code::OK, page, "text/html");
        });

        std::unique_ptr<jsonrpc::Server<rockets::Server>> rpc;
        if (useJsonRpc)
        {
            rpc = std::make_unique<jsonrpc::Server<rockets::Server>>(server);
            rpc->bind("echo", [](const jsonrpc::Request& req) {
                using namespace rockets_nlohmann;
                const auto input = json::parse(req.message, nullptr, false);
                if (input.count("message"))
                    return jsonrpc::Response{input["message"].dump()};
                return jsonrpc::Response::invalidParams();
            });
            // also accept HTTP POST "/" JSON-RPC commands
            jsonrpc::connect(server, "", *rpc);
        }
        else
        {
            server.handleText([](ws::Request request) {
                return "server echo: " + request.message;
            });
        }

        std::cout << "Listening on: http://" << uri
                  << " with websockets subprotocol '" << wsProtocol << "'";
        if (useJsonRpc)
            std::cout << " using JSON-RPC 2.0";
        std::cout << std::endl;

        while (running)
            server.process(100);
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Invalid argument: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
