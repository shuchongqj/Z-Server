#ifndef PROTO_PARSER_H
#define PROTO_PARSER_H

//== ВКЛЮЧЕНИЯ.
#include "protocol.h"

//== МАКРОСЫ.
#define PROTOPARSER_OK					0x00
#define PROTOPARSER_OUT_OF_RANGE		_NMG-4 // См. protocol.h для занятия нового свободного номера.
#define PROTOPARSER_UNKNOWN_COMMAND		_NMG-5 // См. protocol.h для занятия нового свободного номера.
#define POCKET_OUT_OF_RANGE				"Pocket out of range."
#define UNKNOWN_COMMAND					"Unknown command"

//== КЛАССЫ.
/// Класс парсера протокола.
class ProtoParser
{
public:
	/// Структура возвратных результатов парсинга.
	struct ParseResult
	{
		bool bStored; ///< Признак сохранения в новую структуру.
		int iRes; ///< Результат операции.
		char chTypeCode; ///< Код принятого пакета (вне зависимости от статуса сохранения).
		int iDataLength; ///< Длина пакета в байтах.
	};
public:
	/// Парсинг пакета в соответствующий член хранилища.
	ParseResult ParsePocket(char* p_chData, int iLength, ProtocolStorage& aProtocolStorage, bool bDoNotStore = false);
											///< \param[in] p_chData Указатель на пакет.
											///< \param[in] iLength Длина пакета в байтах.
											///< \param[out] aProtocolStorage Ссылка на структуру для заполнения.
											///< \param[in] bDoNotStore Флаг необходимости игнорировать заполнение хранилища.
											///< \return Результат парсинга в соотв. структуре.
};

#endif // PROTO_PARSER_H
