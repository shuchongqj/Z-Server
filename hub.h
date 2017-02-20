#ifndef HUB_H
#define HUB_H

//== ВКЛЮЧЕНИЯ.
#include "TinyXML2/tinyxml2.h"
#ifdef WIN32
#include "dlfcn-win32/dlfcn.h"
#include <WinSock2.h>
#else
#include <dlfcn.h>
#include <sys/socket.h>
#endif
#include "logger.h"
#include "parserext.h"
#include "protoparser.h"

//== МАКРОСЫ.
#define S_MAX_STORED_POCKETS	6
#define C_MAX_STORED_POCKETS	4
#define MAX_DATA				1024
#define SOCKET_ERROR_TOO_BIG	65535
#define S_BUFFER_READY			"Buffer is ready on server."
#define C_BUFFER_READY			"Buffer is ready on client."
#define RETURN_THREAD			pthread_exit(0);																		\
								return 0;
#ifndef WIN32
#define MSleep(val)				usleep(val * 1000)
#else
#define MSleep(val)				Sleep(val)
#endif

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
	bool bProcessed;
	ProtoParser::ParsedObject oParsedObject; ///< Принятый пакет.
};

//== ФУНКЦИИ.
/// Отправка пакета адресату.
void SendToAddress(ConnectionData &oConnectionData, char chCommand, char *p_chBuffer = 0, int iLength = 0);
													///< \param[in,out] oConnectionData Ссылка на стр. описания соединения.
													///< \param[in] chCommand Команда, которая будет задана в начале пакета.
													///< \param[in] p_chBuffer Указатель на буффер с данными.
													///< \param[in] iLength Длина пакета в байтах.
#endif // HUB_H
