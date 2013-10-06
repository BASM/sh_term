#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <uart.h>
#include <twi.h>
#include <adc.h>
#include <twi_sw.h>

#define MAXTERM_PREF 0x4f
#define DSTERM_PREF   0x4d
#define DSTERMSW_PREF 0x4f
#define HUMI_CHAN 0x07

static int getcurterm() {
  int res;
  
  if (twi_req_read(MAXTERM_PREF,0x00)) {
    twi_p_stop();
    return -1;
  }
  res=twi_p_read(0)<<8;
  res|=twi_p_read(1);
  twi_p_stop();

  //res=((res&0x3ffff)>>3)*0.0625*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB, 10 -- output at 0.1C
  res=((res&0x3ffff)>>3)*0.0625*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB,
  return res;
}

static int getcurtermSDS(twi_data *twi) {
  int res;
  
  printf("RR\n");
  if (twi_sw_req_read(twi,DSTERMSW_PREF,0xAA)) {
    return 0;
  }
  res =twi_sw_read(twi,0)<<8;
  res|=twi_sw_read(twi,1);
  twi_sw_stop(twi);

  //res=((res&0x3ffff)>>3)*0.0625*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB, 10 -- output at 0.1C
  res=((res&0x3ffff)>>3)*0.03125*10;// 0x3ffff --remove sign, >>3-- remove LSB bit, 0.0625 -- 1LSB,
  
  twi_sw_req_read(twi,DSTERMSW_PREF,0xEE);
  twi_sw_stop(twi);
  return res;



  return 0;
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

static void s_cycle_wait(void *data) {
  _delay_us(100);
}

#define SCL_NUM  0
#define SCL_DDR  DDRC
#define SCL_PORT PORTC
#define SCL_PIN  PINC

#define SDA_NUM  1
#define SDA_DDR  DDRC
#define SDA_PORT PORTC
#define SDA_PIN  PINC

static int s_scl_rl(void *data) {
  //PC0 
  SCL_DDR &=~(1<<SCL_NUM);

  //while (((SCL_PIN>>SCL_NUM)&0x01)==0) {
  //  printf("W");
  //}
  return 0;
}

static void s_scl_dn(void *data) {
  SCL_DDR |= 1<<SCL_NUM;
}

static void s_sda_rl(void *data) {
  SDA_DDR &=~(1<<SDA_NUM);
}

static void s_sda_dn(void *data) {
  SDA_DDR |= (1<<SDA_NUM);
}

static int s_sda_read(void *data) {

  return !((SDA_PIN>>SDA_NUM)&0x01);
}

static int s_scl_read(void *data) {

  return !((SCL_PIN>>SCL_NUM)&0x01);
}


int main(void) {
  int term;
  int term1;
  int term2;
  //int adc;
  unsigned int i;

  cli();
  uart_init();
  stdioconf_stdio();

  twi_init();
  adc_init();

  printf("HEllo!!\n");

  twi_data stwi_obj;
  twi_data *stwi=&stwi_obj;

  //////////////////////////////
  /////// SOFTWARE TWI /////////
  SCL_PORT &=~(1<<SCL_NUM);//
  SDA_PORT &=~(1<<SDA_NUM);//
  stwi->scl_rl=s_scl_rl;
  stwi->scl_dn=s_scl_dn;
  stwi->sda_rl=s_sda_rl;
  stwi->sda_dn=s_sda_dn;
  stwi->sda_read=s_sda_read;
  stwi->scl_read=s_scl_read;
  stwi->cycle_wait=s_cycle_wait;
  twi_sw_init(stwi,NULL);
  //////////////////////////////


  DDRC=0x00;
  DDRB=0xff;
  DDRD=0xff;
  PORTB=3;
  PORTD=1<<3;

  //OC1A -- PB1

  printf("HELLO!\n");
  i=0;
  while (1) {
    i++;

    printf("CYCLE START! %i\n", i);
    PORTB=0;
    //PORTD=0;
    _delay_ms(50);
    PORTB=3;
    PORTD=1<<3;
    _delay_ms(2000);

    printf("CURRENT 1\n");
    term=getcurterm();
    printf("CURRENT 2\n");
    term1=getcurtermDS();
    
    printf("CURRENT 3\n");
    term2=getcurtermSDS(stwi);
    printf("Current term is: term %i, term1 %i, swterm: %i\n", term, term1, term2);
  }


  return 0;
}
