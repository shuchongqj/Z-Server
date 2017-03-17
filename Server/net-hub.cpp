//== ВКЛЮЧЕНИЯ.
#include "net-hub.h"
#ifndef WIN32
#include <signal.h>
#endif

//== ПЕРЕМЕННЫЕ.
char m_chPocketsBuffer[MAX_DATA];
char* p_chPocketsBufferPointer = m_chPocketsBuffer;

//== ФУНКЦИИ.
// Создание заголовка пакета.
bool AddPocketToBuffer(char chCommand, char *p_chBuffer, int iLength)
{
	unsigned int* p_uiCode;
	//
	if(iLength > (int)(MAX_DATA - 1 - (p_chPocketsBufferPointer - m_chPocketsBuffer) - sizeof(char) - sizeof(int)))
	{
		return false;
	}
	p_uiCode = (unsigned int*)p_chPocketsBufferPointer;
	*p_uiCode = (unsigned int)PROTOCOL_CODE;
	p_uiCode += 1;
	p_chPocketsBufferPointer = (char*)p_uiCode;
	*p_chPocketsBufferPointer = chCommand;
	p_chPocketsBufferPointer += 1;
	if((iLength > 0) & (p_chBuffer != 0))
		memcpy((void*)p_chPocketsBufferPointer, (void*)p_chBuffer, iLength);
	p_chPocketsBufferPointer += iLength;
	return true;
}

// Отправка пакета адресату.
bool SendToAddress(ConnectionData &oConnectionData)
{
	int iLength;
	//
#ifndef WIN32
	sigset_t ssOldset, ssNewset;
	siginfo_t sI;
	struct timespec tsTime = {0, 0};
	sigset_t* p_ssNewset;
	p_ssNewset = &ssNewset;
	sigemptyset(&ssNewset);
	sigaddset(&ssNewset, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &ssNewset, &ssOldset);
	iLength = p_chPocketsBufferPointer - m_chPocketsBuffer;
	oConnectionData.iStatus = send(oConnectionData.iSocket, (void*)m_chPocketsBuffer, iLength, 0);
#else
	oConnectionData.iStatus = send(oConnectionData.iSocket,
								   (const char*)mm_chPocketsBuffer, p_chPocketsBufferPointer - m_chPocketsBuffer, 0);
#endif
#ifndef WIN32
	while(sigtimedwait(p_ssNewset, &sI, &tsTime) >= 0 || errno != EAGAIN);
	pthread_sigmask(SIG_SETMASK, &ssOldset, 0);
#endif
	p_chPocketsBufferPointer = m_chPocketsBuffer;
	if(oConnectionData.iStatus == -1)
	{
		return false;
	}
	else
	{
		return true;
	}
}
