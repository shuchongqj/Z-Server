//== ВКЛЮЧЕНИЯ.
#include <QCoreApplication>
#include "hub.h"

//== МАКРОСЫ.
#define LOG_NAME				"Z-Server"
//

//== ДЕКЛАРАЦИИ СТАТИЧЕСКИХ ПЕРЕМЕННЫХ.
LOGDECL
#ifndef WIN32
; // Баг в QtCreator`е на Windows, на Linux ';' нужна.
#endif

//== ФУНКЦИИ.
/// Точка входа в приложение.
int main(int argc, char *argv[])
{
	argc = argc; // Заглушка.
	argv = argv; // Заглушка.
	LOG_CTRL_INIT;
	_uiRetval = _uiRetval; // Заглушка.
	//
	LOG(LOG_CAT_I, "Starting server");
	//
	LOGCLOSE;
	return 0;
}
