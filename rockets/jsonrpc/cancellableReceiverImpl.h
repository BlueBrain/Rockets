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

#include "asyncReceiverImpl.h"
#include "helpers.h"
#include "utils.h"

#include <mutex>

namespace rockets
{
namespace jsonrpc
{
namespace
{
const std::string cancelMethodName = "cancel";
const std::string progressMethodName = "progress";
const char* reservedMethodError =
    "Method names starting with 'cancel' or 'progress' are reserved.";
const Response::Error requestAborted{"Request aborted",
                                     ErrorCode::request_aborted};
}

class CancellableReceiverImpl : public AsyncReceiverImpl
{
public:
    CancellableReceiverImpl(SendTextCallback sendTextCb)
        : _sendTextCb(sendTextCb)
    {
    }

    void registerMethod(const std::string& method,
                        CancellableResponseCallback action)
    {
        verifyValidMethodName(method);
        _methods[method] = action;
    }

    void verifyValidMethodName(const std::string& method) const final
    {
        if (begins_with(method, cancelMethodName) ||
            begins_with(method, progressMethodName))
        {
            throw std::invalid_argument(reservedMethodError);
        }
        AsyncReceiverImpl::verifyValidMethodName(method);
    }

    bool isRegisteredMethodName(const std::string& method) const override
    {
        return _methods.find(method) != _methods.end() ||
               method == cancelMethodName ||
               AsyncReceiverImpl::isRegisteredMethodName(method);
    }

    void process(const json& requestID, const std::string& method,
                 const Request& request, JsonResponseCallback respond) override
    {
        if (method == cancelMethodName)
        {
            processCancel(requestID, request);
            respond(json());
            return;
        }

        if (_methods.find(method) == _methods.end())
        {
            AsyncReceiverImpl::process(requestID, method, request, respond);
            return;
        }

        // temporary entry for the request is needed in case the action has an
        // early error, so skipResponse() works properly
        {
            std::lock_guard<std::mutex> lock(pendingRequests->mutex);
            pendingRequests->requests[requestID] = {[](VoidCallback done) {
                                                        done();
                                                    },
                                                    respond};
        }

        // detect if response send has to be skipped in case the request was
        // cancelled at the same time when it finishes.
        auto skipResponse = [ requestID, pendingRequests = pendingRequests ]
        {
            if (pendingRequests)
            {
                std::unique_lock<std::mutex> lock{pendingRequests->mutex,
                                                  std::defer_lock};
                if (!lock.try_lock())
                    return true;

                // if request has been cancelled and response is already sent,
                // don't send another response
                if (pendingRequests->requests.erase(requestID) == 0)
                    return true;
            }
            return false;
        };

        auto progressFunc =
            [ requestID, clientID = request.clientID,
              &sendText = _sendTextCb ](const std::string& msg,
                                        const float amount)
        {
            json progress{{"id", requestID},
                          {"amount", amount},
                          {"operation", msg}};
            sendText(makeNotification(progressMethodName, progress.dump()),
                     clientID);
        };

        const auto& func = _methods[method];
        auto cancelFunc =
            func(request,
                 [respond, requestID, skipResponse](const Response rep) {
                     if (skipResponse())
                         return;

                     // No reply for valid "notifications" (requests without an
                     // "id")
                     if (requestID.is_null())
                         respond(json());
                     else
                         respond(makeResponse(rep, requestID));
                 },
                 progressFunc);

        if (cancelFunc)
        {
            std::lock_guard<std::mutex> lock(pendingRequests->mutex);
            pendingRequests->requests[requestID].first = cancelFunc;
        }
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

        // cancel callback to the application. The response from the application
        // has to be a callback in case the cancel processing is blocking.
        cancelFunc->second.first(
            [ respond = cancelFunc->second.second, requestID ] {
                respond(makeErrorResponse(requestAborted, requestID));
            });

        pendingRequest.erase(requestID);
    }

private:
    SendTextCallback _sendTextCb;

    std::map<std::string, CancellableResponseCallback> _methods;

    struct PendingRequests
    {
        std::mutex mutex;
        std::map<json, std::pair<CancelRequestCallback, JsonResponseCallback>>
            requests;
    };

    std::shared_ptr<PendingRequests> pendingRequests{
        std::make_shared<PendingRequests>()};
};
}
}
