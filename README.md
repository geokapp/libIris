libIris - A simple and scalable networking library for Linux
============================================================

LibIris is a C++ library whose goal is to provide a unified platform for networking
applications development. The provided API provides methods for creating, connecting, 
and destroying network endpoints, as well as for receiving connection requests and for
data exchange.
LibIris uses the epoll mechanism to achieve scalable event multiplexing.


Building and installing libIris
---------------------------------

To build libIris just type:
   `make`

To build a development version of libIris with support for debugging 
symbols type:
   `make dev`

To build only the tests type:
   `make tests`

To install the libIris library, run the following as root:
   `make install`
	

Removing libIris
------------------

To remove the libIris library type as root:
   `make remove`


Testing libIris
-----------------

We have included several unit tests which check whether the libIris library
works correctly. Check the `tests` folder for details.


Supported protocols
-------------------

The library supports the TCP and UDP communication protocols. The default protocol is TCP
however the user can change this by calling the method `set_ptotocol()` into an endpoint 
object. For example: :

`server->set_protocol(Endpoint::UDP);`

Additionally, both IPv4 and IPv6 are supported by libIris.


Using libIris
---------------

To use the libIris library you need to include `#include <libiris/libiris.h`
to your C++ source file. Then, providing you have installed libiris into your 
system, you can build your application as follows:
   
   `$(CC) -o application_binary application.cc -liris`

LibIris provides a general network endpoint class. This class is holds general information 
about a network endpoint including the communication protocol, the type i.e. client or server, 
the table of socket descriptors, the size of this table, as well as a pointer to an addrinfo 
structure. It is further categorized into a Client and a Server class.

The Client class defines the client endpoint object. A client side application can use this 
object to exchange data with a server. For example:
  
  ```C
  Client *client = new Client;
  int status = client->attach(server_ip, port);
  if (!status) {
    ...
    client->detach();
  }
  delete client;
  ```
  A server side application can use this object to communicate with a client. For
  example:
 
  ```C
  Client *client = new client;
  int status = server->get_client(client);
  if (status) {
    ...
    client->detach();
  }
  delete client;
  ```
 The Server class defines the server endpoint object. A server  side application can use
 this object to wait for clients and exchange data with them. For example:
  
  ```C
  Server *server = new server;
  status = server->start(NULL, 8000, 10);
  if (!status) {
    while(run) {
      Client *client = new Client;
      status = server->get_client(client);
      if (!status) {
        server->receive_data(buf, buf_len, client); 
        status = client->detach();
      }
      delete client;
   }
   server->stop();
   ```


Development and Contributing
----------------------------

The initial developer of libIris is [Giorgos Kappes](http://cs.uoi.gr/~gkappes). Feel free to 
contribute to the development of libIris by cloning the repository: 
`git clone https://github.com/geokapp/libiris`.
You can also send feedback, new feature requests, and bug reports to 
<geokapp@gmail.com>.

