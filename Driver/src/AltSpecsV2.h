

#ifndef _LINUX_ALTERAPCIECHDMA
#define _LINUX_ALTERAPCIECHDMA

//#define DRIVER_VERSION  1001113
//#define DRIVER_VERSION  11130102
//#define DRIVER_VERSION  1140301
//#define DRIVER_VERSION  1140401
//#define DRIVER_VERSION    29042014 //compatible pof _io_2 bug master 2 
//#define DRIVER_VERSION    7141300  // iSpecs_Master_PCIe.sof du projet xxxxgen1_serie de 22:15
//#define DRIVER_VERSION    29072014
//#define DRIVER_VERSION    3140601 

#define DRIVER_VERSION    2151302  // Specs_Master_PCIe.sof du projet Specs_Master_PCIE_MTCA_13_4x_gen!_serie10

//#ifndef PCI_CODE
typedef enum _ACCESS_TYPE
{
    BitSize8,
    BitSize16,
    BitSize32,
    BitSize64
} ACCESS_TYPE;


// Device Location Structure
typedef struct _DEVICE_LOCATION
{
    unsigned char  BusNumber;
    unsigned char  SlotNumber;
    unsigned short DeviceId;
    unsigned short VendorId;
    unsigned char  SerialNumber[20];
} DEVICE_LOCATION;

//#endif
typedef struct _BUSIOPDATA
{
    int                    MasterID;
    unsigned long          Address;
    void *                 Buffer;
    unsigned int           TransferSize;
    int                    bRemap;
    ACCESS_TYPE            AccessType;
} BUSIOPDATA;

typedef struct _WAIT_OBJ
{
  unsigned short masterId;
  unsigned long long timeout;
} WAIT_OBJ;

typedef struct _INTERRUPT_OBJ
{ 
  unsigned int stat_reg;
  unsigned short pio_line;
} INTERRUPT_OBJ;

#define NB_MASTER 16 
#define MAX_IRQ 16
#define IRQ_FIFO0 4 // numero IRQ FIFO0 les autres doivent suivrent sinon il faudra un tableau

/** driver name, mimicks Altera naming of the reference design */
#define DRV_NAME "AltSpecsV2"
/** number of BARs on the device */
#define SIGTEST 44
/** BAR number where the Descriptor Header sits */
#define BARS 3
#define BAR0 0
#define BAR2 2

/** maximum size in bytes of the descriptor table, chdma logic limit */
#define APE_CHDMA_TABLE_SIZE (4096)
/* single transfer must not exceed 255 table entries. worst case this can be
 * achieved by 255 scattered pages, with only a single byte in the head and
 * tail pages. 253 * PAGE_SIZE is a safe upper bound for the transfer size.
 */
#define PAGE_SHIFT	12
#define PAGE_SZ	(1UL << PAGE_SHIFT)
#define APE_CHDMA_MAX_TRANSFER_LEN (253 * PAGE_SZ)
#define NB_TABLE 2 //MTQ
#define DMA_TRANSFERT NB_TABLE*PAGE_SZ* 0x4 // a revoir si dmasz > 1000
#define DATA_SIZE 2*DMA_TRANSFERT // Pour les 2 fibres
#define OFFSET_FIBER1 DMA_TRANSFERT
#define OFFSET_FIBER0 0

#define OUTPUT_FIFO_OUT_FIFO_DEPTH 512
#define ALMOST_EMPTY 2
//#define ALMOST_FULL OUTPUT_FIFO_OUT_FIFO_DEPTH-5
#define ALMOST_FULL 2 
#define IT_FIFO   1 
#define IT_STATUS 2
static const unsigned long bar_min_len[BARS] = { 4194304, 0, 32768 }; // see lspci -v
// StratixII registers adddress

#define TRANS_TABLE_OFFSET 0x1000

#define ENABLE  0x10
#define DISABLE 0xFFFFFFEF
  
#define BYTE                    0x1
#define HALF_WORD               0x2
#define WORD                    0x4
#define GO                      0x8
#define IRQ                     0x10
#define READ_ENDS_TRANSACTION   0x20
#define WRITE_ENDS_TRANSACTION  0x40
#define LENGHT_ENDS_TRANSACTION 0x80
#define READ_FROM_CONST_ADDR    0x100
#define WRITE_TO_CONST_ADDR     0x200
#define DOUBLE_WORD             0x400
#define QUAD_WORD               0x800
#define SOFT_RESET              0x1000

  
#define IOCTL_FIND_DEVICE                         _IOW ('M',0x18, DEVICE_LOCATION )
#define IOCTL_NOTIFICATION_REGISTER               _IOW ('M',0x19, int)

#define IOCTL_NOTIFICATION_CANCEL                 _IOW ('M',0x20, WAIT_OBJ)
#define IOCTL_NOTIFICATION_WAIT                   _IOWR ('M',0x21,WAIT_OBJ)
#define IOCTL_BUS_IOP_READ                        _IOW ('M',0x22, BUSIOPDATA )
#define IOCTL_BUS_IOP_WRITE                       _IOW ('M',0x23, BUSIOPDATA )
#define IOCTL_ENABLE_PIO_INTERRUPT                _IOW ('M',0x24, INTERRUPT_OBJ )
#define IOCTL_DISABLE_PIO_INTERRUPT               _IOW ('M',0x25, INTERRUPT_OBJ )
#define IOCTL_FIFOS_INIT                          _IOW ('M',0x26, int )
#define IOCTL_READ_FILL_LEVEL                     _IOW ('M',0x27, BUSIOPDATA )
#define IOCTL_OPEN_MASTER                         _IOW ('M',0x28, int )
#define IOCTL_CLOSE_MASTER                        _IOW ('M',0x29, int )
#define IOCTL_RESET                               _IOW ('M',0x30, int )
#define IOCTL_CHECK_SLAVE_IT                      _IOW ('M',0x31, int )

/* obtain the 32 most significant (high) bits of a 32-bit or 64-bit address */
#define pci_dma_h(addr) ((addr >> 16) >> 16)
/* obtain the 32 least significant (low) bits of a 32-bit or 64-bit address */
#define pci_dma_l(addr) (addr & 0xffffffffUL)

#define VALID 1
#define NO_VALID 0	 
#define GET_BLOCK get_block(i)

/* OLD version
#define  PCI_EXPRESS_REGISTER 0x40
#define  AVALON_DMA_CONTROLLER 0x4000
#define  CHIP_MEM_BASE0 0x200000
#define  PIO_IN 0x4040
#define  SPECSM0_MEM    0x5000
*/
/* NEw version
#define  PCI_EXPRESS_REGISTER 0x40
#define  AVALON_DMA_CONTROLLER 0x4000
#define  CHIP_MEM_BASE0 0x8000
#define  SPECSM0_MEM    0x6000
#define  PIO_IN 0x20
#define TX_ADDR 0x20000000
*/
#if  DRIVER_VERSION == 1001113 
#define BASE_ADDRESS 0
#define  PCI_EXPRESS_REGISTER 0x40 
#define  AVALON_DMA_CONTROLLER 0x4020
#define  CHIP_MEM_BASE0 0x202000
#define  SPECSM0_MEM    0x200000
#define  PIO_IN 0x4060  + BASE_ADDRESS
#define  TX_ADDR 0x0
#define  VERSION 0x4040 + BASE_ADDRESS
#define  PIO_CTRL_REG 0x4000 + BASE_ADDRESS
#define  PIO_STATUS_REG 0x4060 + BASE_ADDRESS
#define  FIFO_RECEPTION_IN 0x4070 + BASE_ADDRESS
#define  FIFO_EMISSION_OUT 0x4074 +BASE_ADDRESS
#define  RESET 0x4050 +BASE_ADDRESS 
#endif
#if  DRIVER_VERSION == 1140301
#define  BASE_ADDRESS           0x1000000
#define  PCI_EXPRESS_REGISTER   0x40 
#define  AVALON_DMA_CONTROLLER  0x4000
#define  CHIP_MEM_BASE0         0x8000
#define  SPECSM0_MEM            0x6000
//#define  PIO_IN                 0x40  + BASE_ADDRESS
#define  TX_ADDR                0x200000

#define  SYSID                  0x58 +BASE_ADDRESS 
#define  RESET                  0x60 +BASE_ADDRESS 
#define  VERSION                0x70 + BASE_ADDRESS

// PREMIER MASTER

#define  PIO_CTRL_REG           0x120 + BASE_ADDRESS
#define  PIO_STATUS_REG         0x140 + BASE_ADDRESS
#define  FIFO_RECEPTION_IN      0x160 + BASE_ADDRESS
#define  FIFO_EMISSION_OUT      0x180 +BASE_ADDRESS
#define  FIFO_RECEPTION_STATUS  0x80 + BASE_ADDRESS
#define  FIFO_EMISSION_STATUS   0x0 + BASE_ADDRESS

#define MASTERS_OFFSET 0x100

#endif
#if  DRIVER_VERSION == 1140401
#define  BASE_ADDRESS           0x1000000
#define  PCI_EXPRESS_REGISTER   0x40 
#define  AVALON_DMA_CONTROLLER  0x4000
#define  CHIP_MEM_BASE0         0x8000
#define  SPECSM0_MEM            0x6000
//#define  PIO_IN                 0x40  + BASE_ADDRESS
#define  TX_ADDR                0x200000

#define  SYSID                  0x58 +BASE_ADDRESS 
#define  RESET                  0x60 +BASE_ADDRESS 
#define  VERSION                0x70 + BASE_ADDRESS

// PREMIER MASTER

#define  PIO_CTRL_REG           0x100 + BASE_ADDRESS
#define  PIO_STATUS_REG         0x120 + BASE_ADDRESS
#define  FIFO_RECEPTION_IN      0x160 + BASE_ADDRESS
#define  FIFO_EMISSION_OUT      0x130 + BASE_ADDRESS
#define  FIFO_RECEPTION_STATUS  0x180 + BASE_ADDRESS
#define  FIFO_EMISSION_STATUS   0x140 + BASE_ADDRESS
// ATTENTION PAS DE VALEUS < 0 DANS LES VALEURS SUIVANTES
#define  OFFSET_FIFO_REC_IN_STATUS      0x20                    // FIFO_RECEPTION_STATUS - FIFO_RECEPTION_IN  
#define  OFFSET_FIFO_EMI_OUT_STATUS     0x10                    // FIFO_EMISSION_OUT - FIFO_EMISSION_STATUS
#define  OFFSET_FIFO_IN_CTRL            0x60                    // FIFO_RECEPTION_IN - PIO_CTRL_REG
#define  MASK_FIFO_IN 0X7F // Pour trouver si c'est une addresse FIFO_RECEPTION_IN
#define MASTERS_OFFSET 0x100

#endif
#if  DRIVER_VERSION ==  3140501
#define  BASE_ADDRESS           0x1000000
#define  OFFSET_CROSSING_BRIDGE    0x1000
#define  PCI_EXPRESS_REGISTER        0x40 
#define  AVALON_DMA_CONTROLLER     0x4000
#define  CHIP_MEM_BASE0            0x8000
#define  SPECSM0_MEM               0x6000
//#define  PIO_IN                 0x40  + BASE_ADDRESS
#define  TX_ADDR                 0x200000
 
#define  SYSID                   0x30 + BASE_ADDRESS 
#define  RESET                   0x60 + BASE_ADDRESS 
#define  VERSION                 0x70 + BASE_ADDRESS
#define  PIO_MASTERS_STATUS_IRQS 0x80 + BASE_ADDRESS
#define  PIO_MASTERS_FIFO_IRQS   0x20 + BASE_ADDRESS 
// PREMIER MASTER
#define PIO_TEST               (   0x0 + BASE_ADDRESS  )
#define MEM_TEST               (   0x800 + BASE_ADDRESS )
#define  PIO_CTRL_REG         (  0x100 + BASE_ADDRESS  )
#define  PIO_STATUS_REG        ( 0x120 + BASE_ADDRESS  )
#define  FIFO_EMISSION_OUT     ( 0x130 + BASE_ADDRESS )
#define  FIFO_EMISSION_STATUS  ( 0x140 + BASE_ADDRESS )
#define  FIFO_RECEPTION_IN     ( 0x160 + BASE_ADDRESS )
#define  FIFO_RECEPTION_STATUS  (0x180 + BASE_ADDRESS )

// ATTENTION PAS DE VALEURS < 0 DANS LES VALEURS SUIVANTES
#define  OFFSET_FIFO_EMI_OUT_PIO_STATUS 0x10      
#define  OFFSET_FIFO_REC_IN_STATUS      0x20                    // FIFO_RECEPTION_STATUS - FIFO_RECEPTION_IN    
#define  OFFSET_FIFO_EMI_OUT_STATUS     0x10                    // FIFO_EMISSION_OUT - FIFO_EMISSION_STATUS
#define  OFFSET_FIFO_IN_CTRL            0x60      // FIFO_RECEPTION_IN - PIO_CTRL_REG
#define  MASK_FIFO_IN 0x7F // Pour trouver si c'est une addresse FIFO_RECEPTION_IN
#define  MASTERS_OFFSET 0x100  // Pour passer d'un master au suivant

#endif
//#if  DRIVER_VERSION ==  3140601
#if  DRIVER_VERSION == 29042014 ||  DRIVER_VERSION == 29072014 || DRIVER_VERSION ==  7141300 || DRIVER_VERSION ==  2151302 
#define  BASE_ADDRESS           0x1000000
#define  OFFSET_CROSSING_BRIDGE    0x1000
#define  PCI_EXPRESS_REGISTER        0x40 
#define  AVALON_DMA_CONTROLLER     0x4000
#define  CHIP_MEM_BASE0            0x8000
#define  SPECSM0_MEM               0x6000
//#define  PIO_IN                 0x40  + BASE_ADDRESS
#define  TX_ADDR                 0x20000

#define  SYSID                   0x30 + BASE_ADDRESS 
#define  RESET                   0x60 + BASE_ADDRESS 
#define  VERSION                 0x70 + BASE_ADDRESS
#define  PIO_MASTERS_STATUS_IRQS 0x40 + BASE_ADDRESS
#define  PIO_MASTERS_FIFO_IRQS   0x20 + BASE_ADDRESS 

// PREMIER MASTER
#define PIO_TEST               (   0x0 + BASE_ADDRESS  )
#define MEM_TEST               (   0x800 + BASE_ADDRESS )
#define  PIO_CTRL_REG         (  0x100 + BASE_ADDRESS  )
#define  PIO_STATUS_REG        ( 0x120 + BASE_ADDRESS  )
#define  FIFO_EMISSION_OUT     ( 0x130 + BASE_ADDRESS )
#define  FIFO_EMISSION_STATUS  ( 0x140 + BASE_ADDRESS )
#define  FIFO_RECEPTION_IN     ( 0x160 + BASE_ADDRESS )
#define  FIFO_RECEPTION_STATUS  (0x180 + BASE_ADDRESS )

// ATTENTION PAS DE VALEURS < 0 DANS LES VALEURS SUIVANTES
#define  OFFSET_FIFO_EMI_OUT_PIO_STATUS 0x10                   
#define  OFFSET_FIFO_REC_IN_STATUS      0x20                    // FIFO_RECEPTION_STATUS - FIFO_RECEPTION_IN    
#define  OFFSET_FIFO_EMI_OUT_STATUS     0x10                    // FIFO_EMISSION_OUT - FIFO_EMISSION_STATUS
#define  OFFSET_FIFO_IN_CTRL            0x60      // FIFO_RECEPTION_IN - PIO_CTRL_REG

#define  MASK_FIFO_IN 0x7F // Pour trouver si c'est une addresse FIFO_RECEPTION_IN
#define  MASTERS_OFFSET 0x100  // Pour passer d'un master au suivant

#endif
#endif
