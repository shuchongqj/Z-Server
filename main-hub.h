#ifndef MAINHUB_H
#define MAINHUB_H

//== ВКЛЮЧЕНИЯ.
#include <QFileInfo>

//== ФУНКЦИИ.
/// Проверка на наличие файла.
bool IsFileExists(char *p_chPath);
							///< \param[in] p_chPath Указатель на строку с путём к файлу.
							///< \return true, при удаче.

#endif // MAINHUB_H
