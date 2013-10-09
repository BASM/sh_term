#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <twi_sw.h>

#define SCL_NUM  0
#define SCL_DDR  DDRC
#define SCL_PORT PORTC
#define SCL_PIN  PINC

#define SDA_NUM  1
#define SDA_DDR  DDRC
#define SDA_PORT PORTC
#define SDA_PIN  PINC

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
  
static void s_cycle_wait(void *data) {
  _delay_us(1000);
}

static int s_scl_rl(void *data) {
  //PC0 
  SCL_DDR &=~(1<<SCL_NUM);

  //while (((SCL_PIN>>SCL_NUM)&0x01)==0) {
  //  printf("W");
  //}
  return 0;
}


void twisw_init(twi_data *stwi) {
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
}
