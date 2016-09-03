#ifndef PROTOPARSER_H
#define PROTOPARSER_H

//== ВКЛЮЧЕНИЯ.
#include "protocol.h"

//== МАКРОСЫ.
#define PROTOPARSER_COMMAND			0x00
#define PROTOPARSER_OK				0x01
#define PROTOPARSER_OUT_OF_RANGE	0x02
#define PROTOPARSER_UNKNOWN_COMMAND	0x03

//== КЛАССЫ.
/// Класс парсера протокола.
class ProtoParser
{
public:
	struct ParsedObject
	{
		char chTypeCode;
		int iDataLength;
		ProtocolStorage oProtocolStorage;
	};
public:
	ParsedObject oParsedObject;
public:
	/// Парсинг пакета.
	char ParsePocket(char* p_chData, int iLength);
};

#endif // PROTOPARSER_H
