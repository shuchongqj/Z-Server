//== ВКЛЮЧЕНИЯ.
#include "protoparser.h"
#include "string.h"

//== МАКРОСЫ.
// Утиль.
#define _ParsedPSName(name)				oParsedObject.oProtocolStorage.name
#define _PPControlSize(name)			if((int)sizeof(_ParsedPSName(PObjNaming(name))) < oParsedObject.iDataLength)			\
										{																						\
											oParsedObject.iDataLength = (int)sizeof(_ParsedPSName(PObjNaming(name)));			\
											oParseResult.chRes = PROTOPARSER_OUT_OF_RANGE;										\
											break;																				\
										}																						\
										else oParseResult.chRes = PROTOPARSER_OK;
#define _CopyDataToStructure(name)		_ParsedPSName(PObjNaming(name)) = *(ProtocolStorage::name*)p_chCurrPos
#define ProcessToStorage(name)			_PPControlSize(name);																	\
										if(!bDoNotStore)																		\
										{																						\
											oParseResult.bStored = true;														\
											_CopyDataToStructure(name);															\
										}

//== ФУНКЦИИ КЛАССОВ.
//== Класс парсера протокола.
// Парсинг пакета в соответствующий член хранилища класса парсера.
ProtoParser::ParseResult ProtoParser::ParsePocket(char* p_chData, int iLength, ParsedObject& oParsedObject, bool bDoNotStore)
{
	char* p_chCurrPos;
	ParseResult oParseResult;
	oParseResult.chRes = PROTOPARSER_UNKNOWN_COMMAND;
	oParseResult.bStored = false;
	//
	p_chCurrPos = p_chData;
	oParsedObject.chTypeCode = *p_chCurrPos;
	p_chCurrPos++; // Команда уже обработана.
	oParsedObject.iDataLength = iLength - 1;
	switch(oParsedObject.chTypeCode)
	{
		// ОБРАБОТКА КОМАНД УПРАВЛЕНИЯ.
		case PROTO_C_SEND_PASSW:
		{
			ProcessToStorage(Password);
			break;
		}
		case PROTO_S_PASSW_OK:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		case PROTO_S_PASSW_ERR:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		case PROTO_C_REQUEST_LEAVING:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		case PROTO_S_ACCEPT_LEAVING:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		case PROTO_S_SHUTDOWN_INFO:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		case PROTO_S_BUFFER_OVERFLOW:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		case PROTO_C_BUFFER_OVERFLOW:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		case PROTO_S_BUFFER_READY:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		case PROTO_C_BUFFER_READY:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		// ОБРАБОТКА ПАКЕТОВ.
		// ProcessToStorage(имя структуры в протоколе)
		// - копирует объект в хранилище, проверяя совпадение размера и доверяя любому содержимому (именование по шаблону).
		case PROTO_O_TEXT_MSG:
		{
			ProcessToStorage(TextMsg);
			oParsedObject.oProtocolStorage.oTextMsg.m_chMsg[oParsedObject.iDataLength - 1] = 0; // DEBUG.
			oParsedObject.oProtocolStorage.oTextMsg.m_chMsg[oParsedObject.iDataLength] = 0;
			break;
		}
	}
	return oParseResult;
}
