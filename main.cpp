//== ВКЛЮЧЕНИЯ.
#define _WINSOCKAPI_
#include "hub.h"
#include "protoparser.h"
#include <signal.h>
#ifndef WIN32
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
#include <WS2tcpip.h>
#endif

//== МАКРОСЫ.
#define LOG_NAME				"Z-Server"
#define MAX_CONN				128
#define USER_RESPONSE_MS		100
#define Z_LOG(Category,Text)	pthread_mutex_lock(&ptLogMutex);															\
								LOG(Category,Text);																			\
								pthread_mutex_unlock(&ptLogMutex);
#ifndef WIN32
#define MSleep(val)				usleep(val)
#else
#define MSleep(val)				Sleep(val)
#endif
#define PAWWS_ERROR				"Authentification filed"
#define PAWWS_ABSENT			"Client is not autherised"
#define PAWWS_OK				"Connection is secured"
#define POCKET_OUT_OF_RANGE		"Pocket out of range"
#define UNKNOWN_COMMAND			"Unknown command"
//

//== СТРУКТУРЫ.
/// Структура описания данных потока соединения.
struct ConversationThreadData
{
	bool bInUse; ///< Флаг использования в соотв. потоке.
	int iConnection; ///< ИД соединения.
	sockaddr saInet; ///< Адрес клиента.
#ifndef WIN32
	socklen_t ai_addrlen; ///< Длина адреса.
#else
	size_t ai_addrlen; ///< Длина адреса.
#endif
	pthread_t p_Thread; ///< Указатель на рабочий поток.
	char m_chData[MAX_DATA]; ///< Принятый от клиента пакет.
	bool bSecured;
};

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL
#ifndef WIN32
; // Баг в QtCreator`е на Windows, на Linux ';' нужна.
#endif
static bool bExitSignal = false; ///< Сигнал на общее завершение.
static pthread_mutex_t ptConnMutex = PTHREAD_MUTEX_INITIALIZER; ///< Инициализатор мьютекса соединений.
static pthread_mutex_t ptLogMutex = PTHREAD_MUTEX_INITIALIZER; ///< Инициализатор мьютекса лога.
static int iListener; ///< Сокет приёмника.
static bool bRequestNewConn; ///< Сигнал запроса нового соединения.
static ConversationThreadData mThreadDadas[MAX_CONN]; ///< Массив структур описания потоков соединений.
static bool bListenerAlive; ///< Признак жизни приёмника.
static char *p_chPassword = 0;
static const char m_chPasswError[] = PAWWS_ERROR;
static const char m_chPasswAbsent[] = PAWWS_ABSENT;
static const char m_chPasswOk[] = PAWWS_OK;
static const char m_chPockenOutOfRange[] = POCKET_OUT_OF_RANGE;
static const char m_chUnknownCommand[] = UNKNOWN_COMMAND;

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
	for(; iPos != MAX_CONN; iPos++)
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
	pthread_exit(p_vPlug);
	return 0;
}
#endif

/// Поток соединения.
void* ConversationThread(void* p_vNum)
{
							///< \param[in] p_vNum Ук. на переменную типа int с номером предоставленной структуры в mThreadDadas.
							///< \return Заглушка.
	int iTPos;
	bool bKillListenerAccept;
	ProtoParser* p_ProtoParser;
	char chPersingResult;
	ConnectionData oConnectionData;
	char m_chNameBuffer[INET6_ADDRSTRLEN];
	//
	iTPos = *((int*)p_vNum); // Получили номер в массиве.
	mThreadDadas[iTPos].p_Thread = pthread_self(); // Задали ссылку на текущий поток.
#ifndef WIN32
	Z_LOG(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread);
#else
	Z_LOG(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	bKillListenerAccept = false;
	oConnectionData.iSocket = (int)accept(iListener, NULL, NULL); // Ждём входящих.
	if((oConnectionData.iSocket < 0)) // При ошибке после выхода из ожидания входящих...
	{
		if(!bExitSignal) // Если не было сигнала на выход от основного потока...
		{
			pthread_mutex_lock(&ptConnMutex);
			Z_LOG(LOG_CAT_E, "'accept': " << gai_strerror(oConnectionData.iSocket)); // Про ошибку.
			goto enc;
		}
		else
		{
			pthread_mutex_lock(&ptConnMutex);
			Z_LOG(LOG_CAT_I, "Accepting connections terminated"); // Норм. сообщение.
			bKillListenerAccept = true; // Заказываем подтверждение закрытия приёмника.
			goto enc;
		}
	}
	mThreadDadas[iTPos].bInUse = true; // Флаг занятости структуры.
	Z_LOG(LOG_CAT_I, "New connection accepted");
	mThreadDadas[iTPos].iConnection = oConnectionData.iSocket; // Установка ИД соединения.
	oConnectionData.ai_addrlen = sizeof(sockaddr);
#ifndef WIN32
	getpeername(oConnectionData.iSocket, &oConnectionData.ai_addr, &oConnectionData.ai_addrlen);
#else
	getpeername(oConnectionData.iSocket, &oConnectionData.ai_addr, (int*)&oConnectionData.ai_addrlen);
#endif
	mThreadDadas[iTPos].saInet = oConnectionData.ai_addr;
	mThreadDadas[iTPos].ai_addrlen = oConnectionData.ai_addrlen;
#ifndef WIN32
	getnameinfo(&mThreadDadas[iTPos].saInet, mThreadDadas[iTPos].ai_addrlen,
			m_chNameBuffer, sizeof(m_chNameBuffer), 0, 0, NI_NUMERICHOST);
#else
	getnameinfo(&mThreadDadas[iTPos].saInet, (socklen_t)mThreadDadas[iTPos].ai_addrlen,
			m_chNameBuffer, sizeof(m_chNameBuffer), 0, 0, NI_NUMERICHOST);
#endif
	Z_LOG(LOG_CAT_I, "Connected with: " << m_chNameBuffer); // Инфо про входящий IP.
	if(oConnectionData.iStatus == -1) // Если не вышло отправить...
	{
		pthread_mutex_lock(&ptConnMutex);
		Z_LOG(LOG_CAT_E, "'send': " << gai_strerror(oConnectionData.iStatus));
		goto ec;
	}
	//
	bRequestNewConn = true; // Соединение готово - установка флага для главного потока на запрос нового.
	p_ProtoParser = new ProtoParser;
	while(bExitSignal == false) // Пока не пришёл флаг общего завершения...
	{
		oConnectionData.iStatus =
				recv(oConnectionData.iSocket, mThreadDadas[iTPos].m_chData,
					 sizeof(mThreadDadas[iTPos].m_chData), 0); // Принимаем пакет.
		pthread_mutex_lock(&ptConnMutex);
		if (bExitSignal == true) // Если по выходу из приёмки обнаружен общий сигнал на выход...
		{
			Z_LOG(LOG_CAT_I, "Exiting reading: " << m_chNameBuffer);
			goto ecd;
		}
		if (oConnectionData.iStatus <= 0) // Если статус приёмки - отказ (вместо кол-ва принятых байт)... strPassword
		{
			Z_LOG(LOG_CAT_I, "Reading socket stopped: " << m_chNameBuffer);
			goto ecd;
		}
		chPersingResult = p_ProtoParser->ParsePocket(mThreadDadas[iTPos].m_chData, oConnectionData.iStatus);
		switch(chPersingResult)
		{
			case PROTOPARSER_OK:
			{
				if(mThreadDadas[iTPos].bSecured == false)
				{
					if(p_ProtoParser->oParsedObject.chTypeCode == PROTO_C_SEND_PASSW)
					{
						for(char chI = 0; chI != MAX_PASSW; chI++) // DEBUG Подстраховка для тестов с NetCat`а.
						{
							if(p_ProtoParser->oParsedObject.oProtocolStorage.oPassword.m_chPassw[(int)chI] == 0x0A)
							{
								p_ProtoParser->oParsedObject.oProtocolStorage.oPassword.m_chPassw[(int)chI] = 0;
								break;
							}
						}
						if(!strcmp(p_chPassword, p_ProtoParser->oParsedObject.oProtocolStorage.oPassword.m_chPassw))
						{
							SendToAddress(oConnectionData, PROTO_S_PASSW_OK);
							mThreadDadas[iTPos].bSecured = true;
							Z_LOG(LOG_CAT_I, m_chPasswOk << ": " << m_chNameBuffer);
							break;
						}
						else
						{
							SendToAddress(oConnectionData, PROTO_S_PASSW_ERR);
							Z_LOG(LOG_CAT_W, m_chPasswError << ": " << m_chNameBuffer);
							break;
						}
					}
					else
					{
						SendToAddress(oConnectionData, PROTO_S_UNSECURED);
						Z_LOG(LOG_CAT_W, m_chPasswAbsent << ": " << m_chNameBuffer);
						break;
					}
				}
				else
				{
					switch(p_ProtoParser->oParsedObject.chTypeCode)
					{
						case PROTO_O_TEXT_MSG:
						{
							Z_LOG(LOG_CAT_I, "Received text message: " <<
								  p_ProtoParser->oParsedObject.oProtocolStorage.oTextMsg.m_chMsg);
							break;
						}
					}
				}
				break;
			}
			case PROTOPARSER_OUT_OF_RANGE:
			{
				SendToAddress(oConnectionData, PROTO_S_OUT_OF_RANGE);
				Z_LOG(LOG_CAT_E, m_chPockenOutOfRange << ": " <<
					  m_chNameBuffer << " - " << p_ProtoParser->oParsedObject.iDataLength);
				break;
			}
			case PROTOPARSER_UNKNOWN_COMMAND:
			{
				SendToAddress(oConnectionData, PROTO_S_UNKNOWN_COMMAND);
				Z_LOG(LOG_CAT_W, m_chUnknownCommand << ": " << m_chNameBuffer);
				break;
			}
		}
		pthread_mutex_unlock(&ptConnMutex);
	}
	pthread_mutex_lock(&ptConnMutex);
ecd:delete p_ProtoParser;
	//
ec: if(bExitSignal == false)
	{
#ifndef WIN32
		shutdown(oConnectionData.iSocket, SHUT_RDWR);
		close(oConnectionData.iSocket);
#else
		closesocket(oConnectionData.iSocket);
#endif
		Z_LOG(LOG_CAT_I, "Socket closed by client absence: " << m_chNameBuffer);
	}
enc:
#ifndef WIN32
	Z_LOG(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread);
#else
	Z_LOG(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	CleanThrDadaPos(iTPos);
	pthread_mutex_unlock(&ptConnMutex);
	if(bKillListenerAccept) bListenerAlive = false;
	pthread_exit(p_vNum);
	return 0;
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
	char m_chNameBuffer[INET6_ADDRSTRLEN];
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
	FIND_IN_CHILDLIST(o_lNet.front(), p_ListPassword, "Password",
					  FCN_ONE_LEVEL, p_NodePassword)
	{
		p_chPassword = ((char*)p_NodePassword->FirstChild()->Value());
	} FIND_IN_CHILDLIST_END(p_ListPassword);
	if(p_chPassword == 0)
	{
		LOG(LOG_CAT_E, "Configuration file is corrupt! No 'Password' node");
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
	Z_LOG(LOG_CAT_I, "Accepting connections, press 'Esc' for exit");
	bListenerAlive = true;
	// Цикл ожидания входа клиентов.
nc:	bRequestNewConn = false; // Вход в звено цикла ожидания клиентов - сброс флага запроса.
	iCurrPos = FindFreeThrDadaPos();
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
		MSleep(USER_RESPONSE_MS);
	}
	printf("\b\b");
	Z_LOG(LOG_CAT_I, "Terminated by user.");
	// Закрытие приёмника.
	Z_LOG(LOG_CAT_I, "Closing listener...");
#ifndef WIN32
	shutdown(iListener, SHUT_RDWR);
	close(iListener);
#else
	closesocket(iListener);
#endif
	while(bListenerAlive)
	{
		MSleep(USER_RESPONSE_MS);
	}
	Z_LOG(LOG_CAT_I, "Listener has been closed");
	// Закрытие сокетов клиентов.
	Z_LOG(LOG_CAT_I, "Disconnecting clients...");
	pthread_mutex_lock(&ptConnMutex);
	for(iCurrPos = 0; iCurrPos != MAX_CONN; iCurrPos++) // Закрываем все клиентские сокеты.
	{
		if(mThreadDadas[iCurrPos].bInUse == true)
		{
#ifndef WIN32
			shutdown(mThreadDadas[iCurrPos].iConnection, SHUT_RDWR);
			close(mThreadDadas[iCurrPos].iConnection);
#else
			closesocket(mThreadDadas[iCurrPos].iConnection);
#endif
#ifndef WIN32
			getnameinfo(&mThreadDadas[iCurrPos].saInet, mThreadDadas[iCurrPos].ai_addrlen,
					m_chNameBuffer, sizeof(m_chNameBuffer), 0, 0, NI_NUMERICHOST);
#else
			getnameinfo(&mThreadDadas[iCurrPos].saInet, (socklen_t)mThreadDadas[iCurrPos].ai_addrlen,
					m_chNameBuffer, sizeof(m_chNameBuffer), 0, 0, NI_NUMERICHOST);
#endif
			Z_LOG(LOG_CAT_I, "Socket closed internally: " << m_chNameBuffer);
		}
	}
	pthread_mutex_unlock(&ptConnMutex);
	// Ждём, пока дойдёт до всех потоков соединений с клиентами.
stc:iCurrPos = 0;
	for(; iCurrPos != MAX_CONN; iCurrPos++)
	{
		// Если занят - на начало.
		if(mThreadDadas[iCurrPos].bInUse == true)
		{
			MSleep(USER_RESPONSE_MS);
			goto stc;
		}
	}
	Z_LOG(LOG_CAT_I, "Clients has been disconnected");
ex:
#ifdef WIN32
		WSACleanup();
#endif
	Z_LOG(LOG_CAT_I, "Exiting program");
	LOGCLOSE;
	return _uiRetval;
}
