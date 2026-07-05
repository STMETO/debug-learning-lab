#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

// A simple TCP echo server + client to generate network traffic,
// softirq events, and socket-related system calls.
//
// Use irq:softirq_entry, net:*, and syscalls:sys_enter_* tracepoints.

#define PORT 19876
#define N_REQUESTS 500
#define MSG_SIZE 256

// ----- Echo server thread -----

void *echo_server(void *unused) {
    (void)unused;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("server socket"); return NULL; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = inet_addr("127.0.0.1"),
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return NULL;
    }

    listen(server_fd, 1);

    for (int i = 0; i < N_REQUESTS; i++) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        char buf[MSG_SIZE];
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
        if (n > 0) {
            send(client_fd, buf, n, 0);  // echo back
        }
        close(client_fd);
    }

    close(server_fd);
    return NULL;
}

// ----- Client (runs in main thread) -----

void run_client() {
    // Give server time to start
    usleep(50000);

    for (int i = 0; i < N_REQUESTS; i++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) { perror("client socket"); return; }

        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(PORT),
            .sin_addr.s_addr = inet_addr("127.0.0.1"),
        };

        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(fd);
            continue;
        }

        char send_buf[MSG_SIZE], recv_buf[MSG_SIZE];
        snprintf(send_buf, sizeof(send_buf), "PING-%04d", i);
        send(fd, send_buf, strlen(send_buf) + 1, 0);
        recv(fd, recv_buf, sizeof(recv_buf), 0);
        close(fd);
    }
}

// ----- CPU-intensive work (to mix with network) -----

void cpu_work() {
    // Generate some load to see softirq interleaved with computation
    volatile long x = 0;
    for (int i = 0; i < 5000000; i++) {
        x += i * 7;
    }
}

// ----- Main -----

int main() {
    printf("=== Network & SoftIRQ Analysis ===\n");
    printf("N_REQUESTS = %d, MSG_SIZE = %d\n", N_REQUESTS, MSG_SIZE);

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, echo_server, NULL);

    printf("Running %d echo request/response cycles...\n", N_REQUESTS);
    run_client();

    printf("CPU work mixed in (watch softirq interleave)...\n");
    cpu_work();

    pthread_join(server_thread, NULL);

    printf("=== Done! ===\n");
    return 0;
}
