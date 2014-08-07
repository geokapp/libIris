// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * libIris - A simple and scalable networking library for Linux.
 *
 * Copyright (C) 2014 Giorgos Kappes <geokapp@gmail.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>               
#include <sys/time.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <errno.h>
#include "libiris.h"

using namespace libiris;

/**
 * @name Endpoint - Constructor.
 *
 * This function initializes an endpoint. The default communication protocol
 * is TCP. To change it use either the correct constructor or the set_protocol
 * method.
 */
Endpoint::Endpoint() {
  m_protocol = TCP; // TCP is the default.
  m_sockets = NULL;
  m_sockets_len = 0;
  m_address_info = NULL;
}

/**
 * @name Endpoint - Constructor.
 * @param proto: Protocol (TCP or UDP).
 * @param type: Type (Client or Server).
 *
 * This function initializes an endpoint. The default communication protocol
 * is TCP. To change it use either this constructor or the set_protocol
 * method.
 */
Endpoint::Endpoint(const Endpoint::Protocol proto, const Endpoint::Type type) {
  m_protocol = proto;
  m_type = type;
  m_sockets = NULL;
  m_sockets_len = 0;
  m_address_info = NULL;
}

/**
 * @name Endpoint - Destructor.
 *
 * This function destroys an endpoint and cleans the allocated memory.
 */
Endpoint::~Endpoint() {
  if (m_sockets)
    free(m_sockets);
  m_sockets = NULL;
  m_sockets_len = 0;
  
  // Safely delete m_addr
  if (m_address_info)
    freeaddrinfo(m_address_info);
  m_address_info  = NULL;
}

/**
 * @name set_protocol - Set the protocol.
 * @param proto: The protocol (TCP or UDP).
 *
 * Use this method to set the communication protocol of an endpoint.
 *
 * @return Void.
 */
void Endpoint::set_protocol(const Endpoint::Protocol proto) {
  m_protocol = proto;
}

/**
 * @name set_type - Set the type.
 * @param type: The endpoint type (CLIENT or SERVER).
 *
 * Use this method to set the type of an endpoint.
 *
 * @return Void.
 */
void Endpoint::set_type(const Endpoint::Type type) {
  m_type = type;
}

/**
 * @name protocol - Get the endpoint protocol.
 *
 * This function returns the endpoint protocol.
 *
 * @return Enpoint protocol.
 */
Endpoint::Protocol Endpoint::protocol() {
  return m_protocol;
}

/**
 * @name type - Get the endpoint type.
 *
 * This function returns the endpoint type.
 *
 * @return Enpoint type.
 */
Endpoint::Type Endpoint::type() {
  return m_type;
}

/**
 * @name sockets - Get the socket descriptor table.
 *
 * This function returns the endpoint's socket descriptor table or NULL.
 *
 * @return Socket descriptors.
 */
int32_t *Endpoint::sockets() {
  return m_sockets;
}

/**
 * @name sockets_len - Get the socket descriptor table length.
 *
 * This function returns the length of the socket descriptor table.
 *
 * @return Table length.
 */
int32_t Endpoint::sockets_len() {
  return m_sockets_len;
}

/**
 * @name address_info - Get the address information.
 *
 * This function returns the address information.
 *
 * @return The address information.
 */
struct addrinfo *Endpoint::address_info() {
  return m_address_info;
}

/*
 * @name send_data - Send data.
 * @param data: Pointer to the data buffer.
 * @param data_len: Size of the data buffer.
 * @param client: The endpoint where the data will be sent. By default this is NULL
 *                and must be used only by the Server side to send data to a client.
 *
 * This function sends data to an endpoint.
 *
 * @return Total number of bytes sent or -1 on error.
 */
int32_t Endpoint::send_data(const void *data, const size_t data_len, Endpoint *client) {
  Endpoint *target;
  int32_t bytes;
  size_t total = 0;
  int32_t bytes_left = data_len;
  int32_t packets = data_len / UDPPACKETSIZE;   
  int32_t packet_left = data_len % UDPPACKETSIZE;    
  
  if (client)
    target = client;
  else
    target = this;
  
  // Handle TCP send.
  if (target->m_protocol == Endpoint::TCP) {
    while (total < data_len) {
      bytes = send(target->m_sockets[0], ((char*)data) + total, bytes_left, 0); 
      if (bytes == -1)  {
	return -1; 
      } 
      total += bytes;
      bytes_left -= bytes;
    }
  } else if (target->m_protocol == Endpoint::UDP) { 
    // We have to split data into packets.
    for (int32_t i = 0; i <= packets; i++) {
      // We have only one packet to send.
      if (packets < 1) {
	bytes = sendto(target->m_sockets[0], data, data_len, 0,
		       target->m_address_info->ai_addr,
		       target->m_address_info->ai_addrlen);
	if (bytes == -1) {
	  return -1; 
	} 
	total = bytes;
      } else {
	// We have got more than one packets
	if (i < packets) {
	  bytes = sendto(target->m_sockets[0], data, UDPPACKETSIZE, 0,
			 target->m_address_info->ai_addr,
			 target->m_address_info->ai_addrlen);
	  if (bytes == -1) {
	    return -1; 
	  } 
	  total += bytes;
	  data = ((char*)data) + UDPPACKETSIZE;
	} else {
	  bytes = sendto(target->m_sockets[0], data, packet_left, 0,
			 target->m_address_info->ai_addr,
			 target->m_address_info->ai_addrlen);
	  if (bytes == -1) {
	    return -1; 
	  } 
	  total += bytes;
	}   
      }
    }
  } else {
    // Unknown endpoint type.
    return -1;
  }
  if (total < 0) 
    return -1;
  else 
    return total;
}

/*
 * @name receive_data - Receive data.
 * @param data: Pointer to the data buffer.
 * @param data_len: Size of the data buffer.
 * @param client: The endpoint from which the data will be recceived. By default
 *                this is NULL and must be used only by the Server side to receive
 *                data from a client.
 *
 * This function receives data from an endpoint.
 *
 * @return Total number of bytes received or -1 on error.
 */
int32_t Endpoint::receive_data(void *data, size_t data_len, Endpoint *client) {
  Endpoint *target;
  int32_t bytes = 0, time;
  size_t total = 0;
  int32_t bytes_left = data_len;

  struct sockaddr_storage client_addr;
  socklen_t client_sin_size;
  char *data_ptr = (char*)data;
  
  // info must be point to a valid endpoint.
  if (client) {
    target = client;
  } else
    target = this;
  
  total = bytes = 0;
  client_sin_size = sizeof(client_addr);
  
  if (target->m_protocol == Endpoint::TCP) {
    while(total < data_len) {
      bytes = recv(target->m_sockets[0], data_ptr, bytes_left, 0);
      if (bytes == -1) {
	return -1; 
      } 
      if (bytes == 0)  
	break; 
      total += bytes;
      if (strstr(data_ptr, "\0")) 
	break;
      data_ptr += bytes;
      bytes_left -= bytes;
    }
    return (total);            
  } else if (target->m_protocol == Endpoint::UDP) {         
    // Check if there are data to read.
    time = receive_timeout(target->m_sockets[0], /*5*/ 0, 0);
    switch (time) {
    case 0:
      // Timeout occured. 
      return 0;
    case -1:
      // An error occured. 
      return -1;
    default:
      // We have a udp connection. Call recvfrom to read data. 
      bytes = recvfrom(target->m_sockets[0], data, data_len,
		       0, (struct sockaddr *)&client_addr, &client_sin_size);
      if (bytes == -1) {
	return -1; 
      } 
      total = bytes;
      return total;
    }
  } else {
    // Unknown endpoint protocol.
    return -1;
  }
}

/**
 * @name receive_timeout - Timeout receive.
 * @param sock: Socket descriptor.
 * @param sec: Seconds.
 * @param usec: Microseconds.
 *
 * This function checks whether there is data to the  endpoint ready to
 * be received. It uses select to wait until the socket descriptor has
 * data.
 *
 * @return The number of socket descriptors.
 */
int32_t Endpoint::receive_timeout(int32_t sock, long sec, long usec) {
  struct timeval timeout;
  
  // Setup timeval variable.
  timeout.tv_sec = sec;
  timeout.tv_usec = usec;
  
  // Setup fd_set structure.
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(sock, &fds);
  
  return select((sock + 1), &fds, 0, 0, &timeout);
}

/**
 * @name cleanup - Clean enpoint.
 *
 * This closes an open connections and properly cleans the endpoint.
 *
 * @return 0 on success, 1 on error.
 */
int32_t Endpoint::cleanup() {
  int32_t result;
  
  // Close sockets of TCP connections.  
  if (m_protocol == Endpoint::TCP) {
    if (m_sockets) {
      for (int32_t i = 0; i < m_sockets_len; i++) { 
	if (m_sockets[i] >= 0)
	  result = close(m_sockets[i]);
	
        // Let the code close up the remaining sockets as well.
      }
    }
  }  
  if (m_address_info)
    freeaddrinfo(m_address_info);
  
  m_address_info = NULL;
  if (result < 0)
    return 1;
  else
    return 0;
}

/**
 * @name set_canon_null - Set canonname to NULL
 * @param head: An addrinfo node.
 *
 * It properly sets the ai_canonname field to NULL on all the nodes of
 * the addrinfo list that begins from head.
 *
 * @return Void.
 */
void Endpoint::set_canon_null(struct addrinfo *head) {
  struct addrinfo *next;
  next = head;
  while (next != NULL) {
    next->ai_canonname = NULL;
    next = next->ai_next;
  }
}

/**
 * @name deleteGAINNode - Delete an addrinfo node.
 * @param head: Points to the head of the addrinfo list.
 * @param res: The node that we whould liek to delete.
 * @param prev: The node before res.
 *
 * It properly deletes a node from an addrinfo list.
 *
 * @return Void.
 */
void Endpoint::deleteGAINode(struct addrinfo **head, struct addrinfo **res,
			     struct addrinfo *prev) {
  struct addrinfo *del = *res;
  
  if (del) {
    if (*head == *res)
      *head = (*res)->ai_next;
    if (prev)
      prev->ai_next = (*res)->ai_next;
    *res = (*res)->ai_next;
    if (del->ai_canonname)
      free(del->ai_canonname);
    del->ai_canonname = NULL;
    free(del);
    del = NULL;
  }
}

/**
 * Client - Constructor.
 *
 * Initializes a client object.
 */
Client::Client() {
  m_protocol = Endpoint::TCP;        // TCP is the default.
  m_type = Endpoint::ClientEndpoint;

  // Allocate memory for the socket descriptor. 
  m_sockets = (int32_t *) malloc(sizeof(int32_t));
  m_sockets[0] = UNUSED;
  m_sockets_len = 1;
  m_address_info = NULL;
}

/**
 * Client - Constructor.
 * @param proto: The Endpoint protocol (TCP or UDP).
 * Initializes a client object.
 */
Client::Client(const Endpoint::Protocol proto) {
  m_protocol = proto;
  m_type = Endpoint::ClientEndpoint;

  // allocate memory for the socket descriptor. 
  m_sockets = (int32_t *) malloc(sizeof(int32_t));
  m_sockets[0] = UNUSED;
  m_sockets_len = 1;
  m_address_info = NULL;
}

/**
 * Client - Destructor.
 *
 * Desrtoys a client object.
 */
Client::~Client() {
  if (m_sockets)
    free(m_sockets);
  m_sockets = NULL;
  m_sockets_len = 0;
  
  // Safely delete m_addr
  if (m_address_info)
    freeaddrinfo(m_address_info);
  m_address_info  = NULL;
}

/**
 * @name set_sockets - Set the socket.
 * @param sock: Socket descriptor.
 *
 * This function sets the socket descriptor.
 *
 * @return Void.
 */
void Client::set_socket(int32_t sock) {
  if (!m_sockets)
    m_sockets = (int32_t *) malloc(sizeof(int32_t));
  
  m_sockets[0] = sock;
}

/**
 * @name get_socket - Get the socket.
 *
 * This function returns the socket descriptor.
 *
 * @return The socket descriptor or UNUSED on error.
 */
int32_t Client::get_socket() {
  if (!m_sockets)
    return m_sockets[0];
  else
    return UNUSED;
}

/**
 * @name set_address_info - Set the address information.
 * @param info: The address ifnormation.
 *
 * This function sets the address information.
 *
 * @return Void.
 */
void Client::set_address_info(struct addrinfo *info) {
  if (m_address_info)
    freeaddrinfo(m_address_info);
  m_address_info = info;
}

/**
 * attach - Connects to a server host.
 * @param host: The hostname or ip address of the server host.
 * @param service: the port number of the service.
 *
 * This function creates a new connection with a server on the specified
 * port. 
 *
 * @return 0: success, 1: error.
 *    
 */
int32_t Client::attach(const char *host, const char *service) {
  struct addrinfo hints, *res;
  int32_t connected = 0;
  
  // Check the arguments.
  if (!host) {
    fprintf(stderr, "(attach) Error: host should not be NULL on a Client Endpoint.\n");
    return 1;
  }
  
  if (!service) {
    fprintf(stderr, "(attach) Error: service should not be NULL.\n");
    return 1;
  }
  
  // Specify the hints for the getaddrinfo().
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;       // For both ipv4 and ipv6 protocols.
  
  if (m_protocol == Endpoint::TCP)
    hints.ai_socktype = SOCK_STREAM; // For a TCP server socket.
  else 
    hints.ai_socktype = SOCK_DGRAM;  // For a UDP server socket.
  
  // Call the getaddrinfo.
  if ((getaddrinfo(host, service, &hints, &(this->m_address_info))) < 0) {
    fprintf(stderr, "(attach) Error: getaddrinfo failed.\n");
    return 1;
  }    
  
  // Create the UDP/ TCP sockets and call connect for each one. 
  res = m_address_info;
  while (res) {
    if (m_protocol == Endpoint::UDP || m_protocol == Endpoint::TCP)  
      m_sockets[0] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    if (m_sockets[0] < 0) {
      deleteGAINode(&(m_address_info), &res, NULL);
      continue;
    }
    
    // Now try to connect if the protocol is TCP.
    if (m_protocol == TCP) {
      if (connect(m_sockets[0], res->ai_addr, res->ai_addrlen) < 0) {
	// Can not connect in this address.
	close(m_sockets[0]);
	deleteGAINode(&(m_address_info), &res, NULL);
	continue;   
      }
    }
    connected = 1;
    break;  
  }
  if (connected)
    return 0;
  else {
    fprintf(stderr, "(attach) error: Can not connect  to '%s' for the service '%s'\n",
	    host,service);
    if (m_address_info) {
      freeaddrinfo(m_address_info);
      m_address_info = NULL;
    }
    return 1;
  }
}

/**
 * @name detah - Close connection
 *
 * This method is used to close and cleanup a client endpoint.
 * @return Void.
 */
int32_t Client::detach() {
  this->cleanup();
}

/**
 * Server - Constructor.
 *
 * Initializes a client object.
 */
Server::Server() {
  m_protocol = Endpoint::TCP; // TCP is the default.
  m_type = Endpoint::ServerEndpoint;
  m_epfd = UNUSED;
  m_sockets = NULL;
  m_server_address = NULL;
}

/**
 * Server - Constructor.
 * @param proto: The Endpoint protocol (TCP or UDP).
 *
 * Initializes a server object.
 */
Server ::Server(const Endpoint::Protocol proto) {
  m_protocol = proto;
  m_type = Endpoint::ServerEndpoint;
  m_epfd = UNUSED;
  m_sockets = NULL;
  m_server_address = NULL;
}

/**
 * Server - Destructor.
 *
 * Desrtoys a server object.
 */
Server::~Server() {
  if (m_sockets)
    free(m_sockets);
  m_sockets = NULL;
  m_sockets_len = 0;
  
  // Safely delete m_addr
  if (m_address_info)
    freeaddrinfo(m_address_info);
  m_address_info = NULL;

  if (m_server_address)
    free(m_server_address);
  m_server_address = NULL;
}

/**
 * @name start - Create a server endpoint.
 * @param host: the hostname or ip address of the server host.
 * @param service: the port number of the service.
 * @param backlog: The size of the backlog (for TCP only).
 *
 * This function creates a server endpoint for communication. It adds the created
 * socket descriptors into a new epoll set.
 *
 * @return 0: success, 1: error.
 */
int32_t Server::start(const char *host, const char *service, int32_t backlog) {
  struct addrinfo hints, *res, *prev;
  int32_t len, i = 0, created = 0, error, epres;
  struct epoll_event ev;
  struct address_storage *server_address;
  
  // Set epoll events.
  ev.events  = EPOLLIN;
  
  // Service should not be NULL. 
  if (!service) {
    fprintf(stderr, "(start) Error: service should not be NULL on a Server Endpoint.\n");
    return 1;
  }
  
  // Check the backlog for a TCP server socket. 
  if (m_protocol == Endpoint::TCP && backlog <= 0) {
    fprintf(stderr, "(start) Error: backlog must be a positive value for TCP.\n");
    return 1;
  }
  m_backlog = backlog;
  
  // Specify the hints for the getaddrinfo().
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;       // For both ipv4 and ipv6 protocols
  if (m_protocol == Endpoint::TCP)
    hints.ai_socktype = SOCK_STREAM; // For a TCP server socket
  else 
    hints.ai_socktype = SOCK_DGRAM;  // For a UDP server socket
  
  //
  // The endpoint is a server. If host is NULL, then set it to
  // INADDR_ANY or IN6ADDR_ANY_INIT.
  //
  hints.ai_flags = AI_ADDRCONFIG|AI_PASSIVE;
  
  // Call the getaddrinfo.
  if ((getaddrinfo(host, service, &hints, &(m_address_info)) < 0)) {
    fprintf(stderr, "(start) Error: getaddrinfo error.\n");
    return 1;
  }
  set_canon_null(m_address_info);
  
  // Allocate memory for socket descriptor. 
  len = 0;
  for (res = m_address_info; res; res = res->ai_next) 
    len++;
  if(!(m_sockets = (int32_t *)malloc(len * sizeof(int32_t)))) {
    fprintf(stderr, "(start) Error: No free memory left.\n");
    return 1;
  }
  m_sockets_len = 0;
  res = m_address_info;
  prev = res;
  
  // Create the epoll descriptor. 
  m_epfd = epoll_create(EPOLL_QUEUE_LEN);
  
  // Create the TCP/UDP sockets.        
  while (res) {
    m_sockets[i] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    if (m_sockets[i] < 0) {
      deleteGAINode(&(m_address_info), &res, prev); 
      continue;
    }
    
    // Now bind the socket.
    if (bind(m_sockets[i], res->ai_addr, res->ai_addrlen) < 0) {
      close(m_sockets[i]);
      deleteGAINode(&(m_address_info), &res, prev);          
      continue;
    }
    
    // Check if we use a TCP socket and call listen.
    if (m_protocol == Endpoint::TCP) {
      if (listen(m_sockets[i], m_backlog) < 0) {
	close(m_sockets[i]);
	deleteGAINode(&(m_address_info), &res, prev);              
	continue;
      }
    }
    
    created = 1; 
    m_sockets_len++;
    i++; // Create socket for the next address if there is one. 
    prev = res;
    res = res->ai_next;      
  }
  
  if (created) {
    created = 0;
    server_address = (struct address_storage *)malloc(m_sockets_len * sizeof(address_storage));
    if (!server_address) {
      fprintf(stderr, "(start) ERROR: malloc: No free memory left.\n");
      return 1;
    }
    // Keep a pointer in order to free the memory later.
    m_server_address = server_address;
    
    for (i = 0; i < m_sockets_len; i++) {
      // Add the created socket to epoll for monitor. 
      server_address[i].fd   = m_sockets[i];
      server_address[i].size = 0;
      server_address[i].addr = NULL;
      ev.data.ptr = (void *)&server_address[i];
      epres = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_sockets[i], &ev);
      error = errno;
      if (epres < 0 ) {
	continue;
      }   
      created = 1;      
    }
    
    // If at least one server socket added to the epoll set we are ok.
    if (created) {
      return 0;
    } else {
      return 1;
    }
  } else {
    fprintf(stderr,"(start) Error: Can not create a server to '%s' for the service '%s'.\n", host,service);
    return 1;
  }
}

/*
 * @name get_client - Returns the next ready client.
 * @param client: A pointer to a validclient object.
 *
 * This function waits for events in an epoll file descriptor and returns a
 * client that is ready to send data. It also attaches the client to the server
 * endpoint. Note, that after receiving data from the client, the server side 
 * must properly detach the client by calling its detach function.
 *
 * Return value: 0: success, 1: error.
 *    
 */
int32_t Server::get_client(Client *client) {
  struct address_storage *client_address, *client_address_ptr, *server_address_ptr;
  struct addrinfo *res;
  int32_t i, fd, j, nfds, error;
  char data[200];
  int32_t data_len = sizeof(data);
  int32_t bytes, epres, accept_sd, new_client_done = 0;
  struct epoll_event ev, events[MAX_EPOLL_EVENTS_PER_RUN];
  
  struct sockaddr_storage *client_addr;
  socklen_t client_sin_size;

  // Check the arguments. Client must point to a valid client object. 
  if (!client) {
    fprintf(stderr, "(get_client) Error: Client must not be NULL.\n");
    return 1;
  }
  
  // Set the epoll events. 
  ev.events = EPOLLIN;
  
  client_addr = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
  if (!client_addr) {
    fprintf(stderr,"(get_client) Error: No free memory left.\n");
    return 1;
  }
  memset(client_addr,0,sizeof(struct sockaddr_storage));
  client_sin_size = sizeof(struct sockaddr_storage);
  
  while (1) {   
    // Call epoll wait.
    nfds = epoll_wait(m_epfd, events, MAX_EPOLL_EVENTS_PER_RUN, -1);
    error = errno;
    if (nfds < 0) {
      if (error == EINTR) {
	if (client_addr)
	  free(client_addr);
	return 1;
      } else {
	if (client_addr)
	  free(client_addr);
	return 1;
      }
    }
    
    // Check the event.
    for (i = 0; i < nfds; i++) {
      server_address_ptr = (struct address_storage *)events[i].data.ptr;
      fd = server_address_ptr->fd;
      
      // check if a new client has come. 
      new_client_done = 0;
      for (j = 0; j < m_sockets_len; j++) {
	// If the event is on a server socket.
	if (fd == m_sockets[j]) {
	  if (m_protocol == Endpoint::TCP) {
	    // Call accept and save the new socket descriptor.
	    accept_sd = accept(m_sockets[j],(struct sockaddr *)client_addr,
			       &client_sin_size);
	    if (accept_sd < 0) {
	      continue;
	    }
            
	    // 
	    // We have to keep the client's address 
	    // to do this we 'll use address_storage. 
	    //
	    client_address = (struct address_storage *) malloc(sizeof(struct address_storage));
	    if (!client_address) {
	      fprintf(stderr,"(get_client) Error: No free memory left.\n");
	      if (client_addr)
		free(client_addr);
	      return 1;
	    }
	    client_address->fd = accept_sd;
	    client_address->size = client_sin_size;
	    client_address->addr = client_addr;
	    
	    ev.data.ptr = (void *)client_address;
	    client_address = NULL;
            
	    // Add accepted socket to epoll. 
	    epres = epoll_ctl(m_epfd, EPOLL_CTL_ADD, accept_sd, &ev);
	    if (epres < 0) {
	      continue;
	    } 
	    new_client_done = 1;
	    break;
	  } else {
	    
	    // We have a UDP Endpoint. lets block and wait for data.
	    bytes = recvfrom(m_sockets[j], data, data_len, MSG_PEEK,
			     (struct sockaddr *)client_addr, &client_sin_size);
	    if (bytes < 0) {
	      continue;
	    }

	    // a client with data has come. Initialize the node. 
	    res = (struct addrinfo *)malloc(sizeof(struct addrinfo));
	    if (!res) {
	      fprintf(stderr,"(get_client) Error: No free memory left.\n");
	      if (client_addr)
		free(client_addr);
	      return 1;
	    }                     
	    res->ai_next = NULL;
	    res->ai_addr =(struct sockaddr *)client_addr;             
	    res->ai_addrlen = client_sin_size;
	    res->ai_canonname = NULL;
	    client->set_address_info(res);
	    return 0;
	  }
	}			                   
      }            
      if ((events[i].events & EPOLLIN) && new_client_done == 0) {    
	if (m_protocol == Endpoint::TCP) {
	  
	  // Initialize the node. 
	  client_address_ptr =  (struct address_storage *)events[i].data.ptr;
	  client->set_socket(client_address_ptr->fd);
          
	  res = (struct addrinfo *)malloc(sizeof(struct addrinfo));
	  if (!res) {
	    fprintf(stderr,"(get_client) Error No free memory left.\n");
	    if (client_addr)
	      free(client_addr);
	    if (client_address)
	      free(client_address);
	    return 1;
	  }                    
	  res->ai_next = NULL;
	  if (events[i].data.ptr)
	    res->ai_addr = (struct sockaddr *)(client_address_ptr->addr);
	  if (events[i].data.ptr!= NULL) 
	    res->ai_addrlen = (socklen_t)client_address_ptr->size;
	  res->ai_canonname = NULL;
	  client->set_address_info(res);
          
	  // Delete descriptor from epoll.
	  epoll_ctl(m_epfd, EPOLL_CTL_DEL, client->get_socket(), &ev);
	  return 0;
	}
      } else if (!new_client_done) {
	if (m_protocol == Endpoint::TCP) {
	  client_address_ptr = (struct address_storage *)(events[i].data.ptr);
	  fd = client_address_ptr->fd;
	  // Delete descriptor from epoll.
	  epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &ev);     
	}
      }
    }                   
  }
}

/**
 * @name stop - Stop the server endpoint.
 *
 * This method stops the server endpoint by closing all the remaining sockets
 * that are still open and by poperly cleaning the used memory.
 *
 * @return 0 on success, 1 on error
 */
int32_t Server::stop() {
  //
  // Close the epoll descriptor and then call the general cleanup
  // method from the base Endpoint object.
  //
  if (m_epfd != UNUSED) {
    if ((close(m_epfd)) < 0){
      return 1;
    }
  }
  return this->cleanup();
};
