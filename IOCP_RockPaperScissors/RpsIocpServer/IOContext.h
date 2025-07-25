#pragma once
#include <winsock2.h>
#include <windows.h>

// I/O ���� ����
enum IOType 
{
    IO_ACCEPT,   // AcceptEx �Ϸ�
    IO_READ,     // recv �Ϸ�
    IO_WRITE     // send �Ϸ�
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