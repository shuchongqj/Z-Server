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
void ClientStateChangedCallback(unsigned int uiClientIndex, bool bConnected)
{
	LOG_P(LOG_CAT_I, "Client ID: " << uiClientIndex << " Status: " << bConnected);
}

/// Кэлбэк обработки прихода пакетов от клиентов.
void ClientDataArrivedCallback(unsigned int uiClientIndex)
{
	LOG_P(LOG_CAT_I, "Data arrived from ID: " << uiClientIndex);
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
	oServer.SetClientStateChangedCB(ClientStateChangedCallback);
	oServer.SetClientDataArrivedCB(ClientDataArrivedCallback);
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
