#include "libeventserver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <event2/listener.h>

void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
void conn_writecb(struct bufferevent *, void *);
void conn_readcb(struct bufferevent *, void *);
void conn_eventcb(struct bufferevent *, short, void *);
void accept_error_cb(struct evconnlistener *listener, void *ctx);

LibeventServer::LibeventServer(){

}
LibeventServer::~LibeventServer(){
    for(unsigned int i = 0; i < m_Definers.size(); ++i)
    {
        delete m_Definers.at(i);
    }
    m_Definers.clear();
}

void LibeventServer::init()
{
    m_base = event_base_new();
    if (!m_base) {
        perror("LibeventServer::LibeventServer: Cannot init event base");
        assert(0);
    }
}
void LibeventServer::UDPSendRawTo(char* pcData, size_t nDataLen, char* pcAddress, short nPort)
{
    int                 sock_fd;
    int                 flag = 1;
    struct sockaddr_in  sin;

    /* Create endpoint */
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("LibeventServer::UDPSendRawTo: Cannot init socket\n");
        assert(0);
    }

    /* Set socket option */
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        perror("LibeventServer::UDPSendRawTo: set socket options\n");
        assert(0);
    }

    /* Set IP, port */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(pcAddress);
    sin.sin_port = htons(nPort);

    sendto(sock_fd, pcData, nDataLen, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr));
}
void LibeventServer::TCPSendRawTo(char* pcData, size_t nDataLen, char* pcAddress, short nPort)
{
    /*int                 sock_fd;
    int                 flag = 1;
    struct sockaddr_in  sin;
    printf("TCPSendRawTo:hi\n");

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("LibeventServer::UDPSendRawTo: Cannot init socket\n");
        assert(0);
    }

    printf("TCPSendRawTo:1\n");
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        perror("LibeventServer::UDPSendRawTo: set socket options\n");
        assert(0);
    }

    printf("TCPSendRawTo:2\n");
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(pcAddress);
    sin.sin_port = htons(nPort);

    printf("TCPSendRawTo:3\n");
    sendto(sock_fd, pcData, nDataLen, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr));
    printf("TCPSendRawTo:4\n");*/

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if(!m_base){
        init();
    }
    struct sockaddr_in  sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(pcAddress);
    sin.sin_port = htons(nPort);

    struct bufferevent *bev = bufferevent_socket_new(m_base, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, NULL, NULL, conn_eventcb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    evbuffer_add(bufferevent_get_output(bev), pcData, nDataLen);

    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
      bufferevent_free(bev);
      perror("LibeventServer::UDPSendRawTo: connect error\n");
      assert(0);
    }
}

void LibeventServer::TCPlistenTo(char* pcAddress, unsigned short nPort, void(*function_cb)(char*, size_t, void*), void* arg)
{
    if(!m_base){
        init();
    }
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_addr.s_addr = inet_addr(pcAddress);
    sin.sin_port = htons(nPort);
    sin.sin_family = AF_INET;

    Definer* definer = new Definer();
    definer->arg = arg;
    definer->function_cb = function_cb;
    m_Definers.push_back(definer);

    listener = evconnlistener_new_bind(m_base, listener_cb, (void *)definer,
        LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin,
        sizeof(sin));

    if (!listener) {
        perror("LibeventServer::TCPlistenTo: Cannot init listner\n");
        assert(0);
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);
}

void LibeventServer::UDPlistenTo(char* pcAddress, unsigned short nPort, void(*function_cb)(char*, size_t, void*), void* arg, size_t nMaxLength)
{

    struct event*       ev;
    evutil_socket_t     sock_fd;
    int                 flag = 1;
    struct sockaddr_in  sin;

    if(!m_base){
        init();
    }

    /* Create endpoint */
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("LibeventServer::listenTo: Cannot init socket\n");
        assert(0);
    }

    /* Set socket option */
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) {
        perror("LibeventServer::listenTo: Cannot set socket options\n");
        assert(0);
    }

    /* Set IP, port */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(pcAddress);
    sin.sin_port = htons(nPort);

    /* Bind */
    if (bind(sock_fd, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0) {
        perror("LibeventServer::listenTo: Cannot bind socket\n");
        assert(0);
    } else {
        printf("LibeventServer::listenTo: bind() success - [%s] [%hu]\n", pcAddress, nPort);
    }

    Definer* definer = new Definer();
    definer->arg = arg;
    definer->function_cb = function_cb;
    definer->nBufSize = nMaxLength;
    m_Definers.push_back(definer);
    /* Init one event and add to active events */
    printf("listenTo: 4 \n");

    ev = event_new(m_base, sock_fd, EV_READ | EV_PERSIST, &read_cb, definer);
    printf("listenTo: 5 \n");
    if (event_add(ev, NULL) == -1) {
        perror("LibeventServer::listenTo: event_add() failed\n");
        assert(0);
    }
    printf("listenTo:bye\n");

}

void read_cb(evutil_socket_t fd, short event, void *arg) {
    printf("read_cb\n");
    Definer* definer = (Definer*) arg;
    char*               buf;
    buf = new char[definer->nBufSize];
    int                 len;
    int                 size = sizeof(struct sockaddr);
    struct sockaddr_in  client_addr;

    //memset(buf, 0, sizeof(buf));
    len = recvfrom(fd, buf, definer->nBufSize, 0, (struct sockaddr *)&client_addr, (socklen_t*)&size);

    if (len == -1) {
        perror("LibeventServer::read_cb: Reading from socket failed\n");
        assert(0);
    } else if (len == 0) {
        printf("LibeventServer::read_cb: Connection Closed\n");
    } else {
        printf("LibeventServer::read_cb: Red: len [%d] - content [%s]\n", len, buf);

        definer->function_cb(buf, len, definer->arg);
    }
    delete[] buf;
}
void LibeventServer::run() {
    printf("Run\n");
    event_base_dispatch(m_base);
    printf("Bye\n");
}
//////////////////////////////////////////////////////////////////////////////////////////

void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        event_base_loopbreak(base);
        return;
    }
    bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, (void *)user_data);
    int res = bufferevent_enable(bev, EV_READ);
}


void conn_writecb(struct bufferevent *bev, void *user_data)
{
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0 && BEV_EVENT_EOF ) {
        bufferevent_free(bev);
    }
}

void conn_readcb(struct bufferevent *pBufferEvent, void *user_data)
{
    printf("conn_readcb\n");
    Definer* definer = (Definer*)user_data;

    struct evbuffer* pInputBuffer = bufferevent_get_input(pBufferEvent);
    size_t size = 0;
    char* pcLine = evbuffer_readln(pInputBuffer, &size, EVBUFFER_EOL_CRLF);

    printf("evbuffer_readln\n");
    definer->function_cb(pcLine, size, definer->arg);

    delete[] pcLine;

    //Setup write callback
    bufferevent_setcb(pBufferEvent, NULL, conn_writecb, conn_eventcb, NULL);
    bufferevent_enable(pBufferEvent, EV_WRITE);
}

void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}


void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();\

    event_base_loopexit(base, NULL);
}


