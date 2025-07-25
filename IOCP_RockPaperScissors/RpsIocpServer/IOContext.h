#pragma once
#include <winsock2.h>
#include <windows.h>

// I/O 종류 구분
enum IOType 
{
    IO_ACCEPT,   // AcceptEx 완료
    IO_READ,     // recv 완료
    IO_WRITE     // send 완료
};

struct IOContext 
{
    OVERLAPPED overlapped;
    IOType     type;
    SOCKET     sock;
    WSABUF     wsaBuf;
    char       buffer[1024];

    IOContext(IOType t = IO_READ)
        : type(t), sock(INVALID_SOCKET)
    {
        ZeroMemory(&overlapped, sizeof(overlapped));
        wsaBuf.buf = buffer;
        wsaBuf.len = sizeof(buffer);
    }
};