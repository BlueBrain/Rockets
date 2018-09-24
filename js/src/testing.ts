import uid from 'crypto-uid';
import {
    isNumber,
    isObject,
    isString
} from 'lodash';
import {
    JSON_RPC_VERSION,
    UID_BYTE_LENGTH
} from './constants';
import {
    JsonRpcNotification,
    JsonRpcRequest,
    JsonRpcResponse
} from './types';

export function createJsonRpcResponse<T>(result: T): JsonRpcResponse<T> {
    const id = uid(UID_BYTE_LENGTH);
    return {
        id,
        result,
        jsonrpc: JSON_RPC_VERSION
    };
}

export function isJsonRpcRequest<T>(value: any): value is JsonRpcRequest<T> {
    return isObject(value)
        && value.jsonrpc === JSON_RPC_VERSION
        && isString(value.method)
        && (isString(value.id) || isNumber(value.id));
}

export function createJsonRpcNotification<T>(method: string, params?: T): JsonRpcNotification<T> {
    return {
        method,
        params,
        jsonrpc: JSON_RPC_VERSION
    };
}
