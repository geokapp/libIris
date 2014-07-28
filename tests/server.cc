// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../src/libiris.h"

using namespace std;

int main(int argc, char *argv[]) {
  char s_addr[INET6_ADDRSTRLEN];
  char data[100];
  
  int status;
  Server server;

  status = server.start(NULL, "9999", 10);
  if (status) {
    cout << "(Server) Error on startup.\n";
    return 1;
  } else {
    cout << "(Server) Up and running!\n";
  }
  memset(data, 0, 100);

  //
  // Wait for incoming requests 
  //
  while(1) {
    Client *client = new Client;
    status = server.get_client(client);
    if (status ) {
      cout << "(Server) get_client error.\n";
      delete client;
      break;
    }
    //
    // A client reached. Print its data. 
    //
    cout << "(Server) Client reached.\n";
    status = server.receive_data(data, 100, client);
    cout << data << endl;
    client->detach();
    delete client;
  }
  cout << "(Server) Stopping...\n";
  status = server.stop();
  if (status) {
    cout << "(Server) Error on stop.\n";
    return 1;
  } else {
    cout << "(Server) Stopped!\n";
  }
}
