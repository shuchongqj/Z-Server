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
#define MAXDATA					1024
#define MAXCONN					128
#define ZLOG(Category,Text)		pthread_mutex_lock(&ptLogMutex);															\
								LOG(Category,Text);																			\
								pthread_mutex_unlock(&ptLogMutex);
//

//== СТРУКТУРЫ.
/// Структура описания данных потока соединения.
struct ConversationThreadData
{
	bool bInUse; ///< Флаг использования в соотв. потоке.
	int iConnection; ///< ИД соединения.
	pthread_t p_Thread; ///< Указатель на рабочий поток.
	char m_chData[MAXDATA]; ///< Принятый от клиента пакет.
};

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL
#ifndef WIN32
; // Баг в QtCreator`е на Windows, на Linux ';' нужна.
#endif
bool bExitSignal = false; ///< Сигнал на общее завершение.
pthread_mutex_t ptConnMutex = PTHREAD_MUTEX_INITIALIZER; ///< Инициализатор мьютекса соединений.
pthread_mutex_t ptLogMutex = PTHREAD_MUTEX_INITIALIZER; ///< Инициализатор мьютекса лога.
int iListener; ///< Сокет приёмника.
bool bRequestNewConn; ///< Сигнал запроса нового соединения.
ConversationThreadData mThreadDadas[MAXCONN]; ///< Массив структур описания потоков соединений.

//== ФУНКЦИИ.
/// Очистка позиции данных потока.
void CleanThrDadaPos(int iPos)
{
	memset(&mThreadDadas[iPos], 0, sizeof(ConversationThreadData));
}

/// Поиск свободной позиции данных потока.
int FindFreeThrDadaPos()
							///< \return Возвращает номер свободной позиции, иначе - RETVAL_ERR.
{
	int iPos = 0;
	//
	pthread_mutex_lock(&ptConnMutex);
	for(; iPos != MAXCONN; iPos++)
	{
		if(mThreadDadas[iPos].bInUse == false) // Если не стоит флаг занятости - годен.
		{
			pthread_mutex_unlock(&ptConnMutex);
			return iPos;
		}
	}
	pthread_mutex_unlock(&ptConnMutex);
	return RETVAL_ERR;
}

/// Импортированная функция получения символа от пользователя в терминал (только для линукса).
#ifndef WIN32
char GetChar()
							///< \return Возвращает код символа от пользователя.
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

/// Поток ожидания ввода символа от пользователя в терминал (только для линукса).
void* WaitingThread(void *p_vPlug)
							///< \param[in] p_vPlug Заглушка.
							///< \return Заглушка.
{
	p_vPlug = p_vPlug;
	while(GetChar() != 0x1b);
	bExitSignal = true;
	bRequestNewConn = false;
	pthread_exit(p_vPlug);
	return 0;
}
#endif

/// Поток соединения.
void* ConversationThread(void* p_vNum)
{
							///< \param[in] p_vNum Ук. на переменную типа int с номером предоставленной структуры в mThreadDadas.
							///< \return Заглушка.
	int iConnection, iStatus, iTPos;
	sockaddr_in saInet;
	socklen_t slLenInet;
	char* p_chSocketName;
	//
	iTPos = *((int*)p_vNum); // Получили номер в массиве.
	mThreadDadas[iTPos].p_Thread = pthread_self(); // Задали ссылку на текущий поток.
#ifndef WIN32
	ZLOG(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread);
#else
	ZLOG(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	iConnection = (int)accept(iListener, NULL, NULL); // Ждём входящих.
	if((iConnection < 0)) // При ошибке после выхода из ожидания входящих...
	{
		if(!bExitSignal) // Если не было сигнала на выход от основного потока...
		{
			ZLOG(LOG_CAT_E, "'accept': " << gai_strerror(iConnection)); // Про ошибку.
			goto enc;
		}
		else
		{
			ZLOG(LOG_CAT_I, "Accepting connections terminated"); // Норм. сообщение.
			goto enc;
		}
	}
	ZLOG(LOG_CAT_I, "New connection accepted");
	mThreadDadas[iTPos].iConnection = iConnection; // Установка ИД соединения.
	slLenInet = sizeof(sockaddr);
	getpeername(iConnection, (sockaddr*)&saInet, &slLenInet);
	p_chSocketName = inet_ntoa(saInet.sin_addr);
	ZLOG(LOG_CAT_I, "Connected with: " << p_chSocketName); // Инфо про входящий IP.
	iStatus = send(iConnection, "Welcome\n", 9, 0);
	if(iStatus == -1) // Если не вышло отправить...
	{
		ZLOG(LOG_CAT_E, "'send': " << gai_strerror(iStatus));
		goto ec;
	}
	//
	bRequestNewConn = true; // Соединение готово - установка флага для главного потока на запрос нового.
	while(bExitSignal == false) // Пока не пришёл флаг общего завершения...
	{
		iStatus = recv(iConnection, mThreadDadas[iTPos].m_chData, sizeof(mThreadDadas[iTPos].m_chData), 0); // Принимаем пакет.
		if (bExitSignal == true) // Если по выходу из приёмки обнаружен общий сигнал на выход...
		{
			ZLOG(LOG_CAT_I, "Exiting reading: " << p_chSocketName);
			break;
		}
		if (iStatus <= 0) // Если статус приёмки - отказ (вместо кол-ва принятых байт)...
		{
			ZLOG(LOG_CAT_I, "Reading socket stopped: " << p_chSocketName);
			break;
		}
		pthread_mutex_lock(&ptConnMutex);
		mThreadDadas[iTPos].m_chData[iStatus - 1] = 0; // (временно!) ноль в конце, для текста.
		ZLOG(LOG_CAT_I, "Received: " << mThreadDadas[iTPos].m_chData);
		printf("\b\b");
		pthread_mutex_unlock(&ptConnMutex);
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
	ZLOG(LOG_CAT_I, "Socket closed: " << p_chSocketName);
enc:

#ifndef WIN32
	ZLOG(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread);
#else
	ZLOG(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	CleanThrDadaPos(iTPos);
	pthread_exit(p_vNum);
	return 0;
}

/// Взятие адреса на входе.
void* GetInAddr(sockaddr *p_SockAddr)
							///< \param[in] p_SockAddr Указатель на структуру описания адреса сокета.
{
	if(p_SockAddr->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)p_SockAddr)->sin_addr);
	}
	return &(((struct sockaddr_in6 *)p_SockAddr)->sin6_addr);
}

/// Точка входа в приложение.
int main(int argc, char *argv[])
							///< \param[in] argc Заглушка.
							///< \param[in] argv Заглушка.
							///< \return Общий результат работы.
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
	// Работа с файлом конфигурации.
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
	// Подготовка соединения сервера.
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
	iListener = (int)socket(p_Res->ai_family, p_Res->ai_socktype, p_Res->ai_protocol); // Получение сокета для приёмника.
	if(iListener < 0 )
	{
		LOG(LOG_CAT_E, "'socket': "  << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = bind(iListener, p_Res->ai_addr, (int)p_Res->ai_addrlen); // Привязка к указанному IP.
	if(iServerStatus < 0)
	{
		LOG(LOG_CAT_E, "'bind': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = listen(iListener, 10); // Запуск приёмника.
	if(iServerStatus < 0)
	{
		LOG(LOG_CAT_E, "'listen': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	freeaddrinfo(p_Res);
	//
#ifndef WIN32
	pthread_create(&KeyThr, NULL, WaitingThread, NULL); // Запуск потока ожидания ввода (только для линукса).
#endif
	ZLOG(LOG_CAT_I, "Accepting connections, press 'Esc' for exit");
	// Цикл ожидания входа клиентов.
nc:	bRequestNewConn = false; // Вход в звено цикла ожидания клиентов - сброс флага запроса.
	iCurrPos = FindFreeThrDadaPos();
	mThreadDadas[iCurrPos].bInUse = true; // Флаг занятости структуры.
	pthread_create(&mThreadDadas[iCurrPos].p_Thread, NULL, ConversationThread, &iCurrPos); // Создание нового потока приёмки.
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
	ZLOG(LOG_CAT_I, "Stopped by user");
	// Закрытие приёмника.
#ifndef WIN32
	shutdown(iListener, SHUT_RDWR);
	close(iListener);
#else
	closesocket(iListener);
#endif
	ZLOG(LOG_CAT_I, "Closing client sockets...");
	pthread_mutex_lock(&ptConnMutex);
	for(iCurrPos = 0; iCurrPos != MAXCONN; iCurrPos++) // Закрываем все клиентские сокеты.
	{
		if(mThreadDadas[iCurrPos].bInUse == true)
		{
#ifndef WIN32
			shutdown(mThreadDadas[iCurrPos].iConnection, SHUT_RDWR);
			close(mThreadDadas[iCurrPos].iConnection);
#else
			closesocket(mThreadDadas[iCurrPos].iConnection);
#endif
		}
	}
	pthread_mutex_unlock(&ptConnMutex);
	// Ждём, пока дойдёт до всех потоков соединений с клиентами.
stc:iCurrPos = 0;
	for(; iCurrPos != MAXCONN; iCurrPos++)
	{
		if(mThreadDadas[iCurrPos].bInUse == true)
		{
			goto stc;
		}
	}
ex:
#ifdef WIN32
		WSACleanup();
#endif
	LOGCLOSE;
	return _uiRetval;
}
