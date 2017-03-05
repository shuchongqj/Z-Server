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
	int iExecResult;
	QApplication oApplication(argc, argv);
	MainWindow wMainWindow;
	//
	wMainWindow.show();
	iExecResult = oApplication.exec();
	return iExecResult;
}
