#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QModelIndex>
#include <QTableWidgetItem>
#include "scandevrangedialog.h"
#include "ControlCAN.h"
#include "can_driver.h"
#include <QSettings>
#include <QDir>
#include <QStringList>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int CAN_GetBaudRateNum(unsigned int BaudRate);
    int USB_CAN_status  = 0;
    typedef struct
        {
         unsigned char Timing0;
         unsigned char Timing1;
         unsigned int  BaudRate;
        }CANBus_Baudrate;
CANBus_Baudrate CANBus_Baudrate_table[27]=
{
    {0x00, 0x14, 1000000},//1000Kbps
    {0x00, 0x16, 800000}, //800Kbps
    {0x00, 0x1C, 500000}, //500Kbps
    {0x00, 0x2F, 400000}, //400Kbps
    {0x01, 0x1C, 250000}, //250Kbps
    {0x01, 0x2F, 200000}, //200Kbps
    {0x03, 0x1C, 125000}, //125Kbps
    {0x03, 0x2F, 100000}, //100Kbps
};

private slots:
    void on_openFirmwareFilePushButton_clicked();

    void on_updateFirmwarePushButton_clicked();

    void on_openFirmwareFileAction_triggered();

    void on_scanNodeAction_triggered();

    void on_contactUsAction_triggered();

    void on_aboutAction_triggered();

    void on_exitAction_triggered();

    void on_Connect_USB_CAN_clicked();

    void on_Close_CAN_clicked();

    void on_action_Open_CAN_triggered();

    void on_action_Close_CAN_triggered();

    void on_Fun_test_clicked();

    void on_action_savefile_triggered();

    private:
    Ui::MainWindow *ui;
    Boot_CMD_LIST cmd_list;
    QStringList   chip_list;
    int CAN_BL_Nodecheck(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int *pVersion,unsigned int *pType,unsigned int      TimeOut);
    int CAN_BL_init(Boot_CMD_LIST pCmdList);
    int CAN_BL_erase(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int FlashSize,unsigned int TimeOut,unsigned char file_type);
    int CAN_BL_write(int DevIndex,int CANIndex,unsigned short NodeAddr,SEND_INFO *send_data, unsigned int TimeOut);
    int CAN_BL_excute(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int Type);
    void  Data_clear(char *data,unsigned long int len);
    void  Data_clear_int(unsigned  short int *data,unsigned long int len);
    unsigned char convertion(char *hex_data);
    void hex_to_bin(char *hex_src,char *bin_dst,unsigned char len);
    unsigned short int CRCcalc16(unsigned char *data, unsigned short int len);
};

#endif // MAINWINDOW_H
