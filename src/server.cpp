#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <map>
#include <sstream>
#include <fstream>

#define BUFFER_SIZE 1024

struct HTTPResponse{
  std::string status;
  std::string content_type;
  std::map<std::string, std::string> headers;
  std::string body;

  std::string to_string(){
    std::string response;
    response += status + "\r\n";
    response += "Content-Type: " + content_type + "\r\n";
    for (const auto& header : headers) {
      response += header.first + ": " + header.second + "\r\n";
    }
    response += "\r\n";
    response += body;
    return response;
  }
};

struct HTTPRequest {
  std::string method;
  std::string path;
  std::string version;
  std::map<std::string, std::string> headers;
  std::string body;
};

HTTPRequest pase_request(std::string request) {
  HTTPRequest req;
  std::stringstream ss(request);

  std::string line;
  std::getline(ss, line);
  std::istringstream line_ss(line);
  line_ss >> req.method >> req.path >> req.version;

  while (std::getline(ss, line) && !line.empty()) {
    size_t pos = line.find(":");
    if (pos != std::string::npos) {
      std::string header_name = line.substr(0, pos);
      std::string header_value = line.substr(pos + 2);
      header_value.erase(header_value.end() - 1);
      req.headers[header_name] = header_value;
    }
  }

  std::stringstream body_ss;
  std::getline(ss, line);
  body_ss << line;
  while (std::getline(ss, line)) {
    body_ss << "\n" << line;
  }
  req.body = body_ss.str();

  return req;
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string dir;

  if (argc == 3 && strcmp(argv[1], "--directory") == 0) {
  	dir = argv[2];
  }

  
  
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

  while(1){

    // accepterar connection request av en client socket och fyller i client_addr med dess uppgifter.
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";

    // ta emot meddelande från client socket.
    char buffer[BUFFER_SIZE];
    if(recv(client_fd, buffer, sizeof(buffer), 0) < 0) {
      std::cerr << "Failed to receive message from client\n";
      close(client_fd);
      close(server_fd);
      return 1;
    }
    std::cout << "Message received\n";
    std::string message(buffer);
    HTTPRequest request = pase_request(message);

    // write a response
    std::string res;
    if(request.method == "GET") {
      if(request.path == "/"){
        HTTPResponse response = { "HTTP/1.1 200 OK", "text/plain", {}, "" };
        res = response.to_string();
      }
      else if(request.path.substr(0, 6) == "/echo/"){
        std::string subStr = request.path.substr(6);
        HTTPResponse response = { "HTTP/1.1 200 OK", "text/plain", { {"Content-Length", std::to_string(subStr.length())} }, subStr };
        res = response.to_string();
      }
      else if(request.path == "/user-agent"){
        std::string body = request.headers["User-Agent"];
        HTTPResponse response = { "HTTP/1.1 200 OK", "text/plain", { {"Content-Length", std::to_string(body.length())} }, body };
        res = response.to_string();
      }
      else if(request.path.substr(0,7) == "/files/"){
        std::string fileName = request.path.substr(7);
        std::ifstream ifs(dir + fileName);

        if(ifs.good()) {
          std::stringstream content;
          content << ifs.rdbuf();
          HTTPResponse response = { "HTTP/1.1 200 OK", "application/octet-stream", { {"Content-Length", std::to_string(content.str().length())} }, content.str() };
          res = response.to_string();
        }
        else {
          HTTPResponse response = { "HTTP/1.1 404 Not Found", "text/plain", {}, "Not Found" };
          res = response.to_string();
        }
      }
      else {
        HTTPResponse response = { "HTTP/1.1 404 Not Found", "text/plain", {}, "Not Found" };
        res = response.to_string();
      }
    } 
    else if(request.method == "POST") {
      if(request.path.substr(0,7) == "/files/"){
        std::string fileName = request.path.substr(7);
        std::ofstream ofs(dir + fileName);

        if(ofs.good()) {
          ofs << request.body.substr(0,stoi(request.headers["Content-Length"]));
          HTTPResponse response = { "HTTP/1.1 201 Created", "text/plain", {}, "Created" };
          res = response.to_string();
        }
        else {
          HTTPResponse response = { "HTTP/1.1 404 Not Found", "text/plain", {}, "Not Found" };
          res = response.to_string();
        }
      }
      else {
        HTTPResponse response = { "HTTP/1.1 404 Not Found", "text/plain", {}, "Not Found" };
        res = response.to_string();
      }
    }
    else {
      HTTPResponse response = { "HTTP/1.1 404 Not Found", "text/plain", {}, "Not Found" };
      res = response.to_string();
    }
    
    // send the response
    send(client_fd, res.c_str(), res.size(), 0);
    std::cout << "Message sent\n";
  }

  // stänger server socket
  close(server_fd);

  return 0;
}
