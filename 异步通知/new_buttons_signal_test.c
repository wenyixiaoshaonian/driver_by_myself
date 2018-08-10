#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>

#include <sys/types.h>    
#include <unistd.h>       // getpid
#include <fcntl.h>        // fcnt1


int fd;

void signal_func (int sigsum)
{
  static int cet = 0;
  unsigned char key_val;
  read (fd,&key_val,1);
  printf("signal:%d,times:%d\n",sigsum,++cet);
  printf("key_val:ox%x\n",key_val);
}



int main(int argc,char **argv)
{

  signal(SIGIO,signal_func);
  int Oflags;
  fd = open("/dev/buttons",O_RDWR);
  if (fd < 0)
    {
        printf("error, can't open \n");
    }
  fcntl(fd, F_SETOWN, getpid());  //获取PID号
	  
  Oflags = fcntl(fd, F_GETFL);   // 获取Oflags
	  
  fcntl(fd, F_SETFL, Oflags | FASYNC);  //将Oflags修改为异步通知


  while(1)
  	{
  	  sleep(1000);
  	}
  return 0;

}





