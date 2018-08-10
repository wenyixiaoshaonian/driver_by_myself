#include <stdio.h>

#include <signal.h>

void signal_func (int sigsum)
{
  static int cet = 0;
  printf("signal:%d,times:%d\n",sigsum,++cet);
}

int main (int argc,char **argv)
{
   signal(SIGUSR1,signal_func);
   while(1)
   	{
      sleep(1000);
   	}
   return 0;
}

