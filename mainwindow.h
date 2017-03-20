#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//== ВКЛЮЧЕНИЯ.
#include <QMainWindow>
#include <QSettings>
#include <QTimer>
// Для избежания ошибки при доступе из другого потока.
#include <QVector>
//
#include "Server/server.h"
#include "main-hub.h"

//== ПРОСТРАНСТВА ИМЁН.
namespace Ui {
	class MainWindow;
}

// Для избежания ошибки при доступе из другого потока.
Q_DECLARE_METATYPE(QVector<int>)
//

//== КЛАССЫ.
/// Класс главного окна.
class MainWindow : public QMainWindow
{
	Q_OBJECT

private:
	/// Структура авторизации.
	struct AuthorizationUnit
	{
		char chLevel; ///< Уровень пользователя.
		char m_chLogin[MAX_AUTH_LOGIN]; ///< Ник пользователя.
		char m_chPassword[MAX_AUTH_PASSWORD]; ///< Пароль пользователя.
		int iConnectionIndex; ///< Текущее соединение или
	};
	/// Структура бана по нику.
	struct UserBanUnit
	{
		char m_chLogin[MAX_AUTH_LOGIN]; ///< Ник пользователя.
	};

public:
	int iInitRes; ///< Результат инициализации.

public:
	/// Конструктор.
	explicit MainWindow(QWidget* p_parent = 0);
							///< \param[in] p_parent - Указатель на родительский виджет.
	/// Деструктор.
	~MainWindow();
	/// Процедуры при закрытии окна приложения.
	void closeEvent(QCloseEvent* event);
							///< \param[in] event - Указатель на событие.

	/// Кэлбэк обработки отслеживания статута клиентов.
	static void ClientStatusChangedCallback(NetHub& a_NetHub, bool bConnected, unsigned int uiClientIndex);
							///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
							///< \param[in] bConnected Статус подключения.
							///< \param[in] uiClientIndex Индекс клиента.
	/// Кэлбэк обработки приходящих пакетов данных.
	static void ClientDataArrivedCallback(NetHub& a_NetHub, unsigned int uiClientIndex);
							///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
							///< \param[in] uiClientIndex Индекс клиента.
	/// Кэлбэк обработки приходящих запросов.
	static void ClientRequestArrivedCallback(NetHub& a_NetHub, unsigned int uiClientIndex, char chRequest);
							///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
							///< \param[in] uiClientIndex Индекс клиента.
							///< \param[in] chRequest Запрос клиента.

private:
	/// Загрузка каталога банов.
	static bool LoadBansCatalogue();
							///< \return true, при удаче.
	/// Сохранение каталога банов.
	static bool SaveBansCatalogue();
							///< \return true, при удаче.
	/// Загрузка каталога пользователей.
	static bool LoadUsersCatalogue();
							///< \return true, при удаче.
	/// Сохранение каталога пользователей.
	static bool SaveUsersCatalogue();
							///< \return true, при удаче.
	/// Процедуры запуска сервера.
	static bool ServerStartProcedures();
							///< \return true, при удаче.
	/// Процедуры остановки сервера.
	static void ServerStopProcedures();
	/// Процедуры при логине пользователя.
	static void UserLoginProcedures(NetHub& a_NetHub, int iPosition,
									  unsigned int iIndex, NetHub::ConnectionData& a_ConnectionData, bool bTryLock = true);
							///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
							///< \param[in] iPosition Позиция в списке автоизации.
							///< \param[in] iIndex Индекс соединения.
							///< \param[in] a_ConnectionData Ссылка на данные по соединению.
							///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
	/// Процедуры при логауте пользователя.
	static int UserLogoutProcedures(NetHub& a_NetHub, int iPosition,
									   NetHub::ConnectionData& a_ConnectionData, char chAnswer = AUTH_ANSWER_OK,
									bool bSend = false, bool bTryLock = true);
							///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
							///< \param[in] iPosition Позиция в списке автоизации.
							///< \param[in] a_ConnectionData Ссылка на данные по соединению.
							///< \param[in] chAnswer Ответ пользователю.
							///< \param[in] bSend Отсылать ли отчёт.
							///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
							///< \return Новый номер текущего элемента в листе авторизации после замены параметров.
	/// Процедуры при удалении пользователя.
	static int UserPurgeProcedures(NetHub& a_NetHub, int iPosition,
										NetHub::ConnectionData* p_ConnectionData, char chAnswer = AUTH_ANSWER_OK,
								   bool bLogout = true, bool bTryLock = true);
							///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
							///< \param[in] iPosition Позиция в списке автоизации.
							///< \param[in] p_ConnectionData Указатель на данные по соединению (не используется если не нужен логаут).
							///< \param[in] chAnswer Ответ пользователю.
							///< \param[in] bLogout Нужны ли процедуры логаута бывшего пользователя.
							///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.
							///< \return Новый номер текущего элемента в листе авторизации после замены параметров.
	/// Процедуры при блокировке пользователя.
	static int UserBanProcedures(int iPosition);
							///< \param[in] iPosition Позиция в списке.
							///< \return Новый номер текущего элемента в листе авторизации после замены параметров.
	/// Блокировка и отключение по имени адреса.
	static void BanAndKickByAdressWithMenuProcedures(NetHub& a_NetHub, QString& a_strAddrName);
							///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
							///< \param[in] a_strAddrName Ссылка на строку с именем IP.
	/// Информирование пользователей о изменении лобби.
	static void LobbyChangedInform(NetHub& a_NetHub, bool bTryLock = true);
							///< \param[in] a_NetHub Ссылка на используемый NetHub (с собственным буфером пакетов).
							///< \param[in] bTryLock Установить в false при использовании внутри кэлбэков.

public slots:
	/// Обновление чата.
	static void slot_UpdateChat();

private slots:
	/// При нажатии на 'О программе'.
	static void on_About_action_triggered();
	/// При завершении ввода строки чата.
	static void on_Chat_lineEdit_returnPressed();
	/// При переключении кнопки 'Пуск/Стоп'.
	static void on_StartStop_action_triggered(bool checked);
							///< \param[in] checked - Позиция переключателя.
	/// При изменении текста чата.
	static void on_Chat_textBrowser_textChanged();
	/// При переключении кнопки 'Автостарт'.
	static void on_Autostart_action_triggered(bool checked);
							///< \param[in] checked - Позиция переключателя.
	/// При нажатии ПКМ на элементе списка пользователей.
	static void on_Users_listWidget_customContextMenuRequested(const QPoint &pos);
							///< \param[in] pos Ссылка на координаты точки указателя в виджете.
	/// При нажатии ПКМ на элементе списка банов по пользователям.
	static void on_U_Bans_listWidget_customContextMenuRequested(const QPoint &pos);
							///< \param[in] pos Ссылка на координаты точки указателя в виджете.
	/// При нажатии ПКМ на элементе списка соединений.
	static void on_Clients_listWidget_customContextMenuRequested(const QPoint &pos);
							///< \param[in] pos Ссылка на координаты точки указателя в виджете.
	/// При нажатии ПКМ на элементе списка банов по адресам.
	static void on_C_Bans_listWidget_customContextMenuRequested(const QPoint &pos);
							///< \param[in] pos Ссылка на координаты точки указателя в виджете.

private:
	static Ui::MainWindow *p_ui; ///< Указатель на UI.
	static const char* cp_chUISettingsName; ///< Указатель на имя файла с установками UI.
	static QSettings* p_UISettings; ///< Указатель на строку установок UI.
	static Server* p_Server; ///< Ссылка на объект сервера.
	static QList<unsigned int> lst_uiConnectedClients; ///< Список присоединённых клиентов.
	static list<XMLNode*> o_lUsers; ///< Главный список разъёмов документа авторизации.
	static list<XMLNode*> o_lUserBans; ///< Главный список разъёмов документа банов по никам.
	static list<XMLNode*> o_lIPBans; ///< Главный список разъёмов документа банов по адресам.
	static QList<AuthorizationUnit> lst_AuthorizationUnits; ///< Список авторизованных пользователей.
	static QList<UserBanUnit> lst_UserBanUnits; ///< Список банов по никам.
	static vector<Server::IPBanUnit> vec_IPBanUnits; ///< Список банов по адресам.
	static bool bAutostart; ///< Флаг автозапуска.
	static QTimer* p_ChatTimer; ///< Указатель на таймер обновления GUI.
	static char m_chTextChatBuffer[MAX_MSG]; ///< Буфер обмена с виджетом чата.
	static char m_chIPNameBufferUI[INET6_ADDRSTRLEN];
	static char m_chPortNameBufferUI[PORTSTRLEN];
	LOGDECL
	LOGDECL_PTHRD_INCLASS_ADD
	static NetHub oPrimaryNetHub;
};

#endif // MAINWINDOW_H
