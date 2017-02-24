//== ВКЛЮЧЕНИЯ.

#include <Server/server.h>

//== МАКРОСЫ.
#define LOG_NAME				"Z-Server"

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL_INIT_INCLASS(Server)
LOGDECL_INIT_PTHRD_INCLASS_EXT_ADD(Server)
bool Server::bExitSignal = false;
pthread_mutex_t Server::ptConnMutex = PTHREAD_MUTEX_INITIALIZER;
int Server::iListener = 0;
bool Server::bRequestNewConn = false;
Server::ConversationThreadData Server::mThreadDadas[MAX_CONN];
bool Server::bListenerAlive = false;
char* Server::p_chPassword = 0;
pthread_t Server::ServerThr;
char* Server::p_chSettingsPath = 0;
int Server::iSelectedConnection = -1;
CBClientStateChanged Server::pf_CBClientStateChanged = 0;
CBClientDataArrived Server::pf_CBClientDataArrived = 0;

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
int Server::GetLogStatus()
{
	return LOGRETVAL;
}

// Запрос готовности.
bool Server::CheckReady()
{
	return !bExitSignal;
}

// Функция отправки клиенту.
bool Server::SendToClient(ConnectionData &oConnectionData, char chCommand,
						  bool bFullFlag, char *p_chBuffer, int iLength)
{
	bool bRes = false;
	if(bFullFlag == false)
	{
		if(SendToAddress(oConnectionData, chCommand, p_chBuffer, iLength) == false)
		{
			LOG_P(LOG_CAT_E, "Socket error on sending data.");
		}
		else
		{
			bRes = true;
		}
	}
	else
	{
		LOG_P(LOG_CAT_E, "Client buffer is full.");
	}
	return bRes;
}

// Отправка пакета пользователю.
bool Server::SendToUser(char chCommand, char* p_chBuffer, int iLength)
{
	bool bRes = false;
	pthread_mutex_lock(&ptConnMutex);
	if(iSelectedConnection == CONNECTION_SEL_ERROR) goto gUE;
	if((mThreadDadas[iSelectedConnection].bFullOnClient == false))
	{
		if(SendToClient(mThreadDadas[iSelectedConnection].oConnectionData,
					  chCommand, false, p_chBuffer, iLength) == true)
			bRes = true;
	}
gUE:pthread_mutex_unlock(&ptConnMutex);
	if(bRes == false)
	{
		LOG_P(LOG_CAT_E, "Sending failed.");
	}
	return bRes;
}

// Установка указателя кэлбэка изменения статуса подключения клиента.
void Server::SetClientStateChangedCB(CBClientStateChanged pf_CBClientStateChangedIn)
{
	pthread_mutex_lock(&ptConnMutex);
	pf_CBClientStateChanged = pf_CBClientStateChangedIn;
	pthread_mutex_unlock(&ptConnMutex);
}

// Установка указателя кэлбэка обработки принятых пакетов от клиентов.
void Server::SetClientDataArrivedCB(CBClientDataArrived pf_CBClientDataArrivedIn)
{
	pthread_mutex_lock(&ptConnMutex);
	pf_CBClientDataArrived = pf_CBClientDataArrivedIn;
	pthread_mutex_unlock(&ptConnMutex);
}

// Установка текущего индекса осоединения для исходящих.
bool Server::SetCurrentConnection(unsigned int uiIndex)
{
	bool bRes = false;
	//
	if(uiIndex > (MAX_CONN - 1))
	{
		LOG_P(LOG_CAT_E, "Index is out of range.");
		return false;
	}
	pthread_mutex_lock(&ptConnMutex);
	if(mThreadDadas[uiIndex].bInUse == true)
	{
		iSelectedConnection = uiIndex;
		LOG_P(LOG_CAT_I, "Selected ID: " << uiIndex);
		bRes = true;
	}
	else
	{
		iSelectedConnection = CONNECTION_SEL_ERROR;
		LOG_P(LOG_CAT_E, "Selected ID is unused: " << uiIndex);
	}
	pthread_mutex_unlock(&ptConnMutex);
	return bRes;
}

// Удаление крайнего элемента из массива принятых пакетов.
char Server::ReleaseCurrentData()
{
	char chRes = BUFFER_IS_EMPTY;
	//
	pthread_mutex_lock(&ptConnMutex);
	if(iSelectedConnection != CONNECTION_SEL_ERROR)
	{
		if(mThreadDadas[iSelectedConnection].uiCurrentFreePocket > 0)
		{
			if(mThreadDadas[iSelectedConnection].bFullOnServer == false)
			{
				mThreadDadas[iSelectedConnection].uiCurrentFreePocket--;
			}
			else
			{
				mThreadDadas[iSelectedConnection].bFullOnServer = false;
				SendToClient(mThreadDadas[iSelectedConnection].oConnectionData, PROTO_A_BUFFER_READY);
			}
			if(mThreadDadas[iSelectedConnection].
			   mReceivedPockets[mThreadDadas[iSelectedConnection].uiCurrentFreePocket].bFresh == true)
			{
				mThreadDadas[iSelectedConnection].
						mReceivedPockets[mThreadDadas[iSelectedConnection].uiCurrentFreePocket].bFresh = false;
				mThreadDadas[iSelectedConnection].
						mReceivedPockets[mThreadDadas[iSelectedConnection].uiCurrentFreePocket].oProtocolStorage.CleanPointers();
				chRes = RETVAL_OK;
				LOG_P(LOG_CAT_I, "Position has been released.");
			}
		}
	}
	else
	{
		chRes = CONNECTION_SEL_ERROR;
		LOG_P(LOG_CAT_E, "Wrong connection number (release).");
	}
	pthread_mutex_unlock(&ptConnMutex);
	if(chRes == BUFFER_IS_EMPTY)
	{
		LOG_P(LOG_CAT_E, "Buffer is empty.");
	}
	return chRes;
}

// Доступ к крайнему элементу из массива принятых пакетов от текущего клиента.
char Server::AccessCurrentData(void** pp_vDataBuffer)
{
	char chRes = DATA_ACCESS_ERROR;
	unsigned int uiPos;
	//
	pthread_mutex_lock(&ptConnMutex);
	if(iSelectedConnection != CONNECTION_SEL_ERROR)
	{
		uiPos = mThreadDadas[iSelectedConnection].uiCurrentFreePocket;
		if(mThreadDadas[iSelectedConnection].uiCurrentFreePocket > 0)
		{
			if(mThreadDadas[iSelectedConnection].bFullOnServer == false) uiPos--;
			if(mThreadDadas[iSelectedConnection].mReceivedPockets[uiPos].bFresh == true)
			{
				*pp_vDataBuffer = mThreadDadas[iSelectedConnection].mReceivedPockets[uiPos].oProtocolStorage.GetPointer();
				chRes = mThreadDadas[iSelectedConnection].mReceivedPockets[uiPos].oProtocolStorage.chTypeCode;
			}
		}
	}
	else
	{
		chRes = CONNECTION_SEL_ERROR;
		LOG_P(LOG_CAT_E, "Wrong connection number (access).");
	}
	pthread_mutex_unlock(&ptConnMutex);
	if(chRes == DATA_ACCESS_ERROR)
	{
		LOG_P(LOG_CAT_E, "Data access error.");
	}
	return chRes;
}

// Очистка позиции данных потока.
void Server::CleanThrDadaPos(unsigned int uiPos)
{
	for(unsigned int uiC = 0; uiC < S_MAX_STORED_POCKETS; uiC++)
	{
		mThreadDadas[uiPos].mReceivedPockets[uiC].oProtocolStorage.CleanPointers();
	}
	memset(&mThreadDadas[uiPos], 0, sizeof(ConversationThreadData));
}

// Поиск свободной позиции данных потока.
int Server::FindFreeThrDadaPos()
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
	bool bLocalExitSignal;
	ReceivedData* p_CurrentData;
	//
	bLocalExitSignal = false;
	uiTPos = *((unsigned int*)p_vNum); // Получили номер в массиве.
	memset(&mThreadDadas[uiTPos].mReceivedPockets, 0, sizeof(mThreadDadas[uiTPos].mReceivedPockets));
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
	LOG_P(LOG_CAT_I, "Connected with: " << m_chNameBuffer << " ID: " << uiTPos);
	if(mThreadDadas[uiTPos].oConnectionData.iStatus == -1) // Если не вышло отправить...
	{
		pthread_mutex_lock(&ptConnMutex);
		LOG_P(LOG_CAT_E, "'send': " << gai_strerror(mThreadDadas[uiTPos].oConnectionData.iStatus));
		goto ec;
	}
	//
	bRequestNewConn = true; // Соединение готово - установка флага для главного потока на запрос нового.
	if(pf_CBClientStateChanged != 0) pf_CBClientStateChanged(uiTPos, true); // Вызов кэлбэка смены статуса.
	p_ProtoParser = new ProtoParser;
	while(bExitSignal == false) // Пока не пришёл флаг общего завершения...
	{
		pthread_mutex_lock(&ptConnMutex);
		if(mThreadDadas[uiTPos].uiCurrentFreePocket >= S_MAX_STORED_POCKETS)
		{
			LOG_P(LOG_CAT_W, "Buffer is full for ID: " << uiTPos);
			mThreadDadas[uiTPos].bFullOnServer = true;
			SendToClient(mThreadDadas[uiTPos].oConnectionData, PROTO_S_BUFFER_FULL);
			mThreadDadas[uiTPos].uiCurrentFreePocket = S_MAX_STORED_POCKETS - 1;
		}
		pthread_mutex_unlock(&ptConnMutex);
		mThreadDadas[uiTPos].oConnectionData.iStatus =  // Принимаем пакет в текущую позицию.
				recv(mThreadDadas[uiTPos].oConnectionData.iSocket,
					 mThreadDadas[uiTPos].m_chData,
				sizeof(mThreadDadas[uiTPos].m_chData), 0);
		pthread_mutex_lock(&ptConnMutex);
		if (bExitSignal == true) // Если по выходу из приёмки обнаружен общий сигнал на выход...
		{
			LOG_P(LOG_CAT_I, "Exiting reading from ID: " << uiTPos);
			goto ecd;
		}
		if (mThreadDadas[uiTPos].oConnectionData.iStatus <= 0) // Если статус приёмки - отказ (вместо принятых байт)...
		{
			LOG_P(LOG_CAT_I, "Reading socket has been stopped for ID: " << uiTPos);
			goto ecd;
		}
		p_CurrentData = &mThreadDadas[uiTPos].mReceivedPockets[mThreadDadas[uiTPos].uiCurrentFreePocket];
		oParsingResult = p_ProtoParser->ParsePocket(
					mThreadDadas[uiTPos].m_chData,
					mThreadDadas[uiTPos].oConnectionData.iStatus,
					p_CurrentData->oProtocolStorage,
					mThreadDadas[uiTPos].bFullOnServer);
		if((mThreadDadas[uiTPos].bFullOnServer == true) && (oParsingResult.bStored == true)) // DEBUG.
		{
			LOG_P(LOG_CAT_W, "Received owerflowed pocket from ID: " << uiTPos);
		}
		switch(oParsingResult.chRes)
		{
			case PROTOPARSER_OK:
			{
				if(oParsingResult.bStored == true)
				{
					LOG_P(LOG_CAT_I, "Received pocket: " <<
						(mThreadDadas[uiTPos].uiCurrentFreePocket + 1) << " of " << S_MAX_STORED_POCKETS <<
						" for ID: " << uiTPos);
					mThreadDadas[uiTPos].mReceivedPockets[mThreadDadas[uiTPos].uiCurrentFreePocket].bFresh = true;
				}
				if(mThreadDadas[uiTPos].bSecured == false)
				{
					if(oParsingResult.chTypeCode == PROTO_C_SEND_PASSW)
					{
						if(!strcmp(p_chPassword, p_CurrentData->oProtocolStorage.p_Password->m_chPassw))
						{
							SendToClient(mThreadDadas[uiTPos].oConnectionData, PROTO_S_PASSW_OK);
							mThreadDadas[uiTPos].bSecured = true;
							LOG_P(LOG_CAT_I, "Connection is secured for ID: " << uiTPos);
						}
						else
						{
							SendToClient(mThreadDadas[uiTPos].oConnectionData, PROTO_S_PASSW_ERR);
							LOG_P(LOG_CAT_W, "Authentification failed for ID: " << uiTPos);
						}
						p_CurrentData->oProtocolStorage.CleanPointers();
						p_CurrentData->bFresh = false;
						mThreadDadas[uiTPos].uiCurrentFreePocket--;
						LOG_P(LOG_CAT_I, "Free pockets: " << S_MAX_STORED_POCKETS -
							  (mThreadDadas[uiTPos].uiCurrentFreePocket + 1)
							  << " of " << S_MAX_STORED_POCKETS << " for ID: " << uiTPos);
						break;
					}
					else
					{
						SendToClient(mThreadDadas[uiTPos].oConnectionData, PROTO_S_UNSECURED);
						LOG_P(LOG_CAT_W, "Client is not autherised, ID: " << uiTPos);
						break;
					}
				}
				else
				{
					// Блок взаимодействия.
					switch(oParsingResult.chTypeCode)
					{
						case PROTO_C_BUFFER_FULL:
						{
							LOG_P(LOG_CAT_W, "Buffer is full on ID: " << uiTPos);
							mThreadDadas[uiTPos].bFullOnClient = true;
							goto gI;
						}
						case PROTO_A_BUFFER_READY:
						{
							LOG_P(LOG_CAT_I, "Buffer is ready on ID: " << uiTPos);
							mThreadDadas[uiTPos].bFullOnClient = false;
							goto gI;
						}
						case PROTO_C_REQUEST_LEAVING:
						{
							LOG_P(LOG_CAT_I, "ID: " << uiTPos << " request leaving.");
							// МЕСТО ДЛЯ ВСТАВКИ КОДА ОБРАБОТКИ ВЫХОДА КЛИЕНТОВ.
							//
							SendToClient(mThreadDadas[uiTPos].oConnectionData, PROTO_S_ACCEPT_LEAVING);
							LOG_P(LOG_CAT_I, "ID: " << uiTPos << " leaving accepted.");
							MSleep(WAITING_FOR_CLIENT_DSC);
							bLocalExitSignal = true;
							goto ecd;
						}
					}
					// Блок объектов.
					if(oParsingResult.bStored == true)
					{
						if(pf_CBClientDataArrived != 0) pf_CBClientDataArrived(uiTPos);
					}
				}
gI:				break;
			}
			case PROTOPARSER_OUT_OF_RANGE:
			{
				SendToClient(mThreadDadas[uiTPos].oConnectionData, PROTO_S_OUT_OF_RANGE);
				LOG_P(LOG_CAT_E, (char*)POCKET_OUT_OF_RANGE << " from ID: " <<
					  uiTPos << " - " << oParsingResult.iDataLength);
				break;
			}
			case PROTOPARSER_UNKNOWN_COMMAND:
			{
				SendToClient(mThreadDadas[uiTPos].oConnectionData, PROTO_S_UNKNOWN_COMMAND);
				LOG_P(LOG_CAT_W, (char*)UNKNOWN_COMMAND << " from ID: " << uiTPos);
				break;
			}
		}
		if((mThreadDadas[uiTPos].bFullOnServer == false) &&
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
		LOG_P(LOG_CAT_W, "Closed by client absence: " << m_chNameBuffer << " ID: " << uiTPos);
	}
	else
	{
		LOG_P(LOG_CAT_I, "Closed ordinary: " << m_chNameBuffer << " ID: " << uiTPos);
	}
enc:
#ifndef WIN32
	LOG_P(LOG_CAT_I, "Exiting thread: " << mThreadDadas[uiTPos].p_Thread);
#else
	LOG_P(LOG_CAT_I, "Exiting thread: " << mThreadDadas[uiTPos].p_Thread.p);
#endif
	if(mThreadDadas[uiTPos].bInUse == true)
		if(pf_CBClientStateChanged != 0) pf_CBClientStateChanged(uiTPos, false); // Вызов кэлбэка смены статуса.
	CleanThrDadaPos(uiTPos);
	if(iSelectedConnection == (int)uiTPos) iSelectedConnection = CONNECTION_SEL_ERROR;
	if(bKillListenerAccept) bListenerAlive = false;
	pthread_mutex_unlock(&ptConnMutex);
	RETURN_THREAD;
}

// Серверный поток.
void* Server::ServerThread(void *p_vPlug)
{
	p_vPlug = p_vPlug;
	tinyxml2::XMLDocument xmlDocSConf;
	list <XMLNode*> o_lNet;
	XMLError eResult;
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
			SendToClient(mThreadDadas[uiCurrPos].oConnectionData, PROTO_S_SHUTDOWN_INFO);
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
