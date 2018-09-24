import uid from 'crypto-uid';
import {
    JSON_RPC_VERSION,
    JSON_RPC_VERSION_TYPE,
    UID_BYTE_LENGTH
} from './constants';
import {JsonRpcIdType, JsonRpcRequest} from './types';
import {setParams} from './utils';

export class Request<T = any> implements JsonRpcRequest<T>, Iterable<any> {
    readonly jsonrpc: JSON_RPC_VERSION_TYPE = JSON_RPC_VERSION;
    readonly id: JsonRpcIdType = uid(UID_BYTE_LENGTH);
    readonly params?: T;

    constructor(
        readonly method: string,
        params?: T
    ) {
        setParams(this, params);
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
    toJSON(): JsonRpcRequest<T> {
        const json: any = {
            ...this as object
        };
        return json;
    }
}
