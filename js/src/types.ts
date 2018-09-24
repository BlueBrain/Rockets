import {JSON_RPC_VERSION_TYPE} from './constants';

// http://www.jsonrpc.org/specification#notification
export type JsonRpcNotification<T = any> = CommonProps<T>
    & JsonRpcVersion;

// http://www.jsonrpc.org/specification#request_object
export interface JsonRpcRequest<T = any> extends CommonProps<T>, JsonRpcVersion {
    id: JsonRpcIdType;
}

// http://www.jsonrpc.org/specification#response_object
export interface JsonRpcResponse<T = any, E = any> extends JsonRpcVersion {
    id: JsonRpcIdType | null;
    error?: JsonRpcErrorObject<E>;
    result?: T;
}

// http://www.jsonrpc.org/specification#error_object
export interface JsonRpcErrorObject<T = any> {
    code: number;
    message: string;
    data?: T;
}

export type JsonRpcIdType = string | number;

export interface JsonRpcVersion {
    jsonrpc: JSON_RPC_VERSION_TYPE;
}

export interface CommonProps<T> {
    method: string;
    params?: T;
}

export interface Progress {
    amount: number;
    operation?: string;
}
