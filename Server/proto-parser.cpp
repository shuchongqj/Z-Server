//== ВКЛЮЧЕНИЯ.
#include "proto-parser.h"
#include "string.h"

//== МАКРОСЫ.
// Утиль.
#define _PPControlSize(name)			if((int)sizeof(ProtocolStorage::name) < iCurrentLength)									\
										{																						\
											iCurrentLength = (int)sizeof(ProtocolStorage::name);								\
											bOutOfRange = true;																	\
										} // Если зашкалило, то длина ставится по нормальной длине первого элемента.
// _FillNewStructure(имя структуры в протоколе)
// - копирует данные в новую структуру, проверяя совпадение размера и доверяя любому содержимому (именование по шаблону).
#define _FillNewStructure(name)			_PPControlSize(name);																	\
										if(!bDoNotStore)																		\
										{																						\
											oParseResult.bStored = true;														\
											aProtocolStorage._PObjPointerNaming(name) = new(ProtocolStorage::name);				\
											*(aProtocolStorage._PObjPointerNaming(name)) =										\
												*(ProtocolStorage::name*)p_chCurrPos;											\
										}
#define _CaseCommand(typecode)			case typecode: oParseResult.iRes = PROTOPARSER_OK; break
//
#define CaseCommandHub					_CaseCommand(PROTO_S_PASSW_OK); _CaseCommand(PROTO_S_PASSW_ERR);						\
										_CaseCommand(PROTO_C_REQUEST_LEAVING); _CaseCommand(PROTO_S_ACCEPT_LEAVING);			\
										_CaseCommand(PROTO_S_SHUTDOWN_INFO); _CaseCommand(PROTO_S_BUFFER_FULL);					\
										_CaseCommand(PROTO_C_BUFFER_FULL); _CaseCommand(PROTO_A_BUFFER_READY);					\
										_CaseCommand(PROTO_S_UNSECURED); _CaseCommand(PROTO_S_BAN); _CaseCommand(PROTO_S_KICK)
#define CasePocket(typecode, name)		case typecode: oParseResult.iRes = PROTOPARSER_OK; _FillNewStructure(name); break

//== ФУНКЦИИ КЛАССОВ.
//== Класс парсера протокола.
// Парсинг пакета в соответствующий член хранилища класса парсера.
ProtoParser::ParseResult ProtoParser::ParsePocket(char* p_chData, int iLength,
												  ProtocolStorage& aProtocolStorage, bool bDoNotStore)
{
	char* p_chCurrPos;
	ParseResult oParseResult;
	oParseResult.iRes = PROTOPARSER_UNKNOWN_COMMAND;
	oParseResult.bStored = false;
	bDoNotStore = bDoNotStore;
	int iCurrentLength;
	bool bOutOfRange;
	//
	bOutOfRange = false;
	p_chCurrPos = p_chData;
	oParseResult.chTypeCode = *p_chCurrPos;
	p_chCurrPos++; // Команда уже обработана.
	iCurrentLength = iLength - 1; // Длина всего остального, кроме первого кода.
	switch(oParseResult.chTypeCode)
	{
		CaseCommandHub;
		// ОБРАБОТКА ПАКЕТОВ.
		CasePocket(PROTO_C_SEND_PASSW, Password);
		CasePocket(PROTO_O_TEXT_MSG, TextMsg);
		CasePocket(PROTO_O_AUTHORIZATION_REQUEST, AuthorizationRequest);
		CasePocket(PROTO_O_AUTHORIZATION_ANSWER, AuthorizationAnswer);
	}
	if(oParseResult.bStored == true) aProtocolStorage.chTypeCode = oParseResult.chTypeCode;
	// Если зашкалило...
	if(bOutOfRange == true)
	{
		oParseResult.p_chExtraData = p_chData + iCurrentLength;
		oParseResult.iExtraDataLength = iLength - iCurrentLength;
	}
	else
	{
		oParseResult.p_chExtraData = 0;
		oParseResult.iExtraDataLength = 0;
	}
	return oParseResult;
}
