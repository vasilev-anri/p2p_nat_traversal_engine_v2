#pragma once


/**
 * CONNECTING - EINPROGRESS was returned by connect() syscall
 *
 * CONNECTED - TCP connected, no hello yet
 *
 * HANDSHAKING - hello sent, waiting for response
 *
 * READY - hello exchanged, fully operational
 *
 * CLOSED - session is over
 */
enum class SessionState {
    CONNECTING,
    CONNECTED,
    HANDSHAKING,
    READY,
    CLOSED
};
