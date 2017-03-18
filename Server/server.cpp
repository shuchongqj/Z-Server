//== ВКЛЮЧЕНИЯ.
#include <Server/server.h>

//== МАКРОСЫ.
#define LOG_NAME				"Z-Server"
#define TryMutexInit			int iLocked
#define TryMutexLock			iLocked = pthread_mutex_trylock(&ptConnMutex)
#define TryMutexUnlock			if(iLocked == 0) pthread_mutex_unlock(&ptConnMutex)

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL_INIT_INCLASS(Server)
LOGDECL_INIT_PTHRD_INCLASS_EXT_ADD(Server)
bool Server::bServerAlive = false;
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
CBClientRequestArrived Server::pf_CBClientRequestArrived = 0;
CBClientDataArrived Server::pf_CBClientDataArrived = 0;
CBClientStatusChanged Server::pf_CBClientStatusChanged = 0;
pthread_t Server::p_ThreadOverrunned;
vector<Server::IPBanUnit>* Server::p_vec_IPBansInt = 0;

//== ФУНКЦИИ КЛАССОВ.
//== Класс сервера.
// Конструктор.
Server::Server(const char* cp_chSettingsPathIn, pthread_mutex_t ptLogMutex, vector<IPBanUnit>* p_vec_IPBans)
{
	LOG_CTRL_BIND_EXT_MUTEX(ptLogMutex);
	LOG_CTRL_INIT;
	p_chSettingsPath = (char*)cp_chSettingsPathIn;
	p_vec_IPBansInt = p_vec_IPBans;
}

// Деструктор.
Server::~Server()
{
	LOG_CLOSE;
}

// Запрос запуска клиента.
void Server::Start()
{
	if(!bServerAlive)
	{
		LOG_P_2(LOG_CAT_I, "Starting server thread.");
		pthread_create(&ServerThr, NULL, ServerThread, NULL);
		LOG_RETVAL = RETVAL_OK;
	}
	else
	{
		LOG_P_0(LOG_CAT_E, "Starting server failed - already started.");
		LOG_RETVAL = RETVAL_ERR;
	}
}

// Запрос остановки клиента.
void Server::Stop()
{
	if(bServerAlive)
	{
		bExitSignal = true;
		LOG_RETVAL = RETVAL_OK;
	}
	else
	{
		LOG_P_0(LOG_CAT_E, "Stopping server failed - is off.");
		LOG_RETVAL = RETVAL_ERR;
	}
}

// Запрос статуса лога.
int Server::GetLogStatus()
{
	return LOG_RETVAL;
}

// Запрос готовности.
bool Server::CheckReady()
{
	return bServerAlive;
}

// Функция отправки пакета по соединению немедленно.
bool Server::SendToConnectionImmediately(NetHub& a_NetHub, NetHub::ConnectionData &a_ConnectionData, char chCommand,
						  bool bFullFlag, char *p_chBuffer, int iLength, bool bResetPointer)
{
	if(bFullFlag == false)
	{
		if(a_NetHub.AddPocketToBuffer(chCommand, p_chBuffer, iLength) == false)
		{
			LOG_P_0(LOG_CAT_E, "Pockets buffer is full.");
			return false;
		}
		if(a_NetHub.SendToAddress(a_ConnectionData, bResetPointer) == false)
		{
			LOG_P_0(LOG_CAT_E, "Socket error on sending data.");
			return false;
		}
	}
	else
	{
		LOG_P_0(LOG_CAT_E, "Client buffer is full.");
		return false;
	}
	return true;
}

// Функция отправки буфера по соединению.
bool Server::SendBufferToConnection(NetHub& a_NetHub, NetHub::ConnectionData &a_ConnectionData, bool bFullFlag, bool bResetPointer)
{
	if(bFullFlag == false)
	{
		if(a_NetHub.SendToAddress(a_ConnectionData, bResetPointer) == false)
		{
			LOG_P_0(LOG_CAT_E, "Socket error on sending data.");
			return false;
		}
	}
	else
	{
		LOG_P_0(LOG_CAT_E, "Client buffer is full.");
		return false;
	}
	return true;
}

// Отправка пакета клиенту на текущее выбранное соединение немедленно.
bool Server::SendToClientImmediately(NetHub& a_NetHub, char chCommand, char* p_chBuffer, int iLength, bool bResetPointer)
{
	bool bRes = false;
	TryMutexInit;
	//
	TryMutexLock;
	if(iSelectedConnection == CONNECTION_SEL_ERROR) goto gUE;
	if((mThreadDadas[iSelectedConnection].bFullOnClient == false) & (mThreadDadas[iSelectedConnection].bSecured == true))
	{
		if(SendToConnectionImmediately(a_NetHub, mThreadDadas[iSelectedConnection].oConnectionData,
					  chCommand, false, p_chBuffer, iLength, bResetPointer) == true)
			bRes = true;
	}
gUE:TryMutexUnlock;
	if(bRes == false)
	{
		if(iSelectedConnection == CONNECTION_SEL_ERROR)
		{
			LOG_P_0(LOG_CAT_E, "Sending failed - wrong connection.");
		}
		else
		{
			LOG_P_0(LOG_CAT_E, "Sending failed for: " << iSelectedConnection);
		}
	}
	return bRes;
}

// Отправка буфера клиенту на текущее выбранное соединение.
bool Server::SendBufferToClient(NetHub& a_NetHub, bool bResetPointer)
{
	bool bRes = false;
	TryMutexInit;
	//
	TryMutexLock;
	if(iSelectedConnection == CONNECTION_SEL_ERROR) goto gUE;
	if((mThreadDadas[iSelectedConnection].bFullOnClient == false) & (mThreadDadas[iSelectedConnection].bSecured == true))
	{
		if(SendBufferToConnection(a_NetHub, mThreadDadas[iSelectedConnection].oConnectionData, false, bResetPointer) == true)
			bRes = true;
	}
gUE:TryMutexUnlock;
	if(bRes == false)
	{
		if(iSelectedConnection == CONNECTION_SEL_ERROR)
		{
			LOG_P_0(LOG_CAT_E, "Sending failed - wrong connection.");
		}
		else
		{
			LOG_P_0(LOG_CAT_E, "Sending failed for: " << iSelectedConnection);
		}
	}
	return bRes;
}

// Уст. ук. кэлбэка изменения статуса подключения клиента. !Операции с буф. пакетов в кэлбэке делать ТОЛЬКО СРАЗУ (конкурентные потоки)!
void Server::SetClientRequestArrivedCB(CBClientRequestArrived pf_CBClientRequestArrivedIn)
{
	TryMutexInit;
	//
	TryMutexLock;
	pf_CBClientRequestArrived = pf_CBClientRequestArrivedIn;
	TryMutexUnlock;
}

// Уст.ук. кэлбэка обработки принятых пакетов от клиентов. !Операции с буфф. пакетов в кэлбэке делать ТОЛЬКО СРАЗУ (конкурентные потоки)!
void Server::SetClientDataArrivedCB(CBClientDataArrived pf_CBClientDataArrivedIn)
{
	TryMutexInit;
	//
	TryMutexLock;
	pf_CBClientDataArrived = pf_CBClientDataArrivedIn;
	TryMutexUnlock;
}

// Уст. ук. кэлбэка отслеживания статута клиентов. !Операции с буф. пакетов в кэлбэке делать ТОЛЬКО СРАЗУ (конкурентные потоки)!
void Server::SetClientStatusChangedCB(CBClientStatusChanged pf_CBClientStatusChangedIn)
{
	TryMutexInit;
	//
	TryMutexLock;
	pf_CBClientStatusChanged = pf_CBClientStatusChangedIn;
	TryMutexUnlock;
}

// Установка текущего индекса осоединения для исходящих.
bool Server::SetCurrentConnection(unsigned int uiIndex)
{
	bool bRes = false;
	TryMutexInit;
	//
	if(uiIndex > (MAX_CONN - 1))
	{
		iSelectedConnection = CONNECTION_SEL_ERROR;
		LOG_P_0(LOG_CAT_E, "Index is out of range.");
		return false;
	}
	TryMutexLock;
	if(mThreadDadas[uiIndex].bInUse == true)
	{
		iSelectedConnection = uiIndex;
		LOG_P_2(LOG_CAT_I, "Selected ID: " << uiIndex);
		bRes = true;
	}
	else
	{
		iSelectedConnection = CONNECTION_SEL_ERROR;
		LOG_P_0(LOG_CAT_E, "Selected ID is unused: " << uiIndex);
	}
	TryMutexUnlock;
	return bRes;
}

// Удаление крайнего элемента из массива принятых пакетов.
int Server::ReleaseCurrentData(NetHub& a_NetHub)
{
	int iRes = BUFFER_IS_EMPTY;
	TryMutexInit;
	//
	TryMutexLock;
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
				SendToConnectionImmediately(a_NetHub, mThreadDadas[iSelectedConnection].oConnectionData, PROTO_A_BUFFER_READY);
			}
			if(mThreadDadas[iSelectedConnection].
			   mReceivedPockets[mThreadDadas[iSelectedConnection].uiCurrentFreePocket].bFresh == true)
			{
				mThreadDadas[iSelectedConnection].
						mReceivedPockets[mThreadDadas[iSelectedConnection].uiCurrentFreePocket].bFresh = false;
				mThreadDadas[iSelectedConnection].
						mReceivedPockets[mThreadDadas[iSelectedConnection].uiCurrentFreePocket].
						oProtocolStorage.CleanPointers();
				iRes = RETVAL_OK;
				LOG_P_2(LOG_CAT_I, "Position has been released.");
			}
		}
	}
	else
	{
		iRes = CONNECTION_SEL_ERROR;
		LOG_P_0(LOG_CAT_E, "Wrong connection number (release).");
	}
	TryMutexUnlock;
	if(iRes == BUFFER_IS_EMPTY)
	{
		LOG_P_0(LOG_CAT_E, "Buffer is empty.");
	}
	return iRes;
}

// Доступ к крайнему элементу из массива принятых пакетов от текущего клиента.
int Server::AccessCurrentData(void** pp_vDataBuffer)
{
	int iRes = DATA_ACCESS_ERROR;
	unsigned int uiPos;
	TryMutexInit;
	//
	TryMutexLock;
	if(iSelectedConnection != CONNECTION_SEL_ERROR)
	{
		uiPos = mThreadDadas[iSelectedConnection].uiCurrentFreePocket;
		if(mThreadDadas[iSelectedConnection].uiCurrentFreePocket > 0)
		{
			if(mThreadDadas[iSelectedConnection].bFullOnServer == false) uiPos--;
			if(mThreadDadas[iSelectedConnection].mReceivedPockets[uiPos].bFresh == true)
			{
				*pp_vDataBuffer = mThreadDadas[iSelectedConnection].mReceivedPockets[uiPos].oProtocolStorage.GetPointer();
				iRes = mThreadDadas[iSelectedConnection].mReceivedPockets[uiPos].oProtocolStorage.chTypeCode;
			}
		}
	}
	else
	{
		iRes = CONNECTION_SEL_ERROR;
		LOG_P_0(LOG_CAT_E, "Wrong connection number (access).");
	}
	TryMutexUnlock;
	if(iRes == DATA_ACCESS_ERROR)
	{
		LOG_P_0(LOG_CAT_E, "Data access error.");
	}
	return iRes;
}

// Принудительное отключение клиента.
void Server::KickClient(NetHub& a_NetHub, unsigned int uiIndex)
{
	TryMutexInit;
	//
	TryMutexLock;
	mThreadDadas[uiIndex].bKick = true;
	if(SendToConnectionImmediately(a_NetHub,
								   mThreadDadas[uiIndex].oConnectionData, PROTO_S_KICK, mThreadDadas[uiIndex].bFullOnClient))
	{
		TryMutexUnlock;
		MSleep(WAITING_FOR_CLIENT_DSC);
		TryMutexLock;
	}
#ifndef WIN32
	shutdown(mThreadDadas[uiIndex].oConnectionData.iSocket, SHUT_RDWR);
	close(mThreadDadas[uiIndex].oConnectionData.iSocket);
#else
	closesocket(mThreadDadas[uiIndex].oConnectionData.iSocket);
#endif
	TryMutexUnlock;
}

// Очистка позиции данных потока.
void Server::CleanThrDadaPos(unsigned int uiPos)
{
	for(unsigned int uiC = 0; uiC < S_MAX_STORED_POCKETS; uiC++)
	{
		mThreadDadas[uiPos].mReceivedPockets[uiC].oProtocolStorage.CleanPointers();
	}
	mThreadDadas[uiPos].bFullOnClient = false;
	mThreadDadas[uiPos].bFullOnServer = false;
	mThreadDadas[uiPos].bSecured = false;
	mThreadDadas[uiPos].uiCurrentFreePocket = 0;
	memset(&mThreadDadas[uiPos].mReceivedPockets, 0, sizeof(NetHub::ReceivedData));
	memset(&mThreadDadas[uiPos].m_chData, 0, sizeof(mThreadDadas[uiPos].m_chData));
}

// Поиск свободной позиции данных потока.
int Server::FindFreeThrDadaPos()
{
	unsigned int uiPos = 0;
	TryMutexInit;
	//
	TryMutexLock;
	for(; uiPos != MAX_CONN; uiPos++)
	{
		if(mThreadDadas[uiPos].bInUse == false) // Если не стоит флаг занятости - годен.
		{
			TryMutexUnlock;
			return uiPos;
		}
	}
	TryMutexUnlock;
	return CONNECTION_SEL_ERROR;
}

// Получение копии структуры описания соединения по индексу.
NetHub::ConnectionData Server::GetConnectionData(unsigned int uiIndex)
{
	NetHub::ConnectionData oConnectionDataRes;
	TryMutexInit;
	//
	TryMutexLock;
	if((uiIndex < MAX_CONN) & (mThreadDadas[uiIndex].bInUse == true))
	{
		oConnectionDataRes = mThreadDadas[uiIndex].oConnectionData;
		TryMutexUnlock;
		return oConnectionDataRes;
	}
	TryMutexUnlock;
	oConnectionDataRes.iStatus = CONNECTION_SEL_ERROR;
	return oConnectionDataRes;
}

// Заполнение структуры описания соединения.
void Server::FillConnectionData(int iSocket, NetHub::ConnectionData& a_ConnectionData)
{
	TryMutexInit;
	//
	a_ConnectionData.ai_addrlen = sizeof(sockaddr);
	TryMutexLock;
	a_ConnectionData.iSocket = iSocket;
	a_ConnectionData.iStatus = 0;
#ifndef WIN32
	getpeername(a_ConnectionData.iSocket, &a_ConnectionData.ai_addr,
				&a_ConnectionData.ai_addrlen);
#else
	getpeername(a_ConnectionData.iSocket, &a_ConnectionData.ai_addr,
				(int*)&a_ConnectionData.ai_addrlen);
#endif
	TryMutexUnlock;
}

// Заполнение буферов имён IP и порта.
void Server::FillIPAndPortNames(NetHub::ConnectionData& a_ConnectionData, char* p_chIP, char* p_chPort)
{
	TryMutexInit;
	//
	TryMutexLock;
#ifndef WIN32
	getnameinfo(&a_ConnectionData.ai_addr,
				a_ConnectionData.ai_addrlen,
				p_chIP, INET6_ADDRSTRLEN, p_chPort, PORTSTRLEN, NI_NUMERICHOST);
#else
	getnameinfo(&a_ConnectionData.ai_addr,
				(socklen_t)a_ConnectionData.ai_addrlen,
				p_chIP, INET6_ADDRSTRLEN, p_chPort, PORTSTRLEN, NI_NUMERICHOST);
#endif
	TryMutexUnlock;
}

// Поток соединения.
void* Server::ConversationThread(void* p_vNum)
{
	int iTPos;
	bool bKillListenerAccept;
	ProtoParser* p_ProtoParser;
	ProtoParser::ParseResult oParsingResult;
	char m_chIPNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortNameBuffer[PORTSTRLEN];
	bool bLocalExitSignal;
	NetHub::ReceivedData* p_CurrentData;
	int iTempListener;
	int iTempTPos;
	NetHub::ConnectionData oConnectionDataInt;
	char* p_chData;
	int iLength;
	NetHub* p_LocalNH;
	//
	p_LocalNH = new NetHub();
	bKillListenerAccept = false;
	bServerAlive = true;
	bLocalExitSignal = false;
	iTempTPos = CONNECTION_SEL_ERROR;
	iTPos = *((int*)p_vNum); // Получили номер в массиве.
gBA:if(iTPos != CONNECTION_SEL_ERROR)
	{
		mThreadDadas[iTPos].bKick = false;
		mThreadDadas[iTPos].p_Thread = pthread_self(); // Задали ссылку на текущий поток.
#ifndef WIN32
		LOG_P_2(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread);
#else
		LOG_P_2(LOG_CAT_I, "Waiting connection on thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	}
	else
	{
#ifndef WIN32
		LOG_P_1(LOG_CAT_W, "Waiting connection on reserved thread: " << pthread_self());
#else
        LOG_P_1(LOG_CAT_W, "Waiting connection on reserved thread: " << pthread_self().p);
#endif
gAG:	iTempListener = (int)accept(iListener, NULL, NULL); // Ждём перегруженных входящих.
		FillConnectionData(iTempListener, oConnectionDataInt);
		FillIPAndPortNames(oConnectionDataInt, m_chIPNameBuffer, m_chPortNameBuffer);
		iTempTPos = FindFreeThrDadaPos();
		if(iTempTPos == CONNECTION_SEL_ERROR)
		{
#ifndef WIN32
			shutdown(iTempListener, SHUT_RDWR);
			close(iTempListener);
#else
			closesocket(iTempListener);
#endif
			LOG_P_2(LOG_CAT_W, "Connection rejected for: " << m_chIPNameBuffer);
			if(bExitSignal) goto gOE;
			goto gAG;
		}
		iTPos = iTempTPos;
		mThreadDadas[iTPos].oConnectionData.iSocket = iTempListener;
	}
	if(iTempTPos == CONNECTION_SEL_ERROR) // Если пришло мимо перегруженного ожидания...
		mThreadDadas[iTPos].oConnectionData.iSocket = (int)accept(iListener, NULL, NULL); // Ждём входящих.
	if((mThreadDadas[iTPos].oConnectionData.iSocket < 0)) // При ошибке после выхода из ожидания входящих...
	{
		if(!bExitSignal) // Если не было сигнала на выход от основного потока...
		{
			pthread_mutex_lock(&ptConnMutex);
			LOG_P_0(LOG_CAT_E, "'accept': " << gai_strerror(mThreadDadas[iTPos].oConnectionData.iSocket)); // Про ошибку.
			goto enc;
		}
		else
		{
gOE:		pthread_mutex_lock(&ptConnMutex);
			LOG_P_1(LOG_CAT_I, "Accepting connections terminated."); // Норм. сообщение.
			bKillListenerAccept = true; // Заказываем подтверждение закрытия приёмника.
			goto enc;
		}
	}
	FillConnectionData(mThreadDadas[iTPos].oConnectionData.iSocket, mThreadDadas[iTPos].oConnectionData);
	FillIPAndPortNames(mThreadDadas[iTPos].oConnectionData, m_chIPNameBuffer, m_chPortNameBuffer);
	if(p_vec_IPBansInt != 0) // Обработка банов.
	{
		for(unsigned int uiN = 0; uiN < (*p_vec_IPBansInt).size(); uiN++)
		{
			if(!strcmp((*p_vec_IPBansInt).at(uiN).m_chIP, m_chIPNameBuffer))
			{
				LOG_P_0(LOG_CAT_W, "Connection rejected due ban for: " << m_chIPNameBuffer);
				SendToConnectionImmediately(*p_LocalNH, mThreadDadas[iTPos].oConnectionData, PROTO_S_BAN);
				MSleep(WAITING_FOR_CLIENT_DSC);
#ifndef WIN32
				shutdown(mThreadDadas[iTPos].oConnectionData.iSocket, SHUT_RDWR);
				close(mThreadDadas[iTPos].oConnectionData.iSocket);
#else
				closesocket(mThreadDadas[iTPos].oConnectionData.iSocket);
#endif
				goto gBA;
			}
		}
	}
	LOG_P_1(LOG_CAT_I, "New connection accepted.");
	mThreadDadas[iTPos].bInUse = true; // Флаг занятости структуры.
	LOG_P_1(LOG_CAT_I, "Connected with: " << m_chIPNameBuffer << ":" << m_chPortNameBuffer << " ID: " << iTPos);
	if(mThreadDadas[iTPos].oConnectionData.iStatus == -1) // Если не вышло отправить...
	{
		pthread_mutex_lock(&ptConnMutex);
		LOG_P_0(LOG_CAT_E, "'send': " << gai_strerror(mThreadDadas[iTPos].oConnectionData.iStatus));
		goto ec;
	}
	//
	bRequestNewConn = true; // Соединение готово - установка флага для главного потока на запрос нового.
	if(pf_CBClientStatusChanged != 0)
	{
		// Вызов кэлбэка смены статуса.
		pf_CBClientStatusChanged(*p_LocalNH, true, iTPos);
	}
	p_ProtoParser = new ProtoParser;
	while(bExitSignal == false) // Пока не пришёл флаг общего завершения...
	{
		mThreadDadas[iTPos].oConnectionData.iStatus =  // Принимаем пакет в текущую позицию.
				recv(mThreadDadas[iTPos].oConnectionData.iSocket,
					 mThreadDadas[iTPos].m_chData,
				sizeof(mThreadDadas[iTPos].m_chData), 0);
		pthread_mutex_lock(&ptConnMutex);
		if (bExitSignal == true) // Если по выходу из приёмки обнаружен общий сигнал на выход...
		{
			LOG_P_2(LOG_CAT_I, "Exiting reading from ID: " << iTPos);
			goto ecd;
		}
		if (mThreadDadas[iTPos].oConnectionData.iStatus <= 0) // Если статус приёмки - отказ (вместо принятых байт)...
		{
			LOG_P_2(LOG_CAT_I, "Reading socket has been stopped for ID: " << iTPos);
			goto ecd;
		}
		p_chData = mThreadDadas[iTPos].m_chData;
		iLength = mThreadDadas[iTPos].oConnectionData.iStatus;
		//
gDp:	p_CurrentData = &mThreadDadas[iTPos].mReceivedPockets[mThreadDadas[iTPos].uiCurrentFreePocket];
		oParsingResult = p_ProtoParser->ParsePocket(p_chData, iLength, p_CurrentData->oProtocolStorage, mThreadDadas[iTPos].bFullOnServer);
		switch(oParsingResult.iRes)
		{
			case PROTOPARSER_OK:
			{
				if(oParsingResult.bStored == true)
				{
					LOG_P_2(LOG_CAT_I, "Received pocket: " <<
						(mThreadDadas[iTPos].uiCurrentFreePocket + 1) << " of " << S_MAX_STORED_POCKETS <<
						" for ID: " << iTPos);
					mThreadDadas[iTPos].mReceivedPockets[mThreadDadas[iTPos].uiCurrentFreePocket].bFresh = true;
				}
				if(oParsingResult.bStored == false)
				{
					if(pf_CBClientRequestArrived != 0)
					{
						// Вызов кэлбэка прибытия запроса.
						pf_CBClientRequestArrived(*p_LocalNH, iTPos, oParsingResult.chTypeCode);
					}
				}
				if(mThreadDadas[iTPos].bSecured == false)
				{
					if(oParsingResult.chTypeCode == PROTO_C_SEND_PASSW)
					{
						if(!strcmp(p_chPassword, p_CurrentData->oProtocolStorage.p_Password->m_chPassw))
						{
							SendToConnectionImmediately(*p_LocalNH, mThreadDadas[iTPos].oConnectionData, PROTO_S_PASSW_OK);
							mThreadDadas[iTPos].bSecured = true;
							LOG_P_1(LOG_CAT_I, "Connection is secured for ID: " << iTPos);
							LOG_P_2(LOG_CAT_I, "Free pockets: " << S_MAX_STORED_POCKETS -
								  (mThreadDadas[iTPos].uiCurrentFreePocket)
								  << " of " << S_MAX_STORED_POCKETS << " for ID: " << iTPos);
						}
						else
						{
							SendToConnectionImmediately(*p_LocalNH, mThreadDadas[iTPos].oConnectionData, PROTO_S_PASSW_ERR);
							mThreadDadas[iTPos].bSecured = false;
							LOG_P_0(LOG_CAT_W, "Authentification failed for ID: " << iTPos);
						}
						p_CurrentData->oProtocolStorage.CleanPointers();
						p_CurrentData->bFresh = false;
						mThreadDadas[iTPos].uiCurrentFreePocket--;
						goto gI;
					}
					else
					{
						if(oParsingResult.chTypeCode != PROTO_C_REQUEST_LEAVING)
						{
							SendToConnectionImmediately(*p_LocalNH, mThreadDadas[iTPos].oConnectionData, PROTO_S_UNSECURED);
							LOG_P_0(LOG_CAT_W, "Client is not autherised, ID: " << iTPos);
						}
						goto gI;
					}
				}
				else
				{
					// Блок объектов.
					if(oParsingResult.bStored == true)
					{
						if(pf_CBClientDataArrived != 0)
						{
							mThreadDadas[iTPos].uiCurrentFreePocket++;
							// Вызов кэлбэка прибытия данных.
							pf_CBClientDataArrived(*p_LocalNH, iTPos);
							mThreadDadas[iTPos].uiCurrentFreePocket--;
						}
					}
				}
				// Блок взаимодействия.
gI:				switch(oParsingResult.chTypeCode)
				{
					case PROTO_C_BUFFER_FULL:
					{
						LOG_P_1(LOG_CAT_W, "Buffer is full on ID: " << iTPos);
						mThreadDadas[iTPos].bFullOnClient = true;
						break;
					}
					case PROTO_A_BUFFER_READY:
					{
						LOG_P_1(LOG_CAT_I, "Buffer is ready on ID: " << iTPos);
						mThreadDadas[iTPos].bFullOnClient = false;
						break;
					}
					case PROTO_C_REQUEST_LEAVING:
					{
						LOG_P_1(LOG_CAT_I, "ID: " << iTPos << " request leaving.");
						SendToConnectionImmediately(*p_LocalNH, mThreadDadas[iTPos].oConnectionData, PROTO_S_ACCEPT_LEAVING);
						LOG_P_1(LOG_CAT_I, "ID: " << iTPos << " leaving accepted.");
						bLocalExitSignal = true; // Флаг самостоятельного отключения клиента.
						MSleep(WAITING_FOR_CLIENT_DSC); // Ожидание самостоятельного отключения клиента.
						goto ecd;
					}
				}
				if((mThreadDadas[iTPos].bSecured == false) & (oParsingResult.bStored == true) &
				   (oParsingResult.chTypeCode != PROTO_C_SEND_PASSW))
				{
					LOG_P_2(LOG_CAT_W, "Position has been cleared.");
					p_CurrentData->oProtocolStorage.CleanPointers();
					p_CurrentData->bFresh = false;
					mThreadDadas[iTPos].uiCurrentFreePocket--;
				}
				break;
			}
			case PROTOPARSER_UNKNOWN_COMMAND:
			{
				SendToConnectionImmediately(*p_LocalNH, mThreadDadas[iTPos].oConnectionData, PROTO_S_UNKNOWN_COMMAND);
				LOG_P_0(LOG_CAT_W, (char*)MSG_UNKNOWN_COMMAND  << ": '" << oParsingResult.chTypeCode << "'"
					  << " from ID: " << iTPos);
				break;
			}
			case PROTOPARSER_WRONG_FORMAT:
			{
				LOG_P_0(LOG_CAT_W, (char*)MSG_WRONG_FORMAT  << " from ID: " << iTPos);
				break;
			}
		}
		if(oParsingResult.bStored)
		{
			mThreadDadas[iTPos].uiCurrentFreePocket++;
		}
		if(mThreadDadas[iTPos].uiCurrentFreePocket >= S_MAX_STORED_POCKETS)
		{
			LOG_P_1(LOG_CAT_W, "Buffer is full for ID: " << iTPos);
			mThreadDadas[iTPos].bFullOnServer = true;
			SendToConnectionImmediately(*p_LocalNH, mThreadDadas[iTPos].oConnectionData, PROTO_S_BUFFER_FULL);
			mThreadDadas[iTPos].uiCurrentFreePocket = S_MAX_STORED_POCKETS - 1;
		}
		if(oParsingResult.p_chExtraData != 0)
		{
			LOG_P_2(LOG_CAT_I, MSG_GOT_MERGED);
			p_chData = oParsingResult.p_chExtraData;
			iLength = oParsingResult.iExtraDataLength;
			goto gDp;
		}
		pthread_mutex_unlock(&ptConnMutex);
	}
	pthread_mutex_lock(&ptConnMutex);
ecd:delete p_ProtoParser;
	//
ec: if(bLocalExitSignal == false) // Если не было локального сигнала - будут закрывать снаружи потоков.
	{
		if(bExitSignal == false)
		{
			if(mThreadDadas[iTPos].bKick)
			{
				LOG_P_1(LOG_CAT_W, "Closed due kicking out: " << m_chIPNameBuffer << " ID: " << iTPos);
			}
			else
			{
				LOG_P_0(LOG_CAT_W, "Closed by client absence: " << m_chIPNameBuffer << " ID: " << iTPos);
			}
		}
	}
	else // При локальном - закрываем здесь.
	{
#ifndef WIN32
		shutdown(mThreadDadas[iTPos].oConnectionData.iSocket, SHUT_RDWR);
		close(mThreadDadas[iTPos].oConnectionData.iSocket);
#else
		closesocket(mThreadDadas[iTPos].oConnectionData.iSocket);
#endif
		LOG_P_2(LOG_CAT_I, "Closed ordinary: " << m_chIPNameBuffer << ":" << m_chPortNameBuffer << " ID: " << iTPos);
	}
enc:if(iTPos != CONNECTION_SEL_ERROR)
	{
#ifndef WIN32
	LOG_P_2(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread);
#else
	LOG_P_2(LOG_CAT_I, "Exiting thread: " << mThreadDadas[iTPos].p_Thread.p);
#endif
	}
	else
    {
#ifndef WIN32
        LOG_P_1(LOG_CAT_W, "Exiting reserved thread: " << pthread_self());
#else
        LOG_P_1(LOG_CAT_W, "Exiting reserved thread: " << pthread_self().p);
#endif
	}
	if(!bKillListenerAccept)
	{
		if(pf_CBClientStatusChanged != 0)
		{
			// Вызов кэлбэка смены статуса.
			pf_CBClientStatusChanged(*p_LocalNH, false, iTPos);
		}
	}
	if(iTPos != CONNECTION_SEL_ERROR)
	{
		CleanThrDadaPos(iTPos);
		if(!bExitSignal)
		{
			mThreadDadas[iTPos].bInUse = false;
		}
	}
	if(iSelectedConnection == (int)iTPos) iSelectedConnection = CONNECTION_SEL_ERROR;
	if(bKillListenerAccept) bListenerAlive = false;
	pthread_mutex_unlock(&ptConnMutex);
	delete p_LocalNH;
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
	int iCurrPos = 0;
	char m_chIPNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortNameBuffer[PORTSTRLEN];
	NetHub oMainNH;
#ifdef WIN32
	WSADATA wsadata = WSADATA();
#endif
	//
	LOG_P_1(LOG_CAT_I, "Server thread has been started.");
	//
	memset(&mThreadDadas, 0, sizeof mThreadDadas);
	// Работа с файлом конфигурации.
	eResult = xmlDocSConf.LoadFile(p_chSettingsPath);
	if (eResult != XML_SUCCESS)
	{
		LOG_P_0(LOG_CAT_E, "Can`t open configuration file:" << p_chSettingsPath);
		RETVAL_SET(RETVAL_ERR);
		RETURN_THREAD;
	}
	else
	{
		LOG_P_1(LOG_CAT_I, "Configuration loaded.");
	}
	if(!FindChildNodes(xmlDocSConf.LastChild(), o_lNet,
					   "Net", FCN_ONE_LEVEL, FCN_FIRST_ONLY))
	{
		LOG_P_0(LOG_CAT_E, "Configuration file is corrupt! No 'Net' node.");
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
		LOG_P_1(LOG_CAT_I, "Server IP: " << p_chServerIP);
	}
	else
	{
		LOG_P_0(LOG_CAT_E, "Configuration file is corrupt! No '(Net)IP' node.");
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
		LOG_P_1(LOG_CAT_I, "Port: " << p_chPort);
	}
	else
	{
		LOG_P_0(LOG_CAT_E, "Configuration file is corrupt! No '(Net)Port' node.");
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
		LOG_P_0(LOG_CAT_E, "Configuration file is corrupt! No 'Password' node.");
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
		LOG_P_0(LOG_CAT_E, "'getaddrinfo': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iListener = (int)socket(p_Res->ai_family, p_Res->ai_socktype, p_Res->ai_protocol); // Получение сокета для приёмника.
	if(iListener < 0 )
	{
		LOG_P_0(LOG_CAT_E, "'socket': "  << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = bind(iListener, p_Res->ai_addr, (int)p_Res->ai_addrlen); // Привязка к указанному IP.
	if(iServerStatus < 0)
	{
		LOG_P_0(LOG_CAT_E, "'bind': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	iServerStatus = listen(iListener, 10); // Запуск приёмника.
	if(iServerStatus < 0)
	{
		LOG_P_0(LOG_CAT_E, "'listen': " << gai_strerror(iServerStatus));
		RETVAL_SET(RETVAL_ERR);
		goto ex;
	}
	freeaddrinfo(p_Res);
	//
	LOG_P_1(LOG_CAT_I, "Accepting connections...");
	bListenerAlive = true;
	// Цикл ожидания входа клиентов.
nc:	bRequestNewConn = false; // Вход в звено цикла ожидания клиентов - сброс флага запроса.
	iCurrPos = FindFreeThrDadaPos();
	if(iCurrPos == CONNECTION_SEL_ERROR)
	{
		LOG_P_0(LOG_CAT_W, "Server is full.");
		pthread_create(&p_ThreadOverrunned, NULL,
					   ConversationThread, &iCurrPos);
	}
	else
	{
		LOG_P_1(LOG_CAT_I, "Free ID slot: " << iCurrPos);
		pthread_create(&mThreadDadas[iCurrPos].p_Thread, NULL,
					   ConversationThread, &iCurrPos); // Создание нового потока приёмки.
	}
	while(!bExitSignal)
	{
		if(bRequestNewConn == true)
			goto nc;
		MSleep(USER_RESPONSE_MS);
	}
	printf("\b\b");
	LOG_P_2(LOG_CAT_I, "Terminated by admin.");
	// Закрытие приёмника.
	LOG_P_2(LOG_CAT_I, "Closing listener...");
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
	LOG_P_2(LOG_CAT_I, "Listener has been closed.");
	// Закрытие сокетов клиентов.
	LOG_P_2(LOG_CAT_I, "Disconnecting clients...");
	for(iCurrPos = 0; iCurrPos != MAX_CONN; iCurrPos++) // Закрываем все клиентские сокеты.
	{
		if(mThreadDadas[iCurrPos].bInUse == true)
		{
			SendToConnectionImmediately(oMainNH, mThreadDadas[iCurrPos].oConnectionData, PROTO_S_SHUTDOWN_INFO);
		}
	}
	MSleep(WAITING_FOR_CLIENT_DSC * 2);
	pthread_mutex_lock(&ptConnMutex);
	for(iCurrPos = 0; iCurrPos != MAX_CONN; iCurrPos++) // Закрываем все клиентские сокеты.
	{
		if(mThreadDadas[iCurrPos].bInUse == true)
		{
			pthread_mutex_unlock(&ptConnMutex);
			FillIPAndPortNames(mThreadDadas[iCurrPos].oConnectionData, m_chIPNameBuffer, m_chPortNameBuffer);
			pthread_mutex_lock(&ptConnMutex);
#ifndef WIN32
			shutdown(mThreadDadas[iCurrPos].oConnectionData.iSocket, SHUT_RDWR);
			close(mThreadDadas[iCurrPos].oConnectionData.iSocket);
#else
			closesocket(mThreadDadas[iCurrPos].oConnectionData.iSocket);
#endif
			LOG_P_1(LOG_CAT_I, "Socket closed internally: " << m_chIPNameBuffer << m_chPortNameBuffer);
		}
	}
	pthread_mutex_unlock(&ptConnMutex);
	LOG_P_2(LOG_CAT_I, "Clients has been disconnected.");
ex:
#ifdef WIN32
	WSACleanup();
#endif
	LOG_P_1(LOG_CAT_I, "Exiting server thread.");
	bExitSignal = false;
	bServerAlive = false;
	RETURN_THREAD;
}
