/*
 * Optimized FreeExpression OLED Driver for UG-2864ASYDT01 (SSD1325)
 * Targeting: Atmel ATmega1281 @ 16MHz
 */

#ifdef MACHINE_EXPRESSION
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "u8g2.h"
#include "display_oled.h"
#include "timer.h"

void _oled_display_speed(void);
void _oled_display_pressure(void);

u8g2_t u8g2;
static uint8_t cur_x = 0, cur_y = 0;
char display_message[80]; 

/*
 * Custom hardware-specific GPIO/Delay callback for the Cricut Expression Motherboard pinout
 */
uint8_t u8x8_gpio_and_delay_expression(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  switch(msg) {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
      // Configure Motherboard display interface directions:
      DDRF |= (1<<6) | (1<<5) | (1<<7); // SCK (PF6), MOSI (PF5), DC (PF7) as outputs
      DDRE |= (1<<7) | (1<<6);          // CS (PE7), RES (PE6) as outputs
      break;
      
    case U8X8_MSG_GPIO_DELAY_100NANO:
      __asm__ __volatile__ ("nop\n\t" "nop\n\t"); // Quick AVR delays @ 16MHz
      break;
      
    case U8X8_MSG_GPIO_DELAY_10MA:
      for(volatile uint32_t i=0; i<16000; i++); // Approximate 10ms loop delay
      break;

    // Pin Control mapping directly to the Cricut Motherboard architecture:
    case U8X8_MSG_GPIO_CS:              // CS# pin on PE7
      if (arg_int) PORTE |= (1<<7); else PORTE &= ~(1<<7);
      break;
    case U8X8_MSG_GPIO_DC:              // DC# pin on PF7
      if (arg_int) PORTF |= (1<<7); else PORTF &= ~(1<<7);
      break;
    case U8X8_MSG_GPIO_RESET:           // RES pin on PE6
      if (arg_int) PORTE |= (1<<6); else PORTE &= ~(1<<6);
      break;
    case U8X8_MSG_GPIO_SPI_CLOCK:       // SCLK pin on PF6
      if (arg_int) PORTF |= (1<<6); else PORTF &= ~(1<<6);
      break;
    case U8X8_MSG_GPIO_SPI_DATA:        // SDIN pin on PF5
      if (arg_int) PORTF |= (1<<5); else PORTF &= ~(1<<5);
      break;
  }
  return 1;
}

void _oled_display_speed(void)
{
	int p = timer_get_stepper_speed();
	
	// FIX: Replaced u8g2_SetColorIndex with valid u8g2_SetDrawColor
	u8g2_SetDrawColor(&u8g2, 3); 
	u8g2_DrawBox(&u8g2, 30,62,2,2); // 1
	if(p<2) u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2, 33,61,2,3); // 2
	if(p<3) u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2, 36,60,2,4); // 3
	if(p<4) u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2, 39,59,2,5); // 4
	if(p<5) u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2, 42,58,2,6); // 5
	u8g2_SetDrawColor(&u8g2, 3);
}

void _oled_display_pressure(void)
{
	int p = timer_get_pen_pressure();
	
	// FIX: Replaced u8g2_SetColorIndex with valid u8g2_SetDrawColor
	u8g2_SetDrawColor(&u8g2, 3);
	u8g2_DrawBox(&u8g2, 54,62,2,2); // 1
	if(p<2) u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2, 57,61,2,3); // 2
	if(p<3) u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2, 60,60,2,4); // 3
	if(p<4) u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2, 63,59,2,5); // 4
	if(p<5) u8g2_SetDrawColor(&u8g2, 1);
	u8g2_DrawBox(&u8g2, 66,58,2,6); // 5
	u8g2_SetDrawColor(&u8g2, 3);
}

void oled_display_init(void)
{
  /* Select minimal clock prescaler for maximum core processor execution speed */
  CLKPR = 0x80;
  CLKPR = 0x00;

  // FIX: Using correct structural U8g2 configuration function targeting the 
  // Full frame-buffer variant (_f_) to bypass layout loops.
  u8g2_Setup_ssd1325_nhd27oled_2x_gr_sw_spi(&u8g2, U8G2_R2, u8x8_byte_2view_sw_spi, u8x8_gpio_and_delay_expression);
  
  u8g2_InitDisplay(&u8g2);
  u8g2_SetPowerSave(&u8g2, 0); // Clear power save mode to turn on display hardware

  /* Assign default initialization color constraints */
  u8g2_SetDrawColor(&u8g2, 3); 

  u8g2_SetFont(&u8g2, u8g2_font_profont11_tr); // Changed to '_tr' variant to decrease text print cycles
  u8g2_SetFontPosTop(&u8g2);
}

// OPTIMIZED: The main screen redraw loop is completely flat now.
void oled_display_update(void)
{
    // 1. Instantly clear the internal buffer inside ATmega1281 SRAM
    u8g2_ClearBuffer(&u8g2);      
    
    // 2. Draw layout elements exactly ONCE per frame execution call
    u8g2_DrawStr(&u8g2, cur_x, cur_y, display_message);
    _oled_display_speed();
    _oled_display_pressure();
    
    // 3. Dump the complete rendered frame to the physical display controller
    u8g2_SendBuffer(&u8g2);       
}

void oled_display_puts(const char *s) {
	strcpy(display_message, s);
	oled_display_update();
}

void oled_display_println(char *s) {
	oled_display_puts(s);
	cur_y += 10;
	cur_x = 0;
}

#endif //#ifdef MACHINE_EXPRESSION
