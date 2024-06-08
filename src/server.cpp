#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // socket() system call skapar en socket. returnerar en small integer.
  // första parametern bestämmer vilken communication family som används. I detta fall specificerar AF_INET the IPv4 protocol family.
  // andra parametern bestämmer vilken socket type som används. I detta fall specificerar SOCK_STREAM a TCP type socket.
  // tredje parametern bestämmer vilket protocol i den bestämda familjen som används. Vanligtvis 0 då communication families endast har ett protocol.
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  // setsockopt() funktionen sätter ett option till vår socket på önskad nivå.
  // första parametern bestämmer vilken socket som påverkas.
  // andra parametern bestämer på vilken nivå (level) the option resides. I detta fall ligger SO_RESUEADDR socket level (SOL_SOCKET).
  // tredje parametern är vilken option som används. SO_REUSEADDR tillåter återanvändandet av locala adresser givna i bind(). Valet tar en int value och är en Boolean option.
  // fjärde parametern tar option value. I detta fall en int som representerar en Boolean. option_value är en pointer.
  // femte parametern tar storleken av option_value.
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  

  // struct for handling internet addresses.
  struct sockaddr_in server_addr;
  // sätt communication family
  server_addr.sin_family = AF_INET;
  // binder inte din socket till en specifik ip-adress.
  server_addr.sin_addr.s_addr = INADDR_ANY;
  // sätter porten som används.
  server_addr.sin_port = htons(4221);
  
  // binder sockaddr till bestämd socket.
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  // vår socket väntar på connections. connection_backlog är antalet connection vår socket tillåter i kön. listen() parametern för backlog tar en int 0-5.
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  // accepterar connection request av en client socket och fyller i client_addr med dess uppgifter.
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";

  // ta emot meddelande från client socket.
  std::string buffer(1024, '\0');
  if(recv(client_fd, (void *)&buffer[0], sizeof(buffer), 0) < 0) {
    std::cerr << "Failed to receive message from client\n";
    return 1;
  }
  std::cout << "Message received\n";

  // write a response
  std::string response;
  if(buffer.rfind("GET / HTTP/1.1\r\n", 0) == 0){
    response = "HTTP/1.1 200 OK\r\n\r\n";
  }
  else {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }
 
  // send the response
  send(client_fd, (void *)&response[0], sizeof(response), 0);
  std::cout << "Message sent\n";

  // stänger server socket
  close(server_fd);

  return 0;
}
