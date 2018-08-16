#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <linux/fb.h>
  
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "test1.h"
#include "test2.h"


static int FBCleanScreen(unsigned int dwBackColor);     /* 清屏函数，把显示屏初始化为黑色 */
static int FBShowPixel(int iX, int iY, unsigned int dwColor);   /* 填充像素，dwcolor就是我们要在一个像素显示的颜色 */

#define FB_DEVICE_NAME "/dev/fb0"
#define DBG_PRINTF printf

static int g_fd;
static int f_fd;

static struct fb_var_screeninfo g_tFBVar;
static struct fb_fix_screeninfo g_tFBFix; 
static unsigned char *g_pucFBMem;
static unsigned int   g_dwScreenSize;

static unsigned int g_dwLineWidth;
static unsigned int g_dwPixelWidth;
const unsigned char gImage_test1[1039120];
const unsigned char gImage_test2[261128];


static int i = 0 ;

static int FBShowPixel(int iX, int iY, unsigned int dwColor)  
{
  unsigned char *pucFB;
  unsigned short *pwFB16bpp;
  unsigned short wColor16bpp; /* 565 */
  int iRed;
  int iGreen;
  int iBlue;

  if ((iX >= g_tFBVar.xres) || (iY >= g_tFBVar.yres))
    {
      DBG_PRINTF("out of region\n");
      return -1;
    }

  pucFB      = g_pucFBMem + g_dwLineWidth * iY + g_dwPixelWidth * iX;
  pwFB16bpp  = (unsigned short *)pucFB;

  iRed   = (dwColor >> 11) & 0x1f;
  iGreen = (dwColor >> 5) & 0x3f;
  iBlue  = (dwColor >> 0) & 0x1f;
  wColor16bpp = (iRed << 11) | (iGreen << 5) | iBlue;
  *pwFB16bpp = wColor16bpp;

  return 0;

}



static int FBCleanScreen(unsigned int dwBackColor)
{
  unsigned char *pucFB;
  unsigned short *pwFB16bpp;
  unsigned short wColor16bpp; /* 565 */
  int iRed;
  int iGreen;
  int iBlue;
  int i = 0;

  pucFB      = g_pucFBMem;
  pwFB16bpp  = (unsigned short *)pucFB;

  iRed   = (dwBackColor >> (16+3)) & 0x1f;
  iGreen = (dwBackColor >> (8+2)) & 0x3f;
  iBlue  = (dwBackColor >> 3) & 0x1f;
  wColor16bpp = (iRed << 11) | (iGreen << 5) | iBlue;
  while (i < g_dwScreenSize)
    {
      *pwFB16bpp = wColor16bpp;
       pwFB16bpp++;
       i += 2;
    }
  printf("finished!\n");
}


int main(int argc,char **argv)
{
  int j,k,m = 0;
  int ret;
  unsigned int dwColor;
  g_fd = open(FB_DEVICE_NAME, O_RDWR);   /* 打开lcd驱动设备节点 */
  if (0 > g_fd)
    {
      DBG_PRINTF("can't open %s\n", FB_DEVICE_NAME);
    }
  ret = ioctl(g_fd, FBIOGET_VSCREENINFO, &g_tFBVar);   /* 获取lcd可变参数 */
  if (ret < 0)
    {
      DBG_PRINTF("can't get fb's var\n");
      return -1;
    }
  ret = ioctl(g_fd, FBIOGET_FSCREENINFO, &g_tFBFix);   /* 获取lcd固定参数 */
  if (ret < 0)
    {
      DBG_PRINTF("can't get fb's fix\n");
      return -1;
    }
  g_dwScreenSize = g_tFBVar.xres * g_tFBVar.yres * g_tFBVar.bits_per_pixel / 8;  /* 计算lcd屏幕大小 */
  g_pucFBMem = (unsigned char *)mmap(NULL , g_dwScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fd, 0);   /* 把显存映射成内存一样*/
  if (0 > g_pucFBMem)
    {
      DBG_PRINTF("can't mmap\n");
      return -1;
    }

  g_dwLineWidth  = g_tFBVar.xres * g_tFBVar.bits_per_pixel / 8;
  g_dwPixelWidth = g_tFBVar.bits_per_pixel / 8;


  FBCleanScreen(0x000000);      //清屏函数，把显示屏初始化为黑色
  //480*272,将图片数据写入LCD	
      for(j=0;j<272;j++)
      	{
      	for(k=0;k<480;k++)
      	  {
/*      	  
            if(m<=160) 
			  dwColor = (0xf8<<8) + (0x00<<0);
            if((m>160) && (m<320))
				dwColor = (0x07<<8) + (0xe0<<0);
			if(m>=320)
				dwColor = (0x00<<8) + (0x1f<<0);       //三色程序
*/			
              dwColor = (gImage_test2[i+1]<<8) + (gImage_test2[i]<<0);
              FBShowPixel(k,j, dwColor);     // 将未处理过的颜色数据写到LCD去
	          i = i+2;
			  m++;
	        
  	      }
		m = 0;

		
  	    }
  return 0;

}


