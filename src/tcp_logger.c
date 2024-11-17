#include "tcp_logger.h"

// Callbacks
static err_t tcp_client_sent(void* arg, struct tcp_pcb* tpcb, u16_t len);
static err_t tcp_client_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* packet_buffer, err_t err);
static err_t tcp_client_on_connected(void* arg, struct tcp_pcb* tpcb, err_t err);
static void tcp_client_on_error(void* arg, err_t err);

static void tcpLoggerDestroy(TCPLogger* logger);

////////////////////////
// Implementations
////////////////////////

TCPLogger* tcpLoggerInit(const char* ip, const u16_t port) {
    TCPLogger* logger = calloc(1, sizeof(TCPLogger));
    if (!logger) return NULL;

    logger->pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!logger->pcb) {
        free(logger);
        return NULL;
    }

    tcp_arg(logger->pcb, logger);
    tcp_sent(logger->pcb, tcp_client_sent);
    tcp_recv(logger->pcb, tcp_client_recv);
    // tcp_err(logger->pcb, tcp_client_on_error);

    ip4addr_aton(ip, &logger->addr);
    logger->port = port;
    logger->complete = false; 
    logger->connected = false; 
    logger->failure = false;

    // printf("Line 31\n");

    return logger;
}

bool tcpLoggerSend(TCPLogger* logger, char* message) {
    err_t err;
    logger->complete = false; 
    logger->connected = false; 

    SAFE(tcp_connect(logger->pcb, &logger->addr, logger->port, tcp_client_on_connected));

    TRY_UNTIL(logger->connected);

    SAFE(tcp_write(logger->pcb, message, (u16_t)strlen(message), TCP_WRITE_FLAG_COPY));
    SAFE(tcp_output(logger->pcb));

    TRY_UNTIL(logger->complete);

    SAFE(tcp_close(logger->pcb));
    tcpLoggerDestroy(logger);
    return true;
}

static void tcpLoggerDestroy(TCPLogger* logger) {
    if (logger->pcb) {
        tcp_abort(logger->pcb);
    }
    free(logger);
}

bool sendLogMessage(const char* ip, const u16_t port, char* message) {
    TCPLogger* logger = tcpLoggerInit(ip, port);
    return tcpLoggerSend(logger, message);
}

bool sendHTTPMessage(const char* ip, const u16_t port, char* route, char* message) {
    char buffer[LOGGER_MAX_REQUEST_SIZE];
    snprintf(buffer, sizeof(buffer), 
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        route, ip, port, strlen(message), message);
    sendLogMessage(ip, port, buffer);
}

//////////////////

static err_t tcp_client_sent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    TCPLogger* logger = (TCPLogger*)arg;
    logger->complete = true;
    return ERR_OK;
}
static err_t tcp_client_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* packet_buffer, err_t err) {
    if (packet_buffer) {
        pbuf_free(packet_buffer);
    }
    return ERR_OK;
}
static err_t tcp_client_on_connected(void* arg, struct tcp_pcb* tpcb, err_t err) {
    TCPLogger* logger = (TCPLogger*)arg;
    if (err == ERR_OK) {
        logger->connected = true;
    }
    else {
        logger->failure = true;
    }
    return err;
}
static void tcp_client_on_error(void* arg, err_t err) {
    // printf("on error\n");
    TCPLogger* logger = (TCPLogger*)arg;
    logger->failure = true;
}