#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("Socket!");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(2323);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof addr) == -1) {
        perror("Bind!");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 4) == -1) {
        perror("Listen!");
        close(listen_fd);
        return 1;
    }

    int connection_fd;
    struct sockaddr_storage new_addr;
    socklen_t newaddr_size = sizeof new_addr;

    connection_fd = accept(listen_fd, (struct sockaddr *)&new_addr, &newaddr_size);
    if (connection_fd == -1) {
        perror("accept");
        close(listen_fd);
        return 1;
    }

    printf("connected\n");

    char *msg = "login :";
    int len;
    ssize_t bytes_sent;
    len = strlen(msg);
    bytes_sent = send(connection_fd,msg,len,0);
    if (bytes_sent == -1){
      perror("send");
      close(connection_fd);
      close(listen_fd);
      return 1;
    }
    char buffer[256];
    int rec = recv(connection_fd,buffer,sizeof buffer - 1,0);
    if (rec == -1){
      perror("receive");
      close(connection_fd);
      close(listen_fd);
      return 1;
    }
    char *msg1 = "password: ";
    int len1;
    ssize_t bytes_sent1;
    len1 = strlen(msg1);
    bytes_sent1 = send(connection_fd,msg1,len1,0);
    if (bytes_sent1 == -1){
      perror("send");
      close(connection_fd);
      close(listen_fd);
      return 1;
    }
    char buffer1[256];
    int rec1 = recv(connection_fd,buffer1,sizeof buffer1 - 1,0);
    if (rec1 == -1){
      perror("receive");
      close(connection_fd);
      close(listen_fd);
      return 1;
    }
    char *msg2 = "login incorrect";
    int len2;
    ssize_t bytes_sent2;
    len2 = strlen(msg2);
    bytes_sent2 = send(connection_fd,msg2,len2,0);
    if (bytes_sent2 == -1){
      perror("send");
      close(connection_fd);
      close(listen_fd);
      return 1;
    }




    close(connection_fd);
    close(listen_fd);
    return 0;
}
