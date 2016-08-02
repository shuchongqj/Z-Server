//== ВКЛЮЧЕНИЯ.
#include <QCoreApplication>
#define _WINSOCKAPI_
#include "hub.h"
#ifndef WIN32
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

//== МАКРОСЫ.
#define LOG_NAME				"Z-Server"
//

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL
#ifndef WIN32
; // Баг в QtCreator`е на Windows, на Linux ';' нужна.
#endif

//== ФУНКЦИИ.
/// Взятие адреса.
void* GetInAddr(sockaddr *p_SockAddr)
{
	if(p_SockAddr->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)p_SockAddr)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)p_SockAddr)->sin6_addr);
}
/// Точка входа в приложение.
int main(int argc, char *argv[])
{
	int iAppResult = Z_OK;
	argc = argc; // Заглушка.
	argv = argv; // Заглушка.
	LOG_CTRL_INIT;
	_uiRetval = _uiRetval; // Заглушка.
	int iServerStatus, iListener, iConnection;
	addrinfo oHints, *pRes;
	sockaddr_storage oClientAddr;
	socklen_t slAddrSize;
	char mchAddrString[INET6_ADDRSTRLEN];
#ifdef WIN32
	WSADATA wsadata = WSADATA();
#endif
	//
	LOG(LOG_CAT_I, "Starting server");
	//
#ifdef WIN32
	WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif
	memset(&oHints, 0, sizeof oHints);
	oHints.ai_family = PF_UNSPEC;
	oHints.ai_socktype = SOCK_STREAM;
	oHints.ai_flags = AI_PASSIVE;
	oHints.ai_protocol = IPPROTO_TCP;
	iServerStatus = getaddrinfo(NULL, "8888" , &oHints, &pRes);
	if(iServerStatus != 0)
	{
		LOG(LOG_CAT_E, "getaddrinfo error: " << gai_strerror(iServerStatus));
		iAppResult = Z_ERROR;
		goto ex;
	}
	iListener = socket(pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);
	if(iListener < 0 )
	{
		LOG(LOG_CAT_E, "socket error: "  << gai_strerror(iServerStatus));
		iAppResult = Z_ERROR;
		goto ex;
	}
	iServerStatus = bind(iListener, pRes->ai_addr, (int)pRes->ai_addrlen);
	if(iServerStatus < 0)
	{
		LOG(LOG_CAT_E, "bind: " << gai_strerror(iServerStatus));
		iAppResult = Z_ERROR;
		goto ex;
	}
	iServerStatus = listen(iListener, 10);
	if(iServerStatus < 0)
	{
		LOG(LOG_CAT_E, "listen: " << gai_strerror(iServerStatus));
		iAppResult = Z_ERROR;
		goto ex;
	}
	freeaddrinfo(pRes);
	slAddrSize = sizeof oClientAddr;
	LOG(LOG_CAT_I, "Accepting connections");
	while(true)
	{
		iConnection = accept(iListener, (struct sockaddr *) & oClientAddr, &slAddrSize);
		if(iConnection < 0)
		{
			LOG(LOG_CAT_E, "accept: " << gai_strerror(iConnection));
			continue;
		}
		inet_ntop(oClientAddr.ss_family, GetInAddr((struct sockaddr*) &oClientAddr), mchAddrString, sizeof mchAddrString);
		LOG(LOG_CAT_I, "Connected with " << mchAddrString);
		iServerStatus = send(iConnection, "Welcome", 7, 0);
		if(iServerStatus == -1)
		{
			LOG(LOG_CAT_E, "Error in transaction: " << gai_strerror(iServerStatus));
			iAppResult = Z_ERROR;
			continue;
		}
	}
#ifndef WIN32
	close(iConnection);
#else
	closesocket(iConnection);
#endif
	//
ex:	LOGCLOSE;
#ifdef WIN32
		WSACleanup();
#endif
	return iAppResult;
}
