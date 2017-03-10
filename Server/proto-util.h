#ifndef PROTO_UTIL_H
#define PROTO_UTIL_H

//== МАКРОСЫ.

#define MAX_MSG							128
#define MAX_PASSW						16
// Утиль.
#define _PObjPointerNaming(name)		p_##name // Создаёт имя указателя добавлением 'p_' в начале.
#define _PObjPointerDecl(name)			name* _PObjPointerNaming(name) // Унифицированная декларация ук. на объекты.
#define TypeCode(typecode)				char chType = typecode
// Определение структуры члена протокола с именем структуры, определением данных и авто-добавлением указателя с предикатом 'p_'.
#define ProtocolStorageDef(name, code, typecode)	struct name	{code; typecode}; _PObjPointerDecl(name)
// Определение класса протокола с перечислением структур типа 'ProtocolStorageDef'.
#define ProtocolStorageClassInit(defs, constructor, destructor, access)	class ProtocolStorage						\
										{																			\
										public:																		\
											defs																	\
											char chTypeCode;														\
											void CleanPointers() {destructor};										\
											ProtocolStorage() {constructor};										\
											~ProtocolStorage() {CleanPointers();}									\
											void* GetPointer() {access; return 0;}									\
										};
#define ProtocolStorageConstructorElement(name)	_PObjPointerNaming(name) = 0
#define ProtocolStorageDestructorElement(name)	if(_PObjPointerNaming(name) != 0)									\
										{																			\
											delete _PObjPointerNaming(name);										\
											_PObjPointerNaming(name) = 0;											\
										}
#define ProtocolStorageAccessElement(typecode, name)	if(chTypeCode == typecode) return (void*)_PObjPointerNaming(name)
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
#define PROTO_S_BUFFER_FULL			'^'
#define PROTO_C_BUFFER_FULL			'~'
#define PROTO_A_BUFFER_READY		'+'

#endif // PROTO_UTIL_H
