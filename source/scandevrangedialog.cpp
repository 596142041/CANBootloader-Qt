#include "scandevrangedialog.h"
#include "ui_scandevrangedialog.h"

ScanDevRangeDialog::ScanDevRangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScanDevRangeDialog)
{
    ui->setupUi(this);
    ui->spinBoxStartAddr->setValue(0x132);
    ui->spinBoxEndAddr->setValue(0x134);
}

ScanDevRangeDialog::~ScanDevRangeDialog()
{
    delete ui;
}

void ScanDevRangeDialog::on_spinBoxStartAddr_valueChanged(int arg1)
{
    if(arg1 > ui->spinBoxEndAddr->value()){
        ui->spinBoxEndAddr->setValue(arg1);
    }
}

void ScanDevRangeDialog::on_spinBoxEndAddr_valueChanged(int arg1)
{
    if(arg1 < ui->spinBoxStartAddr->value()){
        ui->spinBoxStartAddr->setValue(arg1);
    }
}

void ScanDevRangeDialog::on_pushButtonConfirm_clicked()
{
    StartAddr = ui->spinBoxStartAddr->value();
    EndAddr = ui->spinBoxEndAddr->value();
    this->accept();
}

void ScanDevRangeDialog::on_pushButtonCancel_clicked()
{
    this->reject();
}
void ScanDevRangeDialog::Set_startaddr(int start)
{

ui->spinBoxStartAddr->setValue(start);
}
void ScanDevRangeDialog::Set_endaddr(int end)
{
ui->spinBoxEndAddr->setValue(end);
}
