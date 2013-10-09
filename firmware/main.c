#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <uart.h>
#include <twi.h>
#include <adc.h>
#include <twi_sw.h>
#include <limits.h>
#include "twisw.h"
#include "terms.h"

#define MAXTERM_PREF 0x4f
#define DSTERM_PREF   0x4d
//#define DSTERMSW_PREF 0x4f
#define DSTERMSW_PREF 0x48
#define HUMI_CHAN 0x07
  
twi_data stwi_obj;
twi_data *stwi=&stwi_obj;


static Term term_MAX_get() {
  int res;
  
  if (twi_req_read(MAXTERM_PREF,0x00)) {
    twi_p_stop();
    return TERM_MAX;
  }
  res=twi_p_read(0)<<8;
  res|=twi_p_read(1);
  twi_p_stop();

  res=((res&0x3ffff)>>3)*0.0625*10;
  return res;
} 

static Term term_sw_DS_get(twi_data *twi, int i) {
  int res,res1,res2;
  
  if (twi_sw_req_read(twi,DSTERMSW_PREF+i,0xAA)) {
    return TERM_MAX;
  }
  res1 =twi_sw_read(twi,0);
  res2 =twi_sw_read(twi,1);

  twi_sw_stop(twi);

  _delay_ms(10);

  res=(res1<<8)|res2;
  //res=((res&0x3ffff)>>3)*0.0625*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB, 10 -- output at 0.1C
  res=((res&0x3ffff)>>3)*0.03125*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB,
  
  twi_sw_req_read(twi,DSTERMSW_PREF+i,0xEE);
  twi_sw_stop(twi);
  return res;
}

static Term term_DS_get() {
  int res;
  
  if (twi_req_read(DSTERM_PREF,0xAA)) {
    twi_p_stop();
    return TERM_MAX;
  }
  res=twi_p_read(0)<<8;
  res|=twi_p_read(1);
  twi_p_stop();

  _delay_ms(100);
  //res=((res&0x3ffff)>>3)*0.0625*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB, 10 -- output at 0.1C
  res=((res&0x3ffff)>>3)*0.03125*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB,
  
  twi_req_read(DSTERM_PREF,0xEE);
  twi_p_stop();
  return res;
}

//////////////////////////////////////////////////

void TWI(void) {
  Term tnew;

  tnew = term_MAX_get();
  TermUpdate(&term_base_int, tnew);

  tnew = term_DS_get();
  TermUpdate(&term_base_ext, tnew);

  {
    int i;
    for (i=0; i<TERMS; i++){
      tnew=term_sw_DS_get(stwi, i);
      TermUpdate(&terms_extend[i], tnew);
    }
  }

   /*
	int i;
	Term tnew;
		tnew = DS1621_get_shot(TERM_I2C_PREF | i); // 1001 111 // 3 | 7
	//	if(tnew.i != 0xFF){ // 128
			//termslist[i].num=i;
		TermUpdate(&termslist[i], (tnew.i<<1) | (tnew.f>>7));
	//	}
	} */
}

void UART(void) {
  int char;
  char=trych();

}

void PrintTerm(terms *t, char *desk) {
  int port,inv;

  port=t->port&0x0F;
  inv =t->port>>4;

  if (t->cur==TERM_MAX) {
    printf("%s: XXX (term don't work)\n",desk);
    return;
  }

  printf("%s: %i (min: %i, max: %i, last %i, port %i",
      desk,(int)t->cur,t->min,t->max,t->last,port);

  if (inv) 
    printf("inverted)\n");
  else
    printf(")\n");
}


void INFO(void) {
  int i;
  char buff[7]="Term ";

  printf("Base term:\n");
  PrintTerm(&term_base_int, "BaseInt");
  PrintTerm(&term_base_ext, "BaseExt");

    
  printf("Ext term:\n");
  for(i=0;i<TERMS;i++) {
    buff[5]='0'+i;
    PrintTerm(&terms_extend[i], buff);
  }
}

int main(void) {
  unsigned int i;

  cli();
  uart_init();
  stdioconf_stdio();

  twi_init();
  adc_init();

  twisw_init(stwi);

  DDRC=0x00;
  DDRB=0xff;
  DDRD=0xff;
  PORTB=3;
  PORTD=1<<3;

  //OC1A -- PB1

#define LED_OFF   PORTB|= 1
#define LED_ON    PORTB&=~1
  printf("\nHELLO!\n");
  i=0;
  while (1) {

    LED_ON;
    
    TWI();
    UART();

    INFO();

    i++;
    
    LED_OFF;
    _delay_ms(2000);
  }

  return 0;
}
