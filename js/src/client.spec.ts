import uid from 'crypto-uid';
import {
    isFunction,
    isObject,
    isString
} from 'lodash';
import {Server} from 'mock-socket';
import {
    merge,
    noop,
    Observable,
    timer
} from 'rxjs';
import {map, take} from 'rxjs/operators';
import {
    Client,
    createBatchResponseFilter,
    createProgressAggregate,
    createProgressFilter,
    createResponseFilter,
    createWs,
    fromJsonAsync,
    isJsonRpcMessage,
    isJsonRpcObject,
    isJsonRpcResponse,
    ProgressNotification,
    setWsProtocol
} from './client';
import {
    CANCEL,
    INVALID_REQUEST,
    JSON_RPC_VERSION,
    METHOD_NOT_FOUND,
    PROGRESS,
    REQUEST_ABORTED,
    SOCKET_CLOSED,
    SOCKET_PIPE_BROKEN,
    UID_BYTE_LENGTH
} from './constants';
import {JsonRpcError} from './error';
import {Notification} from './notification';
import {Request} from './request';
import {Response} from './response';
import {createJsonRpcResponse} from './testing';
import {
    JsonRpcErrorObject,
    JsonRpcNotification,
    JsonRpcRequest,
    JsonRpcResponse
} from './types';
import {
    isJsonRpcNotification,
    isJsonRpcRequest
} from './utils';


describe('Client', () => {
    it('should have a static .create() method that creates a new Client', () => {
        expect(typeof Client.create).toBe('function');
        const rpc = Client.create({url: 'test'});
        expect(rpc).toBeInstanceOf(Client);
    });

    it('should create a WebSocketSubject on init with the given config', done => {
        const host = 'myhost';
        const protocol = 'myprotocol';
        const mockServer = createMockServer(host);

        mockServer.on('connection', socket => {
            expect(socket.url.includes(host)).toBe(true);
            expect(socket.protocol).toBe(protocol);
            mockServer.stop(done);
        });

        Client.create({
            protocol,
            url: host
        });
    });

    it('should call onConnected() cb when connected', done => {
        const host = 'myhost';
        const mockServer = createMockServer(host);

        Client.create({
            url: host,
            onConnected() {
                mockServer.stop(done);
            }
        });
    });

    it('should call onClosed() cb when disconnected', done => {
        const host = 'myhost';

        const mockServer = createMockServer(host);
        mockServer.on('connection', socket => {
            socket.close(0, 'Why not?');
        });

        Client.create({
            url: host,
            onClosed(evt) {
                expect(evt.wasClean).toBe(true);
                mockServer.stop(done);
            }
        });
    });

    describe('.request()', () => {
        const host = 'myhost';
        let mockServer: Server;
        let rpc: Client;
        beforeEach(() => {
            mockServer = createMockServer(host);
            rpc = Client.create({url: host});
        });
        afterEach(done => {
            mockServer.stop(done);
        });

        it('should return a Promise like object', () => {
            const task = rpc.request('test');
            expect(isObject(task)).toBe(true);
            expect(isFunction(task.then)).toBe(true);
        });

        it('should have the request object as a property', () => {
            const method = 'test';
            const params = {ping: true};

            const task = rpc.request(method, params);
            expect(isObject(task.request)).toBe(true);
            expect(task.request.id).toBeDefined();
            expect(task.request.jsonrpc).toBe(JSON_RPC_VERSION);
            expect(task.request.method).toBe(method);
            expect(task.request.params).toEqual(params);
        });

        it('should send a request RPC using the socket', done => {
            const method = 'test';
            const params = {ping: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<any>(data);

                    expect(json.id).toBeDefined();
                    expect(json.method).toBe(method);
                    expect(json.jsonrpc).toBe(JSON_RPC_VERSION);
                    expect(json.params).toEqual(params);

                    done();
                });
            });

            rpc.request(method, params);
        });

        it('can accept a Request instance', done => {
            const method = 'test';
            const params = {ping: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<any>(data);

                    expect(json.id).toBeDefined();
                    expect(json.method).toBe(method);
                    expect(json.jsonrpc).toBe(JSON_RPC_VERSION);
                    expect(json.params).toEqual(params);

                    done();
                });
            });

            const request = new Request(method, params);
            rpc.request(request);
        });

        it('should resolve when the socket emits the response', async done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcResponseJson(request);
                    socket.send(response);
                });
            });

            rpc.request('test')
                .then(res => {
                    expect(res).toBe(true);
                    done();
                }, () => {
                    done.fail('Request failed');
                });
        });

        it('should invoke the success cb if attached to the request promise after it resolved', done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcResponseJson(request);
                    socket.send(response);
                });
            });

            const task = rpc.request('test');

            task.then(() => {
                task.then(res => {
                    expect(res).toBe(true);
                    done();
                });
            });
        });

        it('should work with async/await', async done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcResponseJson(request);
                    socket.send(response);
                });
            });

            try {
                const res = await rpc.request('test');
                expect(res).toBe(true);
                done();
            } catch (err) {
                done.fail(err);
            }
        });

        it('should ignore anything that is not a request object', async done => {
            const notification = new Notification('test-notification');

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    socket.send(toJson(notification));

                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcResponseJson(request);
                    socket.send(response);
                });
            });

            try {
                const res = await rpc.request('test-request');
                expect(res).toBe(true);
                done();
            } catch (err) {
                done.fail(err);
            }
        });

        it('should ignore responses for other requests', async done => {
            const method = 'test-req';

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    socket.send(toJsonRpcResponseJson(new Request(method)));

                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcResponseJson(request);
                    socket.send(response);
                });
            });

            try {
                const res = await rpc.request(method);
                expect(res).toBe(true);
                done();
            } catch (err) {
                done.fail(err);
            }
        });

        it('should reject when the socket emits an error response', async done => {
            const error = {
                code: METHOD_NOT_FOUND,
                message: 'Method not found'
            };

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcErrorObjectJson(request, error);
                    socket.send(response);
                });
            });

            try {
                await rpc.request('test');
                done.fail('Request should have failed');
            } catch (err) {
                expect(err).toBeInstanceOf(JsonRpcError);
                expect(err.code).toBe(error.code);
                expect(err.message).toBe(error.message);
                done();
            }
        });

        it('should reject when the server closes the socket', async done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', () => {
                    socket.close();
                });
            });

            try {
                await rpc.request('test');
                done.fail('Request should have failed');
            } catch (err) {
                expect(err).toBeInstanceOf(JsonRpcError);
                expect(err.code).toBe(SOCKET_CLOSED);
                done();
            }
        });

        it('should reject when the client closes the socket via unsubscribe()', async done => {
            mockServer.on('connection', () => {
                rpc.ws.unsubscribe();
            });

            try {
                await rpc.request('test');
                done.fail('Request should have failed');
            } catch (err) {
                expect(err).toBeInstanceOf(JsonRpcError);
                expect(err.code).toBe(SOCKET_CLOSED);
                done();
            }
        });

        it('should reject when the client closes the socket via complete()', async done => {
            mockServer.on('connection', () => {
                rpc.ws.complete();
            });

            try {
                await rpc.request('test');
                done.fail('Request should have failed');
            } catch (err) {
                expect(err).toBeInstanceOf(JsonRpcError);
                expect(err.code).toBe(SOCKET_CLOSED);
                done();
            }
        });

        it('should reject when the client errors', async done => {
            mockServer.on('connection', () => {
                rpc.ws.error({});
            });

            try {
                await rpc.request('test');
                done.fail('Request should have failed');
            } catch (err) {
                expect(err).toBeInstanceOf(JsonRpcError);
                expect(err.code).toBe(SOCKET_PIPE_BROKEN);
                done();
            }
        });

        it('should reject when the connection with the ws is closed', async done => {
            mockServer.close({
                code: 1005,
                reason: 'Why not?',
                wasClean: false
            });

            try {
                await rpc.request('test');
                done.fail('Request should have failed');
            } catch (err) {
                expect(err).toBeInstanceOf(JsonRpcError);
                expect(err.code).toBe(SOCKET_PIPE_BROKEN);
                done();
            }
        });

        it('should invoke the failure cb if attached to the request promise after it rejected', done => {
            const error = {
                code: METHOD_NOT_FOUND,
                message: 'Method not found'
            };

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcErrorObjectJson(request, error);
                    socket.send(response);
                });
            });

            const task = rpc.request('test');

            task.then(noop, () => {
                task.then(noop, err => {
                    expect(err).toBeInstanceOf(JsonRpcError);
                    expect(err.code).toBe(error.code);
                    expect(err.message).toBe(error.message);
                    done();
                });
            });
        });

        it('should invoke all attached success cb fns on resolve', done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcResponseJson(request);
                    socket.send(response);
                });
            });

            const onSuccess = jest.fn();
            const task = rpc.request('test');

            Promise.all([
                task.then(onSuccess),
                task.then(onSuccess)
            ]).then(() => {
                expect(onSuccess).toHaveBeenCalledTimes(2);
                const args = onSuccess.mock.calls.map(call => call[0]);
                expect(args.every(arg => arg === true)).toBe(true);
                done();
            });
        });

        it('should invoke all attached failure cb fns on reject', done => {
            const error = {
                code: METHOD_NOT_FOUND,
                message: 'Method not found'
            };

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcErrorObjectJson(request, error);
                    socket.send(response);
                });
            });

            const onFailure = jest.fn();
            const task = rpc.request('test');

            Promise.all([
                task.then(noop, onFailure),
                task.then(noop, onFailure)
            ]).then(() => {
                expect(onFailure).toHaveBeenCalledTimes(2);
                const args = onFailure.mock.calls.map(call => call[0]);
                expect(args.every(arg => arg.code === error.code && arg.message === error.message)).toBe(true);
                done();
            });
        });

        it('can chain promises', async done => {
            const result = {
                ping: true
            };

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcResponseJson(request, result);
                    socket.send(response);
                });
            });

            rpc.request<undefined, typeof result>('test')
                .then(res => res.ping)
                .then(res => {
                    expect(res).toBe(true);
                    done();
                });
        });

        describe('.cancel()', () => {
            it('should send a cancel notification for the current request', done => {
                const error = {
                    code: REQUEST_ABORTED,
                    message: 'Request aborted'
                };

                mockServer.on('connection', socket => {
                    let r: JsonRpcRequest;

                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const json = fromJson(data);

                        if (isJsonRpcNotification(json)) {
                            expect(json.method).toBe(CANCEL);
                            expect(json.params).toEqual({
                                id: r.id
                            });

                            const response = toJsonRpcErrorObjectJson(r, error);
                            socket.send(response);
                        } else if (isJsonRpcRequest(json)) {
                            r = json;
                        }
                    });
                });

                const task = rpc.request('test');

                task.then(noop, err => {
                    expect(err).toBeInstanceOf(JsonRpcError);
                    expect(err.code).toBe(error.code);
                    expect(err.message).toBe(error.message);
                    done();
                });

                task.cancel();
            });

            it('should do nothing if already resolved', done => {
                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const json = fromJson(data);
                        if (isJsonRpcRequest(json)) {
                            const request = fromJson<JsonRpcRequest>(data);
                            const response = toJsonRpcResponseJson(request);
                            socket.send(response);
                        } else {
                            done.fail('Cancel should not have send a notification');
                        }
                    });
                });

                const task = rpc.request('test');

                task.then(() => task.cancel())
                    .then(() => {
                        done();
                    });
            });

            it('should not do anything if it\'s already pending cancel', done => {
                const cancelSpy = jest.fn();

                const error = {
                    code: REQUEST_ABORTED,
                    message: 'Request aborted'
                };

                mockServer.on('connection', socket => {
                    let r: JsonRpcRequest;

                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const json = fromJson(data);

                        if (isJsonRpcNotification(json)) {
                            setTimeout(() => {
                                expect(json.method).toBe(CANCEL);
                                expect(json.params).toEqual({
                                    id: r.id
                                });

                                const response = toJsonRpcErrorObjectJson(r, error);
                                socket.send(response);
                            }, 100);
                            cancelSpy();
                        } else if (isJsonRpcRequest(json)) {
                            r = json;
                        }
                    });
                });

                const task = rpc.request('test');

                task.then(noop, err => {
                    expect(err).toBeInstanceOf(JsonRpcError);
                    expect(err.code).toBe(error.code);
                    expect(err.message).toBe(error.message);
                    expect(cancelSpy).toHaveBeenCalledTimes(1);
                    done();
                });

                task.cancel();
                task.cancel();
            });
        });

        describe('.on()', () => {
            it('should return an Observable', () => {
                const task = rpc.request('test');
                expect(task.on('progress')).toBeInstanceOf(Observable);
            });

            it('can emit Progress when socket sends progress notifications for the request', done => {
                const progressSpy = jest.fn();

                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const request = fromJson<JsonRpcRequest>(data);

                        timer(0, 10)
                            .pipe(take(11))
                            .subscribe(num => {
                                const notification = new Notification(PROGRESS, {
                                    id: request.id,
                                    operation: 'Doing ...',
                                    amount: num / 10
                                });
                                const message = toJson(notification);
                                socket.send(message);
                            }, noop, () => requestAnimationFrame(() => {
                                expect(progressSpy).toHaveBeenCalledTimes(11);

                                const args = progressSpy.mock.calls.map(c => c[0]);
                                for (const [index, arg] of args.entries()) {
                                    expect(arg.amount).toBe(index / 10);
                                    expect(isString(arg.operation)).toBe(true);
                                }

                                done();
                            }));
                    });
                });

                const task = rpc.request('test');

                task.on('progress')
                    .subscribe(progressSpy);
            });

            it('should throw when the request throws', done => {
                const error = {
                    code: METHOD_NOT_FOUND,
                    message: 'Method not found'
                };

                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const request = fromJson<JsonRpcRequest>(data);
                        const response = toJsonRpcErrorObjectJson(request, error);
                        socket.send(response);
                    });
                });

                const task = rpc.request('test');

                task.on('progress')
                    .subscribe({
                        error: err => {
                            expect(err).toBeInstanceOf(JsonRpcError);
                            expect(err.code).toBe(error.code);
                            expect(err.message).toBe(error.message);
                            done();
                        }
                    });
            });

            it('should complete when the request is done', done => {
                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const request = fromJson<JsonRpcRequest>(data);
                        const response = toJsonRpcResponseJson(request);
                        socket.send(response);
                    });
                });

                const task = rpc.request('test');

                task.on('progress')
                    .subscribe({
                        complete: () => {
                            done();
                        }
                    });
            });

            it('replays last known value', done => {
                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const request = fromJson<JsonRpcRequest>(data);
                        const notification = new Notification(PROGRESS, {
                            id: request.id,
                            operation: 'Doing ...',
                            amount: 0.4
                        });
                        const message = toJson(notification);
                        socket.send(message);
                    });
                });

                const task = rpc.request('test');
                const obs = task.on('progress');

                setTimeout(() => {
                    obs.subscribe(progress => {
                        expect(progress.amount).toBe(0.4);
                        done();
                    });
                }, 50);
            });

            it('should return an empty Observable for unknown events', () => {
                const task = rpc.request('test');
                expect(task.on('test' as any)).toBeInstanceOf(Observable);
            });
        });
    });

    describe('.notify()', () => {
        const host = 'myhost';
        let mockServer: Server;
        let rpc: Client;
        beforeEach(() => {
            mockServer = createMockServer(host);
            rpc = Client.create({url: host});
        });
        afterEach(done => {
            mockServer.stop(done);
        });

        it('should send events using the socket', done => {
            const method = 'test';
            const params = {ping: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<JsonRpcNotification>(data);
                    expect(json.method).toBe(method);
                    expect(json.params).toEqual(params);
                    done();
                });
            });

            rpc.notify(method, params);
        });

        it('cans send notification objects', done => {
            const method = 'test';
            const params = {ping: true};
            const notification = new Notification(method, params);

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<JsonRpcNotification>(data);
                    expect(json.method).toBe(method);
                    expect(json.params).toEqual(params);
                    done();
                });
            });

            rpc.notify(notification);
        });
    });

    describe('.batch()', () => {
        const host = 'myhost';
        let mockServer: Server;
        let rpc: Client;
        beforeEach(() => {
            mockServer = createMockServer(host);
            rpc = Client.create({url: host});
        });
        afterEach(done => {
            mockServer.stop(done);
        });

        it('should return a Promise like object', () => {
            const task = rpc.batch(...[
                new Request('ping')
            ]);
            expect(isObject(task)).toBe(true);
            expect(isFunction(task.then)).toBe(true);
        });

        it('should send request RPCs using the socket', done => {
            const params = {ping: true};
            const requests = [
                new Request('ping', params),
                new Request('pong', params)
            ];

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<JsonRpcRequest[]>(data);
                    expect(Array.isArray(json)).toBe(true);
                    expect(json).toHaveLength(2);
                    expect(json.every(isJsonRpcRequest)).toBe(true);

                    const [ping] = json;
                    expect(ping.method).toBe('ping');
                    expect(ping.params).toEqual(params);

                    const [, pong] = json;
                    expect(pong.method).toBe('pong');
                    expect(pong.params).toEqual(params);

                    done();
                });
            });

            rpc.batch(...requests);
        });

        it('should send notification RPCs using the socket', done => {
            const params = {ping: true};
            const requests = [
                new Notification('ping', params),
                new Notification('pong', params)
            ];

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<JsonRpcNotification[]>(data);
                    expect(Array.isArray(json)).toBe(true);
                    expect(json).toHaveLength(2);
                    expect(json.every(isJsonRpcNotification)).toBe(true);

                    const [ping] = json;
                    expect(ping.method).toBe('ping');
                    expect(ping.params).toEqual(params);

                    const [, pong] = json;
                    expect(pong.method).toBe('pong');
                    expect(pong.params).toEqual(params);

                    done();
                });
            });

            rpc.batch(...requests);
        });

        it('can send a mix of notification and request RPCs using the socket', done => {
            const params = {ping: true};
            const requests = [
                new Request('ping', params),
                new Notification('pong', params)
            ];

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<any[]>(data);
                    expect(Array.isArray(json)).toBe(true);
                    expect(json).toHaveLength(2);

                    const [ping] = json;
                    expect(isJsonRpcRequest(ping)).toBe(true);
                    expect(ping.method).toBe('ping');
                    expect(ping.params).toEqual(params);

                    const [, pong] = json;
                    expect(isJsonRpcNotification(pong)).toBe(true);
                    expect(pong.method).toBe('pong');
                    expect(pong.params).toEqual(params);

                    done();
                });
            });

            rpc.batch(...requests);
        });

        it('should resolve when the socket emits the responses for all the requests', done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const requests = fromJson<JsonRpcRequest[]>(data);
                    const response = toBatchJsonRpcResponseJson(requests);
                    socket.send(response);
                });
            });

            const request = new Request('ping');
            rpc.batch(request)
                .then(async response => {
                    const err = 'Response is not an array of JSON RPC response objects';
                    const [item] = response;
                    if (item instanceof Response) {
                        expect(item.id).toBe(request.id);
                        try {
                            const result = await item.json<boolean>();
                            expect(result).toBe(true);
                            done();
                        } catch (e) {
                            done.fail(e);
                        }
                    } else {
                        done.fail(err);
                    }
                }, err => {
                    done.fail(err);
                });
        });

        it('should return responses only for requests', done => {
            const requests = [
                new Request('ping'),
                new Request('ping')
            ];
            const ids = requests.map(req => req.id);
            const items = [
                new Notification('pong'),
                ...requests
            ];

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<any[]>(data);
                    const requests = json.filter(isJsonRpcRequest);
                    const response = toBatchJsonRpcResponseJson(requests);
                    socket.send(response);
                });
            });

            rpc.batch(...items)
                .then(async response => {
                    const err = 'Response is not an array of JSON RPC response objects';

                    expect(response).toHaveLength(2);

                    for (const item of response) {
                        if (item instanceof Response) {
                            expect(ids.includes(item.id as any)).toBe(true);
                            try {
                                const result = await item.json<boolean>();
                                expect(result).toBe(true);
                                done();
                            } catch (e) {
                                done.fail(e);
                            }
                        } else {
                            done.fail(err);
                            break;
                        }
                    }
                }, err => {
                    done.fail(err);
                });
        });

        it('should return nothing for notifications', done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<any[]>(data);
                    const requests = json.filter(isJsonRpcRequest);
                    if (requests.length > 0) {
                        const response = toBatchJsonRpcResponseJson(requests);
                        socket.send(response);
                    }
                });
            });

            const notification = new Notification('ping');
            rpc.batch(notification)
                .then(response => {
                    expect(response).toBeUndefined();
                    done();
                }, () => {
                    done.fail('Batch request failed');
                });
        });

        it('should invoke the success cb if attached to the request promise after it resolved', done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const requests = fromJson<JsonRpcRequest[]>(data);
                    const response = toBatchJsonRpcResponseJson(requests);
                    socket.send(response);
                });
            });

            const request = new Request('ping');
            const task = rpc.batch(request);

            task.then(() => {
                task.then(async response => {
                    const err = 'Response is not an array of JSON RPC response objects';
                    const [item] = response;
                    if (item instanceof Response) {
                        expect(item.id).toBe(request.id);
                        try {
                            const result = await item.json<boolean>();
                            expect(result).toBe(true);
                            done();
                        } catch (e) {
                            done.fail(e);
                        }
                    } else {
                        done.fail(err);
                    }
                });
            });
        });

        it('should work with async/await', async done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const requests = fromJson<JsonRpcRequest[]>(data);
                    const response = toBatchJsonRpcResponseJson(requests);
                    socket.send(response);
                });
            });

            try {
                const request = new Request('ping');
                const response = await rpc.batch(request);
                const err = 'Response is not an array of JSON RPC response objects';

                const [item] = response;

                if (item instanceof Response) {
                    expect(item.id).toBe(request.id);
                    try {
                        const result = await item.json<boolean>();
                        expect(result).toBe(true);
                        done();
                    } catch (e) {
                        done.fail(e);
                    }
                } else {
                    done.fail(err);
                }
            } catch (err) {
                done.fail(err);
            }
        });

        it('should ignore responses for other requests', async done => {
            const method = 'ping';

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    socket.send(toBatchJsonRpcResponseJson([
                        new Request(method)
                    ]));

                    const requests = fromJson<JsonRpcRequest[]>(data);
                    const response = toBatchJsonRpcResponseJson(requests);
                    socket.send(response);
                });
            });

            try {
                const request = new Request(method);
                const response = await rpc.batch(request);
                const err = 'Response is not an array of JSON RPC response objects';

                const [item] = response;

                if (item instanceof Response) {
                    expect(item.id).toBe(request.id);
                    try {
                        const result = await item.json<boolean>();
                        expect(result).toBe(true);
                        done();
                    } catch (e) {
                        done.fail(e);
                    }
                } else {
                    done.fail(err);
                }
            } catch (err) {
                done.fail(err);
            }
        });

        it('should reject when sending an empty batch', async done => {
            try {
                await rpc.batch();
                done.fail('Batch request should have failed');
            } catch (err) {
                expect(err).toBeInstanceOf(JsonRpcError);
                expect(err.code).toBe(INVALID_REQUEST);
                expect(err.message).toBe('Invalid request');
                done();
            }
        });

        it('should not reject when the socket emits an error response for one of the requests', async done => {
            const error = {
                code: INVALID_REQUEST,
                message: 'Invalid request'
            };

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const json = fromJson<any[]>(data);

                    const [, request] = json;
                    const response = toJson([
                        createJsonRpcResponseWithError(error, null),
                        toJsonRpcResponse(request)
                    ]);

                    socket.send(response);
                });
            });

            try {
                const request = new Request('ping');
                const response: Response[] = await rpc.batch(...[
                    1 as any,
                    request
                ]) as any;

                expect(response.every(item => item instanceof Response)).toBe(true);

                const [err, item] = response;

                expect(err.id).toBeNull();
                try {
                    await err.json();
                } catch (e) {
                    expect(e).toBeInstanceOf(JsonRpcError);
                    expect(e.code).toBe(error.code);
                    expect(e.message).toBe(error.message);
                }

                expect(item.id).toBe(request.id);
                try {
                    const result = await item.json<boolean>();
                    expect(result).toBe(true);
                    done();
                } catch (e) {
                    done.fail(e);
                }
            } catch (err) {
                done.fail(err);
            }
        });

        it('should reject when the server closes the socket', async done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', () => {
                    socket.close();
                });
            });

            try {
                const request = new Request('ping');
                await rpc.batch(request);
                done.fail('Batch request should have failed');
            } catch (err) {
                expect(err.code).toBe(SOCKET_CLOSED);
                done();
            }
        });

        it('should reject when the client closes the socket via unsubscribe()', async done => {
            mockServer.on('connection', () => {
                rpc.ws.unsubscribe();
            });

            try {
                const request = new Request('ping');
                await rpc.batch(request);
                done.fail('Batch request should have failed');
            } catch (err) {
                expect(err.code).toBe(SOCKET_CLOSED);
                done();
            }
        });

        it('should reject when the client closes the socket via complete()', async done => {
            mockServer.on('connection', () => {
                rpc.ws.complete();
            });

            try {
                const request = new Request('ping');
                await rpc.batch(request);
                done.fail('Batch request should have failed');
            } catch (err) {
                expect(err.code).toBe(SOCKET_CLOSED);
                done();
            }
        });

        it('should reject when the ws subject errors', async done => {
            mockServer.on('connection', () => {
                rpc.ws.error({});
            });

            try {
                const request = new Request('ping');
                await rpc.batch(request);
                done.fail('Batch request should have failed');
            } catch (err) {
                expect(err.code).toBe(SOCKET_PIPE_BROKEN);
                done();
            }
        });

        it('should reject when the connection with the ws is closed', async done => {
            mockServer.close({
                code: 1005,
                reason: 'Why not?',
                wasClean: false
            });

            try {
                const request = new Request('ping');
                await rpc.batch(request);
                done.fail('Batch request should have failed');
            } catch (err) {
                expect(err.code).toBe(SOCKET_PIPE_BROKEN);
                done();
            }
        });

        it('should invoke the failure cb if attached to the request promise after it rejected', done => {
            const task = rpc.batch();

            task.then(noop, () => {
                task.then(noop, err => {
                    expect(err).toBeInstanceOf(JsonRpcError);
                    expect(err.code).toBe(INVALID_REQUEST);
                    expect(err.message).toBe('Invalid request');
                    done();
                });
            });
        });

        it('should invoke all attached success cb fns on resolve', done => {
            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const requests = fromJson<JsonRpcRequest[]>(data);
                    const response = toBatchJsonRpcResponseJson(requests);
                    socket.send(response);
                });
            });

            const onSuccess = jest.fn();
            const request = new Request('ping');
            const task = rpc.batch(request);

            Promise.all([
                task.then(onSuccess),
                task.then(onSuccess)
            ]).then(async () => {
                expect(onSuccess).toHaveBeenCalledTimes(2);

                const callsArgs = onSuccess.mock.calls.map(call => call[0]);

                for (const args of callsArgs) {
                    const [response] = args;
                    expect(response).toBeInstanceOf(Response);
                    const result = await response.json();
                    expect(result).toBe(true);
                }

                done();
            });
        });

        it('should invoke all attached failure cb fns on reject', done => {
            const error = {
                code: INVALID_REQUEST,
                message: 'Invalid request'
            };

            const onFailure = jest.fn();
            const task = rpc.batch();

            Promise.all([
                task.then(noop, onFailure),
                task.then(noop, onFailure)
            ]).then(async () => {
                expect(onFailure).toHaveBeenCalledTimes(2);

                const callsArgs = onFailure.mock.calls.map(call => call[0]);

                for (const err of callsArgs) {
                    expect(err).toBeInstanceOf(JsonRpcError);
                    expect(err.code).toBe(error.code);
                    expect(err.message).toBe(error.message);
                }

                done();
            });
        });

        it('can chain promises', async done => {
            const data = {ping: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (d: any) => {
                    const requests = fromJson<JsonRpcRequest[]>(d);
                    const response = toBatchJsonRpcResponseJson(requests, data);
                    socket.send(response);
                });
            });

            const request = new Request('ping');

            rpc.batch(request)
                .then(async response => {
                    const [item] = response;
                    if (item instanceof Response) {
                        const result = await item.json<typeof data>();
                        return result.ping;
                    }
                    return;
                })
                .then(res => {
                    expect(res).toBe(true);
                    done();
                });
        });

        describe('.cancel()', () => {
            it('should send a batch with cancel notifications for every request', done => {
                mockServer.on('connection', socket => {
                    const error = {
                        code: REQUEST_ABORTED,
                        message: 'Request aborted'
                    };

                    const requests: JsonRpcRequest[] = [];

                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const json = fromJson<any[]>(data);

                        if (json.every(isJsonRpcNotification) && json.every(item => item.method === CANCEL)) {
                            const ids = requests.map(r => r.id);

                            for (const notification of json) {
                                const {id} = notification.params;
                                expect(isString(id)).toBe(true);
                                expect(ids.includes(id));
                            }

                            const response = toBatchJsonRpcErrorObjectJson(requests, error);
                            socket.send(response);
                        } else {
                            requests.push(...json.filter(isJsonRpcRequest));
                        }
                    });
                });

                const task = rpc.batch(
                    new Notification('ping'),
                    new Request('ping'),
                    new Request('ping')
                );

                task.then(() => {
                    done();
                });

                task.cancel();
            });

            it('should do nothing if already resolved', done => {
                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const json = fromJson<any[]>(data);

                        if (json.every(isJsonRpcRequest)) {
                            const response = toBatchJsonRpcResponseJson(json);
                            socket.send(response);
                        } else {
                            done.fail('Cancel should not have send a notification');
                        }
                    });
                });

                const request = new Request('ping');
                const task = rpc.batch(request);

                task.then(() => task.cancel())
                    .then(() => {
                        done();
                    });
            });

            it('should not do anything if it\'s already pending cancel', done => {
                const cancelSpy = jest.fn();

                mockServer.on('connection', socket => {
                    const error = {
                        code: REQUEST_ABORTED,
                        message: 'Request aborted'
                    };

                    const requests: JsonRpcRequest[] = [];

                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const json = fromJson<any[]>(data);
                        const requestItems = json.filter(isJsonRpcRequest);

                        if (json.every(isJsonRpcNotification) && json.every(item => item.method === CANCEL)) {
                            const ids = requests.map(r => r.id);

                            for (const notification of json) {
                                const {id} = notification.params;
                                expect(isString(id)).toBe(true);
                                expect(ids.includes(id));
                            }

                            setTimeout(() => {
                                const response = toBatchJsonRpcErrorObjectJson(requests, error);
                                socket.send(response);
                            }, 50);

                            cancelSpy();
                        }

                        if (requestItems.length > 0) {
                            requests.push(...requestItems);
                        }
                    });
                });

                const task = rpc.batch(
                    new Notification('ping'),
                    new Request('ping'),
                    new Request('ping')
                );

                task.then(() => {
                    expect(cancelSpy).toHaveBeenCalledTimes(1);
                    done();
                });

                task.cancel();
                task.cancel();
            });

            it('should do nothing for notifications', done => {
                const notifySpy = jest.fn();

                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', () => {
                        notifySpy();
                    });
                });

                const notification = new Notification('ping');
                const task = rpc.batch(notification);

                task.cancel();

                setTimeout(() => {
                    expect(notifySpy).toHaveBeenCalledTimes(1);
                    done();
                }, 50);
            });

            it('should do nothing for an empty batch', done => {
                const notifySpy = jest.fn();

                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', () => {
                        notifySpy();
                    });
                });

                const task = rpc.batch();

                task.cancel();

                setTimeout(() => {
                    expect(notifySpy).toHaveBeenCalledTimes(1);
                    done();
                }, 50);
            });
        });

        describe('.on()', () => {
            it('should return an Observable', () => {
                const request = new Request('ping');
                const task = rpc.batch(request);
                expect(task.on('progress')).toBeInstanceOf(Observable);
            });

            it('can emit Progress when socket sends progress notifications for the requests', done => {
                const progressSpy = jest.fn();

                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const [request] = fromJson<JsonRpcRequest[]>(data);

                        timer(0, 10)
                            .pipe(take(11))
                            .subscribe(num => {
                                const notification = new Notification(PROGRESS, {
                                    id: request.id,
                                    operation: 'Doing ...',
                                    amount: num / 10
                                });
                                const message = toJson(notification);
                                socket.send(message);
                            }, noop, () => requestAnimationFrame(() => {
                                expect(progressSpy).toHaveBeenCalledTimes(11);

                                const args = progressSpy.mock.calls.map(c => c[0]);
                                for (const [index, arg] of args.entries()) {
                                    expect(arg.amount).toBe(index / 10);
                                    expect(arg.operation).toBeUndefined();
                                }

                                done();
                            }));
                    });
                });

                const request = new Request('ping');
                const task = rpc.batch(request);

                task.on('progress')
                    .subscribe(progressSpy);
            });

            it('aggregates the progress from all requests', done => {
                const progressSpy = jest.fn();

                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const [r1, r2] = fromJson<JsonRpcRequest[]>(data);

                        merge(
                            timer(0, 10)
                                .pipe(take(11), map(num => ({
                                    id: r1.id,
                                    progress: num / 10
                                }))),
                            timer(0, 10)
                                .pipe(take(11), map(num => ({
                                    id: r2.id,
                                    progress: num / 10
                                }))))
                            .subscribe(item => {
                                const notification = new Notification(PROGRESS, {
                                    id: item.id,
                                    operation: 'Doing ...',
                                    amount: item.progress
                                });
                                const message = toJson(notification);
                                socket.send(message);
                            }, noop, () => requestAnimationFrame(() => {
                                expect(progressSpy).toHaveBeenCalledTimes(21); // For 0 we only take one emit

                                const args = progressSpy.mock.calls.map(c => c[0]);
                                for (const [index, arg] of args.entries()) {
                                    const progress = (index / 2) / 10;
                                    expect(arg.amount).toBeCloseTo(progress);
                                    expect(arg.operation).toBeUndefined();
                                }

                                done();
                            }));
                    });
                });

                const task = rpc.batch(...[
                    new Request('ping'),
                    new Request('pong')
                ]);

                task.on('progress')
                    .subscribe(progressSpy);
            });

            it('should throw when the request throws', done => {
                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', () => {
                        socket.close();
                    });
                });

                const request = new Request('ping');
                const task = rpc.batch(request);

                task.on('progress')
                    .subscribe({
                        error: err => {
                            expect(err.code).toBe(SOCKET_CLOSED);
                            done();
                        }
                    });
            });

            it('should complete when the request is done', done => {
                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const requests = fromJson<JsonRpcRequest[]>(data);
                        const response = toBatchJsonRpcResponseJson(requests);
                        socket.send(response);
                    });
                });

                const request = new Request('ping');
                const task = rpc.batch(request);

                task.on('progress')
                    .subscribe({
                        complete: () => {
                            done();
                        }
                    });
            });

            it('replays last known value', done => {
                mockServer.on('connection', socket => {
                    // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                    // remove when fixed
                    (socket as any).on('message', (data: any) => {
                        const [request] = fromJson<JsonRpcRequest[]>(data);
                        const notification = new Notification(PROGRESS, {
                            id: request.id,
                            operation: 'Doing ...',
                            amount: 0.4
                        });
                        const message = toJson(notification);
                        socket.send(message);
                    });
                });

                const request = new Request('ping');
                const task = rpc.batch(request);
                const obs = task.on('progress');

                setTimeout(() => {
                    obs.subscribe(progress => {
                        expect(progress.amount).toBe(0.4);
                        done();
                    });
                }, 50);
            });

            it('should return an empty Observable for unknown events', () => {
                const request = new Request('ping');
                const task = rpc.batch(request);
                expect(task.on('test' as any)).toBeInstanceOf(Observable);
            });
        });
    });

    describe('.subscribe()', () => {
        const host = 'myhost';
        let mockServer: Server;
        let rpc: Client;
        beforeEach(() => {
            mockServer = createMockServer(host);
            rpc = Client.create({url: host});
        });
        afterEach(done => {
            mockServer.stop(done);
        });

        it('should receive notifications from the server', done => {
            const data = {ping: true};
            const notification = new Notification('test', data);

            mockServer.on('connection', socket => {
                const message = toJson(notification);
                socket.send(message);
            });

            rpc.subscribe(n => {
                expect(n).toEqual(notification);
                done();
            });
        });

        it('should filter anything that\'s not a json string', done => {
            const data = new Blob();
            const malformedJson = '{';
            const notification = new Notification('test', {});

            mockServer.on('connection', socket => {
                socket.send(data);
                socket.send(malformedJson);
                socket.send(toJson(notification));
            });

            rpc.subscribe(n => {
                expect(n).toEqual(notification);
                done();
            });
        });

        it('should filter anything that\'s not a notification', done => {
            const response = createJsonRpcResponse({ping: true});
            const error = createJsonRpcResponseWithError({code: 1, message: 'Oops'});
            const notification = new Notification('test', {});

            mockServer.on('connection', socket => {
                socket.send(toJson(response));
                socket.send(toJson(error));
                socket.send(toJson(notification));
            });

            rpc.subscribe(n => {
                expect(n).toEqual(notification);
                done();
            });
        });

        it('should complete when the server closes the socket', done => {
            mockServer.on('connection', socket => {
                // {wasClean: true}
                socket.close();
            });

            rpc.subscribe({
                complete() {
                    done();
                }
            });
        });

        it('should error when the server closes the socket with {wasClean: false}', done => {
            mockServer.on('connection', socket => {
                mockServer.close({
                    code: 1005,
                    reason: 'Why not?',
                    wasClean: false
                });
            });

            rpc.subscribe({
                error(err) {
                    expect(err.code).toBe(SOCKET_CLOSED);
                    done();
                }
            });
        });

        it('should error when the client closes the socket via unsubscribe()', done => {
            mockServer.on('connection', socket => {
                rpc.ws.unsubscribe();
            });

            rpc.subscribe({
                error(err) {
                    expect(err.code).toBe(SOCKET_CLOSED);
                    done();
                }
            });
        });

        it('should error when the client closes the socket via complete()', done => {
            mockServer.on('connection', socket => {
                rpc.ws.complete();
            });

            rpc.subscribe({
                error(err) {
                    expect(err.code).toBe(SOCKET_CLOSED);
                    done();
                }
            });
        });

        it('should error when the client errors', done => {
            mockServer.on('connection', socket => {
                rpc.ws.error({});
            });

            rpc.subscribe({
                error(err) {
                    expect(err.code).toBe(SOCKET_PIPE_BROKEN);
                    done();
                }
            });
        });
    });

    describe('.asObservable()', () => {
        const host = 'myhost';
        let mockServer: Server;
        let rpc: Client;
        beforeEach(() => {
            mockServer = createMockServer(host);
            rpc = Client.create({url: host});
        });
        afterEach(done => {
            mockServer.stop(done);
        });

        it('should return the source as an Observable', done => {
            const params = {ping: true};
            const notification = new Notification('test', params);

            mockServer.on('connection', socket => {
                const message = toJson(notification);
                socket.send(message);
            });

            rpc.asObservable()
                .pipe(map(notification => notification.params))
                .subscribe(p => {
                    expect(p).toEqual(params);
                    done();
                });
        });
    });

    describe('.disconnect()', () => {
        const host = 'myhost';
        let mockServer: Server;
        let rpc: Client;
        beforeEach(() => {
            mockServer = createMockServer(host);
            rpc = Client.create({url: host});
        });
        afterEach(done => {
            mockServer.stop(done);
        });

        it('should close the socket connection cleanly', done => {
            mockServer.on('connection', socket => {
                rpc.disconnect();
            });

            rpc.subscribe({
                complete() {
                    done();
                }
            });
        });
    });

    describe('Serialization/Deserialization', () => {
        const host = 'myhost';
        let mockServer: Server;
        beforeEach(() => {
            mockServer = createMockServer(host);
        });
        afterEach(done => {
            mockServer.stop(done);
        });

        it('should use custom serializer to send notifications', done => {
            const rpc = Client.create({
                url: host,
                serializer: data => {
                    const json = JSON.stringify(data);
                    const encrypted = btoa(json);
                    return encrypted;
                }
            });

            const method = 'test';
            const params = {ping: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const str = atob(data);
                    const json = fromJson<JsonRpcNotification>(str);
                    expect(json.method).toBe(method);
                    expect(json.params).toEqual(params);
                    done();
                });
            });

            rpc.notify(method, params);
        });

        it('should use custom serializer to send requests', done => {
            const rpc = Client.create({
                url: host,
                serializer: data => {
                    const json = JSON.stringify(data);
                    const encrypted = btoa(json);
                    return encrypted;
                }
            });

            const method = 'test';
            const params = {ping: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const str = atob(data);
                    const json = fromJson<JsonRpcRequest>(str);

                    expect(json.id).toBeDefined();
                    expect(json.method).toBe(method);
                    expect(json.jsonrpc).toBe(JSON_RPC_VERSION);
                    expect(json.params).toEqual(params);

                    done();
                });
            });

            rpc.request(method, params);
        });

        it('should use custom serializer to send batch requests', done => {
            const rpc = Client.create({
                url: host,
                serializer: data => {
                    const json = JSON.stringify(data);
                    const encrypted = btoa(json);
                    return encrypted;
                }
            });

            const method = 'ping';
            const params = {ping: true};
            const request = new Request(method, params);
            const notification = new Notification(method, params);

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const str = atob(data);
                    const [req, n] = fromJson<JsonRpcRequest[]>(str);

                    expect(req.id).toBe(request.id);
                    expect(req.method).toBe(method);
                    expect(req.jsonrpc).toBe(JSON_RPC_VERSION);
                    expect(req.params).toEqual(params);

                    expect(n.method).toBe(method);
                    expect(n.jsonrpc).toBe(JSON_RPC_VERSION);
                    expect(n.params).toEqual(params);

                    done();
                });
            });

            rpc.batch(...[
                request,
                notification
            ]);
        });

        it('should use custom deserializer to unpack received notifications', done => {
            const rpc = Client.create({
                url: host,
                deserializer: async evt => {
                    const str = atob(evt.data);
                    const json = fromJson<JsonRpcNotification>(str);
                    return json;
                }
            });

            const data = {ping: true};
            const notification = new Notification('test', data);

            mockServer.on('connection', socket => {
                const message = toJson(notification);
                const encrypted = btoa(message);
                socket.send(encrypted);
            });

            rpc.subscribe(n => {
                expect(n).toEqual(notification);
                done();
            });
        });

        it('should use custom deserializer to unpack request responses', done => {
            const rpc = Client.create({
                url: host,
                deserializer: async evt => {
                    const str = atob(evt.data);
                    const json = fromJson<JsonRpcRequest>(str);
                    return json;
                }
            });

            const result = {pong: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const message = toJsonRpcResponseJson(request, result);
                    const encrypted = btoa(message);
                    socket.send(encrypted);
                });
            });

            rpc.request('ping')
                .then(res => {
                    expect(res).toEqual(result);
                    done();
                });
        });

        it('should use custom deserializer to unpack batch request responses', done => {
            const rpc = Client.create({
                url: host,
                deserializer: async evt => {
                    const str = atob(evt.data);
                    const json = fromJson<JsonRpcRequest>(str);
                    return json;
                }
            });

            const result = {pong: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const requests = fromJson<JsonRpcRequest[]>(data);
                    const message = toBatchJsonRpcResponseJson(requests, result);
                    const encrypted = btoa(message);
                    socket.send(encrypted);
                });
            });

            const request = new Request('ping');
            rpc.batch(request)
                .then(([response]) => response.json())
                .then(r => {
                    expect(r).toEqual(result);
                    done();
                });
        });

        it('should throw socket pipe error if the deserializer throws', done => {
            const rpc = Client.create({
                url: host,
                deserializer: async () => {
                    throw new Error('oops');
                }
            });

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const request = fromJson<JsonRpcRequest>(data);
                    const response = toJsonRpcResponseJson(request, {pong: true});
                    const encrypted = btoa(response);
                    socket.send(encrypted);
                });
            });

            rpc.request('ping')
                .then(() => {
                    done.fail('Should have failed');
                }, err => {
                    expect(err.code).toBe(SOCKET_PIPE_BROKEN);
                    done();
                });
        });

        it('should throw socket pipe error if the deserializer throws', done => {
            const rpc = Client.create({
                url: host,
                deserializer: async () => {
                    throw new Error('oops');
                }
            });

            const result = {pong: true};

            mockServer.on('connection', socket => {
                // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
                // remove when fixed
                (socket as any).on('message', (data: any) => {
                    const requests = fromJson<JsonRpcRequest[]>(data);
                    const response = toBatchJsonRpcResponseJson(requests, result);
                    const encrypted = btoa(response);
                    socket.send(encrypted);
                });
            });

            const request = new Request('ping');
            rpc.batch(request)
                .then(() => {
                    done.fail('Should have failed');
                }, err => {
                    expect(err.code).toBe(SOCKET_PIPE_BROKEN);
                    done();
                });
        });
    });
});

describe('createWs()', () => {
    const host = 'ws://myhost';
    let mockServer: Server;
    beforeEach(() => {
        mockServer = createMockServer(host);
    });
    afterEach(done => {
        mockServer.stop(done);
    });

    it('should create a ws subject with the given host', done => {
        mockServer.on('connection', socket => {
            // NOTE: mock-socket adds an extra / at the end of the host
            expect(socket.url.startsWith(host)).toBe(true);
            done();
        });

        const socket = createWs({url: host});
        socket.subscribe();
    });

    it('should return the value as is when passing to the serializer/deserializer', done => {
        const outbound = {pong: true};
        const inbound = {ping: true};

        mockServer.on('connection', socket => {
            // TODO: (socket as any) is due to https://github.com/thoov/mock-socket/issues/224,
            // remove when fixed
            (socket as any).on('message', (data: any) => {
                expect(data).toEqual(JSON.stringify(outbound));
                done();
            });

            const message = JSON.stringify(inbound);
            socket.send(message);
        });

        const socket = createWs({url: host});
        socket.subscribe(evt => {
            expect(isObject(evt)).toBe(true);
            expect(evt.data).toEqual(JSON.stringify(inbound));
            // Send a message to the server
            const message = JSON.stringify(outbound);
            socket.next(message);
        });
    });

    it('should call onConnected() cb when connected', done => {
        const socket = createWs({
            url: host,
            onConnected() {
                done();
            }
        });

        socket.subscribe();
    });

    it('should call onClosed() cb when disconnected', done => {
        mockServer.on('connection', socket => {
            socket.close(0, 'Why not?');
        });

        const socket = createWs({
            url: host,
            onClosed(evt) {
                expect(evt.wasClean).toBe(true);
                done();
            }
        });

        socket.subscribe();
    });

    it('should call onClosed() cb when a connection cannot be created', done => {
        mockServer.close({
            code: 1005,
            reason: 'Why not?',
            wasClean: false
        });

        const socket = createWs({
            url: host,
            onClosed(evt) {
                done();
            }
        });

        socket.subscribe();
    });
});

describe('setWsProtocol()', () => {
    it('should return the url as is if it has ws:// or wss:// protocol set', () => {
        expect(setWsProtocol('ws://test.io')).toEqual('ws://test.io');
    });

    it('should set the protocol to ws:// if it has http://', () => {
        expect(setWsProtocol('http://test.io')).toEqual('ws://test.io');
    });

    it('should set the protocol to wss:// if it has https://', () => {
        expect(setWsProtocol('https://test.io')).toEqual('wss://test.io');
    });

    it('should set the protocol to ws:// if it starts with ://', () => {
        expect(setWsProtocol('://test.io')).toEqual('ws://test.io');
    });

    it('should set the protocol to ws:// if it has no protocol', () => {
        expect(setWsProtocol('test.io')).toEqual('ws://test.io');
    });
});

describe('createBatchResponseFilter()', () => {
    it('should return a function', () => {
        const request = new Request('ping');
        const filter = createBatchResponseFilter([request]);
        expect(typeof filter).toBe('function');
    });

    it('should return true for bath response that contain the same requests', () => {
        const request = new Request('ping');
        const filter = createBatchResponseFilter([request]);

        const response = toJsonRpcResponse(request);
        expect(filter([response])).toBe(true);
    });

    it('should return false otherwise', () => {
        const request = new Request('ping');
        const filter = createBatchResponseFilter([request]);

        expect(filter([new Request('ping')])).toBe(false);
        expect(filter([toJsonRpcResponse(new Request('ping'))])).toBe(false);
        expect(filter([new Notification('ping')])).toBe(false);
        expect(filter([])).toBe(false);
        expect(filter(undefined)).toBe(false);
        expect(filter({})).toBe(false);
        expect(filter([])).toBe(false);
        expect(filter(null)).toBe(false);
        expect(filter('')).toBe(false);
        expect(filter(1)).toBe(false);
        expect(filter(false)).toBe(false);
    });
});

describe('createProgressAggregate()', () => {
    it('should return a function', () => {
        const request = new Request('ping');
        const progress = createProgressAggregate([request]);
        expect(typeof progress).toBe('function');
    });

    it('aggregates progress from progress notifications', () => {
        const r1 = new Request('ping');
        const p1 = toProgressNotification(r1);
        const r2 = new Request('pong');
        const p2 = toProgressNotification(r2);
        const aggregate = createProgressAggregate([r1, r2]);

        expect(aggregate(p1(0))).toBe(0);
        expect(aggregate(p1(0.1))).toBeCloseTo(0.05);
        expect(aggregate(p2(0.1))).toBeCloseTo(0.1);
        expect(aggregate(p1(0.4))).toBeCloseTo(0.25);
        expect(aggregate(p1(1))).toBeCloseTo(0.55);
        expect(aggregate(p2(1))).toBeCloseTo(1);
    });
});

describe('createProgressFilter()', () => {
    it('should return a function', () => {
        const filter = createProgressFilter('test');
        expect(typeof filter).toBe('function');
    });

    it('should be true for progress notifications with the same id as the given one', () => {
        const id = 'test';
        const notification = new Notification(PROGRESS, {id});
        const filter = createProgressFilter(id);
        expect(filter(notification)).toBe(true);
    });

    it('should be false otherwise', () => {
        const filter = createProgressFilter('test');
        expect(filter(new Notification(PROGRESS, {
            id: 'test2'
        }))).toBe(false);
        expect(filter(new Notification(PROGRESS, {}))).toBe(false);
        expect(filter(new Notification(PROGRESS))).toBe(false);
        expect(filter({})).toBe(false);
        expect(filter([])).toBe(false);
        expect(filter(null)).toBe(false);
        expect(filter('')).toBe(false);
        expect(filter(1)).toBe(false);
        expect(filter(false)).toBe(false);
        expect(filter({
            jsonrpc: JSON_RPC_VERSION
        })).toBe(false);
        expect(filter(new Notification('test', {
            id: 'test'
        }))).toBe(false);
    });
});

describe('createResponseFilter()', () => {
    it('should return a function', () => {
        const filter = createResponseFilter('test');
        expect(typeof filter).toBe('function');
    });

    it('should be true for responses with the same id', () => {
        const response = createJsonRpcResponse({method: 'test'});
        const filter = createResponseFilter(response.id);
        expect(filter(response)).toBe(true);
    });

    it('should be true for response errors with the same id', () => {
        const response = createJsonRpcResponseWithError({code: -1, message: 'Oops'});
        const filter = createResponseFilter(response.id);
        expect(filter(response)).toBe(true);
    });

    it('should be false otherwise', () => {
        const response = createJsonRpcResponseWithError({code: -1, message: 'Oops'});
        const filter = createResponseFilter('test');
        expect(filter(response)).toBe(false);
        expect(filter({})).toBe(false);
        expect(filter([])).toBe(false);
        expect(filter(null)).toBe(false);
        expect(filter('')).toBe(false);
        expect(filter(1)).toBe(false);
        expect(filter(false)).toBe(false);
        expect(filter({
            jsonrpc: JSON_RPC_VERSION
        })).toBe(false);
        expect(filter({
            jsonrpc: JSON_RPC_VERSION,
            id: response.id
        })).toBe(false);
        expect(filter({
            jsonrpc: '123',
            id: 'test'
        })).toBe(false);
    });
});

describe('fromJsonAsync()', () => {
    it('should parse json async', async () => {
        const data = {ping: true};
        const evt = new MessageEvent('message', {
            data: toJson(data)
        });
        const res = await fromJsonAsync(evt);
        expect(res).toEqual(data);
    });

    it('should return undefined on error', async () => {
        const evt = new MessageEvent('message', {
            data: '{'
        });
        const res = await fromJsonAsync(evt);
        expect(res).toBeUndefined();
    });
});

describe('isJsonRpcMessage()', () => {
    it('should return true for JSON RPC objects', () => {
        const notification = new Notification('ping');
        expect(isJsonRpcMessage(notification)).toBe(true);
        const response = createJsonRpcResponse({method: 'ping'});
        expect(isJsonRpcMessage(response)).toBe(true);
        const err = createJsonRpcResponseWithError({code: INVALID_REQUEST, message: 'Oops'});
        expect(isJsonRpcMessage(err)).toBe(true);
    });

    it('should return true for an array of JSON RPC objects', () => {
        const notification = new Notification('ping');
        expect(isJsonRpcMessage([notification])).toBe(true);
        const response = createJsonRpcResponse({method: 'ping'});
        expect(isJsonRpcMessage([response])).toBe(true);
        const err = createJsonRpcResponseWithError({code: INVALID_REQUEST, message: 'Oops'});
        expect(isJsonRpcMessage([err])).toBe(true);
    });

    it('should return false for JSON RPC requests', () => {
        const request = new Request('ping');
        expect(isJsonRpcMessage(request)).toBe(false);
        expect(isJsonRpcMessage([request])).toBe(false);
    });

    it('should return false for any other objects', () => {
        expect(isJsonRpcMessage({})).toBe(false);
        expect(isJsonRpcMessage([])).toBe(false);
        expect(isJsonRpcMessage(null)).toBe(false);
        expect(isJsonRpcMessage('')).toBe(false);
        expect(isJsonRpcMessage(1)).toBe(false);
        expect(isJsonRpcMessage(false)).toBe(false);
    });
});

describe('isJsonRpcObject()', () => {
    it('should return true for JSON RPC objects', () => {
        const notification = new Notification('ping');
        expect(isJsonRpcObject(notification)).toBe(true);
        const response = createJsonRpcResponse({method: 'ping'});
        expect(isJsonRpcObject(response)).toBe(true);
        const err = createJsonRpcResponseWithError({code: INVALID_REQUEST, message: 'Oops'});
        expect(isJsonRpcObject(err)).toBe(true);
    });

    it('should return false for JSON RPC requests', () => {
        const request = new Request('ping');
        expect(isJsonRpcObject(request)).toBe(false);
        expect(isJsonRpcObject([request])).toBe(false);
    });

    it('should return false for any other objects', () => {
        expect(isJsonRpcObject({})).toBe(false);
        expect(isJsonRpcObject([])).toBe(false);
        expect(isJsonRpcObject(null)).toBe(false);
        expect(isJsonRpcObject('')).toBe(false);
        expect(isJsonRpcObject(1)).toBe(false);
        expect(isJsonRpcObject(false)).toBe(false);
    });
});

describe('isJsonRpcResponse()', () => {
    it('should return true if a value is a JSON RPC response', () => {
        const result = {ping: true};
        const value = createJsonRpcResponse(result);
        expect(isJsonRpcResponse(value)).toBe(true);
        expect(isJsonRpcResponse({
            result,
            jsonrpc: JSON_RPC_VERSION,
            id: 1
        })).toBe(true);
        expect(isJsonRpcResponse({
            jsonrpc: JSON_RPC_VERSION,
            id: 1,
            result: ''
        })).toBe(true);
        expect(isJsonRpcResponse({
            jsonrpc: JSON_RPC_VERSION,
            id: 1,
            result: true
        })).toBe(true);
        expect(isJsonRpcResponse({
            jsonrpc: JSON_RPC_VERSION,
            id: 1,
            result: []
        })).toBe(true);
        expect(isJsonRpcResponse({
            jsonrpc: JSON_RPC_VERSION,
            id: 1,
            result: null
        })).toBe(true);
        expect(isJsonRpcResponse({
            jsonrpc: JSON_RPC_VERSION,
            id: 1,
            result: 1
        })).toBe(true);
    });

    it('should return true if a value is a JSON RPC response with error', () => {
        const error = {code: -1, message: 'Oops'};
        const value = createJsonRpcResponseWithError(error);
        expect(isJsonRpcResponse(value)).toBe(true);
        expect(isJsonRpcResponse({
            error,
            jsonrpc: JSON_RPC_VERSION,
            id: 1
        })).toBe(true);
        expect(isJsonRpcResponse({
            error,
            jsonrpc: JSON_RPC_VERSION,
            id: null
        })).toBe(true);
    });

    it('should return false otherwise', () => {
        const result = {ping: true};
        expect(isJsonRpcResponse({
            jsonrpc: JSON_RPC_VERSION
        })).toBe(false);
        expect(isJsonRpcResponse({result})).toBe(false);
        expect(isJsonRpcResponse({id: 1})).toBe(false);
        expect(isJsonRpcResponse(null)).toBe(false);
        expect(isJsonRpcResponse({
            jsonrpc: JSON_RPC_VERSION,
            id: 1
        })).toBe(false);
        expect(isJsonRpcResponse({
            result,
            jsonrpc: '1.0',
            id: 1
        })).toBe(false);
        expect(isJsonRpcResponse({
            jsonrpc: '2.0',
            id: 1,
            result: undefined
        })).toBe(false);
    });
});


function createMockServer(host: string) {
    const wsHost = setWsProtocol(host);
    return new Server(wsHost);
}

function createJsonRpcResponseWithError(
    error: JsonRpcErrorObject,
    id: string | number | null = uid(UID_BYTE_LENGTH)
): JsonRpcResponse {
    return {
        id,
        error,
        jsonrpc: JSON_RPC_VERSION
    };
}

function toJsonRpcResponseJson(request: Request | JsonRpcRequest, result: any = true) {
    const response = toJsonRpcResponse(request, result);
    return toJson(response);
}

function toJsonRpcResponse(request: Request | JsonRpcRequest, result: any = true): JsonRpcResponse {
    return {
        result,
        jsonrpc: JSON_RPC_VERSION,
        id: request.id
    };
}

function toJsonRpcErrorObjectJson(request: Request | JsonRpcRequest, error: JsonRpcErrorObject) {
    return toJson({
        error,
        jsonrpc: JSON_RPC_VERSION,
        id: request.id
    });
}

function toBatchJsonRpcResponseJson(requests: Array<Request | JsonRpcRequest>, result: any = true) {
    return toJson(requests.map(request => ({
        result,
        jsonrpc: JSON_RPC_VERSION,
        id: request.id
    })));
}

function toBatchJsonRpcErrorObjectJson(requests: Array<Request | JsonRpcRequest>, error: JsonRpcErrorObject) {
    return toJson(requests.map(request => ({
        error,
        jsonrpc: JSON_RPC_VERSION,
        id: request.id
    })));
}

function toProgressNotification(request: Request): (amount: number) => ProgressNotification {
    return (amount: number) => ({
        method: PROGRESS,
        jsonrpc: JSON_RPC_VERSION,
        params: {
            amount,
            id: request.id,
            operation: ''
        }
    });
}

function toJson<T>(data: T) {
    const json = JSON.stringify(data);
    return json;
}

function fromJson<T>(json: string): T {
    const data = JSON.parse(json);
    return data;
}
