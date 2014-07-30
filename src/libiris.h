// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * libIris - A simple and scalable networking library for Linux.
 *
 * Copyright (C) 2013-2014 Giorgos Kappes <geokapp@gmail.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */
 
#ifndef LIBIRIS_H
#define LIBIRIS_H

#include <stdint.h>
#include <stdlib.h>

namespace libiris {

#define TIMEOUT                  5
#define UDPPACKETSIZE            1400
#define EPOLL_QUEUE_LEN          1000
#define MAX_EPOLL_EVENTS_PER_RUN 1000
#define EPOLL_RUN_TIMEOUT	 -1
#define UNUSED                   -999

/** 
 * @name Endpoint - The endpoint object.
 *
 * This class defines the base libiris endpoint object. This object holds general
 * information about a network endpoint including the communication protocol, the
 * type i.e. client or server, the table of socket descriptors, the size of this
 * table, as well as a pointer to an addrinfo structure.
 */
class Endpoint {
 public:
  enum Protocol{
    TCP, 
    UDP
  };  
  enum Type {
    ServerEndpoint,
    ClientEndpoint,
    Unused
  };
  
 protected:
  Protocol m_protocol;
  Type m_type;
  int32_t *m_sockets;
  int32_t m_sockets_len;
  struct addrinfo *m_address_info;
  
 public:
  Endpoint();
  Endpoint(const Protocol proto, const Type type);
  virtual ~Endpoint();

  void set_protocol(const Protocol proto);
  void set_type(const Type type);
  
  Protocol protocol();
  Type type();

  int32_t send_data(const void *data, const size_t data_len,
		    Endpoint *client = NULL);
  int32_t receive_data(void *data, size_t data_len,
		       Endpoint *clientInfo = NULL);

 protected:
  int32_t *sockets();
  int32_t sockets_len();
  struct addrinfo *address_info();
  int32_t receive_timeout(int32_t sock, long sec, long usec);
  void set_canon_null(struct addrinfo *head);
  void deleteGAINode(struct addrinfo **head, struct addrinfo **res,
		     struct addrinfo *prev);
  int32_t cleanup();
};

/** 
 * @name Client - The Client endpoint object.
 *
 * This class defines the client endpoint object. A client side application can use
 * this object to exchange data with a server. For example:
 * ------------------------------------
 * Client *client = new Client;
 * int status = client->attach(server_ip, port);
 * if (!status) {
 *   ...
 *   client->detach();
 * }
 * delete client;
 * ------------------------------------
 * A server side application can use this object to communicate with a client. For
 * example:
 * ------------------------------------
 * Client *client = new client;
 * int status = server->get_client(client);
 * if (status) {
 *   ...
 *   client->detach();
 * }
 * delete client;
 * ------------------------------------
 */
class Client : public Endpoint {
 public:
  Client();
  Client(const Endpoint::Protocol proto);
  ~Client();
  
  int32_t attach(const char *host, const char *service);
  int32_t detach();

  void set_socket(int32_t sock);
  void set_address_info(struct addrinfo *info);  

  int32_t get_socket();
};

/** 
 * @name Server - The Server endpoint object.
 *
 * This class defines the server endpoint object. A server  side application can use
 * this object to wait for clients and exchange data with them. For example:
 * ------------------------------------
 * Server *server = new server;
 * status = server->start(NULL, 8000, 10);
 * if (!status) {
 *   while(run) {
 *     Client *client = new Client;
 *     status = server->get_client(client);
 *     if (!status) {
 *       server->receive_data(buf, buf_len, client); 
 *       status = client->detach();
 *     }
 *     delete client;
 *  }
 *  server->stop();
 * }
 * ------------------------------------
 */
class Server : public Endpoint {
 public:
  /**
   * adress_storage - Client Information holder.
   *
   * The address_storage struct is used to keep the client's information
   * in the epoll queue.  
   */                                                 
  struct address_storage {
    int32_t fd;
    int32_t size;
    struct sockaddr_storage *addr;
  };

 private:
  int32_t m_backlog;
  int32_t m_epfd;
  struct address_storage *m_server_address;
  
 public:
  Server();
  Server(const Endpoint::Protocol proto);
  ~Server();

  int32_t start(const char *host, const char *service, int32_t backlog);
  int32_t stop();
  int32_t get_client(Client *client);
  
};

} // End of namespace

#endif
 


  
