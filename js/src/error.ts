import {JsonRpcErrorObject} from './types';

export class JsonRpcError<T = any> extends Error implements JsonRpcErrorObject<T> {
    code: number;
    data?: T;
    constructor({code, message, data}: JsonRpcErrorObject<T>) {
        super(message);
        // https://bit.ly/2j94DZX
        Object.setPrototypeOf(this, JsonRpcError.prototype);
        this.code = code;
        this.data = data;
    }
}
