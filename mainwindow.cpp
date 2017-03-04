//== ВКЛЮЧЕНИЯ.
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main-hub.h"

//== МАКРОСЫ.
#define LOG_NAME				"Z-Admin"

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL_INIT_INCLASS(MainWindow)
LOGDECL_INIT_PTHRD_INCLASS_OWN_ADD(MainWindow)
const char* MainWindow::cp_chUISettingsName = S_UI_CONF_PATH;
Ui::MainWindow* MainWindow::p_ui = new Ui::MainWindow;
QList<unsigned int> MainWindow::lst_uiConnectedClients;

//== ФУНКЦИИ КЛАССОВ.
//== Класс главного окна.
// Конструктор.
MainWindow::MainWindow(QWidget* p_parent) :
	QMainWindow(p_parent)
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
	iConnectionIndex = RETVAL_ERR;
	p_Server = new Server(S_CONF_PATH, LOG_MUTEX);
	p_Server->SetClientStatusChangedCB(ClientStatusChangedCallback);
}

// Деструктор.
MainWindow::~MainWindow()
{
	if(p_Server->CheckReady()) StopProcedures();
	delete p_Server;
	if(RETVAL == RETVAL_OK)
	{
		LOG_P_0(LOG_CAT_I, "EXIT.");
	}
	else
	{
		LOG_P_0(LOG_CAT_E, "EXIT WITH ERROR: " << RETVAL);
	}
	LOG_CLOSE;
	delete p_ui;
}

// Процедуры при закрытии окна приложения.
void MainWindow::closeEvent(QCloseEvent *event)
{
	p_UISettings->setValue("Geometry", saveGeometry());
	p_UISettings->setValue("WindowState", saveState());
	QMainWindow::closeEvent(event);
}

// Процедуры запуска сервера.
void MainWindow::StartProcedures()
{
	lst_uiConnectedClients.clear();
	p_Server->Start();
	for(unsigned char uchAtt = 0; uchAtt != 64; uchAtt++)
	{
		if(p_Server->CheckReady())
		{
			LOG_P_0(LOG_CAT_I, "Server is on.");
			return;
		}
		MSleep(USER_RESPONSE_MS);
	}
	LOG_P_0(LOG_CAT_E, "Can`t start server.");
	p_ui->StartStop_action->setChecked(false);
}

// Процедуры остановки сервера.
void MainWindow::StopProcedures()
{
	p_Server->Stop();
	for(unsigned char uchAtt = 0; uchAtt != 128; uchAtt++)
	{
		if(!p_Server->CheckReady())
		{
			LOG_P_0(LOG_CAT_I, "Server is off.");
			return;
		}
		MSleep(USER_RESPONSE_MS);
	}
	LOG_P_0(LOG_CAT_E, "Can`t stop server.");
	p_ui->StartStop_action->setChecked(true);
}

// Кэлбэк обработки отслеживания статута клиентов.
void MainWindow::ClientStatusChangedCallback(bool bConnected, unsigned int uiClientIndex, sockaddr ai_addr,
#ifndef WIN32
																								socklen_t ai_addrlen)
#else
																								size_t ai_addrlen)
#endif
{
	char m_chNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortBuffer[6];
	QString strName;
	QList<QListWidgetItem*> lst_MatchItems;
	//
	LOG_P_0(LOG_CAT_I, "ID: " << uiClientIndex << " have status: " << bConnected);
#ifndef WIN32
	getnameinfo(&ai_addr, ai_addrlen, m_chNameBuffer, sizeof(m_chNameBuffer),
				m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#else
	getnameinfo(&ai_addr, (socklen_t)ai_addrlen,
				m_chNameBuffer, sizeof(m_chNameBuffer), m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#endif
	LOG_P_0(LOG_CAT_I, "IP: " << m_chNameBuffer << " Port: " << m_chPortBuffer);
	strName = QString(m_chNameBuffer) + ":" + QString(m_chPortBuffer) + "=" + QString::number(uiClientIndex);
	if(bConnected)
	{
		p_ui->Clients_listWidget->addItem(strName);
		lst_uiConnectedClients.append(uiClientIndex);
	}
	else
	{
		lst_MatchItems = p_ui->Clients_listWidget->findItems(strName, Qt::MatchExactly);
		for(int iNum = 0; iNum != lst_MatchItems.count(); iNum++)
		{
			delete lst_MatchItems.at(iNum);
			lst_uiConnectedClients.removeAt(lst_uiConnectedClients.indexOf(uiClientIndex));
		}
	}
}

// При нажатии на 'Выход'.
void MainWindow::on_Exit_action_triggered()
{
	QApplication::quit();
}

// При нажатии на 'О программе'.
void MainWindow::on_About_action_triggered()
{

}

// При завершении ввода строки чата.
void MainWindow::on_Chat_lineEdit_returnPressed()
{
	if(p_Server->CheckReady())
	{
		if(lst_uiConnectedClients.isEmpty())
		{
			p_ui->Chat_textBrowser->insertPlainText("[Ошибка]: нет клиентов.\n");
		}
		else
		{
			for(int iNum = 0; iNum != lst_uiConnectedClients.count(); iNum++)
			{
				if(p_Server->SetCurrentConnection(lst_uiConnectedClients.at(iNum)))
				{
				   p_Server->SendToUser(PROTO_O_TEXT_MSG, (char*)p_ui->Chat_lineEdit->text().toStdString().c_str(),
										(int)p_ui->Chat_lineEdit->text().toStdString().length());
				}
			}
			p_ui->Chat_textBrowser->insertPlainText("Сервер: " + p_ui->Chat_lineEdit->text() + "\n");
		}
	}
	else
	{
		p_ui->Chat_textBrowser->insertPlainText("[Ошибка]: сервер выключен.\n");
	}
	p_ui->Chat_lineEdit->clear();
}

// При переключении кнопки 'Пуск/Стоп'.
void MainWindow::on_StartStop_action_triggered(bool checked)
{
	if(checked) StartProcedures();
	else StopProcedures();
}
