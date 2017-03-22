//== ВКЛЮЧЕНИЯ.
#include "net-hub.h"
#ifndef WIN32
#include <signal.h>
#endif

//== ФУНКЦИИ КЛАССОВ.
//== Класс хаба сетевых операций.
// Конструктор.
NetHub::NetHub()
{
	ResetPocketsBufferPositionPointer();
}

// Сброс указателя позиции в буфере пакетов.
void NetHub::ResetPocketsBufferPositionPointer()
{
	p_chPocketsBufferPositionPointer = m_chPocketsBuffer;
}

// Добавление пакета в буфер отправки.
bool NetHub::AddPocketToOutputBuffer(char chCommand, char *p_chBuffer, int iLength)
{
	unsigned int* p_uiCode;
	//
	if(iLength > (int)(MAX_DATA - 1 - (p_chPocketsBufferPositionPointer - m_chPocketsBuffer) - sizeof(char) - sizeof(int)))
	{
		return false;
	}
	p_uiCode = (unsigned int*)p_chPocketsBufferPositionPointer;
	*p_uiCode = (unsigned int)PROTOCOL_CODE;
	p_uiCode += 1;
	p_chPocketsBufferPositionPointer = (char*)p_uiCode;
	*p_chPocketsBufferPositionPointer = chCommand;
	p_chPocketsBufferPositionPointer += 1;
	if((iLength > 0) & (p_chBuffer != 0))
		memcpy((void*)p_chPocketsBufferPositionPointer, (void*)p_chBuffer, iLength);
	p_chPocketsBufferPositionPointer += iLength;
	return true;
}

// Отправка пакета адресату.
bool NetHub::SendToAddress(ConnectionData &oConnectionData, bool bResetPointer)
{
	int iLength;
#ifndef WIN32
	sigset_t ssOldset, ssNewset;
	siginfo_t sI;
	struct timespec tsTime = {0, 0};
	sigset_t* p_ssNewset;
	//
	p_ssNewset = &ssNewset;
	sigemptyset(&ssNewset);
	sigaddset(&ssNewset, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &ssNewset, &ssOldset);
	iLength = p_chPocketsBufferPositionPointer - m_chPocketsBuffer;
	oConnectionData.iStatus = send(oConnectionData.iSocket, (void*)m_chPocketsBuffer, iLength, 0);
#else
	iLength = (int)(p_chPocketsBufferPositionPointer - m_chPocketsBuffer);
	oConnectionData.iStatus = send(oConnectionData.iSocket,
								   (const char*)m_chPocketsBuffer, iLength, 0);
#endif
#ifndef WIN32
	while(sigtimedwait(p_ssNewset, &sI, &tsTime) >= 0 || errno != EAGAIN);
	pthread_sigmask(SIG_SETMASK, &ssOldset, 0);
#endif
	if(bResetPointer) p_chPocketsBufferPositionPointer = m_chPocketsBuffer;
	if(oConnectionData.iStatus == -1)
	{
		return false;
	}
	else
	{
		return true;
	}
}

// Поиск свободного элемента хранилища пакетов.
int NetHub::FindFreeReceivedPocketsPos(ReceivedData* p_mReceivedPockets)
{
	unsigned int uiPos = 0;
	for(; uiPos != S_MAX_STORED_POCKETS; uiPos++)
	{
		if(p_mReceivedPockets[uiPos].bBusy == false)
		{
			return uiPos;
		}
	}
	return BUFFER_IS_FULL;
}

// Доступ к первому элементу заданного типа из массива принятых пакетов.
int NetHub::AccessSelectedTypeOfData(void** pp_vDataBuffer, ReceivedData* p_mReceivedPockets, char chType)
{
	for(unsigned int uiPos = 0; uiPos < S_MAX_STORED_POCKETS; uiPos++)
	{
		if(p_mReceivedPockets[uiPos].bBusy == true)
		{
			if(p_mReceivedPockets[uiPos].oProtocolStorage.chTypeCode == chType)
			{
				*pp_vDataBuffer = p_mReceivedPockets[uiPos].oProtocolStorage.GetPointer();
				return (int)uiPos;
			}
		}
	}
	return DATA_NOT_FOUND;
}

// Удаление выбранного элемента в массиве принятых пакетов.
int NetHub::ReleaseDataInPosition(ReceivedData* p_mReceivedPockets, unsigned int uiPos)
{
	if(p_mReceivedPockets[uiPos].bBusy == true)
	{
		p_mReceivedPockets[uiPos].bBusy = false;
		p_mReceivedPockets[uiPos].oProtocolStorage.CleanPointers();
		return RETVAL_OK;
	}
	else return DATA_NOT_FOUND;
}
