//== ВКЛЮЧЕНИЯ.
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QScrollBar>
#include "Dialogs/change_level_dialog.h"

//== МАКРОСЫ.
#define LOG_NAME				"Z-Admin"
#define SERVER_NAME				"SERVER"
#define UserNotLoggedInMacro	p_Server->SendToClientImmediately(a_NetHub,										\
									PROTO_O_AUTHORIZATION_ANSWER,												\
									DEF_CHAR_PTH(AUTH_ANSWER_NOT_LOGGED), 1, true, false);						\
								p_Server->FillIPAndPortNames(oConnectionDataInt,								\
								m_chIPNameBuffer, m_chPortNameBuffer, false);									\
								LOG_P_1(LOG_CAT_W, MSG_USER_NOT_LOGGED_IN <<									\
									QString(m_chIPNameBuffer).toStdString() + ":" +								\
									QString(m_chPortNameBuffer).toStdString());
// Сообщения.
#define MSG_USERS_SINC_FAULT	"Users list sinchronization fault."
#define MSG_CLIENTS_SINC_FAULT	"Clients list sinchronization fault."
#define MSG_CANNOT_SAVE_USERS	"Cat`t save users data."
#define MSG_CANNOT_SAVE_BANS	"Cat`t save bans data."
#define MSG_USERS_AUTH_EMPTY	"Users authorization list is empty."
#define MSG_KICKING				"Kicking out: "
#define MSG_LOGGED_OFF_KICKING	"Logged off after kicking: "
#define MSG_USER_NOT_LOGGED_IN	"User is not logged in: "
#define USER_LEVEL_TAG(Level)	QString("[" + QString::number(Level) + "]")
// Тексты меню.
#define MENU_TEXT_USERS_DELETE		"Удалить"
#define MENU_TEXT_BAN_AND_KICK		"Блокировать и отключить"
#define MENU_TEXT_BAN_AND_KICK_IP	"Блокировать вместе с IP и отключить"
#define MENU_TEXT_USERS_PARDON		"Восстановить"
#define MENU_TEXT_USERS_KICK		"Отключить"
#define MENU_TEXT_CHANGE_LEVEL		"Изменить уровень"

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL_INIT_INCLASS(MainWindow)
LOGDECL_INIT_PTHRD_INCLASS_OWN_ADD(MainWindow)
Server* MainWindow::p_Server;
const char* MainWindow::cp_chUISettingsName = S_UI_CONF_PATH;
Ui::MainWindow* MainWindow::p_ui = new Ui::MainWindow;
QList<unsigned int> MainWindow::lst_uiConnectedClients;
list<XMLNode*> MainWindow::o_lUsers;
list<XMLNode*> MainWindow::o_lUserBans;
list<XMLNode*> MainWindow::o_lIPBans;
QList<MainWindow::AuthorizationUnit> MainWindow::lst_AuthorizationUnits;
QList<MainWindow::UserBanUnit> MainWindow::lst_UserBanUnits;
vector<Server::IPBanUnit> MainWindow::vec_IPBanUnits;
QTimer* MainWindow::p_ChatTimer;
char MainWindow::m_chTextChatBuffer[MAX_MSG];
char MainWindow::m_chIPNameBufferUI[INET6_ADDRSTRLEN];
char MainWindow::m_chPortNameBufferUI[PORTSTRLEN];
NetHub MainWindow::oPrimaryNetHub;
bool MainWindow::bAutostart;
QSettings* MainWindow::p_UISettings;

//== ФУНКЦИИ КЛАССОВ.
//== Класс главного окна.
// Конструктор.
MainWindow::MainWindow(QWidget* p_parent) :
	QMainWindow(p_parent)
{
	// Для избежания ошибки при доступе из другого потока.
	qRegisterMetaType<QVector<int>>("QVector<int>");
	//
	LOG_CTRL_INIT;
	LOG_P_0(LOG_CAT_I, "START.");
	iInitRes = RETVAL_OK;
	bAutostart = false;
	p_UISettings = new QSettings(cp_chUISettingsName, QSettings::IniFormat);
	p_ui->setupUi(this);
	if(IsFileExists((char*)cp_chUISettingsName))
	{
		LOG_P_2(LOG_CAT_I, "Restore UI states.");
		if(!restoreGeometry(p_UISettings->value("Geometry").toByteArray()))
			LOG_P_1(LOG_CAT_E, "Can`t restore Geometry UI state.");
		if(!restoreState(p_UISettings->value("WindowState").toByteArray()))
			LOG_P_1(LOG_CAT_E, "Can`t restore WindowState UI state.");
		bAutostart = p_UISettings->value("Autostart").toBool();
	}
	else
		LOG_P_0(LOG_CAT_W, "ui.ini is missing and will be created by default at the exit from program.");
	memset(m_chTextChatBuffer, 0, MAX_MSG);
	p_ChatTimer = new QTimer(this);
	p_ChatTimer->setInterval(250);
	p_ChatTimer->start();
	connect(p_ChatTimer, SIGNAL(timeout()), this, SLOT(slot_UpdateChat()));
	if(!LoadUsersCatalogue())
	{
		iInitRes = RETVAL_ERR;
		LOG_P_0(LOG_CAT_E, "Cat`t load users catalogue.");
		RETVAL_SET(RETVAL_ERR);
		return;
	}
	for(int iP = 0; iP < lst_AuthorizationUnits.length(); iP++)
	{
		p_ui->Users_listWidget->addItem(QString(lst_AuthorizationUnits.at(iP).m_chLogin) +
										USER_LEVEL_TAG(lst_AuthorizationUnits.at(iP).chLevel));
	}
	if(!LoadBansCatalogue())
	{
		iInitRes = RETVAL_ERR;
		LOG_P_0(LOG_CAT_E, "Cat`t load bans catalogue.");
		RETVAL_SET(RETVAL_ERR);
		return;
	}
	for(int iP = 0; iP < lst_UserBanUnits.length(); iP++)
	{
		p_ui->U_Bans_listWidget->addItem(QString(lst_UserBanUnits.at(iP).m_chLogin));
	}
	for(unsigned int uiP = 0; uiP < vec_IPBanUnits.size(); uiP++)
	{
		p_ui->C_Bans_listWidget->addItem(QString(vec_IPBanUnits.at(uiP).m_chIP));
	}
	p_Server = new Server(S_CONF_PATH, LOG_MUTEX, &vec_IPBanUnits);
	p_Server->SetClientStatusChangedCB(ClientStatusChangedCallback);
	p_Server->SetClientDataArrivedCB(ClientDataArrivedCallback);
	p_Server->SetClientRequestArrivedCB(ClientRequestArrivedCallback);
	if(bAutostart)
	{
		LOG_P_0(LOG_CAT_I, "Autostart server.");
		if(ServerStartProcedures()) p_ui->StartStop_action->setChecked(true);
		else p_ui->StartStop_action->setChecked(false);
		p_ui->Autostart_action->toggle();
	}
}

// Деструктор.
MainWindow::~MainWindow()
{
	if(p_Server->CheckReady()) ServerStopProcedures();
	delete p_Server;
	delete p_ChatTimer;
	if(RETVAL == RETVAL_OK)
		LOG_P_0(LOG_CAT_I, "EXIT.")
	else
		LOG_P_0(LOG_CAT_E, "EXIT WITH ERROR: " << RETVAL);
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

// Загрузка каталога банов.
bool MainWindow::LoadBansCatalogue()
{
	XMLError eResult;
	tinyxml2::XMLDocument xmlDocBans;
	UserBanUnit oUserBanUnit;
	Server::IPBanUnit oIPBanUnit;
	QString strHelper;
	//
	eResult = xmlDocBans.LoadFile(S_BANS_CAT_PATH);
	if (eResult != XML_SUCCESS)
	{
		LOG_P_0(LOG_CAT_E, "Can`t open bans coatalogue file:" << S_BANS_CAT_PATH);
		return false;
	}
	else
	{
		LOG_P_1(LOG_CAT_I, "Bans coatalogue has been loaded.");
		if(!FindChildNodes(xmlDocBans.LastChild(), o_lUserBans,
						   "LoginBans", FCN_ONE_LEVEL, FCN_FIRST_ONLY))
		{
			LOG_P_0(LOG_CAT_E, "Bans catalogue file is corrupt. 'LoginBans' node is absend.");
			return false;
		}
		if(!FindChildNodes(xmlDocBans.LastChild(), o_lIPBans,
						   "IPBans", FCN_ONE_LEVEL, FCN_FIRST_ONLY))
		{
			LOG_P_0(LOG_CAT_E, "Bans catalogue file is corrupt. 'IPBans' node is absend.");
			return false;
		}
		PARSE_CHILDLIST(o_lUserBans.front(), p_ListLogins, "Login",
						FCN_ONE_LEVEL, p_NodeLogin)
		{
			strHelper = QString(p_NodeLogin->FirstChild()->Value());
			if(strHelper.isEmpty())
			{
				LOG_P_2(LOG_CAT_I, "Bans catalogue file is corrupt. 'Login' node is empty.");
				return false;
			}
			else
			{
				memcpy(oUserBanUnit.m_chLogin,
					   strHelper.toStdString().c_str(), SizeOfChars(strHelper.toStdString().length() + 1));
			}
			lst_UserBanUnits.append(oUserBanUnit);
		} PARSE_CHILDLIST_END(p_ListLogins);
		PARSE_CHILDLIST(o_lIPBans.front(), p_ListIPs, "IP",
						FCN_ONE_LEVEL, p_NodeIP)
		{
			strHelper = QString(p_NodeIP->FirstChild()->Value());
			if(strHelper.isEmpty())
			{
				LOG_P_2(LOG_CAT_I, "Bans catalogue file is corrupt. 'IP' node is empty.");
				return false;
			}
			else
			{
				memcpy(oIPBanUnit.m_chIP,
					   strHelper.toStdString().c_str(), SizeOfChars(strHelper.toStdString().length() + 1));
			}
			vec_IPBanUnits.push_back(oIPBanUnit);
		} PARSE_CHILDLIST_END(p_ListIPs);
	}
	return true;
}

// Сохранение каталога банов.
bool MainWindow::SaveBansCatalogue()
{
	XMLError eResult;
	tinyxml2::XMLDocument xmlDocBans;
	XMLNode* p_NodeRoot;
	XMLNode* p_NodeLoginBans;
	XMLNode* p_NodeLogin;
	XMLNode* p_NodeIPBans;
	XMLNode* p_NodeIP;
	//
	xmlDocBans.InsertEndChild(xmlDocBans.NewDeclaration());
	p_NodeRoot = xmlDocBans.InsertEndChild(xmlDocBans.NewElement("Root"));
	p_NodeLoginBans = p_NodeRoot->InsertEndChild(xmlDocBans.NewElement("LoginBans"));
	for(int iC=0; iC < lst_UserBanUnits.length(); iC++)
	{
		p_NodeLogin = p_NodeLoginBans->InsertEndChild(xmlDocBans.NewElement("Login"));
		p_NodeLogin->ToElement()->SetText(lst_UserBanUnits.at(iC).m_chLogin);
	}
	p_NodeIPBans = p_NodeRoot->InsertEndChild(xmlDocBans.NewElement("IPBans"));
	for(unsigned int uiC=0; uiC < vec_IPBanUnits.size(); uiC++)
	{
		p_NodeIP = p_NodeIPBans->InsertEndChild(xmlDocBans.NewElement("IP"));
		p_NodeIP->ToElement()->SetText(vec_IPBanUnits.at(uiC).m_chIP);
	}
	eResult = xmlDocBans.SaveFile(S_BANS_CAT_PATH);
	if (eResult != XML_SUCCESS)
	{
		LOG_P_0(LOG_CAT_E, MSG_CANNOT_SAVE_BANS);
		RETVAL_SET(RETVAL_ERR);
		return false;
	}
	else return true;
}

// Загрузка каталога пользователей.
bool MainWindow::LoadUsersCatalogue()
{
	XMLError eResult;
	tinyxml2::XMLDocument xmlDocUsers;
	bool bName = false;
	bool bPassword = false;
	bool bLevel = false;
	AuthorizationUnit oAuthorizationUnitInt;
	QString strHelper;
	//
	eResult = xmlDocUsers.LoadFile(S_USERS_CAT_PATH);
	if (eResult != XML_SUCCESS)
	{
		LOG_P_0(LOG_CAT_E, "Can`t open users catalogue file:" << S_USERS_CAT_PATH);
		return false;
	}
	else
	{
		LOG_P_1(LOG_CAT_I, "Users catalogue has been loaded.");
		if(!FindChildNodes(xmlDocUsers.LastChild(), o_lUsers,
						   "Users", FCN_ONE_LEVEL, FCN_FIRST_ONLY))
		{
			LOG_P_0(LOG_CAT_E, "Users catalogue file is corrupt. 'Users' node is absend.");
			return false;
		}
		PARSE_CHILDLIST(o_lUsers.front(), p_ListUsers, "User",
						FCN_ONE_LEVEL, p_NodeUser)
		{
			FIND_IN_CHILDLIST(p_NodeUser, p_ListLogins,
							  "Login", FCN_ONE_LEVEL, p_NodeLogin)
			{
				strHelper = QString(p_NodeLogin->FirstChild()->Value());
				if(strHelper.isEmpty())
				{
					LOG_P_0(LOG_CAT_E,
							"Users catalogue file is corrupt. 'User' node format incorrect - wrong 'Login' node.");
					return false;
				}
				else
				{
					memcpy(oAuthorizationUnitInt.m_chLogin,
						   strHelper.toStdString().c_str(), SizeOfChars(strHelper.toStdString().length() + 1));
				}
				bName = true;
			} FIND_IN_CHILDLIST_END(p_ListLogins);
			if(!bName)
			{
				LOG_P_0(LOG_CAT_E, "Users catalogue file is corrupt. 'User' node format incorrect - missing 'Login' node.");
				return false;
			}
			FIND_IN_CHILDLIST(p_NodeUser, p_ListPasswords,
							  "Password", FCN_ONE_LEVEL, p_NodePassword)
			{
				strHelper = QString(p_NodePassword->FirstChild()->Value());
				if(strHelper.isEmpty())
				{
					LOG_P_0(LOG_CAT_E,
							"Users catalogue file is corrupt. 'User' node format incorrect - wrong 'Password' node.");
					return false;
				}
				else
				{
					memcpy(oAuthorizationUnitInt.m_chPassword, strHelper.toStdString().c_str(),
						   SizeOfChars(strHelper.toStdString().length() + 1));
				}
				bPassword = true;
			} FIND_IN_CHILDLIST_END(p_ListPasswords);
			if(!bPassword)
			{
				LOG_P_0(LOG_CAT_E,
						"Users catalogue file is corrupt. 'User' node format incorrect - missing 'Password' node.");
				return false;
			}
			FIND_IN_CHILDLIST(p_NodeUser, p_ListLevels,
							  "Level", FCN_ONE_LEVEL, p_NodeLevel)
			{
				strHelper = QString(p_NodeLevel->FirstChild()->Value());
				if(strHelper.isEmpty())
				{
					LOG_P_0(LOG_CAT_E,
							"Users catalogue file is corrupt. 'User' node format incorrect - wrong 'Level' node.");
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
				LOG_P_0(LOG_CAT_E, "Users catalogue file is corrupt. 'User' node format incorrect - missing 'Level' node.");
				return false;
			}
			oAuthorizationUnitInt.iConnectionIndex = CONNECTION_SEL_ERROR;
			lst_AuthorizationUnits.append(oAuthorizationUnitInt);
		} PARSE_CHILDLIST_END(p_ListUsers);
	}
	return true;
}

// Сохранение каталога пользователей.
bool MainWindow::SaveUsersCatalogue()
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
	eResult = xmlDocUsers.SaveFile(S_USERS_CAT_PATH);
	if (eResult != XML_SUCCESS)
	{
		LOG_P_0(LOG_CAT_E, MSG_CANNOT_SAVE_USERS);
		RETVAL_SET(RETVAL_ERR);
		return false;
	}
	else return true;
}

// Процедуры запуска сервера.
bool MainWindow::ServerStartProcedures()
{
	lst_uiConnectedClients.clear();
	p_Server->Start();
	for(unsigned char uchAtt = 0; uchAtt != 64; uchAtt++)
	{
		if(p_Server->CheckReady())
		{
			LOG_P_0(LOG_CAT_I, "Server is on.");
			return true;;
		}
		MSleep(USER_RESPONSE_MS);
	}
	LOG_P_0(LOG_CAT_E, "Can`t start server.");
	RETVAL_SET(RETVAL_ERR);
	return false;
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
	RETVAL_SET(RETVAL_ERR);
}

// Информирование пользователей о изменении лобби.
void MainWindow::LobbyChangedInform(NetHub& a_NetHub, bool bTryLock)
{
	int iConnMem = p_Server->GetCurrentConnection(bTryLock);
	CHAR_PTH;
	//
	for(int iC=0; iC < lst_AuthorizationUnits.length(); iC++)
	{
		if(lst_AuthorizationUnits.at(iC).iConnectionIndex != CONNECTION_SEL_ERROR)
		{
			p_Server->SetCurrentConnection(lst_AuthorizationUnits.at(iC).iConnectionIndex, bTryLock);
			p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
											  DEF_CHAR_PTH(AUTH_ANSWER_LOBBY_CHANGED), 1, true, bTryLock);
		}
	}
	if(iConnMem != CONNECTION_SEL_ERROR) p_Server->SetCurrentConnection(iConnMem, bTryLock);
}

// Процедуры при логине пользователя.
void MainWindow::UserLoginProcedures(NetHub& a_NetHub, int iPosition, unsigned int iIndex,
									 NetHub::ConnectionData& a_ConnectionData, bool bTryLock)
{
	AuthorizationUnit oAuthorizationUnitInt;
	CHAR_PTH;
	QList<QListWidgetItem*> lstItems;
	char m_chIPNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortNameBuffer[PORTSTRLEN];
	//
	memcpy(oAuthorizationUnitInt.m_chLogin, lst_AuthorizationUnits.at(iPosition).m_chLogin, SizeOfChars(MAX_AUTH_LOGIN));
	memcpy(oAuthorizationUnitInt.m_chPassword,
		   lst_AuthorizationUnits.at(iPosition).m_chPassword, SizeOfChars(MAX_AUTH_PASSWORD));
	oAuthorizationUnitInt.chLevel = lst_AuthorizationUnits.at(iPosition).chLevel;
	oAuthorizationUnitInt.iConnectionIndex = iIndex;
	lst_AuthorizationUnits.removeAt(iPosition);
	lst_AuthorizationUnits.append(oAuthorizationUnitInt);
	p_Server->SendToClientImmediately(a_NetHub,
				PROTO_O_AUTHORIZATION_ANSWER, DEF_CHAR_PTH(AUTH_ANSWER_OK), 1, true, bTryLock);
	LOG_P_0(LOG_CAT_I, "User is logged in: " <<
			QString(oAuthorizationUnitInt.m_chLogin).toStdString());
	p_Server->FillIPAndPortNames(a_ConnectionData, m_chIPNameBuffer, m_chPortNameBuffer, false);
	lstItems = p_ui->Clients_listWidget->findItems(QString(m_chIPNameBuffer) + ":" + QString(m_chPortNameBuffer), Qt::MatchExactly);
	if(lstItems.empty() | (lstItems.length() > 1))
	{
		LOG_P_0(LOG_CAT_E, MSG_CLIENTS_SINC_FAULT);
		RETVAL_SET(RETVAL_ERR);
	}
	else
	{
		lstItems.first()->
				setText(QString(QString(m_chIPNameBuffer) + ":" + QString(m_chPortNameBuffer) +
							"[" + QString(oAuthorizationUnitInt.m_chLogin) + "]"));
	}
	LobbyChangedInform(a_NetHub, bTryLock);
}

// Процедуры при логауте пользователя.
int MainWindow::UserLogoutProcedures(NetHub& a_NetHub, int iPosition, NetHub::ConnectionData& a_ConnectionData,
									 char chAnswer, bool bSend, bool bTryLock)
{
	AuthorizationUnit oAuthorizationUnitInt;
	CHAR_PTH;
	QList<QListWidgetItem*> lstItems;
	char m_chIPNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortNameBuffer[PORTSTRLEN];
	//
	if(lst_AuthorizationUnits.empty())
	{
		LOG_P_0(LOG_CAT_E, MSG_USERS_AUTH_EMPTY);
		RETVAL_SET(RETVAL_ERR);
	}
	memcpy(oAuthorizationUnitInt.m_chLogin,
		   lst_AuthorizationUnits.at(iPosition).m_chLogin, SizeOfChars(MAX_AUTH_LOGIN));
	memcpy(oAuthorizationUnitInt.m_chPassword,
		   lst_AuthorizationUnits.at(iPosition).m_chPassword, SizeOfChars(MAX_AUTH_PASSWORD));
	oAuthorizationUnitInt.chLevel = lst_AuthorizationUnits.at(iPosition).chLevel;
	oAuthorizationUnitInt.iConnectionIndex = CONNECTION_SEL_ERROR;
	lst_AuthorizationUnits.removeAt(iPosition);
	lst_AuthorizationUnits.append(oAuthorizationUnitInt);
	LOG_P_0(LOG_CAT_I, "User is logged out: " <<
			QString(oAuthorizationUnitInt.m_chLogin).toStdString());
	if(bSend)
	{
		// Если был запрос на отсыл ответа, значит пользователь был онлайн. Иначе - строка будет стёрта в любом случае.
		p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER, DEF_CHAR_PTH(chAnswer), 1, true, bTryLock);
		// Убираем метку онлайн.
		p_Server->FillIPAndPortNames(a_ConnectionData, m_chIPNameBuffer, m_chPortNameBuffer, bTryLock);
		lstItems = p_ui->Clients_listWidget->findItems(QString(m_chIPNameBuffer) + ":" + QString(m_chPortNameBuffer) +
													   "[" + oAuthorizationUnitInt.m_chLogin + "]", Qt::MatchExactly);
		if(lstItems.empty() | (lstItems.length() > 1))
		{
			LOG_P_0(LOG_CAT_E, MSG_CLIENTS_SINC_FAULT);
			RETVAL_SET(RETVAL_ERR);
		}
		else
		{
			lstItems.first()->
					setText(QString(QString(m_chIPNameBuffer) + ":" + QString(m_chPortNameBuffer)));

		}
	}
	LobbyChangedInform(a_NetHub, bTryLock);
	return lst_AuthorizationUnits.count() - 1;
}

/// Процедуры при удалении пользователя.
int MainWindow::UserPurgeProcedures(NetHub& a_NetHub, int iPosition,
									   NetHub::ConnectionData* p_ConnectionData, char chAnswer, bool bLogout, bool bTryLock)
{
	QList<QListWidgetItem*> lstItems;
	//
	if(lst_AuthorizationUnits.empty())
	{
		LOG_P_0(LOG_CAT_E, MSG_USERS_AUTH_EMPTY);
		RETVAL_SET(RETVAL_ERR);
	}
	if(bLogout)
	{
		iPosition = UserLogoutProcedures(a_NetHub, iPosition, *p_ConnectionData, chAnswer, true, bTryLock);
	}
	lstItems = p_ui->Users_listWidget->
			findItems(QString(lst_AuthorizationUnits.at(iPosition).m_chLogin) +
					  USER_LEVEL_TAG(lst_AuthorizationUnits.at(iPosition).chLevel), Qt::MatchExactly);
	if(lstItems.empty() & (lstItems.length() > 1))
	{
		LOG_P_0(LOG_CAT_E, "Users list sinchronization fault.");
		RETVAL_SET(RETVAL_ERR);
	}
	else
	{
		delete lstItems.first();
	}
	LOG_P_0(LOG_CAT_I, "User is purged: " <<
			QString(lst_AuthorizationUnits.at(iPosition).m_chLogin).toStdString());
	lst_AuthorizationUnits.removeAt(iPosition);
	SaveUsersCatalogue();
	return lst_AuthorizationUnits.count() - 1;
}

// Процедуры при блокировке пользователя.
int MainWindow::UserBanProcedures(int iPosition)
{
	QList<QListWidgetItem*> lstItems;
	UserBanUnit oUserBanUnit;
	//
	if(lst_AuthorizationUnits.empty())
	{
		LOG_P_0(LOG_CAT_E, MSG_USERS_AUTH_EMPTY);
		RETVAL_SET(RETVAL_ERR);
	}
	p_ui->U_Bans_listWidget->addItem(QString(lst_AuthorizationUnits.at(iPosition).m_chLogin));
	lstItems = p_ui->Users_listWidget->
			findItems(QString(lst_AuthorizationUnits.at(iPosition).m_chLogin) +
					  USER_LEVEL_TAG(lst_AuthorizationUnits.at(iPosition).chLevel), Qt::MatchExactly);
	LOG_P_0(LOG_CAT_I, "User is banned: " <<
			QString(lst_AuthorizationUnits.at(iPosition).m_chLogin).toStdString());
	memcpy(oUserBanUnit.m_chLogin, lst_AuthorizationUnits.at(iPosition).m_chLogin, sizeof(UserBanUnit));
	lst_UserBanUnits.append(oUserBanUnit);
	SaveBansCatalogue();
	return lst_UserBanUnits.count() - 1;
}

// Блокировка и отключение по имени адреса.
void MainWindow::BanAndKickByAdressWithMenuProcedures(NetHub& a_NetHub, QString& a_strAddrName)
{
	Server::IPBanUnit oIPBanUnit;
	NetHub::ConnectionData oConnectionDataInt;
	//
	for(int iC = 0; iC < MAX_CONN; iC++)
	{
		oConnectionDataInt = p_Server->GetConnectionData(iC);
		if(oConnectionDataInt.iStatus != CONNECTION_SEL_ERROR)
		{
			p_Server->FillIPAndPortNames(oConnectionDataInt,
										 m_chIPNameBufferUI, m_chPortNameBufferUI);
			if(a_strAddrName.contains(m_chIPNameBufferUI))
			{
				LOG_P_0(LOG_CAT_I, MSG_KICKING << a_strAddrName.toStdString());
				p_Server->KickClient(a_NetHub, iC);
				p_ui->C_Bans_listWidget->addItem(m_chIPNameBufferUI);
				memcpy(oIPBanUnit.m_chIP, m_chIPNameBufferUI, SizeOfChars(INET6_ADDRSTRLEN));
				vec_IPBanUnits.push_back(oIPBanUnit);
				LOG_P_0(LOG_CAT_I, "Client has been banned and kicked out.");
				SaveBansCatalogue();
			}
		}
	}
}

// Кэлбэк обработки отслеживания статута клиентов.
void MainWindow::ClientStatusChangedCallback(NetHub& a_NetHub, bool bConnected, unsigned int uiClientIndex)
{
	char m_chIPNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortNameBuffer[PORTSTRLEN];
	QString strName;
	QList<QListWidgetItem*> lst_MatchItems;
	NetHub::ConnectionData oConnectionDataInt;
	//
	LOG_P_0(LOG_CAT_I, "ID: " << uiClientIndex << " have status: " << bConnected);
	oConnectionDataInt = p_Server->GetConnectionData(uiClientIndex, false);
	if(oConnectionDataInt.iStatus != CONNECTION_SEL_ERROR)
	{
		p_Server->FillIPAndPortNames(oConnectionDataInt, m_chIPNameBuffer, m_chPortNameBuffer, false);
		LOG_P_0(LOG_CAT_I, "IP: " << m_chIPNameBuffer << " Port: " << m_chPortNameBuffer);
		strName = QString::fromStdString(m_chIPNameBuffer) + ":" + QString::fromStdString(m_chPortNameBuffer);
		if(bConnected)
		{
			p_ui->Clients_listWidget->addItem(strName);
			lst_uiConnectedClients.append(uiClientIndex);
		}
		else
		{
			for(int iC=0; iC < lst_AuthorizationUnits.length(); iC++)
			{
				if(lst_AuthorizationUnits.at(iC).iConnectionIndex == (int)uiClientIndex)
				{
					UserLogoutProcedures(a_NetHub, iC, oConnectionDataInt, 0, false, false);
					break;
				}
			}
			lst_MatchItems = p_ui->Clients_listWidget->findItems(strName, Qt::MatchStartsWith);
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
		RETVAL_SET(RETVAL_ERR);
	}
}

// Кэлбэк обработки приходящих пакетов данных.
void MainWindow::ClientDataArrivedCallback(NetHub& a_NetHub, unsigned int uiClientIndex)
{
	NetHub::ConnectionData oConnectionDataInt;
	char m_chIPNameBuffer[INET6_ADDRSTRLEN];
	char m_chPortNameBuffer[PORTSTRLEN];
	PAuthorizationData oPAuthorizationDataInt;
	AuthorizationUnit oAuthorizationUnitInt;
	void* p_vLastReceivedDataBuffer;
	CHAR_PTH;
	PTextMessage oPTextMessage;
	QString strChatMsg;
	int iAuthPos;
	PLobbyAnswer oPLobbyAnswer;
	int iAccessResult;
	//
	if(p_Server->SetCurrentConnection(uiClientIndex, false) == true)
	{
		oConnectionDataInt = p_Server->GetConnectionData(uiClientIndex, false);
		//========  Раздел PROTO_O_TEXT_MSG. ========
		while(true)
		{
			iAccessResult = p_Server->AccessSelectedTypeOfDataS(a_NetHub, &p_vLastReceivedDataBuffer, PROTO_O_TEXT_MSG, false);
			if(iAccessResult != CONNECTION_SEL_ERROR)
			{
				if(iAccessResult == DATA_NOT_FOUND) break;
				// Работа с PROTO_O_TEXT_MSG.
				memcpy(oPTextMessage.m_chLogin, ((PTextMessage*)p_vLastReceivedDataBuffer)->m_chLogin, SizeOfChars(MAX_AUTH_LOGIN));
				memcpy(oPTextMessage.m_chMsg, ((PTextMessage*)p_vLastReceivedDataBuffer)->m_chMsg, SizeOfChars(MAX_MSG));
				for(int iC=0; iC < lst_AuthorizationUnits.length(); iC++)
				{
					if(lst_AuthorizationUnits.at(iC).iConnectionIndex == (int)uiClientIndex)
					{
						strChatMsg = QString(oPTextMessage.m_chLogin) + " => " +
								QString(oPTextMessage.m_chMsg);
						memcpy(m_chTextChatBuffer, strChatMsg.toStdString().c_str(), SizeOfChars(MAX_MSG));
						a_NetHub.AddPocketToOutputBuffer(PROTO_O_TEXT_MSG, (char*)&oPTextMessage, sizeof(PTextMessage));
						for(int iT=0; iT < lst_AuthorizationUnits.length(); iT++)
						{
							if((lst_AuthorizationUnits.at(iT).iConnectionIndex != (int)uiClientIndex) &
							(lst_AuthorizationUnits.at(iT).iConnectionIndex != CONNECTION_SEL_ERROR))
							{
								p_Server->SetCurrentConnection(lst_AuthorizationUnits.at(iT).iConnectionIndex, false);
								p_Server->SendBufferToClient(a_NetHub, false, false);
							}
						}
						a_NetHub.ResetPocketsBufferPositionPointer();
						p_Server->SetCurrentConnection(uiClientIndex, false);
						goto gTEx;
					}
				}
				p_Server->FillIPAndPortNames(oConnectionDataInt, m_chIPNameBuffer, m_chPortNameBuffer, false);
				strChatMsg = QString(m_chIPNameBuffer) + ":" + QString(m_chPortNameBuffer) +
						" => " + QString(oPTextMessage.m_chMsg);
				memcpy(m_chTextChatBuffer, strChatMsg.toStdString().c_str(), SizeOfChars(MAX_MSG));
gTEx:			p_Server->ReleaseDataInPositionS(a_NetHub, iAccessResult, false);
			}
			else
			{
				RETVAL_SET(RETVAL_ERR);
				break;
			}
		}
		//======== Раздел PROTO_O_AUTHORIZATION_REQUEST. ========
		while(true)
		{
			iAccessResult = p_Server->AccessSelectedTypeOfDataS(a_NetHub, &p_vLastReceivedDataBuffer,
																PROTO_O_AUTHORIZATION_REQUEST, false);
			if(iAccessResult != CONNECTION_SEL_ERROR)
			{
				if(iAccessResult == DATA_NOT_FOUND) break;
				// Работа с PROTO_O_AUTHORIZATION_REQUEST.
				oPAuthorizationDataInt = *((PAuthorizationData*)p_vLastReceivedDataBuffer);
				switch (oPAuthorizationDataInt.chRequestCode)
				{
					case AUTH_REQUEST_REG:
					{
						if(QString(oPAuthorizationDataInt.m_chLogin) == QString(SERVER_NAME))
						{
							p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
															  DEF_CHAR_PTH(AUTH_ANSWER_INCORRECT_NAME), 1, true, false);
							p_Server->FillIPAndPortNames(oConnectionDataInt,
														 m_chIPNameBuffer, m_chPortNameBuffer, false);
							LOG_P_0(LOG_CAT_W, "Client tries to register as server: " <<
									QString(m_chIPNameBuffer).toStdString() + ":" +
									QString(m_chPortNameBuffer).toStdString());
							goto gLEx;
						}
						else
						{
							if(QString(oPAuthorizationDataInt.m_chLogin).isEmpty())
							{
								p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
																  DEF_CHAR_PTH(AUTH_ANSWER_INCORRECT_NAME), 1, true, false);
								goto gLEx;
							}
							for(int iN = 0; iN < lst_UserBanUnits.length(); iN++)
							{
								if(QString(oPAuthorizationDataInt.m_chLogin) == QString(lst_UserBanUnits.at(iN).m_chLogin))
								{
									p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
																	  DEF_CHAR_PTH(AUTH_ANSWER_BAN), 1, true, false);
									p_Server->FillIPAndPortNames(oConnectionDataInt,
																 m_chIPNameBuffer, m_chPortNameBuffer, false);
									LOG_P_0(LOG_CAT_W, "Client tries to register with banned login: " <<
											QString(m_chIPNameBuffer).toStdString() + ":" +
											QString(m_chPortNameBuffer).toStdString());
									goto gLEx;
								}
							}
						}
						for(int iC=0; iC < lst_AuthorizationUnits.length(); iC++)
						{
							if(QString(lst_AuthorizationUnits.at(iC).m_chLogin)
							   == QString(oPAuthorizationDataInt.m_chLogin))
							{

								p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
													 DEF_CHAR_PTH(AUTH_ANSWER_USER_PRESENT), 1, true, false);
								LOG_P_1(LOG_CAT_W, "User`s login is already present: " <<
										QString(oPAuthorizationDataInt.m_chLogin).toStdString());
								goto gLEx;
							}
						}
						oAuthorizationUnitInt.chLevel = 0;
						memcpy(oAuthorizationUnitInt.m_chLogin, oPAuthorizationDataInt.m_chLogin, SizeOfChars(MAX_AUTH_LOGIN));
						memcpy(oAuthorizationUnitInt.m_chPassword, oPAuthorizationDataInt.m_chPassword,
							   SizeOfChars(MAX_AUTH_PASSWORD));
						oAuthorizationUnitInt.iConnectionIndex = CONNECTION_SEL_ERROR;
						lst_AuthorizationUnits.append(oAuthorizationUnitInt);
						p_ui->Users_listWidget->addItem(QString(oAuthorizationUnitInt.m_chLogin) +
														USER_LEVEL_TAG(oAuthorizationUnitInt.chLevel));
						p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
														  DEF_CHAR_PTH(AUTH_ANSWER_OK), 1, true, false);
						LOG_P_0(LOG_CAT_I, "User has been registered successfully: " <<
								QString(oPAuthorizationDataInt.m_chLogin).toStdString());
						if(!SaveUsersCatalogue())
						{
							LOG_P_0(LOG_CAT_E, MSG_CANNOT_SAVE_USERS);
							RETVAL_SET(RETVAL_ERR);
						}
						LobbyChangedInform(a_NetHub, false);
						break;
					}
					case AUTH_REQUEST_LOGIN:
					{
						iAuthPos = CONNECTION_SEL_ERROR;
						for(int iC = 0; iC < lst_AuthorizationUnits.length(); iC++)
						{
							if(lst_AuthorizationUnits.at(iC).iConnectionIndex == (int)uiClientIndex)
							{
								p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
																  DEF_CHAR_PTH(AUTH_ANSWER_DOUBLE_AUTH), 1, true, false);
								p_Server->FillIPAndPortNames(oConnectionDataInt,
															 m_chIPNameBuffer, m_chPortNameBuffer, false);
								LOG_P_0(LOG_CAT_W, "Client tries to perform a double login: " <<
										QString(m_chIPNameBuffer).toStdString() + ":" +
										QString(m_chPortNameBuffer).toStdString());
								goto gLEx;
							}
							if(QString(lst_AuthorizationUnits.at(iC).m_chLogin) ==
							   QString(oPAuthorizationDataInt.m_chLogin))
							{
								if(QString(lst_AuthorizationUnits.at(iC).m_chPassword) ==
								   QString(oPAuthorizationDataInt.m_chPassword))
								{
									if(lst_AuthorizationUnits.at(iC).iConnectionIndex == CONNECTION_SEL_ERROR)
									{
										for(int iN = 0; iN < lst_UserBanUnits.length(); iN++)
										{
											if(QString(oPAuthorizationDataInt.m_chLogin) ==
											   QString(lst_UserBanUnits.at(iN).m_chLogin))
											{
												p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
																				  DEF_CHAR_PTH(AUTH_ANSWER_BAN), 1, true, false);
												p_Server->FillIPAndPortNames(oConnectionDataInt,
																			 m_chIPNameBuffer, m_chPortNameBuffer, false);
												LOG_P_0(LOG_CAT_W, "Client tries to login with ban: " <<
														QString(m_chIPNameBuffer).toStdString() + ":" +
														QString(m_chPortNameBuffer).toStdString());
												goto gLEx;
											}
										}
										iAuthPos = iC;
										continue;
									}
									else
									{
										p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
															 DEF_CHAR_PTH(AUTH_ANSWER_ALREADY_LOGGED), 1, true, false);
										LOG_P_1(LOG_CAT_W, "User is already logged in: " <<
												QString(oPAuthorizationDataInt.m_chLogin).toStdString());
										goto gLEx;
									}
								}
								p_Server->SendToClientImmediately(a_NetHub, PROTO_O_AUTHORIZATION_ANSWER,
													 DEF_CHAR_PTH(AUTH_ANSWER_LOGIN_FAULT), 1, true, false);
								LOG_P_1(LOG_CAT_W, "Wrong password for user: " <<
										QString(oPAuthorizationDataInt.m_chLogin).toStdString());
								goto gLEx;
							}
						}
						if(iAuthPos != CONNECTION_SEL_ERROR)
						{
							UserLoginProcedures(a_NetHub, iAuthPos, uiClientIndex, oConnectionDataInt, false);
							goto gLEx;
						}
						p_Server->SendToClientImmediately(a_NetHub,
														  PROTO_O_AUTHORIZATION_ANSWER,
														  DEF_CHAR_PTH(AUTH_ANSWER_LOGIN_FAULT), 1, true, false);
						LOG_P_1(LOG_CAT_W, "Requested login is not present: " <<
								QString(oPAuthorizationDataInt.m_chLogin).toStdString());
						break;
					}
					case AUTH_REQUEST_LOGOUT:
					{
						for(int iC=0; iC < lst_AuthorizationUnits.length(); iC++)
						{
							if(lst_AuthorizationUnits.at(iC).iConnectionIndex != CONNECTION_SEL_ERROR)
							{
								if(lst_AuthorizationUnits.at(iC).iConnectionIndex == (int)uiClientIndex)
								{
									UserLogoutProcedures(a_NetHub, iC, oConnectionDataInt, AUTH_ANSWER_OK, true, false);
									goto gLEx;
								}
							}
						}
						UserNotLoggedInMacro;
						break;
					}
					case AUTH_REQUEST_PURGE:
					{
						for(int iC=0; iC < lst_AuthorizationUnits.length(); iC++)
						{
							if(lst_AuthorizationUnits.at(iC).iConnectionIndex != CONNECTION_SEL_ERROR)
							{
								if(lst_AuthorizationUnits.at(iC).iConnectionIndex == (int)uiClientIndex)
								{
									UserPurgeProcedures(a_NetHub, iC, &oConnectionDataInt, 0, true, false);
									goto gLEx;
								}
							}
						}
						UserNotLoggedInMacro;
						break;
					}
					case AUTH_REQUEST_LOBBY:
					{
						for(int iC = 0; iC < lst_AuthorizationUnits.length(); iC++)
						{
							if(lst_AuthorizationUnits.at(iC).iConnectionIndex != CONNECTION_SEL_ERROR)
							{
								if(lst_AuthorizationUnits.at(iC).iConnectionIndex == (int)uiClientIndex)
								{
									for(int iA = 0; iA < lst_AuthorizationUnits.length(); iA++)
									{
										if(lst_AuthorizationUnits.at(iA).iConnectionIndex == CONNECTION_SEL_ERROR)
											oPLobbyAnswer.bOnline = false;
										else oPLobbyAnswer.bOnline = true;
										if(iA != (lst_AuthorizationUnits.length() - 1))
											oPLobbyAnswer.bLastInQueue = false;
										else oPLobbyAnswer.bLastInQueue = true;
										memcpy(oPLobbyAnswer.m_chLogin,
											   lst_AuthorizationUnits.at(iA).m_chLogin, SizeOfChars(MAX_AUTH_LOGIN));
										a_NetHub.AddPocketToOutputBuffer(PROTO_O_AUTHORIZATION_LOBBY,
																   (char*)&oPLobbyAnswer, sizeof(PLobbyAnswer));
									}
									p_Server->SendBufferToClient(a_NetHub, true, false);
									goto gLEx;
								}
							}
						}
						UserNotLoggedInMacro;
						break;
					}
					default:
					{
						p_Server->SendToClientImmediately(a_NetHub,
														  PROTO_O_AUTHORIZATION_ANSWER,
														  DEF_CHAR_PTH(AUTH_ANSWER_WRONG_REQUEST), 1, true, false);
						LOG_P_0(LOG_CAT_E, "Wrong authorization request.");
						break;
					}
				}
gLEx:			p_Server->ReleaseDataInPositionS(a_NetHub, iAccessResult, false);
			}
			else
			{
				RETVAL_SET(RETVAL_ERR);
				break;
			}
		}
	}
}

// Кэлбэк обработки приходящих запросов.
void MainWindow::ClientRequestArrivedCallback(NetHub& a_NetHub, unsigned int uiClientIndex, char chRequest)
{
	a_NetHub = a_NetHub;
	LOG_P_2(LOG_CAT_I, "Client: " << uiClientIndex << " request: " << chRequest);
}

// Обновление чата.
void MainWindow::slot_UpdateChat()
{
	if(m_chTextChatBuffer[0] != 0)
	{
		p_ui->Chat_textBrowser->append(QString(m_chTextChatBuffer));
		m_chTextChatBuffer[0] = 0;
	}
}

// При нажатии на 'О программе'.
void MainWindow::on_About_action_triggered()
{

}

// При завершении ввода строки чата.
void MainWindow::on_Chat_lineEdit_returnPressed()
{
	QString strChatMsg;
	PTextMessage oPTextMessage;
	//
	if(p_Server->CheckReady())
	{
		if(lst_uiConnectedClients.isEmpty())
		{
			memcpy(m_chTextChatBuffer, "[Ошибка]: нет клиентов.", SizeOfChars(MAX_MSG));
		}
		else
		{
			memcpy(oPTextMessage.m_chLogin, SERVER_NAME, SizeOfChars(MAX_AUTH_LOGIN));
			memcpy(oPTextMessage.m_chMsg, (char*)p_ui->Chat_lineEdit->text().toStdString().c_str(), SizeOfChars(MAX_MSG));
			oPrimaryNetHub.AddPocketToOutputBuffer(PROTO_O_TEXT_MSG, (char*)&oPTextMessage, sizeof(PTextMessage));
			for(int iNum = 0; iNum != lst_uiConnectedClients.count(); iNum++)
			{
				if(p_Server->SetCurrentConnection(lst_uiConnectedClients.at(iNum)))
				{
					p_Server->SendBufferToClient(oPrimaryNetHub, false);
				}
			}
			oPrimaryNetHub.ResetPocketsBufferPositionPointer();
			strChatMsg = QString(SERVER_NAME) + " => " + p_ui->Chat_lineEdit->text();
			memcpy(m_chTextChatBuffer, strChatMsg.toStdString().c_str(), SizeOfChars(MAX_MSG));
		}
	}
	else
	{
		memcpy(m_chTextChatBuffer, "[Ошибка]: сервер выключен.", SizeOfChars(MAX_MSG));
	}
	p_ui->Chat_lineEdit->clear();
}

// При переключении кнопки 'Пуск/Стоп'.
void MainWindow::on_StartStop_action_triggered(bool checked)
{
	if(checked)
	{
		if(ServerStartProcedures()) p_ui->StartStop_action->setChecked(true);
		else p_ui->StartStop_action->setChecked(false);
	}
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

// При переключении кнопки 'Автостарт'.
void MainWindow::on_Autostart_action_triggered(bool checked)
{
	bAutostart = checked;
	p_UISettings->setValue("Autostart", bAutostart);
}

// При нажатии ПКМ на элементе списка пользователей.
void MainWindow::on_Users_listWidget_customContextMenuRequested(const QPoint &pos)
{
	QPoint oGlobalPos;
	QMenu oMenu;
	QAction* p_SelectedMenuItem;
	QListWidgetItem* p_ListWidgetItem;
	NetHub::ConnectionData oConnectionDataInt;
	ChangeLevelDialog* p_ChangeLevelDialog;
	QRect rctGeom;
	int iLevel;
	int iConnectionIndexInt;
	QString strIPName;
	AuthorizationUnit oAuthorizationUnitInt;
	CHAR_PTH;
	//
	p_ListWidgetItem = p_ui->Users_listWidget->itemAt(pos);
	if(p_ListWidgetItem != 0)
	{
		oGlobalPos = p_ui->Users_listWidget->mapToGlobal(pos);
		oGlobalPos.setX(oGlobalPos.x() + 5);
		oMenu.addAction(MENU_TEXT_USERS_DELETE);
		oMenu.addAction(MENU_TEXT_BAN_AND_KICK);
		oMenu.addAction(MENU_TEXT_BAN_AND_KICK_IP);
		oMenu.addAction(MENU_TEXT_CHANGE_LEVEL);
		p_SelectedMenuItem = oMenu.exec(oGlobalPos);
		if(p_SelectedMenuItem != 0)
		{
			if(p_SelectedMenuItem->text() == MENU_TEXT_USERS_DELETE)
			{
				for(int iC = 0; iC < lst_AuthorizationUnits.count(); iC++)
				{
					if(QString(lst_AuthorizationUnits.at(iC).m_chLogin) + USER_LEVEL_TAG(lst_AuthorizationUnits.at(iC).chLevel) ==
					   p_ListWidgetItem->text())
					{
						if(lst_AuthorizationUnits.at(iC).iConnectionIndex != CONNECTION_SEL_ERROR)
						{
							iConnectionIndexInt = lst_AuthorizationUnits.at(iC).iConnectionIndex;
							oConnectionDataInt = p_Server->GetConnectionData(iConnectionIndexInt);
							LOG_P_2(LOG_CAT_I, "Erased online.");
							p_Server->SetCurrentConnection(iConnectionIndexInt);
							UserPurgeProcedures(oPrimaryNetHub, iC, &oConnectionDataInt, AUTH_ANSWER_ACCOUNT_ERASED);
						}
						else
						{
							LOG_P_2(LOG_CAT_I, "Erased offline.");
							UserPurgeProcedures(oPrimaryNetHub, iC, 0, 0, false);
						}
						return;
					}
				}
			}
			else if((p_SelectedMenuItem->text() == MENU_TEXT_BAN_AND_KICK) | (p_SelectedMenuItem->text() == MENU_TEXT_BAN_AND_KICK_IP))
			{
				for(int iC = 0; iC < lst_AuthorizationUnits.count(); iC++)
				{
					if(QString(lst_AuthorizationUnits.at(iC).m_chLogin) + USER_LEVEL_TAG(lst_AuthorizationUnits.at(iC).chLevel) ==
					   p_ListWidgetItem->text())
					{
						if(lst_AuthorizationUnits.at(iC).iConnectionIndex != CONNECTION_SEL_ERROR)
						{
							iConnectionIndexInt = lst_AuthorizationUnits.at(iC).iConnectionIndex;
							//
							oConnectionDataInt = p_Server->GetConnectionData(iConnectionIndexInt);
							p_Server->SetCurrentConnection(iConnectionIndexInt);
							UserBanProcedures(iC);
							LOG_P_0(LOG_CAT_I, "Banned online.");
							p_Server->FillIPAndPortNames(oConnectionDataInt,
														 m_chIPNameBufferUI, m_chPortNameBufferUI);
							strIPName = QString(m_chIPNameBufferUI);
							if(p_SelectedMenuItem->text() == MENU_TEXT_BAN_AND_KICK_IP)
							{
								BanAndKickByAdressWithMenuProcedures(oPrimaryNetHub, strIPName);
							}
							else p_Server->KickClient(oPrimaryNetHub, iConnectionIndexInt);
						}
						else
						{
							UserBanProcedures(iC);
							LOG_P_0(LOG_CAT_I, "Banned offline.");
						}
						return;
					}
				}
			}
			else if (p_SelectedMenuItem->text() == MENU_TEXT_CHANGE_LEVEL)
			{
				for(int iC = 0; iC < lst_AuthorizationUnits.count(); iC++)
				{
					if(QString(lst_AuthorizationUnits.at(iC).m_chLogin) + USER_LEVEL_TAG(lst_AuthorizationUnits.at(iC).chLevel) ==
					   p_ListWidgetItem->text())
					{
						p_ChangeLevelDialog = new ChangeLevelDialog();
						rctGeom = p_ChangeLevelDialog->geometry();
						rctGeom.setX(oGlobalPos.x());
						rctGeom.setY(oGlobalPos.y());
						p_ChangeLevelDialog->setGeometry(rctGeom);
						p_ChangeLevelDialog->setWindowFlags(Qt::CustomizeWindowHint|Qt::WindowCloseButtonHint);
						p_ChangeLevelDialog->SetLevel((int)(lst_AuthorizationUnits.at(iC).chLevel));
						iLevel = p_ChangeLevelDialog->exec();
						if(iLevel != DIALOGS_REJECT)
						{
							LOG_P_0(LOG_CAT_I, "User [" << lst_AuthorizationUnits.at(iC).m_chLogin
									<< "] have got level [" << QString::number(iLevel).toStdString() << "]");
							oAuthorizationUnitInt = lst_AuthorizationUnits.takeAt(iC);
							oAuthorizationUnitInt.chLevel = iLevel;
							lst_AuthorizationUnits.append(oAuthorizationUnitInt);
							p_ListWidgetItem->setText(oAuthorizationUnitInt.m_chLogin + USER_LEVEL_TAG(oAuthorizationUnitInt.chLevel));
							SaveUsersCatalogue();
							if(oAuthorizationUnitInt.iConnectionIndex != CONNECTION_SEL_ERROR)
							{
								p_Server->SetCurrentConnection(oAuthorizationUnitInt.iConnectionIndex);
								p_Server->SendToClientImmediately(oPrimaryNetHub,
															PROTO_O_AUTHORIZATION_ANSWER,
																  DEF_CHAR_PTH(AUTH_ANSWER_LEVEL_CHANGED), 1);
							}
							return;
						}
					}
				}
			}
		}
	}
}

// При нажатии ПКМ на элементе списка банов по пользователям.
void MainWindow::on_U_Bans_listWidget_customContextMenuRequested(const QPoint &pos)
{
	QPoint oGlobalPos;
	QMenu oMenu;
	QAction* p_SelectedMenuItem;
	QListWidgetItem* p_ListWidgetItem;
	QString strMem;
	//
	p_ListWidgetItem = p_ui->U_Bans_listWidget->itemAt(pos);
	if(p_ListWidgetItem != 0)
	{
		oGlobalPos = p_ui->U_Bans_listWidget->mapToGlobal(pos);
		oGlobalPos.setX(oGlobalPos.x() + 5);
		oMenu.addAction(MENU_TEXT_USERS_PARDON);
		p_SelectedMenuItem = oMenu.exec(oGlobalPos);
		if(p_SelectedMenuItem != 0)
		{
			if(p_SelectedMenuItem->text() == MENU_TEXT_USERS_PARDON)
			{
				bool bFound = false;
				strMem = p_ListWidgetItem->text();
				for(int iC = 0; iC < lst_UserBanUnits.count(); iC++)
				{
					if(QString(lst_UserBanUnits.at(iC).m_chLogin) == p_ListWidgetItem->text())
					{
						lst_UserBanUnits.removeAt(iC);
						bFound = true;
						break;
					}
				}
				if(bFound == false)
				{
					LOG_P_0(LOG_CAT_E, "Users ban list sinchronization fault.");
					RETVAL_SET(RETVAL_ERR);
				}
				delete p_ListWidgetItem;
				LOG_P_0(LOG_CAT_I, "Pardon complete: " << strMem.toStdString());
				SaveBansCatalogue();
			}
		}
	}
}

// При нажатии ПКМ на элементе списка соединений.
void MainWindow::on_Clients_listWidget_customContextMenuRequested(const QPoint &pos)
{
	QPoint oGlobalPos;
	QMenu oMenu;
	QAction* p_SelectedMenuItem;
	QListWidgetItem* p_ListWidgetItem;
	NetHub::ConnectionData oConnectionDataInt;
	QString strIPName;
	//
	p_ListWidgetItem = p_ui->Clients_listWidget->itemAt(pos);
	if(p_ListWidgetItem != 0)
	{
		oGlobalPos = p_ui->Clients_listWidget->mapToGlobal(pos);
		oGlobalPos.setX(oGlobalPos.x() + 5);
		oMenu.addAction(MENU_TEXT_USERS_KICK);
		oMenu.addAction(MENU_TEXT_BAN_AND_KICK);
		p_SelectedMenuItem = oMenu.exec(oGlobalPos);
		if(p_SelectedMenuItem != 0)
		{
			if(p_SelectedMenuItem->text() == MENU_TEXT_USERS_KICK)
			{
				for(int iC = 0; iC < MAX_CONN; iC++)
				{
					oConnectionDataInt = p_Server->GetConnectionData(iC);
					if(oConnectionDataInt.iStatus != CONNECTION_SEL_ERROR)
					{
						p_Server->FillIPAndPortNames(oConnectionDataInt,
													 m_chIPNameBufferUI, m_chPortNameBufferUI);
						if(p_ListWidgetItem->text().contains(QString(m_chIPNameBufferUI) + QString(":") + QString(m_chPortNameBufferUI)))
						{
							LOG_P_0(LOG_CAT_I, MSG_KICKING << p_ListWidgetItem->text().toStdString());
							p_Server->KickClient(oPrimaryNetHub, iC);
							return;
						}
					}
				}
			}
			else if (p_SelectedMenuItem->text() == MENU_TEXT_BAN_AND_KICK)

			{
				strIPName = p_ListWidgetItem->text();
				BanAndKickByAdressWithMenuProcedures(oPrimaryNetHub, strIPName);
			}
		}
	}
}

// При нажатии ПКМ на элементе списка банов по адресам.
void MainWindow::on_C_Bans_listWidget_customContextMenuRequested(const QPoint &pos)
{
	QPoint oGlobalPos;
	QMenu oMenu;
	QAction* p_SelectedMenuItem;
	QListWidgetItem* p_ListWidgetItem;
	QString strMem;
	//
	p_ListWidgetItem = p_ui->C_Bans_listWidget->itemAt(pos);
	if(p_ListWidgetItem != 0)
	{
		oGlobalPos = p_ui->C_Bans_listWidget->mapToGlobal(pos);
		oGlobalPos.setX(oGlobalPos.x() + 5);
		oMenu.addAction(MENU_TEXT_USERS_PARDON);
		p_SelectedMenuItem = oMenu.exec(oGlobalPos);
		if(p_SelectedMenuItem != 0)
		{
			if(p_SelectedMenuItem->text() == MENU_TEXT_USERS_PARDON)
			{
				bool bFound = false;
				strMem = p_ListWidgetItem->text();
				//
				for(int iC = 0; iC < (int)vec_IPBanUnits.size(); iC++)
				{
					if(QString(vec_IPBanUnits.at(iC).m_chIP) == p_ListWidgetItem->text())
					{
						vec_IPBanUnits.erase(vec_IPBanUnits.begin() + iC);
						bFound = true;
						break;
					}
				}
				if(bFound == false)
				{
					LOG_P_0(LOG_CAT_E, "Clients ban list sinchronization fault.");
					RETVAL_SET(RETVAL_ERR);
				}
				delete p_ListWidgetItem;
				LOG_P_0(LOG_CAT_I, "Pardon complete: " << strMem.toStdString());
				SaveBansCatalogue();
			}
		}
	}
}
