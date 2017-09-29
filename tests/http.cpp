/* Copyright (c) 2017, EPFL/Blue Brain Project
 *                     Raphael.Dumusc@epfl.ch
 *                     Stefan.Eilemann@epfl.ch
 *                     Daniel.Nachbaur@epfl.ch
 *                     Pawel.Podhajski@epfl.ch
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

#define BOOST_TEST_MODULE rockets_http

#include <rockets/http/client.h>
#include <rockets/http/helpers.h>
#include <rockets/http/request.h>
#include <rockets/http/response.h>
#include <rockets/server.h>

#include <libwebsockets.h>

#include <iostream>
#include <map>

#include <boost/mpl/vector.hpp>
#include <boost/test/unit_test.hpp>

#define CLIENT_SUPPORTS_REP_PAYLOAD (LWS_LIBRARY_VERSION_NUMBER >= 2000000)
#define CLIENT_SUPPORTS_REQ_PAYLOAD (LWS_LIBRARY_VERSION_NUMBER >= 2001000)
#define CLIENT_SUPPORTS_REP_ERRORS (LWS_LIBRARY_VERSION_NUMBER >= 2002000)

using namespace rockets;

namespace
{
const auto echoFunc = [](const http::Request& request) {
    auto body = request.path;

    for (auto kv : request.query)
        body.append("?" + kv.first + "=" + kv.second);

    if (!request.body.empty())
    {
        if (!body.empty())
            body.append(":");
        body.append(request.body);
    }
    return http::make_ready_response(http::Code::OK, body);
};

const std::string JSON_TYPE = "application/json";

const http::Response response200{http::Code::OK};
const http::Response response204{http::Code::NO_CONTENT};
const http::Response error400{http::Code::BAD_REQUEST};
const http::Response error404{http::Code::NOT_FOUND};
const http::Response error405{http::Code::NOT_SUPPORTED};

const std::string jsonGet("{\"json\": \"yes\", \"value\": 42}");
const std::string jsonPut("{\"foo\": \"no\", \"bar\": true}");

const http::Response responseJsonGet(http::Code::OK, jsonGet, JSON_TYPE);

class Foo
{
public:
    std::string toJson() const
    {
        _called = true;
        return jsonGet;
    }
    bool fromJson(const std::string& json)
    {
        if (jsonPut == json)
        {
            _notified = true;
            return true;
        }
        return false;
    }

    bool getNotified() const { return _notified; }
    void setNotified(const bool notified = true) { _notified = notified; }
    bool getCalled() const { return _called; }
    void setCalled(const bool called = true) { _called = called; }
    std::string getEndpoint() const { return "test/foo"; }
private:
    bool _notified = false;
    mutable bool _called = false;
};

template <typename T>
std::string to_json(const T& obj)
{
    return obj.toJson();
}
template <typename T>
bool from_json(T& obj, const std::string& json)
{
    return obj.fromJson(json);
}

template <typename T>
bool is_ready(const std::future<T>& f)
{
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

class MockClient : public http::Client
{
public:
    using Client::Client;

    http::Response checkGET(Server& server, const std::string& uri)
    {
        return check(server, uri, http::Method::GET, "");
    }

    http::Response checkPUT(Server& server, const std::string& uri,
                            const std::string& body)
    {
        return check(server, uri, http::Method::PUT, body);
    }

    http::Response checkPOST(Server& server, const std::string& uri,
                             const std::string& body)
    {
        return check(server, uri, http::Method::POST, body);
    }

    http::Response check(Server& server, const std::string& uri,
                         const http::Method method, const std::string& body)
    {
        auto response = request(server.getURI() + uri, method, body);
        while (!is_ready(response))
        {
            process(0);
            if (server.getThreadCount() == 0)
                server.process(0);
        }
        return response.get();
    }

private:
};
} // anonymous namespace

namespace rockets
{
namespace http
{
template <typename Map>
bool operator==(const Map& a, const Map& b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
bool operator==(const Response& a, const Response& b)
{
    return a.code == b.code && a.body == b.body && a.headers == b.headers;
}
std::ostream& operator<<(std::ostream& oss, const http::Header& header)
{
    switch (header)
    {
    case Header::ALLOW:
        oss << "Allow";
        break;
    case Header::CONTENT_TYPE:
        oss << "Content-Type";
        break;
    case Header::LAST_MODIFIED:
        oss << "Last-Modified";
        break;
    case Header::LOCATION:
        oss << "Location";
        break;
    case Header::RETRY_AFTER:
        oss << "Retry-After";
        break;
    default:
        oss << "UNDEFINED";
        break;
    }
    return oss;
}

std::ostream& operator<<(std::ostream& oss, const Response& response)
{
    oss << "Code: " << (int)response.code << ", body: '" << response.body;
    oss << "', headers: [";
    for (auto kv : response.headers)
        oss << "(" << kv.first << ": " << kv.second << ")";
    oss << "]" << std::endl;
    return oss;
}
}
}

/**
 * Fixtures to run all test cases with {0, 1, 2} server worker threads.
 */
struct FixtureBase
{
    Foo foo;
    MockClient client;
    http::Response response;
};
struct Fixture0 : public FixtureBase
{
    Server server;
};
struct Fixture1 : public FixtureBase
{
    Server server{1u};
};
struct Fixture2 : public FixtureBase
{
    Server server{2u};
};
using Fixtures = boost::mpl::vector<Fixture0, Fixture1, Fixture2>;

BOOST_AUTO_TEST_CASE(construction)
{
    Server server1;
    BOOST_CHECK_NE(server1.getURI(), "");
    BOOST_CHECK_NE(server1.getPort(), 0);
    BOOST_CHECK_EQUAL(server1.getThreadCount(), 0);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(registration, F, Fixtures, F)
{
    BOOST_CHECK(F::server.handleGET(F::foo.getEndpoint(), F::foo));
    BOOST_CHECK(!F::server.handleGET(F::foo.getEndpoint(), F::foo));
    BOOST_CHECK(F::server.remove(F::foo.getEndpoint()));
    BOOST_CHECK(!F::server.remove(F::foo.getEndpoint()));

    BOOST_CHECK(F::server.handle(F::foo.getEndpoint(), F::foo));
    BOOST_CHECK(!F::server.handle(F::foo.getEndpoint(), F::foo));
    BOOST_CHECK(F::server.remove(F::foo.getEndpoint()));
    BOOST_CHECK(!F::server.remove(F::foo.getEndpoint()));

    using Method = http::Method;
    for (int method = 0; method < int(Method::ALL); ++method)
        BOOST_CHECK(F::server.handle(Method(method), "path", echoFunc));
    for (int method = 0; method < int(Method::ALL); ++method)
        BOOST_CHECK(!F::server.handle(Method(method), "path", echoFunc));
    BOOST_CHECK(F::server.remove("path"));
    for (int method = 0; method < int(Method::ALL); ++method)
        BOOST_CHECK(F::server.handle(Method(method), "path", echoFunc));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(issue157, F, Fixtures, F)
{
    F::server.handle(F::foo.getEndpoint(), F::foo);

    bool running = true;
    std::thread thread([&]() {
        while (running)
            F::server.process(100);
    });

    // Close client before receiving request to provoke #157
    {
        MockClient tmp;
        tmp.request(F::server.getURI() + "/test/foo?" + std::string(1024, 'o'));
    }

    running = false;
    thread.join();
}

#if CLIENT_SUPPORTS_REQ_PAYLOAD

BOOST_FIXTURE_TEST_CASE_TEMPLATE(get_object_json, F, Fixtures, F)
{
    F::server.handleGET(F::foo.getEndpoint(), F::foo);

#if CLIENT_SUPPORTS_REP_ERRORS
    F::response = F::client.checkGET(F::server, "/unknown");
    BOOST_CHECK(!F::foo.getCalled());
    BOOST_CHECK_EQUAL(F::response, error404);
#endif
    F::response = F::client.checkGET(F::server, "/test/foo");
    BOOST_CHECK(F::foo.getCalled());
    BOOST_CHECK_EQUAL(F::response, responseJsonGet);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(get_event, F, Fixtures, F)
{
    bool requested = false;
    F::server.handle(http::Method::GET, "test/foo", [&](const http::Request&) {
        requested = true;
        return http::make_ready_response(http::Code::OK, jsonGet, JSON_TYPE);
    });

#if CLIENT_SUPPORTS_REP_ERRORS
    F::response = F::client.checkGET(F::server, "/unknown");
    BOOST_CHECK(!requested);
    BOOST_CHECK_EQUAL(F::response, error404);

    // regression check for bugfix #190 (segfault with GET '/')
    F::response = F::client.checkGET(F::server, "/");
    BOOST_CHECK(!requested);
    BOOST_CHECK_EQUAL(F::response, error404);
#endif

    F::response = F::client.checkGET(F::server, "/test/foo");
    BOOST_CHECK(requested);
    BOOST_CHECK_EQUAL(F::response, responseJsonGet);
}

#if CLIENT_SUPPORTS_REQ_PAYLOAD

BOOST_FIXTURE_TEST_CASE_TEMPLATE(put_object_json, F, Fixtures, F)
{
    F::server.handlePUT(F::foo.getEndpoint(), F::foo);

#if CLIENT_SUPPORTS_REP_ERRORS
    F::response = F::client.checkPUT(F::server, "/test/foo", "");
    BOOST_CHECK_EQUAL(F::response, error400);
    F::response = F::client.checkPUT(F::server, "/test/foo", "Foo");
    BOOST_CHECK_EQUAL(F::response, error400);
    F::response = F::client.checkPUT(F::server, "/test/bar", jsonPut);
    BOOST_CHECK_EQUAL(F::response, error404);
    BOOST_CHECK(!F::foo.getNotified());
#endif

    F::response = F::client.checkPUT(F::server, "/test/foo", jsonPut);
    BOOST_CHECK_EQUAL(F::response, response200);
    BOOST_CHECK(F::foo.getNotified());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(put_event, F, Fixtures, F)
{
    bool receivedEmpty = false;
    F::server.handle(http::Method::PUT, "empty", [&](const http::Request&) {
        receivedEmpty = true;
        return http::make_ready_response(http::Code::OK);
    });
    F::server.handle(http::Method::PUT, "foo",
                     [&](const http::Request& request) {
                         const auto code = jsonPut == request.body
                                               ? http::Code::OK
                                               : http::Code::BAD_REQUEST;
                         return http::make_ready_response(code);
                     });

#if CLIENT_SUPPORTS_REP_ERRORS
    F::response = F::client.checkPUT(F::server, "/foo", "");
    BOOST_CHECK_EQUAL(F::response, error400);
    F::response = F::client.checkPUT(F::server, "/foo", "Foo");
    BOOST_CHECK_EQUAL(F::response, error400);
    F::response = F::client.checkPUT(F::server, "/test/bar", jsonPut);
    BOOST_CHECK_EQUAL(F::response, error404);
#endif
    F::response = F::client.checkPUT(F::server, "/foo", jsonPut);
    BOOST_CHECK_EQUAL(F::response, response200);
    F::response = F::client.checkPUT(F::server, "/empty", " ");
    BOOST_CHECK_EQUAL(F::response, response200);
    BOOST_CHECK(receivedEmpty);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(post_serializable, F, Fixtures, F)
{
    F::server.handle(F::foo.getEndpoint(), F::foo);

    const http::Response error405GetPut{http::Code::NOT_SUPPORTED,
                                        "",
                                        {{http::Header::ALLOW, "GET, PUT"}}};

    F::response = F::client.checkPOST(F::server, "/test/foo", jsonPut);
    BOOST_CHECK_EQUAL(F::response, error405GetPut);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(handle_all_methods, F, Fixtures, F)
{
    // Register "echo" function for all methods
    for (int method = 0; method < int(http::Method::ALL); ++method)
        F::server.handle(http::Method(method), "path", echoFunc);

    // Extra function with no content
    F::server.handle(http::Method::GET, "nocontent", [](const http::Request&) {
        return http::make_ready_response(http::Code::NO_CONTENT);
    });

    const http::Response expectedResponse{http::Code::OK, "?query=:data"};
    const http::Response expectedResponseNoBody{http::Code::OK, "?query="};

    for (int method = 0; method < int(http::Method::ALL); ++method)
    {
        using Method = http::Method;
        const auto m = Method(method);
        // GET and DELETE should receive => return no payload
        if (m == Method::GET || m == Method::DELETE || m == Method::OPTIONS)
        {
            F::response = F::client.check(F::server, "/path?query", m, "");
            BOOST_CHECK_EQUAL(F::response, expectedResponseNoBody);
        }
        else
        {
            F::response = F::client.check(F::server, "/path?query", m, "data");
            BOOST_CHECK_EQUAL(F::response, expectedResponse);
        }
    }

    // Check extra function with no content
    F::response = F::client.checkGET(F::server, "/nocontent");
    BOOST_CHECK_EQUAL(F::response, response204);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(handle_root, F, Fixtures, F)
{
    F::server.handle(http::Method::GET, "", [](const http::Request&) {
        return http::make_ready_response(http::Code::OK, "homepage",
                                         "text/html");
    });
    F::server.handle(http::Method::PUT, "", [](const http::Request&) {
        return http::make_ready_response(http::Code::OK);
    });

    const http::Response expectedResponse{http::Code::OK,
                                          "homepage",
                                          {{http::Header::CONTENT_TYPE,
                                            "text/html"}}};

    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, ""), expectedResponse);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/"), expectedResponse);
    // Note: libwebsockets strips extra '/' so all the following are equivalent:
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "//"), expectedResponse);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "///"), expectedResponse);

    BOOST_CHECK_EQUAL(F::client.checkPUT(F::server, "", ""), response200);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(handle_root_path, F, Fixtures, F)
{
    F::server.handle(http::Method::GET, "/", echoFunc);
    const char* registry =
        R"({
   "/" : [ "GET" ]
}
)";
    const http::Response registryResponse{http::Code::OK, registry, JSON_TYPE};

    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, ""), response200);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/registry"),
                      registryResponse);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/ABC"),
                      http::Response(http::Code::OK, "ABC"));
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/"), response200);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "//"), response200);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/abc/def/"),
                      http::Response(http::Code::OK, "abc/def/"));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(handle_root_and_root_path, F, Fixtures, F)
{
    F::server.handle(http::Method::GET, "/", echoFunc);
    F::server.handle(http::Method::GET, "", [](const http::Request&) {
        return http::make_ready_response(http::Code::OK, "homepage",
                                         "text/html");
    });

    const http::Response htmlResponse{http::Code::OK,
                                      "homepage",
                                      {{http::Header::CONTENT_TYPE,
                                        "text/html"}}};

    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, ""), htmlResponse);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/"), htmlResponse);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "///"), htmlResponse);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/ABC"),
                      http::Response(http::Code::OK, "ABC"));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(handle_path, F, Fixtures, F)
{
    // Register callback function for all methods
    for (int method = 0; method < int(http::Method::ALL); ++method)
        F::server.handle(http::Method(method), "test/", echoFunc);

    const http::Response expectedResponse{http::Code::OK,
                                          "path/suffix:payload"};
    const http::Response expectedResponseNoBody{http::Code::OK, "path/suffix"};

    for (int method = 0; method < int(http::Method::ALL); ++method)
    {
        using Method = http::Method;
        const auto m = Method(method);
        // GET and DELETE should receive => return no payload
        if (m == Method::GET || m == Method::DELETE || m == Method::OPTIONS)
        {
            F::response =
                F::client.check(F::server, "/test/path/suffix", m, "");
            BOOST_CHECK_EQUAL(F::response, expectedResponseNoBody);
        }
        else
        {
            F::response =
                F::client.check(F::server, "/test/path/suffix", m, "payload");
            BOOST_CHECK_EQUAL(F::response, expectedResponse);
        }
    }

    // Test override endpoints
    const auto get = http::Method::GET;

    F::server.handle(get, "api/object/", echoFunc);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/api/object/"),
                      response200);

    F::server.handle(get, "api/object/properties/", echoFunc);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server,
                                         "/api/object/properties/color"),
                      http::Response(http::Code::OK, "color"));

    F::server.handle(get, "api/object/properties/color/", echoFunc);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server,
                                         "/api/object/properties/color/rgb"),
                      http::Response(http::Code::OK, "rgb"));

    // Test path is not the same as object
    F::server.handle(get, "api/size/", echoFunc);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/api/size"), error404);

    F::server.handle(get, "api/size", echoFunc);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/api/size"), response200);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(handle_headers, F, Fixtures, F)
{
    const std::string allow = "GET, POST, PUT, PATCH, DELETE";
    const std::string type = "text/plain";
    const std::string modified = "Wed, 21 Oct 2015 07:00:00 GMT";
    const std::string location = "index.html";
    const std::string retry = "60";

    for (int method = 0; method < int(http::Method::ALL); ++method)
    {
        F::server.handle(
            http::Method(method), "test/", [&](const http::Request& request) {
                std::map<http::Header, std::string> headers;
                headers[http::Header::ALLOW] = allow;
                headers[http::Header::CONTENT_TYPE] = type;
                headers[http::Header::LAST_MODIFIED] = modified;
                headers[http::Header::LOCATION] = location;
                headers[http::Header::RETRY_AFTER] = retry;
                const auto body = request.path + ":" + request.body;
                return http::make_ready_response(http::Code::OK, body,
                                                 std::move(headers));
            });
    }

    const std::map<http::Header, std::string> expectedHeaders{
        {http::Header::ALLOW, allow},
        {http::Header::CONTENT_TYPE, type},
        {http::Header::LAST_MODIFIED, modified},
        {http::Header::LOCATION, location},
        {http::Header::RETRY_AFTER, retry}};
    const http::Response expectedResponse{http::Code::OK, "path/suffix:",
                                          expectedHeaders};

    for (int method = 0; method < int(http::Method::ALL); ++method)
    {
        F::response = F::client.check(F::server, "/test/path/suffix",
                                      http::Method(method), "");
        BOOST_CHECK_EQUAL(F::response, expectedResponse);
    }
}

/* TODO need access to CORS headers
BOOST_FIXTURE_TEST_CASE_TEMPLATE(cors_preflight_options, F, Fixtures, F)
{
   F::server.handle(F::foo, F::foo.getEndpoint());
   const auto request = std::string("/test/foo");

   const http::Response corsResponse{http::Code::OK, std::string(),
                     {{"Access-Control-Allow-Headers", "Content-Type"},
                      {"Access-Control-Allow-Methods", "GET, PUT"},
                      {"Access-Control-Allow-Origin", "*"}}};

   F::client.checkCORS(F::request, "GET", response, __LINE__);
   F::client.checkCORS(F::request, "PUT", response, __LINE__);

   F::client.checkCORS(F::request, "POST", error405, __LINE__);
   F::client.checkCORS(F::request, "PATCH", error405, __LINE__);
   F::client.checkCORS(F::request, "DELETE", error405, __LINE__);
   F::client.checkCORS(F::request, "OPTIONS", error405, __LINE__);
}
*/

BOOST_FIXTURE_TEST_CASE_TEMPLATE(get_long_uri_query, F, Fixtures, F)
{
    F::server.handle(F::foo.getEndpoint(), F::foo);
    const size_t lwsMaxQuerySize = 4096 - 96 /*Unknown padding.*/;
    const auto path = std::string("/test/foo?");
    const auto size =
        lwsMaxQuerySize - (F::server.getURI().size() + path.size());
    const auto query = std::string(size, 'o');
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, path + query),
                      responseJsonGet);

    const auto excessiveQuery = std::string(4096, 'o');
    BOOST_CHECK_THROW(F::client.checkGET(F::server, path + excessiveQuery),
                      std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(urlcasesensitivity, F, Fixtures, F)
{
    F::server.handle(F::foo.getEndpoint(), F::foo);
    F::server.handle(http::Method::GET, "BlA/CamelCase",
                     [](const http::Request&) {
                         return http::make_ready_response(http::Code::OK, "{}");
                     });

    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/TEST/FOO"), error404);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/test/foo"),
                      responseJsonGet);

    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/BlA/CamelCase"),
                      http::Response(http::Code::OK, "{}"));
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/bla/camelcase"),
                      error404);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/bla/camel-case"),
                      error404);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(empty_registry, F, Fixtures, F)
{
    const http::Response emptyRegistry{http::Code::OK, "{}\n", JSON_TYPE};
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/registry"),
                      emptyRegistry);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(filled_registry, F, Fixtures, F)
{
    F::server.handle(F::foo.getEndpoint(), F::foo);
    F::server.handle(http::Method::PUT, "bla/bar", [](const http::Request&) {
        return http::make_ready_response(http::Code::OK);
    });

    for (int method = 0; method < int(http::Method::ALL); ++method)
        F::server.handle(http::Method(method), "all/", echoFunc);

    const char* registry =
        R"({
   "all/" : [ "GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS" ],
   "bla/bar" : [ "PUT" ],
   "test/foo" : [ "GET", "PUT" ]
}
)";
    const http::Response responseRegistry{http::Code::OK, registry, JSON_TYPE};
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/registry"),
                      responseRegistry);
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/bla/bar/registry"),
                      error404);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(event_registry_name, F, Fixtures, F)
{
    BOOST_CHECK_THROW(F::server.handle(http::Method::GET, "registry", echoFunc),
                      std::invalid_argument);
    BOOST_CHECK_THROW(F::server.handle(http::Method::PUT, "registry", echoFunc),
                      std::invalid_argument);

    BOOST_CHECK(F::server.handle(
        http::Method::GET, "foo/registry", [](const http::Request&) {
            return http::make_ready_response(http::Code::OK, "bar");
        }));
    BOOST_CHECK(F::server.handle(http::Method::PUT, "foo/registry",
                                 [](const http::Request&) {
                                     return http::make_ready_response(
                                         http::Code::OK);
                                 }));

    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/foo/registry"),
                      http::Response(http::Code::OK, "bar"));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(multiple_event_name_for_same_object, F,
                                 Fixtures, F)
{
    F::server.handleGET(F::foo.getEndpoint(), F::foo);
    F::server.handlePUT("test/camel-bar", F::foo);

    BOOST_CHECK_EQUAL(F::client.checkPUT(F::server, "/test/camel-bar", jsonPut),
                      response200);
    BOOST_CHECK(F::foo.getNotified());

    const http::Response error405get{http::Code::NOT_SUPPORTED,
                                     "",
                                     {{http::Header::ALLOW, "GET"}}};
    BOOST_CHECK_EQUAL(F::client.checkPUT(F::server, "/test/foo", ""),
                      error405get);

    F::foo.setNotified(false);

    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/test/foo"),
                      responseJsonGet);
    BOOST_CHECK(F::foo.getCalled());

    const http::Response error405put{http::Code::NOT_SUPPORTED,
                                     "",
                                     {{http::Header::ALLOW, "PUT"}}};
    BOOST_CHECK_EQUAL(F::client.checkGET(F::server, "/test/camel-bar"),
                      error405put);
}

#endif // CLIENT_SUPPORTS_REQ_PAYLOAD
#endif // CLIENT_SUPPORTS_REP_PAYLOAD
