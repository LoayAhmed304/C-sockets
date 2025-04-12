# Socket Communication in C
This project demonstrates network programming in C using both TCP and UDP socket communication.

> The code contained within this repo was compiled on a Linux PC using Gnu’s gcc compiler. It should,
however, build on just about any platform that uses gcc. Naturally, this doesn’t apply if you’re programming
for Windows

## Reference
This repo was made with the help of THE amazing [Beej's Guide to Network Programming - Using Internet Sockets](https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf) book.

# Notes
### Streams Types
- SOCK_STREAM (TCP)
- SOCK_DGRAM (UDP)
>Applications use UDP to transfer files, for example the TFTP (trivial file transfer protocol - or just FTP), without worrying about the fact that UDP is not safe, how?
>Because they build their own protocol on top of UDP, which makes the receiver sends back a packet of “I got it!” (an “ACK” packet), and if the sender got no reply in say 5 seconds, he'll re-send the packet, until he finally gets an ACK.

#### The layered model with Unix
- Application Layer (telnet, ftp, etc.) 
- Host-to-Host Transport Layer (TCP, UDP) 
- Internet Layer (IP and routing)
- Network Access Layer (Ethernet, wi-fi, or whatever)
> But for stream sockets, all we have to do is `send()` the data out, and for datagram sockets is to encapsulate the packet in the method of your choosing and `sendto()` it. The Kernel builds the Transport Layer and Internet Layer, and the hardware takes care of the Network Access Layer.

#### IPv4 vs IPv6
> IPv4's equivalent to `127.0.0.1` (localhost), is IPv6 `::1` (where `::` means leading zeros)

### Structs
#### **addrinfo**
>It comes with the `<netdb.h>` library

We're getting to know what everything in it does/stores. Keep in mind that we don't fill it ourselves, it just stores the address info of a specific web server (or website) to use later to create a socket between us and that web server. The initialization of it is automatically by passing getaddrinfo("example.com") data into it, and it fills everything.
We only select the connection type (TCP or UDP) in ai_sockettype field
``` C
struct addrinfo{
	int ai_flags;              // AI_PASSIVE, AI_CANONNAME, etc. 
	int ai_family;             // AF_INET, AF_INET6, AF_UNSPEC (IPv4 or IPv6)
	int ai_socktype;           // SOCK_STREAM, SOCK_DGRAM
	int ai_protocol;           // use 0 for "any"
	size_t ai_addrlen;         // size of ai_addr in bytes
	struct sockaddr *ai_addr;  // struct sockaddr_in or _in6
	char *ai_canonname;        // full canonical hostname
	struct addrinfo *ai_next;  // linked list, next node
};
```
As said, we'll use this struct often to just retrieve the data from it, as it'll be set almost automatically.

#### sockaddr
> Comes from `sys/socket.h` in linux, and `winsock2.h` in widnows librarys

It stores the actual IP and port used in a socket, unlick addrinfo which stores general info (or hints) of what kind of connection we want.
Why not store the address just in addrinfo? Because there's two types of addresses IPv4 or IPv6 (or in sockaddr words, `sockaddr_in` or `sockaddr_in6`)
To deal with struct sockaddr, programmers created a parallel structure: `struct sockaddr_in` (“in” for “Internet”) to be used with IPv4. And this is the important bit: a pointer to a struct sockaddr_in can be cast to a pointer to a struct sockaddr and vice-versa. So even though connect() wants a struct sockaddr*, you can still use a struct sockaddr_in and cast it at the last minute!
``` C
struct sockaddr {
	unsigned short sa_family; // address family, AF_INET (IPv4) or AF_INET6 (IPv6)
	char sa_data[14];         // contains destination address and port number (14b)
};

//   IPv4 only
struct sockaddr_in {
	short int sin_family;         // address family, AF_INET
	unsigned short int sin_port; // Port number(must be network byte order htons())
	struct in_addr sin_addr;      // Just the address of the socket
	unsigned char sin_zero[8];    // should be set to all zeros using memset()
};

// Whats in_addr struct thats used in sin_addr??
// Literally just a dumb struct made for historical reasons
struct in_addr{
	uint32_t s_addr;           // a 32-bit int (4 bytes)
};
```
What about IPv6?
``` C
struct sockaddr_in6{
	u_int16_t sin6_family;       // Address family, AF_INET6 (always here)
	u_int16_t sin6_port;         // Port number, network byte order
	u_int16_t sin6_flowinfo;     // IPv6 flow information
	struct in6_addr sin6_addr;   // IPv6 address
	u_int32_t sin6_scope_id;     // Scope ID
}

struct in6_addr{
	unsigned char s6_addr[16];   // IPv6 address
}
```
We won't be talking about IPv6 flow or scope, too advanced for now..

Another simple structure, `struct sockaddr_storage` that is designed to be large enough to hold both IPv4 and IPv6 structures. Sometimes you don’t know in advance if it’s going to fill out your `struct sockaddr` with an IPv4 or IPv6 address. So you pass in this parallel structure, very similar to `struct sockaddr` except larger, and then cast it to the type you need:
```C
struct sockaddr_storage{
	sa_family_t ss_family;          // Address family
	
	// Padding implementation specific, just ignore....
	char __ss_pad1[_SS_PAD1SIZE];
	int64_t __ss_align;
	char __ss_pad2[__S_PAD2SIZE];
	
}
```
In the previous struct, we check the `ss_family` type to see if it's AF_INET or AF_INET6, and we cast to `struct sockaddr_in` or `struct sockaddr_in6` accordingly if we want.

So in conclusion:
```C
struct sockaddr    // contains general address type, and IP:PORT data memebrs
struct sockaddr_in // for IPv4 only
struct in_addr     // just contains the address itself only (IPv4)
struct sockaddr_in6// for IPv6 only
struct in6_addr    // just contains the address itself (IPv6)
struct sockaddr_storage // has address family, and can be casted to any struct above
```

### IP Addresses conversion
If we have string-numeric IP addresses and we want to convert them into `sin_addr` or `sin6_addr`, we use `inet_pton()` (`pton` stands for presentation to network), which returns -1 on error & 0 if the address is messed up, if >0 we're fine.
```C
struct sockaddr_in sa;   // IPv4;
struct sockaddr_in6 sa6; // IPv6

inet_pton(AF_INET, "10.12.22.34", &(sa.sin_addr));              // IPv4
inet_pton(AF_INET6, "2001:db7:23b2:1::3490", &(sa6.sin6_addr)); // IPv6
```
To do it the other way around (if we have `sin_addr` or `sin6_addr` and want to extract string form), we use the inverse of the function above, `inet_ntop()` 
```C
// IPv4
struct ip4[INET_ADDRSTRLEN];   // Space to hold the IPv4 string
struct sockaddr_in sa;         // Assume this sa already has data

inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);
printf("THE IP4: %s\n", ip4);

// IPv6
char ip6[INET6_ADDRSTRLEN];   // space to hold the IPv6 string
struct sockaddr_in6 sa6;      // Assume this sa already has data

inet_ntop(AF_INET6, &(sa6.sin6_addr), ip6, INET6_ADDRSTRLEN);
printf("THE IP6: %s\n", ip6);
```

### System Calls
#### `getaddrinfo()`
It helps to set up the structs.
```C
// Header
int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);

// It returns pointer to a lined list of results "res".
// service: can be port number (80), or name of a particular service (http, ftp, telnet, ...), can be NULL.
// hints: contains the basic/initial information about the address we want (typically the type (v4 or v6), UDP or TCP)
// node: can be the DNS we want (www.example.com)
```
Example code of using `getaddrinfo()`
```C
int status;
struct addrinfo hints;
struct addrinfo *servinfo; // linked list for the server information

memset(&hints, 0, sizeof hints); // make sure the struct is empty
hints.ai_family = AF_UNSPEC;     // don't care if IPv6 or IPv6
hints.ai_socktype = SOCK_STREAM; // TCP

// get ready to connect
status = getaddrinfo("www.example.com", NULL, &hints, &servinfo);
/// rest 
/// logic
freeaddrinfo(servinfo);
```

#### `socket()`
For the connection, it returns **`socket descriptor`** (int) it's of no good on itself, but important for others. this int is now used as the mailbox number that will be used by other calls (bind, ...) 

`int socket (int domain, int type, int protocol);`
it has:
- `domain` which is `PF_INET` or `PF_INET6` (similar to `AF_INET`)
- `type` which is `SOCK_STREAM` or `SOCK_DGRAM`
- `protocol` can be set to  0 to choose the proper protocol for the given type.
But we almost always just take the result of the `getaddrinfo()` call and feed into the `socket()` call
```
getaddrinfo("www.google.com", "http", &hints, &res);

s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
```

#### `bind()`
What port am I on? It "binds" a port & IP address to the made socket, it's more used if we're making the server not the client, we bind the socket to listen for incoming connections on the given IP & port.
`int bind(int sockfd, struct sockaddr* my_addr, int addrlen);` (in case we're the server, we pass the `sockaddr` res pointer of our own IP )
```C
hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE; // fill our own IP for us

getaddrinfo(NULL, "3000", &hints, &res); // 3000 is the port

// make a socket
sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
bind(sockfd, res->ai_addr, res->ai_addrlen); // ai_addr has IP and port

```
**Steps**
- Bind the socket to our IP `192.168.1.23`
- Someone on the network tries to connect to that IP
- the OS sees that our socket is bound to this port, so it sends the request to our program

#### `connect()`
(for clients not servers). It takes a **`socket file descriptor`** which should be created earlier before it, and takes the IP + address to assign to that socket, the the OS attempts to establish a connection with the specified server. And VOILA! Now the connection is established and ready to send/recv.
> Note: For UDP, using `connect()` doesn't actually establish a connection, but rather sets a default IP address to the socket.

`int connect(int sockfd, struct sockaddr *serv_addr, int addrlen);`
 Let's take an example now as a client trying to connect to a server `loay.work`
```C
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
// notice that we now don't assign our own IP address;
getaddrinfo("www.loay.work", "80", &hints, &res); // connection on port 80-https
// create socket
sockfd(res->ai_family, res->ai_socktype, res->ai_protocol);
// now connect!
connect(sockfd, res->ai_addr, res->ai_addrlen);
```
> Notice we didn't call `bind()` because we don't care from what port we send the request, the OS will take care of that for us.

#### `listen()`
If we don't want to send/connect to sockets, we instead want to receive connections from sockets. We have two steps: **`listen()`** then **`accept()`**
`int listen(int sockfd, int backlog);`
Backlog is the number of connections allowed on the incoming queue, requests stay in the queue till we `accept()` them. Most servers set that to 20.
So the sequence will be
```C
getaddrinfo();
socket();
bind();
listen();
// accept() goes here
```

#### `accept()`
It accepts the incoming requests/connections to the listening port, and it creates a new socket for each connection that I can use, keeping the main socket as it is receiving new connections if needed.

`int accept(int sockfd, struct sockaddr* addr, socklen_t *addrlen);`

`addr` will usually be a pointer to `struct sockaddr_storage` that we will recast to `struct sockaddr`. `addrlen` is a variable made by us that should be `sizeof(struct sockaddr_storage)`

```C
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;

getaddrinfo(NULL, "3000", &hints, &res);
sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
bind(sockfd, res->ai_addr, res->ai_addrlen);

// Now listen to incoming connections
listen(sockfd, 20); // 10 can be enough
// now accept an incoming connection into a new socket
struct sockaddr_storage their_address;
socklen_t addr_size = sizeof(their_address);
int new_fd = accept(sockfd, (struct sockaddr*) &their_addr, &addr_size);
// now ready to communicate with the new socket.
```

#### `send() & recv()`
For TCP (`SOCK_STREAM`), if we want to use the datagram sockets, use `sendto()` & `recvfrom()`

`int send(int sockfd, const void* msg, int len, int flags);`

- `sockfd` is the socket we want to send, the one we created that has the connection with the server.
- `msg` is pointer to the data we want to send.
- `len` is the length of that data in **bytes**.
- `flags` just set them to 0, they have their own manual.
```C
char* msg = "Beej was here";
int len, bytes_sent;
len = strlen(msg);
bytes_sent = send(sockfd, msg, len, 0); // it returns the number of bytes actually sent out.
```

`int recv(int sockfd, void* buf, int len, int flags);`
Same as send, but the buffer is a pointer to store the incoming data & `len `is the max buffer length. It returns the number of bytes received.

#### `sendto() & recvfrom()`
It's for DGRAM. It's same as `send()` but we add destination  address.
```C
// returns number of bytes actually sent
int sendto(int sockfd, const void* msg, int len, int flags,
		const struct sockaddr* to, socklen_t tolen); // new
```
The 2 new arguments:
- `to` is a pointer to a `struct sockaddr`, which will probably be `struct sockaddr_in` or `struct sockaddr_in6` or even `struct sockaddr_storage` that's casted, which contains the destination IP & port.
- `tolen` can simply be `sizeof *to` or `sizeof (struct sockaddr_storage)`
To know the destination address, we'll get it from `struct getaddrinfo()` or from the `recvfrom()` or manually put in.
Same for `recvfrom()`
```C
// returns number of bytes received
int recvfrom(int sockfd, void* buf, int len, int flags, 
		struct sockaddr* from, int* fromlen); // new
```
The new arguments:
- `from` is pointer to `struct sockaddr_storage` that **will**  be filled with the IP & port of the sender.
- `fromlen` is just `sizeof *from` or `sizeof (struct sockaddr_storage)`
> Note: We don't use struct `sockaddr_in` or `sockaddr_in6` because we don't want to tie ourselves to only 1 type of IP. We not always use `struct sockaddr`? Because it's not big enough, `struct sockaddr_storage` is just bigger to hold IP & port efficiently.

> Remember: If we used `connect()` on a datagram socket, we can just use `send()` and `recv()` and the socket will still behave as UDP and the socket interface will automatically add destination & source for us.

#### `close() & shutdown()`
To close connection, just use the regular Unix file descriptor close `close(sockfd)` We can use `shutdown(int sockfd, int how)` to close with style.
`how`:
- 0: Further receives are disallowed.
- 1: Further sends are disallowed.
- 2: Further sends and receives are disallowed (like `close()`)
> Note: `shutdown()` doesn't actually free the socket descriptor, to free it use `close()`.

#### `getpeername()`
It tells us who is at the other end of a connected **stream** socket.
`int getpeername(int sockfd, struct sockaddr* addr, int* addrlen);`
> Note: Should be used after server `accept()`, client `connect()` and of course before `close()`

We can then take the filled `addr` and use `inet_ntop()` to display its data

#### `gethostname()`
Who am I? It returns the name of the computer that our program is running on.
`int gethostname(char* hostname, size_t size);`

### Bonus: When creating a server
#### `setsockopt()`
It is used with its options to eliminate the idle time of the reserved port when closing a server.
> After a server is closed, the OS actually saves the port for a while to ensure the full transfer of the data to that port, so if we ran the server again, it'll say port already in use, we must wait for a minute or two. That's why we use `setsockopt` with its options to eliminate that delay from the OS.

#### Usage (where `yes` should be  = 1)
`setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1? : erorr`
