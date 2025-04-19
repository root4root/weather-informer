/*
Partial ported from: https://github.com/johnrickman/LiquidCrystal_I2C
*/
#ifndef LCD_H
#define LCD_H

#include "unistd.h"

void lcd_begin(uint8_t i2c_addr, uint8_t lcd_cols, uint8_t lcd_rows);
void lcd_backlight(void);
void lcd_noBacklight(void);
void lcd_clear(void);
void lcd_command(uint8_t);
void lcd_createChar(uint8_t, uint8_t[]);
void lcd_cursor();
void lcd_noCursor();
void lcd_setCursor(uint8_t x, uint8_t y);
void lcd_display(void);
void lcd_noDisplay(void);
void lcd_home(void);
void lcd_write(uint8_t);
void lcd_writeString(char *str);

#endif //LCD_H
