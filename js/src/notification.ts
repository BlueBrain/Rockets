import {
    JSON_RPC_VERSION,
    JSON_RPC_VERSION_TYPE
} from './constants';
import {JsonRpcNotification} from './types';
import {setParams} from './utils';

export class Notification<T = any> implements JsonRpcNotification<T>, Iterable<any> {
    static fromJson<T>(json: JsonRpcNotification<T>) {
        return new this<T>(json.method, json.params);
    }

    readonly jsonrpc: JSON_RPC_VERSION_TYPE = JSON_RPC_VERSION;
    readonly params?: T;

    constructor(
        readonly method: string,
        params?: T
    ) {
        setParams(this, params);
    }

    isOfType(method: string) {
        return this.method === method;
    }

    /**
     * Make the class iterable
     * @see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Iterators_and_Generators
     */
    *[Symbol.iterator]() {
        for (const key in this) {
            if (this.hasOwnProperty(key)) {
                yield [key, this[key]];
            }
        }
    }

    /**
     * Custom JSON serializer
     * @see https://mzl.la/2hgTyXG
     */
    toJSON(): JsonRpcNotification<T> {
        const json: any = {
            ...this as object
        };
        return json;
    }
}
