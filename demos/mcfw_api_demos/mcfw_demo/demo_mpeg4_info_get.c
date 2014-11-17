
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int Mpeg4_getFrameSize(FILE *stream, unsigned int *frameSize, unsigned int *offset)
{
    int x;
    unsigned int state = 0;
    int vop_found;

    *frameSize = 0;
    *offset = ftell(stream);
    do 
    {
        x = getc(stream);
        if(x == EOF)
        {
            return -1;
        }

        *frameSize += 1;

        state = (state<<8) | x;
        if(state == 0x1B6)
        {
            vop_found = 1;
            break;
        }
    }while(1);

    do 
    {
        x = getc(stream);
        if(x == EOF)
        {
            return -1;
        }

        *frameSize += 1;

        state= (state<<8) | x;
        if((state&0xFFFFFF00) == 0x100)
        {
            *frameSize -= 4;
            fseek(stream,-4,SEEK_CUR);
            return 0;
        }
    }while(1);

    return 0;
}




#define FILE_NAME_LEN 1024
void Demo_generateMpeg4HdrFile(char *filename)
{
    FILE *file ,*file2;
    unsigned int offset, frameSize;

    char frameSizeFileName[FILE_NAME_LEN];

    unsigned int Total     = 0;

    file = fopen(filename, "rb" );

    bzero(frameSizeFileName,FILE_NAME_LEN);
    strcat(frameSizeFileName, filename);
    strcat(frameSizeFileName, ".hdr");
    
    if(file == NULL)
    {
        printf("Error: can't open Input file.\n");
        exit(1);
    }
    else
    {
        printf(" Input file [%s] opened successfully !!!\n", filename);
    }

    if((file2 = fopen(frameSizeFileName, "w+")) == NULL)
    {
        printf("Error: Cannot open Output file.\n");
        exit(1);
    }
    else
    {
        printf(" Output file [%s] opened successfully !!!\n", frameSizeFileName);
    }

    while (Mpeg4_getFrameSize(file, &frameSize, &offset) != -1)
    {
	    Total += frameSize;
	    //print out the offset as the same
        //fprintf(file2,"%d + %d = %x\n",frameSize,offset,(frameSize+offset));
        fprintf(file2,"%d\n",frameSize);
    }

    if (frameSize != 0)
        fprintf(file2,"%d\n",frameSize);

    fclose(file);
    fclose(file2);
}
#if 0 // compile it alone to get the header parser on PC
int main(int argc , char * argv[])
{
    Demo_generateMpeg4HdrFile(argv[1]);
    return 0;
}
#endif
