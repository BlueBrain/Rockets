/* Copyright (c) 2017-2018, EPFL/Blue Brain Project
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

#include "asyncReceiver.h"
#include "requestProcessor.h"
#include "utils.h"

namespace rockets
{
namespace jsonrpc
{
namespace
{
const std::string cancelMethodName = "cancel";
const char* reservedMethodError =
    "Method names starting with 'cancel' are reserved by the async receiver.";
const Response::Error requestAborted{"Request aborted",
                                     ErrorCode::request_aborted};
}

class AsyncReceiver::Impl : public RequestProcessor
{
public:
    bool _isValidMethodName(const std::string& method) const final
    {
        return _methods.find(method) != _methods.end() ||
               method == cancelMethodName;
    }

    void _registerMethod(const std::string& method,
                         DelayedResponseCallback action)
    {
        if (begins_with(method, cancelMethodName))
            throw std::invalid_argument(reservedMethodError);
        _methods[method] = action;
    }

    void _process(const json& requestID, const std::string& method,
                  const Request& request, JsonResponseCallback respond) final
    {
        if (method == cancelMethodName)
        {
            processCancel(requestID, request);
            respond(json());
            return;
        }

        const auto& cancelFunc = cancels.find(method);
        if (cancelFunc != cancels.end())
        {
            std::lock_guard<std::mutex> lock(pendingRequests->mutex);
            pendingRequests->requests[requestID] = {cancelFunc->second,
                                                    respond};
        }

        auto skipResponse = [
            requestID, pendingRequests = cancelFunc != cancels.end()
                                             ? pendingRequests
                                             : nullptr
        ]
        {
            if (pendingRequests)
            {
                std::lock_guard<std::mutex> lock(pendingRequests->mutex);

                // if request has been cancelled and response is already sent,
                // don't send another response
                if (pendingRequests->requests.erase(requestID) == 0)
                    return true;
            }
            return false;
        };

        const auto& func = _methods[method];
        func(request, [respond, requestID, skipResponse](const Response rep) {
            if (skipResponse())
                return;

            // No reply for valid "notifications" (requests without an "id")
            if (requestID.is_null())
                respond(json());
            else
                respond(makeResponse(rep, requestID));
        });
    }

    void processCancel(const json& id, const Request& request)
    {
        auto params = json::parse(request.message, nullptr, false);

        // need a valid notification with the ID of the request to cancel
        const bool isNotification = id.is_null();
        if (!isNotification || !params.count("id"))
            return;

        std::lock_guard<std::mutex> lock(pendingRequests->mutex);

        // invalid request ID or request already processed
        const auto requestID = params["id"];
        auto& pendingRequest = pendingRequests->requests;
        auto cancelFunc = pendingRequest.find(requestID);
        if (cancelFunc == pendingRequest.end())
            return;

        // cancel callback to the application
        cancelFunc->second.first();

        // respond to request
        cancelFunc->second.second(makeErrorResponse(requestAborted, requestID));

        pendingRequest.erase(requestID);
    }

    std::map<std::string, CancelRequestCallback> cancels;

private:
    std::map<std::string, DelayedResponseCallback> _methods;

    struct PendingRequests
    {
        std::mutex mutex;
        std::map<json, std::pair<CancelRequestCallback, JsonResponseCallback>>
            requests;
    };

    std::shared_ptr<PendingRequests> pendingRequests{
        std::make_shared<PendingRequests>()};
};

AsyncReceiver::AsyncReceiver()
    : _impl{new Impl}
{
}

void AsyncReceiver::bindAsync(const std::string& method,
                              DelayedResponseCallback action,
                              CancelRequestCallback cancel)
{
    ReceiverInterface::bindAsync(method, action);
    _impl->cancels[method] = cancel;
}

void AsyncReceiver::_process(const Request& request,
                             AsyncStringResponse callback)
{
    _impl->process(request, callback);
}

void AsyncReceiver::_registerMethod(const std::string& method,
                                    DelayedResponseCallback action)
{
    _impl->_registerMethod(method, action);
}
}
}
