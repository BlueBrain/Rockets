import {JsonRpcError} from './error';

describe('JsonRpcError', () => {
    it('should be an instance of Error', () => {
        expect(new JsonRpcError({
            code: 0,
            message: 'Oops'
        })).toBeInstanceOf(Error);
    });

    it('should be an instance of itself', () => {
        expect(new JsonRpcError({
            code: 0,
            message: 'Oops'
        })).toBeInstanceOf(JsonRpcError);
    });

    it('should have the JSON RPC error object props', () => {
        const data = {ping: true};
        const err = new JsonRpcError({
            data,
            code: 0,
            message: 'Oops'
        });
        expect(err.code).toBe(0);
        expect(err.message).toBe('Oops');
        expect(err.data).toEqual(data);
    });
});
