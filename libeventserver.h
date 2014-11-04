#ifndef LIBEVENTSERVER_H
#define LIBEVENTSERVER_H

#include <iostream>
#include <string>
#include <assert.h>
#include <errno.h>
#include <vector>
#include <cstring>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

struct Definer
{
    void(*function_cb)(char*, size_t, void*);
    void* arg;
    size_t nBufSize;
};

class LibeventServer
{
public:
    LibeventServer();
    ~LibeventServer();
    void TCPlistenTo(char* pcAddress, unsigned short nPort, void(*function_cb)(char*, size_t, void*), void* arg);
    void UDPlistenTo(char* pcAddress, unsigned short nPort, void(*function_cb)(char*, size_t, void*), void* arg, size_t nMaxLength = 1024);
    void TCPSendRawTo(char* pcData, size_t nDataLen, char* pcAddress, short nPort);
    void UDPSendRawTo(char* pcData, size_t nDataLen, char* pcAddress, short nPort);
    void run();

private:
    void init();
    bool m_bTcp;
    std::vector<Definer*> m_Definers;
    struct event_base *m_base;
    struct evconnlistener* listener;
};

void read_cb(int fd, short event, void *arg);

#endif // LIBEVENTHTTPSERVER_H
