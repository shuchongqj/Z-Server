//== ВКЛЮЧЕНИЯ.
#include "protoparser.h"
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
#define FillNewStructure(name)			_PPControlSize(name);																	\
										if(!bDoNotStore)																		\
										{																						\
											oParseResult.bStored = true;														\
											aProtocolStorage._PObjPointerNaming(name) = new(ProtocolStorage::name);				\
											*(aProtocolStorage._PObjPointerNaming(name)) =										\
												*(ProtocolStorage::name*)p_chCurrPos;											\
										}

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
		// ОБРАБОТКА КОМАНД УПРАВЛЕНИЯ.
		case PROTO_C_SEND_PASSW:
		{
			FillNewStructure(Password);
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
		case PROTO_A_BUFFER_READY:
		{
			oParseResult.chRes = PROTOPARSER_OK;
			break;
		}
		// ОБРАБОТКА ПАКЕТОВ.
		// FillNewStructure(имя структуры в протоколе)
		// - копирует данные в новую структуру, проверяя совпадение размера и доверяя любому содержимому (именование по шаблону).
		case PROTO_O_TEXT_MSG:
		{
			FillNewStructure(TextMsg);
			if(aProtocolStorage.p_TextMsg != 0)
			{
				aProtocolStorage.p_TextMsg->m_chMsg[oParseResult.iDataLength - 1] = 0; // DEBUG.
				aProtocolStorage.p_TextMsg->m_chMsg[oParseResult.iDataLength] = 0;
			}
			break;
		}
	}
	if(oParseResult.bStored == true) aProtocolStorage.chTypeCode = oParseResult.chTypeCode;
	return oParseResult;
}
