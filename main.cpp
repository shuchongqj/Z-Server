//== ВКЛЮЧЕНИЯ.
#include "Server/server.h"

//== МАКРОСЫ.
#define LOG_NAME				"Z-Admin"
#define S_CONF_PATH				"./settings/server.ini"
#ifdef WIN32
#include <string>
#endif

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL
LOGDECL_INIT_PTHRD_ADD

//== ФУНКЦИИ.
/// Кэлбэк обработки статутов подключений клиентов.
void ClientRequestArrivedCallback(unsigned int uiClientIndex, char chRequest)
{
	LOG_P(LOG_CAT_I, "Client ID: " << uiClientIndex << " Request: " << chRequest);
}

/// Кэлбэк обработки прихода пакетов от клиентов.
void ClientDataArrivedCallback(unsigned int uiClientIndex)
{
	LOG_P(LOG_CAT_I, "Data arrived from ID: " << uiClientIndex);
}

/// Кэлбэк обработки отслеживания статута клиентов.
void ClientStatusChangedCallback(bool bConnected, unsigned int uiClientIndex, sockaddr ai_addr,
#ifndef WIN32
																								socklen_t ai_addrlen)
#else
																								size_t ai_addrlen)
#endif
{
	char m_chNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortBuffer[6];
	//
	LOG_P(LOG_CAT_I, "ID: " << uiClientIndex << " have status: " << bConnected);
#ifndef WIN32
	getnameinfo(&ai_addr, ai_addrlen, m_chNameBuffer, sizeof(m_chNameBuffer),
				m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#else
	getnameinfo(&ai_addr, ai_addrlen,
				m_chNameBuffer, sizeof(m_chNameBuffer), m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#endif
	LOG_P(LOG_CAT_I, "IP: " << m_chNameBuffer << " Port: " << m_chPortBuffer);
}


/// Точка входа в приложение.
int main(int argc, char *argv[])
							///< \param[in] argc Заглушка.
							///< \param[in] argv Заглушка.
							///< \return Общий результат работы.
{
	argc = argc; // Заглушка.
	argv = argv; // Заглушка.
	LOG_CTRL_INIT;
	LOG_P(LOG_CAT_I, "Starting application, server initialization.");
	Server oServer(S_CONF_PATH, _ptLogMutex);
	string strAdminCommand;
	string strArgument;
	unsigned int uiConnNr = 0;
	size_t szPos;
	void* p_ReceivedData;
	char chTypeCode;
	//
	oServer.SetClientRequestArrivedCB(ClientRequestArrivedCallback);
	oServer.SetClientDataArrivedCB(ClientDataArrivedCallback);
	oServer.SetClientStatusChangedCB(ClientStatusChangedCallback);
	oServer.Start();
gAg:cin >> strAdminCommand;
	if(strAdminCommand.find("exit") == std::string::npos)
	{
		szPos = strAdminCommand.find("select:");
		if(szPos == std::string::npos)
		{
			if(strAdminCommand.find("fetch") == std::string::npos)
			{
				oServer.SendToUser(PROTO_O_TEXT_MSG, (char*)strAdminCommand.c_str(), (int)(strAdminCommand.length() + 1));
			}
			else
			{
				chTypeCode = oServer.AccessCurrentData(&p_ReceivedData);
				if(chTypeCode != DATA_ACCESS_ERROR)
				{
					if(chTypeCode == PROTO_O_TEXT_MSG)
					{
						LOG_P(LOG_CAT_I, "Got last data: " << string((char*)p_ReceivedData));
					}
					else if(chTypeCode != CONNECTION_SEL_ERROR)
					{
						LOG_P(LOG_CAT_W, "Got unknown data with type: " << chTypeCode);
					}
				}
				oServer.ReleaseCurrentData();
			}
		}
		else
		{
			strArgument = strAdminCommand.substr(szPos + 7);
			uiConnNr = (unsigned int)stoi(strArgument);
			oServer.SetCurrentConnection(uiConnNr);
		}
		goto gAg;
	}
	switch (oServer.GetLogStatus())
	{
		case RETVAL_OK:
		{
			oServer.Stop();
			LOG_P(LOG_CAT_I, "Stopping server.");
			while(oServer.CheckReady() == false)
			{
				MSleep(USER_RESPONSE_MS);
			}
			break;
		}
		case RETVAL_ERR:
		{
			LOG_P(LOG_CAT_W, "Server returned an error.");
			break;
		}
	}
	LOG_P(LOG_CAT_I, "Exiting application.");
	LOGCLOSE;
	return LOGRETVAL;
}
