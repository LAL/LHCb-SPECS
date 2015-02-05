
/*! @mainpage AltSpecsV2 : the LHCb SpecsV2-DRIVER  Documentation
 *
 * \section intro_sec Introduction
 *
 * Ce driver permet de piloter la carte SpecsV2 ayant le firmware
 * spécifique développé pour le projet LHCb par Daniel Charlet.
 * On accéde à la carte via le fichier de peripherique /dev/AltSpecsV2Master0
 * par deux types d'acces open, ioctl,close comme un fichier ou bien par open , mmap,close
 * Le driver accédera au périphérique via une structure  
 * file_operations" qui permet certaines actions dont la pricipale ioctl: offre à
 * l'utilisateur la possibilite d'envoyer des commandes spécifiques.
 * \section install_sec Installation
 *  Dans le repertoire courant en tant que root lancer le script load_mod
 * \subsection tools_subsec Tools required:
 * version de Linux actuellement utilisé 2.6.32-431.1.2.el6.x86_64 
 *
 * \subsection running Running the program
 * .Le programme de test sera test.exe dans le package SpecsUser
 *
 * Dans ce code reste tout ce qui est lie aux interruptions (non utilise)
 */


#include "linux/types.h"
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <asm/pci.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#include <linux/capability.h>
#include <linux/pci.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/device.h>
#include <asm/page.h>
#include <linux/sched.h>
#include "AltSpecsV2.h"
struct task_struct *tsk;


/**
 *  Descriptor Header, controls the DMA read engine or write engine.
 * The SG-DMA controller core has three registers accessible from its Avalon-MM
 *
 */
// Doc Altera IP Compiler for PCI Express Chapitre6-12
// descrition du des registres du PCI 0x40 et 0x50
struct AvalonMM_CSR {
  u32 interrupt_status;
  u32 status_pad[3];
  u32 enableIrq;
  u32 enableIrq_pad[3];

}__attribute__ ((packed));

// Doc Altera Embedded Peripherals IP (ug_embed_ip) chapitre 10-5
// PIO regsiters
struct PIO_Register {
  u32 data_status;
  u32 direction;
  u32 interrupt_mask;
  u32 edge_capture;
  u32 ouset;
  u32 clear;
     
}__attribute__ ((packed));
// Doc Altera Embedded Peripherals IP (ug_embed_ip) chapitre 16 
// FIFO Avalan MM

struct FIFO_StatusRegister {
  u32 fill_level;
  u32 i_status;
  u32 event;
  u32 interrupt_enable;
  u32 almost_full;
  u32 almost_empty;
     
}__attribute__ ((packed));

/**
 * Altera PCI Express ('Aria 5') board specific book keeping data
 *
 * Keeps state of the PCIe core and the Chaining DMA controller
 * application.
 */
struct Aria5Specs_dev {
  /** the kernel pci device data structure provided by probe() */
    

  struct pci_dev *pci_device;
  int minor;
  /**
   * kernel virtual address of the mapped BAR memory and IO regions of
   * the End Point. Used by map_bars()/unmap_bars().
   */
  void * __iomem bar[DEVICE_COUNT_RESOURCE];
  
 
  /** whether this driver could obtain the regions */
  int got_regions;
  /** irq line succesfully requested by this driver, -1 otherwise */
  int irq_num;
  /** If Dma use irq irq_used == 1 and irq_handler AltSpecsV2_isr() not called
      so test it before the read and skip wait_event_interruptible() */
  int irq_used;
  int tab_IrqUsed[MAX_IRQ];
  /** board revision */
 
  u8 revision;
  /** interrupt count, incremented by the interrupt handler */
  int irq_count;

  /** character device */
  dev_t cdevno;
  struct cdev cdev;
  /** todo  : test not done*/
  int dma_lock;
  /** virtual address of the allocation which you can use to access  from the CPU*/
  unsigned long* virtual_mem;
  /** is and handle which you pass to the card corresponding of the virtual memory allocation */
  dma_addr_t buffer_bus;

  // struct vm_area_struct *vma_ref;
  struct siginfo info;
  int test_version;
  struct PIO_Register *pioMasterFifoIrqs; 
  struct PIO_Register *pioMasterStatusIrqs; 
  int masterIdUsed[NB_MASTER];
  int firstWriteFifo[NB_MASTER];
  int reference;
};
struct Master {
  int id;
  struct Aria5Specs_dev *specs_dev ;

  struct FIFO_StatusRegister *fifoRecIn_Status; 
  struct FIFO_StatusRegister *fifoEmiOut_Status; 
  u32                 *fifoEmiOut;
  u32                 *fifoRecIn;
  struct PIO_Register *pioRegStatus;
  struct PIO_Register *pioRegCtrl;
  int flagPIO;
  int SlaveIt;
  int maxDelay;
 
};

/**
 * Using the subsystem vendor id and subsystem id, it is possible to
 * distinguish between different cards bases around the same
 * (third-party) logic core.
 *
 * Default Altera vendor and device ID's, and some (non-reserved)
 * ID's are now used here that are used amongst the testers/developers.
 */
static const struct pci_device_id ids[] =
  { 
    { PCI_DEVICE( 0x1172, 0x0005) },
    {  }
  };
MODULE_DEVICE_TABLE(pci, ids);

static int major = 0; /* Major number */
module_param(major, int, 0660);
MODULE_PARM_DESC(major, "Static major number (none = dynamic)");
static wait_queue_head_t dma_irq_waitq;
static wait_queue_head_t pio_irq_waitq[NB_MASTER];

static int nb_irq[NB_MASTER]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static int SlaveIt[NB_MASTER]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static  struct Aria5Specs_dev *SpecsDev=NULL;
static  struct Master  *MasterList[NB_MASTER] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* prototypes for character device */
static int specs_dev_init(struct Aria5Specs_dev *specs_dev);
static void specs_dev_exit(struct Aria5Specs_dev *specs_dev);

/**
 * @bug AltSpecsV2_isr() - Interrupt handler not working
 *
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19) //MTQ add 3 args

static   irqreturn_t AltSpecsV2_isr(int irq, void *dev_id,struct pt_regs *regs)
#else
  static  irqreturn_t AltSpecsV2_isr(int irq, void *dev_id)
#endif
{

  struct Aria5Specs_dev *specs_dev;
  struct AvalonMM_CSR *pci_register;
  int  statusCtrl,status,num_fifo,statusFifo;
  
  struct PIO_Register *pioFifoStatus;
  struct PIO_Register *pioRegCtrl;
 
  specs_dev =dev_id;
 
  pci_register =( struct AvalonMM_CSR*)((specs_dev->bar[BAR2]) + PCI_EXPRESS_REGISTER);
 
  //iowrite32(0x0,&pioReg->interrupt_mask);
 

  status= ioread32(&pci_register->interrupt_status);

#ifdef DEBUG_IRQ
  printk(KERN_INFO DRV_NAME " Irq receive number %d\n",irq);
  printk ( KERN_INFO DRV_NAME " Irq PCIE %x     ! ! ! ! ! ! ! ! ! ! ! ! ! status IRQ %X\n",irq,status);
#endif

  
  if (status & 0x10 )
    {
    
      status = ioread32(&specs_dev->pioMasterStatusIrqs->edge_capture);
      if (status )
	{
	  iowrite32(0xFFFFFFFF,&specs_dev->pioMasterStatusIrqs->edge_capture);
	  for ( num_fifo=0; num_fifo < NB_MASTER; num_fifo++)
	    {
	      pioFifoStatus= (struct PIO_Register*) ((specs_dev->bar[BAR2])+ PIO_STATUS_REG  + ( (num_fifo %4) * MASTERS_OFFSET)+ (OFFSET_CROSSING_BRIDGE* (num_fifo/4 + 1)));
	      statusFifo = ioread32(&pioFifoStatus->edge_capture);
	      iowrite32(0xFFFFFFFF,&pioFifoStatus->edge_capture);
	      if (statusFifo )
		{
		  // fifo_reg_recept= (struct FIFO_StatusRegister*) ((specs_dev->bar[BAR2])+FIFO_RECEPTION_STATUS+ ( (num_fifo %4) * MASTERS_OFFSET)+ (OFFSET_CROSSING_BRIDGE* (num_fifo/4 + 1)));
		  pioRegCtrl =( struct PIO_Register*)((specs_dev->bar[BAR2])+PIO_CTRL_REG+ ((num_fifo%4) * MASTERS_OFFSET) + (OFFSET_CROSSING_BRIDGE* (num_fifo/4+1)));
		  printk(KERN_INFO DRV_NAME "master status %x Reception of IT_FRAME/Checksum_error Irq %x for Master %d  nb_irq =%d\n",status,statusFifo,num_fifo,nb_irq[num_fifo]);
		  SlaveIt[num_fifo]= statusFifo ;

		  // on devrait lire  la FIFO ici mais a priori il n'y a rien qui rentre dans la fifo seul la valeur du status permet de decoder le type d'IT 
		

		  statusCtrl= ioread32(&pioRegCtrl->data_status);

		  printk(KERN_INFO DRV_NAME "RAZ du bit 9 Ctrl %x add %x \n",statusCtrl,PIO_CTRL_REG+ ((num_fifo%4) * MASTERS_OFFSET) + (OFFSET_CROSSING_BRIDGE* (num_fifo/4+1)));	
		  iowrite32((0xFFFFF9FF & statusCtrl),&pioRegCtrl->data_status);
		  iowrite32((0x600 |  statusCtrl),&pioRegCtrl->data_status);
		  iowrite32((0xFFFFF9FF & statusCtrl),&pioRegCtrl->data_status);
		}
	    }
	  printk(KERN_INFO DRV_NAME " PIO Status interrupt Master %x \n",status);
#ifdef DEBUG_IRQ 
	  printk(KERN_INFO DRV_NAME " PIO Status interrupt Master0 %x \n",status);
#endif
	 
	 
	}
    }
   

  return IRQ_HANDLED;
  
    
}

/**
 * @details   Find all the BAR used by the card and display it in the /var/log/messages or by dmesg
 *
 * @param     specs_dev pointer under struct Aria5Specs_dev
 *
 * @param dev pointer under struct pci_dev
 *
 * @retval always 0
 */

static int __devinit scan_bars(struct Aria5Specs_dev *specs_dev, struct pci_dev *dev) {
  int i;
#ifdef DEBUG_PCI
  printk(KERN_INFO DRV_NAME "specs_dev %p pci_dev %p\n",specs_dev,dev);
#endif
  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
    unsigned long bar_start = pci_resource_start(dev, i);
    if (bar_start) {
      unsigned long bar_end = pci_resource_end(dev, i);
      unsigned long bar_flags = pci_resource_flags(dev, i);
      printk(KERN_ALERT "BAR%d 0x%08lx-0x%08lx flags 0x%08lx\n",
	     i, bar_start, bar_end, bar_flags);
    }
  }
  return 0;
}


/**
 * @details      Unmap the BAR regions that had been mapped earlier using map_bars()
 *
 * @param      specs_dev pointer of struct Aria5Specs_dev .
 * @param      dev pointer of struct pci_dev
 * @return     nothing
 *
 *
 */
static void unmap_bars(struct Aria5Specs_dev *specs_dev, struct pci_dev *dev) {
  int i;
  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
    /* is this BAR mapped? */
    if (specs_dev->bar[i]) {
      /* unmap BAR */
      pci_iounmap(dev, specs_dev->bar[i]);
      specs_dev->bar[i] = NULL;
    }
  }
}

/**
 * @details Map the device memory regions into kernel virtual address space after
 * verifying their sizes respect the minimum sizes needed, given by the
 * bar_min_len[] array.
 * @param      specs_dev pointer of struct Aria5Specs_dev .
 * @param      dev pointer of struct pci_dev
 * @retval  rc status
 *                      <ul>
 *                         <li> < 0  Failure
 *                         <li> 0 = Success
 *                      </ul>
 *
 *
 */
static int __devinit map_bars(struct Aria5Specs_dev *specs_dev, struct pci_dev *dev) {
  int rc;
  int i;
  /* iterate through all the BARs */
  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++) {
    if (pci_resource_len(dev, i) == 0)
      continue;
	
    if (pci_resource_start(dev, i) == 0)
      continue;
#ifdef DEBUG_PCI	
    printk(KERN_DEBUG DRV_NAME" : BAR %d (%#08x-%#08x), len=%d, flags=%#08x\n", i, (u32) pci_resource_start(dev, i), (u32) pci_resource_end(dev, i), (u32) pci_resource_len(dev, i), (u32) pci_resource_flags(dev, i));
#endif
    if (pci_resource_flags(dev, i) & IORESOURCE_MEM) {
      specs_dev->bar[i] = ioremap(pci_resource_start(dev, i), pci_resource_len(dev, i));
      if ( specs_dev->bar[i] == NULL) {
	printk(KERN_WARNING DRV_NAME " : unable to remap I/O memory\n");
	    
	rc = -ENOMEM;
	goto cleanup_ioremap;
      }
#ifdef DEBUG_PCI
      printk(KERN_DEBUG DRV_NAME" : I/O memory has been remaped at %p\n", specs_dev->bar[i]);
#endif
    } else {
      specs_dev->bar[i] = NULL;
    }
  }
  return 0;
 cleanup_ioremap:
  pci_release_regions(dev);
  return rc;
}
static void __devinit test_version (  struct Aria5Specs_dev *specs_dev) {
 
  u32* pversion;
  u32 version;
 
 
  pversion =  (u32 *) ( (specs_dev->bar[BAR2]) + VERSION );

  version=ioread32(pversion);

  if (version == DRIVER_VERSION )
    { 
      printk(KERN_INFO DRV_NAME " Version number %d \n",version);
      specs_dev->test_version =1;
    }
  else
    {
      printk(KERN_ALERT DRV_NAME "!!!!!!!!     !!!!!  Driver not compatible %d \n",version); 
      specs_dev->test_version =0;
    }
  return ;	   	
} 
/**
 *
 *
 * @details  Called when the PCI sub system thinks we can control the given device.
 * Inspect if we can support the device and if so take control of it.
 *
 * Return 0 when we have taken control of the given device.
 *
 * - allocate board specific bookkeeping
 * - allocate coherently-mapped memory for the descriptor table
 * - enable the board
 * - verify board revision
 * - request regions
 * - query DMA mask
 * - obtain and request irq
 * - map regions into kernel address space
 * @param      dev   struct pci_dev
 * @param      id const struct pci_device_id
 * @return     0 is the device is reconized
 *
 * @retval     status   The program status.
 * */

static int __devinit probe(struct pci_dev *dev, const struct pci_device_id *id)
{
  int rc = 0;
  u8  mypin;
  struct Aria5Specs_dev *specs_dev=NULL;
  static int minor = 0;
  struct AvalonMM_CSR *p;
  u32 status = 0;
  int i=0;
  struct Master *master;

#ifdef DEBUG_PCI
  printk(KERN_ALERT "probe(dev = 0x%p, pciid = 0x%p)\n", dev, id);
  printk(KERN_INFO DRV_NAME " found %x:%x\n", id->vendor, id->device);
  printk(KERN_INFO DRV_NAME " dev->irq %d\n",dev->irq);
#endif
  /* allocate memory for per-board book keeping */
  specs_dev= kzalloc(sizeof(struct Aria5Specs_dev), GFP_KERNEL);

  if (!specs_dev) {
    printk(KERN_ALERT "Could not kzalloc()ate memory.\n");
    goto cleanup_kmalloc;
  }
  pci_set_drvdata(dev,specs_dev); // inutile ?
  specs_dev->pci_device = dev;
  specs_dev->minor = minor++;
  /* enable device */
  

  rc = pci_enable_device(dev);
  if (rc  < 0) {
    printk(KERN_ALERT DRV_NAME "pci_enable_device() failed\n");
    goto err_enable;
  }
   
  /** XXX check for native or legacy PCIe endpoint? */

  rc = pci_request_regions(dev, DRV_NAME);
  /* could not request all regions? */
  if (rc) {
    printk( KERN_ALERT DRV_NAME " : Unable to reserve PCI resources \n");
    goto err_regions;
  }
  specs_dev->got_regions = 1;
 
  /* query for DMA transfer */
  /* @see Documentation/PCI/PCI-DMA-mapping.txt */
  /* */ 
 
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,25)
  if ( (sizeof(dma_addr_t) > 4) && !pci_set_dma_mask(dev, DMA_BIT_MASK(64))) {
    pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(64));
#ifdef DEBUG_PCI
    printk(KERN_WARNING DRV_NAME " : Using  64-bit DMA mask.\n");
#endif
  } 
  else {
    if (!pci_set_dma_mask(dev, DMA_BIT_MASK(32))) {
#ifdef DEBUG_PCI
      printk(KERN_WARNING DRV_NAME" : set 32-bit DMA mask.\n");
#endif
      pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(32));
    }
    else 
      { 
	printk(KERN_ALERT DRV_NAME" : Could not set  32-bit DMA mask.\n");
	rc = -1;
	goto err_mask;
      }
  }
#else
   if (!pci_set_dma_mask(dev, DMA_32BIT_MASK)) {
#ifdef DEBUG_PCI
    printk(KERN_WARNING DRV_NAME " : set 32-bit DMA mask.\n");
#endif
    pci_set_consistent_dma_mask(dev, DMA_32BIT_MASK);
	
  }
  else
    {
      printk(KERN_WARNING DRV_NAME "Could not set  32-bit DMA mask.\n");
      rc = -1;
      goto err_mask;
    }

#endif    
	  
  /* show BARs */
  scan_bars(specs_dev, dev);
  /* map BARs */ 

  rc = map_bars(specs_dev, dev);
  if (rc)
    goto err_map;
  
  rc= pci_write_config_byte(dev,0x52, 0x84);  //MSI DISABLE REGISTER

  rc = pci_read_config_byte(dev,0x52, &mypin); 
#ifdef DEBUG_PCI
  printk(KERN_WARNING DRV_NAME " 0x52 : %x \n",mypin);
#endif
   
  specs_dev->irq_count=0;
  /* initialize character device */
  rc = specs_dev_init(specs_dev);
  if (rc)
    { printk(KERN_ALERT DRV_NAME " !! specs_dev_init error\n");
      goto err_cdev;
    }
#ifdef DEBUG_PCI 
  printk(KERN_ALERT DRV_NAME " !! specs_dev_init Ok \n" ); 
#endif
  /* enable bus master capability on device */
  pci_set_master(dev);

 
  SpecsDev= specs_dev;
  init_waitqueue_head(&dma_irq_waitq);
  for (i=0 ;i< NB_MASTER ;i++)
    init_waitqueue_head(&pio_irq_waitq[i]);

  pci_read_config_byte(dev, PCI_REVISION_ID, &specs_dev->revision);
#ifdef DEBUG_PCI
  printk(KERN_INFO DRV_NAME " Revision %d \n",specs_dev->revision );
#endif
  test_version(specs_dev);
  //specs_dev->test_version =1;
  rc = pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &mypin);
  if (mypin)
    {
      specs_dev->irq_num= dev->irq;
       	   	  
      if( request_irq(dev->irq,AltSpecsV2_isr, IRQF_SHARED,DRV_NAME,specs_dev ) <0)
	{
	  printk(KERN_ALERT DRV_NAME " : unable to register irq handler\n");
	  goto err_irq;
	}
#ifdef DEBUG_PCI
      printk(KERN_WARNING DRV_NAME " : install irq Handler OK dev->irq= %d mypin %d \n",dev->irq,mypin);
#endif
    }
  else
    {
      printk(KERN_ALERT DRV_NAME ": no IRQ!\n");
      specs_dev->irq_num=0;
      goto err_irq;
    }
#ifdef DEBUG_PCI
  printk(KERN_WARNING DRV_NAME " : IRQ PIN #%d (0=none, 1=INTA#...4=INTD#).\n",mypin); 
#endif
  p =( struct AvalonMM_CSR*)((specs_dev->bar[BAR2])+PCI_EXPRESS_REGISTER);
  status = ioread32(&p->interrupt_status);
  iowrite32(cpu_to_le32( 0x0),&p->enableIrq); 
  status = ioread32(&p->enableIrq);
  // A revoir 
     	
  iowrite32(cpu_to_le32(status | 0xFFFF),&p->enableIrq);


  for (i=0;i <NB_MASTER;i++) 
    {
      specs_dev->masterIdUsed[i]=0; // initialisation des Master en cours
      specs_dev->firstWriteFifo[i]=0; // on a jamais ecrit dans la fifo du master
    }
  for (i=0;i <NB_MASTER;i++) 
    {
      master= kzalloc(sizeof(struct Master), GFP_KERNEL);
      if (!master) {
	printk(KERN_ALERT "Could not kzalloc()ate memory.\n");
	return -199;
      }
      master->id = -2;
      MasterList[i]= master;
  
    }
  // enable les irqs sur  PIO masterFifoIrqs pour data in FIFOS
  specs_dev->pioMasterFifoIrqs =( struct PIO_Register*)((specs_dev->bar[BAR2])+PIO_MASTERS_FIFO_IRQS);
  iowrite32(0xFFFFFFFF,& specs_dev->pioMasterFifoIrqs->edge_capture);		
  iowrite32(0xFFFF,& specs_dev->pioMasterFifoIrqs->interrupt_mask);
 
  udelay(50); 
  // enable les irqs sur  PIO masterStatusIrqs pour IT_FRAME /checksum error   
  specs_dev->pioMasterStatusIrqs =( struct PIO_Register*)((specs_dev->bar[BAR2])+PIO_MASTERS_STATUS_IRQS);
  iowrite32(0xFFFFFFFF,& specs_dev->pioMasterStatusIrqs->edge_capture);		
  iowrite32(0xFFFF,& specs_dev->pioMasterStatusIrqs->interrupt_mask);

  udelay(50); 
  goto end;


 err_cdev:
  /* unmap the BARs */
  specs_dev_exit(specs_dev);
  unmap_bars(specs_dev, dev);

 err_map:
  if (specs_dev->irq_num > 0)
    free_irq(specs_dev->irq_num,specs_dev); 
  pci_release_regions(dev);
 err_irq :
 err_mask: err_regions:
  pci_disable_device(dev);
 err_enable: 
  for (i=0;i <NB_MASTER;i++) 
    {
      if (MasterList[i]) kfree (MasterList[i]);
    }
  kfree(specs_dev); 
  printk(KERN_ALERT DRV_NAME " !! Probe() Not done\n"); 
  return rc;
 cleanup_kmalloc:
  return -ENOMEM;
 end:
  printk(KERN_ALERT DRV_NAME " !! Probe() successful.\n"); 
  return rc;
}
      
 
/**
 * @details  Call when the driver is removed (call  of rmmod)
 * free the allocation of struct Aria5Specs_dev
 * @param dev pointer of struct pci_dev
 */
    
static void __devexit remove(struct pci_dev *dev) {
  struct Aria5Specs_dev *specs_dev=NULL;
  u8 mypin;
  int i=0;
  if ( &dev->dev == NULL)
    {
      printk(KERN_WARNING DRV_NAME " Already removed \n");
      return;
    }
  specs_dev = SpecsDev; 
  if ( specs_dev == NULL) return ;
  if (specs_dev->pci_device == dev )
    { 
#ifdef DEBUG_PCI
      printk(KERN_WARNING  DRV_NAME "found dev(0x%p)\n", dev);
#endif
      
    }
  else  printk(KERN_WARNING DRV_NAME  "not found dev(0x%p)==(0x%p) \n", dev,specs_dev->pci_device);
  
#ifdef DEBUG_PCI
  printk(KERN_WARNING  DRV_NAME "remove(0x%p)\n", dev);
  printk(KERN_WARNING  DRV_NAME "remove(dev = 0x%p) where specs_dev = 0x%p\n", dev, specs_dev);
#endif
  if (specs_dev->irq_num > 0)
    {
      pci_read_config_byte (dev, PCI_INTERRUPT_PIN, &mypin);
      if (mypin)
	free_irq(specs_dev->irq_num, specs_dev);
    }	
  /* unmap the BARs */
  unmap_bars(specs_dev, dev);
  
  if (specs_dev->got_regions)
    pci_release_regions(dev);
  pci_disable_device(dev);

  specs_dev_exit(specs_dev);
#ifdef DEBUG_PCI
  printk(KERN_WARNING DRV_NAME "Freeing specs_dev %p\n", specs_dev);
#endif
  for (i=0;i <NB_MASTER;i++) 
    {
      if (MasterList[i]) kfree (MasterList[i]);
    }
  if (specs_dev)
    kfree(specs_dev);
  
}


/**
 * @details  Called when the user application open on of device caracter /dev/AltSpecsV2[0-3]
 * @param inode pointer of struct inode
 * @param file pointer of struct file
 *
 *  @retval  rc status
 *                      <ul>
 *                         <li> < 0  Failure
 *                         <li> 0 = Success
 *                      </ul>
 */

static int specs_dev_open(struct inode *inode, struct file *file) {
  struct Aria5Specs_dev *specs_dev;
  
 
  int minor = MINOR(inode->i_rdev);
 
  
  specs_dev = SpecsDev;
  if ( specs_dev == NULL)
    { 
      printk(KERN_INFO DRV_NAME "Try to open with NULL spec_dev  \n");
      return -199;
    }
  if (specs_dev->minor == minor) {
    if (specs_dev->test_version == 0)
      { 
	printk(KERN_INFO DRV_NAME "Try to open with bad version  \n");
	return -199;
      }
   
    
    return 0;

  }
#ifdef DEBUG_PCI 
  printk(KERN_WARNING DRV_NAME " : minor %d not found\n", minor);
#endif 
  return -ENODEV;
}

/**
 * @details Management of all the functions of the StartixGXII board
 * @param inode pointer of struct inode
 * @param file pointer of struct file
 * @param ioctl_num number of the command
 * @param ioctl_param argument if need by the command
 * @retval  rc status
 *                      <ul>
 *                         <li> < 0  Failure
 *                         <li> 0 =  Success
 *                         <li> > 0 
 *                      </ul>
 */

static int specs_dev_ioctl(struct inode *inode, struct file *file,
			   unsigned int ioctl_num, /* number and param for ioctl */
			   unsigned long ioctl_param) 
{
  struct Master *master =  NULL;
  struct Aria5Specs_dev *specs_dev= NULL;
  struct pci_dev *pDev;
  /* the write DMA header sits after the read header at address 0x10 */
  
  int  rc = 0;
  u32 status = 0;
  u32 status_reg=0;
  unsigned char slot,bus;
  int level2,level1;
  DEVICE_LOCATION pLoc;
  BUSIOPDATA pBusIop;
  INTERRUPT_OBJ pIntObj;
  int i=0,k=0;
  u32 *pBuf32; 
  int level_fifo; 
  u32 *pReset;
  u32 valReset=0;
  rc=0;
  
  specs_dev = SpecsDev;
 
  switch (ioctl_num) {
    if (!capable(CAP_SYS_RAWIO))
      return -EPERM;
    
  
 
  case IOCTL_FIND_DEVICE:
    if (specs_dev == NULL) {  rc =2; break;}
    if ( specs_dev->test_version !=1 ) {rc =2; break;}
    if (! access_ok(VERIFY_READ,(void *) ioctl_param, sizeof ( DEVICE_LOCATION ))) return -EFAULT;
    if (!copy_from_user(&pLoc,( DEVICE_LOCATION*) ioctl_param, sizeof( DEVICE_LOCATION)))
      {
	pDev= specs_dev->pci_device;
	slot = PCI_SLOT(pDev->devfn);
	bus=pDev->bus->number;
#ifdef DEBUG_SPECS 
	printk(KERN_INFO DRV_NAME " BUS %d:%d Slot %d:%d \n",pLoc.BusNumber,bus, pLoc.SlotNumber,slot);
#endif
	if (( pLoc.BusNumber==bus)&&( pLoc.SlotNumber==slot)) rc= 0;
	else rc =1;
      }
    else rc=1;
    break;
 
  case IOCTL_READ_FILL_LEVEL :
   
    if (! access_ok(VERIFY_READ,(void *) ioctl_param, sizeof ( BUSIOPDATA ))) return -EFAULT;
    if ( !copy_from_user(&pBusIop,( BUSIOPDATA*) ioctl_param, sizeof( BUSIOPDATA)))
      { 
	master=MasterList[pBusIop.MasterID];	
	pBuf32=(u32*)pBusIop.Buffer;
        // permet d'attendre ici un si au lieu de repasser au niveau user
	for (i=0; i<10 ;i++){
	  level1=ioread32 (&master->fifoRecIn_Status->fill_level);
	  udelay(1);
	  level2=ioread32 (&master->fifoRecIn_Status->fill_level);
	  if (level1 ==  level2 )
	    {
	      pBuf32[0]=level1;
	      break;
	    }
	}
	pBuf32[0]= ioread32 (&master->fifoRecIn_Status->fill_level);
#ifdef DEBUG_SPECS 	
	printk(KERN_INFO DRV_NAME "FIFO Recep read level =%d event %x Fifo Addr %x\n",ioread32 (&master->fifoRecIn_Status->fill_level),ioread32 (&master->fifoRecIn_Status->event), (pBusIop.Address + OFFSET_FIFO_REC_IN_STATUS));
#endif
	rc=0;
      }
    else rc=1;
    break;
  case IOCTL_CHECK_SLAVE_IT:
    k=ioctl_param;
    put_user(SlaveIt[k], (int *)ioctl_param);
    SlaveIt[k]=0;
    rc=0;
    break;
  case IOCTL_BUS_IOP_READ :
    if (! access_ok(VERIFY_READ,(void *) ioctl_param, sizeof ( BUSIOPDATA ))) return -EFAULT;
    if (!copy_from_user(&pBusIop,( BUSIOPDATA*) ioctl_param, sizeof( BUSIOPDATA)))
      {
	 

	if (pBusIop.AccessType ==    BitSize32)
	  {    
	    pBuf32=(u32*)pBusIop.Buffer;

	    for (i=0; i< pBusIop.TransferSize ; i++)
	      { 
		pBuf32[i]=ioread32((void*)((specs_dev->bar[BAR2])+ pBusIop.Address ) );
		 
#ifdef DEBUG_SPECS		    
		printk(KERN_INFO DRV_NAME " Read32: pBuf[%d] = %x addr  %x\n",i,pBuf32[i],pBusIop.Address);
#endif
	
	      }
	  }
      }
    else rc=-1;
    
   
    
    break;
  case IOCTL_BUS_IOP_WRITE :

    if (! access_ok(VERIFY_READ,(void *) ioctl_param, sizeof ( BUSIOPDATA ))) return -EFAULT;
    if (!copy_from_user(&pBusIop,( BUSIOPDATA*) ioctl_param, sizeof( BUSIOPDATA)))
      {
       
#ifdef DEBUG_SPECS
	printk(KERN_INFO DRV_NAME " Size %d type %d  address %lx\n",pBusIop.TransferSize,pBusIop.AccessType,pBusIop.Address );
#endif
	if (  (pBusIop.Address & MASK_FIFO_IN )  ==  ( FIFO_EMISSION_OUT & MASK_FIFO_IN) )
	  {
	    master=MasterList[pBusIop.MasterID];
	    level_fifo = ioread32 (&master->fifoEmiOut_Status->fill_level);
	    // si level_fifo > 0 il faut faire un reset de la fifo
	    if (level_fifo > 0)  
	      {
		printk(KERN_INFO DRV_NAME "Fifo avec %d octets avant ecriture\n",level_fifo);
		rc = -2;
		break;
	      }
	    // on ecrit les donnees dans la fifo
	    pBuf32=(u32*)pBusIop.Buffer;
	    for (i=0; i< pBusIop.TransferSize ; i++)
	      { 
		iowrite32( pBuf32[i],(master->fifoEmiOut ) );
	       
#ifdef DEBUG_SPECS
		printk(KERN_INFO DRV_NAME " Write32: pBuf[%d] = %x    \n",i,pBuf32[i] );
#endif	       
	     
	      }
	    // ecriture du bit de start qui declenche l'ecriture de la FIFO vers le slave
	    status= ioread32(&master->pioRegCtrl->data_status);
	    iowrite32((0x4000 |  status),&master->pioRegCtrl->data_status);

	    // attente du 3eme bit du status register indiquant la fin de trame de fla fifo
	    for (i=0 ;i < 10000 ;i++) 
	      {
		status_reg = ioread32(&master->pioRegStatus->data_status);
		if  ( status_reg  & 0x4 ) {
		  rc=1; 
		  break;
		}
	      }
	   
	    if ( i== 10000){
	      printk(KERN_INFO DRV_NAME " FIFO non videe \n");
	      rc=-2;
	    }
	    iowrite32(( 0xFFFF8FFF & status),&master->pioRegCtrl->data_status);
	  
	  
	  }
	// ecriture dans un registre
	else 
	  {
	    if (pBusIop.AccessType ==    BitSize32)
	      {    
		pBuf32=(u32*)pBusIop.Buffer;
		for (i=0; i< pBusIop.TransferSize ; i++)
		  { 
		    iowrite32( pBuf32[i],(u32*)((specs_dev->bar[BAR2])+ pBusIop.Address ) );
#ifdef DEBUG_SPECS
		    printk(KERN_INFO DRV_NAME " Write32: pBuf[%d] = %x   address %lx \n",i,pBuf32[i],pBusIop.Address );
#endif	       
		    rc=1;
		   
		  }
	      }
	   
	  }
    
      }
	  
    else rc=-1;
    break;
  case IOCTL_ENABLE_PIO_INTERRUPT : 
    if (! access_ok(VERIFY_READ,(void *) ioctl_param, sizeof ( INTERRUPT_OBJ ))) return -EFAULT;
    if (!copy_from_user(&pIntObj,( INTERRUPT_OBJ*) ioctl_param, sizeof( INTERRUPT_OBJ)))
      {
 
#ifdef DEBUG_IRQ
	printk(KERN_INFO DRV_NAME " INTERRUPT Enable : Reg add %lx IT %d  \n",pIntObj.stat_reg,pIntObj.pio_line);
#endif 
	if (pIntObj.pio_line == IT_STATUS ) // a revoir si besoin 
	  {
	    iowrite32(0xFF0080,&master->pioRegStatus->interrupt_mask); // frame_it+ checksum bit 0x8
	    iowrite32(0xFFFFFFFF,&master->pioRegStatus->edge_capture);
	    iowrite32(0xFFFFFFFF,&master->pioRegStatus->edge_capture);

	    rc=0;
	  }

      }
    else rc= 1;
    break;
  case IOCTL_DISABLE_PIO_INTERRUPT : 
    if (! access_ok(VERIFY_READ,(void *) ioctl_param, sizeof ( INTERRUPT_OBJ ))) return -EFAULT;
    if (!copy_from_user(&pIntObj,( INTERRUPT_OBJ*) ioctl_param, sizeof( INTERRUPT_OBJ)))
      {
#ifdef DEBUG_IRQ		
	printk(KERN_INFO DRV_NAME " Interrupt Disable Reg  value %lx pioline = %d should be  %d \n",(pIntObj.stat_reg )+ OFFSET_FIFO_REC_IN_STATUS,pIntObj.pio_line,IT_FIFO );   
#endif	
	if (pIntObj.pio_line == IT_STATUS) // A revoir aussi
	  {
	   
	    iowrite32(0,&master->pioRegStatus->interrupt_mask);
	    status = ioread32(&master->pioRegStatus->edge_capture);
	    iowrite32(0xFFFFFFFF,&master->pioRegStatus->edge_capture);
	    iowrite32(0xFFFFFFFF,&master->pioRegStatus->edge_capture);

	  }
	
	rc=0;
      }
    else rc=1;
    break;
    
  case IOCTL_FIFOS_INIT:
  
    master=MasterList[ioctl_param];
  
    k=ioctl_param;
    
    master->fifoRecIn_Status= (struct FIFO_StatusRegister*) ((specs_dev->bar[BAR2])+FIFO_RECEPTION_STATUS+ ((k%4) * MASTERS_OFFSET)+ (OFFSET_CROSSING_BRIDGE* (k/4+1)));
    master->fifoEmiOut_Status= (struct FIFO_StatusRegister*)( (specs_dev->bar[BAR2])+FIFO_EMISSION_STATUS+ ((k%4) * MASTERS_OFFSET)+ (OFFSET_CROSSING_BRIDGE* (k/4+1)));
    master->pioRegCtrl =( struct PIO_Register*)((specs_dev->bar[BAR2])+PIO_CTRL_REG+ ((k%4) * MASTERS_OFFSET) + (OFFSET_CROSSING_BRIDGE* (k/4+1)));
    master->pioRegStatus =( struct PIO_Register*)((specs_dev->bar[BAR2])+ PIO_STATUS_REG + ((k%4) * MASTERS_OFFSET) + (OFFSET_CROSSING_BRIDGE* (k/4+1)));
    master->fifoRecIn =(u32* )((specs_dev->bar[BAR2])+ FIFO_RECEPTION_IN   + ((k%4) * MASTERS_OFFSET) + (OFFSET_CROSSING_BRIDGE* (k/4+1)));
    master->fifoEmiOut =(u32* )((specs_dev->bar[BAR2])+ FIFO_EMISSION_OUT  + ((k%4) * MASTERS_OFFSET) + (OFFSET_CROSSING_BRIDGE* (k/4+1)));
#ifdef DEBUG_SPECS
    printk(KERN_INFO DRV_NAME " FIFO %d event %x interrupt %x level %d  full %d empty %d \n",k,ioread32(&master->fifoEmiOut_Status->event),ioread32(&master->fifoEmiOut_Status->interrupt_enable),ioread32 (&master->fifoEmiOut_Status->fill_level),ioread32(&master->fifoEmiOut_Status->almost_full) ,ioread32(&master->fifoEmiOut_Status->almost_empty) );
    printk(KERN_INFO DRV_NAME " FIFO %d level %d %d \n",k,ioread32 (&master->fifoRecIn_Status->fill_level),ioread32 (&master->fifoRecIn_Status->fill_level));
    printk(KERN_INFO DRV_NAME " FIFO %d level %d %d \n",k,ioread32 (&master->fifoRecIn_Status->fill_level),ioread32 (&master->fifoEmiOut_Status->fill_level));
#endif 
    /*
      int reset;
      reset= ( 1 << k);
      pReset= (u32*) (specs_dev->bar[BAR2]+ RESET);
      printk(KERN_INFO DRV_NAME " RESET one %x = %x\n",pReset,reset);	
      iowrite32(reset, pReset);
      iowrite32(0x0000, pReset);
	
      do {
      iowrite32(0x0000, pReset);
      printk(KERN_INFO DRV_NAME " Reset one niveau haut \n");
	
      }while (ioread32 (pReset) != 0 );
    */	
	
		
    //iowrite32(0x0000, pReset);
	

    // udelay(50);	
			  
    iowrite32((0x00000000 ),&master->pioRegCtrl->direction);
    // reset FIFOS 	
    iowrite32(0x0 ,&master->pioRegCtrl->data_status);
    iowrite32(0x10,&master->pioRegCtrl->data_status); //avant mis 0x8
    iowrite32(0x0 ,&master->pioRegCtrl->data_status);
	
    for (i=0 ; i< 4;i++)
      {
	rc=0;
	iowrite32( 0x3F,&master->fifoEmiOut_Status->event);
	if ( ioread32(&master->fifoEmiOut_Status->event) != 0x0) rc=1;
	iowrite32( 0,&master->fifoEmiOut_Status->interrupt_enable);
	if ( ioread32(&master->fifoEmiOut_Status->interrupt_enable) != 0) rc=2;
	iowrite32(280,&master->fifoEmiOut_Status->almost_full);
	if ( ioread32(&master->fifoEmiOut_Status->almost_full) != 280) rc =3;
	iowrite32(ALMOST_EMPTY,&master->fifoEmiOut_Status->almost_empty);
	if ( ioread32(&master->fifoEmiOut_Status->almost_empty) !=ALMOST_EMPTY) rc =4;
	iowrite32( 0x3F,&master->fifoRecIn_Status->event);
	if ( ioread32(&master->fifoRecIn_Status->event) !=0x0 )  rc=5;
	    
	iowrite32( 0,&master->fifoRecIn_Status->interrupt_enable);
	if ( ioread32(&master->fifoRecIn_Status->interrupt_enable) !=0 ) rc =6;
	iowrite32(ALMOST_FULL,&master->fifoRecIn_Status->almost_full);
	if ( ioread32(&master->fifoRecIn_Status->almost_full) != ALMOST_FULL ) rc=7;
	iowrite32(ALMOST_EMPTY,&master->fifoRecIn_Status->almost_empty);
	if (ioread32(&master->fifoRecIn_Status->almost_empty) != ALMOST_EMPTY) rc =8;
      }

    //	iowrite32( 0x4,&fifo_reg_recept->interrupt_enable);
 

#ifdef DEBUG_SPECS
    printk(KERN_INFO DRV_NAME " FIFO emission %d event %x interrupt %x level %d  full %d empty %d \n",k,ioread32(&master->fifoEmiOut_Status->event),ioread32(&master->fifoEmiOut_Status->interrupt_enable),ioread32 (&master->fifoEmiOut_Status->fill_level),ioread32(&master->fifoEmiOut_Status->almost_full) ,ioread32(&master->fifoEmiOut_Status->almost_empty) );
#endif
    status= ioread32(&master->pioRegCtrl->data_status);
#ifdef DEBUG_SPECS
    printk(KERN_INFO DRV_NAME "Valeur initiale du PIO Ctrl %x addr CtrlReg %x\n",status,PIO_CTRL_REG+ ((k%4) * MASTERS_OFFSET) + (OFFSET_CROSSING_BRIDGE* (k/4+1)));
#endif
       
  
    break; 
  case IOCTL_OPEN_MASTER :
    master=MasterList[ioctl_param];
#ifdef PROTECTED_ACCESS
    
    if (specs_dev->masterIdUsed[ioctl_param] != 0)
      {   
	 
	rc=1;
      }
    else {
      if (master->id== -2)
	{
	  specs_dev->masterIdUsed[ioctl_param]=1;
	  master->id=ioctl_param;
	  rc=0;
	}
      else rc=1;
    }
#else
    master->id=ioctl_param;
    rc=0;
#endif
    break;
  case IOCTL_CLOSE_MASTER :
    master=MasterList[ioctl_param];
    level_fifo = ioread32 (&master->fifoEmiOut_Status->fill_level);
 
#ifdef PROTECTED_ACCESS
    master=MasterList[ioctl_param];
    if (specs_dev->masterIdUsed[ioctl_param] == 0)
      {
	rc=1;
      }
    else
      { 
        if (	master->id == ioctl_param)
	  {
	    specs_dev->masterIdUsed[ioctl_param]=0;
	    master->id=-2;
	    specs_dev->firstWriteFifo[ioctl_param]=0;
	    rc=0;
	  }
	else rc=1;
      }
#else 
    master=MasterList[ioctl_param];
    if ( master->id == ioctl_param )
      {
	specs_dev->firstWriteFifo[ioctl_param]=0;
	rc=0; 
      }
    else rc=1;
#endif
    break;
  case IOCTL_RESET :
    break;
    // on ne fait plus de RESET car cela bloque le Kontron (bus Avalon bloqu� ??) , de plus aucun effet sur le fonctionnement.
    master=MasterList[ioctl_param];
    if ( master->id == ioctl_param )
      {  

	pReset= (u32*)( specs_dev->bar[BAR2]+ RESET);
        // IL FAUT RESETER LE BIT CORRESPONDANT
	valReset = ( 1 <<  ioctl_param);
	iowrite32(valReset, pReset);
	iowrite32(0x0000, pReset);
	
	do {
	  iowrite32(0x0000, pReset);
	  printk(KERN_INFO DRV_NAME " Reset niveau haut \n");
	
	}while (ioread32 (pReset) != 0 );
		
	udelay(5);
      }
    break;
  default:
    printk(KERN_INFO DRV_NAME " Unknown command \n");
    rc=1;
    break;
  }
  return rc;
}


/** @details function call by read() by the user programm
 * @param file pointer of struct file
 * @param buf char pointer allocated by the user programm
 * @param count number of bytes to be copied in buf
 * @param pos offset in octets
 @retval  count number of bytes read (return to the user), if < 0 error
*/
static ssize_t specs_dev_read(struct file *file, char __user *buf, size_t count,
			      loff_t *pos)
{
  
	
  return 0;
}
/** @details function call by write() by the user programm
 * @param file pointer of struct file
 * @param buf char pointer allocated by the user programm
 * @param count number of bytes to be copied in buf
 * @param pos offset in octets
 @retval  count number of bytes read from user space , < 0 if error.
*/
static ssize_t specs_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *pos) {

  return 0;
}

/**
 * @details Called when the device goes from used to unused. called by close() in user programm
 * @param inode struct inode
 * @param file struct file
 @retval  0
*/
static int specs_dev_close(struct inode *inode, struct file *file) {
  struct Aria5Specs_dev *specs_dev;
  int i;
  
  specs_dev = SpecsDev;
  /*
    iowrite32(0xFFFF,&master->pioRegStatus->edge_capture);
    iowrite32(0,&master->pioRegStatus->interrupt_mask);
  */ 
   
    
  
   
  for ( i=0 ; i< NB_MASTER ; i++)
    {
      
      if ( specs_dev->masterIdUsed[i] == 1 )
	{ 
	  
	  return 0;
	}
      
    }
 
  return 0;
}

static int specs_dev_mmap(struct file * file, struct vm_area_struct *vma) ;
static const struct file_operations specs_devfops = { 
  .owner   = THIS_MODULE, 
  .open    = specs_dev_open,
  .ioctl   = specs_dev_ioctl, 
  .release = specs_dev_close,
  .read    = specs_dev_read,
  .write   = specs_dev_write,
  .mmap    = specs_dev_mmap, 
};

/** @details memory mapping of the virtuel memory for the user programm called by mmap( function not used)
 * @param file pointer of struct file
 * @param  vma pointer of struct vm_area_struct
 * @retval 0 if case of succes  else < 0
 */
static int specs_dev_mmap(struct file * file, struct vm_area_struct *vma) {
  struct Aria5Specs_dev *specs_dev;
  unsigned long mem_addr;
  unsigned int size,max_size;

  specs_dev = (struct Aria5Specs_dev*) (file->private_data);

  vma->vm_ops = (struct vm_operations_struct *)&specs_devfops;
  vma->vm_flags |= VM_RESERVED;
  size = vma->vm_end - vma->vm_start;
  // specs_dev->vma_ref = vma;
  // offset is in long word size
#ifdef DEBUG_PCI
  printk(KERN_INFO DRV_NAME " Mmap \n");
#endif
  mem_addr =  pci_resource_start(specs_dev->pci_device, 2);  // mmap BAR 2
  max_size =pci_resource_len(specs_dev->pci_device, 2); // size of BAR2
  if ( size > max_size )
    {
      printk(KERN_INFO DRV_NAME " size of mmap() to big max :%x \n",max_size);
      return -EAGAIN;
    }  
  //  mem_addr = virt_to_phys(specs_dev->virtual_mem); pour le dma
#ifdef DEBUG_PCI
  if (mem_addr )
    printk(KERN_INFO DRV_NAME " map vma %p pfn %lX vm_start %lX vm_end %lX size %X\n", vma,
	   (mem_addr >> PAGE_SHIFT), vma->vm_start, vma->vm_end, size);
#endif
  if (remap_pfn_range(vma, vma->vm_start, (mem_addr >> PAGE_SHIFT), size, vma->vm_page_prot))
    return -EAGAIN;
  
  return 0;
}



/** @details specs_dev_init() - Initialize character device
 *  @param specs_dev pointer of struct Aria5Specs_dev
 *  @retval 0 if succes else <0
 */
static int specs_dev_init(struct Aria5Specs_dev *specs_dev) {
  int rc;
  printk(KERN_INFO DRV_NAME " specs_devinit()\n");
 
  if (major)
    {
      specs_dev->cdevno= MKDEV(major,specs_dev->minor);
      rc =register_chrdev_region(specs_dev->cdevno,1,DRV_NAME);
    }
  else
    {  /* allocate a dynamically allocated character device node for 1srt device */
      rc = alloc_chrdev_region(&specs_dev->cdevno, specs_dev->minor , 1,DRV_NAME);
      major =MAJOR(specs_dev->cdevno);
   
    }
  /* allocation failed? */
  if (rc < 0) {
    printk(KERN_ALERT DRV_NAME "alloc_chrdev_region() = %d\n", rc);
    goto fail_alloc;
  }
  /* couple the device file operations to the character device */
  cdev_init(&specs_dev->cdev, &specs_devfops);
  specs_dev->cdev.owner = THIS_MODULE;
  /* bring character device live */
  rc = cdev_add(&specs_dev->cdev, specs_dev->cdevno, 1/*count*/);
  if (rc < 0) {
    printk("cdev_add() = %d\n", rc);
    goto fail_add;
  }
  //  specs_dev->vma_ref = NULL;
  specs_dev->virtual_mem = NULL;
  specs_dev->dma_lock = -1;
#ifdef DEBUG_PCI
  printk(KERN_INFO DRV_NAME "= %d:%d\n", MAJOR(specs_dev->cdevno),MINOR(specs_dev->cdevno) );
#endif
  return 0;
 fail_add:
  /* free the dynamically allocated character device node */
  unregister_chrdev_region(specs_dev->cdevno, 1/*count*/);
 fail_alloc: return -1;
}

/** @detail specs_dev_exit() - Cleanup character device
 *
 * XXX Should ideally be tied to the device, on device remove, not module exit.
 */

static void specs_dev_exit(struct Aria5Specs_dev *specs_dev) {
  printk(KERN_INFO DRV_NAME " specs_dev exit()\n");
  /* remove the character device */
  if (specs_dev == NULL) return;
  cdev_del(&specs_dev->cdev);
  /* free the dynamically allocated character device node */
  unregister_chrdev_region(specs_dev->cdevno, 1);
}



/* used to register the driver with the PCI kernel sub system
 * @see LDD3 page 311
 */
static struct pci_driver pci_driver = { 
  .name = DRV_NAME,
  .id_table = ids,
  .probe = probe, 
  .remove = remove ,
  /* resume, suspend are optional */
};

/**
 * @ AltSpecsV2_init() - Module initialization, registers devices.
 */
static int __init AltSpecsV2_init(void) {
  int rc = 0;
  //struct pci_dev *dev;
  printk(KERN_INFO DRV_NAME " init(), built at " __DATE__ " " __TIME__ "\n");
  /* register this driver with the PCI bus driver */
  rc = pci_register_driver(&pci_driver);
  if (rc < 0) 
    { 
      printk(KERN_ALERT DRV_NAME " : unable to register PCI driver\n");
      unregister_chrdev(major, DRV_NAME);
      return rc;
    }
  return 0;
 
} 

/**
 * @ AltSpecsV2_init() - Module cleanup, unregisters devices.
 */
static void __exit AltSpecsV2_exit(void) {
  printk(KERN_INFO DRV_NAME " exit(), built at " __DATE__ " " __TIME__ "\n");
  /* unregister this driver from the PCI bus driver */
  pci_unregister_driver(&pci_driver);

  unregister_chrdev(major, DRV_NAME );
  printk(KERN_INFO DRV_NAME"  successfully unloaded\n");

}

MODULE_LICENSE("GPL");

module_init(AltSpecsV2_init);
module_exit(AltSpecsV2_exit);

