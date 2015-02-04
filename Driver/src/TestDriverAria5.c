#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "AltSpecsV2.h"
 int Card;
  unsigned int DmaSize=0x100;
 unsigned char Mem[65536];
double DiffTime(struct timeval *heure_depart,struct  timeval *heure_fin);

static void hdl (int sig,siginfo_t *info ,void *unused )
{
  //  printf ("Sending signal %d\n",sig);
  return;
}

void PrintDatas(unsigned long int *p)
{
  int i;
  //  return;
  static int last=0;
  for (    i=0;i <DmaSize / (sizeof(*p)) ;i++)
    { 
         printf("%08X ",p[i]);
      
      if (!( i%16)) printf("\n");
    }
  
  if ((last+1) != p[0])
    printf("missing paquet  p[0]=%d\n",p[0]);
  last=p[0];
  
}
void ReadDataByPolling()
{  struct timeval Heure_depart;
  struct timeval Heure_fin;
  struct timezone Tz;
 int i=0;
  int j=0;
  int k=0;
  int cnt;
  double Time=0.0;
 
  unsigned long int *p;
  int rc=0;
 memset (Mem,0,DmaSize);
  printf("\n__________________ BY POLING _____________________________________\n");

  for (k=0;k<10;k++)
    {
      printf("Tranfert by pooling number %d\n",k);
      gettimeofday(&Heure_depart,&Tz); 
      for ( j=0; j < 10000;j++)
	{
	  rc = ioctl (Card,DMA_SET_SIZE,DmaSize);
	  
	  rc = ioctl (Card,DMA_GO,GO);	     
	  
	  do {
	    rc = ioctl (Card,GET_STATE_END_DMA);
	    //  usleep(10);
	    
	  }while (rc != 1);
	  // printf(" dma fini %d\n",rc);
	  
	  
	  cnt=read(Card,Mem,DmaSize);
	  
	  
	  p=(unsigned long int *)Mem ;
	  //  PrintDatas(p);
	  
	}
      gettimeofday(&Heure_fin,&Tz);
      Time = DiffTime(&Heure_depart,&Heure_fin);
      
      printf(" >>> Time = %lf nbOctect %lf Mo  Debit obtenu %lf Mo/s\n",
	     Time,(double)(DmaSize)*j / 1000000.0,(double)(DmaSize)*j/(double)(Time*1000000.0));
      
      // printf("last paquet counter %d\n",p[0]);
    }

}

void ReadDataByInterrupt()
{ struct timeval Heure_depart;
  struct timeval Heure_fin;
  struct timezone Tz;
 int i=0;
  int j=0;
  int k=0;
  int cnt;
  double Time=0.0;

  unsigned long int *p;
   int rc=0;

   printf("\n_________________________ BY IRQ  ______________________________\n");

  memset (Mem,0,DmaSize);
  for (k=0;k<10;k++)
    {
  printf("Tranfert by IRQ number %d\n",k);
  gettimeofday(&Heure_depart,&Tz); 
    for ( j=0; j < 10000;j++)
	  {
	rc = ioctl (Card,DMA_SET_SIZE,DmaSize);
       
	    rc = ioctl (Card,DMA_GO,GO);
	   
	    cnt=read(Card,Mem,DmaSize);
	    
	    p=(unsigned long  int *)Mem ;
	    //   PrintDatas(p);
	  
	  }
	gettimeofday(&Heure_fin,&Tz);
	Time = DiffTime(&Heure_depart,&Heure_fin);

	printf(" >>> Time = %lf nbOctect %lf Mo  Debit obtenu %lf Mo/s\n",
	       Time,(double)(DmaSize)*j / 1000000.0,(double)(DmaSize)*j/(double)(Time*1000000.0));
	
	//printf("last paquet counter %d\n",p[0]);
    }
rc = ioctl (Card,DMA_DISABLE_IRQ,DISABLE);
 return;

}
void InitSpecs()
{
 
  Card = open("/dev/AltSpecsV20", O_RDWR);

 
  if (Card < 0) {

    if ( errno  == 199 ) printf(" \n Version du driver non compatible avec le firmware \n \n"); 
    else  printf("ouverture device alterapciechdma impossible try dmesg \n");
    
    close(Card);
    exit(0);
	}
}
void InitDmaTransfertByInterrupt()
{
  int rc=0;
  rc = ioctl (Card,GET_STATE_END_DMA);
  
  if(rc>1) 
    {	printf(" dma en cours .. reset %d\n",rc);
      rc = ioctl (Card, DMA_SOFT_RESET,SOFT_RESET);
      exit(0);
      
    }
  rc = ioctl (Card,DMA_SET_SIZE,DmaSize);
  rc = ioctl (Card,SET_CONF_TRANSLATION_TABLE);
  rc = ioctl (Card, DMA_TRANSACTION , LENGHT_ENDS_TRANSACTION);
  //rc = ioctl (Card,DMA_TRANSFERS_TYPE,DOUBLE_WORD );
    rc = ioctl (Card,DMA_TRANSFERS_TYPE,QUAD_WORD );
  rc = ioctl (Card,DMA_ENABLE_IRQ,ENABLE);
  //		rc = ioctl (Card,DMA_DISABLE_IRQ,DISABLE);
  rc = ioctl (Card,DMA_FROM_CHIP_SOURCE_ADDRESS,CHIP_MEM_BASE0);

  return;
}
void InitDmaTransfertByPolling()
{
  int rc=0;
  rc = ioctl (Card,GET_STATE_END_DMA);
  
  if(rc>1) 
    {	printf(" dma en cours .. reset %d\n",rc);
      rc = ioctl (Card, DMA_SOFT_RESET,SOFT_RESET);
      exit(0);
      
    }
  rc = ioctl (Card,DMA_SET_SIZE,DmaSize);
  rc = ioctl (Card,SET_CONF_TRANSLATION_TABLE);
  rc = ioctl (Card, DMA_TRANSACTION , LENGHT_ENDS_TRANSACTION);
  rc = ioctl (Card,DMA_TRANSFERS_TYPE,QUAD_WORD );
  //   rc = ioctl (Card,DMA_TRANSFERS_TYPE,DOUBLE_WORD );
  rc = ioctl (Card,DMA_DISABLE_IRQ,DISABLE);
  rc = ioctl (Card,DMA_FROM_CHIP_SOURCE_ADDRESS,CHIP_MEM_BASE0);
  printf("fin init Polling \n");
  return;
}


void InitDmaTransfertToDevice()
{
  int rc=0;
  unsigned long int *p;

  int i;
  rc = ioctl (Card,GET_STATE_END_DMA);
  
  if(rc>1) 
    {	printf(" dma en cours .. reset %d\n",rc);
      rc = ioctl (Card, DMA_SOFT_RESET,SOFT_RESET);
      exit(0);
      
    }
  
  rc = ioctl (Card,SET_CONF_TRANSLATION_TABLE);
  rc = ioctl (Card, DMA_TRANSACTION , LENGHT_ENDS_TRANSACTION);
  //  rc = ioctl (Card,DMA_TRANSFERS_TYPE,DOUBLE_WORD );
     rc = ioctl (Card,DMA_TRANSFERS_TYPE,QUAD_WORD );
  rc = ioctl (Card,DMA_DISABLE_IRQ,DISABLE);
 
 

  rc = ioctl (Card,DMA_TO_CHIP_DESTINATION_ADDRESS,CHIP_MEM_BASE0);// not used
  rc = ioctl (Card,DMA_SET_SIZE,DmaSize);
  
  printf("Memoire initialisee avec des mots de %d bytes \n", sizeof (*p));
  p=( unsigned long int *)Mem;
  for (i=0; i< (DmaSize / sizeof (*p));i++)
    {
      *p=(unsigned long int )i*0x1000;

      p++;
    }
  
  write(Card,Mem,DmaSize);    

   rc = ioctl (Card,DMA_GO,GO);
 
	  do {
	    rc = ioctl (Card,GET_STATE_END_DMA);
	      usleep(10);
	    
	  }while (rc != 1);
	  printf(" dma fini %d\n",rc);
  	  
  return;
}
void InitDmaTransfertChipToChip()
{
  int rc=0;
 
  rc = ioctl (Card,GET_STATE_END_DMA);
  
  if(rc>1) 
    {	printf(" dma en cours .. reset %d\n",rc);
      rc = ioctl (Card, DMA_SOFT_RESET,SOFT_RESET);
      exit(0);
      
    }
  
  rc = ioctl (Card,SET_CONF_TRANSLATION_TABLE);
  rc = ioctl (Card, DMA_TRANSACTION , LENGHT_ENDS_TRANSACTION);
   rc = ioctl (Card,DMA_TRANSFERS_TYPE,QUAD_WORD );
   //rc = ioctl (Card,DMA_TRANSFERS_TYPE,DOUBLE_WORD );
  rc = ioctl (Card,DMA_DISABLE_IRQ,DISABLE);
 
  rc = ioctl (Card,DMA_CHIP_TO_CHIP_SOURCE_ADDRESS,CHIP_MEM_BASE0); 

  rc = ioctl (Card,DMA_CHIP_TO_CHIP_DESTINATION_ADDRESS,SPECSM0_MEM);
  rc = ioctl (Card,DMA_SET_SIZE,DmaSize);
  
  memset (Mem,37,DmaSize);
  write(Card,Mem,DmaSize);    

   rc = ioctl (Card,DMA_GO,GO);
 
	  do {
	    rc = ioctl (Card,GET_STATE_END_DMA);
	      usleep(10);
	    
	  }while (rc != 1);
	  printf(" dma fini %d\n",rc);
  	  
  return;
}
/*-----------------------------------------------------------------------------------------------------*/
double DiffTime(struct timeval *heure_depart,struct  timeval *heure_fin)
/*-----------------------------------------------------------------------------------------------------*/
{
  double val;
  if (heure_fin->tv_sec != heure_depart->tv_sec)
    {
      if (heure_fin->tv_usec > heure_depart->tv_usec)
        {
	  //  printf(" %ld.%06ld \n",heure_fin->tv_sec-heure_depart->tv_sec,heure_fin->tv_usec-heure_depart->tv_usec);
          val=(double) (heure_fin->tv_sec-heure_depart->tv_sec)+((double)(heure_fin->tv_usec-heure_depart->tv_usec)/1000000.0);
        }
      else
        {
	  //   printf(" %ld.%06ld \n ",(heure_fin->tv_sec-heure_depart->tv_sec)-1,(1000000-heure_depart->tv_usec)+heure_fin->tv_usec);
          val=(double)(heure_fin->tv_sec-heure_depart->tv_sec)-1.0+(double)((double)((1000000.0-(double)(heure_depart->tv_usec)+(double)(heure_fin->tv_usec))/1000000.0));
        }
    }
  else
    {
      //    printf(" %ld.%06ld \n ",heure_fin->tv_sec-heure_depart->tv_sec,heure_fin->tv_usec-heure_depart->tv_usec);
      val=(double)( heure_fin->tv_sec-heure_depart->tv_sec)+(double)((double)(heure_fin->tv_usec-heure_depart->tv_usec)/1000000.0);
    }
 
 return(val);
}
 
int main() {

  struct sigaction act;
  
  
  memset (&act, '\0', sizeof(act));
  
  
  act.sa_sigaction = &hdl;
  sigemptyset(&act.sa_mask);
  sigaddset(&act.sa_mask, SIGINT);
  
  act.sa_flags = SA_SIGINFO;
  
  if (sigaction(SIGTEST, &act, NULL) < 0) {
    perror ("sigaction");
    return 1;
  }	
  
  printf("user progran altera PCIE Card\n");
  InitSpecs();
     InitDmaTransfertToDevice();
          InitDmaTransfertByInterrupt();
  
       ReadDataByInterrupt();
  
  
 
  
 
        InitDmaTransfertByPolling();
       ReadDataByPolling();
  
  //sleep(10);

    	 InitDmaTransfertChipToChip();

  ioctl (Card,WRITE_IN_PIO_IN,GO); 
  close(Card);
  exit(0);

}
