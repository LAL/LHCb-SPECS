#include <stdarg.h>            // For va_start/va_end
#include "linux/pci.h"
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "AltSpecsV2.h"
#include "ConsoleUtilities.h"
#include "Plx.h" 
#include <sys/time.h>

/**
 * @details Close Handle of the master 
 * @param      handle of the master 
 * @retval  RETURN_CODE ApiSuccess
 *
 */
struct timeval Heure_depart;
struct timeval Heure_fin;
struct timezone Tz;
static long long nbOctets=0;
double Time=0;

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

          val=(double)(heure_fin->tv_sec-heure_depart->tv_sec)-1.0+(double)((double)((1000000.0-(double)(heure_depart->tv_usec)+(double)(heure_fin->tv_usec))/1000000.0));
        }
    }
  else
    {
      //    printf(" %ld.%06ld \n ",heure_fin->tv_sec-heure_depart->tv_sec,heure_fin->tv_usec-heure_depart->tv_usec);
      val=(double)( heure_fin->tv_sec-heure_depart->tv_sec)+(double)((double)(heure_fin->tv_usec-heure_depart->tv_usec)/1000000.0)
	;
    }
 
  return(val);
}

RETURN_CODE PlxPciDeviceClose(  HANDLE Handle ,U8 masterId   )
{ int err;

  if (Handle == -1) 
    {
      printf(" Master %d handle NULL \n",masterId);
      return ApiInvalidDeviceInfo;
    }
  err = ioctl(Handle,IOCTL_CLOSE_MASTER,masterId);
 
  if (err)
    {
      printf(" Master %d  Already close \n",masterId);
      return ApiInvalidDeviceInfo;
    }
  gettimeofday(&Heure_fin,&Tz);
  Time = DiffTime(&Heure_depart,&Heure_fin);
  nbOctets*=4;  // se sont des mots de 32 bits
  printf(" >>> Time = %lf nbOctect %lld   Debit obtenu %lf Mo/s\n",
	 Time,nbOctets ,(double)(nbOctets)  /(double)(Time*1000000.0));
 
  err= close(Handle);
  if (err < 0)  printf(" Master close err \n");
  return ApiSuccess;
}
/**
 * @details   Scan alldevices in /proc/bus/pci/devices aand search for  Altera Device board
 *  with the informationd given in pDevice 
 * @param   DEVICE_LOCATION *pDevice a structure with the Altera VendorId 0x1172 and deviceId 0x5  
 * @param   U32 pRequestLimit to return the number of board 
 * @retval  RETURN_CODE ApiSuccess if a board is present
 *                 else ApiInvalidDeviceInfo ;
 */
RETURN_CODE PlxPciDeviceFind( DEVICE_LOCATION *pDevice,
			      U32             *pRequestLimit    )
{
  char buf[1024];
  FILE *fptr;
  unsigned int bus, device, function;
  unsigned int  slot=0, svend;
  unsigned long  sbar =0;
  unsigned int TotalDevFound=0;
 
  if( (fptr = fopen( "/proc/bus/pci/devices", "r" )) == NULL )
    {
      perror( "Unable to open /proc/bus/pci/devices" );
      return ApiFailed;
    } 
  while( fgets( buf, sizeof(buf) - 1, fptr ) ) 
    {
      sscanf( buf, "%2x%2x %4x%4x %x %lx", &bus, &slot, &svend, &device,&function,&sbar); 
      
      if (pDevice->DeviceId == device )
	{
	  pDevice->BusNumber = bus;
	  pDevice->SlotNumber = slot >> 3;
	  pDevice->VendorId = svend;
	  TotalDevFound++;
	  
	}
    }
  fclose( fptr );
  *pRequestLimit = TotalDevFound;
  if (TotalDevFound == 0)
    {
      return ApiInvalidDeviceInfo;
    }
  else
    {
      return ApiSuccess; 
    }
  
  return ApiInvalidDeviceInfo;
}
/**
 * @details  open the device characters file /de/AltSpecsV2xx
 * make an important function: the reset of the FIFOs to make empty the 2 FIFOs
 * @param   DEVICE_LOCATION *pDevice a structure with the Altera VendorId 0x1172 and deviceId 0x5  
 * @param  HANDLE          *pHandle the file description id.
 * @retval  RETURN_CODE ApiSuccess if a board is presentif all is OK
 *                      ApiInvalidDriverVersion if the version of the firmware is not compatible with the loaded driver
 *                      ApiInvalidDeviceInfo if the FIFO are not cleared , try to re-execute the programm 
 */
RETURN_CODE  PlxPciDeviceOpen(  DEVICE_LOCATION *pDevice, 
				HANDLE          *pHandle,
				U8               masterId)
{
  RETURN_CODE rc =ApiNullParam;
  U32  pNumberDevice;
  char DeviceName[256];
  int i;
  int err;
   
  if ( (masterId> NB_MASTER)|| (masterId <0))
    {
      return ApiInvalidDeviceInfo;
    }
  rc= PlxPciDeviceFind( pDevice, &pNumberDevice    ); 

  if (rc != ApiSuccess)
    {
      *pHandle = 0;
      return rc;
    }
  for ( i= 0 ; i < pNumberDevice ; i++)
    {
      sprintf(  DeviceName, "/dev/AltSpecsV2Master0");
     
      *pHandle = open(DeviceName, O_RDWR);
      
      if (*pHandle < 0) {
	
	if ( errno  == 199 ) printf(" \n Version du driver non compatible avec le firmware \n \n");
	else  printf("  ouverture device %s impossible try dmesg \n",DeviceName);
	usleep (1000000);

 
	return ApiInvalidDriverVersion ;
      }
      
      rc=ioctl(*pHandle,IOCTL_FIND_DEVICE,pDevice);
     
      err = ioctl(*pHandle,IOCTL_OPEN_MASTER,masterId);
      if (err)
	{
	  printf(" Master Already in use (open) \n");
	  close(*pHandle);
	  return ApiInvalidDeviceInfo;
	}

      err=ioctl(*pHandle,IOCTL_FIFOS_INIT,masterId);

      
      if ( err )
	{
	  printf("erreur d'initialisation des FIFOS %d\n",err);
	  close(*pHandle);
	  return ApiInvalidDeviceInfo;
	} 
      nbOctets=0;
      gettimeofday(&Heure_depart,&Tz); 

     
      if (!rc) return ApiSuccess;
      
    } 
  sleep(10);
  return rc;
}

/**
 * @details  read data from the drivers at differents address ( registers)
 a specific action is made on the FIFO_RECEPTION_IN register we need to know how many bytes are 
 * realy present in the FIFO 
 * make an important function: the reset of the FIFOs to make empty the 2 FIFOs
 * @param   DEVICE_LOCATION *pDevice a structure with the Altera VendorId 0x1172 and deviceId 0x5  
 * @param  HANDLE          *pHandle the file description id.
 * @retval  RETURN_CODE ApiSuccess if a board is presentif all is OK
 *                      ApiInvalidDriverVersion if the version of the firmware is not compatible with the loaded driver
 *                      ApiInvalidDeviceInfo if the FIFO are not cleared , try to re-execute the programm 
 */
RETURN_CODE PlxBusIopRead(
			  HANDLE       Handle,
			  int           masterId,
			  U32          address,
			  BOOLEAN      bRemap,
			  VOID        *pBuffer,
			  U32          ByteCount,
			  ACCESS_TYPE  AccessType
			  )
{
  BUSIOPDATA IoBuffer;
  RETURN_CODE rc =ApiNullParam;
  U32 *val;
  int i;
  static int error=0;
  
  IoBuffer.MasterID     = masterId;
  IoBuffer.Address      = address;
  IoBuffer.bRemap       = bRemap;
  IoBuffer.TransferSize = ByteCount; //mtq
  IoBuffer.AccessType   = AccessType;
  IoBuffer.Buffer       = pBuffer;
  
  if (  (address & MASK_FIFO_IN )  ==  ( FIFO_RECEPTION_IN & MASK_FIFO_IN) )
    {
      for (i=0;i < 30;i++)
	{
	  rc=ioctl(Handle,IOCTL_READ_FILL_LEVEL,&IoBuffer);
	  val = (U32*) IoBuffer.Buffer;
	  if(*val <  ByteCount)
	    {
 	      usleep (1); 
	    }
	  else  {
	    break;
	  }
	  
	}
      // on attendu d'avoir le compte de mot si ce n4'est pas le cas en fin de boucle
      // on lit uniquement le compte de mot trouve 

      if (*val == 0) return ApiInvalidSize ;
      if (*val < IoBuffer.TransferSize) {
        IoBuffer.TransferSize = *val;
	// print enleve suite a la fonction JTAG qui de demande pas le compte de mot exact
	//   printf("FIFO pas avec le byteCount attendu %d recu %d\n",ByteCount,*val);   
     
      }
      nbOctets += (*val);
     
    }
  
  
  if (  (address & MASK_FIFO_IN )  ==  ( FIFO_RECEPTION_IN & MASK_FIFO_IN) )
    {
      
      rc=ioctl(Handle,IOCTL_BUS_IOP_READ,&IoBuffer);
      // error=read(Handle,IoBuffer.Buffer, IoBuffer.TransferSize);
    }
  else
    {
      error=ioctl(Handle,IOCTL_BUS_IOP_READ,&IoBuffer);
    }
  val = (U32*)IoBuffer.Buffer ;
  if (  (address & MASK_FIFO_IN )  ==  ( FIFO_RECEPTION_IN & MASK_FIFO_IN) )
    if (val[0]!= 0x76543210) 
      { //printf("error %d\n",error++);
	//	printf("PlxRead() %x should be  0x76543210 \n",val[0]);
	//       exit(0);
	return ApiInvalidData;
      }
  
  if (error <0) 
    return ApiNullParam;  
  else 
    return ApiSuccess;
}
RETURN_CODE PlxBusIopWrite( HANDLE       Handle,
			    int          masterId,
			    U32          address,
			    BOOLEAN      bRemap,
			    VOID        *pBuffer,
			    U32          ByteCount,
			    ACCESS_TYPE  AccessType
			    
			    )
{
  BUSIOPDATA IoBuffer;
  U32 *val;
  int error=0;
  int err=0;
  IoBuffer.MasterID     = masterId;
  IoBuffer.Address      = address;
  IoBuffer.bRemap       = bRemap;
  IoBuffer.TransferSize = ByteCount; 
  IoBuffer.AccessType   = AccessType;
  IoBuffer.Buffer       = pBuffer;
  if (  (address & MASK_FIFO_IN )  ==  ( FIFO_EMISSION_OUT & MASK_FIFO_IN) )
    //  error=write (Handle,pBuffer,ByteCount);
    error=ioctl(Handle,IOCTL_BUS_IOP_WRITE,&IoBuffer);
  else
    error=ioctl(Handle,IOCTL_BUS_IOP_WRITE,&IoBuffer);
  val = (U32*)IoBuffer.Buffer ;
 
  if (error == 1 ) 
    return ApiSuccess;  
  else 
    { 
      
      err=ioctl(Handle,IOCTL_RESET,masterId);
      
      if ( err )
	{
	  printf("erreur RESET des FIFOS %d\n",err);
	  
	} 
 
      return ApiInvalidSize; 
    }
}

RETURN_CODE PlxIntrEnable(
			  HANDLE       Handle,
			  PLX_INTR     *pInter,
			  U32 Register
			  )
{
  // les interuptions sont valides au niveau de la config PCI
  
  RETURN_CODE rc =ApiNullParam;
  INTERRUPT_OBJ  pIO_Int;
  // printf(" Validation des IRQ PIO %x \n", Register);
  
  pIO_Int.stat_reg=Register;
  pIO_Int.pio_line= pInter->PIO_Line;
  rc=ioctl(Handle,IOCTL_ENABLE_PIO_INTERRUPT,&pIO_Int);
  
  if (! rc )
    
    return ApiSuccess;
  else return ApiNullParam;
}

RETURN_CODE PlxIntrDisable(
			   HANDLE       Handle,
			   PLX_INTR     *pInter,
			   U32 statRegister
			   )
{
  // les interuptions sont valides au niveau de la config PCI
  
  RETURN_CODE rc =ApiNullParam;
  INTERRUPT_OBJ pIO_Int;
  pIO_Int.stat_reg=statRegister;
  //printf(" Devalidation des IRQ PIO %x \n", statRegister);
  pIO_Int.pio_line= pInter->PIO_Line;
  rc=ioctl(Handle,IOCTL_DISABLE_PIO_INTERRUPT,&pIO_Int);
  if (!rc) return ApiSuccess;
  else 
    return ApiNullParam;
}

RETURN_CODE PlxNotificationRegisterFor(
				       HANDLE handle,
				       PLX_INTR     *pPlxIntr,
				       unsigned short masterId
				       )
{  
        
  return ApiSuccess;
    
}

RETURN_CODE PlxNotificationWait(
				HANDLE  handle,
				unsigned short masterId,
				U64            Timeout_ms
				)
{  
  
  return ApiSuccess;

  /* On utilise plus les interruptions pour lir eles FIFOS  */

  
}
RETURN_CODE PlxNotificationCancel(
				  HANDLE handle,
				  U32 card
				  )
  
{ 
 
  return ApiSuccess;
 
}

RETURN_CODE PlxPciConfigRegisterRead(
				     U32 bus,
				     U32 slot,
				     U32 registerNumber,
				     U32* pData )
{
  
  // Lecture des registres de config du PCI sans interrets 
  // sert a retourner un id de la carte mais comme on de peu pas le modier ..
  // fonction non utilisée
  
  return ApiNullParam;
}

RETURN_CODE PlxSerialEepromReadByOffset (  HANDLE busIndex,
					   int eepromType,
					   U32* buffer,
					   U32 size)
{
  return ApiNullParam;
} 
U64 PlxRegisterRead ( HANDLE    busIndex,
		      U32 registerOffset,
		      RETURN_CODE *error )
{
  
  // printf("Fonction non implementee car registre %x inconnu \n",registerOffset);
  *error =ApiSuccess;
  return ApiSuccess;
}
int  PlxPciBoardReset (	HANDLE  handle , U8 masterId )
{
  int err;

  err=ioctl(handle,IOCTL_RESET,masterId);
  if ( err )
    {
      return  ApiInvalidDeviceInfo;
    } 
  return ApiSuccess;
}
VOID PlxPciFIFOReset (	HANDLE  handle , U8 masterId )
{
  int err=0;
 
  err=ioctl(handle,IOCTL_FIFOS_INIT,masterId);

  if ( err )
    {
      printf("erreur d'initialisation des FIFOS %d\n",err);

    } 
 
  return ;
}

U64 PlxRegisterWrite ( HANDLE busIndex,
		       U32 registerOffset,
		       U64 datas)
{
  printf("Fonction non implementee car registre %x inconnu \n",registerOffset);
  return 0;
  
}

U32 PlxCheckSlaveIt( HANDLE handle, U8 masterId )
{
  U32 err;

  err = ioctl(handle,IOCTL_CHECK_SLAVE_IT,masterId);
  return err;
}
