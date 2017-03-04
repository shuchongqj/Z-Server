//== ВКЛЮЧЕНИЯ.
#include "mainwindow.h"
#include <QApplication>

///// Кэлбэк обработки статутов подключений клиентов.
//void ClientRequestArrivedCallback(unsigned int uiClientIndex, char chRequest)
//{
//	LOG_P_0(LOG_CAT_I, "Client ID: " << uiClientIndex << " Request: " << chRequest);
//}

///// Кэлбэк обработки прихода пакетов от клиентов.
//void ClientDataArrivedCallback(unsigned int uiClientIndex)
//{
//	uiClientIndex = uiClientIndex; // Заглушка от #define.
//	LOG_P_2(LOG_CAT_I, "Data arrived from ID: " << uiClientIndex);
//}

///// Кэлбэк обработки отслеживания статута клиентов.
//void ClientStatusChangedCallback(bool bConnected, unsigned int uiClientIndex, sockaddr ai_addr,
//#ifndef WIN32
//																								socklen_t ai_addrlen)
//#else
//																								size_t ai_addrlen)
//#endif
//{
//	char m_chNameBuffer[INET6_ADDRSTRLEN];
//	char m_chPortBuffer[6];
//	//
//	LOG_P_0(LOG_CAT_I, "ID: " << uiClientIndex << " have status: " << bConnected);
//#ifndef WIN32
//	getnameinfo(&ai_addr, ai_addrlen, m_chNameBuffer, sizeof(m_chNameBuffer),
//				m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
//#else
//	getnameinfo(&ai_addr, (socklen_t)ai_addrlen,
//				m_chNameBuffer, sizeof(m_chNameBuffer), m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
//#endif
//	LOG_P_0(LOG_CAT_I, "IP: " << m_chNameBuffer << " Port: " << m_chPortBuffer);
//}

//== ФУНКЦИИ.
// Точка входа в приложение.
int main(int argc, char *argv[])
							///< \param[in] argc Заглушка.
							///< \param[in] argv Заглушка.
							///< \return Общий результат работы.
{
	int iExecResult;
//	LOG_CTRL_INIT;
//	LOG_P_0(LOG_CAT_I, "Starting application, server initialization.");
//	Server oServer(S_CONF_PATH, _ptLogMutex);
//	unsigned int uiConnNr = 0;
//	size_t szPos;
//	void* p_ReceivedData;
//	char chTypeCode;
	QApplication oApplication(argc, argv);
	MainWindow wMainWindow;
	//
//	oServer.SetClientRequestArrivedCB(ClientRequestArrivedCallback);
//	oServer.SetClientDataArrivedCB(ClientDataArrivedCallback);
//	oServer.SetClientStatusChangedCB(ClientStatusChangedCallback);

	wMainWindow.show();
	iExecResult = oApplication.exec();
	return iExecResult;

//	LOG_P_0(LOG_CAT_I, "Exiting application.");
//	LOGCLOSE;
//	return LOGRETVAL;
}
