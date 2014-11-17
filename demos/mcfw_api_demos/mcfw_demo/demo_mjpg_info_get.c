
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int Mjpg_getFrameSize(FILE *stream, unsigned int *frameSize, unsigned int *offset)
{
    int x;

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
        if(x == 0xff)
        {
            x = getc(stream);
            if(x == EOF)
            {
                return -1;
            }
            
            *frameSize += 1;
            if(x == 0xD9)
            {
                return 0;
            }

        }

    }while(1); 

    return 0;
}




#define FILE_NAME_LEN 1024
void Demo_generateMjpgHdrFile(char *filename)
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

    while (Mjpg_getFrameSize(file, &frameSize, &offset) != -1)
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
    Demo_generateMjpgHdrFile(argv[1]);
    return 0;
}
#endif
