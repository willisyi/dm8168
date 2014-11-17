#ifndef __PCI_EP__
#define __PCI_EP__

#define u32 unsigned long

struct pcie_buffer
{
    u32 status;
    u32 index;
    u32 length;
    u32 physical_addr;
};

struct pcie_buffer_info
{   
    u32 index;
    u32 length;
    u32 physical_addr;
    char *user_addr;
};

#define PCI_EP_GET_TXBUFF 10
#define PCI_EP_GET_RXBUFF 11
#define PCI_EP_DQ_TXBUFF 12
#define PCI_EP_EQ_TXBUFF 13
#define PCI_EP_DQ_RXBUFF 14
#define PCI_EP_EQ_RXBUFF 15

#endif
