#include "mainwindow.h"
#include "ui_mainwindow_ch.h"
#include <QDebug>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
   int ret;
    VCI_BOARD_INFO1 vci;
    timeout_flag = 0;
    ui->setupUi(this);
    ui->cmdListTableWidget->setColumnWidth(0,180);
    ui->cmdListTableWidget->setColumnWidth(1,180);
    for(int i=0;i<ui->cmdListTableWidget->rowCount();i++){
        ui->cmdListTableWidget->setRowHeight(i,35);
    }
    //qDebug << "sizeof(CANBaudRateTab) = " << sizeof(CANBaudRateTab);
    //检测是否有CAN连接
    ret = VCI_FindUsbDevice(&vci);
    if(ret <= 0)
    {
        USB_CAN_status = 1;
       // ui->deviceIndexComboBox->se

        ui->Connect_USB_CAN->setText(tr("无设备"));
    }
    else
    {
        ui->Connect_USB_CAN->setText(tr("连接"));
        ui->deviceIndexComboBox->setMaxCount(ret);
        ui->deviceIndexComboBox->addItem(tr("USB_CAN"),Qt::DisplayRole);
    }
    //初始化部分按钮的状态
    ui->Close_CAN->setEnabled(false);
    ui->Connect_USB_CAN->setEnabled(true);
    ui->Fun_test->setEnabled(true);
    ui->updateFirmwarePushButton->setEnabled(false);
    ui->newBaudRateComboBox->setEnabled(false);
    ui->allNodeCheckBox->setEnabled(false);
    ui->baudRateComboBox->setCurrentIndex(1);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_openFirmwareFilePushButton_clicked()
{
    QString fileName;
    fileName=QFileDialog::getOpenFileName(this,
                                          tr("Open files"),
                                          "",
                                          "Binary Files (*.bin);;Hex Files (*.hex);;All Files (*.*)");
    if(fileName.isNull()){
        return;
    }
    ui->firmwareLineEdit->setText(fileName);
}

int MainWindow::CAN_GetBaudRateNum(unsigned int BaudRate)
{
    for(int i=0;i<27;i++){
        if(BaudRate == CANBaudRateTab[i].BaudRate){
            return i;
        }
    }
    return 0;
}

bool MainWindow::DeviceConfig(void)
{

    return true;
}

void MainWindow::on_updateFirmwarePushButton_clicked()
{
    QTime time;
    time.start();
    int ret;
    bool ConfFlag;
    uint32_t appversion,appType;
    uint8_t FirmwareData[1026]={0};
    if(ui->allNodeCheckBox->isChecked())
    {
        if(ui->nodeListTableWidget->rowCount()<=0)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("无任何节点！"));
            return;
        }
    }
    else
        {
            if(ui->nodeListTableWidget->currentIndex().row()<0)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("请选择节点！"));
                return;
             }
        }
    uint16_t NodeAddr;
     //USB_CAN_status = 1;
#if 1
    ConfFlag = DeviceConfig();
    if(!ConfFlag){
        return;
    }
#endif
    if(ui->allNodeCheckBox->isChecked()){
        NodeAddr = 0x00;
        ret = CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            NodeAddr,
                            CAN_BL_BOOT);
        if(ret != CAN_SUCCESS)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));
            //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
        Sleep(500);
    }
    else
        {
        NodeAddr = ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),0)->text().toInt(NULL,16);
        ret = CAN_BL_Nodecheck(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            NodeAddr,
                            &appversion,
                            &appType,
                            500);
        if(ret == CAN_SUCCESS){
            if(appType != CAN_BL_BOOT){//当前固件不为Bootloader
                ret = CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),
                                    ui->channelIndexComboBox->currentIndex(),
                                    NodeAddr,
                                    CAN_BL_BOOT);
                if(ret != CAN_SUCCESS)
                   {
                        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));
                        //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                        return;
                    }

                Sleep(500);
            }
        }
        else
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("节点检测失败！"));
            //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
    }
    QFile firmwareFile(ui->firmwareLineEdit->text());
    if (firmwareFile.open(QFile::ReadOnly)){
        if(!ui->allNodeCheckBox->isChecked()){
            ret = CAN_BL_NodeCheck(ui->deviceIndexComboBox->currentIndex(),
                                ui->channelIndexComboBox->currentIndex(),
                                NodeAddr,
                                &appversion,
                                &appType,
                                100);
            if(ret == CAN_SUCCESS)
            {
                if(appType != CAN_BL_BOOT)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("当前固件不为Bootloader固件！"));
                    //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                    return;
                }
            }
            else
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("节点检测失败！"));
                //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                return;
            }
        }
        ret = CAN_BL_erase(ui->deviceIndexComboBox->currentIndex(),
                           ui->channelIndexComboBox->currentIndex(),
                           NodeAddr,
                           firmwareFile.size(),
                           1000);
        if(ret != CAN_SUCCESS)
        {
            qDebug()<<"CBL_EraseFlash = "<<ret;
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("擦出Flash失败！"));
            //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
        if(ui->allNodeCheckBox->isChecked())
        {
            Sleep(1000);
        }
        int read_data_num;
        QProgressDialog writeDataProcess(QStringLiteral("正在更新固件..."),QStringLiteral("取消"),0,firmwareFile.size(),this);
        writeDataProcess.setWindowTitle(QStringLiteral("更新固件"));
        writeDataProcess.setModal(true);
        writeDataProcess.show();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        int i=0;
        int PackSize = 512;
        for(i=0;i<firmwareFile.size();i+=PackSize){
            read_data_num = firmwareFile.read((char*)FirmwareData,PackSize);
            /*
            ret = CAN_BL_write(ui->deviceIndexComboBox->currentIndex(),
                               ui->channelIndexComboBox->currentIndex(),
                               NodeAddr,
                               i,
                               FirmwareData,
                               read_data_num,
                               1000);
                               */
            if(ret != CAN_SUCCESS)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("写Flash数据失败！"));
                //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                return;
            }
            writeDataProcess.setValue(i);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            if(writeDataProcess.wasCanceled()){
                //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                return;
            }
            if(ui->allNodeCheckBox->isChecked()){
                Sleep(10);
            }
        }
        writeDataProcess.setValue(firmwareFile.size());
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if(writeDataProcess.wasCanceled()){
            //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
    }
    else
        {
        //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("打开固件文件失败！"));
        return;
         }
    //执行固件
    ret = CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),
                        ui->channelIndexComboBox->currentIndex(),
                        NodeAddr,
                        CAN_BL_APP);
    if(ret != CAN_SUCCESS)
    {
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));

    }
    Sleep(50);
    if(!ui->allNodeCheckBox->isChecked()){
        ret = CAN_BL_Nodecheck(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            NodeAddr,
                            &appversion,
                            &appType,
                            500);
        if(ret == CAN_SUCCESS){
            QString str;
            if(appType == CAN_BL_BOOT){
                str = "BOOT";
            }else{
                str = "APP";
            }
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),1)->setText(str);
            str.sprintf("v%d.%d",(((appversion>>24)&0xFF)*10)+(appversion>>16)&0xFF,(((appversion>>8)&0xFF)*10)+appversion&0xFF);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),2)->setText(str);
        }else{
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));
        }
    }
    //USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
    qDebug()<<time.elapsed()/1000.0<<"s";
}

void MainWindow::on_openFirmwareFileAction_triggered()
{
    on_openFirmwareFilePushButton_clicked();
}


void MainWindow::on_scanNodeAction_triggered()
{
    int ret;
    int startAddr = 0,endAddr = 0;
    ScanDevRangeDialog *pScanDevRangeDialog = new ScanDevRangeDialog();
    if(pScanDevRangeDialog->exec() == QDialog::Accepted)
    {
        startAddr = pScanDevRangeDialog->StartAddr;
        endAddr = pScanDevRangeDialog->EndAddr;
    }
    else
    {
        return ;
    }
    if( USB_CAN_status != 0x04)
    {
          QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("打开设备失败！"));
          return ;
    }
    ui->nodeListTableWidget->verticalHeader()->hide();
    ui->nodeListTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->nodeListTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->nodeListTableWidget->setRowCount(0);
    QProgressDialog scanNodeProcess(QStringLiteral("正在扫描节点..."),QStringLiteral("取消"),0,endAddr-startAddr,this);
    scanNodeProcess.setWindowTitle(QStringLiteral("扫描节点"));
    scanNodeProcess.setModal(true);
    scanNodeProcess.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    int i=0;
    while(startAddr <= endAddr)
    {
        uint32_t appversion,appType;
        i++;
        /*
        ret = CAN_BL_NodeCheck(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            startAddr,
                            &appversion,
                            &appType,
                            10);
                            */
        ret = CAN_BL_Nodecheck(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            startAddr,
                            &appversion,
                            &appType,
                            10);
        if(ret == CAN_SUCCESS)
        {
            ui->nodeListTableWidget->setRowCount(ui->nodeListTableWidget->rowCount()+1);
            ui->nodeListTableWidget->setRowHeight(ui->nodeListTableWidget->rowCount()-1,20);
            QString str;
            str.sprintf("0x%X",startAddr);
            QTableWidgetItem *item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,0,item);
            if(appType == CAN_BL_BOOT){
                     qDebug() << "appType=0x%x " << appType;
                //   qDebug << "appType=0x%x " << appType;
                str = "BOOT";
            }else{
                str = "APP";
                qDebug() << "appType=0x%x " << appType;
            }
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,1,item);
            str.sprintf("v%d.%d",(((appversion>>24)&0xFF)*10)+(appversion>>16)&0xFF,(((appversion>>8)&0xFF)*10)+appversion&0xFF);
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,2,item);
        }
        scanNodeProcess.setValue(i);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if(scanNodeProcess.wasCanceled()){
            USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
        startAddr++;
    }
    USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
}
/*********************************************
 * 目前该按钮主要适用于我测试相关的功能函数
 *
 *
 *
 *
*/
void MainWindow::on_Fun_test_clicked()
{
    on_setbaudRatePushButton_clicked();
    //int   CAN_BL_write(int DevIndex,int CANIndex,unsigned short NodeAddr,SEND_INFO *send_data, unsigned int TimeOut);
    SEND_INFO send_data;
    send_data.data_addr = 0x318000;
   // send_data.data_cnt  = 16;
    send_data.data_len = 16;
    for(int i = 0;i <send_data.data_len;i++)
        {
          send_data.data[i] = i<<1;
        }
    CAN_BL_write(ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),0x134,&send_data,100);
    qint64 test = 0xFF;
    bool ret;
    PACK_INFO pack_info;
    int   hex_size = 0;
    char hex_buf[128];
    char bin_buf[128];
    QFile file(ui->firmwareLineEdit->text());
    qDebug() << "file name ;"<<file.fileName();
    ret = file.open(QFile::ReadOnly);//以只读的方式打开文件
    if(ret == 0)
        {
           qDebug() << "open file fals;";
            return;
        }
    ret = file.seek(0);//移动文件指针到文件头
    if(ret == 0)
        {

             qDebug() << "firmwareFile.seek";
            return;
        }
    test = file.pos();
    ui->progressBar->setWindowTitle("文件读取");
    ui->progressBar->setRange(0,file.size());
    qDebug() << "file.size() = "<<file.size();
    while(hex_size <file.size())
        {
            file.readLine((char*)hex_buf,10);
            if(hex_buf[0] == ':')//表示是起始标志,判断刚才读取的数据中的第一个字节是否是起始标志
                {
                  qDebug() << "hex_buf  = "<<hex_buf ;
                  hex_to_bin(&hex_buf[1],bin_buf,8);//将读取的9个字节后面8字节由ASC_II转换为hex(16进制数据)
                  pack_info.data_type = bin_buf[6]<<4|bin_buf[7];
                  switch (pack_info.data_type)
                      {
                      case DATA_BASE_ADDR:
                           qDebug() << " DATA_BASE_ADDR";
                          break;
                      case DATA_Rrecord:
                           qDebug() << " DATA_Rrecord";
                          break;
                      case DATA_END:
                           qDebug() << " DATA_END";
                      default:
                          break;
                      }
                      pack_info.data_len = bin_buf[0]<<4|bin_buf[1];
                      if(pack_info.data_type == DATA_Rrecord)//判断该行的数据是,如果是表示基地址
                      {
                          pack_info.data_addr_offset = bin_buf[2]<<12|bin_buf[3]<<8|bin_buf[4]<<4|bin_buf[5];
                      }
                      else
                      {
                          pack_info.data_addr_offset = 0x0000;
                      }
                      Data_clear(hex_buf,128);
                      file.readLine((char*)hex_buf,(pack_info.data_len*2+3+1));
                      qDebug() << "hex_buf is = "<<hex_buf;
                      hex_to_bin(&hex_buf[0],bin_buf,pack_info.data_len*2);//将读取的数据转换为hex;
                      if(pack_info.data_type == DATA_BASE_ADDR)
                      {
                        pack_info.data_base_addr =bin_buf[0]<<12|bin_buf[1]<<8|bin_buf[2]<<4|bin_buf[3];
                        pack_info.data_base_addr = pack_info.data_base_addr<<16;
                      }
                      else if(pack_info.data_type == 0x00)
                      {
                            pack_info.data_addr = pack_info.data_base_addr+pack_info.data_addr_offset;//表示该行数据应该写入的真实地址
                            for( int i = 0;i <pack_info.data_len;i++)
                            {
                               pack_info.Data[i] = bin_buf[i*2]<<4|bin_buf[2*i+1];
                                qDebug() << "pack_info.Data["<<i<<"]="<<pack_info.Data[i];
                            }

                     }
                    Data_clear(hex_buf,128);
                    Data_clear(bin_buf,128);
                    Data_clear_int(&pack_info.Data[0],64);
                    test = file.pos();
                    qDebug() << "test = "<<test;
                    file.seek(file.pos()+1);
                    test = file.pos();
                    qDebug() << "test = "<<test;
                }
            hex_size = file.pos();
            qDebug() << "hex_size = "<<hex_size;
            ui->progressBar->setValue(hex_size);
        }
}
void MainWindow::on_setbaudRatePushButton_clicked()
{
        #if 0
    if(ui->allNodeCheckBox->isChecked())
    {
        if(ui->nodeListTableWidget->rowCount()<=0)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("无任何节点！"));
            return;
        }
    }
    else
    {
        if(ui->nodeListTableWidget->currentIndex().row()<0)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("请选择节点！"));
            return;
        }
    }
    /*
     *    CAN_INIT_CONFIG CAN_InitConfig;
    bool ConfFlag = DeviceConfig();
    if(!ConfFlag)
    {
        USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
        return;
    }
    QString str = ui->newBaudRateComboBox->currentText();
    str.resize(str.length()-4);
    int baud = str.toInt(NULL,10)*1000;
    CAN_InitConfig.CAN_BRP = CANBaudRateTab[CAN_GetBaudRateNum(baud)].PreScale;
    CAN_InitConfig.CAN_SJW = CANBaudRateTab[CAN_GetBaudRateNum(baud)].SJW;
    CAN_InitConfig.CAN_BS1 = CANBaudRateTab[CAN_GetBaudRateNum(baud)].BS1;
    CAN_InitConfig.CAN_BS2 = CANBaudRateTab[CAN_GetBaudRateNum(baud)].BS2;


    ret = CAN_BL_SetNewBaudRate(ui->deviceIndexComboBox->currentIndex(),
                                ui->channelIndexComboBox->currentIndex(),
                                NodeAddr,
                                &CAN_InitConfig,
                                baud,
                                100);
    if(ret != CAN_SUCCESS)
    {
        USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("设置波特率失败！"));
        return;
    }
    ui->baudRateComboBox->setCurrentIndex(ui->newBaudRateComboBox->currentIndex());
    USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
    */
    uint16_t NodeAddr;
    if(ui->allNodeCheckBox->isChecked())
    {
        NodeAddr = 0x00;
    }
    else
    {
        NodeAddr = ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),0)->text().toInt(NULL,16);
    }
   #if 1
     int ret;
   ret  = CAN_BL_erase(ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),NodeAddr,0x120,50);
    if(ret == 1)
        {
              qDebug() << "数据擦除成功";
        }
    else
        {
            qDebug() << "失败";
        }
       #endif

     int ret;
    if(ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),1)->text() == "APP")
        {
            qDebug() << "CAN_BL_BOOT";
            CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),NodeAddr,CAN_BL_BOOT);
        }
        else
        {
             qDebug() << "CAN_BL_APP";
             CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),NodeAddr,CAN_BL_APP);
        }

#endif
}

void MainWindow::on_contactUsAction_triggered()
{
    QString AboutStr;
    AboutStr.append(("官方网站<span style=\"font-size:12px;\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span>：<a href=\"http://www.usbxyz.com\">www.usbxyz.com</a><br>"));
    AboutStr.append(("官方论坛<span style=\"font-size:12px;\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span>：<a href=\"http://www.embed-net.com\">www.embed-net.com</a><br>"));
    AboutStr.append(("官方淘宝店<span style=\"font-size:9px;\">&nbsp;&nbsp;&nbsp;</span>：<a href=\"http://usbxyz.taobao.com/\">usbxyz.taobao.com</a><br>"));
    AboutStr.append(("技术支持QQ：188298598<br>"));
    AboutStr.append(("产品咨询QQ：188298598"));
    QMessageBox::about(this,("联系我们"),AboutStr);
}

void MainWindow::on_aboutAction_triggered()
{
    QString AboutStr;
    AboutStr.append("USB2XXX USB2CAN Bootloader 1.0.1<br>");
    AboutStr.append(("支持硬件：USB2XXX<br>"));
    AboutStr.append(("购买地址：<a href=\"http://usbxyz.taobao.com/\">usbxyz.taobao.com</a>"));
    QMessageBox::about(this,("关于USB2XXX USB2CAN Bootloader"),AboutStr);
}

void MainWindow::on_exitAction_triggered()
{
    this->close();
}

void MainWindow::on_Connect_USB_CAN_clicked()
{
    int ret;
    bool state;
    state = VCI_OpenDevice(4,ui->deviceIndexComboBox->currentIndex(),0);
    if(!state)
    {
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("打开设备失败！"));
         ui->Connect_USB_CAN->setText(tr("无设备"));
         USB_CAN_status = 1;
        return;
    }
    else
        {
            if((USB_CAN_status == 1)||(USB_CAN_status == 0))
                {
                    USB_CAN_status = 0;
                     ui->Connect_USB_CAN->setText(tr("连接"));
                }

        }
    if(USB_CAN_status == 1)
        {

            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("无设备连接！"));
        }

    CBL_CMD_LIST CMD_List;
   // QString cmdStr[]={"Erase","WriteInfo","Write","Check","SetBaudRate","Excute","CmdSuccess","CmdFaild"};
    QString cmdStr[]={"Erase","Write","Check","Excute","WriteInfo","CmdFaild","CmdSuccess","SetBaudRate"};
    uint8_t cmdData[16];
    for(int i=0;i<ui->cmdListTableWidget->rowCount();i++)
    {
         ui->cmdListTableWidget->item(i,0)->setTextAlignment(Qt::AlignHCenter);
          ui->cmdListTableWidget->item(i,0)->setTextAlignment(Qt::AlignCenter);
         ui->cmdListTableWidget->item(i,1)->setTextAlignment(Qt::AlignHCenter);
          ui->cmdListTableWidget->item(i,1)->setTextAlignment(Qt::AlignCenter);
          qDebug()<<""<<cmdStr[i]<<""<<ui->cmdListTableWidget->item(i,1)->text().toInt(NULL,16);
        if(ui->cmdListTableWidget->item(i,0)->text()==cmdStr[i])
        {
            cmdData[i] = ui->cmdListTableWidget->item(i,1)->text().toInt(NULL,16);
            qDebug()<<""<<cmdStr[i]<<""<< cmdData[i];
        }
    }
    CMD_List.Erase = cmdData[0];
    CMD_List.WriteInfo = cmdData[4];
    CMD_List.Write = cmdData[1];
    CMD_List.Check = cmdData[2];
    CMD_List.SetBaudRate = cmdData[7];
    CMD_List.Excute = cmdData[3];
    CMD_List.CmdSuccess = cmdData[6];
    CMD_List.CmdFaild = cmdData[5];
    CAN_BL_init(&CMD_List);//初始化cmd
    int baud_indx;
    baud_indx = ui->baudRateComboBox->currentIndex();
    VCI_INIT_CONFIG VCI_init;
    VCI_init.AccCode = 0x00000000;
    VCI_init.AccMask = 0xFFFFFFFF;
    VCI_init.Filter = 1;
    VCI_init.Mode = 0;
    VCI_init.Reserved = 0x00;
    VCI_init.Timing0  = CANBus_Baudrate_table[baud_indx].Timing0;//波特率的配置
    VCI_init.Timing1  = CANBus_Baudrate_table[baud_indx].Timing1;//波特率的配置
    if(ui->channelIndexComboBox->currentIndex() == 0||(ui->channelIndexComboBox->currentIndex() == 1))
        {
            ret = VCI_InitCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),&VCI_init);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败！"));
                USB_CAN_status = 3;
                return;
            }
            ret = 0;
            ret = VCI_StartCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败 USB_CAN_status = 6 ！"));
                USB_CAN_status = 3;
                return;
            }
            ui->Connect_USB_CAN->setEnabled(false);
            ui->Close_CAN->setEnabled(true);
         //   ui->Fun_test->setEnabled(true);
            ui->updateFirmwarePushButton->setEnabled(true);
             ui->newBaudRateComboBox->setEnabled(true);
             ui->allNodeCheckBox->setEnabled(true);
             ui->baudRateComboBox->setEnabled(false);
             ui->deviceIndexComboBox->setEnabled(false);
             ui->channelIndexComboBox->setEnabled(false);
             USB_CAN_status = 0x04;
             VCI_ClearBuffer(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
        }
    else if (ui->channelIndexComboBox->currentIndex() == 2)
        {
            ret = VCI_InitCAN(4,ui->deviceIndexComboBox->currentIndex(),0,&VCI_init);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败！"));
                USB_CAN_status = 3;
                return;
            }
            ret = VCI_StartCAN(4,ui->deviceIndexComboBox->currentIndex(),0);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败 USB_CAN_status =4！"));
               USB_CAN_status = 3;
                return;
            }
            ret = VCI_InitCAN(4,ui->deviceIndexComboBox->currentIndex(),1,&VCI_init);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败！"));
                USB_CAN_status = 3;
                return;
            }
            ret = VCI_StartCAN(4,ui->deviceIndexComboBox->currentIndex(),1);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败 USB_CAN_status =5！"));
                USB_CAN_status = 3;
                return;
            }
            ui->channelIndexComboBox->setDisabled(true);
            ui->channelIndexComboBox->setEnabled(false);
            ui->Connect_USB_CAN->setEnabled(false);
            ui->Close_CAN->setEnabled(true);
         //   ui->Fun_test->setEnabled(true);
            ui->updateFirmwarePushButton->setEnabled(true);
            ui->newBaudRateComboBox->setEnabled(true);
            ui->allNodeCheckBox->setEnabled(true);
            ui->baudRateComboBox->setEnabled(false);
            ui->deviceIndexComboBox->setEnabled(false);
            ui->channelIndexComboBox->setEnabled(false);
             USB_CAN_status = 0x04;
             VCI_ClearBuffer(4,ui->deviceIndexComboBox->currentIndex(),0);
             VCI_ClearBuffer(4,ui->deviceIndexComboBox->currentIndex(),1);
        }

    }

void MainWindow::on_Close_CAN_clicked()
{
    int ret;
    if(ui->deviceIndexComboBox->currentIndex() != 2)
        {
            ret = VCI_ResetCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("复位设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
            ret = VCI_CloseDevice(4,ui->deviceIndexComboBox->currentIndex());
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("关闭设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
        }
    else if(ui->deviceIndexComboBox->currentIndex() == 2)
        {
            ret = VCI_ResetCAN(4,ui->deviceIndexComboBox->currentIndex(),0);
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("复位设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
            ret = VCI_ResetCAN(4,ui->deviceIndexComboBox->currentIndex(),1);
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("复位设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
            ret = VCI_CloseDevice(4,ui->deviceIndexComboBox->currentIndex());
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("关闭设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
        }
 //   ui->Fun_test->setEnabled(false);
    ui->updateFirmwarePushButton->setEnabled(false);
    ui->Connect_USB_CAN->setEnabled(true);
    ui->Close_CAN->setEnabled(false);
    ui->baudRateComboBox->setEnabled(true);
    ui->deviceIndexComboBox->setEnabled(true);
    ui->channelIndexComboBox->setEnabled(true);
    ui->newBaudRateComboBox->setEnabled(false);
    ui->allNodeCheckBox->setEnabled(false);
}
void MainWindow::on_action_Open_CAN_triggered()
{
on_Connect_USB_CAN_clicked();
}

void MainWindow::on_action_Close_CAN_triggered()
{
    on_Close_CAN_clicked();
}
//------------------------------------------------------------------------
//以下函数是根据自己的CAN设备进行编写
int MainWindow::CAN_BL_Nodecheck(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int *pVersion,unsigned int *pType,unsigned int TimeOut)
{
    int ret = 0;
    int read_num = 0;
    bootloader_data Bootloader_data ;
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    VCI_CAN_OBJ can_send_msg,can_read_msg[1000];
    Bootloader_data.ExtId.bit.reserve = 0x00;
    Bootloader_data.ExtId.bit.cmd = cmd_list.Check;
    Bootloader_data.ExtId.bit.addr = NodeAddr;
    can_send_msg.DataLen = 0;
    can_send_msg.SendType = 1;
    can_send_msg.RemoteFlag = 0;
    can_send_msg.ExternFlag = 1;
    can_send_msg.ID = Bootloader_data.ExtId.all;
    ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
    if(ret == -1)
    {
     return 1;
    }
    /*
    QTimer::singleShot(TimeOut, this, &MainWindow::Time_update);
    if(timeout_flag == 1)
        {
            timeout_flag = 0;
             read_num  =VCI_GetReceiveNum(4,DevIndex,CANIndex);
        }
     else
        {
            read_num = 0;
        }
*/
    //QTimer::singleShot(TimeOut, this, &MainWindow::Time_update);  //lpr 删除2017-07-13
        //-----------------------------------------
        //lpr 添加
        DWORD current_time = 0;
        DWORD new_time = 0;
        current_time = GetTickCount();
        unsigned char CAN_BL_Nodecheck_flag = 0;
        while (CAN_BL_Nodecheck_flag == 0)
            {
                new_time = GetTickCount();
                if(new_time-current_time>TimeOut)
                    {
                       CAN_BL_Nodecheck_flag = 1;
                    }
            }
        //--------------------------------------------
        read_num  =VCI_GetReceiveNum(4,DevIndex,CANIndex);
    if(read_num == 0)
        {
            return 1;
        }
    else if(read_num == -1)
        {
            return 1;
        }
    else
        {
            ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
            if(ret == -1)
            {
                return 1;
                VCI_ClearBuffer(4,DevIndex,CANIndex);
            }
            if(ret == 1)
            {
                *pVersion = can_read_msg[0].Data[0]<<24|can_read_msg[0].Data[1]<<16|can_read_msg[0].Data[2]<<8|can_read_msg[0].Data[3]<<0;
                *pType =    can_read_msg[0].Data[4]<<24|can_read_msg[0].Data[5]<<16|can_read_msg[0].Data[6]<<8|can_read_msg[0].Data[7]<<0;
            }
        }

    VCI_ClearBuffer(4,DevIndex,CANIndex);
    return 0;
}
int MainWindow::CAN_BL_init(PCBL_CMD_LIST pCmdList)
{

    cmd_list.Check      = pCmdList->Check;
    cmd_list.Erase      = pCmdList->Erase;
    cmd_list.Excute     = pCmdList->Excute;
    cmd_list.Write      = pCmdList->Write;
    cmd_list.WriteInfo  = pCmdList->WriteInfo;
    cmd_list.CmdFaild   = pCmdList->CmdFaild;
    cmd_list.CmdSuccess = pCmdList->CmdSuccess;
    return 0;
}

int MainWindow::CAN_BL_erase(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int FlashSize,unsigned int TimeOut)
{

    int ret;
    bootloader_data Bootloader_data ;
    int i,read_num;
    VCI_CAN_OBJ can_send_msg,can_read_msg[1000];
    Bootloader_data.DLC = 4;
    Bootloader_data.ExtId.bit.cmd = cmd_list.Erase;
    Bootloader_data.ExtId.bit.addr = NodeAddr;
    Bootloader_data.ExtId.bit.reserve = 0;
    Bootloader_data.IDE = CAN_ID_EXT;
    Bootloader_data.data[0] = ( FlashSize & 0xFF000000 ) >> 24;
    Bootloader_data.data[1] = ( FlashSize & 0xFF0000 ) >> 16;
    Bootloader_data.data[2] = ( FlashSize & 0xFF00 ) >> 8;
    Bootloader_data.data[3] = ( FlashSize & 0x00FF );
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    can_send_msg.DataLen = Bootloader_data.DLC;
    can_send_msg.SendType = 1;
    can_send_msg.RemoteFlag = 0;
    can_send_msg.ExternFlag = Bootloader_data.IDE;
    can_send_msg.ID = Bootloader_data.ExtId.all;
    for(i = 0;i<Bootloader_data.DLC;i++)
        {
            can_send_msg.Data[i] = Bootloader_data.data[i];
        }
    ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
    if(ret == -1)
    {
     return 1;
    }
    //QTimer::singleShot(TimeOut, this, &MainWindow::Time_update);  //lpr 删除2017-07-13
    //-----------------------------------------
    //lpr 添加
    DWORD current_time = 0;
    DWORD new_time = 0;
    current_time = GetTickCount();
    unsigned char CAN_BL_erase_flag = 0;
    while (CAN_BL_erase_flag == 0)
        {
            new_time = GetTickCount();
            if(new_time-current_time>TimeOut)
                {
                   CAN_BL_erase_flag = 1;
                }
        }
    //--------------------------------------------
    if(CAN_BL_erase_flag == 1)
        {
            CAN_BL_erase_flag = 0;
             read_num  =VCI_GetReceiveNum(4,DevIndex,CANIndex);
        }
        else
        {
            read_num = 0;
        }

    if(read_num == 0)
        {
            return 1;
        }
    else if(read_num == -1)
        {
            return 1;
        }
    else
        {
            ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
            if(ret == -1)
            {
                return 1;
                VCI_ClearBuffer(4,DevIndex,CANIndex);
            }
            if(ret == 1)
            {
//判断返回结果
                if(can_read_msg[0].ID == (Bootloader_data.ExtId.bit.addr<<4|cmd_list.CmdSuccess))//表示反馈数据有效
                    {
                        return 0;
                        qDebug()<<"成功擦除数据";
                    }
                    else
                    {
                        qDebug()<<"数据无效";
                        return 1;
                    }

            }
        }

    VCI_ClearBuffer(4,DevIndex,CANIndex);
    return 0;
}

//int MainWindow::CAN_BL_write(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int AddrOffset,unsigned char *pData,unsigned int DataNum,unsigned int TimeOut)
int MainWindow:: CAN_BL_write(int DevIndex,int CANIndex,unsigned short NodeAddr,SEND_INFO *send_data, unsigned int TimeOut)
{
        unsigned char i;
        unsigned char cnt = 0;
        //lpr 添加
        DWORD current_time = 0;
        DWORD new_time = 0;
        int read_num = 0;
        bool CAN_BL_write_flag = 0;
        UINT crc_temp = 0x0000;
      //  int CAN1_CanRxMsgFlag;
         int ret;
        bootloader_data Bootloader_data;
        //Bootloader_data部分成员进行赋值,后面的程序无需关注
        Bootloader_data.ExtId.bit.addr = NodeAddr;
        Bootloader_data.ExtId.bit.reserve = 0;
        Bootloader_data.IDE = CAN_ID_EXT;
       // CanTxMsg TxMessage;
        VCI_CAN_OBJ can_send_msg,can_read_msg[1000];
        //进行crc计算,并且进行赋值
       // crc_temp = CRCcalc16(&send_data->data[0],send_data->data_len);
        crc_temp = CRCcalc16((char*)send_data->data ,send_data->data_len);
        //对crc计算结果进行赋值;
        send_data->data[send_data->data_len] = crc_temp&0xFF;
        send_data->data[send_data->data_len+1] = (crc_temp>>8)&0xFF;
        //准备发送数据
        //计算DSP 写入数据的地址
        //发送当前数据包的相关信息,数据包的数据长度,数据包的偏移地址
        //包含8字节:
        //0-3字节表示地址偏移 4-7表示数据包的大小
        Bootloader_data.DLC = 8;
        Bootloader_data.ExtId.bit.cmd = cmd_list.WriteInfo;
        Bootloader_data.data[0] = ( send_data->data_addr & 0xFF000000 ) >> 24;
        Bootloader_data.data[1] = ( send_data->data_addr & 0xFF0000 ) >> 16;
        Bootloader_data.data[2] = ( send_data->data_addr & 0xFF00 ) >> 8;
        Bootloader_data.data[3] = ( send_data->data_addr & 0x00FF );

        Bootloader_data.data[4] = ( ( send_data->data_len + 2 ) & 0xFF000000 ) >> 24;
        Bootloader_data.data[5] = ( ( send_data->data_len + 2 ) & 0xFF0000 ) >> 16;
        Bootloader_data.data[6] = ( ( send_data->data_len + 2 ) & 0xFF00 ) >> 8;
        Bootloader_data.data[7] = ( ( send_data->data_len + 2 ) & 0x00FF );
       // can_send_msg.DLC = Bootloader_data.DLC;
        can_send_msg.DataLen = Bootloader_data.DLC;
        can_send_msg.ExternFlag = Bootloader_data.IDE;
        can_send_msg.ID = Bootloader_data.ExtId.all;
      //  TxMessage.ExtId  = Bootloader_data.ExtId.all;
        for (i = 0; i < Bootloader_data.DLC; i++)
        {
             can_send_msg.Data[i] = Bootloader_data.data[i];
        }
         //  CAN_WriteData(&TxMessage);
        ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
        if(ret == -1)
        {
         return 1;
        }

       current_time = GetTickCount();//超时判断
       while (CAN_BL_write_flag == 0)
           {
               new_time = GetTickCount();
               if(new_time-current_time>TimeOut)
                   {
                      CAN_BL_write_flag = 1;
                   }
           }

  //--------------------------------------------
       //读取设备反馈数据
        if(CAN_BL_write_flag == 1)
          {
             CAN_BL_write_flag = 0;
             read_num  =VCI_GetReceiveNum(4,DevIndex,CANIndex);
         }
         else
         {
           read_num = 0;
         }
        switch (read_num) {
            case 0:
                   return 1;
                   break;
             case -1:
                    return 1;
                    break;
            default:
                ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
                            if(ret == -1)
                            {
                                return 1;
                                VCI_ClearBuffer(4,DevIndex,CANIndex);
                            }
                            if(ret == 1)
                            {
                                 //判断返回结果
                                if(can_read_msg[0].ID == (UINT)(Bootloader_data.ExtId.bit.addr<<4|cmd_list.CmdSuccess))//表示反馈数据有效
                                    {

                                    }
                                    else
                                    {
                                       // qDebug()<<"数据无效";
                                        return 1;
                                    }

                            }
                    break;
            }
      /*  CAN1_CanRxMsgFlag = 0;
        if (CAN1_RxMessage.ExtId != ( ( Bootloader_data.ExtId.all & 0xFFF0 )| cmd_list.CmdSuccess ))
        {

            return 0xA0;
        }
        */
        //发送数据
        while(cnt < send_data->data_len+2)
        {
            int temp;
            temp = send_data->data_len + 2 - cnt;
            if (temp >= 8)
            {
                Bootloader_data.DLC = 8;
                Bootloader_data.ExtId.bit.cmd = cmd_list.Write;
                //--------------------------------------------------------
                /*
                TxMessage.DLC = Bootloader_data.DLC;
                TxMessage.IDE = Bootloader_data.IDE;
                TxMessage.ExtId = Bootloader_data.ExtId.all;
                */
                // can_send_msg.DLC = Bootloader_data.DLC;
                 can_send_msg.DataLen = Bootloader_data.DLC;
                 can_send_msg.ExternFlag = Bootloader_data.IDE;
                 can_send_msg.ID = Bootloader_data.ExtId.all;
                for (i = 0; i < Bootloader_data.DLC; i++)
                {
                    can_send_msg.Data[i] = send_data->data[cnt];
                    cnt++;
                }
            }
            else
            {
                Bootloader_data.DLC = temp;
                Bootloader_data.ExtId.bit.cmd = cmd_list.Write;
                //--------------------------------------------------------
                // can_send_msg.DLC = Bootloader_data.DLC;
                 can_send_msg.DataLen = Bootloader_data.DLC;
                 can_send_msg.ExternFlag = Bootloader_data.IDE;
                 can_send_msg.ID = Bootloader_data.ExtId.all;
                 /*
                TxMessage.DLC = Bootloader_data.DLC;
                TxMessage.IDE = Bootloader_data.IDE;
                TxMessage.ExtId = Bootloader_data.ExtId.all;
                */
                for (i = 0; i < Bootloader_data.DLC; i++)
                {
                    can_send_msg.Data[i] = send_data->data[cnt];
                    cnt++;
                }
            }
            //CAN_WriteData(&TxMessage);//发送数据
            ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
            if(ret == -1)
            {
             return 1;
            }
        }
        for (i = 0; i < send_data->data_len + 2; i++)
        {
            send_data->data[cnt] = 0x00;
        }
        //发送数据完成,等待响应
        /*
        while (CAN1_CanRxMsgFlag != 1)
        {
            ;
        }
        CAN1_CanRxMsgFlag = 0;
        */
        //-----------------------------------
        current_time = GetTickCount();//超时判断
        while (CAN_BL_write_flag == 0)
            {
                new_time = GetTickCount();
                if(new_time-current_time>TimeOut)
                    {
                       CAN_BL_write_flag = 1;
                    }
            }

        //读取设备反馈数据
         if(CAN_BL_write_flag == 1)
           {
              CAN_BL_write_flag = 0;
              read_num  =VCI_GetReceiveNum(4,DevIndex,CANIndex);
          }
          else
          {
            read_num = 0;
          }
         switch (read_num) {
             case 0:
                    return 1;
                    break;
              case -1:
                     return 1;
                     break;
             default:
                 ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
                             if(ret == -1)
                             {
                                 return 1;
                                 VCI_ClearBuffer(4,DevIndex,CANIndex);
                             }
                             if(ret == 1)
                             {
                                  //判断返回结果
                                 if(can_read_msg[0].ID == (Bootloader_data.ExtId.bit.addr<<4|cmd_list.CmdSuccess))//表示反馈数据有效
                                     {

                                     }
                                     else
                                     {
                                        // qDebug()<<"数据无效";
                                         return 1;
                                     }

                             }
                     break;
             }
        //将数据清零准备下一次数据发送工作
        cnt = 0;
        return 0;
}

int MainWindow::CAN_BL_excute(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int Type)
{
    int ret = 0;
    bootloader_data Bootloader_data ;
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    VCI_CAN_OBJ can_send_msg;
    Bootloader_data.ExtId.bit.reserve = 0x00;
    Bootloader_data.ExtId.bit.cmd = cmd_list.Excute;
    Bootloader_data.ExtId.bit.addr = NodeAddr;
    can_send_msg.DataLen = 4;
    can_send_msg.SendType = 1;
    can_send_msg.RemoteFlag = 0;
    can_send_msg.ExternFlag = 1;
    can_send_msg.ID = Bootloader_data.ExtId.all;
    can_send_msg.Data[0] =Type&0x000000FF;
    can_send_msg.Data[1] =Type&0x000000FF;
    can_send_msg.Data[2] =Type&0x000000FF;
    can_send_msg.Data[3] =Type&0x000000FF;
    ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
    if(ret == -1)
    {
     return 1;
    }
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    return 0;
}
//-----------------------------------------------------------------------------------
//以下代码均为对hex文件解码需要的代码
void MainWindow:: Data_clear_int(  unsigned short  int *data,unsigned long int len)
    {
        unsigned long int i;
        for(i = 0;i < len;i++)
        {
            *data = 0;
            data++;
        }
    }

  void MainWindow:: Data_clear(  char *data,unsigned long int len)
{
     unsigned long int i;
     for(i = 0;i < len;i++)
     {
         *data = 0;
         data++;
     }
}
  unsigned char MainWindow:: convertion(  char *hex_data)
{
    unsigned char bin_data = 0xFF;
    if(*hex_data >= '0'&&*hex_data <= '9')
    {

        bin_data = *hex_data-NUM_OFFSET;
    }
    else if(*hex_data >= 'A'&&*hex_data <= 'F')
    {
        bin_data = *hex_data-CHAR_OFFSET;
    }
    return bin_data;
}
  void MainWindow:: hex_to_bin(  char *hex_src,  char *bin_dst,unsigned char len)
{
    unsigned char i;
    for(i = 0;i <len;i++)
    {
        *bin_dst = convertion(hex_src);
        bin_dst++;
        hex_src++;
    }
}
 unsigned short int MainWindow::CRCcalc16(  char *data, unsigned short int len)
{
    unsigned short int crc_res = 0xFFFF;
     while (len--)
     {
         crc_res ^= *data++;
         for ( int  i = 0; i < 8; i++)
         {
             if (crc_res & 0x01)
             {
                 crc_res = ( crc_res >> 1 ) ^ 0xa001;
             }
             else
             {
                 crc_res = ( crc_res >> 1 );
             }
         }
     }
     return crc_res;
}