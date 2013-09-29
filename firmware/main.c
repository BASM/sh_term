#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <uart.h>
#include <twi.h>
#include <adc.h>

#define MAXTERM_PREF 0x4f
#define DSTERM_PREF 0x4d
#define HUMI_CHAN 0x07

static int getcurterm() {
  int res;
  
  twi_req_read(MAXTERM_PREF,0x00);
  res=twi_p_read(0)<<8;
  res|=twi_p_read(1);
  twi_p_stop();

  //res=((res&0x3ffff)>>3)*0.0625*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB, 10 -- output at 0.1C
  res=((res&0x3ffff)>>3)*0.0625*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB,
  return res;
}

static int getcurtermDS() {
  int res;
  
  twi_req_read(DSTERM_PREF,0xAA);
  res=twi_p_read(0)<<8;
  res|=twi_p_read(1);
  twi_p_stop();

  //res=((res&0x3ffff)>>3)*0.0625*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB, 10 -- output at 0.1C
  res=((res&0x3ffff)>>3)*0.03125*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB,
  
  twi_req_read(DSTERM_PREF,0xEE);
  twi_p_stop();
  return res;
}

int main(void) {
  int term;
  int term1;
  int adc;
  unsigned int i;

  cli();
  uart_init();
  stdioconf_stdio();

  twi_init();
  adc_init();

  printf("HEllo!!\n");


  DDRC=0x00;
  DDRB=0xff;
  DDRD=0xff;
  PORTB=3;
  PORTD=1<<3;

  //OC1A -- PB1

  i=0;
  while (1) {
    i++;

    PORTB=0;
    //PORTD=0;
    _delay_ms(50);
    PORTB=3;
    PORTD=1<<3;
    _delay_ms(2000);

    term=getcurterm();
    term1=getcurtermDS();
    printf("Current term is: %i, and %i\n", term, term1);
  }


  return 0;
}
