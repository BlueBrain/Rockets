/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
 *                          Raphael.Dumusc@epfl.ch
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

#include "channel.h"

#include "response.h"
#include "utils.h"

namespace rockets
{
namespace http
{
namespace
{
const int HEADERS_BUFFER_SIZE = 4096;
const int MAX_HEADER_LENGTH = 512;
const int MAX_QUERY_PARAM_LENGTH = 4096;
const std::string JSON_TYPE = "application/json";

lws_token_indexes to_lws_token(const Header header)
{
    switch (header)
    {
    case Header::ALLOW:
        return WSI_TOKEN_HTTP_ALLOW;
    case Header::CONTENT_TYPE:
        return WSI_TOKEN_HTTP_CONTENT_TYPE;
    case Header::LAST_MODIFIED:
        return WSI_TOKEN_HTTP_LAST_MODIFIED;
    case Header::LOCATION:
        return WSI_TOKEN_HTTP_LOCATION;
    case Header::RETRY_AFTER:
        return WSI_TOKEN_HTTP_RETRY_AFTER;
    default:
        return WSI_TOKEN_COUNT; // should not happen
    }
}
} // anonymous namespace

Channel::Channel(lws* wsi_)
    : wsi{wsi_}
{
}

Method Channel::readMethod() const
{
    if (lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI) != 0)
        return Method::GET;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI) != 0)
        return Method::POST;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_PUT_URI) != 0)
        return Method::PUT;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_PATCH_URI) != 0)
        return Method::PATCH;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_DELETE_URI) != 0)
        return Method::DELETE;
    if (lws_hdr_total_length(wsi, WSI_TOKEN_OPTIONS_URI) != 0)
        return Method::OPTIONS;
    return Method::ALL;
}

std::string Channel::readOrigin() const
{
    return _readHeader(WSI_TOKEN_ORIGIN);
}

size_t Channel::readContentLength() const
{
    const auto header = _readHeader(WSI_TOKEN_HTTP_CONTENT_LENGTH);
    if (header.empty())
        return 0;
    try
    {
        return std::stoi(header);
    }
    catch (std::exception&)
    {
        return 0;
    }
}

std::map<std::string, std::string> Channel::readQueryParameters() const
{
    std::map<std::string, std::string> query;
    int n = 0;
    char buf[MAX_QUERY_PARAM_LENGTH];
    while (lws_hdr_copy_fragment(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_URI_ARGS,
                                 n++) > 0)
    {
        const std::string frag{buf};
        const auto equalPos = frag.find('=');
        if (equalPos != std::string::npos)
            query.emplace(frag.substr(0, equalPos), frag.substr(equalPos + 1));
        else
            query.emplace(frag, "");
    }
    return query;
}

CorsRequestHeaders Channel::readCorsRequestHeaders() const
{
    CorsRequestHeaders cors;
    cors.origin = _readHeader(WSI_TOKEN_ORIGIN);
    cors.accessControlRequestHeaders =
        _readHeader(WSI_TOKEN_HTTP_AC_REQUEST_HEADERS);

    // Missing support for Access-Control-Request-Method in libwebsockets
    // https://github.com/warmcat/libwebsockets/issues/853
    /*
    cors.accessControlRequestMethod =
        read_header(wsi, WSI_TOKEN_HTTP_AC_REQUEST_METHOD);
    */
    return cors;
}

void Channel::requestCallback()
{
    lws_callback_on_writable(wsi);
}

int Channel::writeResponseHeaders(const CorsResponseHeaders& corsHeaders,
                                  const Response& response)
{
    unsigned char buffer[HEADERS_BUFFER_SIZE];
    unsigned char* const start = buffer + LWS_PRE;
    unsigned char* const end = buffer + sizeof(buffer);
    unsigned char* p = start;

    if (lws_add_http_header_status(wsi, response.code, &p, end))
        return 1;

    if (lws_add_http_header_content_length(wsi, response.body.size(), &p, end))
        return 1;

    for (const auto& header : response.headers)
    {
        const auto token = to_lws_token(header.first);
        const auto& data = header.second;
        const auto value = reinterpret_cast<const unsigned char*>(data.c_str());
        const auto size = static_cast<int>(data.size());
        if (lws_add_http_header_by_token(wsi, token, value, size, &p, end))
            return 1;
    }

    for (const auto& header : corsHeaders)
    {
        const auto str = to_string(header.first) + ":";
        const auto& data = header.second;

        const auto value = reinterpret_cast<const unsigned char*>(data.c_str());
        const auto name = reinterpret_cast<const unsigned char*>(str.c_str());
        const auto size = static_cast<int>(data.size());
        if (lws_add_http_header_by_name(wsi, name, value, size, &p, end))
            return 1;
    }

    if (lws_finalize_http_header(wsi, &p, end))
        return 1;

    const int n = lws_write(wsi, start, p - start, LWS_WRITE_HTTP_HEADERS);
    if (n < 0)
        return -1;

    if (response.body.size() > 0)
    {
        // Only one lws_write() allowed, book another callback for sending body
        requestCallback();
        return 0;
    }

    // Close and free connection if complete, else keep open
    return lws_http_transaction_completed(wsi) ? -1 : 0;
}

int Channel::writeResponseBody(const Response& response)
{
    if (!_write(response.body, LWS_WRITE_HTTP_FINAL))
        return -1;

    // Close and free connection if complete, else keep open
    return lws_http_transaction_completed(wsi) ? -1 : 0;
}

#if LWS_LIBRARY_VERSION_NUMBER >= 2001000
int Channel::writeRequestBody(const std::string& body)
{
    if (!_write(body, LWS_WRITE_HTTP_FINAL))
        return -1;

    // Tell libwebsockets that we have finished sending the headers + body
    lws_client_http_body_pending(wsi, 0);
    return 0;
}

Code Channel::readResponseCode() const
{
    return Code(lws_http_client_http_response(wsi));
}
#endif

Response::Headers Channel::readResponseHeaders() const
{
    Response::Headers headers;
    for (auto header :
         {Header::ALLOW, Header::CONTENT_TYPE, Header::LAST_MODIFIED,
          Header::LOCATION, Header::RETRY_AFTER})
    {
        auto value = _readHeader(to_lws_token(header));
        if (!value.empty())
            headers.emplace(header, std::move(value));
    }
    return headers;
}

int Channel::writeRequestHeader(const std::string& body, unsigned char** buffer,
                                const size_t bufferSize)
{
#if LWS_LIBRARY_VERSION_NUMBER >= 2001000
    if (body.empty())
        return 0;

    const auto end = *buffer + bufferSize - 1;

    const auto length = std::to_string(body.size());
    const auto data = (unsigned char*)length.c_str();
    if (lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_CONTENT_LENGTH, data,
                                     length.size(), buffer, end))
    {
        return -1;
    }
    if (lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_CONTENT_TYPE,
                                     (unsigned char*)JSON_TYPE.c_str(),
                                     JSON_TYPE.size(), buffer, end))
    {
        return -1;
    }

    /* inform lws we have http body to send */
    lws_client_http_body_pending(wsi, 1);
    lws_callback_on_writable(wsi);
#else
#define UNUSED(expr) (void)(expr)
    UNUSED(body);
    UNUSED(buffer);
    UNUSED(bufferSize);
#endif
    return 0;
}

std::string Channel::_readHeader(const lws_token_indexes token) const
{
    const int length = lws_hdr_total_length(wsi, token) + 1;
    if (length == 1 || length >= MAX_HEADER_LENGTH)
        return std::string();
    char buf[MAX_HEADER_LENGTH];
    lws_hdr_copy(wsi, buf, length, token);
    return std::string(buf, (size_t)(length - 1));
}

bool Channel::_write(const std::string& message, lws_write_protocol protocol)
{
    auto buffer = std::string(LWS_PRE, '\0');
    buffer.append(message);
    auto data = (unsigned char*)(&buffer.data()[LWS_PRE]);
    return lws_write(wsi, data, message.size(), protocol) >= 0;
}
}
}
