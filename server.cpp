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
char* Server::p_chSettingsPath;

//== ФУНКЦИИ КЛАССОВ.
//== Класс сервера.
// Конструктор.
Server::Server(const char* cp_chSettingsPath, pthread_mutex_t ptLogMutex)
{
	LOG_CTRL_BIND_EXT_MUTEX(ptLogMutex);
	LOG_CTRL_INIT;
	p_chSettingsPath = (char*)cp_chSettingsPath;
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

// Отправка пакета пользователю.
bool Server::SendToUser(char* p_chIP, char chCommand, char* p_chBuffer, int iLength)
{
	unsigned int uiPos = 0;
	char m_chNameBuffer[INET6_ADDRSTRLEN];
	//
	pthread_mutex_lock(&ptConnMutex);
	for(; uiPos != MAX_CONN; uiPos++)
	{
		if((mThreadDadas[uiPos].bOverflowOnClient == false) & (mThreadDadas[uiPos].bInUse))
		{
#ifndef WIN32
			getnameinfo(&mThreadDadas[uiPos].oConnectionData.ai_addr,
						mThreadDadas[uiPos].oConnectionData.ai_addrlen,
						m_chNameBuffer, sizeof(m_chNameBuffer), 0, 0, NI_NUMERICHOST);
#else
			getnameinfo(&mThreadDadas[uiPos].oConnectionData.ai_addr,
						(socklen_t)mThreadDadas[uiPos].oConnectionData.ai_addrlen,
						m_chNameBuffer, sizeof(m_chNameBuffer), NULL, NULL, NI_NUMERICHOST);
#endif
			if(!strcmp(m_chNameBuffer, p_chIP))
			{
				SendToAddress(mThreadDadas[uiPos].oConnectionData,
							  chCommand, p_chBuffer, iLength);
				pthread_mutex_unlock(&ptConnMutex);
				return true;
			}
		}
	}
	pthread_mutex_unlock(&ptConnMutex);
	return false;
}

// Очистка позиции данных потока.
void Server::CleanThrDadaPos(unsigned int uiPos)
{
	memset(&mThreadDadas[uiPos], 0, sizeof(ConversationThreadData));
}

// Поиск свободной позиции данных потока.
unsigned int Server::FindFreeThrDadaPos()
{
	unsigned int uiPos = 0;
	//
	pthread_mutex_lock(&ptConnMutex);
	for(; uiPos != MAX_CONN; uiPos++)
	{
		if(mThreadDadas[uiPos].bInUse == false) // Если не стоит флаг занятости - годен.
		{
			pthread_mutex_unlock(&ptConnMutex);
			return uiPos;
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
	unsigned int uiTPos;
	bool bKillListenerAccept;
	ProtoParser* p_ProtoParser;
	ProtoParser::ParseResult oParsingResult;
	char m_chNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortBuffer[6];
	ProtoParser::ParsedObject* pParsedObject;
	bool bLocalExitSignal;
	//
	bLocalExitSignal = false;
	uiTPos = *((unsigned int*)p_vNum); // Получили номер в массиве.
	mThreadDadas[uiTPos].p_Thread = pthread_self(); // Задали ссылку на текущий поток.
#ifndef WIN32
	LOG_P(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[uiTPos].p_Thread);
#else
	LOG_P(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[uiTPos].p_Thread.p);
#endif
	bKillListenerAccept = false;
	mThreadDadas[uiTPos].oConnectionData.iSocket = (int)accept(iListener, NULL, NULL); // Ждём входящих.
	if((mThreadDadas[uiTPos].oConnectionData.iSocket < 0)) // При ошибке после выхода из ожидания входящих...
	{
		if(!bExitSignal) // Если не было сигнала на выход от основного потока...
		{
			pthread_mutex_lock(&ptConnMutex);
			LOG_P(LOG_CAT_E, "'accept': " << gai_strerror(mThreadDadas[uiTPos].oConnectionData.iSocket)); // Про ошибку.
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
	mThreadDadas[uiTPos].bInUse = true; // Флаг занятости структуры.
	LOG_P(LOG_CAT_I, "New connection accepted.");
	mThreadDadas[uiTPos].iConnection = mThreadDadas[uiTPos].oConnectionData.iSocket; // Установка ИД соединения.
	mThreadDadas[uiTPos].oConnectionData.ai_addrlen = sizeof(sockaddr);
#ifndef WIN32
	getpeername(mThreadDadas[uiTPos].oConnectionData.iSocket, &mThreadDadas[uiTPos].oConnectionData.ai_addr,
				&mThreadDadas[uiTPos].oConnectionData.ai_addrlen);
#else
	getpeername(mThreadDadas[uiTPos].oConnectionData.iSocket, &mThreadDadas[uiTPos].oConnectionData.ai_addr,
				(int*)&mThreadDadas[uiTPos].oConnectionData.ai_addrlen);
#endif
#ifndef WIN32
	getnameinfo(&mThreadDadas[uiTPos].oConnectionData.ai_addr,
				mThreadDadas[uiTPos].oConnectionData.ai_addrlen,
				m_chNameBuffer, sizeof(m_chNameBuffer), m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#else
	getnameinfo(&mThreadDadas[uiTPos].oConnectionData.ai_addr,
				(socklen_t)mThreadDadas[uiTPos].oConnectionData.ai_addrlen,
				m_chNameBuffer, sizeof(m_chNameBuffer), m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#endif
	LOG_P(LOG_CAT_I, "Connected with: " << m_chNameBuffer << " : " << m_chPortBuffer); // Инфо про входящий IP.
	if(mThreadDadas[uiTPos].oConnectionData.iStatus == -1) // Если не вышло отправить...
	{
		pthread_mutex_lock(&ptConnMutex);
		LOG_P(LOG_CAT_E, "'send': " << gai_strerror(mThreadDadas[uiTPos].oConnectionData.iStatus));
		goto ec;
	}
	//
	bRequestNewConn = true; // Соединение готово - установка флага для главного потока на запрос нового.
	p_ProtoParser = new ProtoParser;
	while(bExitSignal == false) // Пока не пришёл флаг общего завершения...
	{
		pthread_mutex_lock(&ptConnMutex);
		if(mThreadDadas[uiTPos].uiCurrentFreePocket >= S_MAX_STORED_POCKETS)
		{
			LOG_P(LOG_CAT_E, (char*)S_BUFFER_OVERFLOW << ": " << m_chNameBuffer);
			mThreadDadas[uiTPos].bOverflowOnServer = true;
			SendToAddress(mThreadDadas[uiTPos].oConnectionData, PROTO_S_BUFFER_OVERFLOW);
			mThreadDadas[uiTPos].uiCurrentFreePocket = S_MAX_STORED_POCKETS - 1;
		}
		pthread_mutex_unlock(&ptConnMutex);
		mThreadDadas[uiTPos].oConnectionData.iStatus =  // Принимаем пакет в текущую позицию.
				recv(mThreadDadas[uiTPos].oConnectionData.iSocket,
					 mThreadDadas[uiTPos].m_chData,
				sizeof(mThreadDadas[uiTPos].m_chData), 0);
		pthread_mutex_lock(&ptConnMutex);
		if(mThreadDadas[uiTPos].bOverflowOnServer == true) // DEBUG.
		{
			LOG_P(LOG_CAT_W, "Received overflowed pocket.");
		}
		if (bExitSignal == true) // Если по выходу из приёмки обнаружен общий сигнал на выход...
		{
			LOG_P(LOG_CAT_I, "Exiting reading: " << m_chNameBuffer);
			goto ecd;
		}
		if (mThreadDadas[uiTPos].oConnectionData.iStatus <= 0) // Если статус приёмки - отказ (вместо принятых байт)...
		{
			LOG_P(LOG_CAT_I, "Reading socket has been stopped: " << m_chNameBuffer);
			goto ecd;
		}
		oParsingResult = p_ProtoParser->ParsePocket(
					mThreadDadas[uiTPos].m_chData,
					mThreadDadas[uiTPos].oConnectionData.iStatus,
					mThreadDadas[uiTPos].mReceivedPockets[mThreadDadas[uiTPos].uiCurrentFreePocket].oParsedObject,
					mThreadDadas[uiTPos].bOverflowOnServer);
		pParsedObject = &mThreadDadas[uiTPos].mReceivedPockets[mThreadDadas[uiTPos].uiCurrentFreePocket].oParsedObject;
		switch(oParsingResult.chRes)
		{
			case PROTOPARSER_OK:
			{
				if(oParsingResult.bStored)
				{
					LOG_P(LOG_CAT_I, "Received pocket nr." <<
						(mThreadDadas[uiTPos].uiCurrentFreePocket + 1) << " of " << S_MAX_STORED_POCKETS);
					mThreadDadas[uiTPos].mReceivedPockets[mThreadDadas[uiTPos].uiCurrentFreePocket].bProcessed = false;
				}
				if(mThreadDadas[uiTPos].bSecured == false)
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
							SendToAddress(mThreadDadas[uiTPos].oConnectionData, PROTO_S_PASSW_OK);
							mThreadDadas[uiTPos].bSecured = true;
							LOG_P(LOG_CAT_I, (char*)PASSW_OK << ": " << m_chNameBuffer);
							break;
						}
						else
						{
							SendToAddress(mThreadDadas[uiTPos].oConnectionData, PROTO_S_PASSW_ERR);
							LOG_P(LOG_CAT_W, (char*)PASSW_ERROR << ": " << m_chNameBuffer);
							break;
						}
					}
					else
					{
						SendToAddress(mThreadDadas[uiTPos].oConnectionData, PROTO_S_UNSECURED);
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
							mThreadDadas[uiTPos].bOverflowOnClient = true;
							goto gI;
						}
						case PROTO_C_BUFFER_READY:
						{
							LOG_P(LOG_CAT_I, (char*)C_BUFFER_READY);
							mThreadDadas[uiTPos].bOverflowOnClient = false;
							goto gI;
						}
						case PROTO_C_REQUEST_LEAVING:
						{
							LOG_P(LOG_CAT_I, m_chNameBuffer << " : request leaving.");
							// МЕСТО ДЛЯ ВСТАВКИ КОДА ОБРАБОТКИ ВЫХОДА КЛИЕНТОВ.
							//
							SendToAddress(mThreadDadas[uiTPos].oConnectionData, PROTO_S_ACCEPT_LEAVING);

							LOG_P(LOG_CAT_I, m_chNameBuffer << " : leaving accepted.");
							MSleep(WAITING_FOR_CLIENT_DSC);
							bLocalExitSignal = true;
							goto ecd;
						}
					}
					// Блок объектов.
					if(mThreadDadas[uiTPos].bOverflowOnServer == false)
					{
						switch(pParsedObject->chTypeCode)
						{
							case PROTO_O_TEXT_MSG:
							{
								LOG_P(LOG_CAT_I, "Received text message: " <<
									  pParsedObject->oProtocolStorage.oTextMsg.m_chMsg);
								break;
							}
						}
					}
				}
gI:				break;
			}
			case PROTOPARSER_OUT_OF_RANGE:
			{
				SendToAddress(mThreadDadas[uiTPos].oConnectionData, PROTO_S_OUT_OF_RANGE);
				LOG_P(LOG_CAT_E, (char*)POCKET_OUT_OF_RANGE << ": " <<
					  m_chNameBuffer << " - " << pParsedObject->iDataLength);
				break;
			}
			case PROTOPARSER_UNKNOWN_COMMAND:
			{
				SendToAddress(mThreadDadas[uiTPos].oConnectionData, PROTO_S_UNKNOWN_COMMAND);
				LOG_P(LOG_CAT_W, (char*)UNKNOWN_COMMAND << ": " << m_chNameBuffer);
				break;
			}
		}
		if((mThreadDadas[uiTPos].bOverflowOnServer == false) &&
		   oParsingResult.bStored) mThreadDadas[uiTPos].uiCurrentFreePocket++;
		pthread_mutex_unlock(&ptConnMutex);
	}
	pthread_mutex_lock(&ptConnMutex);
ecd:delete p_ProtoParser;
	//
ec: if((bLocalExitSignal || bExitSignal) == false)
	{
#ifndef WIN32
		shutdown(mThreadDadas[uiTPos].oConnectionData.iSocket, SHUT_RDWR);
		close(mThreadDadas[uiTPos].oConnectionData.iSocket);
#else
		closesocket(mThreadDadas[uiTPos].oConnectionData.iSocket);
#endif
		LOG_P(LOG_CAT_W, "Socket closed by client absence: " << m_chNameBuffer);
	}
	else
	{
		LOG_P(LOG_CAT_I, "Socket closed ordinary: " << m_chNameBuffer);
	}
enc:
#ifndef WIN32
	LOG_P(LOG_CAT_I, "Exiting thread: " << mThreadDadas[uiTPos].p_Thread);
#else
	LOG_P(LOG_CAT_I, "Exiting thread: " << mThreadDadas[uiTPos].p_Thread.p);
#endif
	CleanThrDadaPos(uiTPos);
	mThreadDadas[uiTPos].bInUse = false;
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
	unsigned int uiCurrPos = 0;
	char m_chNameBuffer[INET6_ADDRSTRLEN];
#ifdef WIN32
	WSADATA wsadata = WSADATA();
#endif
	//
	LOG_P(LOG_CAT_I, "Server thread has been started.");
	//
	memset(&mThreadDadas, 0, sizeof mThreadDadas);
	// Работа с файлом конфигурации.
	eResult = xmlDocSConf.LoadFile(p_chSettingsPath);
	if (eResult != XML_SUCCESS)
	{
		LOG_P(LOG_CAT_E, "Can`t open configuration file:" << p_chSettingsPath);
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
	uiCurrPos = FindFreeThrDadaPos();
	pthread_create(&mThreadDadas[uiCurrPos].p_Thread, NULL,
				   ConversationThread, &uiCurrPos); // Создание нового потока приёмки.
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
	for(uiCurrPos = 0; uiCurrPos != MAX_CONN; uiCurrPos++) // Закрываем все клиентские сокеты.
	{
		if(mThreadDadas[uiCurrPos].bInUse == true)
		{
			SendToAddress(mThreadDadas[uiCurrPos].oConnectionData, PROTO_S_SHUTDOWN_INFO);
		}
	}
	MSleep(WAITING_FOR_CLIENT_DSC * 2);
	pthread_mutex_lock(&ptConnMutex);
	for(uiCurrPos = 0; uiCurrPos != MAX_CONN; uiCurrPos++) // Закрываем все клиентские сокеты.
	{
		if(mThreadDadas[uiCurrPos].bInUse == true)
		{
#ifndef WIN32
			shutdown(mThreadDadas[uiCurrPos].iConnection, SHUT_RDWR);
			close(mThreadDadas[uiCurrPos].iConnection);
#else
			closesocket(mThreadDadas[uiCurrPos].iConnection);
#endif
#ifndef WIN32
			getnameinfo(&mThreadDadas[uiCurrPos].oConnectionData.ai_addr,
						mThreadDadas[uiCurrPos].oConnectionData.ai_addrlen,
						m_chNameBuffer, sizeof(m_chNameBuffer), 0, 0, NI_NUMERICHOST);
#else
			getnameinfo(&mThreadDadas[uiCurrPos].oConnectionData.ai_addr,
						(socklen_t)mThreadDadas[uiCurrPos].oConnectionData.ai_addrlen,
						m_chNameBuffer, sizeof(m_chNameBuffer), 0, 0, NI_NUMERICHOST);
#endif
			LOG_P(LOG_CAT_I, "Socket closed internally: " << m_chNameBuffer);
		}
	}
	pthread_mutex_unlock(&ptConnMutex);
	// Ждём, пока дойдёт до всех потоков соединений с клиентами.
stc:uiCurrPos = 0;
	for(; uiCurrPos != MAX_CONN; uiCurrPos++)
	{
		// Если занят - на начало.
		if(mThreadDadas[uiCurrPos].bInUse == true)
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
	LOG_P(LOG_CAT_I, "Exiting server thread.");
	bExitSignal = false;
	RETURN_THREAD;
}
