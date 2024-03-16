#include "gtest/gtest.h"
#include "../src/server_client.h"

TEST(ServerTest, AcceptConnection) {
  int server_sock = socket(AF_INET, SOCK_STREAM, 0);

  // Bind and listen on server socket

  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // Connect client socket

    accept_connection(server_sock);

    // Validate client socket was accepted and processed

    close(client_sock);
    exit(0);
  } else {
    // Parent process
    wait(NULL);
    close(server_sock);
  }
}

TEST(ServerTest, ClientDisconnect) {
  int server_sock = socket(AF_INET, SOCK_STREAM, 0);

  // Bind and listen on server socket

  pid_t pid = fork();
  if (pid == 0) {  
    // Child process
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // Connect client socket

    accept_connection(server_sock);

    // Disconnect client socket

    exit(0);
  } else {
    // Parent process
    wait(NULL);
    close(server_sock);
  }
}

TEST(ServerTest, InvalidRequest) {
  int server_sock = socket(AF_INET, SOCK_STREAM, 0);

  // Bind and listen on server socket

  pid_t pid = fork();
  if (pid == 0) {
    // Child process  
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // Connect client socket
    // Send invalid request

    accept_connection(server_sock);

    // Validate invalid request handling

    close(client_sock);
    exit(0);
  } else {
    // Parent process
    wait(NULL);
    close(server_sock);
  }
}
