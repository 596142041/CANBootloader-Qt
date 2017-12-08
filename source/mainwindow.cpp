#include "mainwindow.h"
#include "ui_mainwindow_ch.h"
#include <QDebug>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
   int ret;
    VCI_BOARD_INFO1 vci;
    ui->setupUi(this);
    ui->cmdListTableWidget->setColumnWidth(0,180);
    ui->cmdListTableWidget->setColumnWidth(1,180);
    ui->nodeListTableWidget->setColumnWidth(0,100);
    ui->nodeListTableWidget->setColumnWidth(1,100);
    ui->nodeListTableWidget->setColumnWidth(2,120);
    ui->nodeListTableWidget->setColumnWidth(3,200);
    for(int i=0;i<ui->cmdListTableWidget->rowCount();i++)
    {
        ui->cmdListTableWidget->setRowHeight(i,35);
    }
    //检测是否有CAN连接
    ret = VCI_FindUsbDevice(&vci);
    if(ret <= 0)
    {
        USB_CAN_status = 1;
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
    ui->baudRateComboBox->setCurrentIndex(2);
    ui->action_Open_CAN->setEnabled(true);
    ui->action_Close_CAN->setEnabled(false);
    ui->scanNodeAction->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_openFirmwareFilePushButton_clicked()
{
    QString fileName;
    fileName=QFileDialog::getOpenFileName(this,tr("Open files"),"","Hex Files (*.hex);;Binary Files (*.bin);;All Files (*.*)");
    if(fileName.isNull())
    {
        return;
    }
    ui->firmwareLineEdit->setText(fileName);
#if DEBUG
    qDebug()<<"uui->nodeListTableWidget->rowCount() = "<<ui->nodeListTableWidget->rowCount();
    qDebug()<<"ui->nodeListTableWidget->columnCount() = "<<ui->nodeListTableWidget->currentColumn();
#endif
}

int MainWindow::CAN_GetBaudRateNum(unsigned int BaudRate)
{

    for(int i=0;i<27;i++)
    {
        if(BaudRate == CANBus_Baudrate_table[i].BaudRate)
        {
            return i;
        }
    }
    return 0;
}

void MainWindow::on_updateFirmwarePushButton_clicked()
{
    QTime time;
     QString warning_str;
    unsigned char current_chip_model = 0;
    unsigned char file_type          = File_None;
    time.start();
    int ret;
    Device_INFO  DEVICE_INFO;
    DEVICE_INFO.FW_Version      =  (uint32_t)0x00;
    DEVICE_INFO.FW_TYPE.all     =  (uint32_t)0x00;
    DEVICE_INFO.Device_addr.all =  (uint32_t)0x00;
    if(ui->allNodeCheckBox->isChecked())//选中所有节点复选框
    {
        if(ui->nodeListTableWidget->rowCount()<=0)
        {
            QMessageBox::warning(this,
                                 QStringLiteral("警告"),
                                 QStringLiteral("无任何节点！")
                                 );
            return;
        }
    }
    else
    {
        if(ui->nodeListTableWidget->currentIndex().row()<0)
        {
            QMessageBox::warning(this,
                                 QStringLiteral("警告"),
                                 QStringLiteral("请选择节点！")
                                 );
            return;
         }
    }
    uint16_t NodeAddr  = (uint16_t)0x00;
    if(ui->allNodeCheckBox->isChecked())//选中所有节点复选框
    {
        /******************************************************************************************************************
         * 1，由于CAN总线是广播传输，所以在实际使用的时候是可以进行多节点同时升级的，比如可以将0地址设置为广播地址，也就是当命令地址为0的时候，
         * 每个节点收到命令之后都应该执行该命令，但是由于同一时刻，CAN总线上不能传输多个节点的数据，所以这些从节点再执行0地址命令的时候可以不
         * 用返回状态，所以这样做在实际使用的时候，若某个节点的某个命令执行出了问题，主节点缺无法立即知道，因此就需要额外的方式来判断升级是否
         * 成功，比如可以通过升级完毕之后，通过获取每个节点的固件信息来判断。
         *****************************************************************************************************************/
        int row_count = 0;
        row_count = ui->nodeListTableWidget->rowCount();
        for(int i = 0; i <row_count;i++)
            {
                if(ui->nodeListTableWidget->item(i,2)->text() != "BOOT")
                    {
                        NodeAddr = ui->nodeListTableWidget->item(i,0)->text().toInt(NULL,16);
                        ret = CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),
                                            ui->channelIndexComboBox->currentIndex(),
                                            NodeAddr,
                                            CAN_BL_BOOT);
                        if(ret != CAN_SUCCESS)
                        {
                            QMessageBox::warning(this,
                                                 QStringLiteral("警告"),
                                                 QStringLiteral("执行固件程序失败！")
                                                 );
                            return;
                        }
                        Sleep(500);
                    }
            }
    }
    else
    {
        NodeAddr = ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),0)->text().toInt(NULL,16);
        if(ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),3)->text().startsWith("STM32") == true)
            {
                current_chip_model = STM32_SER;
                #if DEBUG
                    qDebug()<<"当前选中芯片为ST的STM32芯片";
                #endif
            }
        else if(ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),3)->text().startsWith("TMS320") == true)
            {
                current_chip_model = TMS320_SER;
                #if DEBUG
                 qDebug()<<"当前选中芯片为TI的DSP芯片";
                #endif
            }
        #if DEBUG
         qDebug()<<"节点地址 = "<<NodeAddr;
        #endif
         //判断当前选中的文件类型是否正确,同时判断文件类型,用于区分后续的发送文件的方式及其格式
         switch (current_chip_model)
         {
             case STM32_SER:
                 {
                 //当前选中的是STM32系列芯片,烧写文件可为bin和hex文件,需要判断选中的文件类型;
                 if((ui->firmwareLineEdit->text().endsWith("bin") == false)&&\
                    (ui->firmwareLineEdit->text().endsWith("hex") == false))
                     {
                         QMessageBox::warning(this,
                                              QStringLiteral("警告"),
                                              QStringLiteral("选中文件类型错误,当前芯片为STM32系列芯片,请选hex文件或者bin文件")
                                              );
                         return;
                     }
                 else if(ui->firmwareLineEdit->text().endsWith("bin") == true)
                     {
                         file_type = File_bin;
#if DEBUG
            qDebug()<<"当前芯片为STM32系列芯片,文件类型为bin文件 file_type = "<<file_type;
#endif

                     }
                  else if(ui->firmwareLineEdit->text().endsWith("hex") == true)
                     {
                          file_type = File_hex;
#if DEBUG
            qDebug()<<"当前芯片为STM32系列芯片,文件类型为hex文件 file_type ="<<file_type;
#endif
                     }
                    else
                     {
                         QMessageBox::warning(this,
                                              QStringLiteral("警告"),
                                              QStringLiteral("选中文件类型错误,当前芯片为STM32系列芯片,文件类型未知")
                                              );
                         return;
                     }
                 }
                 break;
              case TMS320_SER:
                 {
                     //当前选中的是为TI的C2000系列芯,烧写文件hex文件,需要判断选中的文件类型;
                     if((ui->firmwareLineEdit->text().endsWith("hex") == false))
                         {
                             QMessageBox::warning(this,
                                                  QStringLiteral("警告"),
                                                  QStringLiteral("选中文件类型错误,当前芯片为TI的C2000系列芯片,请选hex文件")
                                                  );
                             return;
                         }
                     else if(ui->firmwareLineEdit->text().endsWith("hex") == true)
                        {
                             file_type = File_hex;
    #if DEBUG
                qDebug()<<"当前芯片为TI的C2000系列芯片,文件类型为hex文件 file_type ="<<file_type;
    #endif
                        }
                     else
                        {
                          QMessageBox::warning(this,
                                               QStringLiteral("警告"),
                                               QStringLiteral("选中文件类型错误,当前芯片为TI的C2000系列芯片,文件类型未知")
                                               );
                          return;
                        }
                 }
                 break;
             default:
                 QMessageBox::warning(this,
                                      QStringLiteral("警告"),
                                      QStringLiteral("芯片类型未知"));
                 return;
                 break;
             }
        ret = CAN_BL_Nodecheck(ui->deviceIndexComboBox->currentIndex(),
                               ui->channelIndexComboBox->currentIndex(),
                               NodeAddr,
                               &DEVICE_INFO.FW_Version,
                               &DEVICE_INFO.FW_TYPE.all,
                               120);
        if(ret == CAN_SUCCESS)
        {
            if(DEVICE_INFO.FW_TYPE.bits.FW_TYPE != CAN_BL_BOOT){//当前固件不为Bootloader
                ret = CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),
                                    ui->channelIndexComboBox->currentIndex(),
                                    NodeAddr,
                                    CAN_BL_BOOT);
                if(ret != CAN_SUCCESS)
                   {
                        QMessageBox::warning(this,
                                             QStringLiteral("警告"),
                                             QStringLiteral("执行固件程序失败！")
                                             );
                        return;
                    }
                Sleep(500);
            }
        }
        else
        {
            QMessageBox::warning(this,
                                 QStringLiteral("警告"),
                                 QStringLiteral("节点检测失败-1！")
                                 );
            return;
        }
    }
    /***********************************************************************************************
    *此处是准备发送数据给下位机
    ***********************************************************************************************/
    QFile firmwareFile(ui->firmwareLineEdit->text());
    if (firmwareFile.open(QFile::ReadOnly))
    {
        QProgressDialog writeDataProcess(QStringLiteral("正在更新固件..."),
                                         QStringLiteral("取消"),
                                         0,
                                         firmwareFile.size(),
                                         this);
        writeDataProcess.setWindowTitle(QStringLiteral("更新固件"));
        if(!ui->allNodeCheckBox->isChecked())
        {
            #if DEBUG
              qDebug()<<"NodeAddr = "<<NodeAddr;
            #endif
            ret = CAN_BL_Nodecheck(ui->deviceIndexComboBox->currentIndex(),
                                   ui->channelIndexComboBox->currentIndex(),
                                   NodeAddr,
                                   &DEVICE_INFO.FW_Version,
                                   &DEVICE_INFO.FW_TYPE.all,
                                   100);
            #if DEBUG
             qDebug()<<"ret = "<<ret<<"CAN_SUCCESS = "<<CAN_SUCCESS;
            #endif
            if(ret== CAN_SUCCESS)
            {
                if(DEVICE_INFO.FW_TYPE.bits.FW_TYPE != CAN_BL_BOOT)
                {
                    QMessageBox::warning(this,
                                         QStringLiteral("警告"),
                                         QStringLiteral("当前固件不为Bootloader固件！")
                                         );
                    return;
                }
            }
            else
            {
                QMessageBox::warning(this,
                                     QStringLiteral("警告"),
                                     QStringLiteral("节点检测失败-2！")
                                     );
                return;
            }
        }
        unsigned int erase_timeout_temp = 0;
        qint64 size_temp = 0;
        PACK_INFO pack_info;
        SEND_INFO send_data;
        int status         = CAN_SUCCESS;
        send_data.pack_cnt = 0;
        int file_size      = 0 ;
        char hex_buf[128];
        char bin_buf[1028];
        Data_clear((char *)send_data.data,1028);
        if(file_type == File_hex)
        {
            //此处的擦除超时时间需要注意,针对STM32和DSP芯片计算不一样
            if(current_chip_model == 2)//此处是计算DSP的擦除超时时间
                {
                    erase_timeout_temp = firmwareFile.size()/0x10000;
                    if(firmwareFile.size()%0x10000 != 0)
                        {
                           erase_timeout_temp = erase_timeout_temp+1;
                        }
                    else
                        {
                            erase_timeout_temp = erase_timeout_temp;
                        }
                    if(erase_timeout_temp >5||erase_timeout_temp == 5)
                        {
                            erase_timeout_temp = 5;
                        }
                    erase_timeout_temp = erase_timeout_temp*2000;
                    size_temp = firmwareFile.size();
                }
            else if(current_chip_model == 1)//此处是计算STM32擦除超时时间
                {
                    unsigned long int line_cnt_temp = 0;
                    size_temp = firmwareFile.size()-28;//减掉文件头和文件尾的数据长度
                    line_cnt_temp = size_temp%45;//此处计算hex有多少行
                    if(line_cnt_temp == 0)
                        {
                            line_cnt_temp = ( unsigned long int)(size_temp/45)+1;
                        }
                    else if(line_cnt_temp != 0)
                        {
                             line_cnt_temp = ( unsigned long int)(size_temp/45)+2;
                        }
                    size_temp = line_cnt_temp*16;//近似计算hex真实的文件长度
                    if(size_temp <= 64*1024)
                        {
                            if(size_temp%0xFFFF == 0)
                                {
                                    erase_timeout_temp = size_temp/0xFFFF;
                                }
                            else
                                {
                                   erase_timeout_temp  = (size_temp/0xFFFF)+1;
                                }
                            erase_timeout_temp         = 550*erase_timeout_temp;
                        }
                    else if((size_temp>64*1024)&&(size_temp<=128*1024))
                        {
                            erase_timeout_temp =4*550+1100;//都表示超时时间为毫秒
                        }
                    else if((size_temp>128*1024)&&(size_temp<=850*1024))
                        {
                            if((size_temp-128*1024)%0x20000 == 0)
                                {
                                    erase_timeout_temp = size_temp/0x20000;
                                }
                            else
                                {
                                   erase_timeout_temp   = (size_temp/0x20000)+1;
                                }
                            erase_timeout_temp          = 2000*erase_timeout_temp+4*550+1100;
                        }
                    else if(size_temp>850*1024)
                        {
                            QMessageBox::warning(this,
                                                 QStringLiteral("警告"),
                                                 QStringLiteral("文件过大,超过1M")
                                                 );
                            return;
                        }
                    else
                        {
                            QMessageBox::warning(this,
                                                 QStringLiteral("警告"),
                                                 QStringLiteral("文件大小未知")
                                                 );
                            return;
                        }
                }
            //超时时间计算结束
#if DEBUG
            qDebug()<<"erase_timeout_temp = "<<erase_timeout_temp;
#endif
            status = CAN_BL_erase(ui->deviceIndexComboBox->currentIndex(),
                           ui->channelIndexComboBox->currentIndex(),
                           NodeAddr,
                           size_temp,
                           erase_timeout_temp,
                           File_hex);
            if(status != CAN_SUCCESS)
            {
                qDebug()<<"ret = "<<ret;
                QMessageBox::warning(this,
                                     QStringLiteral("警告"),
                                     QStringLiteral("擦出Flash失败！")
                                     );
                return;
            }
            if(ui->allNodeCheckBox->isChecked())
            {
                Sleep(1000);
            }
            writeDataProcess.setModal(true);
            writeDataProcess.show();
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        //--------------------------------------------------------//
            writeDataProcess.setValue(0);
            send_data.read_start_flag = 0;
            send_data.data_cnt  = 0;
            send_data.data_addr = 0x00;
            send_data.data_len  = 0;
            send_data.line_cnt  = 0;
            if(current_chip_model == STM32_SER)
                {
                     send_data.line_num  = 32;
                }
            else if(current_chip_model == TMS320_SER)
                {
                     send_data.line_num  = 32;
                }
            else
                {
                     send_data.line_num  = 16;
                }
#if DEBUG
            qint64 test = 0xFF;
#endif
            ret = firmwareFile.seek(0);//移动文件指针到文件头
            if(ret == 0)
            {
                #if DEBUG
                    qDebug() << "firmwareFile.seek 失败";
                #endif
                return;
            }
            while(file_size <firmwareFile.size())//file_size
             {
                 firmwareFile.readLine((char*)hex_buf,10);
                 if(hex_buf[0] == ':')//表示是起始标志,判断刚才读取的数据中的第一个字节是否是起始标志
                     {
                        #if DEBUG
                         qDebug() << "hex_buf  = "<<hex_buf ;
                        #endif
                       hex_to_bin(&hex_buf[1],bin_buf,8);//将读取的9个字节后面8字节由ASC_II转换为hex(16进制数据)
                       pack_info.data_type = bin_buf[6]<<4|bin_buf[7];
                        #if DEBUG
                       switch (pack_info.data_type)
                           {
                           case DATA_BASE_ADDR: qDebug() << " DATA_BASE_ADDR";
                               break;
                           case DATA_Rrecord:   qDebug() << " DATA_Rrecord";
                               break;
                           case DATA_END:       qDebug() << " DATA_END";
                                 break;
                           default:
                               break;
                           }
                        #endif
                       //---------------------------------------------------------
                       if(send_data.read_start_flag == 0)//如果该标志位为0,表示这是第一次读取数据,此时将标志位置一
                           {
                               send_data.read_start_flag = 1;
                               if(current_chip_model == STM32_SER)
                                   {
                                        send_data.line_num  = 32;
                                   }
                               else if(current_chip_model == TMS320_SER)
                                   {
                                        send_data.line_num  = 32;
                                   }
                               else
                                   {
                                        send_data.line_num  = 16;
                                   }
                               send_data.line_cnt = 0;
                               send_data.data_cnt = 0;
                               send_data.data_len = 0;
                           }
                       else
                           {
                               if(pack_info.data_type == DATA_BASE_ADDR||pack_info.data_type == DATA_END)//判断该行的数据是,如果是表示基地址
                               {
                                 status =  CAN_BL_write(ui->deviceIndexComboBox->currentIndex(),
                                                        ui->channelIndexComboBox->currentIndex(),
                                                        NodeAddr,
                                                        &send_data,
                                                        90);
                                   if(status != 0x00)
                                   {
                                           warning_str.sprintf("故障代码是%d",status);
                                       switch(status)
                                       {
                                           case CAN_ERR_NOT_SUPPORT:    warning_str = warning_str+"适配器不支持该函数";break;
                                           case CAN_ERR_USB_WRITE_FAIL: warning_str = warning_str+"USB写数据失败";    break;
                                           case CAN_ERR_USB_READ_FAIL:  warning_str = warning_str+"USB读数据失败";    break;
                                           case CAN_ERR_CMD_FAIL:       warning_str = warning_str+"命令执行失败";     break;
                                           case CAN_BL_ERR_CONFIG:      warning_str = warning_str+ "配置设备错误";    break;
                                           case CAN_BL_ERR_SEND:        warning_str = warning_str+"发送数据出错";     break;
                                           case CAN_BL_ERR_TIME_OUT:    warning_str = warning_str+"超时错误";        break;
                                           case CAN_BL_ERR_CMD:         warning_str = warning_str+ "执行命令失败";   break;
                                           default:warning_str = "";break;
                                       }
                                      QMessageBox::warning(this,
                                                           QStringLiteral("警告"),
                                                           warning_str
                                                           );
                                     #if DEBUG
                                       qDebug() << " write faile-1"<<"status = "<<status<<"  send_data.pack_cnt = "<< send_data.pack_cnt;
                                     #endif
                                       return ;
                                   }
                                   send_data.pack_cnt++;
                                   status               = 0xFF;
                                   send_data.data_len   = 0;
                                   send_data.data_cnt   = 0;
                                   send_data.data_addr  = 0x00;
                                   send_data.line_cnt   = 0;
                                   for(int i = 0;i < 1028;i++)
                                   {
                                       send_data.data[i] = 0x00;
                                   }
                               }
                               else if(send_data.line_cnt == send_data.line_num)//到了指定的行数进行数据发送
                               {
                                   status =  CAN_BL_write(ui->deviceIndexComboBox->currentIndex(),
                                                          ui->channelIndexComboBox->currentIndex(),
                                                          NodeAddr,&send_data,
                                                          90);
                                     if(status != 0x00)
                                     {
                                         warning_str.sprintf("故障代码是%d",status);
                                         switch(status)
                                         {
                                             case CAN_ERR_NOT_SUPPORT:    warning_str = warning_str+"适配器不支持该函数";break;
                                             case CAN_ERR_USB_WRITE_FAIL: warning_str = warning_str+"USB写数据失败";    break;
                                             case CAN_ERR_USB_READ_FAIL:  warning_str = warning_str+"USB读数据失败";    break;
                                             case CAN_ERR_CMD_FAIL:       warning_str = warning_str+"命令执行失败";     break;
                                             case CAN_BL_ERR_CONFIG:      warning_str = warning_str+"配置设备错误";     break;
                                             case CAN_BL_ERR_SEND:        warning_str = warning_str+"发送数据出错";     break;
                                             case CAN_BL_ERR_TIME_OUT:    warning_str = warning_str+"超时错误";        break;
                                             case CAN_BL_ERR_CMD:         warning_str = warning_str+"执行命令失败";     break;
                                             default:warning_str = "";break;
                                          }
                                          QMessageBox::warning(this,
                                                               QStringLiteral("警告"),
                                                               warning_str
                                                               );
                                        #if DEBUG
                                          qDebug() << " write faile-2"<<"status = "<<status<<" send_data.pack_cnt = "<< send_data.pack_cnt;
                                        #endif
                                         return ;
                                     }
                                   send_data.pack_cnt++;
                                   status              = 0xFF;
                                   send_data.data_len  = 0;
                                   send_data.data_cnt  = 0;
                                   send_data.data_addr = 0x00;
                                   send_data.line_cnt  = 0;
                                   for(int i = 0;i < 1028;i++)
                                   {
                                       send_data.data[i] = 0x00;
                                   }
                               }
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
                           firmwareFile.readLine((char*)hex_buf,(pack_info.data_len*2+3+1));
                            #if DEBUG
                             qDebug() << "hex_buf is = "<<hex_buf;
                            #endif
                           hex_to_bin(&hex_buf[0],bin_buf,pack_info.data_len*2);//将读取的数据转换为hex;
                           if(pack_info.data_type == DATA_BASE_ADDR)
                           {
                             pack_info.data_base_addr = bin_buf[0]<<12|bin_buf[1]<<8|bin_buf[2]<<4|bin_buf[3];
                             pack_info.data_base_addr = pack_info.data_base_addr<<16;
                           }
                           else if(pack_info.data_type == DATA_Rrecord)
                           {
                                 pack_info.data_addr = pack_info.data_base_addr+pack_info.data_addr_offset;//表示该行数据应该写入的真实地址
                                 for( int i = 0;i <pack_info.data_len;i++)
                                 {
                                    pack_info.Data[i] = bin_buf[i*2]<<4|bin_buf[2*i+1];
                                    #if DEBUG
                                     qDebug() << "pack_info.Data["<<i<<"]="<<pack_info.Data[i];
                                    #endif
                                 }
                          }
                           if(pack_info.data_type == DATA_Rrecord)
                               {
                                   if(send_data.line_cnt == 0)//如果计数器还为0,表示还是第一次读取,因此需要更新写入数据的地址
                                   {
                                       send_data.data_addr = pack_info.data_addr;//将第一行的数据地址作为该数据包的写入地址
                                   }
                                   //以下是将刚才读取的数据写入send_data.data数组中
                                   for(int i = 0;i < pack_info.data_len;i++)
                                   {
                                       send_data.data_cnt                   = i;
                                       send_data.data[i+send_data.data_len] = pack_info.Data[i];
                                   }
                                   send_data.data_cnt = pack_info.data_len;
                                   send_data.data_len = send_data.data_len+send_data.data_cnt;
                                   send_data.line_cnt++;
                               }
                         Data_clear(hex_buf,128);
                         Data_clear(bin_buf,1028);
                         Data_clear_int(&pack_info.Data[0],64);
                         #if DEBUG
                            test = firmwareFile.pos();
                            qDebug() << "test = "<<test;
                         #endif
                         firmwareFile.seek(firmwareFile.pos()+1);
                         #if DEBUG
                            test = firmwareFile.pos();
                            qDebug() << "test = "<<test;
                         #endif
                     }
                 file_size = firmwareFile.pos();
                #if DEBUG
                 qDebug() << "file_size = "<<file_size;
                #endif
                 writeDataProcess.setValue(file_size);
                 QCoreApplication::processEvents(QEventLoop::AllEvents);
                 if(writeDataProcess.wasCanceled())
                 {
                     return;
                 }
                 if(ui->allNodeCheckBox->isChecked())
                 {
                     Sleep(10);
                 }
             }
        //----------------------------------------------------------//
            writeDataProcess.setValue(firmwareFile.size());
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            if(writeDataProcess.wasCanceled())
            {
                return;
            }
        }
        else if(file_type == File_bin)
        {
            file_size = 0;
            send_data.pack_size = 1024;
            //此处是计算擦除超时时间
            size_temp = firmwareFile.size();
            if(size_temp <= 64*1024)
            {
                if(size_temp%0xFFFF == 0)
                {
                    erase_timeout_temp = size_temp/0xFFFF;
                }
                else
                {
                   erase_timeout_temp = (size_temp/0xFFFF)+1;
                }
                erase_timeout_temp    = 550*erase_timeout_temp;
            }
            else if((size_temp>64*1024)&&(size_temp<=128*1024))
            {
                erase_timeout_temp =4*550+1000;//都表示超时时间为毫秒
            }
            else if((size_temp>128*1024)&&(size_temp<=850*1024))
            {
                if((size_temp-128*1024)%0x20000 == 0)
                {
                    erase_timeout_temp = size_temp/0x20000;
                }
                else
                {
                   erase_timeout_temp = (size_temp/0x20000)+1;
                }
                    erase_timeout_temp = 2000*erase_timeout_temp+4*550+1000;
            }
            else if(size_temp>850*1024)
            {
                QMessageBox::warning(this,
                                     QStringLiteral("警告"),
                                     QStringLiteral("文件过大,超过1M")
                                     );
                return;
            }
            else
            {
                QMessageBox::warning(this,
                                     QStringLiteral("警告"),
                                     QStringLiteral("文件大小未知")
                                     );
                return;
            }
            //开始擦除文件命令
              status = CAN_BL_erase(ui->deviceIndexComboBox->currentIndex(),
                                    ui->channelIndexComboBox->currentIndex(),
                                    NodeAddr,
                                    firmwareFile.size(),
                                    erase_timeout_temp,
                                    File_bin
                                    );
#ifdef DEBUG
               qDebug()<<"erase_timeout_temp = "<<erase_timeout_temp<<"firmwareFile.size() = "<<firmwareFile.size();
#endif
            if(status != CAN_SUCCESS)
            {
                warning_str.sprintf("故障代码是%d",status);
                warning_str = warning_str+"擦出Flash失败！";
#ifdef DEBUG
                qDebug()<<"status = "<<status;
#endif
                QMessageBox::warning(this,
                                     QStringLiteral("警告"),
                                     warning_str
                                     );
                return;
            }
            if(ui->allNodeCheckBox->isChecked())
            {
                Sleep(1000);
            }
            //------擦除文件命令结束

            //准备写入数据
            writeDataProcess.setModal(true);
            writeDataProcess.show();
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            file_size = 0;
           while(file_size < firmwareFile.size())
            {
                send_data.data_len = firmwareFile.read((char*)send_data.data,send_data.pack_size);
#if DEBUG
                qDebug()<<"send_data.data_len = "<<send_data.data_len;
#endif
                send_data.data_addr = file_size;
                status =  CAN_BL_write(ui->deviceIndexComboBox->currentIndex(),
                                       ui->channelIndexComboBox->currentIndex(),
                                       NodeAddr,
                                       &send_data,
                                       90);
                if(status != CAN_SUCCESS)
                {
                    warning_str.sprintf("故障代码是%d",status);
                    switch(status)
                                {
                                    case CAN_ERR_NOT_SUPPORT:    warning_str = warning_str+"适配器不支持该函数";break;
                                    case CAN_ERR_USB_WRITE_FAIL: warning_str = warning_str+"USB写数据失败";    break;
                                    case CAN_ERR_USB_READ_FAIL:  warning_str = warning_str+"USB读数据失败";    break;
                                    case CAN_ERR_CMD_FAIL:       warning_str = warning_str+"命令执行失败";     break;
                                    case CAN_BL_ERR_CONFIG:      warning_str = warning_str+ "配置设备错误";    break;
                                    case CAN_BL_ERR_SEND:        warning_str = warning_str+"发送数据出错";     break;
                                    case CAN_BL_ERR_TIME_OUT:    warning_str = warning_str+"超时错误";        break;
                                    case CAN_BL_ERR_CMD:         warning_str = warning_str+ "执行命令失败";   break;
                                    default:warning_str = "";break;
                                }
                        QMessageBox::warning(this,
                                             QStringLiteral("警告"),
                                             warning_str
                                             );
                    #if DEBUG
                      qDebug() << " write bin file faile "<<"status = "<<status;
                    #endif
                     return ;
                }
                file_size =file_size+ send_data.data_len;
#if DEBUG
                qDebug()<<"file_size= "<<file_size;
#endif
                writeDataProcess.setValue(file_size);
                QCoreApplication::processEvents(QEventLoop::AllEvents);
                if(writeDataProcess.wasCanceled())
                {
                    return;
                }
                if(ui->allNodeCheckBox->isChecked())
                {
                    #ifndef OS_UNIX
                        Sleep(10);
                    #else
                        usleep(10*1000);
                    #endif
                }

            }
            writeDataProcess.setValue(firmwareFile.size());
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            if(writeDataProcess.wasCanceled())
            {
                return;
            }
        }
        else
            {
                QMessageBox::warning(this,
                                     QStringLiteral("警告"),
                                     QStringLiteral("文件类型错误！")
                                     );
                return;
            }
    }
    else
    {
        QMessageBox::warning(this,
                             QStringLiteral("警告"),
                             QStringLiteral("打开固件文件失败！")
                             );
        return;
    }
   firmwareFile.close();
    //执行固件
    ret = CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),NodeAddr,CAN_BL_APP);
    if(ret != CAN_SUCCESS)
    {
        warning_str.sprintf("故障代码是%d",ret);
        warning_str = warning_str+"执行固件程序失败!";
        QMessageBox::warning(this,
                             QStringLiteral("警告"),
                              warning_str
                             );
    }
    Sleep(50);
    if(!ui->allNodeCheckBox->isChecked())
    {
         ret = CAN_BL_Nodecheck(ui->deviceIndexComboBox->currentIndex(),
                                ui->channelIndexComboBox->currentIndex(),
                                NodeAddr,
                                &DEVICE_INFO.FW_Version,
                                &DEVICE_INFO.FW_TYPE.all,
                                100);
        if(ret == CAN_SUCCESS)
        {
            QString str;
            if(DEVICE_INFO.FW_TYPE.bits.FW_TYPE == CAN_BL_BOOT)
            {
                str = "BOOT";
            }else
            {
                str = "APP";
            }
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),1)->setText(str);
            str.sprintf("v%d.%d",
                        (((DEVICE_INFO.FW_Version>>24)&0xFF)*10)|((DEVICE_INFO.FW_Version>>16)&0xFF),
                        (((DEVICE_INFO.FW_Version>>8)&0xFF)*10)|(DEVICE_INFO.FW_Version&0xFF)
                       );
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),2)->setText(str);
            //--------------------添加烧写时间长度
            str.sprintf("用时%2.3f 秒",time.elapsed()/1000.0);
            QMessageBox::warning(this,
                                 QStringLiteral("时长"),
                                 str
                                 );
        }
        else
        {
            warning_str.sprintf("故障代码是:%d",ret);
            warning_str = warning_str+"执行固件程序失败-1";
            QMessageBox::warning(this,
                                 QStringLiteral("警告"),
                                 warning_str
                                 );
        }
    }
    #if  DEBUG
      qDebug()<<time.elapsed()/1000.0<<"秒";
    #endif
}

void MainWindow::on_openFirmwareFileAction_triggered()
{
    on_openFirmwareFilePushButton_clicked();
}

void MainWindow::on_scanNodeAction_triggered()
{
    int ret;
    Device_INFO  DEVICE_INFO;
    DEVICE_INFO.Device_addr.all = (uint32_t)0x00;
    DEVICE_INFO.FW_TYPE.all     =  (uint32_t)0x00;
    DEVICE_INFO.FW_Version      =  (uint32_t)0x00;
    int startAddr = 0,endAddr   = 0;
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
          QMessageBox::warning(this,
                               QStringLiteral("警告"),
                               QStringLiteral("打开设备失败！")
                               );
          return ;
    }
    ui->nodeListTableWidget->verticalHeader()->hide();
    ui->nodeListTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->nodeListTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->nodeListTableWidget->setRowCount(0);
    QProgressDialog scanNodeProcess(QStringLiteral("正在扫描节点..."),
                                    QStringLiteral("取消"),
                                    0,
                                    endAddr-startAddr,
                                    this
                                    );
    scanNodeProcess.setWindowTitle(QStringLiteral("扫描节点"));
    scanNodeProcess.setModal(true);
    scanNodeProcess.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    int i=0;
    while(startAddr <= endAddr)
    {
        i++;
        ret = CAN_BL_Nodecheck(ui->deviceIndexComboBox->currentIndex(),
                               ui->channelIndexComboBox->currentIndex(),
                               startAddr,
                               &DEVICE_INFO.FW_Version,
                               &DEVICE_INFO.FW_TYPE.all,
                               60);
        if(ret == CAN_SUCCESS)
        {
            ui->nodeListTableWidget->setRowCount(ui->nodeListTableWidget->rowCount()+1);
            ui->nodeListTableWidget->setRowHeight(ui->nodeListTableWidget->rowCount()-1,20);
            QString str;
            str.sprintf("0x%X",startAddr);
            QTableWidgetItem *item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,0,item);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->rowCount()-1,0)->setTextAlignment(Qt::AlignHCenter);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->rowCount()-1,0)->setTextAlignment(Qt::AlignCenter);
            //--------------------------------------------------------------------------------------------------------
            str.sprintf("0x%X",DEVICE_INFO.FW_TYPE.all);
            #if DEBUG
            qDebug()<<"DEVICE_INFO.FW_TYPE.all = "<<str;
            #endif
            if(DEVICE_INFO.FW_TYPE.bits.FW_TYPE == CAN_BL_BOOT)
            {
                str = "BOOT";
            }
            else
            {
                str = "APP";
            }
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,1,item);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->rowCount()-1,1)->setTextAlignment(Qt::AlignHCenter);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->rowCount()-1,1)->setTextAlignment(Qt::AlignCenter);
            //-----------------------------------------------------------------------------------------------------------------
            str.sprintf("v%d.%d",(((DEVICE_INFO.FW_Version>>24)&0xFF)*10)|((DEVICE_INFO.FW_Version>>16)&0xFF),(((DEVICE_INFO.FW_Version>>8)&0xFF)*10)|(DEVICE_INFO.FW_Version&0xFF));
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,2,item);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->rowCount()-1,2)->setTextAlignment(Qt::AlignHCenter);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->rowCount()-1,2)->setTextAlignment(Qt::AlignCenter);
            //------------------------------------------------------------------------------------------------------------------
            uint32_t chip_temp = 0x00;
            chip_temp = DEVICE_INFO.FW_TYPE.bits.Chip_Value&((uint32_t)0xFF);
            #if DEBUG
            qDebug()<<"DEVICE_INFO.FW_TYPE.bits.Chip_Value = "<<DEVICE_INFO.FW_TYPE.bits.Chip_Value<<"startAddr = "<<startAddr<<"chip_temp = "<<chip_temp;
            #endif
            //此处是对ini配置文件读取
            //用于来判断芯片类型
            //-----------------------------------------------------------------------------
            str = "None";
            QString file_path = QCoreApplication::applicationDirPath()+"/chip.ini";
            QSettings settings(file_path, QSettings::IniFormat);
            settings.beginGroup("chip");
             chip_list = settings.allKeys();
            for (int i = 0; i < chip_list.size(); ++i)
                {
                    if(chip_temp == (uint32_t)(settings.value(chip_list.at(i)).toInt()))
                        {
                            str = chip_list.at(i);
#if DEBUG
                            qDebug()<<"keys.at= "<< chip_list.at(i)<<""<<settings.value(chip_list.at(i)).toInt();
#endif
                            break;
                        }
                    else
                        {
                            str = "None";
                        }
                }
            //---------------------------------------------------------------------------------
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,3,item);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->rowCount()-1,3)->setTextAlignment(Qt::AlignHCenter);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->rowCount()-1,3)->setTextAlignment(Qt::AlignCenter);
        }
        scanNodeProcess.setValue(i);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if(scanNodeProcess.wasCanceled())
        {
            return;
        }
        startAddr++;
    }
}

void MainWindow::on_Fun_test_clicked()
{
    int status = 0;
    SEND_INFO send_data;
    send_data.read_start_flag = 0;
    send_data.data_cnt  = 0;
    send_data.data_addr = 0x00;
    send_data.data_len  = 0;
    send_data.line_cnt  = 0;
    send_data.line_num  = 16;
#if DEBUG
    ui->progressBar->setValue(0);
#endif
    status = CAN_BL_erase(ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),0x134,0x800,9800,File_hex);
    if(status == 1)
        {
#if DEBUG
            qDebug()<<tr("擦除失败");
#endif
            return ;
        }
#if DEBUG
             qint64 test = 0xFF;
#endif
    bool ret;
    PACK_INFO pack_info;
    int   hex_size = 0;
    char hex_buf[128];
    char bin_buf[1028];
    QFile file(ui->firmwareLineEdit->text());
#if DEBUG
    qDebug() << "file name ;"<<file.fileName();
#endif
    ret = file.open(QFile::ReadOnly);//以只读的方式打开文件
    if(ret == 0)
        {
#if DEBUG
           qDebug() << "open file fals;";
#endif
            return;
        }
    ret = file.seek(0);//移动文件指针到文件头
    if(ret == 0)
        {
#if DEBUG
             qDebug() << "firmwareFile.seek";
#endif
            return;
        }

#if DEBUG
    test = file.pos();
    ui->progressBar->setWindowTitle("文件读取");
    ui->progressBar->setRange(0,file.size());
    qDebug() << "file.size() = "<<file.size();
#endif
    while(hex_size <file.size())
        {
            file.readLine((char*)hex_buf,10);
            if(hex_buf[0] == ':')//表示是起始标志,判断刚才读取的数据中的第一个字节是否是起始标志
                {
#if DEBUG
                  qDebug() << "hex_buf  = "<<hex_buf ;
#endif
                  hex_to_bin(&hex_buf[1],bin_buf,8);//将读取的9个字节后面8字节由ASC_II转换为hex(16进制数据)
                  pack_info.data_type = bin_buf[6]<<4|bin_buf[7];
                  switch (pack_info.data_type)
                      {
                      case DATA_BASE_ADDR:
#if DEBUG
                           qDebug() << " DATA_BASE_ADDR";
#endif
                          break;
                      case DATA_Rrecord:
#if DEBUG
                           qDebug() << " DATA_Rrecord";
#endif
                          break;
                      case DATA_END:
#if DEBUG
                           qDebug() << " DATA_END";
#endif
                      default:
                          break;
                      }
                  //---------------------------------------------------------
                  if(send_data.read_start_flag == 0)//如果该标志位为0,表示这是第一次读取数据,此时将标志位置一
                      {
                          send_data.read_start_flag = 1;
                          send_data.line_num        = 16;
                          send_data.line_cnt        = 0;
                          send_data.data_cnt        = 0;
                          send_data.data_len        = 0;
                      }
                  else
                      {
                          if(pack_info.data_type == DATA_BASE_ADDR||pack_info.data_type == DATA_END)//判断该行的数据是,如果是表示基地址
                          {
                            status =  CAN_BL_write(ui->deviceIndexComboBox->currentIndex(),
                                                   ui->channelIndexComboBox->currentIndex(),
                                                   0x134,
                                                   &send_data,
                                                   90);
                              if(status != 0x00)
                              {
#if DEBUG
                                  qDebug() << " write faile-1";
#endif
                                  return ;
                              }
                              status              = 0xFF;
                              send_data.data_len  = 0;
                              send_data.data_cnt  = 0;
                              send_data.data_addr = 0x00;
                              send_data.line_cnt  = 0;
                              for(int i = 0;i < 1028;i++)
                              {
                                  send_data.data[i] = 0x00;
                              }
                          }
                          else if(send_data.line_cnt == send_data.line_num)//到了指定的行数进行数据发送
                          {
                              status =  CAN_BL_write(ui->deviceIndexComboBox->currentIndex(),
                                                     ui->channelIndexComboBox->currentIndex(),
                                                     0x134,
                                                     &send_data,
                                                     90);
                                if(status != 0x00)
                                {
#if DEBUG
                                        qDebug() << " write faile-2";
#endif
                                    return ;
                                }
                              status = 0xFF;
                              send_data.data_len  = 0;
                              send_data.data_cnt  = 0;
                              send_data.data_addr = 0x00;
                              send_data.line_cnt  = 0;
                              for(int i = 0;i < 1028;i++)
                              {
                                  send_data.data[i] = 0x00;
                              }
                          }
                          else
                          {

                          }
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
#if DEBUG
                      qDebug() << "hex_buf is = "<<hex_buf;
#endif
                      hex_to_bin(&hex_buf[0],bin_buf,pack_info.data_len*2);//将读取的数据转换为hex;
                      if(pack_info.data_type == DATA_BASE_ADDR)
                      {
                        pack_info.data_base_addr = bin_buf[0]<<12|bin_buf[1]<<8|bin_buf[2]<<4|bin_buf[3];
                        pack_info.data_base_addr = pack_info.data_base_addr<<16;
                      }
                      else if(pack_info.data_type == DATA_Rrecord)
                      {
                            pack_info.data_addr = pack_info.data_base_addr+pack_info.data_addr_offset;//表示该行数据应该写入的真实地址
                            for( int i = 0;i <pack_info.data_len;i++)
                            {
                               pack_info.Data[i] = bin_buf[i*2]<<4|bin_buf[2*i+1];
#if DEBUG
                                qDebug() << "pack_info.Data["<<i<<"]="<<pack_info.Data[i];
#endif
                            }

                     }
                      if(pack_info.data_type == DATA_Rrecord)
                          {
                              if(send_data.line_cnt == 0)//如果计数器还为0,表示还是第一次读取,因此需要更新写入数据的地址
                              {
                                  send_data.data_addr = pack_info.data_addr;//将第一行的数据地址作为该数据包的写入地址
                              }
                              //以下是将刚才读取的数据写入send_data.data数组中
                              for(int i = 0;i < pack_info.data_len;i++)
                              {
                                  send_data.data_cnt                   = i;
                                  send_data.data[i+send_data.data_len] = pack_info.Data[i];
                              }
                              send_data.data_cnt = pack_info.data_len;
                              send_data.data_len = send_data.data_len+send_data.data_cnt;
                              send_data.line_cnt++;
                          }
                    Data_clear(hex_buf,128);
                    Data_clear(bin_buf,1028);
                    Data_clear_int(&pack_info.Data[0],64);
#if DEBUG
                    test = file.pos();
                    qDebug() << "test = "<<test;
#endif
                    file.seek(file.pos()+1);
#if DEBUG
                    test = file.pos();
                    qDebug() << "test = "<<test;
#endif
                }

#if DEBUG
             hex_size = file.pos();
            qDebug() << "hex_size = "<<hex_size;
            ui->progressBar->setValue(hex_size);
#endif
        }
     CAN_BL_excute(ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),0x134,CAN_BL_APP);
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
    QString file_path = QCoreApplication::applicationDirPath()+"/chip.ini"; ;
    QSettings settings(file_path, QSettings::IniFormat);
    settings.beginGroup("chip");
     chip_list = settings.allKeys();
#if DEBUG
    qDebug()<<"keys.size() = "<<chip_list.size();
    for (int i = 0; i < chip_list.size(); ++i)
        {
             qDebug()<<"keys.at= "<< chip_list.at(i)<<""<<settings.value(chip_list.at(i)).toInt();
        }
#endif
    state = VCI_OpenDevice(4,ui->deviceIndexComboBox->currentIndex(),0);
    if(!state)
    {
        QMessageBox::warning(this,
                             QStringLiteral("警告"),
                             QStringLiteral("打开设备失败！")
                             );
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

            QMessageBox::warning(this,
                                 QStringLiteral("警告"),
                                 QStringLiteral("无设备连接！")
                                 );
        }
    QString cmdStr[]={"Erase","Write","Check","Excute","WriteInfo","CmdFaild","CmdSuccess","SetBaudRate"};
    uint8_t cmdData[16];
    for(int i=0;i<ui->cmdListTableWidget->rowCount();i++)
    {
        ui->cmdListTableWidget->item(i,0)->setTextAlignment(Qt::AlignHCenter);
        ui->cmdListTableWidget->item(i,0)->setTextAlignment(Qt::AlignCenter);
        ui->cmdListTableWidget->item(i,1)->setTextAlignment(Qt::AlignHCenter);
        ui->cmdListTableWidget->item(i,1)->setTextAlignment(Qt::AlignCenter);
        if(ui->cmdListTableWidget->item(i,0)->text()==cmdStr[i])
        {
            cmdData[i] = ui->cmdListTableWidget->item(i,1)->text().toInt(NULL,16);
        }
    }
    Boot_CMD_LIST CMD_List;
    CMD_List.Erase       = cmdData[0];
    CMD_List.Write       = cmdData[1];
    CMD_List.Check       = cmdData[2];
    CMD_List.Excute      = cmdData[3];
    CMD_List.CmdFaild    = cmdData[5];
    CMD_List.WriteInfo   = cmdData[4];
    CMD_List.CmdSuccess  = cmdData[6];
    CMD_List.SetBaudRate = cmdData[7];
    CAN_BL_init(CMD_List);//初始化cmd
    VCI_INIT_CONFIG VCI_init;
     VCI_init.Mode       = 0x00;
    VCI_init.Filter      = 0x01;
    VCI_init.AccCode     = 0x00000000;
    VCI_init.AccMask     = 0xFFFFFFFF;
    VCI_init.Reserved    = 0x00;
    //---------波特率处理
    QString str = ui->baudRateComboBox->currentText();
    str.resize(str.length()-4);
    int baud = str.toInt(NULL,10)*1000;
    VCI_init.Timing0  =  CANBus_Baudrate_table[CAN_GetBaudRateNum(baud)].Timing0;//波特率的配置
    VCI_init.Timing1  =  CANBus_Baudrate_table[CAN_GetBaudRateNum(baud)].Timing1;//波特率的配置
    if(ui->channelIndexComboBox->currentIndex() == 0||(ui->channelIndexComboBox->currentIndex() == 1))
        {
            ret = VCI_InitCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),&VCI_init);
            if(ret!=1)
            {
                QMessageBox::warning(this,
                                     QStringLiteral("警告"),
                                     QStringLiteral("配置设备失败！")
                                     );
                USB_CAN_status = 3;
                return;
            }
            ret = 0;
            ret = VCI_StartCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
            if(ret!=1)
            {
                QMessageBox::warning(this,
                                     QStringLiteral("警告"),
                                     QStringLiteral("配置设备失败 USB_CAN_status = 6 ！")
                                     );
                USB_CAN_status = 3;
                return;
            }
            ui->Connect_USB_CAN->setEnabled(false);
            ui->Close_CAN->setEnabled(true);
            ui->updateFirmwarePushButton->setEnabled(true);
             ui->newBaudRateComboBox->setEnabled(true);
             ui->allNodeCheckBox->setEnabled(true);
             ui->baudRateComboBox->setEnabled(false);
             ui->deviceIndexComboBox->setEnabled(false);
             ui->channelIndexComboBox->setEnabled(false);
             USB_CAN_status = 0x04;
             VCI_ClearBuffer(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
        }
    ui->action_Open_CAN->setEnabled(false);
    ui->action_Close_CAN->setEnabled(true);
    ui->scanNodeAction->setEnabled(true);
}

void MainWindow::on_Close_CAN_clicked()
{
    ui->action_Open_CAN->setEnabled(true);
    ui->action_Close_CAN->setEnabled(false);
    ui->scanNodeAction->setEnabled(false);
    int ret;
    ret = VCI_ResetCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
    if(ret != 1)
        {
            QMessageBox::warning(this,
                                 QStringLiteral("警告"),
                                 QStringLiteral("复位设备失败！")
                                 );
        }
    else
        {
             USB_CAN_status = 0;
        }
    ret = VCI_CloseDevice(4,ui->deviceIndexComboBox->currentIndex());
    if(ret != 1)
        {
            QMessageBox::warning(this,
                                 QStringLiteral("警告"),
                                 QStringLiteral("关闭设备失败！")
                                 );
        }
    else
        {
             USB_CAN_status = 0;
        }
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
ui->action_Open_CAN->setEnabled(false);
ui->action_Close_CAN->setEnabled(true);
ui->action_Open_CAN->setEnabled(false);
ui->action_Close_CAN->setEnabled(true);
ui->scanNodeAction->setEnabled(true);
}

void MainWindow::on_action_Close_CAN_triggered()
{
    on_Close_CAN_clicked();
    ui->action_Open_CAN->setEnabled(true);
    ui->action_Close_CAN->setEnabled(false);
    ui->scanNodeAction->setEnabled(false);
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
     return CAN_ERR_USB_WRITE_FAIL;//USB写数据失败
    }
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
            return  CAN_BL_ERR_TIME_OUT;//超时错误
        }
    else if(read_num == -1)
        {
            return CAN_ERR_USB_READ_FAIL;//USB读数据失败
        }
    else
        {
            ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
            if(ret == -1)
            {
                 return CAN_ERR_USB_READ_FAIL;//USB读数据失败
                VCI_ClearBuffer(4,DevIndex,CANIndex);
            }
            if(ret == 1)
            {
                *pVersion = can_read_msg[0].Data[0]<<24|can_read_msg[0].Data[1]<<16|can_read_msg[0].Data[2]<<8|can_read_msg[0].Data[3]<<0;
                *pType    = can_read_msg[0].Data[4]<<24|can_read_msg[0].Data[5]<<16|can_read_msg[0].Data[6]<<8|can_read_msg[0].Data[7]<<0;
            }
        }

    VCI_ClearBuffer(4,DevIndex,CANIndex);
    return CAN_SUCCESS;
}

int MainWindow::CAN_BL_init(Boot_CMD_LIST pCmdList)
{

    cmd_list.Check      = pCmdList.Check;
    cmd_list.Erase      = pCmdList.Erase;
    cmd_list.Excute     = pCmdList.Excute;
    cmd_list.Write      = pCmdList.Write;
    cmd_list.WriteInfo  = pCmdList.WriteInfo;
    cmd_list.CmdFaild   = pCmdList.CmdFaild;
    cmd_list.CmdSuccess = pCmdList.CmdSuccess;
    return CAN_SUCCESS;
}

int MainWindow::CAN_BL_erase(int DevIndex, int CANIndex, unsigned short NodeAddr, unsigned int FlashSize, unsigned int TimeOut, unsigned char file_type)
{

    int ret;
    bootloader_data Bootloader_data ;
    int i,read_num;
    VCI_CAN_OBJ can_send_msg,can_read_msg[1000];
    Bootloader_data.DLC               = 5;
    Bootloader_data.ExtId.bit.cmd     = cmd_list.Erase;
    Bootloader_data.ExtId.bit.addr    = NodeAddr;
    Bootloader_data.ExtId.bit.reserve = 0;
    Bootloader_data.IDE               = CAN_ID_EXT;
    Bootloader_data.data[0]           = ( FlashSize & 0xFF000000 ) >> 24;
    Bootloader_data.data[1]           = ( FlashSize & 0xFF0000 ) >> 16;
    Bootloader_data.data[2]           = ( FlashSize & 0xFF00 ) >> 8;
    Bootloader_data.data[3]           = ( FlashSize & 0x00FF );
    Bootloader_data.data[4]           = file_type;
    can_send_msg.DataLen              = Bootloader_data.DLC;
    can_send_msg.SendType             = 1;
    can_send_msg.RemoteFlag           = 0;
    can_send_msg.ExternFlag           = Bootloader_data.IDE;
    can_send_msg.ID                   = Bootloader_data.ExtId.all;
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    for(i = 0;i<Bootloader_data.DLC;i++)
        {
            can_send_msg.Data[i] = Bootloader_data.data[i];
        }
    ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
    if(ret == -1)
    {
     return CAN_ERR_USB_WRITE_FAIL;
    }
    DWORD current_time              = 0;
    DWORD new_time                  = 0;
    current_time                    = GetTickCount();
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
            return CAN_BL_ERR_TIME_OUT;
        }
    else if(read_num == -1)
        {
            return CAN_ERR_USB_READ_FAIL;
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
                if(can_read_msg[0].ID != (UINT)(Bootloader_data.ExtId.bit.addr<<4|cmd_list.CmdSuccess))//表示反馈数据有效
                    {
                        return CAN_BL_ERR_CMD;
                    }

            }
        }

    VCI_ClearBuffer(4,DevIndex,CANIndex);
    return CAN_SUCCESS;
}

int MainWindow::CAN_BL_write(int DevIndex,int CANIndex,unsigned short NodeAddr,SEND_INFO *send_data, unsigned int TimeOut)
{
        unsigned short int i;
        unsigned short int  cnt = 0;
        DWORD current_time = 0;
        DWORD new_time = 0;
        int read_num = 0;
        bool CAN_BL_write_flag = 0;
        UINT crc_temp = 0x0000;
         int ret;
        bootloader_data Bootloader_data;
        //Bootloader_data部分成员进行赋值,后面的程序无需关注
        Bootloader_data.ExtId.bit.addr = NodeAddr;
        Bootloader_data.ExtId.bit.reserve = 0;
        Bootloader_data.IDE = CAN_ID_EXT;
        VCI_CAN_OBJ can_send_msg,can_read_msg[1000];
        can_send_msg.RemoteFlag = 0;
        can_send_msg.SendType = 1;
        //此处避免发生奇数个数据导致芯片写入数据错误
        if(send_data->data_len%2 !=0)
            {
                send_data->data[send_data->data_len] = 0xFF;
                send_data->data_len +=1;
            }
        //进行crc计算,并且进行赋值
        crc_temp = CRCcalc16((unsigned char*)send_data->data ,send_data->data_len);
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
        Bootloader_data.data[0] = ( send_data->data_addr & 0xFF000000 )>> 0x18;
        Bootloader_data.data[1] = ( send_data->data_addr & 0x00FF0000 )>> 0x10;
        Bootloader_data.data[2] = ( send_data->data_addr & 0x0000FF00 )>> 0x08;
        Bootloader_data.data[3] = ( send_data->data_addr & 0x000000FF )>> 0x00;
//----------------------------------------------------------------------
        Bootloader_data.data[4] = ( ( send_data->data_len + 2 ) & 0xFF000000 )>> 0x18;
        Bootloader_data.data[5] = ( ( send_data->data_len + 2 ) & 0x00FF0000 )>> 0x10;
        Bootloader_data.data[6] = ( ( send_data->data_len + 2 ) & 0x0000FF00 )>> 0x08;
        Bootloader_data.data[7] = ( ( send_data->data_len + 2 ) & 0x000000FF )>> 0x00;
        can_send_msg.DataLen    = Bootloader_data.DLC;
        can_send_msg.ExternFlag = Bootloader_data.IDE;
        can_send_msg.ID         = Bootloader_data.ExtId.all;
        for (i = 0; i < Bootloader_data.DLC; i++)
        {
             can_send_msg.Data[i] = Bootloader_data.data[i];
        }
        ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
        if(ret == -1)
        {
         return CAN_ERR_USB_WRITE_FAIL;
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
                   return CAN_BL_ERR_TIME_OUT;
                   break;
            case -1:
                    return CAN_ERR_USB_READ_FAIL;
                    break;
            default:
                ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
                            if(ret == -1)
                            {
                                return CAN_ERR_USB_READ_FAIL;
                                VCI_ClearBuffer(4,DevIndex,CANIndex);
                            }
                            if(ret == 1)
                            {
                                 //判断返回结果
                                if(can_read_msg[0].ID != (UINT)(Bootloader_data.ExtId.bit.addr<<4|cmd_list.CmdSuccess))//表示反馈数据有效
                                    {
                                        return CAN_BL_ERR_CMD;
                                    }

                            }
                    break;
            }
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
                 can_send_msg.DataLen = Bootloader_data.DLC;
                 can_send_msg.ExternFlag = 1;
                 can_send_msg.ID = Bootloader_data.ExtId.all;
                for (i = 0; i < Bootloader_data.DLC; i++)
                {
                    can_send_msg.Data[i] = send_data->data[cnt];
                    cnt++;
                }
            }
            else
            {
                Bootloader_data.DLC           = temp;
                Bootloader_data.ExtId.bit.cmd = cmd_list.Write;
                //-------------------------------------------------------
                 can_send_msg.DataLen         = Bootloader_data.DLC;
                 can_send_msg.ExternFlag      = Bootloader_data.IDE;
                 can_send_msg.ID              = Bootloader_data.ExtId.all;
                for (i = 0; i < Bootloader_data.DLC; i++)
                {
                    can_send_msg.Data[i]      = send_data->data[cnt];
                    cnt++;
                }
            }
            ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
            if(ret == -1)
            {
             return CAN_BL_ERR_SEND;
            }
        }
        for (i = 0; i < send_data->data_len + 2; i++)
        {
            send_data->data[cnt] = 0x00;
        }
        //发送数据完成,等待响应
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
                    return CAN_BL_ERR_TIME_OUT;
                    break;
              case -1:
                     return CAN_ERR_USB_READ_FAIL;
                     break;
             default:
                 ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
                             if(ret == -1)
                             {
                                 return CAN_ERR_USB_READ_FAIL;
                                 VCI_ClearBuffer(4,DevIndex,CANIndex);
                             }
                             if(ret == 1)
                             {
                                 if(can_read_msg[0].ID != (UINT)(Bootloader_data.ExtId.bit.addr<<4|cmd_list.CmdSuccess))//表示反馈数据有效
                                     {
                                         return CAN_BL_ERR_CMD;
                                     }

                             }
                     break;
             }
        //将数据清零准备下一次数据发送工作
        cnt = 0;
        return CAN_SUCCESS;
}

int MainWindow::CAN_BL_excute(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int Type)
{
    int ret = 0;
    bootloader_data Bootloader_data ;
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    VCI_CAN_OBJ can_send_msg;
    Bootloader_data.ExtId.bit.reserve = 0x00;
    Bootloader_data.ExtId.bit.cmd     = cmd_list.Excute;
    Bootloader_data.ExtId.bit.addr    = NodeAddr;
    can_send_msg.DataLen              = 3;
    can_send_msg.SendType             = 1;
    can_send_msg.RemoteFlag           = 0;
    can_send_msg.ExternFlag           = 1;
    can_send_msg.ID                   = Bootloader_data.ExtId.all;
    can_send_msg.Data[0]              = ((Type>>16)&0x000000FF);
    can_send_msg.Data[1]              = ((Type>>8) &0x000000FF);
    can_send_msg.Data[2]              = ((Type>>0) &0x000000FF);
    ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
    if(ret == -1)
    {
     return CAN_BL_ERR_SEND;
    }
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    return CAN_SUCCESS;
}
//-----------------------------------------------------------------------------------
//以下代码均为对hex文件解码需要的代码
//对hex解码的方式目前是根据自己的想法进行写的,后续可参考其他的方式
void MainWindow::Data_clear_int(  unsigned short  int *data,unsigned long int len)
    {
        unsigned long int i;
        for(i = 0;i < len;i++)
        {
            *data = 0;
            data++;
        }
    }

void MainWindow::Data_clear(  char *data,unsigned long int len)
{
     unsigned long int i;
     for(i = 0;i < len;i++)
     {
         *data = 0;
         data++;
     }
}
unsigned char MainWindow::convertion(  char *hex_data)
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
void MainWindow::hex_to_bin(  char *hex_src, char *bin_dst,unsigned char len)
{
    unsigned char i;
    for(i = 0;i <len;i++)
    {
        *bin_dst = convertion(hex_src);
        bin_dst++;
        hex_src++;
    }
}
unsigned short int MainWindow::CRCcalc16( unsigned char *data, unsigned short int len)
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


void MainWindow::on_action_savefile_triggered()
{
        QString fileName;
        fileName = QFileDialog::getSaveFileName(this,
                                                tr("保存文件"),
                                                "",
                                                tr("Hex Files (*.hex);;Binary Files (*.bin);;All Files (*.*);;文本文档(*.txt)")
                                                );
        qDebug()<<"on_action_savefile_triggered "<<fileName;

        if (!fileName.isNull())
        {
                            //fileName是文件名
        }
        else
        {

        }
}
