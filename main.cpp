//== ВКЛЮЧЕНИЯ.
#include "server.h"

//== МАКРОСЫ.
#define LOG_NAME				"Z-Admin"
#define S_CONF_PATH				"./settings/server.ini"
#ifdef WIN32
#include <string>
#endif

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL

/// Точка входа в приложение.
int main(int argc, char *argv[])
							///< \param[in] argc Заглушка.
							///< \param[in] argv Заглушка.
							///< \return Общий результат работы.
{
	argc = argc; // Заглушка.
	argv = argv; // Заглушка.
	LOG_CTRL_INIT;
	LOGDECL_INIT_PTHRD_ADD;
	LOG_P(LOG_CAT_I, "Starting application, server initialization.");
	Server oServer(S_CONF_PATH, _ptLogMutex);
	string strAdminCommand;
	string strArgument;
	unsigned int uiConnNr = 0;
	size_t szPos;
	//
	oServer.Start();
gAg:cin >> strAdminCommand;
	if(strAdminCommand.find("exit") == std::string::npos)
	{
		szPos = strAdminCommand.find("select:");
		if(szPos == std::string::npos)
		{
			bool bRes;
			//
			bRes = oServer.SendToUser(PROTO_O_TEXT_MSG, (char*)strAdminCommand.c_str(), (int)(strAdminCommand.length() + 1));
			if(!bRes)
			{
				LOG_P(LOG_CAT_E, "Sending failed.");
			}
			else
			{
				LOG_P(LOG_CAT_I, "Message has been sent.");
			}
		}
		else
		{
			strArgument = strAdminCommand.substr(szPos + 7);
			uiConnNr = (unsigned int)stoi(strArgument);
			if(uiConnNr > (MAX_CONN - 1))
			{
				LOG_P(LOG_CAT_E, "Index is out of range.");
			}
			else
			{
				oServer.SetCurrentConnectoin(uiConnNr);
			}
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
