import {
    isFunction,
    isNumber,
    isObject,
    isString
} from 'lodash';
import {
    EMPTY,
    merge,
    noop,
    Observable,
    PartialObserver,
    ReplaySubject,
    Subscribable,
    Subscription
} from 'rxjs';
import {
    catchError,
    distinct,
    filter,
    map,
    mergeMap,
    take,
    tap
} from 'rxjs/operators';
import {
    webSocket,
    WebSocketSubject,
    WebSocketSubjectConfig
} from 'rxjs/webSocket';
import {
    CANCEL,
    HTTP,
    HTTPS,
    INVALID_REQUEST,
    JSON_RPC_VERSION,
    PROGRESS,
    PROGRESS_EVENT,
    PROGRESS_EVENT_TYPE,
    SOCKET_CLOSED,
    SOCKET_PIPE_BROKEN,
    WS,
    WSS
} from './constants';
import {JsonRpcError} from './error';
import {Notification} from './notification';
import {Request} from './request';
import {Response} from './response';
import {
    JsonRpcErrorObject,
    JsonRpcNotification,
    JsonRpcResponse,
    Progress
} from './types';
import {
    isJsonRpcErrorObject,
    isJsonRpcNotification
} from './utils';


const SOCKET_CLOSED_ERROR = {
    code: SOCKET_CLOSED,
    message: 'Socket connection closed'
};


/**
 * Simple impl. of the JSON RPC 2.0 spec with WebSocket using rxjs
 * http://www.jsonrpc.org/specification
 */
export class Client implements Subscribable<Notification> {
    static create(options: ClientOptions): Client {
        return new this(options);
    }

    readonly ws = createWs(this.options);

    private serialize = getSerializer(this.options);
    private deserialize = getDeserializer(this.options);

    // NOTE: Anything that's not a JSON RPC string is filtered
    private json: Observable<any> = this.ws.pipe(
        mergeMap(this.deserialize),
        filter(isJsonRpcMessage),
        catchError(err => {
            const error = isObject(err) && isNumber(err.code) && isString(err.reason) // CloseEvent
                ? toJsonRpcError(SOCKET_CLOSED_ERROR)
                : toJsonRpcError({
                    code: SOCKET_PIPE_BROKEN,
                    message: 'Socket pipe broken'
                });

            return Promise.reject(error);
        }));

    private notifications: Observable<Notification> = this.json.pipe(
        filter(isJsonRpcNotification),
        map(toNotification));

    // Create a connection with the socket on init;
    // notifications cannot be sent unless there is at least one subscription.
    // @ts-ignore
    private sub = this.json.subscribe({error: noop});

    constructor(readonly options: ClientOptions) {}

    /**
     * Subscribe to server notifications
     * @param observerOrNext
     * @param error
     * @param complete
     */
    subscribe(
        observerOrNext?: PartialObserver<Notification> | ((value: Notification) => void),
        error?: (error: any) => void,
        complete?: () => void
    ): Subscription {
        return this.notifications.subscribe(observerOrNext as any, error, complete);
    }

    /**
     * Server notifications as Observable
     */
    asObservable(): Observable<Notification> {
        return this.notifications;
    }

    /**
     * Make a JSON RPC notification
     * @param method
     * @param params
     */
    notify<P>(notification: Notification<P>): void;
    notify<P>(method: string, params?: P): void;
    notify<P>(methodOrNotification: string | Notification, params?: P): void {
        const notification = methodOrNotification instanceof Notification
            ? methodOrNotification
            : new Notification(methodOrNotification, params);
        const data = this.serialize(notification);
        this.ws.next(data);
    }

    /**
     * Make a JSON RPC request
     * @param request
     */
    request<P, R>(request: Request<P>): RequestTask<P, R>;

    /**
     * Make a JSON RPC request
     * @param method
     * @param [params]
     */
    request<P, R>(method: string, params?: P): RequestTask<P, R>;

    request<P, R>(requestOrMethod: string | Request<P>, params?: P): RequestTask<P, R> {
        const request = isRequest(requestOrMethod)
            ? requestOrMethod
            : new Request(requestOrMethod, params);
        const {id} = request;

        const responseFilter = createResponseFilter<R>(id);
        const progressFilter = createProgressFilter(id);

        let done: boolean;
        let canceling: boolean;

        const response = new Promise<R>((resolve, reject) => {
            this.json.pipe(
                filter(responseFilter),
                take(1),
                tap(() => {
                    done = true;
                }))
                .subscribe(response => {
                    const {result, error} = response;
                    if (isJsonRpcErrorObject(error)) {
                        reject(toJsonRpcError(error));
                    } else {
                        resolve(result);
                    }
                }, err => {
                    reject(err);
                }, () => {
                    const error = toJsonRpcError(SOCKET_CLOSED_ERROR);
                    reject(error);
                });
        });

        const progress = new ReplaySubject<Progress>(1);

        const sub = this.json.pipe(filter(progressFilter), map(toProgress))
            .subscribe(p => {
                progress.next(p);
                // Cleanup
                if (p.amount >= 1) {
                    sub.unsubscribe();
                }
            }, noop);

        response.then(() => {
            progress.complete();
        }, err => {
            progress.error(err);
        });

        const data = this.serialize(request);
        this.ws.next(data);

        return {
            request,
            then: (onSuccess, onFailure) => {
                return response.then(onSuccess, onFailure);
            },
            on: (event: TaskEvent) => {
                if (event === PROGRESS_EVENT) {
                    return progress.asObservable();
                }
                return EMPTY;
            },
            cancel: () => {
                if (!done && !canceling) {
                    canceling = true;
                    this.notify(CANCEL, {id});
                }
            }
        };
    }

    /**
     * Make a JSON RPC batch request
     * @param items
     */
    batch(...items: Notification[]): BatchTask<undefined>;
    batch(...items: Request[]): BatchTask<BatchResponse>;
    batch(...items: Array<Notification | Request>): BatchTask<BatchResponse>;
    batch(...items: Array<Notification | Request>): BatchTask<BatchResponse | undefined> {
        const requests = items.filter(isRequest);
        const requestsCount = requests.length;
        const hasRequests = requestsCount > 0;
        const batchResponseFilter = createBatchResponseFilter(requests);

        let done: boolean;
        let canceling: boolean;

        const response = new Promise<BatchResponse>((resolve, reject) => {
            this.json.pipe(
                filter(batchResponseFilter),
                take(1),
                tap(() => {
                    done = true;
                }))
                .subscribe(response => {
                    resolve(response.map(r => new Response(r)));
                }, err => {
                    reject(err);
                }, () => {
                    const error = toJsonRpcError(SOCKET_CLOSED_ERROR);
                    reject(error);
                });
        });

        const progress = new ReplaySubject<Progress>(1);
        const progressAggregate = createProgressAggregate(requests);

        const sub = merge(...requests.map(({id}) => createProgressFilter(id))
            .map(f => this.json.pipe(
                filter(f),
                map(progressAggregate))))
            .pipe(distinct())
            .subscribe(amount => {
                progress.next({amount});
                // Cleanup
                if (amount >= 1) {
                    sub.unsubscribe();
                }
            }, noop);

        response.then(() => {
            progress.complete();
        }, err => {
            progress.error(err);
        });

        const data = this.serialize(items);
        this.ws.next(data);

        return {
            then: (onSuccess, onFailure) => {
                const result: Promise<BatchResponse | undefined> = !items.length
                    ? Promise.reject(toJsonRpcError({
                        code: INVALID_REQUEST,
                        message: 'Invalid request'
                    }))
                    : !hasRequests
                        ? Promise.resolve(void 0)
                        : response;

                return result.then(onSuccess, onFailure);
            },
            on: (event: TaskEvent) => {
                if (event === PROGRESS_EVENT && hasRequests) {
                    return progress.asObservable();
                }
                return EMPTY;
            },
            cancel: () => {
                if (!done && !canceling && hasRequests) {
                    canceling = true;
                    const notifications = requests.map(({id}) => new Notification(CANCEL, {id}));
                    this.batch(...notifications);
                }
            }
        };
    }

    /**
     * Close the connection with the websocket cleanly
     * @see https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/close
     */
    disconnect() {
        this.ws.error({
            // https://developer.mozilla.org/en-US/docs/Web/API/CloseEvent#Status_codes
            code: 1000, // Normal closure
            reason: 'Client wanted disconnect'
        });
    }
}


export interface ClientOptions extends Pick<WebSocketSubjectConfig<any>, 'url' | 'protocol'> {
    onConnected?(): void;
    onClosed?(evt: CloseEvent): void;
    deserializer?(evt: MessageEvent): Promise<JsonRpcNotification | JsonRpcResponse>;
    serializer?(data: Notification | Request): ArrayBuffer | string;
}

export interface RequestTask<P, R> extends PromiseLike<R> {
    request: Request<P>;
    on(event: TaskEvent): Observable<Progress>;
    cancel(): void;
}

export interface BatchTask<T> extends PromiseLike<T> {
    on(event: TaskEvent): Observable<Progress>;
    cancel(): void;
}

export type BatchResponse = Response[];

export type TaskEvent = PROGRESS_EVENT_TYPE;


/**
 * @param config
 * @private
 */
export function createWs(config: ClientOptions): WebSocketSubject<any> {
    const {onConnected, onClosed} = config;
    const url = setWsProtocol(config.url);

    return webSocket({
        url,
        protocol: config.protocol,
        // Override rxjs' default mechanism to JSON.stringify()
        serializer: (message: any) => message,
        // Override rxjs' default mechanism to JSON.parse().
        // Parsing will be done async by the client in a different thread,
        // without blocking the UI.
        deserializer: (evt: MessageEvent) => evt,
        openObserver: {
            next: () => {
                if (isFunction(onConnected)) {
                    onConnected();
                }
            }
        },
        closeObserver: {
            next: evt => {
                if (isFunction(onClosed)) {
                    onClosed(evt);
                }
            }
        }
    });
}

/**
 * @param url
 * @private
 */
export function setWsProtocol(url: string): string {
    const segment = '://';
    if (url.startsWith(WS) || url.startsWith(WSS)) {
        return url;
    } else if (url.startsWith(HTTP)) {
        return url.replace(HTTP, WS);
    } else if (url.startsWith(HTTPS)) {
        return url.replace(HTTPS, WSS);
    } else if (url.startsWith(segment)) {
        return `${WS}${url}`;
    }
    return `${WS}${segment}${url}`;
}

/**
 * @param id
 * @private
 */
export function createResponseFilter<T>(id: string | number | null) {
    return (json: any): json is JsonRpcResponse<T> => isObject(json)
        && json.jsonrpc === JSON_RPC_VERSION
        && isString(json.id)
        && json.id === id;
}

/**
 * @param requests
 * @private
 */
export function createBatchResponseFilter(requests: Request[]) {
    return (json: any): json is BatchResponse => Array.isArray(json)
        && json.every(isJsonRpcResponse)
        && requests.every(request => json.findIndex(item => item.id === request.id) !== -1);
}

/**
 * @param id
 * @private
 */
export function createProgressFilter(id: string | number) {
    return (json: any): json is ProgressNotification => isObject(json)
        && json.jsonrpc === JSON_RPC_VERSION
        && json.method === PROGRESS
        && isObject(json.params)
        && json.params.id === id;
}

/**
 * @param notification
 * @private
 */
export function toProgress(notification: ProgressNotification): Progress {
    const {amount, operation} = notification.params!;
    return {amount, operation};
}

/**
 * @param evt
 * @private
 */
export async function fromJsonAsync<T>(evt: MessageEvent): Promise<T | undefined> {
    try {
        const data = JSON.parse(evt.data);
        return data as T;
    } catch {
        return;
    }
}

function getSerializer(config: ClientOptions) {
    const {serializer} = config;
    if (isFunction(serializer)) {
        return serializer;
    }
    return toJson;
}

function getDeserializer(config: ClientOptions) {
    const {deserializer} = config;
    if (isFunction(deserializer)) {
        return deserializer;
    }
    return fromJsonAsync;
}

function toJson(data: any) {
    return JSON.stringify(data);
}

/**
 * @param value
 * @private
 */
export function isJsonRpcMessage(value: any) {
    return (Array.isArray(value) && value.length > 0 &&  value.every(isJsonRpcObject))
        || isJsonRpcObject(value);
}

/**
 * @param value
 * @private
 */
export function isJsonRpcObject(value: any) {
    return isJsonRpcNotification(value)
        || isJsonRpcResponse(value);
}

/**
 * @param value
 * @private
 */
export function isRequest(value: any): value is Request {
    return value instanceof Request;
}

/**
 * @param value
 * @private
 */
export function isJsonRpcResponse<T>(value: any): value is JsonRpcResponse<T> {
    return isObject(value)
        && value.jsonrpc === JSON_RPC_VERSION
        && (isString(value.id) || isNumber(value.id) || value.id === null)
        && (value.result !== undefined || isJsonRpcErrorObject(value.error));
}


/**
 * @param requests
 * @private
 */
export function createProgressAggregate(requests: Request[]) {
    const items: ProgressNotification[] = [];

    return (progress: ProgressNotification) => {
        const index = items.findIndex(p => p.params!.id === progress.params!.id);

        if (index !== -1) {
            items.splice(index, 1, progress);
        } else {
            items.push(progress);
        }

        const total = items.map(toProgress)
            .map(item => item.amount)
            .reduce((acc, amount) => acc + amount, 0);
        const amount = total / requests.length;

        return amount;
    };
}

/**
 * @param error
 * @private
 */
function toJsonRpcError(error: JsonRpcErrorObject) {
    return new JsonRpcError(error);
}

/**
 * @param json
 * @private
 */
function toNotification(json: JsonRpcNotification) {
    return Notification.fromJson(json);
}


export type ProgressNotification = JsonRpcNotification<UpstreamProgress>;

export interface UpstreamProgress extends Progress {
    id: string | number;
}
