//== ВКЛЮЧЕНИЯ.

#include <server.h>

//== МАКРОСЫ.
#define LOG_NAME				"Z-Server"
#define PASSW_ERROR				"Authentification failed."
#define PASSW_ABSENT			"Client is not autherised."
#define PASSW_OK				"Connection is secured."
#define S_BUFFER_OVERFLOW		"Buffer overflow for"
#define C_BUFFER_OVERFLOW		"Buffer overflow on"

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL_INIT_INCLASS(Server)
LOGDECL_INIT_PTHRD_INCLASS_EXT_ADD(Server)
bool Server::bExitSignal = false;
pthread_mutex_t Server::ptConnMutex = PTHREAD_MUTEX_INITIALIZER;
int Server::iListener;
bool Server::bRequestNewConn;
Server::ConversationThreadData Server::mThreadDadas[MAX_CONN];
bool Server::bListenerAlive;
char* Server::p_chPassword = 0;
pthread_t Server::ServerThr;

//== ФУНКЦИИ КЛАССОВ.
//== Класс сервера.
// Конструктор.
Server::Server(pthread_mutex_t ptLogMutex)
{
	LOG_CTRL_BIND_EXT_MUTEX(ptLogMutex);
	LOG_CTRL_INIT;
}

// Деструктор.
Server::~Server()
{
	LOGCLOSE;
}

// Запрос запуска клиента.
void Server::Start()
{
	bExitSignal = false;
	LOG_P(LOG_CAT_I, "Starting server thread.");
	pthread_create(&ServerThr, NULL, ServerThread, NULL);
}

// Запрос остановки клиента.
void Server::Stop()
{
	bExitSignal = true;
}

// Запрос статуса лога.
unsigned int Server::GetLogStatus()
{
	return LOGRETVAL;
}

// Запрос готовности.
bool Server::CheckReady()
{
	return !bExitSignal;
}

// Функция отправки клиенту.
void Server::SendToClient(bool bOverflowFlag, ConnectionData &oConnectionData, char chCommand, char *p_chBuffer, int iLength)
{
	if(bOverflowFlag == false)
	{
		SendToAddress(oConnectionData, chCommand, p_chBuffer, iLength);
	}
	else
	{
		LOG_P(LOG_CAT_E, "Client buffer is overflowed.");
	}
}

// Очистка позиции данных потока.
void Server::CleanThrDadaPos(int iPos)
{
	memset(&mThreadDadas[iPos], 0, sizeof(ConversationThreadData));
}

// Поиск свободной позиции данных потока.
int Server::FindFreeThrDadaPos()
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

// Поток соединения.
void* Server::ConversationThread(void* p_vNum)
{
	///< \param[in] p_vNum Ук. на переменную типа int с номером предоставленной структуры в mThreadDadas.
	///< \return Заглушка.
	int iTPos;
	bool bKillListenerAccept;
	ProtoParser* p_ProtoParser;
	ProtoParser::ParseResult oParsingResult;
	ConnectionData oConnectionData;
	char m_chNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortBuffer[6];
	ProtoParser::ParsedObject* pParsedObject;
	//
	iTPos = *((int*)p_vNum); // Получили номер в массиве.
	mThreadDadas[iTPos].p_Thread = pthread_self(); // Задали ссылку на текущий поток.
#ifndef WIN32
	LOG_P(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread);
#else
	LOG_P(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	bKillListenerAccept = false;
	oConnectionData.iSocket = (int)accept(iListener, NULL, NULL); // Ждём входящих.
	if((oConnectionData.iSocket < 0)) // При ошибке после выхода из ожидания входящих...
	{
		if(!bExitSignal) // Если не было сигнала на выход от основного потока...
		{
			pthread_mutex_lock(&ptConnMutex);
			LOG_P(LOG_CAT_E, "'accept': " << gai_strerror(oConnectionData.iSocket)); // Про ошибку.
			goto enc;
		}
		else
		{
			pthread_mutex_lock(&ptConnMutex);
			LOG_P(LOG_CAT_I, "Accepting connections terminated."); // Норм. сообщение.
			bKillListenerAccept = true; // Заказываем подтверждение закрытия приёмника.
			goto enc;
		}
	}
	mThreadDadas[iTPos].bInUse = true; // Флаг занятости структуры.
	LOG_P(LOG_CAT_I, "New connection accepted.");
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
				m_chNameBuffer, sizeof(m_chNameBuffer), m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#else
	getnameinfo(&mThreadDadas[iTPos].saInet, (socklen_t)mThreadDadas[iTPos].ai_addrlen,
				m_chNameBuffer, sizeof(m_chNameBuffer), m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#endif
	LOG_P(LOG_CAT_I, "Connected with: " << m_chNameBuffer << " : " << m_chPortBuffer); // Инфо про входящий IP.
	if(oConnectionData.iStatus == -1) // Если не вышло отправить...
	{
		pthread_mutex_lock(&ptConnMutex);
		LOG_P(LOG_CAT_E, "'send': " << gai_strerror(oConnectionData.iStatus));
		goto ec;
	}
	//
	bRequestNewConn = true; // Соединение готово - установка флага для главного потока на запрос нового.
	p_ProtoParser = new ProtoParser;
	while(bExitSignal == false) // Пока не пришёл флаг общего завершения...
	{
		if(mThreadDadas[iTPos].uiCurrentPocket >= S_MAX_STORED_POCKETS)
		{
			LOG_P(LOG_CAT_E, (char*)S_BUFFER_OVERFLOW << ": " << m_chNameBuffer);
			mThreadDadas[iTPos].bOverflowOnServer = true;
			SendToAddress(oConnectionData, PROTO_S_BUFFER_OVERFLOW);
			mThreadDadas[iTPos].uiCurrentPocket = S_MAX_STORED_POCKETS - 1;
		}
		oConnectionData.iStatus =  // Принимаем пакет в текущую позицию.
				recv(oConnectionData.iSocket,
					 mThreadDadas[iTPos].m_chData,
				sizeof(mThreadDadas[iTPos].m_chData), 0);
		pthread_mutex_lock(&ptConnMutex);
		if(mThreadDadas[iTPos].bOverflowOnServer == true) // DEBUG.
		{
			LOG_P(LOG_CAT_W, "Received overflowed pocket.");
		}
		if (bExitSignal == true) // Если по выходу из приёмки обнаружен общий сигнал на выход...
		{
			LOG_P(LOG_CAT_I, "Exiting reading: " << m_chNameBuffer);
			goto ecd;
		}
		if (oConnectionData.iStatus <= 0) // Если статус приёмки - отказ (вместо кол-ва принятых байт)...
		{
			LOG_P(LOG_CAT_I, "Reading socket has been stopped: " << m_chNameBuffer);
			goto ecd;
		}
		oParsingResult = p_ProtoParser->ParsePocket(
					mThreadDadas[iTPos].m_chData,
				oConnectionData.iStatus, mThreadDadas[iTPos].mReceivedPockets[mThreadDadas[iTPos].uiCurrentPocket].oParsedObject,
				mThreadDadas[iTPos].bOverflowOnServer);
		pParsedObject = &mThreadDadas[iTPos].mReceivedPockets[mThreadDadas[iTPos].uiCurrentPocket].oParsedObject;
		switch(oParsingResult.chRes)
		{
			case PROTOPARSER_OK:
			{
				if(oParsingResult.bStored)
				{
					LOG_P(LOG_CAT_I, "Received pocket nr." <<
						(mThreadDadas[iTPos].uiCurrentPocket + 1) << " of " << S_MAX_STORED_POCKETS);
				}
				if(mThreadDadas[iTPos].bSecured == false)
				{
					if(pParsedObject->chTypeCode == PROTO_C_SEND_PASSW)
					{
						for(char chI = 0; chI != MAX_PASSW; chI++) // DEBUG Подстраховка для тестов с NetCat`а.
						{
							if(pParsedObject->oProtocolStorage.oPassword.m_chPassw[(int)chI] == 0x0A)
							{
								pParsedObject->oProtocolStorage.oPassword.m_chPassw[(int)chI] = 0;
								break;
							}
						}
						if(!strcmp(p_chPassword, pParsedObject->oProtocolStorage.oPassword.m_chPassw))
						{
							SendToAddress(oConnectionData, PROTO_S_PASSW_OK);
							mThreadDadas[iTPos].bSecured = true;
							LOG_P(LOG_CAT_I, (char*)PASSW_OK << ": " << m_chNameBuffer);
							break;
						}
						else
						{
							SendToAddress(oConnectionData, PROTO_S_PASSW_ERR);
							LOG_P(LOG_CAT_W, (char*)PASSW_ERROR << ": " << m_chNameBuffer);
							break;
						}
					}
					else
					{
						SendToAddress(oConnectionData, PROTO_S_UNSECURED);
						LOG_P(LOG_CAT_W, (char*)PASSW_ABSENT << ": " << m_chNameBuffer);
						break;
					}
				}
				else
				{
					// Блок взаимодействия.
					switch(pParsedObject->chTypeCode)
					{
						case PROTO_C_BUFFER_OVERFLOW:
						{
							LOG_P(LOG_CAT_E, (char*)C_BUFFER_OVERFLOW << ": " << m_chNameBuffer);
							mThreadDadas[iTPos].bOverflowOnClient = true;
							goto gI;
						}
						case PROTO_C_BUFFER_READY:
						{
							LOG_P(LOG_CAT_I, (char*)C_BUFFER_READY);
							mThreadDadas[iTPos].bOverflowOnClient = false;
							goto gI;
						}
					}
					// Блок объектов.
					if(mThreadDadas[iTPos].bOverflowOnServer == false)
					{
						switch(pParsedObject->chTypeCode)
						{
							case PROTO_O_TEXT_MSG:
							{
								LOG_P(LOG_CAT_I, "Received text message: " <<
									  pParsedObject->oProtocolStorage.oTextMsg.m_chMsg);
								LOG_P(LOG_CAT_I, "Sending answer..."); // DEBUG.
								SendToClient(mThreadDadas[iTPos].bOverflowOnClient, oConnectionData,
											 PROTO_O_TEXT_MSG, (char*)"Got text.", 10); // DEBUG.
								break;
							}
						}
					}
				}
gI:				break;
			}
			case PROTOPARSER_OUT_OF_RANGE:
			{
				SendToAddress(oConnectionData, PROTO_S_OUT_OF_RANGE);
				LOG_P(LOG_CAT_E, (char*)POCKET_OUT_OF_RANGE << ": " <<
					  m_chNameBuffer << " - " << pParsedObject->iDataLength);
				break;
			}
			case PROTOPARSER_UNKNOWN_COMMAND:
			{
				SendToAddress(oConnectionData, PROTO_S_UNKNOWN_COMMAND);
				LOG_P(LOG_CAT_W, (char*)UNKNOWN_COMMAND << ": " << m_chNameBuffer);
				break;
			}
		}
		if((mThreadDadas[iTPos].bOverflowOnServer == false) && oParsingResult.bStored) mThreadDadas[iTPos].uiCurrentPocket++;
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
		LOG_P(LOG_CAT_I, "Socket closed by client absence: " << m_chNameBuffer);
	}
enc:
#ifndef WIN32
	LOG_P(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread);
#else
	LOG_P(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	CleanThrDadaPos(iTPos);
	pthread_mutex_unlock(&ptConnMutex);
	if(bKillListenerAccept) bListenerAlive = false;
	RETURN_THREAD;
}

// Серверный поток.
void* Server::ServerThread(void *p_vPlug)
{
	p_vPlug = p_vPlug;
	tinyxml2::XMLDocument xmlDocSConf;
	list <XMLNode*> o_lNet;
	XMLError eResult;
	_uiRetval = _uiRetval; // Заглушка.
	int iServerStatus;
	addrinfo o_Hints;
	char* p_chServerIP = 0;
	char* p_chPort = 0;
	addrinfo* p_Res;
	int iCurrPos = 0;
	char m_chNameBuffer[INET6_ADDRSTRLEN];
#ifdef WIN32
	WSADATA wsadata = WSADATA();
#endif
	//
	LOG_P(LOG_CAT_I, "Server thread has been started.");
	//
	memset(&mThreadDadas, 0, sizeof mThreadDadas);
	// Работа с файлом конфигурации.
	eResult = xmlDocSConf.LoadFile(S_CONF_PATH);
	if (eResult != XML_SUCCESS)
	{
		LOG_P(LOG_CAT_E, "Can`t open configuration file:" << S_CONF_PATH);
		RETVAL_SET(RETVAL_ERR);
		RETURN_THREAD;
	}
	else
	{
		LOG_P(LOG_CAT_I, "Configuration loaded.");
	}
	if(!FindChildNodes(xmlDocSConf.LastChild(), o_lNet,
					   "Net", FCN_ONE_LEVEL, FCN_FIRST_ONLY))
	{
		LOG_P(LOG_CAT_E, "Configuration file is corrupt! No 'Net' node.");
		RETVAL_SET(RETVAL_ERR);
		RETURN_THREAD;
	}
	FIND_IN_CHILDLIST(o_lNet.front(), p_ListServerIP, "IP",
					  FCN_ONE_LEVEL, p_NodeServerIP)
	{
		p_chServerIP = (char*)p_NodeServerIP->FirstChild()->Value();
	} FIND_IN_CHILDLIST_END(p_ListServerIP);
	if(p_chServerIP != 0)
	{
		LOG_P(LOG_CAT_I, "Server IP: " << p_chServerIP);
	}
	else
	{
		LOG_P(LOG_CAT_E, "Configuration file is corrupt! No '(Net)IP' node.");
		RETVAL_SET(RETVAL_ERR);
		RETURN_THREAD;
	}
	FIND_IN_CHILDLIST(o_lNet.front(), p_ListPort, "Port",
					  FCN_ONE_LEVEL, p_NodePort)
	{
		p_chPort = (char*)p_NodePort->FirstChild()->Value();
	} FIND_IN_CHILDLIST_END(p_ListPort);
	if(p_chPort != 0)
	{
		LOG_P(LOG_CAT_I, "Port: " << p_chPort);
	}
	else
	{
		LOG_P(LOG_CAT_E, "Configuration file is corrupt! No '(Net)Port' node.");
		RETVAL_SET(RETVAL_ERR);
		RETURN_THREAD;
	}
	FIND_IN_CHILDLIST(o_lNet.front(), p_ListPassword, "Password",
					  FCN_ONE_LEVEL, p_NodePassword)
	{
		p_chPassword = ((char*)p_NodePassword->FirstChild()->Value());
	} FIND_IN_CHILDLIST_END(p_ListPassword);
	if(p_chPassword == 0)
	{
		LOG_P(LOG_CAT_E, "Configuration file is corrupt! No 'Password' node.");
		RETVAL_SET(RETVAL_ERR);
		RETURN_THREAD;
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
		LOG_P(LOG_CAT_E, "'getaddrinfo': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iListener = (int)socket(p_Res->ai_family, p_Res->ai_socktype, p_Res->ai_protocol); // Получение сокета для приёмника.
	if(iListener < 0 )
	{
		LOG_P(LOG_CAT_E, "'socket': "  << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = bind(iListener, p_Res->ai_addr, (int)p_Res->ai_addrlen); // Привязка к указанному IP.
	if(iServerStatus < 0)
	{
		LOG_P(LOG_CAT_E, "'bind': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = listen(iListener, 10); // Запуск приёмника.
	if(iServerStatus < 0)
	{
		LOG_P(LOG_CAT_E, "'listen': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	freeaddrinfo(p_Res);
	//
	LOG_P(LOG_CAT_I, "Accepting connections...");
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
	LOG_P(LOG_CAT_I, "Terminated by admin.");
	// Закрытие приёмника.
	LOG_P(LOG_CAT_I, "Closing listener...");
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
	LOG_P(LOG_CAT_I, "Listener has been closed.");
	// Закрытие сокетов клиентов.
	LOG_P(LOG_CAT_I, "Disconnecting clients...");
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
			LOG_P(LOG_CAT_I, "Socket closed internally: " << m_chNameBuffer);
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
	LOG_P(LOG_CAT_I, "Clients has been disconnected.");
ex:
#ifdef WIN32
	WSACleanup();
#endif
	LOG_P(LOG_CAT_I, "Exiting program.");
	bExitSignal = false;
	RETURN_THREAD;
}
