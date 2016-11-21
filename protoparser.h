#ifndef PROTOPARSER_H
#define PROTOPARSER_H

//== ВКЛЮЧЕНИЯ.
#include "protocol.h"

//== МАКРОСЫ.
#define PROTOPARSER_OK				0x00
#define PROTOPARSER_OUT_OF_RANGE	0x01
#define PROTOPARSER_UNKNOWN_COMMAND	0x02

//== КЛАССЫ.
/// Класс парсера протокола.
class ProtoParser
{
public:
	/// Структура описания и хранинлища.
	struct ParsedObject
	{
		char chTypeCode; ///< Тип пакета.
		int iDataLength; ///< Длина пакета в байтах.
		ProtocolStorage oProtocolStorage; ///< Составной объект хранилища, определяемый в протоколе.
	};
public:
	ParsedObject oParsedObject; ///< Объект структуры описания и хранилище.
public:
	/// Парсинг пакета в соответствующий член хранилища.
	char ParsePocket(char* p_chData, int iLength);
											///< \param[in] p_chData Указатель на пакет.
											///< \param[in] iLength Длина пакета в байтах.
											///< \return Результат парсинга.
};

#endif // PROTOPARSER_H
