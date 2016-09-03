//== ВКЛЮЧЕНИЯ.
#include "protoparser.h"
#include "string.h"

//== МАКРОСЫ.
// Утиль.
#define _ParsedPSName(name)				oParsedObject.oProtocolStorage.name
#define _PPControlSize(name)				if((int)sizeof(_ParsedPSName(PObjNaming(name))) < oParsedObject.iDataLength)		\
										{																						\
											oParsedObject.iDataLength = (int)sizeof(_ParsedPSName(PObjNaming(name)));			\
											chRetVal = PROTOPARSER_OUT_OF_RANGE;												\
											break;																				\
										}																						\
										else chRetVal = PROTOPARSER_OK
#define _CopyDataToStructure(name)		_ParsedPSName(PObjNaming(name)) = *(ProtocolStorage::name*)p_chCurrPos
#define ProcessToStorage(name)			_PPControlSize(name);																	\
										_CopyDataToStructure(name)

//== ФУНКЦИИ КЛАССОВ.
//== Класс парсера протокола.

char ProtoParser::ParsePocket(char* p_chData, int iLength)
{
	char* p_chCurrPos;
	char chRetVal = PROTOPARSER_UNKNOWN_COMMAND;
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
			chRetVal = PROTOPARSER_COMMAND;
			break;
		}
		case PROTO_S_PASSW_OK:
		{
			chRetVal = PROTOPARSER_COMMAND;
			break;
		}
		case PROTO_S_PASSW_ERR:
		{
			chRetVal = PROTOPARSER_COMMAND;
			break;
		}
		// ОБРАБОТКА ПАКЕТОВ.
		// ProcessToStorage(имя структуры в протоколе)
		// - копирует объект в хранилище, проверяя совпадение размера и доверяя любому содержимому (именование по шаблону).
		case PROTO_O_TEXT_MSG:
		{
			ProcessToStorage(TextMsg);
			break;
		}
	}
	return chRetVal;
}
