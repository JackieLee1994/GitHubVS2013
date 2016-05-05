// IoCompletionPort.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <WinSock2.h>
#include <stdio.h>
#pragma comment(lib,"WS2_32.lib")
#define PORT 9990
#define DATA_BUFSIZE 8192
typedef struct
{
	OVERLAPPED Overlapped;
	WSABUF DataBuf;
	CHAR Buffer[DATA_BUFSIZE];
	DWORD BytesSEND;
	DWORD BytesRECV;
} PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;
typedef struct
{
	SOCKET Socket;
} PER_HANDLE_DATA,*LPPER_HANDLE_DATA;
DWORD WINAPI ServerWorkerThread(LPVOID CompletionPortID)
{
	HANDLE CompletionPort = (HANDLE)CompletionPortID;
	DWORD BytesTransferred;
	LPOVERLAPPED Overlapped;
	LPPER_HANDLE_DATA PerHandleData;
	LPPER_IO_OPERATION_DATA PerIoData;
	DWORD SendBytes, RecvBytes;
	DWORD Flags;

	while (true)
	{
		//检查完成端口的状态
		if (GetQueuedCompletionStatus(CompletionPort, &BytesTransferred,
			(LPDWORD)&PerHandleData, (LPOVERLAPPED*)&PerIoData, INFINITE) == 0)
		{
			printf("GetQueuedCompletionStatus failed!\n");
			return 0;
		}
		//如果数据传送完成
		if (BytesTransferred == 0){
			printf("Closing Socket %d\n", PerHandleData->Socket);
			//关闭socket
			if (closesocket(PerHandleData->Socket) == SOCKET_ERROR)
			{
				printf("closesocket() failed!\n");
				return 0;
			}
			//释放资源
			GlobalFree(PerHandleData);
			GlobalFree(PerIoData);
			continue;
		}
		//如果还没有记录接收的数据数量，则将收到的字节数保存在PerIoData->BytesRECV中
		if (PerIoData->BytesRECV == 0)
		{
			PerIoData->BytesRECV = BytesTransferred;
			PerIoData->BytesSEND = 0;
		}
		else
		{
			PerIoData->BytesSEND += BytesTransferred;
		}
		//将收到的数据原样发送到客户端
		if (PerIoData->BytesRECV > PerIoData->BytesSEND)
		{
			ZeroMemory(&(PerIoData->Overlapped), sizeof(OVERLAPPED));

			PerIoData->DataBuf.buf = PerIoData->Buffer + PerIoData->BytesSEND;

			//每发送一个字节的数据
			if (WSASend(PerHandleData->Socket, &(PerIoData->DataBuf), 1, &SendBytes, 0, &(PerIoData->Overlapped), NULL) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != ERROR_IO_PENDING)
				{
					printf("WSASend() failed!\n");
					return 0;
				}
			}
		}
		else
		{
			PerIoData->BytesRECV = 0;
			Flags = 0;
			ZeroMemory(&(PerIoData->Overlapped), sizeof(Overlapped));
			PerIoData->DataBuf.len = DATA_BUFSIZE;
			PerIoData->DataBuf.buf = PerIoData->Buffer;
			if (WSARecv(PerHandleData->Socket, &(PerIoData->DataBuf), 
				1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != ERROR_IO_PENDING)
				{
					printf("WSARecv() failed!\n");
					return 0;
				}
			}
		}
	}
}
int _tmain(int argc, _TCHAR* argv[])
{
	printf("I have changed!\n");
	return 0;
}

