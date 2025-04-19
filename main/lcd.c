#include "lcd.h"
#include "config.h"

#include "driver/i2c_master.h"

#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT     0x10
#define LCD_FUNCTIONSET     0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON   0x04
#define LCD_DISPLAYOFF  0x00
#define LCD_CURSORON    0x02
#define LCD_CURSOROFF   0x00
#define LCD_BLINKON     0x01
#define LCD_BLINKOFF    0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT   0x04
#define LCD_MOVELEFT    0x00

// flags for function set
#define LCD_8BITMODE    0x10
#define LCD_4BITMODE    0x00
#define LCD_2LINE       0x08
#define LCD_1LINE       0x00
#define LCD_5x10DOTS    0x04
#define LCD_5x8DOTS     0x00

// flags for backlight control
#define LCD_BACKLIGHT   0x08
#define LCD_NOBACKLIGHT 0x00

#define EN 0b00000100   // Enable bit
#define RW 0b00000010   // Read/Write bit
#define RS 0b00000001   // Register select bit

static i2c_master_dev_handle_t dev_handle;

static uint8_t _displayfunction;
static uint8_t _displaycontrol;
static uint8_t _displaymode;
static uint8_t _backlightval;
static uint8_t _numlines;
static uint8_t _cols;
static uint8_t _rows;

static void send(uint8_t, uint8_t);
static void expanderWrite(uint8_t);
static void pulseEnable(uint8_t);
static void write4bits(uint8_t);

static void init(uint8_t i2c_addr)
{
    i2c_master_bus_handle_t bus_handle;

    ESP_ERROR_CHECK(i2c_master_get_bus_handle(CONFIG_PORT_NUMBER, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_addr,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

}

void lcd_begin(uint8_t i2c_addr, uint8_t lcd_cols,uint8_t lcd_rows)
{
    init(i2c_addr);

    _cols = lcd_cols;
    _rows = lcd_rows;

    _backlightval = LCD_NOBACKLIGHT;
    _displayfunction |= LCD_2LINE;
    _numlines = _cols;

    usleep(50000);
    expanderWrite(_backlightval);    // reset expanderand turn backlight off (Bit 8 =1)
    usleep(250000);
    write4bits(0x03 << 4);
    usleep(4500); // wait min 4.1ms
    write4bits(0x03 << 4);
    usleep(4500); // wait min 4.1ms
    write4bits(0x03 << 4);
    usleep(150);
    write4bits(0x02 << 4);
    lcd_command(LCD_FUNCTIONSET | _displayfunction);

    _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;

    lcd_display();
    lcd_clear();

    _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

    lcd_command(LCD_ENTRYMODESET | _displaymode);
    lcd_home();
}

void lcd_backlight(void)
{
    _backlightval = LCD_BACKLIGHT;
    expanderWrite(0);
}

void lcd_noBacklight(void)
{
    _backlightval = LCD_NOBACKLIGHT;
    expanderWrite(0);
}

void lcd_clear()
{
    lcd_command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
    usleep(2000);  // this command takes a long time!
}

void lcd_command(uint8_t value)
{
    send(value, 0);
}

void lcd_createChar(uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // we only have 8 locations 0-7
    lcd_command(LCD_SETCGRAMADDR | (location << 3));

    for (int i = 0; i < 8; ++i) {
        lcd_write(charmap[i]);
    }
}

void lcd_cursor() {
    _displaycontrol |= LCD_CURSORON;
    lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_noCursor() {
    _displaycontrol &= ~LCD_CURSORON;
    lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_setCursor(uint8_t x, uint8_t y)
{
    static int row_offsets[] = {0x00, 0x40, 0x14, 0x54};

    if (y > _numlines) {
        y = _numlines - 1;  // we count rows starting w/0
    }

    lcd_command(LCD_SETDDRAMADDR | (x + row_offsets[y]));
}

void lcd_display(void)
{
    _displaycontrol |= LCD_DISPLAYON;
    lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_noDisplay(void)
{
    _displaycontrol &= ~LCD_DISPLAYON;
    lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_home(void)
{
    lcd_command(LCD_RETURNHOME);  // set cursor position to zero
    usleep(2000);  // this command takes a long time!
}

void lcd_write(uint8_t value)
{
    send(value, RS);
}

void lcd_writeString(char *str)
{
    while (*str) {
        lcd_write(*str++);
    }
}//


//--Private functions goes next:

static void send(uint8_t value, uint8_t mode)
{
    uint8_t highnib = value & 0xF0;
    uint8_t lownib = (value << 4) & 0xF0;
    write4bits((highnib) | mode);
    write4bits((lownib) | mode);
}

static void write4bits(uint8_t value)
{
    expanderWrite(value);
    pulseEnable(value);
}

static void expanderWrite(uint8_t data)
{
    uint8_t buff = data | _backlightval;
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &buff, 1, -1));
}

static void pulseEnable(uint8_t data)
{
    expanderWrite(data | EN);   // En high
    usleep(1);  // enable pulse must be >450ns

    expanderWrite(data & ~EN);  // En low
    usleep(50); // commands need > 37us to settle
}
