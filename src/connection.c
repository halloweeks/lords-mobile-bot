#include "connection.h"
#include "log.h"
// #include"protocol.h"

int set_nonblocking(Connection *conn)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(conn->sock, FIONBIO, &mode);
#else
    int flags = fcntl(conn->sock, F_GETFL, 0);
    if (flags < 0) return -1;

    return fcntl(conn->sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

int connect_server(const char *ip, unsigned short port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close_socket(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close_socket(sock);
        return -1;
    }

    return sock;
}

bool send_packet(Connection *conn, bool enc)
{
	if (enc) {
		EncryptData(conn->data + 4, conn->size - 4, conn->data + 4, ENCRYPTION_KEY);
	}
	
    return send(conn->sock, conn->data, conn->size, 0) == conn->size;
}

void disconnect(Connection *c)
{
    if (c->sock >= 0)
        close_socket(c->sock);
        c->sock = -1;
}


void reset_connection(Connection *c)
{
    c->sock = -1;
    
    memset(&c, 0, sizeof(c));
}

bool ConnectServer(Connection *c, const char *ip, unsigned short port)
{
	c->sock = socket(AF_INET, SOCK_STREAM, 0);
	
	if (c->sock < 0) {
		LOGE("socket() failed\n");
		return false;
	}
	
	/* Set non-blocking mode */
	int flags = fcntl(c->sock, F_GETFL, 0);
	
	if (flags == -1) {
		close(c->sock);
		return false;
	}
	
	if (fcntl(c->sock, F_SETFL, flags | O_NONBLOCK) == -1) {
		close(c->sock);
		return false;
	}
	
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	
	if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
		LOGE("Invalid server address: %s\n", ip);
		close_socket(c->sock);
		return false;
	}
	
	int ret = connect(c->sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	
	if (ret == 0) {
		/* Connected immediately (rare) */
		c->state = CONN_CONNECTED_GATEWAY;
		// RequestGuestLogIn(c);
		return true;
	}
	
	if (errno == EINPROGRESS) {
		/* Normal for non-blocking sockets */
		c->state = CONN_CONNECTING_GATEWAY;
		return true;
	}
	
	LOGE("connect() failed: %s\n", strerror(errno));
	
	close(c->sock);
	return false;
}


bool ConnectGateway(Connection *c)
{
    struct sockaddr_in addr;

    c->sock = socket(AF_INET, SOCK_STREAM, 0);

    if (c->sock == -1) {
        LOGE("socket() failed\n");
        return false;
    }

    /* Set non-blocking mode */
    int flags = fcntl(c->sock, F_GETFL, 0);

    if (flags == -1) {
        close(c->sock);
        return false;
    }

    if (fcntl(c->sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(c->sock);
        return false;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(c->gateway_server.port);

    if (inet_pton(AF_INET,
                  c->gateway_server.addr,
                  &addr.sin_addr) != 1) {
        LOGE("Invalid gateway address: %s\n",
             c->gateway_server.addr);

        close(c->sock);
        return false;
    }

    int ret = connect(c->sock,
                      (struct sockaddr *)&addr,
                      sizeof(addr));

    if (ret == 0) {
        /* Connected immediately (rare) */
        c->state = CONN_WAIT_GATEWAY_LOGIN;
        // RequestGuestLogIn(c);
        return true;
    }

    if (errno == EINPROGRESS) {
        /* Normal for non-blocking sockets */
        c->state = CONN_CONNECTING_GATEWAY;
        return true;
    }

    LOGE("connect() failed: %s\n", strerror(errno));

    close(c->sock);
    return false;
}


// helper in connection.c
bool epoll_register(int epoll_fd, Connection *c, uint32_t events)
{
    struct epoll_event ev = {0};
    ev.events = events;
    ev.data.ptr = c;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, c->sock, &ev) != -1;
}