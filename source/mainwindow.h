#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include "scandevrangedialog.h"
#include "usb_device.h"
#include "usb2can.h"
#include "ControlCAN.h"
#include "can_driver.h"
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
    bool DeviceConfig(void);
    int USB_CAN_status  = 0;
    //定义CAN波特率参数表
    typedef struct _CAN_BAUD_RATE{
      unsigned char   SJW;
      unsigned char   BS1;
      unsigned char   BS2;
      unsigned short  PreScale;
      unsigned int    BaudRate;//前面4个参数对应的波特率，单位为Hz
    } CAN_BAUD_RATE,*PCAN_BAUD_RATE;

    CAN_BAUD_RATE  CANBaudRateTab[27]= {
    //SJW,BS1,BS2,BRP,BAUD
    {1,2,1,25,1000000},    // 1M
    {1,2,1,28,900000},     // 900K
    {1,3,1,25,800000},     // 800K
    {1,3,1,30,666000},     // 666K
    {1,4,1,28,600000},     // 600K
    {1,6,1,25,500000},     // 500K
    {1,8,1,25,400000},     // 400K
    {1,7,1,37,300000},     // 300K
    {1,6,1,50,250000},     // 250K
    {1,3,1,89,225000},     // 225K
    {1,16,3,25,200000},    // 200K
    {1,6,1,78,160000},     // 160K
    {1,15,2,37,150000},    // 150K
    {1,6,1,87,144000},     // 144K
    {1,6,1,100,125000},    // 125K
    {1,6,1,104,120000},    // 120K
    {1,16,3,50,100000},    // 100K
    {1,6,1,139,90000},     // 90K
    {1,1,3,278,80000},     // 80K
    {1,6,1,167,75000},     // 75K
    {1,6,1,208,60000},     // 60K
    {1,6,1,250,50000},     // 50K
    {1,6,1,312,40000},     // 40K
    {1,6,1,417,30000},     // 30K
    {1,6,1,625,20000},     // 20K
    {1,13,2,625,10000},    // 10K
    {1,16,3,1000,5000},    // 5K
    };
    typedef struct
        {
            // VCI_init.Timing0 = 0x00;//波特率的配置
           // VCI_init.Timing1  = 0x1C;//波特率的配置
             unsigned char Timing0;
             unsigned char Timing1;
        }CANBus_Baudrate;
CANBus_Baudrate CANBus_Baudrate_table[4]=
{
    {0x00,0x14},//1000Kbps
    {0x00,0x1C},//500Kbps
    {0x1,0x2F},//200Kbps
    {0x03,0x2F},//100Kbps
};

private slots:
    void on_openFirmwareFilePushButton_clicked();

    void on_updateFirmwarePushButton_clicked();

    void on_openFirmwareFileAction_triggered();

    void on_scanNodeAction_triggered();

    void on_setbaudRatePushButton_clicked();

    void on_contactUsAction_triggered();

    void on_aboutAction_triggered();

    void on_exitAction_triggered();

    void on_Connect_USB_CAN_clicked();

    void on_Close_CAN_clicked();

    void on_Connect_USB_CAN_clicked(bool checked);
    void Time_update();
    void on_action_Open_CAN_triggered();

    private:
    Ui::MainWindow *ui;
     Boot_CMD_LIST cmd_list;
     unsigned char timeout_flag;
    int   CAN_BL_Nodecheck(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int *pVersion,unsigned int *pType,unsigned int TimeOut);
    int   CAN_BL_init(PCBL_CMD_LIST pCmdList);
    int   CAN_BL_erase(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int FlashSize,unsigned int TimeOut);
    int   CAN_BL_write(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int AddrOffset,unsigned char *pData,unsigned int DataNum,unsigned int TimeOut);
    int   CAN_BL_excute(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int Type);
};

#endif // MAINWINDOW_H
