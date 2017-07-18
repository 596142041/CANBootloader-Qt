#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H
#define CAN_Id_Standard   0//表示标准帧
#define CAN_Id_Extended   1//表示扩展帧
#define CAN_ID_STD      CAN_Id_Standard
#define CAN_ID_EXT      CAN_Id_Extended
//--以下宏定义白哦是当前设备运行的固件属性
#define CAN_BL_BOOT     0x55555555  //运行的是Boot
#define CAN_BL_APP      0xAAAAAAAA //运行的是APP
//--------------------
//对hex文件解码使用的数据结构
#define NUM_OFFSET   48
#define CHAR_OFFSET  55
//---------------
//关于hex文件中的相关信息
/*
第一个字节 表示本行数据的长度；
第二、三字节表示本行数据的起始地址；
第四字节表示数据类型，数据类型有：0x00、0x01、0x02、0x03、0x04、0x05。
'00' Data Rrecord：用来记录数据，HEX文件的大部分记录都是数据记录
'01' End of File Record:用来标识文件结束，放在文件的最后，标识HEX文件的结尾
'02' Extended Segment Address Record:用来标识扩展段地址的记录
'03' Start Segment Address Record:开始段地址记录
'04' Extended Linear Address Record:用来标识扩展线性地址的记录
'05' Start Linear Address Record:开始线性地址记录
然后是数据，最后一个字节 为校验和。
校验和的算法为：计算校验和前所有16进制码的累加和(不计进位)，检验和 = 0x100 - 累加和
*/
//对于第四个字节的数据类型定义
#define DATA_Rrecord 0x00
#define DATA_END     0x01
#define DATA_BASE_ADDR 0x04
//---------------关于DSP的相应扇区----------------
#define SECTORA   (Uint16)0x0001
#define SECTORB   (Uint16)0x0002
#define SECTORC   (Uint16)0x0004
#define SECTORD   (Uint16)0x0008
#define SECTORE   (Uint16)0x0010
#define SECTORF   (Uint16)0x0020
#define SECTORG   (Uint16)0x0040
#define SECTORH   (Uint16)0x0080
//------------------------------------------------
#define  FW_TYPE_APP  0xAAAAAAAA
#define  FW_TYPE_BOOT 0x55555555
typedef struct _PACK_INFO
{
    unsigned short int data_len;
    unsigned long  int data_addr;
    unsigned long  int data_base_addr;
    unsigned short int data_addr_offset;
    unsigned char      data_type;
    unsigned short int      Data[64];
}PACK_INFO;

//----------------------
typedef struct _Bootloader_data
{
    union
    {
         unsigned long int all;
        struct
        {
            unsigned short int cmd     :4; //命令
            unsigned short int addr    :12; //设备地址
            unsigned short int  reserve :16; //保留位
        }bit;
    } ExtId; //扩展帧ID
    unsigned char IDE;   //帧类型,可为：CAN_ID_STD(标准帧),CAN_ID_EXT(扩展帧)
    unsigned char DLC;  //数据长度，可为0到8;
    unsigned char data[8];
}  bootloader_data;
typedef struct _Boot_CMD_LIST
{
    //Bootloader相关命令
    unsigned char Erase;        //擦出APP储存扇区数据
    unsigned char WriteInfo;    //设置多字节写数据相关参数（写起始地址，数据量）
    unsigned char Write;        //以多字节形式写数据
    unsigned char Check;        //检测节点是否在线，同时返回固件信息
    unsigned char SetBaudRate;  //设置节点波特率
    unsigned char Excute;       //执行固件
    //节点返回状态
    unsigned char CmdSuccess;   //命令执行成功
    unsigned char CmdFaild;     //命令执行失败
} Boot_CMD_LIST;
typedef struct _SEND_INFO
{
    unsigned char line_num;//表示读取数据的行数,最大为2;
    unsigned char line_cnt;//表示读取文件的函数,最大值不能大于line_num
    unsigned char read_start_flag;//表示开始读取数据标志位
    unsigned char send_state;//表示是否有数据需要发送
    unsigned long int data_addr;
    unsigned char data[68];//
    unsigned char data_cnt;//表示需要发送多少数据,最大值为66
    unsigned char data_len;//表示需要发送的数据长度;
}SEND_INFO;
typedef struct _Device_INFO
{
    union
    {
        unsigned short int all;
        struct
        {
            unsigned short int Device_addr:	12;
            unsigned short int reserve:	4;
        }bits;//设备地址
    }Device_addr;
    unsigned long int FW_TYPE;//固件类型
    unsigned long int FW_Version;//固件版本
}Device_INFO;
#endif // CAN_DRIVER_H
