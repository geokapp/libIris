// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include "../src/libiris.h"

using namespace libiris;

int main(int argc, char *argv[]) {
  
  char s_addr[INET6_ADDRSTRLEN];
  char data[20] = "Hello World!\0";
  
  int status; 
  
  Client client;
  
  status = client.attach("localhost", "9999");    
  if (status) {
    std::cout << "(Client) Error connecting with the server.\n";
  } else {
    status = client.send_data(data, strlen(data), NULL);
  }
  std::cout << "(Client) Detaching...\n";
  client.detach();
 
  return 0;
}
