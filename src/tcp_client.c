#include "string.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/malloc.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "tcp_client.h"

const char TCP_SERVER_IP[] = "192.168.0.104";
const u16_t TCP_PORT = 9988;

static err_t tcp_client_sent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    printf("ON SENT CALLBACK\n\n");
    printf("sent %d", len);
    TCP_CLIENT_T* client = (TCP_CLIENT_T*)arg;
    client->complete = true;
    return ERR_OK;
}

static err_t tcp_client_recv(void* arg, struct tcp_pcb* tpcb, struct pbuf* packet_buffer, err_t err) {
    /*
    API implementation details are in the section 'Receiving TCP data':
    https://www.nongnu.org/lwip/2_0_x/raw_api.html
    */
    TCP_CLIENT_T* client = (TCP_CLIENT_T*)arg;

    cyw43_arch_lwip_check();
    if (!packet_buffer) {
        // The callback function will be passed a NULL pbuf 
        // to indicate that the remote host has closed the connection.
        printf("CONNECTION CLOSED");
    }

    if (packet_buffer->tot_len > 0) {
        printf("recv %d err %d\n", packet_buffer->tot_len, err);

        for (struct pbuf* packet = packet_buffer; packet != NULL; packet = packet->next) {

            const char* payload = (char*)packet->payload;
            char message[packet->len];
            strncpy(message, payload, packet->len);
            printf("%s\n", message);

            if (strcmp(message, "HELLO FROM GO") == 0) {
                printf("MESSAGE ACKNOWLEDGED, COMPLETING");
                client->complete = true;
            }
        }

        // API requires tcp_recved be called within the recv callback
        // The len argument indicates the length of the received data.
        tcp_recved(tpcb, packet_buffer->tot_len);
    }

    // If there are no errors and the callback function is to return
    // ERR_OK, then it must free the pbuf. Otherwise, it must not
    // free the pbuf so that lwIP core code can store it.
    pbuf_free(packet_buffer);


    return ERR_OK;
}

err_t tcp_client_on_connected(void* arg, struct tcp_pcb* tpcb, err_t err) {
    TCP_CLIENT_T* client = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return err;
    }

    printf("CONNECTED TO SERVER\n\n");

    client->connected = true;
    return ERR_OK;
}

TCP_CLIENT_T* tcp_client_init(void) {
    TCP_CLIENT_T* client = calloc(1, sizeof(TCP_CLIENT_T));
    if (!client) {
        printf("failed to allocate state\n");
        return NULL;
    }

    client->tcp_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!client->tcp_pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(client->tcp_pcb, client);
    tcp_sent(client->tcp_pcb, tcp_client_sent);
    tcp_recv(client->tcp_pcb, tcp_client_recv);

    ip4addr_aton(TCP_SERVER_IP, &client->remote_addr);

    strcpy(client->buffer, "Initializing...\n");
    client->buffer_len = strlen(client->buffer);

    return client;
}

bool tcp_client_connect(TCP_CLIENT_T* client) {

    printf("Connecting to %s port %d\n", ip4addr_ntoa(&client->remote_addr), TCP_PORT);
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(client->tcp_pcb, &client->remote_addr, TCP_PORT, tcp_client_on_connected);
    if (err != ERR_OK) {
        printf("CONNECT ERROR, error number %d", err);
        return false;
    }

    err = tcp_write(client->tcp_pcb, client->buffer, client->buffer_len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("WRITE ERROR CODE %d", err);
        return false;
    }

    err = tcp_output(client->tcp_pcb);
    if (err != ERR_OK) {
        printf("TCP OUTPUT ERR CODE %d", err);
        return false;
    }
    cyw43_arch_lwip_end();

    while (!client->complete) {
        
        printf("waiting\n");    
        sleep_ms(1000);
        
    }
    printf("DONE");

    return true;
}

void tcp_client_send(TCP_CLIENT_T* client) {
    client->buffer_len = strlen(client->buffer);
    client->complete = false;
    cyw43_arch_lwip_begin();
    tcp_write(client->tcp_pcb, client->buffer, client->buffer_len, TCP_WRITE_FLAG_COPY);
    tcp_output(client->tcp_pcb);
    cyw43_arch_lwip_end();

    while (!client->complete) {
        printf("waiting\n");    
        sleep_ms(1000);   
    }
}