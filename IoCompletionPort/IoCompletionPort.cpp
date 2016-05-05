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
	printf("I have changed!\n");
	return 0;
}

