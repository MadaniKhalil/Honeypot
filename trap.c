#include <stdio.h> // Standard Input/Output
#include <stdlib.h> // Standard Library
#include <unistd.h> // Provides access to the POSIX operating system API 
#include <sys/socket.h> // Provides the data types, structures, constants, and functions declarations needed for socket programming and network/inter-process communication (IPC).
#include <string.h> // for memset()
#include <netinet/in.h> 
#include <arpa/inet.h>

int main(void){
  int listen_fd = socket(AF_INET,SOCK_STREAM,0);
  if (listen_fd == -1){
    perror("Socket!");
    return 1;
  }
 
  // make an empry struct to store the Full Address Details : family + port + IP
  struct sockaddr_in addr; // sockaddr_in is a IPv4 specific struct  . Imported from the netinet/in header 
  // You have to zero out the struct so it doesn't have garbage values (can cause bugs)
  memset(&addr,0,sizeof addr); // void *memset(void *ptr,int value,sizeof value)
  
  // Build the network address by filling the details
  addr.sin_family = AF_INET;
  addr.sin_port = htons(2323);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // INADDR_LOOPBACK = "Only this Machine" and it's a constant for 127.0.0.1 , htonl = "host to network, long (32-bit)" IP adress is 32 bits . addr.sin_addr is the IP field and it is in itself a struct 
  // Last line of struct basically means : "Listen only for visitors from inside this machine"
  
  // bind the socket file descriptor to its network address with one thing in mind : bind() takes the generic sockaddr struct pointer(Which accepts IPv4,IPv6,and Unix sockets)
  // so its second paremeter has to be cast to a generic struct : struct sockaddr
  if (bind(listen_fd, (struct sockaddr *)&addr,sizeof addr) == -1){
    perror("Bind!");
    close(listen_fd); // if the socket opens -> bind fails -> return 1 (exit program) -> the socket stays open ! Solution : close the socket in case of failure just like freeing memory :>D ! HABIT : LEARN SAFETY !
    return 1;
  }
  
  // Since this is a listening socket we are going to make it wait for incoming connections !
  // args are : int socket_file_descriptor , int backlog (which is the limit of connections waiting in the queue before you accept() them!)
  if (listen(listen_fd, 4) == -1){
    close(listen_fd); // If listen() failed we make sure the socket is close()d ! .Otherwise , if the program ends without closing the socket , nothing bad happens ! The kernel closes every fd for you , but this is a good habit to learn like free() ! 
    perror("Listen!");
    return 1;
  }
  // getchar(); 
  
  // at this point in the program , the socket_fd is created , binded with a full network addres (address family,Port,IP address),and made it wait for connections ! 
  // We paused the program with getchar(); -> connected to the socket with nc 127.0.0.1 2323 -> the kernel completed the handshake and put the visitor in queue . That's all you can do for now 
  

  
  close(listen_fd);
  return 0;
} 
