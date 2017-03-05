//== ВКЛЮЧЕНИЯ.
#include "mainwindow.h"
#include <QApplication>

//== ФУНКЦИИ.
// Точка входа в приложение.
int main(int argc, char *argv[])
							///< \param[in] argc Заглушка.
							///< \param[in] argv Заглушка.
							///< \return Общий результат работы.
{
	int iExecResult = -1;
	QApplication oApplication(argc, argv);
	MainWindow wMainWindow;
	//
	if(wMainWindow.bInitOk)
	{
		wMainWindow.show();
		iExecResult = oApplication.exec();
	}
	return iExecResult;
}
