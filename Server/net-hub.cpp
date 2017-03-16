//== ВКЛЮЧЕНИЯ.
#include "net-hub.h"
#ifndef WIN32
#include <signal.h>
#endif

//== ФУНКЦИИ.
// Отправка пакета адресату.
bool SendToAddress(ConnectionData &oConnectionData, char chCommand, char *p_chBuffer, int iLength)
{
	char* p_chCode;
	unsigned int* p_uiCode;
	//
#ifndef WIN32
	sigset_t ssOldset, ssNewset;
	siginfo_t sI;
	struct timespec tsTime = {0, 0};
	sigset_t* p_ssNewset;
#endif
	char m_chData[MAX_DATA];
	//
	if(iLength > (MAX_DATA - 1))
	{
		oConnectionData.iStatus = SOCKET_ERROR_TOO_BIG;
		return false;
	}
	p_uiCode = (unsigned int*)m_chData;
	*p_uiCode = (unsigned int)PROTOCOL_CODE;
	p_uiCode += 1;
	p_chCode = (char*)p_uiCode;
	*p_chCode = chCommand;
	p_chCode += 1;
	if(iLength > 0)
		memcpy((void*)p_chCode, (void*)p_chBuffer, iLength);
	//
#ifndef WIN32
	p_ssNewset = &ssNewset;
	sigemptyset(&ssNewset);
	sigaddset(&ssNewset, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &ssNewset, &ssOldset);
	oConnectionData.iStatus = send(oConnectionData.iSocket, (void*)m_chData, iLength + sizeof(char) + sizeof(unsigned int), 0);
#else
	oConnectionData.iStatus = send(oConnectionData.iSocket, (const char*)m_chData, iLength + sizeof(char) + sizeof(unsigned int), 0);
#endif
#ifndef WIN32
	while(sigtimedwait(p_ssNewset, &sI, &tsTime) >= 0 || errno != EAGAIN);
	pthread_sigmask(SIG_SETMASK, &ssOldset, 0);
#endif
	if(oConnectionData.iStatus == -1)
		return false;
	else
		return true;
}
