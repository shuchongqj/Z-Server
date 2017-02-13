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

//== МАКРОСЫ.
#define S_CONF_PATH				"./settings/server.ini"
#define C_CONF_PATH				"./settings/client.ini"
#define MAX_STORED_POCKETS		16
#define MAX_DATA				1024
#define S_BUFFER_OVERFLOW		"Buffer overflow for"
#define C_BUFFER_OVERFLOW		"Buffer overflow on server."
#define OWNER_RESPONSE_MS		100
#define SOCKET_ERROR_TOO_BIG	65535

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
	char m_chData[MAX_DATA]; ///< Принятый пакет.
};

//== ФУНКЦИИ.
/// Отправка пакета адресату.
void SendToAddress(ConnectionData &oConnectionData, char chCommand, char *p_chBuffer = 0, int iLength = 0);
													///< \param[in,out] oConnectionData Ссылка на структуру описания соединения.
													///< \param[in] chCommand Команда, которая будет задана в начале пакета.
													///< \param[in] p_chBuffer Указатель на буффер с данными.
													///< \param[in] iLength Длина пакета в байтах.
#endif // HUB_H
