#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//== ВКЛЮЧЕНИЯ.
#include <QMainWindow>
#include <QSettings>
// Для избежания ошибки при доступе к текстовому браузеру из другого потока.
#include <QTextCursor>
#include <QTextBlock>
#include <QVector>
//
#include "Server/server.h"
#include "main-hub.h"

//== ПРОСТРАНСТВА ИМЁН.
namespace Ui {
	class MainWindow;
}

// Для избежания ошибки при доступе к текстовому браузеру из другого потока.
Q_DECLARE_METATYPE(QTextCursor)
Q_DECLARE_METATYPE(QTextBlock)
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
		char chLevel;
		char m_chLogin[MAX_AUTH_LOGIN];
		char m_chPassword[MAX_AUTH_PASSWORD];
		int iConnectionIndex;
	};

public:
	bool bInitOk; ///< Признак успешной инициализации.
	static void* p_vLastReceivedDataBuffer; ///< Указатель на текущий запрошенный принятый пакет.
	static int iLastReceivedDataCode; ///< Код текущего запрошенного принятого пакета.

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
	static void ClientStatusChangedCallback(bool bConnected, unsigned int uiClientIndex);
							///< \param[in] bConnected Статус подключения.
							///< \param[in] uiClientIndex Индекс клиента.
	/// Кэлбэк обработки приходящих пакетов данных.
	static void ClientDataArrivedCallback(unsigned int uiClientIndex);
							///< \param[in] uiClientIndex Индекс клиента.
	/// Кэлбэк обработки приходящих запросов.
	static void ClientRequestArrivedCallback(unsigned int uiClientIndex, char chRequest);

private:
	/// Загрузка конфигурации пользователей.
	static bool LoadUsersConfig();
							///< \return true, при удаче.
	/// Сохранение конфигурации пользователей.
	static bool SaveUsersConfig();
							///< \return true, при удаче.
	/// Процедуры запуска сервера.
	void ServerStartProcedures();
	/// Процедуры остановки сервера.
	void ServerStopProcedures();
	/// Процедуры при логине пользователя.
	static void UserLoginProcedures(QList<AuthorizationUnit>& a_lst_AuthorizationUnits, int iPosition,
									  unsigned int iIndex, ConnectionData& a_ConnectionData);
							///< \param[in] a_lst_AuthorizationUnits Ссыка на список авторизации.
							///< \param[in] iPosition Позиция в списке
							///< \param[in] iIndex Индекс соединения.
							///< \param[in] a_ConnectionData Ссылка на данные по соединению.
	/// Процедуры при логауте пользователя.
	static int UserLogoutProcedures(QList<AuthorizationUnit>& a_lst_AuthorizationUnits, int iPosition,
									   ConnectionData& a_ConnectionData, char chAnswer = AUTH_ANSWER_OK, bool bSend = true);
							///< \param[in] a_lst_AuthorizationUnits Ссыка на список авторизации.
							///< \param[in] iPosition Позиция в списке.
							///< \param[in] a_ConnectionData Ссылка на данные по соединению.
							///< \param[in] chAnswer Ответ пользователю.
							///< \param[in] bSend Отсылать ли отчёт.
							///< \return Новый номер текущего элемента в листе авторизации после замены параметров.
	/// Процедуры при удалении пользователя.
	static int UserPurgeProcedures(QList<AuthorizationUnit>& a_lst_AuthorizationUnits, int iPosition,
										ConnectionData* p_ConnectionData, char chAnswer = AUTH_ANSWER_OK, bool bLogout = true);
							///< \param[in] a_lst_AuthorizationUnits Ссыка на список авторизации.
							///< \param[in] iPosition Позиция в списке.
							///< \param[in] p_ConnectionData Указатель на данные по соединению (не используется если не нужен логаут).
							///< \param[in] chAnswer Ответ пользователю.
							///< \param[in] bLogout Нужны ли процедуры логаута бывшего пользователя.
							///< \return Новый номер текущего элемента в листе авторизации после замены параметров.

private slots:
	/// При нажатии на 'О программе'.
	void on_About_action_triggered();
	/// При завершении ввода строки чата.
	void on_Chat_lineEdit_returnPressed();
	/// При переключении кнопки 'Пуск/Стоп'.
	void on_StartStop_action_triggered(bool checked);
							///< \param[in] checked - Позиция переключателя.
	/// При изменении текста чата.
	void on_Chat_textBrowser_textChanged();
	/// При переключении кнопки 'Автостарт'.
	void on_Autostart_action_triggered(bool checked);
							///< \param[in] checked - Позиция переключателя.

	void on_Users_listWidget_customContextMenuRequested(const QPoint &pos);

private:
	static Ui::MainWindow *p_ui; ///< Указатель на UI.
	static const char* cp_chUISettingsName; ///< Указатель на имя файла с установками UI.
	QSettings* p_UISettings; ///< Указатель на строку установок UI.
	static Server* p_Server; ///< Ссылка на объект сервера.
	static QList<unsigned int> lst_uiConnectedClients; ///< Список присоединённых клиентов.
	static list<XMLNode*> o_lUsers; ///< Главный список разъёмов документа авторизации.
	static QList<AuthorizationUnit> lst_AuthorizationUnits; ///< Список авторизованных пользователей.
	bool bAutostart; ///< Флаг автозапуска.
	LOGDECL
	LOGDECL_PTHRD_INCLASS_ADD
};

#endif // MAINWINDOW_H
