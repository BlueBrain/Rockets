import {
    isNumber,
    isObject,
    isString
} from 'lodash';
import {JSON_RPC_VERSION} from './constants';
import {Notification} from './notification';
import {Request} from './request';
import {
    JsonRpcErrorObject,
    JsonRpcNotification
} from './types';

/**
 * Check if value is a RPC notification
 * @param value
 */
export function isJsonRpcNotification<T>(value: any): value is JsonRpcNotification<T> {
    return isObject(value)
        && value.jsonrpc === JSON_RPC_VERSION
        && isString(value.method)
        && !value.hasOwnProperty('id');
}

/**
 * Check if value is an RPC error
 * @param value
 */
export function isJsonRpcErrorObject<T>(value: any): value is JsonRpcErrorObject<T> {
    return isObject(value)
        && isNumber(value.code)
        && isString(value.message);
}

/**
 * @param obj
 * @param params
 * @private
 */
export function setParams<T>(
    obj: Request<T> | Notification<T>,
    params: T
): void {
    if (isObject(params) || Array.isArray(params)) {
        Object.assign(obj, {params});
    }
}
