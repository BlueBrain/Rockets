import uid from 'crypto-uid';
import {
    JSON_RPC_VERSION,
    UID_BYTE_LENGTH
} from './constants';
import {createJsonRpcNotification} from './testing';
import {
    isJsonRpcErrorObject,
    isJsonRpcNotification
} from './utils';


describe('isJsonRpcErrorObject()', () => {
    it('should return true if a value is a RocketsError', () => {
        const error = {code: 1, message: 'Oops'};
        expect(isJsonRpcErrorObject(error)).toBe(true);
    });

    it('should return false otherwise', () => {
        expect(isJsonRpcErrorObject({code: 1})).toBe(false);
        expect(isJsonRpcErrorObject({message: 'Oops'})).toBe(false);
        expect(isJsonRpcErrorObject({})).toBe(false);
        expect(isJsonRpcErrorObject(null)).toBe(false);
    });
});

describe('isJsonRpcNotification()', () => {
    it('should return true if a value is a Notification', () => {
        const value = createJsonRpcNotification('test');
        expect(isJsonRpcNotification(value)).toBe(true);
    });

    it('should not be true for a request', () => {
        const request = createRequest('test');
        expect(isJsonRpcNotification(request)).toBe(false);
    });

    it('should return false otherwise', () => {
        expect(isJsonRpcNotification({jsonrpc: JSON_RPC_VERSION})).toBe(false);
        expect(isJsonRpcNotification({
            jsonrpc: '1.0',
            method: 'test'
        })).toBe(false);
        expect(isJsonRpcNotification({method: 'test'})).toBe(false);
        expect(isJsonRpcNotification({})).toBe(false);
        expect(isJsonRpcNotification(null)).toBe(false);
    });
});


function createRequest<T>(method: string, params?: T) {
    const id = uid(UID_BYTE_LENGTH);
    return {
        method,
        id,
        params,
        jsonrpc: JSON_RPC_VERSION
    };
}
