/* Copyright (c) 2017, EPFL/Blue Brain Project
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

#include <rockets/server.h>

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
            function debug(message) {
                debugTextArea.value += message + "\n";
                debugTextArea.scrollTop = debugTextArea.scrollHeight;
            }
            function sendMessage() {
                var msg = document.getElementById("inputText").value;
                if (websocket != null)
                {
                    document.getElementById("inputText").value = "";
                    websocket.send(msg);
                    debug('Sent message: "' + msg + '"');
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
                        debug('Received message: "' + event.data + '"');
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

template<typename ...Args>
std::string string_format(const std::string& input, Args... args)
{
    const auto size = std::snprintf(nullptr, 0, input.c_str(), args...);
    std::string output(size, '\0');
    std::snprintf(&output[0], size + 1, input.c_str(), args...);
    return output;
}

int main(int argc, char** argv)
{
    setupCtrlCHandler();

    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--help")
        {
            std::cout << "Usage: rockets-server [interface:port] [protocol]"
                      << std::endl;
            return EXIT_SUCCESS;
        }
    }

    const auto interface = (argc >= 2) ? argv[1] : ":8888";
    const auto wsProtocol = (argc >= 3) ? argv[2] : "rockets";

    try
    {
        Server server{interface, wsProtocol};

        const auto uri = server.getURI();
        const auto page = string_format(htmlPage, uri.c_str(), wsProtocol);

        server.handle(http::Method::GET, "", [&page](const http::Request&) {
            return http::make_ready_response(http::Code::OK, page, "text/html");
        });
        server.handleText([](ws::Request request) {
            return "server echo: " + request.message;
        });

        std::cout << "Listening on: http://" << uri << std::endl;
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
