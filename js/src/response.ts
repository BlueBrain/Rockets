import {
    JSON_RPC_VERSION,
    JSON_RPC_VERSION_TYPE
} from './constants';
import {JsonRpcError} from './error';
import {
    JsonRpcIdType,
    JsonRpcResponse,
    JsonRpcVersion
} from './types';

export class Response implements JsonRpcVersion, Iterable<any> {
    readonly id: JsonRpcIdType | null;
    readonly jsonrpc: JSON_RPC_VERSION_TYPE = JSON_RPC_VERSION;

    // @ts-ignore
    private response: JsonRpcResponse;

    constructor(response: JsonRpcResponse) {
        this.id = response.id;
        Object.defineProperty(this, 'response', {
            enumerable: false,
            value: response
        });
    }

    async json<T>(): Promise<T> {
        const {error, result} = this.response;
        if (!result) {
            throw new JsonRpcError(error!);
        }
        return result;
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
    toJSON(): Pick<Response, 'id' | 'jsonrpc'> {
        const json: any = {
            ...this as object
        };
        return json;
    }
}
