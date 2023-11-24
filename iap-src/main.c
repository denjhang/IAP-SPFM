#include "STC15.H"//"STC12C5630AD.H"
#include <intrins.h>
//#include "absacc.h"

//#define FM_REG_ADDR1		XBYTE[0x7ffc]
//#define FM_REG_DATA1		XBYTE[0x7ffd]
//#define FM_REG_ADDR2		XBYTE[0x7ffe]
//#define FM_REG_DATA2		XBYTE[0x7fff]

sbit BUS_A0						=	P2^0;
sbit BUS_A1						=	P2^1;
sbit BUS_A2						=	P2^2;
sbit BUS_A3						=	P2^3;
sbit BUS_WR						=	P4^2;
sbit BUS_RD						=	P4^4;
sbit BUS_BSEL0				=	P2^6;
sbit BUS_BSEL1				=	P2^7;
sbit BUS_IC						=	P4^5;
#define BUS_DATA_PORT	P0

void UART_Init(void)		//1500000bps@24.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率
	AUXR |= 0x40;		//定时器时钟1T模式
	AUXR &= 0xFE;		//串口1选择定时器1为波特率发生器
	TMOD &= 0x0F;		//设置定时器模式
	//PCON |= 0x40;	//启用帧错误检测
	TL1 = 0xFC;		//设置定时初始值
	TH1 = 0xFF;		//设置定时初始值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//定时器1开始计时
}

void UART_TransmitByte(unsigned char dat)
{
	TI = 0;
	SBUF = dat;
	while(TI == 0);
	TI = 0;
}

void delayus(unsigned int t) 
{
	while(--t);
}

void BUS_Write(unsigned char slot, unsigned char addr, unsigned char dat)
{
	BUS_A0 = addr & 0x1;
	BUS_A1 = ((addr >> 1) & 0x1);
	BUS_A2 = ((addr >> 2) & 0x1);     //基本不用
	BUS_A3 = ((addr >> 3) & 0x1);
	if(slot == 1) {BUS_BSEL0 = 1;BUS_BSEL1 = 0;}
	else {BUS_BSEL0 = 0;BUS_BSEL1 = 1;}
	BUS_DATA_PORT = dat;
	BUS_WR = 0;
	delayus(10);
	BUS_WR = 1;
	BUS_BSEL0 = 1;BUS_BSEL1 = 1;
	_nop_();
}

/*

void OPN_Test()
{
	unsigned char i;
	unsigned int code testData[40]={0x2200,0x2700,0x2800,0x2b00,0x3071,0x340d,0x3833,0x3c01,
									0x4023,0x442d,0x4826,0x4c00,0x505f,0x5499,0x585f,0x5c94,
									0x6005,0x6405,0x6805,0x6c07,0x7002,0x7402,0x7802,0x7c02,
									0x8011,0x8411,0x8811,0x8ca6,0x9000,0x9400,0x9800,0x9c00,
									0xb032,0xb4c0,0x2800,0xa422,0xa069,0x28f0};
	for(i=0;i<40;i++) {
		BUS_Write(0 , 0, testData[i]/256);
									delayus(20);
		BUS_Write(0, 1, testData[i]%256);
									delayus(90);
	}
}
*/

void main()
{
	unsigned short i, flip_cnt = 2000, reg_flip = 0;
	unsigned char dt = 0, 	//串口传输，接收到的数据
								cmd = 0, 	//收到的命令
								reg = 0,
								dat = 0, 
								cs = 0, 	//板卡编号(0 or 1)
								addr = 0;	//当前板卡地址编号
	unsigned char fe_flag = 0;	//串口传输，帧错误标志

	P0M0 = 0;
	P0M1 = 0;
	P1M0 = 0;
	P1M1 = 0;
	P2M0 = 0;
	P2M1 = 0;
	P3M0 = 0;
	P3M1 = 0;
	P4M0 = 0;
	P4M1 = 0;

	BUS_DATA_PORT = 0;
	BUS_A0 = 0;
	BUS_A1 = 0;
	BUS_A2 = 0;
	BUS_A3 = 0;
	BUS_WR = 1;
	BUS_RD = 1;
	BUS_BSEL0 = 1;
	BUS_BSEL1 = 1;
	BUS_IC = 0;
	
	//OPN_Test();
	
	UART_Init();
	
	for(i=0; i<32767; i++);
	for(i=0; i<32767; i++);
	
	BUS_IC = 1;
	
	while(1)
	{
		if(--flip_cnt == 0)
		{
			reg_flip = 0;
			flip_cnt = 1000;
			continue;
		}
		if(RI) {
			RI = 0;
			//PCON |= 0x40;
			//fe_flag = SCON & 0x80;
			/*if(fe_flag == 0x80) {
				fe_flag = 0;
				SCON &= 0x7f;
				dt = '?';
			}
			else {*/
				dt = SBUF;
				if(reg_flip == 0) {
					switch((dt&0xf0)) {
						case 0xf0:														//握手信号
							if(dt == 0xff) {										//握手信号1
								UART_TransmitByte('R');
								UART_TransmitByte('S');
							}
							else if(dt == 0xfe) {								//握手信号2
								BUS_IC = 0;
								for(i=0; i< 20000; i++);
								BUS_IC = 1;
								for(i=0; i< 20000; i++);
								UART_TransmitByte('O');
								UART_TransmitByte('K');
							}
							break;
						case 0x80:													//0x80：延时
							if(dt == 0x80) _nop_();
							break;
						case 0x00:													//0x0n:向指定板卡指定地址写数据
							if(dt == 1) cs = 1;
							else cs = 0;
							reg_flip ++;										//该命令四个字节，继续接收下一字节
							break;
						default:
							break;
					}
				}
				else if(reg_flip == 1) {							//0x0n命令的第一个参数
					reg_flip ++;
					cmd = dt & 0xf0;
 					if(cmd == 0x00 || cmd == 0x80) {
							addr = dt & 0x0f;
							cmd = dt;
					}
					else if(cmd == 0x20)		//用于SN76489
						cmd = dt;
					else 
						reg_flip = 0;
				}
				else if(reg_flip == 2) {								//0x0n命令的第二个参数
					if((cmd&0xf0) == 0x00) {
						BUS_Write(cs, addr, dt);             //写寄存器地址（A0=0）
						reg_flip ++;
					}
					else if((cmd&0xf0) == 0x80) {
						BUS_Write(cs, addr, dt);
						reg_flip = 0;
					}
					else if(cmd == 0x20) {
						BUS_Write(cs, 0, dt);
						reg_flip = 0;
					}
				}
				else if(reg_flip == 3) {                //0x0n命令的第三个参数
					BUS_Write(cs, addr|0x01, dt);           //寄存器数据（A0=1）
					reg_flip = 0;
				}
			//}
			flip_cnt = 10000;
		}
	}
}

