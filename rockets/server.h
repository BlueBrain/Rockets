/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
 *                          Raphael.Dumusc@epfl.ch
 *                          Stefan.Eilemann@epfl.ch
 *                          Daniel.Nachbaur@epfl.ch
 *
 * This file is part of Rockets <https://github.com/BlueBrain/Rockets>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef ROCKETS_SERVER_H
#define ROCKETS_SERVER_H

#include <rockets/http/filter.h>
#include <rockets/http/helpers.h>
#include <rockets/http/request.h>
#include <rockets/socketBasedInterface.h>
#include <rockets/ws/types.h>

#include <set>

namespace rockets
{
/**
 * Serves HTTP requests and Websockets connections.
 *
 * Not thread safe.
 *
 * Example usage: @include apps/server.cpp
 */
class Server : public SocketBasedInterface
{
public:
    /** @name Setup */
    //@{
    /**
     * Construct a new server.
     *
     * If no interface/hostname/IP is given, the server listens on all
     * interfaces. If no port is given, the server selects a random port. Use
     * getURI() to retrieve the chosen parameters.
     *
     * There are three ways of processing requests on the interface:
     * - Calling process() regularly in the application's main loop.
     * - Integrating the socket descriptor(s) in an external poll array, using
     *   setSocketListener() and calling processSocket() when notified.
     * - Using internal service thread(s) by setting threadCount > 0. Note that
     *   in this case the registered callbacks will be executed asynchronously
     *   from the internal service threads.
     *
     * @param uri The server address in the form "[hostname|IP|iface][:port]".
     * @param name The name of the websockets protocol, disabled if empty.
     * @param threadCount The number of internal service threads to use.
     * @throw std::runtime_error on malformed URI or connection issues.
     */
    ROCKETS_API Server(const std::string& uri, const std::string& name,
                       unsigned int threadCount = 0);
    ROCKETS_API explicit Server(unsigned int threadCount = 0);

    /**
     * Construct a new server and integrate it to a libuv loop.
     *
     * If no interface/hostname/IP is given, the server listens on all
     * interfaces. If no port is given, the server selects a random port. Use
     * getURI() to retrieve the chosen parameters.
     *
     * All send & receive operations are automatically handled by the given
     * libuv loop, given that libwebsockets has been built with libuv support.
     *
     * @param uvLoop The libuv loop to run the send & receive operations on.
     * @param uri The server address in the form "[hostname|IP|iface][:port]".
     * @param name The name of the websockets protocol, disabled if empty.
     * @throw std::runtime_error on malformed URI, connection issues or no libuv
     * suppport.
     */
    ROCKETS_API Server(void* uvLoop, const std::string& uri,
                       const std::string& name);

    /** Terminate the server. */
    ROCKETS_API ~Server();

    /** @return the server URI in the form "[hostname|IP][:port]". */
    ROCKETS_API std::string getURI() const;

    /** @return the server port. */
    ROCKETS_API uint16_t getPort() const;

    /** @return the number of internal service threads. */
    ROCKETS_API unsigned int getThreadCount() const;

    /**
     * Set a filter for HTTP requests.
     *
     * @param filter to set, nullptr to remove.
     */
    ROCKETS_API void setHttpFilter(const http::Filter* filter);
    //@}

    /** @name HTTP functionality */
    //@{
    /**
     * Handle a single method on a given endpoint.
     *
     * @param method to handle
     * @param endpoint the endpoint to receive requests for during receive().
     * @param func the callback function for serving the request.
     * @return true if subscription was successful.
     * @throw std::invalid_argument if attempting to register "registry"
     *        endpoint.
     */
    ROCKETS_API bool handle(http::Method method, const std::string& endpoint,
                            http::RESTFunc func);

    /**
     * Handle a JSON-serializable object.
     *
     * @param object to expose.
     * @param endpoint for accessing the object.
     * @return true if subscription was successful.
     */
    template <typename Obj>
    bool handle(const std::string& endpoint, Obj& object)
    {
        return handleGET(endpoint, object) && handlePUT(endpoint, object);
    }

    /**
     * Expose a JSON-serializable object.
     *
     * @param object to expose.
     * @param endpoint for accessing the object.
     * @return true if subscription was successful.
     */
    template <typename Obj>
    bool handleGET(const std::string& endpoint, Obj& object)
    {
        using namespace rockets::http;
        return handle(Method::GET, endpoint, [&object](const Request&) {
            return make_ready_response(http::Code::OK, to_json(object),
                                       "application/json");
        });
    }

    /**
     * Subscribe a JSON-deserializable object.
     *
     * @param object to subscribe.
     * @param endpoint for modifying the object.
     * @return true if subscription was successful.
     */
    template <typename Obj>
    bool handlePUT(const std::string& endpoint, Obj& object)
    {
        using namespace rockets::http;
        return handle(Method::PUT, endpoint, [&object](const Request& req) {
            const auto success = from_json(object, req.body);
            return make_ready_response(success ? Code::OK : Code::BAD_REQUEST);
        });
    }

    /**
     * Remove all handling for a given endpoint.
     *
     * @return false if endpoint did not exist.
     */
    ROCKETS_API bool remove(const std::string& endpoint);
    //@}

    /** @name Websockets functionality */
    //@{
    /** Set a callback for handling incoming connections. */
    ROCKETS_API void handleOpen(ws::ConnectionCallback callback);

    /** Set a callback for handling closing connections. */
    ROCKETS_API void handleClose(ws::ConnectionCallback callback);

    /** Set a callback for handling text messages from websocket clients. */
    ROCKETS_API void handleText(ws::MessageCallback callback);

    /** Set a callback for handling text messages from websocket clients. */
    ROCKETS_API void handleText(ws::MessageCallbackAsync callback);

    /** Set a callback for handling binray messages from websocket clients. */
    ROCKETS_API void handleBinary(ws::MessageCallback callback);

    /** Broadcast a text message to all websocket clients. */
    ROCKETS_API void broadcastText(const std::string& message);

    /**
     * Broadcast a text message to all websocket clients, except the filtered
     * ones.
     */
    ROCKETS_API void broadcastText(const std::string& message,
                                   const std::set<uintptr_t>& filter);

    /** Send a text message to the given client. */
    ROCKETS_API void sendText(const std::string& message, uintptr_t client);

    /** Broadcast a binary message to all websocket clients. */
    ROCKETS_API void broadcastBinary(const char* data, size_t size);

    /** @return the number of connected websockets clients. */
    ROCKETS_API size_t getConnectionCount() const;
    //@}

    class Impl; // must be public for static_cast from C callback
private:
    std::unique_ptr<Impl> _impl;

    void _setSocketListener(SocketListener* listener) final;
    void _processSocket(SocketDescriptor fd, int events) final;
    void _process(int timeout_ms) final;
};
} // namespace rockets

#endif
