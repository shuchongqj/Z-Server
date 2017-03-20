#include "change_level_dialog.h"
#include "ui_change_level_dialog.h"
#include <QPushButton>

ChangeLevelDialog::ChangeLevelDialog(QWidget *p_parent) :
	QDialog(p_parent),
	p_ui(new Ui::ChangeLevelDialog)
{
	p_ui->setupUi(this);
	p_ui->Accept_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setText("Принять");
	p_ui->Accept_buttonBox->button(QDialogButtonBox::StandardButton::Cancel)->setText("Отмена");
}

ChangeLevelDialog::~ChangeLevelDialog()
{
	delete p_ui;
}

void ChangeLevelDialog::accept()
{
	this->done(p_ui->Level_spinBox->value());
}

void ChangeLevelDialog::reject()
{
	this->done(DIALOGS_REJECT);
}

void ChangeLevelDialog::SetLevel(int iLevel)
{
	p_ui->Level_spinBox->setValue(iLevel);
}
