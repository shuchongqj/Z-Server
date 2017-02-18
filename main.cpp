//== ВКЛЮЧЕНИЯ.
#include "server.h"

//== МАКРОСЫ.
#define LOG_NAME				"Z-Admin"

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
	Server oServer(_ptLogMutex);
	//
	oServer.Start();
	getchar();
	LOG_P(LOG_CAT_I, "Stopping server.");
	oServer.Stop();
	while(oServer.CheckReady() == false)
	{
		MSleep(USER_RESPONSE_MS);
	}
	LOG_P(LOG_CAT_I, "Exiting application.");
	LOGCLOSE;
	return LOGRETVAL;
}
