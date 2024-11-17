#include "tcp_logger.h"

// Callbacks
static err_t tcp_client_sent(void* arg, struct tcp_pcb* tpcb, u16_t len);
static err_t tcp_client_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* packet_buffer, err_t err);
static err_t tcp_client_on_connected(void* arg, struct tcp_pcb* tpcb, err_t err);
static void tcp_client_on_error(void* arg, err_t err);

////////////////////////
// Implementations
////////////////////////

TCPLogger* tcpLoggerInit(const char* ip, const u16_t port) {
    TCPLogger* logger = calloc(1, sizeof(TCPLogger));
    if (!logger) return NULL;

    logger->pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!logger->pcb) return false;

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

    cyw43_arch_lwip_begin();
        VERIFY(tcp_connect(logger->pcb, &logger->addr, logger->port, tcp_client_on_connected));
        VERIFY(tcp_write(logger->pcb, message, (u16_t)strlen(message), TCP_WRITE_FLAG_COPY));
        VERIFY(tcp_output(logger->pcb));
    cyw43_arch_lwip_end();

    int tries = LOGGER_MAX_RETRIES;
    while ((tries-- > 0) && !logger->complete && !logger->failure) 
        sleep_ms(LOGGER_WAIT);
    if (!logger->complete) {
        tcp_abort(logger->pcb);
        cyw43_arch_lwip_end();
        return false;
    }

    cyw43_arch_lwip_begin();
        VERIFY(tcp_close(logger->pcb));
    cyw43_arch_lwip_end();

    return true;
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
    // printf("on send\n");
    TCPLogger* logger = (TCPLogger*)arg;
    logger->complete = true;
    return ERR_OK;
}
static err_t tcp_client_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* packet_buffer, err_t err) {
    // printf("on receive\n");
    return ERR_OK;
}
static err_t tcp_client_on_connected(void* arg, struct tcp_pcb* tpcb, err_t err) {
    // printf("on connect\n");
    TCPLogger* logger = (TCPLogger*)arg;
    logger->connected = true;
    return ERR_OK;
}
static void tcp_client_on_error(void* arg, err_t err) {
    // printf("on error\n");
    TCPLogger* logger = (TCPLogger*)arg;
    logger->failure = true;
}