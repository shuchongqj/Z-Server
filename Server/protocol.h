#ifndef PROTOCOL_H
#define PROTOCOL_H

//== ВКЛЮЧЕНИЯ.
#include "proto-util.h"

//== МАКРОСЫ.
// ============================ КОДЫ ПАКЕТОВ ==================================
#define PROTO_O_TEXT_MSG			'T'
// ============================================================================
ProtocolStorageClassInit
(
// ====================== ОБЪЯВЛЕНИЯ СТРУКТУР ПАКЕТОВ =========================
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
// ============================================================================
,
// ====================== ЭЛЕМЕНТЫ КОНСТРУКТОРА ПАКЕТОВ =======================
	ProtocolStorageConstructorElement(Password);
	ProtocolStorageConstructorElement(TextMsg);
,
// ====================== ЭЛЕМЕНТЫ ДЕСТРУКТОРА ПАКЕТОВ ========================
	ProtocolStorageDestructorElement(Password);
	ProtocolStorageDestructorElement(TextMsg);
,
// ====================== ЭЛЕМЕНТЫ ДОСТУПА К ПАКЕТАМ ==========================
	ProtocolStorageAccessElement(PROTO_C_SEND_PASSW, Password);
	ProtocolStorageAccessElement(PROTO_O_TEXT_MSG, TextMsg);
)
#endif // PROTOCOL_H
