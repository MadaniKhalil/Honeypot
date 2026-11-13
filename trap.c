#include <stdio.h>
#include <unistd.h> // Provides POSIX system calls. C does not give you close() for file descriptors, because it is a linux call.
#include <string.h> 
#include <sys/socket.h> 
#include <netinet/in.h> // Provides internet address structs and constants for you! for example : {struct sockaddr_in, htons(), htonl(), INADDR_LOOPBACK}
#include <arpa/inet.h> // Provides address conversion functions like inet_ntop() and constants like INET_ADDRSTRLEN
#include <time.h>
#include <sqlite3.h>

static int send_text(int socket_fd, const char *msg) // static = private to this file (Other .c files can't see it!) . const = read-only. Compiler shoots you if you attempt to change the text.
{
    ssize_t len = strlen(msg); // ssize_t here instead of size_t because later we're gonna comapare it with bytes_sent (which is type ssize_t)
    ssize_t bytes_sent = send(socket_fd, msg, len, MSG_NOSIGNAL); // ssize_t = signed integer for byte counts (send() and recv() return how bytes moved or -1 are lost so ssize_t is better than size_t here)

    if (bytes_sent == -1) {
        perror("send"); 
        return -1;
    }
    if (bytes_sent < len) { // If some data was lost 
        fprintf(stderr, "Data not fully sent \n");
        return -1;
    }
    return 0; // Unix convention : 0 = success (like close() , listen()....)
}

static ssize_t read_line(int socket_fd, char *buffer, size_t size)
{
    ssize_t bytes_read = recv(socket_fd, buffer, size - 1, 0);

    if (bytes_read == -1) {
        perror("recv");
        return -1;
    }
    if (bytes_read == 0)
        return 0; // if visitor closed the connection.

    buffer[bytes_read] = '\0'; // This obviously ends the string , but why ? => recv() returns raw bytes and if you try printf() or strlen() on it , which read until they hit the string terminator , they will run past your data into garbage
    return bytes_read;
}

static void trim_line_ending(char *text)
{
    text[strcspn(text, "\r\n")] = '\0'; // strcp(text,"\r\n") = counts how many characters at the start of text are not \r or \n 
}


int main(void)
{   // AF_INET == AddressFamily: internet (IPv4)
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr); // zero out the struct so it has no garbage! but we fill immediatly after why zero it ? sockaddr_in has more fields than just 3!
    addr.sin_family = AF_INET; // AddressFamily
    addr.sin_port = htons(2323); // the port. htons converts it to network byte order (big-endian), so the kernel reads 2323 and not a wrong number.
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // the IP. INADDR_LOOPBACK is 127.0.0.1, meaning only this machine can connect.
    // htonl is the 32-bit version of the byte-order conversion.

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

    // SQLite database creation (before accept, so no connection_fd to close yet)
    sqlite3 *db;
    if (sqlite3_open("attempts.db",&db) != SQLITE_OK){
        fprintf(stderr, "sqlite: %s\n",sqlite3_errmsg(db));
        sqlite3_close(db);
        close(listen_fd);
        return 1;
    }

    char *error_message = NULL;
    if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS attempts (time INTEGER,ip TEXT,username TEXT,password TEXT)", NULL, NULL, &error_message) != SQLITE_OK){
        fprintf(stderr, "sqlite: %s\n",error_message);
        sqlite3_free(error_message); // SQLite allocates the error text , so free it;
        sqlite3_close(db);
        close(listen_fd);
        return 1;
    }

    struct sockaddr_storage visitor_addr; // sockaddr_storage = a big,generic address box. Large enough to hold any address type: IPv4, IPv6,and Unix; Good for any type of VISITORS.
    socklen_t visitor_addr_size = sizeof visitor_addr;

    int connection_fd = accept(listen_fd, (struct sockaddr *)&visitor_addr, &visitor_addr_size);
    if (connection_fd == -1) {
        perror("accept");
        sqlite3_close(db);
        close(listen_fd);
        return 1;
    }

    printf("connected\n");

    // = ""; => In case visitors don't send anything and avoid garbage.
    char username[256] = "";
    char password[256] = "";
    if (send_text(connection_fd, "username: ") == -1 ||
        read_line(connection_fd, username, sizeof username) == -1 ||
        send_text(connection_fd, "password: ") == -1 ||
        read_line(connection_fd, password, sizeof password) == -1 ||
        send_text(connection_fd, "Login incorrect\n") == -1) {

        sqlite3_close(db);
        close(connection_fd);
        close(listen_fd);
        return 1;
    }
    char ip_text_buffer[INET_ADDRSTRLEN]; // INET_ADDRSTRLEN = a constant: the buffer size needed to hold an IPv4 address as text, including the '\0'.
    struct sockaddr_in * visitor_ipv4 = (struct sockaddr_in *)&visitor_addr;
    // (->) is for struct pointers , if you can access struct directly just use (.)
    if (inet_ntop(AF_INET,&visitor_ipv4->sin_addr,ip_text_buffer,sizeof ip_text_buffer) == NULL){ // inet_ntop() converts a binary IP address into readable text.
        perror("inet_ntop");
        sqlite3_close(db);
        close(connection_fd); 
        close(listen_fd);
        return 1;
    }
    
    trim_line_ending(username);
    trim_line_ending(password);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,"INSERT INTO attempts(time, ip, username, password) VALUES(?,?,?,?)",
                           -1, &stmt, NULL) != SQLITE_OK){
        fprintf(stderr,"sqlite: %s\n",sqlite3_errmsg(db));
        sqlite3_close(db);
        close(connection_fd);
        close(listen_fd);
        return 1;
    }
    if (sqlite3_bind_int64(stmt,1,(sqlite3_int64)time(NULL)) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 2, ip_text_buffer, -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 3, username, -1, SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_bind_text(stmt, 4, password, -1, SQLITE_TRANSIENT) != SQLITE_OK ) {
        fprintf(stderr,"sqlite: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt); // free()
        sqlite3_close(db);
        close(connection_fd);
        close(listen_fd);
        return 1;
    }
    
    if (sqlite3_step(stmt) != SQLITE_DONE){
        fprintf(stderr,"sqlite: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        close(connection_fd);  
        close(listen_fd);
        return 1;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    close(connection_fd);
    close(listen_fd);
    return 0;
}
