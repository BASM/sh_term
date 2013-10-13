#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>

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

//////// UART CMD ///////////
typedef enum {
  AT_NONE,
  AT_A,
  AT_AT,
  AT_CMDOK,
} AT_CMD;

AT_CMD AT=AT_NONE;

#define UART_BUFF_SIZE 64
uint8_t uart_buff[UART_BUFF_SIZE];
uint8_t uart_buff_i;
uint8_t cbyte;

ISR(USART_RX_vect) {
  //if (uart_buff_i>=UART_BUFF_SIZE) return;
  cli();
  cbyte=UDR0;

  switch(AT) {
    case AT_NONE:
      if (cbyte=='A') {
        AT=AT_A;
        UDR0=cbyte;
      }
      break;
    case AT_A:
      if (cbyte=='T') { 
        uart_buff_i=0;
        AT=AT_AT;
        UDR0=cbyte;
      } else {
        if (cbyte=='A')  UDR0=cbyte;
        else             AT=AT_NONE;
      }
      break;
    case AT_AT: 
      if ((cbyte == '\r') || (cbyte == '\n') )
             AT=AT_CMDOK;
      else   uart_buff[uart_buff_i++]=cbyte;
      UDR0=cbyte;
      break;
    case AT_CMDOK:
      //do nothing, wait 
      //changing to AT_NONE from user program.
      break;
  }

  sei();
}

void uart_ATmode()
{
  uart_buff_i=0;
  UCSR0B |= (1 << RXCIE0);
}
//////////////////////////


twi_data stwi_obj;
twi_data *stwi=&stwi_obj;


static void INFO(void);

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

static terms* parce_term(uint8_t *str) {
  uint8_t type=str[0];

  if ( (type>='0') && (type<='9')) {
      uint8_t en=type-'0';
      if ( (en>=0) && (en<TERM_MAX) ) {
        printf("Ext timer: %i\n",en);
        return &terms_extend[en];
      }
      return NULL;
  }
  switch (type) {
    case 'I': 
      {
        printf("Internal timer\n");
        return &term_base_int;
      }
      break;
    case 'E':
      {
        printf("External timer\n");
        return &term_base_ext;
      }
      break;
  }
  return NULL;
}
void PrintTerm(terms *t, char *desk);

void TERMWORK() {
  terms* term=parce_term(&uart_buff[1]);
  if (term!=NULL) {
    uint16_t min,max;
    uint8_t port;

    char *p,*e=NULL;
    uint16_t res=0;
    p=(char*)&uart_buff[3];
    res=strtol(p,&e,10);
    if (p!=e) min=res;
    else min=TERM_MAX;
    if (*p=='\0') return;

    p=e+1;
    if (*p=='\0') return;
    res=strtol(p,&e,10);
    if (p!=e) max=res;
    else max=TERM_MAX;
    if (*p=='\0') return;

    p=e+1;
    if (*p=='\0') return;
    res=strtol(p,&e,10);
    if (p!=e) port=res;
    else port=-1;

    term->min=min;
    term->max=max;
    term->port=port;
    printf("Save to %i\n", term->num);
    TermSave(term, term->num);

    PrintTerm(term,"Term set");
  }
}

void UART(void) {

  if (AT==AT_CMDOK) {
    uart_buff[uart_buff_i]='\0';
    AT=AT_NONE;
  } else {
    return;
  }

  switch(uart_buff[0]) {
    case 'i': INFO(); break;
    case 't': TERMWORK(); break;
  }
}

void PrintTerm(terms *t, char *desk) {

  printf("%s: ",desk);

  if (t->cur==TERM_MAX) printf("XXX ");
  else                  printf("%i ", (int)t->cur);

  if (t->min==TERM_MAX) printf("(min not set, ");
  else                  printf("(min: %i, ", (unsigned int)t->min);

  if (t->max==TERM_MAX)  printf("max not set, ");
  else                   printf("max: %i, ",    (unsigned int)t->max);

  if (t->port==(uint8_t)-1)
    printf("port not set)\n");
  else {
    int inv =t->port>>4;
    printf("port %i", (unsigned int)t->port%16);
    if (inv)  printf(" inv,");
    else      printf(",");

    if (t->stat) printf("on)\n");
    else         printf("off)\n");
  }
}


static void INFO(void) {
  int i;
  char buff[7]="Term ";

  printf("Base term:\n");
  PrintTerm(&term_base_int, "BaseInt");
  PrintTerm(&term_base_ext, "BaseExt");

  printf("Ext term:\n");
  for (i=0;i<TERMS;i++) {
    buff[5]='0'+i;
    PrintTerm(&terms_extend[i], buff);
  }
}
///////// UART ////////////


////////////////////////

#define LED_OFF   PORTB|= 1
#define LED_ON    PORTB&=~1
int main(void) {
  unsigned int i;

  cli();
  LED_ON;
  uart_init();

  uart_ATmode();

  stdioconf_stdio();

  twi_init();
  adc_init();

  twisw_init(stwi);

  DDRC=0x00;
  DDRB=0xff;
  DDRD=0xff;
  PORTB=3;
  //PORTD=1<<3;

  //OC1A -- PB1

  printf("\nHELLO!\n");
  i=0;
  sei();
  LED_OFF;

  {
    int i;

    term_base_int.num=10;
    TermLoad(&term_base_int, 10);
    term_base_int.stat=-1;

    term_base_ext.num=11;
    TermLoad(&term_base_ext, 11);
    term_base_ext.stat=-1;

    for (i=0; i<TERMS; i++) {
      terms_extend[i].num=i;
      TermLoad(&terms_extend[i], i);
      terms_extend[i].stat=-1;
    }
  }

  while (1) {
    i++;

    LED_ON;
    TWI();
    UART();

    LED_OFF;
    _delay_ms(500);
  }

  return 0;
}
