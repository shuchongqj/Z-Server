#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//== ВКЛЮЧЕНИЯ.
#include <QMainWindow>
#include <QSettings>
#include <QTextCursor> // Для избежания ошибки при доступе к текстовому браузеру из другого потока.
#include "Server/server.h"

//== ПРОСТРАНСТВА ИМЁН.
namespace Ui {
	class MainWindow;
}

Q_DECLARE_METATYPE(QTextCursor) // Для избежания ошибки при доступе к текстовому браузеру из другого потока.

//== КЛАССЫ.
/// Класс главного окна.
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	static int iConnectionIndex; ///< Индекс текущего соединения для работы или RETVAL_ERR при отсутствии выбранного.
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

private:
	/// Процедуры запуска сервера.
	void StartProcedures();
	/// Процедуры остановки сервера.
	void StopProcedures();

private slots:
	/// При нажатии на 'О программе'.
	void on_About_action_triggered();
	/// При завершении ввода строки чата.
	void on_Chat_lineEdit_returnPressed();
	// При переключении кнопки 'Пуск/Стоп'.
	void on_StartStop_action_triggered(bool checked);
							///< \param[in] checked - Позиция переключателя.
	// При изменении текста чата.
	void on_Chat_textBrowser_textChanged();

private:
	static Ui::MainWindow *p_ui; ///< Указатель на UI.
	static const char* cp_chUISettingsName; ///< Указатель на имя файла с установками UI.
	QSettings* p_UISettings; ///< Указатель на строку установок UI.
	static Server* p_Server; ///< Ссылка на объект сервера.
	static QList<unsigned int> lst_uiConnectedClients; ///< Список присоединённых клиентов.
	LOGDECL
	LOGDECL_PTHRD_INCLASS_ADD
};

#endif // MAINWINDOW_H
