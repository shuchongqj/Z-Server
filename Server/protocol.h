#ifndef PROTOCOL_H
#define PROTOCOL_H

//== ВКЛЮЧЕНИЯ.
#include "proto-util.h"

//== МАКРОСЫ.
#define _NMG					-32768		// !!! Текущий свободный номер _NMG-7 !!!
// ============================ КОДЫ ПАКЕТОВ ==================================
#define PROTO_O_TEXT_MSG			'T'
#define PROTO_O_AUTHORITY_REQUEST	'a'
#define PROTO_O_AUTHORITY_ANSWER	'A'
//========================== ИСПОЛЬЗУЕМЫЕ МАКРОСЫ =============================
// Авторизация.
#define MAX_AUTH_LOGIN					16
#define MAX_AUTH_PASSWORD				24
#define MAX_AUTH_REQUEST_REG			0
#define MAX_AUTH_REQUEST_LOGIN			1
#define MAX_AUTH_REQUEST_LOGOUT			2
#define MAX_AUTH_REQUEST_PURGE			3
#define MAX_AUTH_ANSWER_OK				0
#define MAX_AUTH_ANSWER_USER_PRESENT	1
#define MAX_AUTH_ANSWER_LOGIN_FAULT		2
//========================== ИСПОЛЬЗУЕМЫЕ СТРУКТУРЫ ===========================
/// Структура запросов авторизации.
struct PAuthorizationData
{
	char chRequestCode;
	char m_chLogin[MAX_AUTH_LOGIN];
	char m_chPassword[MAX_AUTH_LOGIN];
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
		char m_chMsg[MAX_MSG],
		TypeCode(PROTO_O_TEXT_MSG);
	);
	ProtocolStorageDef
	(
		AuthorizationRequest,
		PAuthorizationData oPAuthorizationData,
		TypeCode(PROTO_O_AUTHORITY_REQUEST);
	);
	ProtocolStorageDef
	(
		AuthorizationAnswer,
		char chAnswer,
		TypeCode(PROTO_O_AUTHORITY_ANSWER);
	);
,
// ====================== ЭЛЕМЕНТЫ КОНСТРУКТОРА ПАКЕТОВ =======================
	ProtocolStorageConstructorElement(Password);
	ProtocolStorageConstructorElement(TextMsg);
	ProtocolStorageConstructorElement(AuthorizationRequest);
	ProtocolStorageConstructorElement(AuthorizationAnswer);
,
// ====================== ЭЛЕМЕНТЫ ДЕСТРУКТОРА ПАКЕТОВ ========================
	ProtocolStorageDestructorElement(Password);
	ProtocolStorageDestructorElement(TextMsg);
	ProtocolStorageDestructorElement(AuthorizationRequest);
	ProtocolStorageDestructorElement(AuthorizationAnswer);
,
// ====================== ЭЛЕМЕНТЫ ДОСТУПА К ПАКЕТАМ ==========================
	// ! Не забываем вносить пары в proto-parser.cpp в раздел // ОБРАБОТКА ПАКЕТОВ. !
	ProtocolStorageAccessElement(PROTO_C_SEND_PASSW, Password);
	ProtocolStorageAccessElement(PROTO_O_TEXT_MSG, TextMsg);
	ProtocolStorageAccessElement(PROTO_O_AUTHORITY_REQUEST, AuthorizationRequest);
	ProtocolStorageAccessElement(PROTO_O_AUTHORITY_ANSWER, AuthorizationAnswer);
)
#endif // PROTOCOL_H
