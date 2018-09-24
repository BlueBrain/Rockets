// tslint:disable: forin
import {JSON_RPC_VERSION} from './constants';
import {Notification} from './notification';
import {createJsonRpcNotification} from './testing';
import {isJsonRpcNotification} from './utils';

describe('Notification', () => {
    it('complies with the JSON RPC 2.0 spec', () => {
        const method = 'test';
        const params = {ping: true};
        const notification = new Notification(method, params);
        expect(notification.jsonrpc).toBe(JSON_RPC_VERSION);
        expect(notification.method).toBe(method);
        expect(notification.params).toEqual(params);
    });

    it('should skip the {params} if its type does not follow the spec', () => {
        const notification = new Notification('test', null);
        expect(notification.hasOwnProperty('params')).toBe(false);
    });

    describe('.fromJson', () => {
        it('converts a JSON RPC notification into a Notification object', () => {
            const method = 'test';
            const params = {ping: true};
            const json = createJsonRpcNotification(method, params);
            const notification = Notification.fromJson(json);
            expect(notification).toBeInstanceOf(Notification);
            expect(notification.method).toBe(method);
            expect(notification.params).toEqual(params);
        });
    });

    describe('.isOfType()', () => {
        it('returns true if the Notification has the same method', () => {
            const notification = new Notification('test');
            expect(notification.isOfType('test')).toBe(true);
        });

        it('returns false otherwise', () => {
            const notification = new Notification('test');
            expect(notification.isOfType('test1')).toBe(false);
        });
    });

    describe('.toJSON()', () => {
        it('returns the Notification as an object', () => {
            const method = 'test';
            const params = {ping: true};
            const notification = new Notification(method, params);
            const obj = notification.toJSON();
            expect(isJsonRpcNotification(obj)).toBe(true);
            expect(obj.params).toEqual(params);
            expect(obj.method).toBe(method);
        });

        it('skips {params} if not set', () => {
            const method = 'test';
            const notification = new Notification(method);
            const obj = notification.toJSON();
            expect(isJsonRpcNotification(obj)).toBe(true);
            expect(obj.hasOwnProperty('params')).toBe(false);
            expect(obj.method).toBe(method);
        });

        it('JSON.stringify() uses this method', () => {
            const method = 'test';
            const params = {ping: true};
            const notification = new Notification(method, params);

            const toJSONSpy = spyOn(notification, 'toJSON').and.callThrough();

            const str = JSON.stringify(notification);

            expect(toJSONSpy).toHaveBeenCalled();

            const json = createJsonRpcNotification(method, params);
            expect(JSON.parse(str)).toEqual(json);
        });
    });

    describe('Iterator', () => {
        it('can iterate with for .. in', () => {
            const notification = new Notification('ping', {
                ping: true
            });

            const keys = [];

            // NOTE: If we target es5,
            // class methods will show up as well (e.g. 'toJSON')
            // https://github.com/Microsoft/TypeScript/issues/782
            for (const item in notification) {
                keys.push(item);
            }

            expect(keys).toEqual([
                'method',
                'jsonrpc',
                'params'
            ]);
        });

        it('can iterate with for .. of', () => {
            const method = 'ping';
            const params = {
                ping: true
            };
            const notification = new Notification(method, params);

            const map = [];

            for (const item of notification) {
                map.push(item);
            }

            expect(map).toEqual([
                ['method', method],
                ['jsonrpc', JSON_RPC_VERSION],
                ['params', params]
            ]);
        });

        test('Object.keys() does not include {response}', () => {
            const notification = new Notification('ping', {
                ping: true
            });
            const keys = Object.keys(notification);

            expect(keys).toEqual([
                'method',
                'jsonrpc',
                'params'
            ]);
        });
    });
});
