//== ВКЛЮЧЕНИЯ.
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main-hub.h"

//== МАКРОСЫ.
#define LOG_NAME				"Z-Admin"
#define S_CONF_PATH				"./settings/server.ini"
#define S_UI_CONF_PATH			"./settings/ui.ini"

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL_INIT_INCLASS(MainWindow)
LOGDECL_INIT_PTHRD_INCLASS_OWN_ADD(MainWindow)
const char* MainWindow::cp_chUISettingsName = S_UI_CONF_PATH;

//== ФУНКЦИИ КЛАССОВ.
//== Класс главного окна.
// Конструктор.
MainWindow::MainWindow(QWidget* p_parent) :
	QMainWindow(p_parent),
	p_ui(new Ui::MainWindow)
{
	LOG_CTRL_INIT;
	LOG_P_0(LOG_CAT_I, "START.");
	p_UISettings = new QSettings(cp_chUISettingsName, QSettings::IniFormat);
	p_ui->setupUi(this);
	if(IsFileExists((char*)cp_chUISettingsName))
	{
		LOG_P_2(LOG_CAT_I, "Restore UI states.");
		if(!restoreGeometry(p_UISettings->value("Geometry").toByteArray()))
		{
			LOG_P_1(LOG_CAT_E, "Can`t restore Geometry UI state.");
		}
		if(!restoreState(p_UISettings->value("WindowState").toByteArray()))
		{
			LOG_P_1(LOG_CAT_E, "Can`t restore WindowState UI state.");
		}
	}
	else
	{
		LOG_P_0(LOG_CAT_W, "ui.ini is missing and will be created by default at the exit from program.");
	}
}

// Деструктор.
MainWindow::~MainWindow()
{
	LOG_P_0(LOG_CAT_I, "EXIT: " << RETVAL);
	LOGCLOSE;
	delete p_ui;
}

// Процедуры при закрытии окна приложения.
void MainWindow::closeEvent(QCloseEvent *event)
{
	p_UISettings->setValue("Geometry", saveGeometry());
	p_UISettings->setValue("WindowState", saveState());
	QMainWindow::closeEvent(event);
}
