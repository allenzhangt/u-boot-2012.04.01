
#define __REG(x)					(*(volatile unsigned long *)(x)) 
#define __REG_BYTE(x)				(*(volatile unsigned char *)(x)) 

/*NAND FLASH控制器*/
#define NFCONF 					 __REG(0x4E000000)
#define NFCONT 					 __REG(0x4E000004) 
#define NFCMMD 					 __REG_BYTE(0x4E000008) 
#define NFADDR 					 __REG_BYTE(0x4E00000c) 
#define NFDATA 					 __REG_BYTE(0x4E000010) 
#define NFSTAT 					 __REG_BYTE(0x4E000020) 

/* GPIO */                
#define GPHCON                   __REG(0x56000070)  //Port H control                   
#define GPHUP                    __REG(0x56000078)  //Pull-up control H                                
/* UART regieters */
#define ULCON0                   __REG(0x50000000)  //UART 0 line control      
#define UCON0                    __REG(0x50000004)  //UART 0 control           
#define UFCON0                   __REG(0x50000008)  //UART 0 FIFO control      
#define UMCON0                   __REG(0x5000000C)  //UART 0 modem control     
#define UTRSTAT0                 __REG(0x50000010)  //UART 0 Tx/Rx status      
#define UERSTAT0                 __REG(0x50000014)  //UART 0 Rx error status   
#define UFSTAT0                  __REG(0x50000018)  //UART 0 FIFO status       
#define UMSTAT0                  __REG(0x5000001C)  //UART 0 modem status    
#define UTXH0                    __REG_BYTE(0x50000020)  //UART 0 transmission hold 
#define URXH0                    __REG_BYTE(0x50000024)  //UART 0 receive buffer    
#define UBRDIV0                  __REG(0x50000028)  //UART 0 baud rate divisor 

void nand_read_ll(unsigned int addr, unsigned char *buf, unsigned int len);

static int bBootFrmNORFlash(void)
{
	volatile int *p = (volatile int *)0;
	int val;

	val = *p;
	*p = 0x12345678;
	if(*p == 0x12345678) 
	{
		*p = val;
		return 0;
	}
	else
	{
		/* Nor不能像内存一样写*/
		return 1;
	}
}

int CopyCode2SDRAM(unsigned char *src, unsigned char *dest, int len)
{
	int i = 0;
	if (bBootFrmNORFlash())
	{
		while(i < len)
		{
			dest[i] = src[i];
			i++;
		}
	}
	else
	{
		//nand_init();
		nand_read_ll((unsigned int)src,dest,len);
	}

	return 0;
}

void clear_bss(void)
{
    extern int __bss_start, __bss_end__;
    int *p = &__bss_start;
    
    for (; p < &__bss_end__; p++)
        *p = 0;
}

void nand_init_ll(void)
{
#define  TACLS   0
#define  TWRPH0  1
#define  TWRPH1  0
	/*设置NAND FLASH的时序*/
    NFCONF = (TACLS<<12) | (TWRPH0<<8) | (TWRPH1<<4);
    /*使能NAND FLASH控制器,初始化ECC，禁止片选*/
    NFCONT = (1<<4) | (1<<1) | (1<<0);
}

static void nand_select(void)
{
    /*使能片选*/
    NFCONT &=~(1<<1);
}

static void nand_deselect(void)
{
    /*禁止片选*/
    NFCONT |= (1<<1);
}

static void nand_cmd(unsigned char cmd)
{
    volatile int i;
    NFCMMD = cmd;
    for(i=0; i<10; i++);
}

static void nand_addr(unsigned int addr)
{
    volatile int i;

    int page = addr / 2048;
    int col  = addr % 2048;

    NFADDR = col & 0xff;
    for(i=0; i<10; i++);

    NFADDR = (col >> 8) & 0xff;
    for(i=0; i<10; i++);

    NFADDR = page & 0xff;
    for(i=0; i<10; i++);

    NFADDR = (page >> 8) & 0xff;
    for(i=0; i<10; i++);

    NFADDR = (page >> 16) & 0xff;
    for(i=0; i<10; i++);
}

static void nand_wait_ready(void)
{
    while (!(NFSTAT & 1));
}

static unsigned char nand_data(void)
{
    return  NFDATA;
}

void nand_read_ll(unsigned int addr, unsigned char *buf, unsigned int len)
{
	int i = 0;
	int col  = addr % 2048;

	/* 1. 选中 */
	nand_select();

	while(i < len)
	{
		/* 2. 发出读命令00h */
		nand_cmd(00);

		/* 3. 发出地址（分5步） */
		nand_addr(addr);
		
		/* 4. 发出读命令30h */
		nand_cmd(0x30);

		/* 5.  判断状态 */
		nand_wait_ready();

		/* 6. 读数据 */
		for (; (col < 2048) && (i < len); col++ )
		{
			buf[i++] = nand_data();
			addr++;
		}
		col = 0;
	}

	/* 7. 取消选中 */
	nand_deselect();
}


