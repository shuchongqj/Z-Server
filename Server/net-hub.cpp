//== ВКЛЮЧЕНИЯ.
#include "net-hub.h"
#include <signal.h>

//== ФУНКЦИИ.
// Отправка пакета адресату.
bool SendToAddress(ConnectionData &oConnectionData, char chCommand, char *p_chBuffer, int iLength)
{
	sigset_t ssOldset, ssNewset;
	siginfo_t sI;
	struct timespec tsTime = {0, 0};
	sigset_t* p_ssNewset;
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
	//
	p_ssNewset = &ssNewset;
	sigemptyset(&ssNewset);
	sigaddset(&ssNewset, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &ssNewset, &ssOldset);
#ifdef WIN32
	oConnectionData.iStatus = send(oConnectionData.iSocket, (const char*)m_chData, iLength + 1, 0);
#else
	oConnectionData.iStatus = send(oConnectionData.iSocket, (void*)m_chData, iLength + 1, 0);
#endif
	while(sigtimedwait(p_ssNewset, &sI, &tsTime) >= 0 || errno != EAGAIN);
	pthread_sigmask(SIG_SETMASK, &ssOldset, 0);
	if(oConnectionData.iStatus == -1)
		return false;
	else
		return true;
}
