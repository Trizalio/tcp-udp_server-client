#include <QCoreApplication>

#include <QDebug>

#include "libeventserver.h"

void function_cb(char* pcData, size_t nLen, void* arg);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    LibeventServer server;

    int test = 23;
    qDebug() << "1";
    //server.UDPlistenTo("127.0.0.1", 9999, function_cb, (void*)&test, 4);
    //server.TCPlistenTo("127.0.0.1", 9998, function_cb, (void*)&test);
    //server.UDPSendRawTo("test", 4, "127.0.0.1", 9999);
    server.TCPSendRawTo("test", 4, "127.0.0.1", 9997);
    qDebug() << "2";
    server.run();
    qDebug() << "3";

    return a.exec();
}

void function_cb(char* pcData, size_t nLen, void* arg)
{
    qDebug() << *((int*) arg);
    for(int i = 0; i < nLen; ++i)
    {
        qDebug() << (short)pcData[i];
    }
    qDebug() << nLen << pcData;
}
