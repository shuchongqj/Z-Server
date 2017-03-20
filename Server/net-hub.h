#ifndef NET_HUB_H
#define NET_HUB_H

//== ВКЛЮЧЕНИЯ.
#include "../TinyXML2/tinyxml2.h"
#ifdef WIN32
#include "../dlfcn-win32/dlfcn.h"
#include <WinSock2.h>
#include <vector>
#else
#include <dlfcn.h>
#include <sys/socket.h>
#endif
#include "../logger.h"
#include "../parser-ext.h"
#include "proto-parser.h"

//== МАКРОСЫ.
#define S_MAX_STORED_POCKETS		8
#define C_MAX_STORED_POCKETS		8
#define PTHREAD_TRYLOCK_ATTEMPTS	100
#define PTHREAD_TRYLOCK_TIMESTEP	25
#define MAX_DATA					1024
#define PORTSTRLEN					6
#define BUFFER_IS_FULL				_NMG-1 // См. protocol.h для занятия нового свободного номера.
#define RETURN_THREAD				pthread_exit(0); return 0;
#define SizeOfChars(num)			(sizeof(char) * num)
#ifndef WIN32
#define MSleep(val)					usleep(val * 1000)
#else
#define MSleep(val)					Sleep(val)
#endif
#define	DATA_ACCESS_ERROR			_NMG-2 // См. protocol.h для занятия нового свободного номера.
#define	BUFFER_IS_EMPTY				_NMG-3 // См. protocol.h для занятия нового свободного номера.
#define TryMutexInit			int* p_iLocked
#define TryMutexLock			{p_iLocked = new int;																					\
								unsigned short* p_ushC = new unsigned short;															\
								for(*p_ushC = 0; *p_ushC < PTHREAD_TRYLOCK_ATTEMPTS; *p_ushC += 1)										\
								{																										\
									*p_iLocked = pthread_mutex_trylock(&ptConnMutex);													\
									if(*p_iLocked == 0) break;																			\
									if(*p_ushC > (PTHREAD_TRYLOCK_ATTEMPTS / 10))														\
										LOG_P_2(LOG_CAT_W, "'ptherad' locking is too long.");											\
									MSleep(PTHREAD_TRYLOCK_TIMESTEP);																	\
								}																										\
								delete p_ushC;																							\
								if(*p_iLocked != 0)																						\
								{																										\
									LOG_P_0(LOG_CAT_E, "'ptherad' locking fault.");														\
								}}
#define TryMutexUnlock			{if(*p_iLocked == 0) pthread_mutex_unlock(&ptConnMutex);												\
								delete p_iLocked;}
// Сообщения.
#define MSG_GOT_MERGED			"Merged pockets has been received."

//== КЛАССЫ.
/// Класс хаба сетевых операций.
class NetHub
{
public:
	/// Сруктура для данных по соединению.
	struct ConnectionData
	{
		int iSocket; ///< Сокет.
		sockaddr ai_addr; ///< Адрес.
#ifndef WIN32
		socklen_t ai_addrlen; ///< Длина адреса.
#else
		size_t ai_addrlen; ///< Длина адреса.
#endif
		int iStatus; ///< Статус последней операции.
	};
	/// Структура принятого пакета.
	struct ReceivedData
	{
		bool bFresh; ///< Свежее сообщение.
		ProtocolStorage oProtocolStorage; ///< Принятая структура хаба указателей.
	};

public:
	/// Конструктор.
	NetHub();
	/// Сброс указателя позиции в буфере пакетов.
	void ResetPocketsBufferPositionPointer();
	/// Добавление пакета в буфер.
	bool AddPocketToBuffer(char chCommand, char *p_chBuffer = 0, int iLength = 0);
													///< \param[in] chCommand Команда, которая будет задана в начале пакета.
													///< \param[in] p_chBuffer Указатель на буффер с данными.
													///< \param[in] iLength Длина пакета в байтах.
													///< \return true, при удаче.
	/// Отправка пакета адресату.
	bool SendToAddress(ConnectionData &oConnectionData, bool bResetPointer = true);
													///< \param[in,out] oConnectionData Ссылка на структуру описания соединения.
													///< \param[in] bResetPointer Сбрасывать ли указатель на начало буфера.
													///< \return true, при удаче.

private:
	char m_chPocketsBuffer[MAX_DATA]; ///< Рабочий буфер пакетов.
	char* p_chPocketsBufferPositionPointer; ///< Указатель на позицию в буфере.
};
#endif // NET_HUB_H
