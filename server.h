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
#define USER_RESPONSE_MS		100
#define WAITING_FOR_CLIENT_DSC	1000
#define MAX_CONN				16
#define CONNECTION_SEL_ERROR	-1

//== КЛАССЫ.
/// Класс сервера.
class Server
{
private:
	/// Структура описания данных потока соединения.
	struct ConversationThreadData
	{
		bool bInUse; ///< Флаг использования в соотв. потоке.
		int iConnection; ///< ИД соединения.
		pthread_t p_Thread; ///< Указатель на рабочий поток.
		ReceivedData mReceivedPockets[S_MAX_STORED_POCKETS]; ///< Массив принятых пакетов.
		ConnectionData oConnectionData; ///< Данные по соединению.
		char m_chData[MAX_DATA]; ///< Принятый пакет.
		unsigned int uiCurrentFreePocket; ///< Текущий свободный пакет в массиве.
		bool bOverflowOnServer; ///< Флаг переполнения буфера на сервере.
		bool bOverflowOnClient; ///< Флаг переполнения буфера на клиенте.
		bool bSecured; ///< Флаг защищённого соединения.
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
	static char* p_chSettingsPath; ///< Ссылка на строку с путём к установкам сервера.
	static int iSelectedConnection; ///< Индекс соединения для исходящих или CONNECTION_SEL_ERROR.
	static CBConnectionChanged pf_CBConnectionChanged; ///< Ук. на кэлбэк изменения статуса подкл.
	LOGDECL
	LOGDECL_PTHRD_INCLASS_ADD
public:
	/// Конструктор.
	Server(const char* cp_chSettingsPath, pthread_mutex_t ptLogMutex);
								///< \param[in] p_chSettingsPath Ссылка на строку с путём к установкам сервера.
								///< \param[in] ptLogMutex Инициализатор мбютекса лога.
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
	/// Отправка пакета пользователю на текущее выбранное соединение.
	bool SendToUser(char chCommand, char* p_chBuffer, int iLength);
								///< \param[in] chCommand Код команды протокола.
								///< \param[in] p_chBuffer Указатель на буфер с данными для отправки.
								///< \param[in] iLength Длина буфера в байтах.
								///< \return true, при удаче.
	/// Установка текущего индекса соединения для исходящих.
	bool SetCurrentConnectoin(unsigned int uiIndex);
								///< \param[in] uiIndex Индекс соединения.
								///< \return true, если соединение действительно.
	/// Установка указателя кэлбэка изменения статуса подкл.
	void SetConnectionChangedCB(CBConnectionChanged pf_CBConnectionChangedIn);
								///< \param[in] pf_CBConnectionChangedIn Указатель на пользовательскую функцию.
private:
	/// Функция отправки пакета клиенту.
	static bool SendToClient(ConnectionData &oConnectionData,
							 char chCommand, bool bOverflowFlag = false, char* p_chBuffer = 0, int iLength = 0);
								///< \param[in] oConnectionData Ссылка структуру принятых данных и описания соединения.
								///< \param[in] chCommand Код команды протокола.
								///< \param[in] bOverflowFlag Признак переполнения на сервере для фиктивной попытки отправки.
								///< \param[in] p_chBuffer Указатель на буфер с данными для отправки.
								///< \param[in] iLength Длина буфера в байтах.
								///< \return true, при удаче.
	/// Очистка позиции данных потока.
	static void CleanThrDadaPos(unsigned int uiPos);
								///< \param[in] iPos Позиция в массиве.
	/// Поиск свободной позиции данных потока.
	static unsigned int FindFreeThrDadaPos();
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
