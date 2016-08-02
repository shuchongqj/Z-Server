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
	argc = argc; // Заглушка.
	argv = argv; // Заглушка.
	tinyxml2::XMLDocument xmlDocSConf;
	list <XMLNode*> o_lNet;
	XMLError eResult;
	LOG_CTRL_INIT;
	_uiRetval = _uiRetval; // Заглушка.
	int iServerStatus, iListener, iConnection;
	addrinfo oHints, *pRes;
	char* p_chServerIP = 0;
	char* p_chPort = 0;
#ifndef WIN32

#else
	WSADATA wsadata = WSADATA();
#endif
	//
	LOG(LOG_CAT_I, "Starting server");
	//
	eResult = xmlDocSConf.LoadFile(S_CONF_PATH);
	if (eResult != XML_SUCCESS)
	{
		LOG(LOG_CAT_E, "Can`t open configuration file:" << S_CONF_PATH);
		RETVAL_SET(RETVAL_ERR);
		LOG_CTRL_EXIT;
	}
	else
	{
		LOG(LOG_CAT_I, "Configuration loaded");
	}
	if(!FindChildNodes(xmlDocSConf.LastChild(), o_lNet,
					   "Net", FCN_ONE_LEVEL, FCN_FIRST_ONLY))
	{
		LOG(LOG_CAT_E, "Configuration file is corrupt! No 'Net' node");
		RETVAL_SET(RETVAL_ERR);
		LOG_CTRL_EXIT;
	}
	FIND_IN_CHILDLIST(o_lNet.front(), p_ListServerIP, "IP",
					  FCN_ONE_LEVEL, p_NodeServerIP)
	{
		p_chServerIP = (char*)p_NodeServerIP->FirstChild()->Value();
	} FIND_IN_CHILDLIST_END(p_ListServerIP);
	if(p_chServerIP != 0)
	{
		LOG(LOG_CAT_I, "Server IP: " << p_chServerIP);
	}
	else
	{
		LOG(LOG_CAT_E, "Configuration file is corrupt! No '(Net)IP' node");
		RETVAL_SET(RETVAL_ERR);
		LOG_CTRL_EXIT;
	}
	FIND_IN_CHILDLIST(o_lNet.front(), p_ListPort, "Port",
					  FCN_ONE_LEVEL, p_NodePort)
	{
		p_chPort = (char*)p_NodePort->FirstChild()->Value();
	} FIND_IN_CHILDLIST_END(p_ListPort);
	if(p_chPort != 0)
	{
		LOG(LOG_CAT_I, "Port: " << p_chPort);
	}
	else
	{
		LOG(LOG_CAT_E, "Configuration file is corrupt! No '(Net)Port' node");
		RETVAL_SET(RETVAL_ERR);
		LOG_CTRL_EXIT;
	}
#ifdef WIN32
	if(WSAStartup(MAKEWORD(2, 2), &wsadata) != NO_ERROR)
	{
		LOG(LOG_CAT_E, "'WSAStartup' failed");
	}
#endif
	memset(&oHints, 0, sizeof oHints);
	oHints.ai_family = PF_UNSPEC;
	oHints.ai_socktype = SOCK_STREAM;
	oHints.ai_flags = AI_PASSIVE;
	oHints.ai_protocol = IPPROTO_TCP;
	iServerStatus = getaddrinfo(p_chServerIP, p_chPort, &oHints, &pRes);
	if(iServerStatus != 0)
	{
		LOG(LOG_CAT_E, "'getaddrinfo': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iListener = socket(pRes->ai_family, pRes->ai_socktype, pRes->ai_protocol);
	if(iListener < 0 )
	{
		LOG(LOG_CAT_E, "'socket': "  << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = bind(iListener, pRes->ai_addr, (int)pRes->ai_addrlen);
	if(iServerStatus < 0)
	{
		LOG(LOG_CAT_E, "'bind': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = listen(iListener, 10);
	if(iServerStatus < 0)
	{
		LOG(LOG_CAT_E, "'listen': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	freeaddrinfo(pRes);
	LOG(LOG_CAT_I, "Accepting connections");
	while(true)
	{
		iConnection = accept(iListener, NULL, NULL);
		LOG(LOG_CAT_I, "Accepted");
		if(iConnection < 0)
		{
			LOG(LOG_CAT_E, "'accept': " << gai_strerror(iConnection));
			RETVAL_SET(RETVAL_ERR);
			continue;
		}
		//
		sockaddr_in oAddrInet;
		socklen_t slLenInet;
		slLenInet = sizeof(sockaddr);
		getpeername(iConnection, (sockaddr*)&oAddrInet, &slLenInet);
		LOG(LOG_CAT_I, "Connected with: " << inet_ntoa(oAddrInet.sin_addr));
		iServerStatus = send(iConnection, "Welcome", 7, 0);
		if(iServerStatus == -1)
		{
			LOG(LOG_CAT_E, "'send': " << gai_strerror(iServerStatus));
			RETVAL_SET(RETVAL_ERR);
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
	LOG_CTRL_EXIT;
}
