import uid from 'crypto-uid';
import {
    JSON_RPC_VERSION,
    UID_BYTE_LENGTH
} from './constants';
import {createJsonRpcNotification} from './testing';
import {
    isJsonRpcErrorObject,
    isJsonRpcNotification,
    isJsonRpcRequest
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
    it('should return true if a value is a JsonRpcNotification', () => {
        const value = createJsonRpcNotification('test');
        expect(isJsonRpcNotification(value)).toBe(true);
    });

    it('should not be true for a request', () => {
        const request = createJsonRpcRequest('test');
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

describe('isJsonRpcRequest()', () => {
    it('should return true if a value is a JsonRpcRequest', () => {
        const request = createJsonRpcRequest('test');
        expect(isJsonRpcRequest(request)).toBe(true);
    });

    it('should not be true for a notification', () => {
        const notification = createJsonRpcNotification('test');
        expect(isJsonRpcRequest(notification)).toBe(false);
    });

    it('should return false otherwise', () => {
        expect(isJsonRpcRequest({jsonrpc: JSON_RPC_VERSION})).toBe(false);
        expect(isJsonRpcRequest({
            jsonrpc: '1.0',
            method: 'test',
            id: 2
        })).toBe(false);
        expect(isJsonRpcRequest({
            jsonrpc: JSON_RPC_VERSION,
            method: 'test'
        })).toBe(false);
        expect(isJsonRpcRequest(null)).toBe(false);
        expect(isJsonRpcRequest([])).toBe(false);
        expect(isJsonRpcRequest({})).toBe(false);
        expect(isJsonRpcRequest(true)).toBe(false);
        expect(isJsonRpcRequest(1)).toBe(false);
    });
});


function createJsonRpcRequest<T>(method: string, params?: T) {
    const id = uid(UID_BYTE_LENGTH);
    return {
        method,
        id,
        params,
        jsonrpc: JSON_RPC_VERSION
    };
}
