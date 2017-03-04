#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//== ВКЛЮЧЕНИЯ.
#include <QMainWindow>
#include <QSettings>
#include "Server/server.h"

//== ПРОСТРАНСТВА ИМЁН.
namespace Ui {
	class MainWindow;
}

//== КЛАССЫ.
/// Класс главного окна.
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	int iConnectionIndex; ///< Индекс текущего соединения для работы или RETVAL_ERR при отсутствии выбранного.

public:
	/// Конструктор.
	explicit MainWindow(QWidget* p_parent = 0);
							///< \param[in] p_parent - Указатель на родительский виджет.
	/// Деструктор.
	~MainWindow();
	/// Процедуры при закрытии окна приложения.
	void closeEvent(QCloseEvent* event);
							///< \param[in] event - Указатель на событие.
	/// Процедуры запуска сервера.
	void StartProcedures();
	/// Процедуры остановки сервера.
	void StopProcedures();
	/// Кэлбэк обработки отслеживания статута клиентов.
	static void ClientStatusChangedCallback(bool bConnected, unsigned int uiClientIndex, sockaddr ai_addr,
	#ifndef WIN32
																									socklen_t ai_addrlen);
	#else
																									size_t ai_addrlen);
	#endif
							///< \param[in] bConnected Статус подключения.
							///< \param[in] uiClientIndex Индекс клиента.
							///< \param[in] ai_addr Адрес.
							///< \param[in] ai_addrlen Длина адреса.
private slots:
	/// При нажатии на Выход.
	void on_Exit_action_triggered();
	/// При нажатии на 'О программе'.
	void on_About_action_triggered();
	/// При завершении ввода строки чата.
	void on_Chat_lineEdit_returnPressed();
	// При переключении кнопки 'Пуск/Стоп'.
	void on_StartStop_action_triggered(bool checked);
							///< \param[in] checked - Позиция переключателя.

private:
	static Ui::MainWindow *p_ui; ///< Указатель на UI.
	static const char* cp_chUISettingsName; ///< Указатель на имя файла с установками UI.
	QSettings* p_UISettings; ///< Указатель на строку установок UI.
	Server* p_Server; ///< Ссылка на объект сервера.
	static QList<unsigned int> lst_uiConnectedClients; ///< Список присоединённых клиентов.
	LOGDECL
	LOGDECL_PTHRD_INCLASS_ADD
};

#endif // MAINWINDOW_H
