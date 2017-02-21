#ifndef PROTOCOL_H
#define PROTOCOL_H

//== МАКРОСЫ.
#define MAX_MSG							512
#define MAX_PASSW						16

// Утиль.
#define PObjNaming(name)				o##name // Создаёт имя объекта структуры добавлением 'o' в начале.
#define PObjDecl(name)					name PObjNaming(name) // Унифицированная декларация объектов.
#define PObjPointerNaming(name)			p_##name // Создаёт имя объекта структуры добавлением 'p_' в начале.
#define PObjPointerDecl(name)			name* PObjPointerNaming(name) // / Унифицированная декларация ук. на объекты.
// Определение структуры члена протокола с именем структуры, определением данных и авто-добавлением объекта с предикатом 'o'.
#define ProtocolStorageDef(name, code)	struct name															\
										{																	\
											code															\
										};																	\
										PObjDecl(name);														\
										PObjPointerDecl(name);
// Определение класса протокола с перечислением структур типа 'ProtocolStorageDef'.
#define ProtocolStorageClassInit(defs)	class ProtocolStorage												\
										{																	\
										public:																\
											defs															\
										}

// ========================= КОДЫ ВЗАИМОДЕЙСТВИЯ ==============================
#define PROTO_C_SEND_PASSW			'H'
#define PROTO_S_PASSW_OK			'P'
#define PROTO_S_PASSW_ERR			'p'
#define PROTO_S_OUT_OF_RANGE		'|'
#define PROTO_S_UNSECURED			'!'
#define PROTO_S_UNKNOWN_COMMAND		'?'
#define PROTO_C_REQUEST_LEAVING		'L'
#define PROTO_S_ACCEPT_LEAVING		'B'
#define PROTO_S_SHUTDOWN_INFO		'Q'
#define PROTO_S_BUFFER_OVERFLOW		'^'
#define PROTO_C_BUFFER_OVERFLOW		'~'
#define PROTO_S_BUFFER_READY		'+'
#define PROTO_C_BUFFER_READY		'*'
// ============================================================================

// ============================ КОДЫ ПАКЕТОВ ==================================
#define PROTO_O_TEXT_MSG			'T'
// ============================================================================

ProtocolStorageClassInit
(
// ====================== ОБЪЯВЛЕНИЯ СТРУКТУР ПАКЕТОВ =========================
	ProtocolStorageDef
	(
		Password, // Имя типа структуры.
		char m_chPassw[MAX_PASSW]; // Данные.
	);
	ProtocolStorageDef
	(
		TextMsg, // Имя типа структуры.
		char m_chMsg[MAX_MSG]; // Данные.
	);
// ============================================================================
);
#endif // PROTOCOL_H
