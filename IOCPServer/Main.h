#pragma once
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <thread>
#include <ws2def.h>
#include <WS2tcpip.h>
#include "Test.pb.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define PACKET_SIZE 1024

#pragma comment(lib, "ws2_32.lib")

enum EIOType
{
	RECV,
	SEND
};

enum EMessageTypes {
	None = 0,
	SignUp = 1,
	LogIn = 2
};

struct ClientMessage {
	EMessageTypes Type;
	std::string ID;
	std::string PW;
};

struct OverLapped {
	WSAOVERLAPPED OverLapped;
	WSABUF WSABuf;
	char Buf[PACKET_SIZE];

	EIOType IOType;
};

struct Client {
	SOCKET Sock;

	OverLapped RecvOverLapped;
	OverLapped SendOverLapped;

	Client() {
		ZeroMemory(&RecvOverLapped, sizeof(OverLapped));
		ZeroMemory(&SendOverLapped, sizeof(OverLapped));
		Sock = INVALID_SOCKET;
	}
};

class IOCPServ {
public:
	IOCPServ() { 
		_handle = INVALID_HANDLE_VALUE;
		_servSock = INVALID_SOCKET;
		_maxClientCount = (std::thread::hardware_concurrency() << 1) - 1; 
	}

	~IOCPServ() {
		WSACleanup();
	}

	bool Init();
	bool StartServer();
	void DestroyThread();

private:
	void CreateThread();
	void AcceptPlayer();
	void CloseSocket(Client* client, bool bForce);
	bool BindRecv(Client*);
	bool CreateAcceptThread();
	Client* GetEmptyClientInfo();
	bool BindIOPort(Client*);

	std::vector<Client> _connectedClients;
	std::vector<std::thread> _workerThreadPool;
	std::thread _acceptThread;
	SOCKET _servSock;
	HANDLE _handle;
	int _currentClientCount = 0;
	int _maxClientCount;
	bool _bWork = true;
	bool _bAccept = true;
};

bool IOCPServ::Init() {
	//winsock 초기화
	//윈도우 소켓 구현에 대한 정보가 포함된 구조체
	WSADATA wsa;
	//winsock DLL 사용 시작
	//2, 2: winsock 2.2 요청
	if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
		perror("WSAStart Error");
		return false;
	}

	//소켓 생성, PF_INET: IPv4, SOCK_STREAM: TCP
	_servSock = WSASocketW(PF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (_servSock == INVALID_SOCKET) {
		perror("Sock Error");
		return false;
	}

	SOCKADDR_IN addr;
	//IPv4
	addr.sin_family = AF_INET;
	//port, 하드코딩 없애기
	addr.sin_port = htons(4444);
	//IPv4 address INADDR_ANY: 어느 주소든 받겠다는 것
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//로컬 주소와 소켓을 연결
	if (bind(_servSock, (SOCKADDR*)&addr, sizeof(addr))) {
		perror("Bind Error");
		return false;
	}

	//수신 대기
	if (listen(_servSock, _maxClientCount)) {
		perror("Listen Error");
		return false;
	}

	return true;
}

bool IOCPServ::StartServer() {
	for (int i = 0; i < _maxClientCount; ++i)
		_connectedClients.emplace_back();

	_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, _maxClientCount);

	if (_handle == NULL) {
		perror("CreateIOCompletion Error");
		return false;
	}

	for (int i = 0; i < _maxClientCount; ++i) {
		_workerThreadPool.emplace_back([this]() { CreateThread(); });
	}

	std::cout << "workerThread 생성...\n";

	return CreateAcceptThread();
}

void IOCPServ::DestroyThread() {
	_bWork = false;
	CloseHandle(_handle);

	for (auto& thread : _workerThreadPool) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	_bAccept = false;
	closesocket(_servSock);

	if (_acceptThread.joinable())
		_acceptThread.join();
}

bool IOCPServ::CreateAcceptThread() {
	_acceptThread = std::thread([this]() { AcceptPlayer(); });
	std::cout << "Accept Start...\n";
	return true;
}

Client* IOCPServ::GetEmptyClientInfo() {
	for (auto& client : _connectedClients) {
		if (client.Sock == INVALID_SOCKET)
			return &client;
	}

	return nullptr;
}

void IOCPServ::CreateThread() {
	Client* client = NULL;
	bool bSuccess;
	DWORD IOSize;
	LPOVERLAPPED lpOverLapped = NULL;

	while (_bWork) {
		bSuccess = GetQueuedCompletionStatus(_handle, &IOSize, (PULONG_PTR)&client, &lpOverLapped, 100 * 1000);

		if (bSuccess && !IOSize && lpOverLapped == NULL) {
			_bWork = false;
			continue;
		}

		if (lpOverLapped == NULL)
			continue;

		if (!bSuccess || (!IOSize && bSuccess)) {
			std::cout << "socket" << client->Sock << " disConnect\n";
			CloseSocket(client, false);
			continue;
		}

		OverLapped* overlapped = (OverLapped*)lpOverLapped;

		if (overlapped->IOType == EIOType::RECV) {
			overlapped->Buf[IOSize] = NULL;
			std::cout << "Recv msg: " << overlapped->Buf << '\n';
			SearchRequest s;
			s.ParseFromArray(overlapped->Buf, overlapped->WSABuf.len);
			BindRecv(client);
		}

		else if (overlapped->IOType == EIOType::SEND)
			std::cout << "Send msg: " << overlapped->Buf << '\n';

		else 
			std::cout << "socket " << client->Sock << " Exception\n";
	}
}

bool IOCPServ::BindRecv(Client* client) {
	DWORD flag = 0, recvBytes;

	//패킷 크기
	client->RecvOverLapped.WSABuf.len = PACKET_SIZE;
	//??
	client->RecvOverLapped.WSABuf.buf = client->RecvOverLapped.Buf;
	client->RecvOverLapped.IOType = EIOType::RECV;

	int ret = WSARecv(client->Sock,
		&(client->RecvOverLapped.WSABuf),
		1,
		&recvBytes,
		&flag,
		(LPWSAOVERLAPPED) & (client->RecvOverLapped),
		NULL);

	if (ret == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING)) {
		std::cout << "WSARecv Failed: " << WSAGetLastError() << '\n';
		return false;
	}

	return true;
}

void IOCPServ::CloseSocket(Client* client, bool bForce = false) {
	struct linger clientLinger = { 0, 0 };

	if (bForce) {
		clientLinger.l_onoff = 1;
	}

	shutdown(client->Sock, SD_BOTH);

	setsockopt(client->Sock, SOL_SOCKET, SO_LINGER, (char*)&clientLinger, sizeof(clientLinger));

	closesocket(client->Sock);
	client->Sock = INVALID_SOCKET;
}

void IOCPServ::AcceptPlayer() {
	SOCKADDR_IN addr;
	int len = sizeof(addr);
	while (_bAccept) {
		Client* client = GetEmptyClientInfo();

		if (client == nullptr) {
			std::cout << "Max Client" << '\n';
			return;
		}

		client->Sock = accept(_servSock, (SOCKADDR*)&addr, &len);
		
		if (client->Sock == INVALID_SOCKET)
			continue;

		if (!BindIOPort(client)) {
			return;
		}

		if (!BindRecv(client)) {
			return;
		}

		char clientIP[32] = { 0, };

		inet_ntop(AF_INET, &(addr.sin_addr), clientIP, (int)client->Sock);

		std::cout << "Connect Client: " << clientIP << '\n';

		++_currentClientCount;
	}
}

bool IOCPServ::BindIOPort(Client* client) {
	auto handle = CreateIoCompletionPort((HANDLE)client->Sock,
		_handle,
		(ULONG_PTR)client,
		0);

	if (handle == NULL || handle != _handle) {
		std::cout << "CreateIoCompletionPort 에러: " << GetLastError() << '\n';
		return false;
	}

	return true;
}