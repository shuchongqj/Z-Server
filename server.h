#ifndef SERVER_H
#define SERVER_H

//== ВКЛЮЧЕНИЯ.
#define _WINSOCKAPI_
#ifdef WIN32
#define _TIMESPEC_DEFINED
#endif
#include "hub.h"
#include "protoparser.h"
#include <signal.h>
#ifndef WIN32
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <termios.h>
#else
#include <pthread/include/pthread.h>
#include <WS2tcpip.h>
#endif

//== МАКРОСЫ.
#define S_CONF_PATH				"./settings/server.ini"
#define USER_RESPONSE_MS		100
#define MAX_CONN				16
#ifndef WIN32
#define MSleep(val)				usleep(val)
#else
#define MSleep(val)				Sleep(val)
#endif
//

//== КЛАССЫ.
/// Класс сервера.
class Server
{
public:
	/// Структура описания данных потока соединения.
	struct ConversationThreadData
	{
		bool bInUse; ///< Флаг использования в соотв. потоке.
		int iConnection; ///< ИД соединения.
		sockaddr saInet; ///< Адрес клиента.
#ifndef WIN32
		socklen_t ai_addrlen; ///< Длина адреса.
#else
		size_t ai_addrlen; ///< Длина адреса.
#endif
		pthread_t p_Thread; ///< Указатель на рабочий поток.
		ReceivedData mReceivedPockets[S_MAX_STORED_POCKETS]; ///< Массив принятых пакетов.
		char m_chData[MAX_DATA]; ///< Принятый пакет.
		unsigned int uiCurrentPocket;
		bool bOverflowOnServer;
		bool bOverflowOnClient;
		bool bSecured;
	};
private:
	static bool bExitSignal; ///< Сигнал на общее завершение.
	static pthread_mutex_t ptConnMutex; ///< Инициализатор мьютекса соединений.
	static int iListener; ///< Сокет приёмника.
	static bool bRequestNewConn; ///< Сигнал запроса нового соединения.
	static ConversationThreadData mThreadDadas[MAX_CONN]; ///< Массив структур описания потоков соединений.
	static bool bListenerAlive; ///< Признак жизни приёмника.
	static char *p_chPassword; ///< Указатель на строку с паролем.
	static pthread_t ServerThr; ///< Идентификатор потока сервера.
	LOGDECL
	LOGDECL_PTHRD_INCLASS_ADD
public:
	/// Конструктор.
	Server(pthread_mutex_t ptLogMutex);
	/// Деструктор.
	~Server();
	/// Запрос запуска сервера.
	void Start();
	/// Запрос остановки сервера.
	void Stop();
	/// Запрос статуса лога.
	unsigned int GetLogStatus();
				///< \return Статус лога по макросам логгера.
	/// Запрос готовности.
	bool CheckReady();
				///< \return true - готов.
private:
	/// Функция отправки пакета клиенту.
	static void SendToClient(bool bOverflowFlag, ConnectionData &oConnectionData,
							 char chCommand, char *p_chBuffer = 0, int iLength = 0);
								///< \param[in] bOverflowFlag Признак переполнения на сервере для фиктивной попытки отправки.
								///< \param[in] oConnectionData Ссылка структуру принятых данных и описания соединения.
								///< \param[in] chCommand Код команды протокола.
								///< \param[in] p_chBuffer Указатель на буфер с данными для отправки.
								///< \param[in] iLength Длина буфера в байтах.
	/// Очистка позиции данных потока.
	static void CleanThrDadaPos(int iPos);
								///< \param[in] iPos Позиция в массиве.
	/// Поиск свободной позиции данных потока.
	static int FindFreeThrDadaPos();
								///< \return Возвращает номер свободной позиции, иначе - RETVAL_ERR.
	/// Поток соединения.
	static void* ConversationThread(void* p_vNum);
								///< \param[in] p_vNum Неопределённый указатель на int-переменную с номером соединения.
								///< \return Заглушка.
	/// Серверный поток.
	static void* ServerThread(void *p_vPlug);
								///< \param[in] p_vPlug Заглушка.
								///< \return Заглушка.
};

#endif // SERVER_H
