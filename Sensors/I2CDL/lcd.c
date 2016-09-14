#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev-user.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <inttypes.h>

#define RGB_SLAVE       0x62
#define LCD_SLAVE       0x3E
#define BUS             0x06
#define REG_RED         0x04        // PWM2
#define REG_GREEN       0x03        // PWM1
#define REG_BLUE        0x02        // PWM0
#define REG_MODE1       0x00
#define REG_MODE2       0x01
#define REG_OUTPUT      0x08

// commands
#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL    0x08
#define LCD_CURSORSHIFT    0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT    0x00
#define LCD_ENTRYLEFT    0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON    0x04
#define LCD_DISPLAYOFF    0x00
#define LCD_CURSORON    0x02
#define LCD_CURSOROFF    0x00
#define LCD_BLINKON    0x01
#define LCD_BLINKOFF    0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE    0x00
#define LCD_MOVERIGHT    0x04
#define LCD_MOVELEFT    0x00

// flags for function set
#define LCD_8BITMODE    0x10
#define LCD_4BITMODE    0x00
#define LCD_2LINE    0x08
#define LCD_5x8DOTS    0x00
//////////////////////////////////////////////////////////
uint8_t _displayfunction = 0;
uint8_t _displaycontrol = 0;
uint8_t _displaymode = 0;
uint8_t lines = 2;

typedef struct
{
    int addr; //i2c address of device
    int file; //reprsents a place to do the i2c read & write
    int adapter_nr; //the bus number of the device
    char filename[20]; //the name of the device
}I2CCONTEXT;

I2CCONTEXT lcd; //will store the lcd controller context
I2CCONTEXT rgb; //will store the rgb controller context

int initContext(I2CCONTEXT *ctx, int addr_, int bus)
{
    ctx->addr = addr_;
    ctx->adapter_nr = bus;
    snprintf(ctx->filename, 19, "/dev/i2c-%d", ctx->adapter_nr);
    ctx->file = open(ctx->filename, O_RDWR);

    if (ctx->file < 0) 
    {
       /* ERROR HANDLING; you can check errno 2 c what went wrong */
        printf("Error ocurred @ opening BUS: (errno:%d) %s\n",
                errno,
                strerror(errno));
        return -errno;
    }

    if (ioctl(ctx->file, I2C_SLAVE, ctx->addr) < 0)
    {
        /* ERROR HANDLING; you can check errno 2 c what went wrong */
        printf("Error ocurred @ accessing slave: (errno:%d) %s\n",
                    errno,
                    strerror(errno));
        return -errno;
    }
    return 0;
}

__s32 writeByteRegister(int file, __u8 register_, __u8 value)
{
    __s32 res = -1;
    res = i2c_smbus_write_byte_data(file, register_, value);
    if (res < 0)
    {
        /* ERROR HANDLING: i2c transaction failed */
        printf("Error writing byte, (errno:%d),%s",errno,
                strerror(errno));
    }
    return res;
}

__s32 readRegister(int register_, int file)
{
    __u8 reg = register_;
    __s32 res = -1;
    char buf[10];  
    res = i2c_smbus_read_word_data(file, reg);
    if (res < 0)
    {
        /* ERROR HANDLING: i2c transaction failed */
        printf("Error reading reg: 0x%x, (errno:%d),%s",
                reg,errno,strerror(errno));
    }
    return res;
}

void setRGBColor(I2CCONTEXT *rgb, int r, int g, int b)
{
    writeByteRegister(rgb->file, REG_RED, (__u8)r);
    writeByteRegister(rgb->file, REG_GREEN, (__u8)g);
    writeByteRegister(rgb->file, REG_BLUE, (__u8)b);            
}

void initRGB(I2CCONTEXT *rgb)
{
    // backlight init
    writeByteRegister(rgb->file, REG_MODE1, 0);
    // set LEDs controllable by both PWM and GRPPWM registers
    writeByteRegister(rgb->file, REG_OUTPUT, 0xFF);
    // set MODE2 values
    writeByteRegister(rgb->file, REG_MODE2, 0x20);
    // set the baklight Color to white :)
    setRGBColor(rgb, 0xFF, 0xFF, 0xFF);
}

void turnOffRGB(I2CCONTEXT *rgb)
{
    setRGBColor(rgb, 0x00, 0x00, 0x00);    
}

void initLCD(I2CCONTEXT *lcd)
{
    //set the number of lines of our Display
    ////////////////////////////////////////////////////////////////////
    writeByteRegister(lcd->file, 0x00, LCD_FUNCTIONSET | LCD_2LINE );
    usleep(100);

    //enable display, show cursor, show a blinking box cursor
    ////////////////////////////////////////////////////////////////////
    writeByteRegister(lcd->file, 0x00,LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON );
    usleep(100);

    //clear display and set cursor at 0 position
    ////////////////////////////////////////////////////////////////////
    writeByteRegister(lcd->file, 0x00, LCD_CLEARDISPLAY );
    usleep(4000);

    //set how the cursor will move everytime a letter is written to the LCD
    ////////////////////////////////////////////////////////////////////////
    writeByteRegister(lcd->file, 0x00,LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT );
}

void writeToLCD(I2CCONTEXT *lcd, char *c)
{

    uint ascii_val=0;
    int i;
    int s = strlen(c);
    for(i = 0 ; i < s; i++)
    {
        ascii_val = toascii(c[i]);
        ///////////////////////////////////////////////////////////
        //sending a 0x40 tell the LCD we will send a character code
        //to CGRAM
        ///////////////////////////////////////////////////////////
        writeByteRegister(lcd->file, 0x40, ascii_val);
    }
}

void turnOffLCD(I2CCONTEXT *lcd)
{    
    writeByteRegister(lcd->file, 0x00, LCD_DISPLAYCONTROL );
    writeByteRegister(lcd->file, 0x00, LCD_CLEARDISPLAY );
}

int main()
{
    //Grove LCD has two devices one that drives LCD
    //and a second that drives RGB
    initContext(&rgb, RGB_SLAVE , BUS);
    initContext(&lcd, LCD_SLAVE , BUS);
    ////////////////////////////////////////////////////////////

    //init RGB device & set White color
    initLCD(&lcd);
    initRGB(&rgb);    

    writeToLCD(&lcd, ":) HOLO!");
    /*sleep for 5 secs before turning off*/
    sleep(5);
    /*turn off RGB LEDS*/
    turnOffRGB(&rgb);
    /*turn off LCD*/
    turnOffLCD(&lcd);
    printf("\nDONE!\n");

    return 0;
}
