// https://www.jsonrpc.org/specification#compatibility
export const JSON_RPC_VERSION = '2.0';
export type JSON_RPC_VERSION_TYPE = typeof JSON_RPC_VERSION;

// http://www.jsonrpc.org/specification#error_object
// http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php
export const PARSE_ERROR = -32700;
export const INVALID_REQUEST = -32600;
export const METHOD_NOT_FOUND = -32601;
export const INVALID_PARAMS = -32602;
export const INTERNAL_ERROR = -32603;

// Rockets error codes
export const INVALID_JSON_RESPONSE = -31001;
export const REQUEST_ABORTED = -31002;
export const HTTP_ERROR = -31003;

// Rockets server notifications
export const CANCEL = 'cancel';
export const PROGRESS = 'progress';

// Request events
export const PROGRESS_EVENT = 'progress';
export type PROGRESS_EVENT_TYPE = typeof PROGRESS_EVENT;

// Custom error codes
export const SOCKET_CLOSED = -30100;
export const SOCKET_PIPE_BROKEN = -30101;

// UID
export const UID_BYTE_LENGTH = 6;

// Networking
export const HTTP = 'http';
export const HTTPS = 'https';
export const WS = 'ws';
export const WSS = 'wss';
export type ProtocolType = typeof HTTP
    | typeof HTTPS
    | typeof WS
    | typeof WSS;
