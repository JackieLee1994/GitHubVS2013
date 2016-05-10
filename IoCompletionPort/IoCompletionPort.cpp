// IoCompletionPort.cpp : �������̨Ӧ�ó������ڵ㡣
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
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;
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
		//�����ɶ˿ڵ�״̬
		if (GetQueuedCompletionStatus(CompletionPort, &BytesTransferred,
			(LPDWORD)&PerHandleData, (LPOVERLAPPED*)&PerIoData, INFINITE) == 0)
		{
			printf("GetQueuedCompletionStatus failed!\n");
			return 0;
		}
		//������ݴ������
		if (BytesTransferred == 0){
			printf("Closing Socket %d\n", PerHandleData->Socket);
			//�ر�socket
			if (closesocket(PerHandleData->Socket) == SOCKET_ERROR)
			{
				printf("closesocket() failed!\n");
				return 0;
			}
			//�ͷ���Դ
			GlobalFree(PerHandleData);
			GlobalFree(PerIoData);
			continue;
		}
		//�����û�м�¼���յ��������������յ����ֽ���������PerIoData->BytesRECV��
		if (PerIoData->BytesRECV == 0)
		{
			PerIoData->BytesRECV = BytesTransferred;
			PerIoData->BytesSEND = 0;
		}
		else
		{
			PerIoData->BytesSEND += BytesTransferred;
		}
		//���յ�������ԭ�����͵��ͻ���
		if (PerIoData->BytesRECV > PerIoData->BytesSEND)
		{
			ZeroMemory(&(PerIoData->Overlapped), sizeof(OVERLAPPED));

			PerIoData->DataBuf.buf = PerIoData->Buffer + PerIoData->BytesSEND;

			//ÿ����һ���ֽڵ�����
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
	SOCKADDR_IN InternetAddr;
	SOCKET Listen;
	SOCKET Accept;
	HANDLE CompletionPort;
	SYSTEM_INFO SystemInfo;
	LPPER_HANDLE_DATA PerHandleData;
	LPPER_IO_OPERATION_DATA PerIoData;
	DWORD RecvBytes;
	DWORD Flags;
	DWORD ThreadID;
	WSADATA wsaData;
	DWORD Ret;

	if ((Ret = WSAStartup(0x0202, &wsaData)) != 0){
		printf("WSAStartup failed!\n");
		return -1;
	}

	//�����µ���ɶ˿�
	if ((CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL)
	{
		printf("CreateCompletionPort failed!\n");
		return -1;
	}

	GetSystemInfo(&SystemInfo);

	for (int i = 0; i < SystemInfo.dwNumberOfProcessors * 2; i++)
	{
		HANDLE ThreadHandle;
		if ((ThreadHandle = CreateThread(NULL, 0, ServerWorkerThread, CompletionPort, 0, &ThreadID)) == NULL)
		{
			printf("CreateThread falied!\n");
			return -1;
		}
		CloseHandle(ThreadHandle);
	}

	if ((Listen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printf("WSASocket failed!\n");
		return -1;
	}
	//�󶨵����ص�ַ
	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	InternetAddr.sin_port = htons(PORT);
	if (bind(Listen, (PSOCKADDR)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
	{
		printf("bind failed!\n");
		return -1;
	}
	if (listen(Listen, 5) == SOCKET_ERROR)
	{
		printf("listen faild!\n");
		return -1;
	}
	while (true)
	{
		if ((Accept = WSAAccept(Listen, NULL, NULL, NULL, 0)) == SOCKET_ERROR)
		{
			printf("WSAAccept failed!\n");
			return -1;
		}
		if ((PerHandleData = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA))) == NULL)
		{
			printf("GlobalAlloc failed!\n");
			return -1;
		}
		PerHandleData->Socket = Accept;
		if (CreateIoCompletionPort((HANDLE)Accept, CompletionPort, (DWORD)PerHandleData, 0) == NULL)
		{
			printf("CreateIoCompletionport!\n");
			return -1;
		}
		//Ϊio�ṹ������ڴ�ռ�
		if ((PerIoData = (LPPER_IO_OPERATION_DATA)GlobalAlloc(GPTR, sizeof(PER_IO_OPERATION_DATA))) == NULL)
		{
			printf("GlobalAlloc failed!\n");
			return -1;
		}
		ZeroMemory(&(PerIoData->Overlapped), sizeof(OVERLAPPED));
		PerIoData->BytesSEND = 0;
		PerIoData->BytesRECV = 0;
		PerIoData->DataBuf.len = DATA_BUFSIZE;
		PerIoData->DataBuf.buf = PerIoData->Buffer;
		Flags = 0;

		if (WSARecv(Accept, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				printf("WSARecv() failed!\n");
				return -1;
			}
		}
	}
	return 0;
}

