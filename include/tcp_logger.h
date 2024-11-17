// Based on code from: https://github.com/achillesdawn/pico-tcp-client
#ifndef _TCP_LOGGER_H_
#define _TCP_LOGGER_H_

#define LOGGER_WAIT 500
#define LOGGER_MAX_RETRIES 3
#define LOGGER_MAX_REQUEST_SIZE 4096

#include "string.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/malloc.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

typedef struct TCPLogger_ {
    struct tcp_pcb* pcb;
    ip_addr_t addr;
    u16_t port;

    // Status flags
    bool connected;
    bool complete;
    bool failure;
} TCPLogger;

// Helper methods
TCPLogger* tcpLoggerInit(const char* ip, const u16_t port);
bool tcpLoggerSend(TCPLogger* logger, char* message); // Returns true if success

bool sendLogMessage(const char* ip, const u16_t port, char* message);
bool sendHTTPMessage(const char* ip, const u16_t port, char* route, char* message);

#define VERIFY(expr) { err = ((expr)); if ((err != ERR_OK)) {cyw43_arch_lwip_end(); return false;} }

#endif