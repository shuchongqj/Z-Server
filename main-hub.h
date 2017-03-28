#ifndef MAINHUB_H
#define MAINHUB_H

//== ВКЛЮЧЕНИЯ.
#include <QFileInfo>

//== МАКРОСЫ.
#define S_CONF_PATH				"./settings/server.xml"
#define S_UI_CONF_PATH			"./settings/ui.ini"
#define S_USERS_CAT_PATH		"./settings/users.xml"
#define S_BANS_CAT_PATH			"./settings/bans.xml"
#define DEF_CHAR_PTH(def)		&(_chpPH = def)
#define CHAR_PTH				char _chpPH

//== ОПРЕДЕЛЕНИЯ ТИПОВ.
typedef void (*CBEOnClose)();

//== ФУНКЦИИ.
/// Проверка на наличие файла.
bool IsFileExists(char *p_chPath);
							///< \param[in] p_chPath Указатель на строку с путём к файлу.
							///< \return true, при удаче.

#endif // MAINHUB_H
