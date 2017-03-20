#ifndef SERVER_H
#define SERVER_H

//== ВКЛЮЧЕНИЯ.
#define _WINSOCKAPI_
#ifdef WIN32
#define _TIMESPEC_DEFINED
#endif
#include "net-hub.h"
#include "proto-parser.h"
#include <signal.h>
#ifndef WIN32
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
//#include <X11/Xlib.h> // (console)
//#include <X11/Xutil.h> // (console)
#include <termios.h>
#else
#include <pthread/include/pthread.h>
#include <WS2tcpip.h>
#endif

//== ОПРЕДЕЛЕНИЯ ТИПОВ.
typedef void (*CBClientRequestArrived)(NetHub& a_NetHub, unsigned int uiClientIndex, char chRequest);
typedef void (*CBClientDataArrived)(NetHub& a_NetHub, unsigned int uiClientIndex);
typedef void (*CBClientStatusChanged)(NetHub& a_NetHub, bool bConnected, unsigned int uiClientIndex);

//== МАКРОСЫ.
#define USER_RESPONSE_MS		100
#define WAITING_FOR_CLIENT_DSC	1000
#define MAX_CONN				2
#define CONNECTION_SEL_ERROR	_NMG-5 // См. protocol.h для занятия нового свободного номера.

//== КЛАССЫ.
/// Класс сервера.
class Server
{
public:
	/// Структура бана по адресу.
	struct IPBanUnit
	{
		char m_chIP[INET6_ADDRSTRLEN]; ///< Адрес пользователя.
	};

private:
	/// Структура описания данных потока соединения.
	struct ConversationThreadData
	{
		bool bInUse; ///< Флаг использования в соотв. потоке.
		pthread_t p_Thread; ///< Указатель на рабочий поток.
		NetHub::ReceivedData mReceivedPockets[S_MAX_STORED_POCKETS]; ///< Массив принятых пакетов.
		NetHub::ConnectionData oConnectionData; ///< Данные по соединению.
		char m_chData[MAX_DATA]; ///< Принятый пакет.
		unsigned int uiCurrentFreePocket; ///< Текущий свободный пакет в массиве.
		bool bFullOnServer; ///< Флаг переполнения буфера на сервере.
		bool bFullOnClient; ///< Флаг переполнения буфера на клиенте.
		bool bSecured; ///< Флаг защищённого соединения.
		bool bKick; ///< Флаг команды на выброс клиента.
	};

private:
	static bool bServerAlive; ///< Признак жизни потока сервера.
	static bool bExitSignal; ///< Сигнал на общее завершение.
	static pthread_mutex_t ptConnMutex; ///< Инициализатор мьютекса соединений.
	static int iListener; ///< Сокет приёмника.
	static bool bRequestNewConn; ///< Сигнал запроса нового соединения.
	static ConversationThreadData mThreadDadas[MAX_CONN]; ///< Массив структур описания потоков соединений.
	static bool bListenerAlive; ///< Признак жизни потока приёмника.
	static char *p_chPassword; ///< Указатель на строку с паролем.
	static pthread_t ServerThr; ///< Идентификатор потока сервера.
	static char* p_chSettingsPath; ///< Ссылка на строку с путём к установкам сервера.
	static int iSelectedConnection; ///< Индекс соединения для исходящих или CONNECTION_SEL_ERROR.
	static CBClientRequestArrived pf_CBClientRequestArrived; ///< Указатель на кэлбэк приёма запросов.
	static CBClientDataArrived pf_CBClientDataArrived; ///< Указатель на кэлбэк приёма пакетов.
	static CBClientStatusChanged pf_CBClientStatusChanged; ///< Указатель на кэлбэк отслеживания статута клиентов.
	static pthread_t p_ThreadOverrunned; ///< Указатель на рабочий поток при переполнении.
	static vector<IPBanUnit>* p_vec_IPBansInt; ///< Внутреннй указатель на список с банами по IP.
	LOGDECL
	LOGDECL_PTHRD_INCLASS_ADD

public:
	/// Конструктор.
	Server(const char* cp_chSettingsPathIn, pthread_mutex_t ptLogMutex, vector<IPBanUnit>* p_vec_IPBans = 0);
								///< \param[in] cp_chSettingsPathIn Ссылка на строку с путём к установкам сервера.
								///< \param[in] ptLogMutex Инициализатор мьютекса лога.
								///< \param[in] p_vec_IPBans Установка внутреннего указателя на список с банами по IP.
	/// Деструктор.
	~Server();
	/// Запрос запуска сервера.
	static void Start();
	/// Запрос остановки сервера.
	static void Stop();
	/// Запрос статуса лога.
	static int GetLogStatus();
				///< \return Статус лога по макросам логгера.
	/// Запрос готовности.
	static bool CheckReady();
				///< \return true - готов.
	/// Отправка пакета клиенту на текущее выбранное соединение немедленно.
	static bool SendToClientImmediately(NetHub& a_NetHub, char chCommand, char* p_chBuffer, int iLength,
										bool bResetPointer = true, bool bTryLock = true);
								///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
								///< \param[in] chCommand Код команды протокола.
								///< \param[in] p_chBuffer Указатель на буфер с данными для отправки.
								///< \param[in] iLength Длина буфера в байтах.
								///< \param[in] bResetPointer Сбрасывать ли указатель на начало буфера (для нового заполнения).
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
								///< \return true, при удаче.
	/// Отправка буфера клиенту на текущее выбранное соединение.
	static bool SendBufferToClient(NetHub& a_NetHub, bool bResetPointer = true, bool bTryLock = true);
								///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
								///< \param[in] bResetPointer Сбрасывать ли указатель на начало буфера (для нового заполнения).
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
								///< \return true, при удаче.
	// Получение текущего индекса соединения для исходящих.
	int GetCurrentConnection(bool bTryLock = true);
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
	/// Установка текущего индекса соединения для исходящих.
	static bool SetCurrentConnection(unsigned int uiIndex, bool bTryLock = true);
								///< \param[in] uiIndex Индекс соединения.
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
								///< \return true, если соединение действительно.
	/// Уст. ук. кэлбэка изменения статуса подключения клиента;
	/// a_NetHub (буфер отправляемых паетов) использовать только единовременно - он уникален для потока и используется внутри него.
	/// Внутри кэлбэка ОБЯЗАТЕЛЬНО в вызовах с возможностью установки bTryLock - ставить false, кэлбэки и так под локом.
	static void SetClientRequestArrivedCB(CBClientRequestArrived pf_CBClientRequestArrivedIn);
								///< \param[in] pf_CBClientRequestArrivedIn Указатель на пользовательскую функцию.
	/// Уст.ук. кэлбэка обработки принятых пакетов от клиентов;
	/// a_NetHub (буфер отправляемых паетов) использовать только единовременно - он уникален для потока и используется внутри него.
	/// Внутри кэлбэка ОБЯЗАТЕЛЬНО в вызовах с возможностью установки bTryLock - ставить false, кэлбэки и так под локом.
	static void SetClientDataArrivedCB(CBClientDataArrived pf_CBClientDataArrivedIn);
								///< \param[in] pf_CBClientDataArrivedIn Указатель на пользовательскую функцию.
	/// Уст. ук. кэлбэка отслеживания статута клиентов;
	/// a_NetHub (буфер отправляемых паетов) использовать только единовременно - он уникален для потока и используется внутри него.
	/// Внутри кэлбэка ОБЯЗАТЕЛЬНО в вызовах с возможностью установки bTryLock - ставить false, кэлбэки и так под локом.
	static void SetClientStatusChangedCB(CBClientStatusChanged pf_CBClientStatusChangedIn);
								///< \param[in] pf_CBClientStatusChangedIn Указатель на пользовательскую функцию.
	/// Доступ к крайнему элементу из массива принятых пакетов от текущего клиента.
	static int AccessCurrentData(void** pp_vDataBuffer, bool bTryLock = true);
								///< \param[in,out] pp_vDataBuffer Указатель на указатель на буфер с данными.
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
								///< \return Код пакета, DATA_ACCESS_ERROR при ошибке, CONNECTION_SEL_ERROR соотв.
	/// Удаление крайнего элемента из массива принятых пакетов.
	static int ReleaseCurrentData(NetHub& a_NetHub, bool bTryLock = true);
								///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
								///< \return RETVAL_OK, если удачно, BUFFER_IS_EMPTY, если пусто, CONNECTION_SEL_ERROR соотв.
	/// Получение копии структуры описания соединения по индексу.
	static NetHub::ConnectionData GetConnectionData(unsigned int uiIndex, bool bTryLock = true);
								///< \param[in] uiIndex Индекс соединения.
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
								///< \return NetHub::ConnectionData.iStatus == CONNECTION_SEL_ERROR - соединение не действительно.
	/// Заполнение буферов имён IP и порта.
	static void FillIPAndPortNames(NetHub::ConnectionData& a_ConnectionData, char* p_chIP, char* p_chPort = 0, bool bTryLock = true);
								///< \param[in] a_ConnectionData Ссылка на структуру описания соединения.
								///< \param[in,out] p_chIP Указатель на буфер имени IP.
								///< \param[in,out] p_chPort Указатель на буфер имени порта.
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
	/// Принудительное отключение клиента.
	static void KickClient(NetHub& a_NetHub, unsigned int uiIndex, bool bTryLock = true);
								///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
								///< \param[in] uiIndex Индекс соединения.
								///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
private:
	/// Функция отправки пакета по соединению немедленно.
	static bool SendToConnectionImmediately(NetHub& a_NetHub, NetHub::ConnectionData& a_ConnectionData,
							 char chCommand, bool bFullFlag = false, char* p_chBuffer = 0,
											int iLength = 0, bool bResetPointer = true);
								///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
								///< \param[in] a_ConnectionData Ссылка структуру принятых данных и описания соединения.
								///< \param[in] chCommand Код команды протокола.
								///< \param[in] bFullFlag Признак переполнения на сервере для фиктивной попытки отправки.
								///< \param[in] p_chBuffer Указатель на буфер с данными для отправки.
								///< \param[in] iLength Длина буфера в байтах.
								///< \param[in] bResetPointer Сбрасывать ли указатель на начало буфера (для нового заполнения).
								///< \return true, при удаче.
	/// Функция отправки буфера по соединению.
	static bool SendBufferToConnection(NetHub& a_NetHub, NetHub::ConnectionData &a_ConnectionData,
									   bool bFullFlag = false, bool bResetPointer = true);
								///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
								///< \param[in] a_ConnectionData Ссылка структуру принятых данных и описания соединения.
								///< \param[in] bFullFlag Признак переполнения на сервере для фиктивной попытки отправки.
								///< \param[in] bResetPointer Сбрасывать ли указатель на начало буфера (для нового заполнения).
								///< \return true, при удаче.
	/// Очистка позиции данных потока.
	static void CleanThrDadaPos(unsigned int uiPos);
								///< \param[in] iPos Позиция в массиве.
	/// Поиск свободной позиции данных потока.
	static int FindFreeThrDadaPos();
								///< \return Возвращает номер свободной позиции, иначе - CONNECTION_SEL_ERROR.
	/// Поток соединения.
	static void* ConversationThread(void* p_vNum);
								///< \param[in] p_vNum Неопределённый указатель на int-переменную с номером соединения.
								///< \return Заглушка.
	/// Серверный поток.
	static void* ServerThread(void *p_vPlug);
								///< \param[in] p_vPlug Заглушка.
								///< \return Заглушка.
	/// Заполнение структуры описания соединения.
	static void FillConnectionData(int iSocket, NetHub::ConnectionData& a_ConnectionData);
								///< \param[in] iSocket Сокет.
								///< \param[in,out] a_ConnectionData Ссылка на структуру для заполнения.
};

#endif // SERVER_H
