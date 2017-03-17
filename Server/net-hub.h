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
#define S_MAX_STORED_POCKETS	8
#define C_MAX_STORED_POCKETS	8
#define MAX_DATA				1024
#define PORTSTRLEN				6
#define BUFFER_IS_FULL			_NMG-1 // См. protocol.h для занятия нового свободного номера.
#define RETURN_THREAD			pthread_exit(0); return 0;
#define SizeOfChars(num)		(sizeof(char) * num)
#ifndef WIN32
#define MSleep(val)				usleep(val * 1000)
#else
#define MSleep(val)				Sleep(val)
#endif
#define	DATA_ACCESS_ERROR		_NMG-2 // См. protocol.h для занятия нового свободного номера.
#define	BUFFER_IS_EMPTY			_NMG-3 // См. protocol.h для занятия нового свободного номера.
#define MSG_GOT_MERGED			"Merged pockets has been received."

//== СТРУКТУРЫ.
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

//== ФУНКЦИИ.
// Создание заголовка пакета.
bool AddPocketToBuffer(char chCommand, char *p_chBuffer = 0, int iLength = 0);
													///< \param[in] chCommand Команда, которая будет задана в начале пакета.
													///< \param[in] p_chBuffer Указатель на буффер с данными.
													///< \param[in] iLength Длина пакета в байтах.
													///< \return true, при удаче.
/// Отправка пакета адресату.
bool SendToAddress(ConnectionData &oConnectionData);
													///< \param[in,out] oConnectionData Ссылка на стр. описания соединения.
													///< \return true, при удаче.
#endif // NET_HUB_H
