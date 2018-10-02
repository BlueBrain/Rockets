// tslint:disable: forin
import {JSON_RPC_VERSION} from './constants';
import {Request} from './request';
import {isJsonRpcRequest} from './utils';

describe('Request', () => {
    it('complies with the JSON RPC 2.0 spec', () => {
        const method = 'test';
        const params = {ping: true};
        const request = new Request(method, params);
        expect(typeof request.id).toBe('string');
        expect(request.jsonrpc).toBe(JSON_RPC_VERSION);
        expect(request.method).toBe(method);
        expect(request.params).toEqual(params);
    });

    it('should skip the {params} if its not provided', () => {
        const request = new Request('test');
        expect(request.hasOwnProperty('params')).toBe(false);
    });

    describe('.toJSON()', () => {
        it('returns the Notification as an object', () => {
            const method = 'test';
            const params = {ping: true};
            const request = new Request(method, params);
            const obj = request.toJSON();
            expect(isJsonRpcRequest(obj)).toBe(true);
            expect(obj.params).toEqual(params);
            expect(obj.method).toBe(method);
        });

        it('skips {params} if not set', () => {
            const method = 'test';
            const request = new Request(method);
            const obj = request.toJSON();
            expect(isJsonRpcRequest(obj)).toBe(true);
            expect(obj.hasOwnProperty('params')).toBe(false);
            expect(obj.method).toBe(method);
        });

        it('JSON.stringify() uses this method', () => {
            const method = 'test';
            const params = {ping: true};
            const request = new Request(method, params);

            const toJSONSpy = spyOn(request, 'toJSON').and.callThrough();

            const str = JSON.stringify(request);

            expect(toJSONSpy).toHaveBeenCalled();

            expect(JSON.parse(str)).toEqual({
                method,
                params,
                jsonrpc: JSON_RPC_VERSION,
                id: request.id
            });
        });
    });

    describe('Iterator', () => {
        it('can iterate with for .. in', () => {
            const request = new Request('ping', {
                ping: true
            });

            const keys = [];

            // NOTE: If we target es5,
            // class methods will show up as well (e.g. 'toJSON')
            // https://github.com/Microsoft/TypeScript/issues/782
            for (const item in request) {
                keys.push(item);
            }

            expect(keys).toEqual([
                'method',
                'jsonrpc',
                'id',
                'params'
            ]);
        });

        it('can iterate with for .. of', () => {
            const method = 'test';
            const params = {ping: true};
            const request = new Request(method, params);

            const map = [];

            for (const item of request) {
                map.push(item);
            }

            expect(map).toEqual([
                ['method', method],
                ['jsonrpc', JSON_RPC_VERSION],
                ['id', request.id],
                ['params', params]
            ]);
        });

        test('Object.keys() does not include {response}', () => {
            const request = new Request('ping', {
                ping: true
            });
            const keys = Object.keys(request);

            expect(keys).toEqual([
                'method',
                'jsonrpc',
                'id',
                'params'
            ]);
        });
    });
});
