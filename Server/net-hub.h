#ifndef HUB_H
#define HUB_H

//== ВКЛЮЧЕНИЯ.
#include "../TinyXML2/tinyxml2.h"
#ifdef WIN32
#include "../dlfcn-win32/dlfcn.h"
#include <WinSock2.h>
#else
#include <dlfcn.h>
#include <sys/socket.h>
#endif
#include "../logger.h"
#include "../parser-ext.h"
#include "proto-parser.h"

//== МАКРОСЫ.
#define S_MAX_STORED_POCKETS	6
#define C_MAX_STORED_POCKETS	4
#define MAX_DATA				1024
#define SOCKET_ERROR_TOO_BIG	65535
#define RETURN_THREAD			pthread_exit(0); return 0;
#ifndef WIN32
#define MSleep(val)				usleep(val * 1000)
#else
#define MSleep(val)				Sleep(val)
#endif
#define	DATA_ACCESS_ERROR		1
#define	BUFFER_IS_EMPTY			2

//== ОПРЕДЕЛЕНИЯ ТИПОВ.
typedef void (*CBConnectionChanged)(unsigned int uiIndex, bool bConnected);

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
/// Отправка пакета адресату.
bool SendToAddress(ConnectionData &oConnectionData, char chCommand, char *p_chBuffer = 0, int iLength = 0);
													///< \param[in,out] oConnectionData Ссылка на стр. описания соединения.
													///< \param[in] chCommand Команда, которая будет задана в начале пакета.
													///< \param[in] p_chBuffer Указатель на буффер с данными.
													///< \param[in] iLength Длина пакета в байтах.
													///< \return true, при удаче.
#endif // HUB_H
