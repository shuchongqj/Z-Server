#ifndef MAINHUB_H
#define MAINHUB_H

//== ВКЛЮЧЕНИЯ.
#include <QFileInfo>

//== МАКРОСЫ.
#define S_CONF_PATH				"./settings/server.ini"
#define S_UI_CONF_PATH			"./settings/ui.ini"

//== ФУНКЦИИ.
/// Проверка на наличие файла.
bool IsFileExists(char *p_chPath);
							///< \param[in] p_chPath Указатель на строку с путём к файлу.
							///< \return true, при удаче.

#endif // MAINHUB_H
