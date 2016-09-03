#ifndef PROTOCOL_H
#define PROTOCOL_H

//== МАКРОСЫ.
#define MAX_MSG							512

// Утиль.
#define PObjNaming(name)				o##name // Создаёт имя объекта структуры добавлением 'o' в начале.
#define PObjDecl(name)					name PObjNaming(name) // Унифицированная декларация объектов.
#define ProtocolStorageDef(name, code)	struct name															\
										{																	\
											code															\
										};																	\
										PObjDecl(name)
#define ProtocolClassInit(defs)			class ProtocolStorage												\
										{																	\
										public:																\
											defs															\
										}

// ========================= КОДЫ ВЗАИМОДЕЙСТВИЯ ==============================
#define PROTO_C_SEND_PASSW			'a'
#define PROTO_S_PASSW_OK			'b'
#define PROTO_S_PASSW_ERR			'c'
// ============================================================================

// ============================ КОДЫ ПАКЕТОВ ==================================
#define PROTO_O_TEXT_MSG			't'
// ============================================================================

ProtocolClassInit(

// ====================== ОБЪЯВЛЕНИЯ СТРУКТУР ПАКЕТОВ =========================
	ProtocolStorageDef
	(
		TextMsg,
		char m_chMsg[MAX_MSG];
	);
// ============================================================================

);
#endif // PROTOCOL_H
