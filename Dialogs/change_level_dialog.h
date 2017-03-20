#ifndef CHANGE_LEVEL_DIALOG_H
#define CHANGE_LEVEL_DIALOG_H

//== ВКЛЮЧЕНИЯ.
#include <QDialog>
#include "../Server/protocol.h"

#define DIALOGS_REJECT			-1

//== ПРОСТРАНСТВА ИМЁН.
namespace Ui {
	class ChangeLevelDialog;
}

//== КЛАССЫ.
/// Класс диалога выбора уровня.
class ChangeLevelDialog : public QDialog
{
	Q_OBJECT

public:
	/// Конструктор.
	explicit ChangeLevelDialog(QWidget *p_parent = 0);
											///< \param[in] p_parent - Указатель на родительский виджет.
	/// Деструктор.
	~ChangeLevelDialog();
	/// Предварительная установка уровня.
	void SetLevel(int iLevel);
											///< \param[in] iLevel - Уровень.

private slots:
	/// Принято.
	void accept();
	/// Отменено.
	void reject();

private:
	Ui::ChangeLevelDialog *p_ui; ///< Указатель на UI.
};

#endif // CHANGE_LEVEL_DIALOG_H
