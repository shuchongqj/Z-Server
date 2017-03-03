//== ВКЛЮЧЕНИЯ.
#include "main-hub.h"

//== ФУНКЦИИ.
// Проверка на наличие файла.
bool IsFileExists(char *p_chPath)
{
	QFileInfo oCheckFile(p_chPath);
	//
	if (oCheckFile.exists() && oCheckFile.isFile())
		return true;
	return false;
}
