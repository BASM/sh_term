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
		case 0: real=5; break;
		case 1: real=1; break;
		case 2: real=2; break;
		case 3: real=3; break;
		case 4: real=4; break;
		case 5: real=0; break;
		default:
			return 0;
	}
	num=real;

	PORTB=((PORTB&(~(1<<num))) | (s<<num));

  return 0;
}

int TermLoad(terms* T, int num)
{
	uint8_t *EEaddress;
	EEaddress=(void*)EE_FROM_TERM(num);	 
	T->min = eeprom_read_byte(EEaddress+0);
	T->max = eeprom_read_byte(EEaddress+1);
  T->port= eeprom_read_byte(EEaddress+2);
	
	if( (T->min==-1) && (T->max==-1) && (T->port==0xFF)){ 
		T->num=0xFF;
		return 1;
	}
	T->num=num;
	
	return 0;
}

void TermSave(terms* T, uint8_t num)
{
	uint8_t *EEaddress;
	if(num>=8)
		return;

	EEaddress=(void*)EE_FROM_TERM(num);	 

	eeprom_write_byte(EEaddress+0,T->min);
	eeprom_write_byte(EEaddress+1,T->max);
	eeprom_write_byte(EEaddress+2,T->port);
}

void TermUpdate(terms* T, int16_t cur)
{
	T->cur = cur;
	if(T->max <= T->min)
		return;
	if(T->num > 8)
		return;
  if(T->cur == TERM_MAX)
    return;

	if(cur>=T->max) // Температура слижком высокая
	{
		TermPort(T->port, 0); // Включить 
		T->last=cur;
	}
	if(cur<=T->min) // Температура слижком низкая
	{	
		TermPort(T->port, 1); // Включить 
		T->last=cur;
	}
}
