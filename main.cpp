//== ВКЛЮЧЕНИЯ.
#define _WINSOCKAPI_
#include "hub.h"
#include <signal.h>
#ifndef WIN32
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <termios.h>
#else
#include <pthread/include/pthread.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

//== МАКРОСЫ.
#define LOG_NAME				"Z-Server"
#define ThreadDataPtr			((ThreadData*)p_Object)
#define MAXDATA					1024
#define MAXCONN					128
//

//== СТРУКТУРЫ.
//
struct ThreadData
{
	bool bInUse;
	bool bConnected;
	int iConnection;
	pthread_t p_Thread;
	char m_chData[MAXDATA];
};

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL
#ifndef WIN32
; // Баг в QtCreator`е на Windows, на Linux ';' нужна.
#endif
bool bExitSignal = false;
pthread_mutex_t ptMutex = PTHREAD_MUTEX_INITIALIZER;
int iListener;
bool bRequestNewConn;
//
ThreadData mThreadDadas[MAXCONN];
//

//== ФУНКЦИИ.
/// Очистка позиции данных потока.
void CleanThrDadaPos(int iPos)
{
	memset(&mThreadDadas[iPos], 0, sizeof(ThreadData));
}

/// Поиск свободной позиции данных потока.
int FindFreeThrDadaPos()
{
	int iPos = 0;
	//
	pthread_mutex_lock(&ptMutex);
	for(; iPos != MAXCONN; iPos++)
	{
		if(mThreadDadas[iPos].bInUse == false)
		{
			pthread_mutex_unlock(&ptMutex);
			return iPos;
		}
	}
	pthread_mutex_unlock(&ptMutex);
	return -1;
}

///
#ifndef WIN32
char getch()
{
	char buf = 0;
	termios old;
	//
	memset(&old, 0, sizeof old);
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr ICANON");
	if (read(0, &buf, 1) < 0)
		perror ("read()");
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
		perror ("tcsetattr ~ICANON");
	return (buf);
}

///
void* WaitingThread(void *p_vPlug)
{
	p_vPlug = p_vPlug;
	while(getch() != 0x1b);
	bExitSignal = true;
	bRequestNewConn = false;
	pthread_exit(p_vPlug);
}
#endif

///
void* ConversationThread(void* p_vNum)
{
	int iConnection, iStatus, iTPos;
	sockaddr_in saInet;
	socklen_t slLenInet;
	char* p_chSocketName;
	//
	iTPos = *((int*)p_vNum);
	mThreadDadas[iTPos].p_Thread = pthread_self();
#ifndef WIN32
	LOG(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread);
#else
	LOG(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	iConnection = (int)accept(iListener, NULL, NULL);
	mThreadDadas[iTPos].bConnected = true;
	if((iConnection < 0))
	{
		if(!bExitSignal)
		{
			LOG(LOG_CAT_E, "'accept': " << gai_strerror(iConnection));
			goto enc;
		}
		else
		{
			LOG(LOG_CAT_I, "Accepting connections terminated");
			goto enc;
		}
	}
	LOG(LOG_CAT_I, "New connection accepted");
	mThreadDadas[iTPos].iConnection = iConnection;
	slLenInet = sizeof(sockaddr);
	getpeername(iConnection, (sockaddr*)&saInet, &slLenInet);
	p_chSocketName = inet_ntoa(saInet.sin_addr);
	LOG(LOG_CAT_I, "Connected with: " << p_chSocketName);
	iStatus = send(iConnection, "Welcome\n", 9, 0);
	if(iStatus == -1)
	{
		LOG(LOG_CAT_E, "'send': " << gai_strerror(iStatus));
		goto ec;
	}
	//
	bRequestNewConn = true;
	while(bExitSignal == false)
	{
		iStatus = recv(iConnection, mThreadDadas[iTPos].m_chData, sizeof(mThreadDadas[iTPos].m_chData), 0);
		if (bExitSignal == true)
		{
			LOG(LOG_CAT_I, "Exiting reading: " << p_chSocketName);
			break;
		}
		if (iStatus <= 0)
		{
			LOG(LOG_CAT_I, "Reading socket stopped: " << p_chSocketName);
			break;
		}
		pthread_mutex_lock(&ptMutex);
		mThreadDadas[iTPos].m_chData[iStatus - 1] = 0;
		LOG(LOG_CAT_I, "Received: " << mThreadDadas[iTPos].m_chData);
		printf("\b\b");
		pthread_mutex_unlock(&ptMutex);
		iStatus = send(iConnection, "Message received\n", sizeof("Message received\n"), 0);
	}
	//
ec:
#ifndef WIN32
	shutdown(iConnection, SHUT_RDWR);
	close(iConnection);
#else
	closesocket(iConnection);
#endif
	LOG(LOG_CAT_I, "Socket closed: " << p_chSocketName);
enc:

#ifndef WIN32
	LOG(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread);
#else
	LOG(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	CleanThrDadaPos(iTPos);
	pthread_exit(p_vNum);
	return 0;
}

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
	int iServerStatus;
	addrinfo o_Hints;
	char* p_chServerIP = 0;
	char* p_chPort = 0;
	addrinfo* p_Res;
	int iCurrPos = 0;
#ifndef WIN32
	pthread_t KeyThr;
#else
	WSADATA wsadata = WSADATA();
#endif
	//
	LOG(LOG_CAT_I, "Starting server");
	//
	memset(&mThreadDadas, 0, sizeof mThreadDadas);
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
	memset(&o_Hints, 0, sizeof o_Hints);
	o_Hints.ai_family = PF_UNSPEC;
	o_Hints.ai_socktype = SOCK_STREAM;
	o_Hints.ai_flags = AI_PASSIVE;
	o_Hints.ai_protocol = IPPROTO_TCP;
	iServerStatus = getaddrinfo(p_chServerIP, p_chPort, &o_Hints, &p_Res);
	if(iServerStatus != 0)
	{
		LOG(LOG_CAT_E, "'getaddrinfo': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iListener = (int)socket(p_Res->ai_family, p_Res->ai_socktype, p_Res->ai_protocol);
	if(iListener < 0 )
	{
		LOG(LOG_CAT_E, "'socket': "  << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = bind(iListener, p_Res->ai_addr, (int)p_Res->ai_addrlen);
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
	freeaddrinfo(p_Res);
	//
#ifndef WIN32
	pthread_create(&KeyThr, NULL, WaitingThread, NULL);
#endif
	LOG(LOG_CAT_I, "Accepting connections, press 'Esc' for exit");
nc:	bRequestNewConn = false;
	iCurrPos = FindFreeThrDadaPos();
	mThreadDadas[iCurrPos].bInUse = true;
	pthread_create(&mThreadDadas[iCurrPos].p_Thread, NULL, ConversationThread, &iCurrPos);
#ifndef WIN32
	sleep(1);
#else
	Sleep(1000);
#endif
	while(!bExitSignal)
	{
#ifndef WIN32
#else
		if(GetConsoleWindow() == GetForegroundWindow())
		{
			if(GetAsyncKeyState(VK_ESCAPE)) bExitSignal = true;
		}
#endif
		if(bRequestNewConn == true)
			goto nc;
	}
	printf("\b\b");
	LOG(LOG_CAT_I, "Stopped by user");
#ifndef WIN32
	shutdown(iListener, SHUT_RDWR);
	close(iListener);
#else
	closesocket(iListener);
#endif
#ifndef WIN32
	sleep(1);
#else
	Sleep(1000);
#endif
	LOG(LOG_CAT_I, "Cleaning...");
	pthread_mutex_lock(&ptMutex);
	for(int iPos = 0; iPos != MAXCONN; iPos++)
	{
		if(mThreadDadas[iPos].bInUse == true)
		{
#ifndef WIN32
			shutdown(mThreadDadas[iPos].iConnection, SHUT_RDWR);
			close(mThreadDadas[iPos].iConnection);
#else
			closesocket(mThreadDadas[iPos].iConnection);
#endif
		}
	}
	pthread_mutex_unlock(&ptMutex);
#ifdef WIN32
		WSACleanup();
#endif
ex:
#ifndef WIN32
	sleep(1);
#else
	Sleep(1000);
#endif
	LOGCLOSE;
	pthread_exit(NULL);
}
