//== ВКЛЮЧЕНИЯ.
#include "proto-parser.h"
#include "string.h"

//== МАКРОСЫ.
// Утиль.
#define _PPControlSize(name)			if((int)sizeof(ProtocolStorage::name) < oParseResult.iDataLength)						\
										{																						\
											oParseResult.iDataLength = (int)sizeof(ProtocolStorage::name);						\
											oParseResult.chRes = PROTOPARSER_OUT_OF_RANGE;										\
											break;																				\
										}																						\
										else oParseResult.chRes = PROTOPARSER_OK;
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
#define _CaseCommand(typecode)			case typecode: oParseResult.chRes = PROTOPARSER_OK; break
//
#define CaseCommandHub					_CaseCommand(PROTO_S_PASSW_OK); _CaseCommand(PROTO_S_PASSW_ERR);						\
										_CaseCommand(PROTO_C_REQUEST_LEAVING); _CaseCommand(PROTO_S_ACCEPT_LEAVING);			\
										_CaseCommand(PROTO_S_SHUTDOWN_INFO); _CaseCommand(PROTO_S_BUFFER_FULL);					\
										_CaseCommand(PROTO_C_BUFFER_FULL); _CaseCommand(PROTO_A_BUFFER_READY);					\
										_CaseCommand(PROTO_S_UNSECURED);
#define CasePocket(typecode, name)		case typecode: _FillNewStructure(name); oParseResult.chRes = PROTOPARSER_OK; break

//== ФУНКЦИИ КЛАССОВ.
//== Класс парсера протокола.
// Парсинг пакета в соответствующий член хранилища класса парсера.
ProtoParser::ParseResult ProtoParser::ParsePocket(char* p_chData, int iLength,
												  ProtocolStorage& aProtocolStorage, bool bDoNotStore)
{
	char* p_chCurrPos;
	ParseResult oParseResult;
	oParseResult.chRes = PROTOPARSER_UNKNOWN_COMMAND;
	oParseResult.bStored = false;
	bDoNotStore = bDoNotStore;
	//
	p_chCurrPos = p_chData;
	oParseResult.chTypeCode = *p_chCurrPos;
	p_chCurrPos++; // Команда уже обработана.
	oParseResult.iDataLength = iLength - 1;
	switch(oParseResult.chTypeCode)
	{
		CaseCommandHub;
		// ОБРАБОТКА ПАКЕТОВ.
		CasePocket(PROTO_C_SEND_PASSW, Password);
		CasePocket(PROTO_O_TEXT_MSG, TextMsg);
	}
	if(oParseResult.bStored == true) aProtocolStorage.chTypeCode = oParseResult.chTypeCode;
	return oParseResult;
}
