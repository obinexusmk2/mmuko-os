/* ============================================================================
 * NSIGII WEBSOCKET SERVER
 * Real-time Communication for Web Interface
 * 
 * WebSocket must be used to serialize data and send/deserialize
 * ============================================================================ */

#include "nsigii.h"
#include "serialization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close(fd) closesocket(fd)
#define usleep(us) Sleep((us) / 1000)
#define sleep(s) Sleep((s) * 1000)
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#define WS_PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define STATIC_ROOT "pages/mac2loopback"

/* ============================================================================
 * WEBSOCKET CLIENT STRUCTURE
 * ============================================================================ */

typedef struct {
    int socket;
    struct sockaddr_in addr;
    int connected;
    uint8_t buffer[BUFFER_SIZE];
    size_t buffer_len;
} ws_client_t;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static volatile int server_running = 1;
static int server_socket = -1;
static ws_client_t clients[MAX_CLIENTS];
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * SIGNAL HANDLER
 * ============================================================================ */

void ws_signal_handler(int sig) {
    (void)sig;
    printf("\n[WebSocket] Shutting down server...\n");
    server_running = 0;
}

/* ============================================================================
 * SERVER INITIALIZATION
 * ============================================================================ */

int ws_server_init(void) {
    /* Create server socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return -1;
    }
    
    /* Allow socket reuse */
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(server_socket);
        return -1;
    }
    
    /* Set up server address */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(WS_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    /* Bind socket */
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return -1;
    }
    
    /* Listen for connections */
    if (listen(server_socket, 5) < 0) {
        perror("listen");
        close(server_socket);
        return -1;
    }
    
    /* Set non-blocking */
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(server_socket, FIONBIO, &mode);
#else
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
#endif
    
    /* Initialize clients */
    memset(clients, 0, sizeof(clients));
    
    printf("[WebSocket] Server initialized on port %d\n", WS_PORT);
    return 0;
}

void ws_server_cleanup(void) {
    /* Close all client connections */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].connected) {
            close(clients[i].socket);
            clients[i].connected = 0;
        }
    }
    
    /* Close server socket */
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
}

/* ============================================================================
 * CLIENT MANAGEMENT
 * ============================================================================ */

int ws_accept_client(void) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }
        return -1;
    }
    
    /* Find free client slot */
    pthread_mutex_lock(&clients_mutex);
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].connected) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        pthread_mutex_unlock(&clients_mutex);
        printf("[WebSocket] Max clients reached, rejecting connection\n");
        close(client_socket);
        return -1;
    }
    
    /* Initialize client */
    clients[slot].socket = client_socket;
    clients[slot].addr = client_addr;
    clients[slot].connected = 1;
    clients[slot].buffer_len = 0;
    
    pthread_mutex_unlock(&clients_mutex);
    
    printf("[WebSocket] Client %d connected from %s:%d\n",
           slot, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    return slot;
}

void ws_remove_client(int slot) {
    if (slot < 0 || slot >= MAX_CLIENTS) return;
    
    pthread_mutex_lock(&clients_mutex);
    
    if (clients[slot].connected) {
        close(clients[slot].socket);
        clients[slot].connected = 0;
        clients[slot].buffer_len = 0;
        printf("[WebSocket] Client %d disconnected\n", slot);
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

/* ============================================================================
 * STATIC FILE SERVING (HTTP)
 * ============================================================================ */

static const char* ws_get_content_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0 || strcmp(ext, ".mjs") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    return "application/octet-stream";
}

static int ws_serve_static_file(int client_socket, const char* request) {
    /* Extract path from GET request */
    if (strncmp(request, "GET ", 4) != 0) return -1;

    const char* path_start = request + 4;
    const char* path_end = strchr(path_start, ' ');
    if (!path_end) return -1;

    char url_path[256];
    size_t path_len = (size_t)(path_end - path_start);
    if (path_len >= sizeof(url_path)) path_len = sizeof(url_path) - 1;
    memcpy(url_path, path_start, path_len);
    url_path[path_len] = '\0';

    /* Map URL to file path */
    char file_path[512];
    if (strcmp(url_path, "/") == 0) {
        snprintf(file_path, sizeof(file_path), "%s/html/index.html", STATIC_ROOT);
    } else if (strncmp(url_path, "/js/", 4) == 0) {
        snprintf(file_path, sizeof(file_path), "%s/js/%s", STATIC_ROOT, url_path + 4);
    } else if (strncmp(url_path, "/styles/", 8) == 0) {
        snprintf(file_path, sizeof(file_path), "%s/styles/%s", STATIC_ROOT, url_path + 8);
    } else if (strncmp(url_path, "/html/", 6) == 0) {
        snprintf(file_path, sizeof(file_path), "%s/html/%s", STATIC_ROOT, url_path + 6);
    } else {
        /* Try direct path under static root */
        snprintf(file_path, sizeof(file_path), "%s%s", STATIC_ROOT, url_path);
    }

    /* Security: reject path traversal */
    if (strstr(file_path, "..") != NULL) {
        const char* resp = "HTTP/1.1 403 Forbidden\r\nContent-Length: 9\r\n\r\nForbidden";
        send(client_socket, resp, strlen(resp), 0);
        return 0;
    }

    /* Open and serve file */
    FILE* f = fopen(file_path, "rb");
    if (!f) {
        printf("[HTTP] 404 Not Found: %s\n", file_path);
        const char* resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        send(client_socket, resp, strlen(resp), 0);
        return 0;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Send HTTP response header */
    const char* content_type = ws_get_content_type(file_path);
    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n",
        content_type, file_size);
    send(client_socket, header, strlen(header), 0);

    /* Send file body */
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        send(client_socket, buf, (int)n, 0);
    }
    fclose(f);

    printf("[HTTP] 200 OK: %s (%ld bytes)\n", file_path, file_size);
    return 0;
}

/* ============================================================================
 * WEBSOCKET HANDSHAKE
 * ============================================================================ */

int ws_handle_handshake(int slot) {
    if (slot < 0 || slot >= MAX_CLIENTS || !clients[slot].connected) return -1;
    
    ws_client_t* client = &clients[slot];
    
    /* Receive HTTP request */
    ssize_t received = recv(client->socket, client->buffer, BUFFER_SIZE - 1, 0);
    if (received <= 0) return -1;
    
    client->buffer[received] = '\0';
    
    /* Check if this is a WebSocket upgrade request or plain HTTP */
    char* key_start = strstr((char*)client->buffer, "Sec-WebSocket-Key: ");
    if (!key_start) {
        /* Not a WebSocket request - serve static file */
        ws_serve_static_file(client->socket, (const char*)client->buffer);
        return -1;  /* Close after serving (HTTP/1.0 style) */
    }
    
    key_start += 19;
    char* key_end = strstr(key_start, "\r\n");
    if (!key_end) return -1;
    
    char client_key[32];
    size_t key_len = key_end - key_start;
    if (key_len >= sizeof(client_key)) key_len = sizeof(client_key) - 1;
    strncpy(client_key, key_start, key_len);
    client_key[key_len] = '\0';
    
    /* Generate accept key */
    char accept_key[32];
    if (nsigii_ws_generate_accept_key(client_key, accept_key) != 0) return -1;
    
    /* Send handshake response */
    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_key
    );
    
    if (send(client->socket, response, strlen(response), 0) < 0) return -1;
    
    printf("[WebSocket] Client %d handshake completed\n", slot);
    return 0;
}

/* ============================================================================
 * WEBSOCKET FRAME HANDLING
 * ============================================================================ */

int ws_send_frame(int slot, uint8_t opcode, const uint8_t* payload, size_t len) {
    if (slot < 0 || slot >= MAX_CLIENTS || !clients[slot].connected) return -1;
    
    ws_client_t* client = &clients[slot];
    
    /* Build frame */
    uint8_t frame[BUFFER_SIZE];
    size_t frame_len = 0;
    
    /* FIN + opcode */
    frame[frame_len++] = 0x80 | opcode;
    
    /* Payload length */
    if (len < 126) {
        frame[frame_len++] = (uint8_t)len;
    } else if (len < 65536) {
        frame[frame_len++] = 126;
        frame[frame_len++] = (len >> 8) & 0xFF;
        frame[frame_len++] = len & 0xFF;
    } else {
        frame[frame_len++] = 127;
        for (int i = 7; i >= 0; i--) {
            frame[frame_len++] = (len >> (i * 8)) & 0xFF;
        }
    }
    
    /* Payload */
    memcpy(frame + frame_len, payload, len);
    frame_len += len;
    
    /* Send frame */
    if (send(client->socket, frame, frame_len, 0) < 0) {
        return -1;
    }
    
    return 0;
}

int ws_send_text(int slot, const char* text) {
    return ws_send_frame(slot, NSIGII_WS_TEXT, (const uint8_t*)text, strlen(text));
}

int ws_send_json_packet(int slot, const nsigii_packet_t* packet) {
    char json[4096];
    if (nsigii_serialize_json(packet, json, sizeof(json)) != 0) return -1;
    return ws_send_text(slot, json);
}

int ws_receive_frame(int slot, uint8_t* opcode, uint8_t* payload, size_t* len) {
    if (slot < 0 || slot >= MAX_CLIENTS || !clients[slot].connected) return -1;
    
    ws_client_t* client = &clients[slot];
    
    /* Receive header */
    uint8_t header[2];
    ssize_t received = recv(client->socket, header, 2, 0);
    if (received < 2) return -1;
    
    *opcode = header[0] & 0x0F;
    uint8_t masked = (header[1] >> 7) & 0x01;
    uint64_t payload_len = header[1] & 0x7F;
    
    /* Extended length */
    if (payload_len == 126) {
        uint8_t ext_len[2];
        recv(client->socket, ext_len, 2, 0);
        payload_len = ((uint64_t)ext_len[0] << 8) | ext_len[1];
    } else if (payload_len == 127) {
        uint8_t ext_len[8];
        recv(client->socket, ext_len, 8, 0);
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | ext_len[i];
        }
    }
    
    if (payload_len > *len) payload_len = *len;
    
    /* Masking key */
    uint8_t mask[4] = {0};
    if (masked) {
        recv(client->socket, mask, 4, 0);
    }
    
    /* Payload */
    *len = (size_t)payload_len;
    received = recv(client->socket, payload, *len, 0);
    if (received < 0) return -1;
    
    /* Unmask */
    if (masked) {
        for (size_t i = 0; i < *len; i++) {
            payload[i] ^= mask[i % 4];
        }
    }
    
    return 0;
}

/* ============================================================================
 * PACKET PROCESSING
 * ============================================================================ */

void ws_process_packet(int slot, const uint8_t* data, size_t len) {
    if (slot < 0 || slot >= MAX_CLIENTS) return;
    
    /* Try to deserialize as NSIGII packet */
    nsigii_packet_t packet;
    memset(&packet, 0, sizeof(packet));
    
    /* First try JSON */
    if (nsigii_deserialize_json((const char*)data, &packet) == 0) {
        printf("[WebSocket] Client %d received JSON packet: seq=%u\n",
               slot, packet.header.sequence_token);
        
        /* Echo back with verification */
        packet.topology.wheel_position = 240;
        packet.verification.rwx_flags |= RWX_EXECUTE;
        
        ws_send_json_packet(slot, &packet);
        return;
    }
    
    /* Try binary */
    if (nsigii_deserialize_binary(data, len, &packet) == 0) {
        printf("[WebSocket] Client %d received binary packet: seq=%u\n",
               slot, packet.header.sequence_token);
        
        ws_send_json_packet(slot, &packet);
        return;
    }
    
    /* Treat as text message */
    printf("[WebSocket] Client %d received text: %.*s\n",
           slot, (int)len, data);
    
    /* Echo back */
    ws_send_frame(slot, NSIGII_WS_TEXT, data, len);
}

/* ============================================================================
 * MAIN SERVER LOOP
 * ============================================================================ */

void* ws_server_thread(void* arg) {
    (void)arg;
    
    printf("[WebSocket] Server thread started\n");
    
    while (server_running) {
        /* Accept new connections */
        ws_accept_client();
        
        /* Handle client I/O */
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].connected) continue;
            
            /* Check if handshake needed */
            if (clients[i].buffer_len == 0) {
                /* Try handshake */
                if (ws_handle_handshake(i) != 0) {
                    ws_remove_client(i);
                    continue;
                }
                clients[i].buffer_len = 1;  /* Mark as handshaked */
            }
            
            /* Receive frame */
            uint8_t opcode;
            uint8_t payload[BUFFER_SIZE];
            size_t len = sizeof(payload);
            
            if (ws_receive_frame(i, &opcode, payload, &len) == 0) {
                switch (opcode) {
                    case NSIGII_WS_TEXT:
                    case NSIGII_WS_BINARY:
                        ws_process_packet(i, payload, len);
                        break;
                    
                    case NSIGII_WS_CLOSE:
                        ws_remove_client(i);
                        break;
                    
                    case NSIGII_WS_PING:
                        ws_send_frame(i, NSIGII_WS_PONG, payload, len);
                        break;
                    
                    default:
                        break;
                }
            }
        }
        
        usleep(1000);  /* 1ms sleep */
    }
    
    printf("[WebSocket] Server thread stopped\n");
    return NULL;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int nsigii_websocket_start(void) {
    if (ws_server_init() != 0) return -1;
    
    pthread_t thread;
    if (pthread_create(&thread, NULL, ws_server_thread, NULL) != 0) {
        ws_server_cleanup();
        return -1;
    }
    
    pthread_detach(thread);
    return 0;
}

void nsigii_websocket_stop(void) {
    server_running = 0;
    ws_server_cleanup();
}

int nsigii_websocket_broadcast_packet(const nsigii_packet_t* packet) {
    if (!packet) return -1;
    
    pthread_mutex_lock(&clients_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].connected && clients[i].buffer_len > 0) {
            ws_send_json_packet(i, packet);
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    
    return 0;
}

/* ============================================================================
 * STANDALONE SERVER MAIN
 * ============================================================================ */

#ifdef WS_STANDALONE
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    signal(SIGINT, ws_signal_handler);
    signal(SIGTERM, ws_signal_handler);
    
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║  NSIGII WebSocket Server                                         ║\n");
    printf("║  Port: %d                                                        ║\n", WS_PORT);
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
    
    if (nsigii_websocket_start() != 0) {
        fprintf(stderr, "Failed to start WebSocket server\n");
        return 1;
    }
    
    printf("WebSocket server running. Press Ctrl+C to stop.\n\n");
    
    while (server_running) {
        sleep(1);
    }
    
    nsigii_websocket_stop();
    printf("Server stopped.\n");
    
    return 0;
}
#endif
