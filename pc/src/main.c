#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>



#define TTY "/dev/ttyUSB0"

static int tty_CTRL(int fdtty, int sercmd, int stat) {
  if (stat==0) return ioctl(fdtty, TIOCMBIC, &sercmd);
  else         return ioctl(fdtty, TIOCMBIS, &sercmd);
  return -1;
}


int setterminal(int fd) {
  //Term in NON block
  struct termios newfd;
  tcgetattr(fd,&newfd);

  cfmakeraw(&newfd);
  //c_cc[VMIN]) and TIME (c_cc[VTIME]
  newfd.c_cc[VMIN]=1;
  newfd.c_cc[VTIME]=1;
  newfd.c_cflag|=CSTOPB;
  //newfd.c_cflag&=~CSTOPB;
  tcsetattr(fd,TCSANOW,&newfd);

  return 0;
}


int main(void) {
  int res;
  
  printf("Prog\n");

  int fdn = open(TTY,O_RDWR);
  //open
  if (fdn<0) {
    perror("");
    return 1;
  }
  setterminal(fdn);
  tty_CTRL(fdn,TIOCM_DTR,0);

  FILE* fd = fdopen(fdn,"r+");
  if (fd == NULL) {
    perror("");
    return 1;
  }

  sleep(2);

  /* 
   * i -- Get info Human Readable
   * I -- Get info in raw data
   */

  //fprintf(fd,"ATi\n");

  ///////////////////////////////

  //fprintf(fd,"ATt7:240,250,16\n");
  //sleep(2);
  fprintf(fd,"ATtE:230,250,16\n");


  return 0;
}
