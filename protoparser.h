#ifndef PROTOPARSER_H
#define PROTOPARSER_H

//== ВКЛЮЧЕНИЯ.
#include "protocol.h"

//== МАКРОСЫ.
#define PROTOPARSER_OK					0x00
#define PROTOPARSER_OUT_OF_RANGE		0x01
#define PROTOPARSER_UNKNOWN_COMMAND		0x02
#define POCKET_OUT_OF_RANGE				"Pocket out of range."
#define UNKNOWN_COMMAND					"Unknown command."

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
	struct ParseResult
	{
		bool bStored;
		char chRes;
	};
public:
	/// Парсинг пакета в соответствующий член хранилища.
	ParseResult ParsePocket(char* p_chData, int iLength, ParsedObject& oParsedObject, bool bDoNotStore = false);
											///< \param[in] p_chData Указатель на пакет.
											///< \param[in] iLength Длина пакета в байтах.
											///< \param[in] oParsedObject Ссылка на объект структуры описания и хранилище.
											///< \param[in] bDoNotStore Флаг необходимости игнорировать заполнение хранилища.
											///< \return Результат парсинга.
};

#endif // PROTOPARSER_H
