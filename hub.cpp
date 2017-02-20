//== ВКЛЮЧЕНИЯ.
#include "hub.h"

//== ФУНКЦИИ.
// Отправка пакета адресату.
bool SendToAddress(ConnectionData &oConnectionData, char chCommand, char *p_chBuffer, int iLength)
{
	char m_chData[MAX_DATA];
	//
	if(iLength > (MAX_DATA - 1))
	{
		oConnectionData.iStatus = SOCKET_ERROR_TOO_BIG;
		return false;
	}
	m_chData[0] = chCommand;
	if(iLength > 0)
		memcpy((void*)&m_chData[1], (void*)p_chBuffer, iLength);
#ifdef WIN32
	oConnectionData.iStatus = send(oConnectionData.iSocket, (const char*)m_chData, iLength + 1, 0);
#else
	oConnectionData.iStatus = send(oConnectionData.iSocket, (void*)m_chData, iLength + 1, 0);
#endif
	if(oConnectionData.iStatus == -1)
		return false;
	else
		return true;
}
