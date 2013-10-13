#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <inttypes.h>

#include "terms.h"
terms term_base_int;
terms term_base_ext;
terms terms_extend[TERMS]; 


int TermPort(uint8_t num, uint8_t s)
{
	if (s > 1)
		s=1;

	if (num&0x10){ // 0b 0001 0000
		s=s?0:1;     // REVERS MODE
	}
	
	num=num&0x0F;
	int real;
	switch(num){
		case 0: real=3; break;
		case 1: real=1; break;
		case 2: real=2; break;
		case 3: real=3; break;
		case 4: real=4; break;
		case 5: real=5; break;
		default:
			return 0;
	}
	num=real;

	PORTD=((PORTD&(~(1<<num))) | (s<<num));

  return 0;
}

int TermLoad(terms* T, int num)
{
	uint16_t *EEaddress;
	EEaddress=(void*)EE_FROM_TERM(num);	 
	T->min = eeprom_read_word(EEaddress+0);
	T->max = eeprom_read_word(EEaddress+1);
  T->port= eeprom_read_word(EEaddress+2);
  
	return 0;
}

void TermSave(terms* T, uint8_t num)
{
	uint16_t *EEaddress;
	//if (num>=8)
	//	return;

	EEaddress=(void*)EE_FROM_TERM(num);	 

	eeprom_write_word(EEaddress+0,T->min);
	eeprom_write_word(EEaddress+1,T->max);
	eeprom_write_word(EEaddress+2,T->port);
}

void TermUpdate(terms* T, int16_t cur)
{
	T->cur = cur;
  if (T->cur == TERM_MAX)
    return;
  if (T->port == (uint8_t)-1)
    return;

  if (T->max != TERM_MAX)
    if (cur>=T->max) // Температура слижком высокая
      if (T->stat!=0) {
        printf("OFF from %i\n",T->num);
        TermPort(T->port, 0); // Включить 
        T->stat=0;
      }

  if (T->min != TERM_MAX)
    if (cur<=T->min) // Температура слижком низкая
      if (T->stat!=1) {
        printf("ON from %i\n",T->num);
        TermPort(T->port, 1); // Включить 
        T->stat=1;
      }
}

