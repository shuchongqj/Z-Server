//== ВКЛЮЧЕНИЯ.
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "main-hub.h"
#include <QScrollBar>

//== МАКРОСЫ.
#define LOG_NAME				"Z-Admin"

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL_INIT_INCLASS(MainWindow)
LOGDECL_INIT_PTHRD_INCLASS_OWN_ADD(MainWindow)
Server* MainWindow::p_Server;
const char* MainWindow::cp_chUISettingsName = S_UI_CONF_PATH;
Ui::MainWindow* MainWindow::p_ui = new Ui::MainWindow;
QList<unsigned int> MainWindow::lst_uiConnectedClients;
int MainWindow::iConnectionIndex = CONNECTION_SEL_ERROR;
void* MainWindow::p_vLastReceivedDataBuffer = 0;
int MainWindow::iLastReceivedDataCode = DATA_ACCESS_ERROR;
QList<MainWindow::AuthorizationUnit> MainWindow::lst_AuthorizationUnits;
list<XMLNode*> MainWindow::o_lUsers;

//== ФУНКЦИИ КЛАССОВ.
//== Класс главного окна.
// Конструктор.
MainWindow::MainWindow(QWidget* p_parent) :
	QMainWindow(p_parent)
{
	//
	LOG_CTRL_INIT;
	LOG_P_0(LOG_CAT_I, "START.");
	bInitOk = true;
	bAutostart = false;
	qRegisterMetaType<QTextCursor>("QTextCursor"); // Для избежания ошибки при доступе к текстовому браузеру из другого потока.
    qRegisterMetaType<QTextBlock>("QTextBlock"); // Для избежания ошибки при доступе к текстовому браузеру из другого потока.
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
		bAutostart = p_UISettings->value("Autostart").toBool();
	}
	else
	{
		LOG_P_0(LOG_CAT_W, "ui.ini is missing and will be created by default at the exit from program.");
	}
	iConnectionIndex = RETVAL_ERR;
	p_Server = new Server(S_CONF_PATH, LOG_MUTEX);
	p_Server->SetClientStatusChangedCB(ClientStatusChangedCallback);
	p_Server->SetClientDataArrivedCB(ClientDataArrivedCallback);
	p_Server->SetClientRequestArrivedCB(ClientRequestArrivedCallback);
	bInitOk = LoadUsersConfig();
	if(!bInitOk)
	{
		LOG_P_0(LOG_CAT_E, "Cat`t load users data.");
		return;
	}
	for(int iP = 0; iP < lst_AuthorizationUnits.length(); iP++)
	{
		p_ui->Users_listWidget->addItem(QString(lst_AuthorizationUnits.at(iP).m_chLogin) + "[offline]");
	}
	if(bAutostart)
	{
		LOG_P_0(LOG_CAT_I, "Autostart server.");
		ServerStartProcedures();
		p_ui->StartStop_action->toggle();
	}
}

// Деструктор.
MainWindow::~MainWindow()
{
	if(p_Server->CheckReady()) ServerStopProcedures();
	delete p_Server;
	if(!SaveUsersConfig())
	{
		LOG_P_0(LOG_CAT_E, "Cat`t unload users data.");
		RETVAL_SET(RETVAL_ERR);
	}
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
	p_UISettings->setValue("Autostart", bAutostart);
	QMainWindow::closeEvent(event);
}

// Загрузка конфигурации пользователей.
bool MainWindow::LoadUsersConfig()
{
	XMLError eResult;
	tinyxml2::XMLDocument xmlDocUsers;
	//
	eResult = xmlDocUsers.LoadFile(S_USERS_CONF_PATH);
	if (eResult != XML_SUCCESS)
	{
		LOG_P_0(LOG_CAT_E, "Can`t open users configuration file:" << S_USERS_CONF_PATH);
		return false;
	}
	else
	{
		LOG_P_1(LOG_CAT_I, "Users configuration loaded.");
		if(!FindChildNodes(xmlDocUsers.LastChild(), o_lUsers,
						   "Users", FCN_ONE_LEVEL, FCN_FIRST_ONLY))
		{
			return true;
		}
		PARSE_CHILDLIST(o_lUsers.front(), p_ListUsers, "User",
						FCN_ONE_LEVEL, p_NodeUser)
		{
			bool bName = false;
			bool bPassword = false;
			bool bLevel = false;
			AuthorizationUnit oAuthorizationUnitInt;
			QString strHelper;
			//
			FIND_IN_CHILDLIST(p_NodeUser, p_ListNames,
							  "Login", FCN_ONE_LEVEL, p_NodeName)
			{
				strHelper = QString(p_NodeName->FirstChild()->Value());
				if(strHelper.isEmpty())
				{
					LOG_P_0(LOG_CAT_E, "Users configuration file is corrupt! 'User' node format incorrect - wrong 'Login' node.");
					return false;
				}
				else
				{
					memcpy(oAuthorizationUnitInt.m_chLogin, strHelper.toStdString().c_str(), strHelper.toStdString().length() + 1);
				}
				bName = true;
			} FIND_IN_CHILDLIST_END(p_ListNames);
			if(!bName)
			{
				LOG_P_0(LOG_CAT_E, "Users configuration file is corrupt! 'User' node format incorrect - missing 'Login' node.");
				return false;
			}
			FIND_IN_CHILDLIST(p_NodeUser, p_ListPasswords,
							  "Password", FCN_ONE_LEVEL, p_NodePassword)
			{
				strHelper = QString(p_NodePassword->FirstChild()->Value());
				if(strHelper.isEmpty())
				{
					LOG_P_0(LOG_CAT_E, "Users configuration file is corrupt! 'User' node format incorrect - wrong 'Password' node.");
					return false;
				}
				else
				{
					memcpy(oAuthorizationUnitInt.m_chPassword, strHelper.toStdString().c_str(), strHelper.toStdString().length() + 1);
				}
				bPassword = true;
			} FIND_IN_CHILDLIST_END(p_ListPasswords);
			if(!bPassword)
			{
				LOG_P_0(LOG_CAT_E, "Users configuration file is corrupt! 'User' node format incorrect - missing 'Password' node.");
				return false;
			}
			FIND_IN_CHILDLIST(p_NodeUser, p_ListLevels,
							  "Level", FCN_ONE_LEVEL, p_NodeLevel)
			{
				strHelper = QString(p_NodeLevel->FirstChild()->Value());
				if(strHelper.isEmpty())
				{
					LOG_P_0(LOG_CAT_E, "Users configuration file is corrupt! 'User' node format incorrect - wrong 'Level' node.");
					return false;
				}
				else
				{
					oAuthorizationUnitInt.chLevel = strHelper.toInt();
				}
				bLevel = true;
			} FIND_IN_CHILDLIST_END(p_ListLevels);
			if(!bLevel)
			{
				LOG_P_0(LOG_CAT_E, "Users configuration file is corrupt! 'User' node format incorrect - missing 'Level' node.");
				return false;
			}
			lst_AuthorizationUnits.append(oAuthorizationUnitInt);
		} PARSE_CHILDLIST_END(p_ListUsers);
	}
	return true;
}

// Сохранение конфигурации пользователей.
bool MainWindow::SaveUsersConfig()
{
	XMLError eResult;
	tinyxml2::XMLDocument xmlDocUsers;
	XMLNode* p_NodeRoot;
	XMLNode* p_NodeUsers;
	XMLNode* p_NodeUser;
	XMLNode* p_NodeInfoH;
	//
	xmlDocUsers.InsertEndChild(xmlDocUsers.NewDeclaration());
	p_NodeRoot = xmlDocUsers.InsertEndChild(xmlDocUsers.NewElement("Root"));
	p_NodeUsers = p_NodeRoot->InsertEndChild(xmlDocUsers.NewElement("Users"));
	for(int iC=0; iC < lst_AuthorizationUnits.length(); iC++)
	{
		p_NodeUser = p_NodeUsers->InsertEndChild(xmlDocUsers.NewElement("User"));
		p_NodeInfoH = p_NodeUser->InsertEndChild(xmlDocUsers.NewElement("Login"));
		p_NodeInfoH->ToElement()->SetText(lst_AuthorizationUnits.at(iC).m_chLogin);
		p_NodeInfoH = p_NodeUser->InsertEndChild(xmlDocUsers.NewElement("Password"));
		p_NodeInfoH->ToElement()->SetText(lst_AuthorizationUnits.at(iC).m_chPassword);
		p_NodeInfoH = p_NodeUser->InsertEndChild(xmlDocUsers.NewElement("Level"));
		p_NodeInfoH->ToElement()->SetText(QString::number(lst_AuthorizationUnits.at(iC).chLevel).toStdString().c_str());
	}
	eResult = xmlDocUsers.SaveFile(S_USERS_CONF_PATH);
	if (eResult != XML_SUCCESS)
		return false;
	else return true;
}

// Процедуры запуска сервера.
void MainWindow::ServerStartProcedures()
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
}

// Процедуры остановки сервера.
void MainWindow::ServerStopProcedures()
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
}

// Кэлбэк обработки отслеживания статута клиентов.
void MainWindow::ClientStatusChangedCallback(bool bConnected, unsigned int uiClientIndex)
{
	char m_chNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortBuffer[6];
	QString strName;
	QList<QListWidgetItem*> lst_MatchItems;
	ConnectionData oConnectionData;
	//
	LOG_P_0(LOG_CAT_I, "ID: " << uiClientIndex << " have status: " << bConnected);
	oConnectionData = p_Server->GetConnectionData(uiClientIndex);
	if(oConnectionData.iStatus != CONNECTION_SEL_ERROR)
	{
#ifndef WIN32
		getnameinfo(&oConnectionData.ai_addr, oConnectionData.ai_addrlen, m_chNameBuffer, sizeof(m_chNameBuffer),
					m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#else
		getnameinfo(&oConnectionData.ai_addr, (socklen_t)oConnectionData.ai_addrlen,
					m_chNameBuffer, sizeof(m_chNameBuffer), m_chPortBuffer, sizeof(m_chPortBuffer), NI_NUMERICHOST);
#endif
		LOG_P_0(LOG_CAT_I, "IP: " << m_chNameBuffer << " Port: " << m_chPortBuffer);
		strName = QString::fromStdString(m_chNameBuffer) + ":" + QString::fromStdString(m_chPortBuffer) +
				"[" + QString::number(uiClientIndex) + "]";
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
	else
	{
		LOG_P_0(LOG_CAT_E, "Can`t get connection data from server storage.");
	}
}

// Кэлбэк обработки приходящих пакетов данных.
void MainWindow::ClientDataArrivedCallback(unsigned int uiClientIndex)
{
	ConnectionData oConnectionDataInt;
	char m_chIPNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortNameBuffer[PORTSTRLEN];
	PAuthorizationData oPAuthorizationDataInt;
	//
	if(p_Server->SetCurrentConnection(uiClientIndex) == true)
	{
		iLastReceivedDataCode = p_Server->AccessCurrentData(&p_vLastReceivedDataBuffer);
		if((iLastReceivedDataCode != DATA_ACCESS_ERROR) & (iLastReceivedDataCode != CONNECTION_SEL_ERROR) )
		{
			switch (iLastReceivedDataCode)
			{
				case PROTO_O_TEXT_MSG:
				{
					oConnectionDataInt = p_Server->GetConnectionData(uiClientIndex);
					if(oConnectionDataInt.iStatus != CONNECTION_SEL_ERROR)
					{
						p_Server->FillIPAndPortNames(oConnectionDataInt, m_chIPNameBuffer, m_chPortNameBuffer);
						p_ui->Chat_textBrowser->insertPlainText(QString(m_chIPNameBuffer) + ":" + QString(m_chPortNameBuffer) +
									" => " + QString((char*)p_vLastReceivedDataBuffer) + "\n");
						p_Server->ReleaseCurrentData();
					}
					break;
				}
				case PROTO_O_AUTHORITY_REQUEST:
				{
					// DEBUG.
					oConnectionDataInt = p_Server->GetConnectionData(uiClientIndex);
					if(oConnectionDataInt.iStatus != CONNECTION_SEL_ERROR)
					{
						oPAuthorizationDataInt = *((PAuthorizationData*)p_vLastReceivedDataBuffer);
						p_Server->FillIPAndPortNames(oConnectionDataInt, m_chIPNameBuffer, m_chPortNameBuffer);
						p_ui->Chat_textBrowser->insertPlainText(QString(m_chIPNameBuffer) + ":" + QString(m_chPortNameBuffer) +
									" #> Request: " + QString::number(oPAuthorizationDataInt.chRequestCode) + ", Login: " +
									QString(oPAuthorizationDataInt.m_chLogin) + ", Password: " +
									QString(oPAuthorizationDataInt.m_chPassword) + "\n");
						p_Server->ReleaseCurrentData();
					}
					//
					break;
				}
			}
		}
	}
}

// Кэлбэк обработки приходящих запросов.
void MainWindow::ClientRequestArrivedCallback(unsigned int uiClientIndex, char chRequest)
{
	LOG_P_2(LOG_CAT_I, "Client: " << uiClientIndex << " request: " << chRequest);
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
										(int)p_ui->Chat_lineEdit->text().toStdString().length() + 1);
				}
			}
			p_ui->Chat_textBrowser->insertPlainText("Server => ");
			p_ui->Chat_textBrowser->insertPlainText(p_ui->Chat_lineEdit->text());
			p_ui->Chat_textBrowser->insertPlainText("\n");
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
	if(checked) ServerStartProcedures();
	else ServerStopProcedures();
}

// При изменении текста чата.
void MainWindow::on_Chat_textBrowser_textChanged()
{
	QScrollBar* p_ScrollBar;
	//
	p_ScrollBar = p_ui->Chat_textBrowser->verticalScrollBar();
	p_ScrollBar->setValue(p_ScrollBar->maximum());
}
