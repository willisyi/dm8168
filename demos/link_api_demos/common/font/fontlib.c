#ifndef _UNICODE_CN_422I_420_30_C_
#define _UNICODE_CN_422I_420_30_C_
#include <stdlib.h>
#include<stdio.h>

#define P_ROWS_ 16
#define P_CLUMNS_ 32
#define font 16
#define p_clu 64

//unsigned char *tit;
void osd16(char *dest,char *src,int len)
{
	int i,j,k,offset;
	i=0;k=0;
	for(j=0;j<len*P_ROWS_;j++)
	{
		offset=(P_ROWS_*(k%len)+i)*P_CLUMNS_;
		memcpy(dest+j*P_CLUMNS_,src+offset,P_CLUMNS_);
		k++;
		if (k%len==0&&k!=0)
		 {
			 i++;
		 }
	}
}

void osd32(char *dest,char *src,int len)
{
	int i,j,k,offset;
	i=0;k=0;
	for(j=0;j<len*P_CLUMNS_;j++)
	{
		offset=(P_CLUMNS_*(k%len)+i)*p_clu;
		memcpy(dest+j*p_clu,src+offset,p_clu);
		k++;
		if (k%len==0&&k!=0)
		 {
			 i++;
		 }
	}
}
/*
*将输入的字符串转换为汉字机内码
主要是因为char*中英文字符采用了ASSIC码
*/
int fontdect(char *dest,char src[])
{
	int i,k,len;
	//unsigned char *title=(unsigned char*)malloc(16*16*sizeof(char));
	unsigned char *title;
	title=src;//title中存放的是每个字符的低字节
	len=strlen((char*)src);
		
	for (i=0;i<len;i++)
	{
		if (src[i]<0x80)//对ASCII码进行处理，将其转化为汉字区位码格式
		{	
			k=len;
			//数组后移
			while(k>=i+1)
			{
				title[k+1]=title[k];
				k--;
			}			
			title[i+1]=src[i]-32+0xa0;//-32变成位码，+0xa0位码变成汉字机内码
			title[i]=0x03+0xa0;	//将区码变成汉字机内码，对于ASCII码对应的字符在国标码里全部在0x03区
			i++;
			len++;//扩展后数组长度
		}		
		else
		{
			title[i]=src[i];//可知title存放的是字符的汉字机内码			
		}
	}

	memcpy(dest,title,len);
	//free(title);
	return len;
}

/****************guo*********

********************/
typedef unsigned char BYTE;
void font16(char *incode,char *dest) 
{	   
	//unsigned char incode[3]="南"; // 要读出的汉字，GB编码  
	unsigned char qh = 0, wh = 0;  
	unsigned long offset = 0;  
	BYTE *character_buffer;
	BYTE *string_buffer;
	BYTE a[1]={0};
	BYTE b[1]={1};
	int p[8]={128,64,32,16,8,4,2,1};
	FILE *HZK = 0;  
	int i,j;
	character_buffer=(unsigned char*)malloc(32*sizeof(char));
	string_buffer=(unsigned char*)malloc(16*16*sizeof(char)*2); //yuyv
	qh = incode[0] - 0xa0;	//获得区码  : -128 get guobiaoma,-32 get quweima:total -160
	wh = incode[1] - 0xa0;	//获得位码  
	offset = (94*(qh-1)+(wh-1))*32; //得到偏移位置 
	if((HZK=fopen("HZK16", "rb")) == NULL)  
	{  
		printf("Can't Open hzk16\n");  
		getchar(); 
		return 0; 
	}  

	fseek(HZK, offset, SEEK_SET);  
	fread(character_buffer, 32, 1, HZK); 
	fclose(HZK);

	for (i=0;i<32;i++)
	{
		for (j=0;j<8;j++)
		{
			if(*(character_buffer+i) & p[j])
			{
				memcpy(string_buffer+i*8+j,b,1);
			}
			else
			{
				memcpy(string_buffer+i*8+j,a,1);
			}
		}
	}
	memcpy(dest+0,string_buffer+0,256);

}
/*
*get YUYV data,from a font
@len :the length of font data (sizeof char)
*/
Void font2Yuv(unsigned char *dest,unsigned char *src,int len)
{
    //BYTE a[2]={0x00,0x7D};
BYTE a[2]={0x00,0x00};
    BYTE b[2]={0xFF,0x7D};
    int i;
    for(i=0;i<len ;i++)
    {
        if(src[i])
        {
            memcpy(dest+2*i,b,2);       
	 }
        else
        {
            memcpy(dest+2*i,a,2);
	 }
    }
}

/****
fontsize change 16 to 32 
@char *dest
@char* src
*/
Void font16to32(char *dest,char *src)
{
    int i,j;
    for (i=0;i<16;i++)
    {
        for (j=0;j<16;j++)
        {
  		*(dest+32*(2*i)+2*j)=src[16*i+j];
		*(dest+32*(2*i)+2*j+1)=src[16*i+j];
		*(dest+32*(2*i+1)+2*j)=src[16*i+j];
		*(dest+32*(2*i+1)+2*j+1)=src[16*i+j];
        }
    }
}

char cn_422i_420_30_YuvDataMy[] = {};
UInt16 cn_422i_420_30_WidthListMy[100]={};
/*guo
@allYuvBuf is the destination
*/
UInt32 FontGetAllYUYVData(unsigned char* allYuvbuf,char *src,int fontSize,unsigned  *changdu,unsigned  *mianji )
{
	int j,length;
	int ret=0;
	//get 机内码
	unsigned char* p=(unsigned char*)malloc(16*sizeof(char)*2);//16个汉字,每个汉字两字节表示
	length = fontdect(p,src);
	//给个汉字转换过程所需要的存储
	unsigned char* buffer=(unsigned char*)malloc(sizeof(char)*3);
	unsigned char* fontbuf=(unsigned char*)malloc(16*sizeof(char)*16);
	unsigned char* fontYUVbuf=(unsigned char*)malloc(32*sizeof(char)*32*2);
	unsigned char* font32buf=(unsigned char*)malloc(32*sizeof(char)*32);
  	 for(j=0;j<length;j+=2)//每次取两个字节(一个汉字)
	{
		memcpy(buffer+0,p+j,2);//read a word
		font16(buffer,fontbuf);//根据区位码提取字模数据存入fontbuf
		if(fontSize==16)
		{
			unsigned char a[1]={16};
			memcpy(cn_422i_420_30_WidthListMy+j/2,a,1);
			font2Yuv(fontYUVbuf, fontbuf, 16*16);//将buf转换成yuyv的信息
		}
		if(fontSize==32)
		{	unsigned char a[1]={32};
			memcpy(cn_422i_420_30_WidthListMy+j/2,a,1);
			font16to32(font32buf, fontbuf);
			font2Yuv(fontYUVbuf, font32buf, 32*32);
		}
		else
		{
			printf("Font size not supported!!!\n");
			ret = -1;
		}
		memcpy(allYuvbuf+fontSize*fontSize*j,fontYUVbuf,fontSize*fontSize*2);
	}
	 if(fontSize==16)
	{
		*changdu=16;
		*mianji=*changdu*length/2;
		osd16(cn_422i_420_30_YuvDataMy,allYuvbuf,length/2);
	}
	if(fontSize==32)
	{
		*changdu=32;
		*mianji=*changdu*length/2;
		//memcpy(cn_422i_420_30_YuvData,AllYuvbuf,2048);
		osd32(cn_422i_420_30_YuvDataMy,allYuvbuf,length/2);
	}	
	 free(p);
	 free(buffer);
	 free(fontbuf);
	 free(fontYUVbuf);
	free(font32buf);
	return length;
}


#endif
