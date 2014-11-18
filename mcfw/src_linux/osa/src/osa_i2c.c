

#include <osa_i2c.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>


//#define OSA_I2C_DEBUG

int OSA_i2cOpen(OSA_I2cHndl *hndl, Uint8 instId)
{
    char deviceName[20];
    int status = 0;

    sprintf(deviceName, "/dev/i2c-%d", instId);

    hndl->fd = open(deviceName, O_RDWR);

    if(hndl->fd<0)
        return OSA_EFAIL;

    return status;
}

int OSA_i2cRead8(OSA_I2cHndl *hndl, Uint16 devAddr, Uint8 *reg, Uint8 *value, Uint32 count)
{
    int i, j;
    int status = 0;
    struct i2c_msg * msgs = NULL;
    struct i2c_rdwr_ioctl_data data;

    msgs = (struct i2c_msg *) malloc(sizeof(struct i2c_msg) * count * 2);

    if(msgs==NULL)
    {
        printf(" I2C (0x%02x): Malloc ERROR during Read !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return OSA_EFAIL;
    }

    for (i = 0, j = 0; i < count * 2; i+=2, j++)
    {
        msgs[i].addr  = devAddr;
        msgs[i].flags = 0;
        msgs[i].len   = 1;
        msgs[i].buf   = &reg[j];

        msgs[i+1].addr  = devAddr;
        msgs[i+1].flags = I2C_M_RD /* | I2C_M_REV_DIR_ADDR */;
        msgs[i+1].len   = 1;
        msgs[i+1].buf   = &value[j];
    }

    data.msgs = msgs;
    data.nmsgs = count * 2;

    status = ioctl(hndl->fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
        #ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Read ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        #endif
    }
    else
        status = OSA_SOK;

    free(msgs);

    return status;
}

int OSA_i2cWrite8(OSA_I2cHndl *hndl, Uint16 devAddr,  Uint8 *reg, Uint8 *value, Uint32 count)
{
    int i,j;
    unsigned char * bufAddr;
    int status = 0;

    struct i2c_msg * msgs = NULL;
    struct i2c_rdwr_ioctl_data data;

    msgs = (struct i2c_msg *) malloc(sizeof(struct i2c_msg) * count);

    if(msgs==NULL)
    {
        printf(" I2C (0x%02x): Malloc ERROR during Write !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return OSA_EFAIL;
    }

    bufAddr = (unsigned char *) malloc(sizeof(unsigned char) * count * 2);

    if(bufAddr == NULL)
    {
        free(msgs);

        printf(" I2C (0x%02x): Malloc ERROR during Write !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        return OSA_EFAIL;

    }

    for (i = 0, j = 0; i < count; i++, j+=2)
    {
        bufAddr[j] = reg[i];
        bufAddr[j + 1] = value[i];

        msgs[i].addr  = devAddr;
        msgs[i].flags = 0;
        msgs[i].len   = 2;
        msgs[i].buf   = &bufAddr[j];
    }
    data.msgs = msgs;
    data.nmsgs = count;

    status = ioctl(hndl->fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
        #ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Write ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, reg[0], count);
        #endif
    }
    else
        status = OSA_SOK;

    free(msgs);
    free(bufAddr);

    return status;
}

int OSA_i2cRawWrite8(OSA_I2cHndl *hndl, Uint16 devAddr, Uint8 *value, Uint32 count)
{
    int status = 0;

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data data;

    msgs[0].addr  = devAddr;
    msgs[0].flags = 0;
    msgs[0].len   = count;
    msgs[0].buf   = value;

    data.msgs = msgs;
    data.nmsgs = 1;

    status = ioctl(hndl->fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
        #ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Raw Write ERROR !!! (count = %d)\n", devAddr, count);
        #endif
    }
    else
        status = OSA_SOK;

    return status;
}


int OSA_i2cRawRead8(OSA_I2cHndl *hndl, Uint16 devAddr, Uint8 *value, Uint32 count)
{
    int status = 0;

    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data data;

    msgs[0].addr  = devAddr;
    msgs[0].flags = I2C_M_RD;
    msgs[0].len   = count;
    msgs[0].buf   = value;

    data.msgs = msgs;
    data.nmsgs = 1;

    status = ioctl(hndl->fd, I2C_RDWR, &data);
    if(status < 0)
    {
        status = OSA_EFAIL;
        #ifdef OSA_I2C_DEBUG
        printf(" I2C (0x%02x): Raw Read ERROR !!! count = %d)\n", devAddr, count);
        #endif
    }
    else
        status = OSA_SOK;

    return status;
}

int OSA_i2cClose(OSA_I2cHndl *hndl)
{
    return close(hndl->fd);
}


int OSA_i2cTestShowUsage(char *str)
{
  printf(" \n");
  printf(" I2C Test Utility, \r\n");
  printf(" Usage: %s -r|-w <devAddrInHex> <regAddrInHex> <regValueInHex or numRegsToReadInDec> \r\n", str);
  printf(" \n");
  return 0;
}

int OSA_i2cTestMain(int argc, char **argv)
{
  OSA_I2cHndl i2cHndl;
  Uint8 devAddr, numRegs;
  Bool doRead;
  int status, i;

  static Uint8 regAddr[I2C_TRANSFER_SIZE_MAX], regValue8[I2C_TRANSFER_SIZE_MAX];

  if(argc<3) {
    OSA_i2cTestShowUsage(argv[0]);
    return -1;
  }

  if(strcmp(argv[1], "-r")==0)
    doRead=TRUE;
  else
  if(strcmp(argv[1], "-w")==0)
    doRead=FALSE;
  else {
    OSA_i2cTestShowUsage(argv[0]);
    return -1;
  }

  devAddr = 0;
  numRegs = 4;
  regValue8[0] = 0;
  regAddr[0] = 0;

  if(argc>2)
    devAddr = xstrtoi(argv[2]);

  if(argc>3)
    regAddr[0] = xstrtoi(argv[3]);

  if(argc>4) {
    if(doRead)
    {
      numRegs = atoi(argv[4]);
      if(numRegs>I2C_TRANSFER_SIZE_MAX)
        numRegs = I2C_TRANSFER_SIZE_MAX;
    }
    else {
      regValue8[0] = xstrtoi(argv[4]);
    }
  }

  if(devAddr==0) {
    printf(" I2C: Invalid device address\n");
    OSA_i2cTestShowUsage(argv[0]);
    return -1;
  }

  status = OSA_i2cOpen(&i2cHndl, I2C_DEFAULT_INST_ID);

  if(status != OSA_SOK) {
    OSA_ERROR("OSA_i2cOpen( instId = %d )\n", I2C_DEFAULT_INST_ID);
    return status;
  }

  if(status==OSA_SOK)
  {
    if(doRead) {
      for(i=0; i<numRegs; i++)
        regValue8[i] = 0;

      for(i=1; i<numRegs; i++)
      {
        regAddr[i] = regAddr[0]+i;
      }

      status = OSA_i2cRead8(&i2cHndl, devAddr, regAddr, regValue8, numRegs);

      if(status==OSA_SOK) {
        for(i=0; i<numRegs; i++) {
          printf(" I2C (0x%02x): 0x%02x = 0x%02x \n", devAddr, regAddr[i], regValue8[i] );
        }
      } else {
        printf(" I2C (0x%02x): Read ERROR !!! (reg[0x%02x], count = %d)\n", devAddr, regAddr[0], numRegs);
      }

    } else {
        status = OSA_i2cWrite8(&i2cHndl, devAddr, regAddr, regValue8, 1);
        if(status==OSA_SOK) {
          status = OSA_i2cRead8(&i2cHndl, devAddr, regAddr, regValue8, 1);
        }

        if(status==OSA_SOK) {
            printf(" I2C (0x%02x): 0x%02x = 0x%02x \n", devAddr, regAddr[0], regValue8[0] );
        } else {
            printf(" I2C (0x%02x): Write ERROR !!! (reg[0x%02x], value = 0x%02x\n", devAddr, regAddr[0], regValue8[0]);
        }
    }

    OSA_i2cClose(&i2cHndl);
  }

  return 0;
}

