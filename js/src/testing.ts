import uid from 'crypto-uid';
import {
    JSON_RPC_VERSION,
    UID_BYTE_LENGTH
} from './constants';
import {
    JsonRpcNotification,
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

export function createJsonRpcNotification<T>(method: string, params?: T): JsonRpcNotification<T> {
    return {
        method,
        params,
        jsonrpc: JSON_RPC_VERSION
    };
}
