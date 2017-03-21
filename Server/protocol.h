#ifndef PROTOCOL_H
#define PROTOCOL_H

//== ВКЛЮЧЕНИЯ.
#include "proto-util.h"

//== МАКРОСЫ.
#define _NMG					-32768		// !!! Текущий свободный номер _NMG-8 !!!
#define PROTOCOL_CODE			314159265358
// ============================ КОДЫ ПАКЕТОВ ==================================
#define PROTO_O_TEXT_MSG				'T'
#define PROTO_O_AUTHORIZATION_REQUEST	'a'
#define PROTO_O_AUTHORIZATION_ANSWER	'A'
#define PROTO_O_AUTHORIZATION_LOBBY		'l'
//========================== ИСПОЛЬЗУЕМЫЕ МАКРОСЫ =============================
// Авторизация.
#define MAX_AUTH_LOGIN				16
#define MAX_AUTH_PASSWORD			24
#define AUTH_REQUEST_REG			0
#define AUTH_REQUEST_LOGIN			1
#define AUTH_REQUEST_LOGOUT			2
#define AUTH_REQUEST_PURGE			3
#define AUTH_REQUEST_LOBBY			4
#define AUTH_ANSWER_OK				0
#define AUTH_ANSWER_USER_PRESENT	1
#define AUTH_ANSWER_INCORRECT_NAME	2
#define AUTH_ANSWER_LOGIN_FAULT		3
#define AUTH_ANSWER_LOGOFF_FAULT	4
#define AUTH_ANSWER_NOT_LOGGED		5
#define AUTH_ANSWER_WRONG_REQUEST	6
#define AUTH_ANSWER_ALREADY_LOGGED	7
#define AUTH_ANSWER_ACCOUNT_IN_USE	8
#define AUTH_ANSWER_ACCOUNT_ERASED	9
#define AUTH_ANSWER_BAN				10
#define AUTH_ANSWER_LEVEL_CHANGED	11
#define AUTH_ANSWER_DOUBLE_AUTH		12
#define AUTH_ANSWER_LOBBY_CHANGED	13
//========================== ИСПОЛЬЗУЕМЫЕ СТРУКТУРЫ ===========================
/// Структура текстового сообщения.
struct PTextMessage
{
	char m_chLogin[MAX_AUTH_LOGIN]; ///< Буфер ника.
	char m_chMsg[MAX_MSG]; ///< Буфер сообщения.
};
/// Структура запросов авторизации.
struct PAuthorizationData
{
	char chRequestCode; ///< Код запроса.
	char m_chLogin[MAX_AUTH_LOGIN]; ///< Буфер ника.
	char m_chPassword[MAX_AUTH_PASSWORD]; ///< Буфер пароля.
};
/// Структура предоставления списка пользователей.
struct PLobbyAnswer
{
	bool bLastInQueue; ///< Признак последнего пункта в цепочке ответов.
	bool bOnline; ///< Признак присутствия пользователя онлайн.
	char m_chLogin[MAX_AUTH_LOGIN]; ///< Буфер ника.
};
// ====================== ОБЪЯВЛЕНИЯ СТРУКТУР ПАКЕТОВ =========================
ProtocolStorageClassInit
(
	ProtocolStorageDef
	(
		Password, // Имя типа структуры.
		char m_chPassw[MAX_PASSW], // Данные.
		TypeCode(0); // Сопоставленный код пакета.
	);
	ProtocolStorageDef
	(
		TextMsg,
		PTextMessage oTextMessage,
		TypeCode(PROTO_O_TEXT_MSG);
	);
	ProtocolStorageDef
	(
		AuthorizationRequest,
		PAuthorizationData oPAuthorizationData,
		TypeCode(PROTO_O_AUTHORIZATION_REQUEST);
	);
	ProtocolStorageDef
	(
		AuthorizationAnswer,
		char chAnswer,
		TypeCode(PROTO_O_AUTHORIZATION_ANSWER);
	);
	ProtocolStorageDef
	(
		LobbyAnswer,
		PLobbyAnswer oPLobbyAnswer,
		TypeCode(PROTO_O_AUTHORIZATION_LOBBY);
	);
,
// ====================== ЭЛЕМЕНТЫ КОНСТРУКТОРА ПАКЕТОВ =======================
	ProtocolStorageConstructorElement(Password);
	ProtocolStorageConstructorElement(TextMsg);
	ProtocolStorageConstructorElement(AuthorizationRequest);
	ProtocolStorageConstructorElement(AuthorizationAnswer);
	ProtocolStorageConstructorElement(LobbyAnswer);
,
// ====================== ЭЛЕМЕНТЫ ДЕСТРУКТОРА ПАКЕТОВ ========================
	ProtocolStorageDestructorElement(Password);
	ProtocolStorageDestructorElement(TextMsg);
	ProtocolStorageDestructorElement(AuthorizationRequest);
	ProtocolStorageDestructorElement(AuthorizationAnswer);
	ProtocolStorageDestructorElement(LobbyAnswer);
,
// ====================== ЭЛЕМЕНТЫ ДОСТУПА К ПАКЕТАМ ==========================
	// ! Не забываем вносить пары в proto-parser.cpp в раздел // ОБРАБОТКА ПАКЕТОВ. !
	ProtocolStorageAccessElement(PROTO_C_SEND_PASSW, Password);
	ProtocolStorageAccessElement(PROTO_O_TEXT_MSG, TextMsg);
	ProtocolStorageAccessElement(PROTO_O_AUTHORIZATION_REQUEST, AuthorizationRequest);
	ProtocolStorageAccessElement(PROTO_O_AUTHORIZATION_ANSWER, AuthorizationAnswer);
	ProtocolStorageAccessElement(PROTO_O_AUTHORIZATION_LOBBY, LobbyAnswer);
)
#endif // PROTOCOL_H
