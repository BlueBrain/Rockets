// tslint:disable: forin
import {
    INVALID_PARAMS,
    JSON_RPC_VERSION
} from './constants';
import {JsonRpcError} from './error';
import {Response} from './response';
import {createJsonRpcResponse} from './testing';
import {JsonRpcResponse} from './types';

describe('Request', () => {
    it('complies with the JSON RPC 2.0 spec', () => {
        const rpcResponse = createJsonRpcResponse(true);
        const response = new Response(rpcResponse);
        expect(response.id).toBe(rpcResponse.id);
        expect(response.jsonrpc).toBe(JSON_RPC_VERSION);
    });

    describe('.json()', () => {
        it('should return the response result', async () => {
            const rpcResponse: JsonRpcResponse = {
                jsonrpc: JSON_RPC_VERSION,
                id: '123',
                result: true
            };
            const response = new Response(rpcResponse);
            expect(await response.json()).toBe(true);
        });

        it('should throw if the response is an error', async () => {
            const data = {ping: true};
            const rpcResponse: JsonRpcResponse = {
                jsonrpc: JSON_RPC_VERSION,
                id: '123',
                error: {
                    data,
                    code: INVALID_PARAMS,
                    message: 'Ops'
                }
            };
            const response = new Response(rpcResponse);
            try {
                await response.json();
            } catch (err) {
                expect(err instanceof JsonRpcError).toBe(true);
                const e: JsonRpcError = err;
                expect(e.code).toBe(INVALID_PARAMS);
                expect(e.message).toEqual('Ops');
                expect(e.data).toEqual(data);
            }
        });
    });

    describe('.toJSON()', () => {
        it('returns only {jsonrpc, id}', () => {
            const rpcResponse = createJsonRpcResponse(true);
            const response = new Response(rpcResponse);
            const obj = response.toJSON();
            expect(Object.keys(obj)).toHaveLength(2);
            expect(obj.jsonrpc).toBe(JSON_RPC_VERSION);
            expect(obj.id).toEqual(rpcResponse.id);
        });

        it('JSON.stringify() uses this method', () => {
            const rpcResponse = createJsonRpcResponse(true);
            const response = new Response(rpcResponse);

            const toJSONSpy = spyOn(response, 'toJSON').and.callThrough();

            const str = JSON.stringify(response);

            expect(toJSONSpy).toHaveBeenCalled();

            expect(JSON.parse(str)).toEqual({
                jsonrpc: JSON_RPC_VERSION,
                id: response.id
            });
        });
    });

    describe('Iterator', () => {
        it('can iterate with for .. in', () => {
            const rpcResponse = createJsonRpcResponse(true);
            const response = new Response(rpcResponse);

            const keys = [];

            // NOTE: If we target es5,
            // class methods will show up as well (e.g. 'toJSON')
            // https://github.com/Microsoft/TypeScript/issues/782
            for (const item in response) {
                keys.push(item);
            }

            expect(keys).toEqual([
                'jsonrpc',
                'id'
            ]);
        });

        it('can iterate with for .. of', () => {
            const rpcResponse = createJsonRpcResponse(true);
            const response = new Response(rpcResponse);

            const map = [];

            for (const item of response) {
                map.push(item);
            }

            expect(map).toEqual([
                ['jsonrpc', JSON_RPC_VERSION],
                ['id', response.id]
            ]);
        });

        test('Object.keys() does not include {response}', () => {
            const rpcResponse = createJsonRpcResponse(true);
            const response = new Response(rpcResponse);
            const keys = Object.keys(response);

            expect(keys).toEqual([
                'jsonrpc',
                'id'
            ]);
        });
    });
});
