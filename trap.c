#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int send_text(int socket_fd, const char *msg)
{
    ssize_t len = strlen(msg);
    ssize_t bytes_sent = send(socket_fd, msg, len, MSG_NOSIGNAL);

    if (bytes_sent == -1) {
        perror("send");
        return -1;
    }
    if (bytes_sent < len) {
        fprintf(stderr, "Data not fully sent \n");
        return -1;
    }
    return 0;
}

static ssize_t read_line(int socket_fd, char *buffer, size_t size)
{
    ssize_t bytes_read = recv(socket_fd, buffer, size - 1, 0);

    if (bytes_read == -1) {
        perror("recv");
        return -1;
    }
    if (bytes_read == 0)
        return 0;

    buffer[bytes_read] = '\0';
    return bytes_read;
}

int main(void)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2323);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof addr) == -1) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 4) == -1) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    struct sockaddr_storage visitor_addr;
    socklen_t visitor_addr_size = sizeof visitor_addr;

    int connection_fd = accept(listen_fd, (struct sockaddr *)&visitor_addr, &visitor_addr_size);
    if (connection_fd == -1) {
        perror("accept");
        close(listen_fd);
        return 1;
    }

    printf("connected\n");

    char buffer[256];

    if (send_text(connection_fd, "username: ") == -1 ||
        read_line(connection_fd, buffer, sizeof buffer) == -1 ||
        send_text(connection_fd, "password: ") == -1 ||
        read_line(connection_fd, buffer, sizeof buffer) == -1 ||
        send_text(connection_fd, "Login incorrect\n") == -1) {
        close(connection_fd);
        close(listen_fd);
        return 1;
    }

    close(connection_fd);
    close(listen_fd);
    return 0;
}
