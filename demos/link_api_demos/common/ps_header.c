/*
*
* Author: Sue
* Date: 2013-02
*
*/


#include "Pack_Header_Def.h"

/*
* make_ps_packet_header
* input:
*     _iFrameIndex: the index of packing frame
* output:
*     *pHeader:
* return the length of ps packet header
*/


void ps_header_tag(PS_HEADER_tag *PSHeader_tag)
//void ps_header_tag(struct PS_HEADER_tag PSHeader_tag)
{
	PSHeader_tag->pack_start_code[0] = 0x00;
	PSHeader_tag->pack_start_code[1] = 0x00;
	PSHeader_tag->pack_start_code[2] = 0x01;
	PSHeader_tag->pack_start_code[3] = 0xBA;
	PSHeader_tag->fix_bit = 0x01;	
	PSHeader_tag->marker_bit = 0x01;
	PSHeader_tag->marker_bit1 = 0x01;
	PSHeader_tag->marker_bit2 = 0x01;
	PSHeader_tag->marker_bit3 = 0x01;
	PSHeader_tag->marker_bit4 = 0x01;
	PSHeader_tag->marker_bit5 = 0x01;
	PSHeader_tag->reserved = 0x1F;
	PSHeader_tag->pack_stuffing_length = 0x00;
	PSHeader_tag->system_clock_reference_extension1 = 0;
	PSHeader_tag->system_clock_reference_extension2 = 0;
}
void getSystem_clock_reference_base(PS_HEADER_tag *PSHeader_tag, UINT64 _ui64SCR)
//void getSystem_clock_reference_base(struct PS_HEADER_tag PSHeader_tag, UINT64 _ui64SCR)
{
	_ui64SCR = (PSHeader_tag->system_clock_reference_base1 << 30) | (PSHeader_tag->system_clock_reference_base21 << 28)
		| (PSHeader_tag->system_clock_reference_base22 << 20) | (PSHeader_tag->system_clock_reference_base23 << 15)
		| (PSHeader_tag->system_clock_reference_base31 << 13) | (PSHeader_tag->system_clock_reference_base32 << 5)
		| (PSHeader_tag->system_clock_reference_base33);
	
}
void setSystem_clock_reference_base(PS_HEADER_tag *PSHeader_tag, UINT64 _ui64SCR)
//void setSystem_clock_reference_base(struct PS_HEADER_tag PSHeader_tag, UINT64 _ui64SCR)
{
	PSHeader_tag->system_clock_reference_base1 = (_ui64SCR >> 30) & 0x07;
	PSHeader_tag->system_clock_reference_base21 = (_ui64SCR >> 28) & 0x03;
	PSHeader_tag->system_clock_reference_base22 = (_ui64SCR >> 20) & 0xFF;
	PSHeader_tag->system_clock_reference_base23 = (_ui64SCR >> 15) & 0x1F;
	PSHeader_tag->system_clock_reference_base31 = (_ui64SCR >> 13) & 0x03;
	PSHeader_tag->system_clock_reference_base32 = (_ui64SCR >> 5) & 0xFF;
	PSHeader_tag->system_clock_reference_base33 = _ui64SCR & 0x1F;
}
void getProgram_mux_rate(PS_HEADER_tag *PSHeader_tag, unsigned int _uiMux_rate)
//void getProgram_mux_rate(struct PS_HEADER_tag PSHeader_tag, unsigned int _uiMux_rate)
{
	_uiMux_rate = (PSHeader_tag->program_mux_rate1 << 14) | (PSHeader_tag->program_mux_rate2 << 6) | PSHeader_tag->program_mux_rate3;
}
void setProgram_mux_rate(PS_HEADER_tag *PSHeader_tag, unsigned int _uiMux_rate)
//void setProgram_mux_rate(struct PS_HEADER_tag PSHeader_tag, unsigned int _uiMux_rate)
{
	PSHeader_tag->program_mux_rate1 = (_uiMux_rate >> 14) & 0xFF;
	PSHeader_tag->program_mux_rate2 = (_uiMux_rate >> 6) & 0xFF;
	PSHeader_tag->program_mux_rate3 = _uiMux_rate & 0x3F;
}

void ps_System_Header_tag(Ps_System_Header_tag *System_Header_tag)
//void ps_System_Header_tag(struct Ps_System_Header_tag System_Header_tag)
	{
		System_Header_tag->system_header_start_head[0]=0x00;
		System_Header_tag->system_header_start_head[1]=0x00;
		System_Header_tag->system_header_start_head[2]=0x01;
		System_Header_tag->system_header_start_head[3]=0xBB;
		/*
		*  其它流组合并未列出，可根据实际情况进行扩充，或重新定义函数接口
		*/
		#ifdef AUDIO_VIDEO_STREAM
				System_Header_tag->header_length[0] = 0x00;
				System_Header_tag->header_length[1] = 0x0C;  // the length in bytes of the system header following the header_length field
		#endif

		#ifndef AUDIO_VIDEO_STREAM
				System_Header_tag->header_length[0] = 0x00;
				System_Header_tag->header_length[1] = 0x09;
		#endif

				System_Header_tag->fixed_flag = 0x00;
				System_Header_tag->CSPS_flag = 0x00;
				System_Header_tag->system_audio_local_flag = 0x00;
				System_Header_tag->system_video_local_flag = 0x00;

				System_Header_tag->packet_rate_restriction_flag = 0x01;
				System_Header_tag->reserved = 0x7F;

				System_Header_tag->res_1 = 0x03;
				
				System_Header_tag->maker_bit_1 = 0x01;
				System_Header_tag->maker_bit_2 = 0x01;
				System_Header_tag->maker_bit_3 = 0x01;
		//#ifndef AUDIO_STREAM		
				System_Header_tag->streamid_1 =  0xE0;
		//#endif
		#ifdef AUDIO_VIDEO_STREAM
				System_Header_tag->streamid_2 = 0xC0;
				System_Header_tag->pstd_buf_bound_scale_2 = 0x00; // 
				System_Header_tag->res_2 = 0x03;
			
		#endif

	}
void get_rate_bound(Ps_System_Header_tag *System_Header_tag,unsigned int _ui_rate_bound)
//void get_rate_bound(struct Ps_System_Header_tag System_Header_tag,unsigned int _ui_rate_bound)
{
	_ui_rate_bound = (System_Header_tag->rate_bound_1 << 15) | (System_Header_tag->rate_bound_2 << 7) | System_Header_tag->rate_bound_3;
}
void set_rate_bound(Ps_System_Header_tag *System_Header_tag,unsigned int _ui_rate_bound)
//void set_rate_bound(struct Ps_System_Header_tag System_Header_tag,unsigned int _ui_rate_bound)
{
	System_Header_tag->rate_bound_1 = (_ui_rate_bound >> 15) & 0x7F;
	System_Header_tag->rate_bound_2 = (_ui_rate_bound >> 7) & 0xFF;
	System_Header_tag->rate_bound_3 = _ui_rate_bound & 0x7F;
}
void set_pstd_buf_size_bound_1(Ps_System_Header_tag *System_Header_tag,unsigned int _ui_pstd_buf_size_bound)
//void set_pstd_buf_size_bound_1(struct Ps_System_Header_tag System_Header_tag,unsigned int _ui_pstd_buf_size_bound)
{
	System_Header_tag->pstd_buf_size_bound_11 = (_ui_pstd_buf_size_bound >> 8) & 0x1F;
	System_Header_tag->pstd_buf_size_bound_12 = _ui_pstd_buf_size_bound & 0xFF;
}
void set_pstd_buf_size_bound_2(Ps_System_Header_tag *System_Header_tag,unsigned int _ui_pstd_buf_size_bound)
//void set_pstd_buf_size_bound_2(struct Ps_System_Header_tag System_Header_tag,unsigned int _ui_pstd_buf_size_bound)
{
	System_Header_tag->pstd_buf_size_bound_21 = (_ui_pstd_buf_size_bound >> 8) & 0x1F;
	System_Header_tag->pstd_buf_size_bound_22 = _ui_pstd_buf_size_bound & 0xFF;
}
void pes_HEADER_tag(PES_HEADER_tag *pes_header_tag)
//void pes_HEADER_tag(struct PES_HEADER_tag pes_header_tag)
{
	pes_header_tag->packet_start_code_prefix[0] = 0x00;
	pes_header_tag->packet_start_code_prefix[1] = 0x00;
	pes_header_tag->packet_start_code_prefix[2] = 0x01;
	
	pes_header_tag->PES_packet_length[0] = 0x00;
	pes_header_tag->PES_packet_length[1] = 0x00;
	
	pes_header_tag->fix_bit = 0x02;
}

void pts_Tag(PTS_tag *pts_tag)
//void pts_Tag(struct PTS_tag pts_tag)
{
	pts_tag->fix_bit = 0x02;//PTS Only
	pts_tag->marker_bit = 0x01;
	pts_tag->marker_bit1 = 0x01;
	pts_tag->marker_bit2 = 0x01;
}
void getPTS(PTS_tag *pts_tag,UINT64 _ui64PTS)
//void getPTS(struct PTS_tag pts_tag,UINT64 _ui64PTS)
{
	_ui64PTS = (pts_tag->PTS1 << 30) | (pts_tag->PTS21 << 22)
		| (pts_tag->PTS22 << 15) | (pts_tag->PTS31 << 7) | (pts_tag->PTS32);
	
}
void setPTS(PTS_tag *pts_tag,UINT64 _ui64PTS)
//void setPTS(struct PTS_tag pts_tag,UINT64 _ui64PTS)
{
	pts_tag->PTS1 = (_ui64PTS >> 30) & 0x07;
	pts_tag->PTS21 = (_ui64PTS >> 22) & 0xFF;
	pts_tag->PTS22 = (_ui64PTS >> 15) & 0x7F;
	pts_tag->PTS31 = (_ui64PTS >> 7) & 0xFF;
	pts_tag->PTS32 = _ui64PTS & 0x7F;
}
void psm_Tag(PSM_tag *psm_tag)
//void psm_Tag(struct PSM_tag psm_tag)
{
	psm_tag->packet_start_code_prefix[0] = 0x00;
	psm_tag->packet_start_code_prefix[1] = 0x00;
	psm_tag->packet_start_code_prefix[2] = 0x01;
	
	psm_tag->map_stream_id = 0xBC;
	
#ifndef AUDIO_VIDEO_STREAM
	psm_tag->program_stream_map_length[0] = 0x00;
	psm_tag->program_stream_map_length[1] = 0x0E;
	
	psm_tag->elementary_stream_map_length[0] = 0x00;
	psm_tag->elementary_stream_map_length[1] = 0x04;
	
	
	
#endif
#ifdef AUDIO_VIDEO_STREAM
	psm_tag->program_stream_map_length[0] = 0x00;
	psm_tag->program_stream_map_length[1] = 0x12;
	
	psm_tag->elementary_stream_map_length[0] = 0x00;
	psm_tag->elementary_stream_map_length[1] = 0x08;
	
	psm_tag->stream_type_2 = 0x90;
	psm_tag->elementary_stream_id_2 = 0xC0;
	psm_tag->elementary_stream_info_length_2[0] = 0x00;
	psm_tag->elementary_stream_info_length_2[1] = 0x00;
#endif
	
	
	psm_tag->program_stream_map_version = 0x00;
	psm_tag->current_next_indicator = 0x01;
	psm_tag->reserved1 = 0x03;
	psm_tag->program_stream_map_version = 0x00;
	
	psm_tag->reserved2 = 0x7F;
	psm_tag->marker_bit = 0x01;
	
	psm_tag->program_stream_info_length[0] = 0x00;
	psm_tag->program_stream_info_length[1] = 0x00;
	
	psm_tag->stream_type_1 = 0x1B;
	psm_tag->elementary_stream_id_1 = 0xE0;
	psm_tag->elementary_stream_info_length_1[0] = 0x00;
	psm_tag->elementary_stream_info_length_1[1] = 0x00;
	
	psm_tag->CRC_32[3] = 0x45;
	psm_tag->CRC_32[2] = 0xBD;
	psm_tag->CRC_32[1] = 0xDC;
	psm_tag->CRC_32[0] = 0xF4;
}



int make_ps_packet_header(unsigned char *_pHeader,unsigned int _iFrameIndex,unsigned char Packet_Type , unsigned int fps)
{
	
    //struct PS_HEADER_tag ePSHeader;
	PS_HEADER_tag ePSHeader;
	float fInterval;
	UINT64 ui64SCR;
	ps_header_tag(&ePSHeader);
	
	if (NULL == _pHeader )
	{
		return -1;
	}
	
	//根据同步测试结果再做具体调整
//	if ( Packet_Type == VIDEO_PACK)
//	{
		fInterval = 1000/fps;  //fps:30   unit:ms
	//	fInterval = 16.6666666667; //fps:60
		//	float fInterval = 40;//fps:25
		ui64SCR = fInterval * _iFrameIndex * 90;


//	}
// 	else if ( Packet_Type == AUDIO_PACK)
// 	{
// 		fInterval = 181.8181818182;
// 		ui64SCR = fInterval * _iFrameIndex * 8;//////////////////////////////////////////////////////////////////////////
// 	}
	//ePSHeader.setSystem_clock_reference_base(ui64SCR);
	setSystem_clock_reference_base(&ePSHeader,ui64SCR);

	//ePSHeader.setProgram_mux_rate(10240);// code rate =4096Kbps 4096*1000/8/50
	setProgram_mux_rate(&ePSHeader,10240);

	memcpy(_pHeader,&ePSHeader,sizeof(PS_HEADER_tag));
	
	return sizeof(PS_HEADER_tag);
}




/* 
* make system header 
* input:
*        NULL;
* output: 
*        *pHeader:
* return the length of system packet header
*/

int make_sys_packet_header(unsigned char *pHeader)
{
	
	Ps_System_Header_tag SysHeader;
	//struct Ps_System_Header_tag SysHeader;
	ps_System_Header_tag(&SysHeader);
	//SysHeader.set_rate_bound(20480);
	set_rate_bound(&SysHeader,20480);
	SysHeader.audio_bound=01;// the num of audio streams.
	SysHeader.video_bound = 0x01; // the num of video streams.
	SysHeader.pstd_buf_bound_scale_1 = 0x01; // use 1024 Bytes as unit
	if (NULL == pHeader)
	{
		return -1;
	}

	//SysHeader.set_pstd_buf_size_bound_1(400);
	set_pstd_buf_size_bound_1(&SysHeader,400);
#ifdef AUDIO_VIDEO_STREAM
	//SysHeader.set_pstd_buf_size_bound_2(20);//128
	set_pstd_buf_size_bound_2(&SysHeader,20);//128


#endif

	memcpy(pHeader,&SysHeader,sizeof(Ps_System_Header_tag));

	return sizeof(Ps_System_Header_tag);

	

}


/* 
* make pes header
* input:
*        _iFrameIndex: the index of packing frame
*        _ui_data_length: the length of packing data
* output:
*        *_pHeader
* return the length of pes header
*/

int make_pes_packet_header(unsigned char *_pHeader,unsigned int _iFrameIndex,unsigned int _ui_data_length,unsigned char Packet_Type,unsigned int fps)
{
	
	PES_HEADER_tag ePESHeader;
	PTS_tag ePTS;
	float fInterval;
	UINT64 ui64SCR;
	//struct PES_HEADER_tag ePESHeader;
    pes_HEADER_tag(&ePESHeader);
//	unsigned int sDataLen = sizeof(PES_HEADER_tag) + sizeof(PTS_tag) - 6 + _ui_data_length; 
	
	//struct PTS_tag ePTS;
	
	pts_Tag(&ePTS);

	
	if(NULL == _pHeader)
	{
		return -1;
	}

	if (Packet_Type == VIDEO_PACK)
	{
		ePESHeader.stream_id =0xE0;
	} 
	else if(Packet_Type == AUDIO_PACK)
	{
		ePESHeader.stream_id = 0xC0;
	//	fInterval = 181.8181818182; //fps:60
	}
	else
	{
		return -1;
	}

	ePESHeader.PES_packet_length[0] = 0x00;
	ePESHeader.PES_packet_length[1] = 0x00;
	
	ePESHeader.original_or_copy = 0;
	ePESHeader.copyright = 0;
	ePESHeader.data_alignment_indicator = 1;
	ePESHeader.PES_priority = 0;
	ePESHeader.PES_scrambling_control = 0;
	
	ePESHeader.PES_extension_flag = 0;
	ePESHeader.PES_CRC_flag = 0;
	ePESHeader.additional_copy_info_flag = 0;
	ePESHeader.DSM_trick_mode_flag = 0;
	ePESHeader.ES_rate_flag = 0;
	ePESHeader.ESCR_flag = 0;
	ePESHeader.PTS_DTS_flags = 0x02;
	
	ePESHeader.PES_header_data_length = 0x05;
	
	memcpy(_pHeader,&ePESHeader,sizeof(PES_HEADER_tag));
	


	fInterval = 1000/fps;
	ui64SCR = fInterval * _iFrameIndex * 90;
	//ePTS.setPTS(ui64SCR);
	setPTS(&ePTS,ui64SCR);
	memcpy(_pHeader + sizeof(PES_HEADER_tag),&ePTS,sizeof(PTS_tag));
	
	return sizeof(PES_HEADER_tag) + sizeof(PTS_tag);
}

/*
* make_ps_packet
* input:
*        Nalu: NALU_t类型的结构体，如sps,pps I帧封成一个包，请在Nalu.buf写入参数集和I帧的数据。
*        Frame_type: 请参照宏定义,  SPS 0 (PPS 1 I_frame 2,暂不用，)  P_B_frame 3
*        Frame_Index：帧的序列号，请准确定义，关系到解码时间
*        Stop_Flag: 1 for the end.
* output:
*         pPacket: ps packet(header+data)
* return the length of the ps packet
*/
unsigned int make_ps_packet(unsigned char *pPacket,  NALU_t *Nalu,unsigned int Frame_Index,unsigned int Stop_Flag)
//unsigned int make_ps_packet(unsigned char *pPacket, struct NALU_t *Nalu,unsigned int Frame_Index,unsigned int Stop_Flag)
{
	PSM_tag Psm_Header;
	unsigned char Packet_Tpye;
	unsigned int Ps_Packet_Len = 0;
	int PS_Header_Len = 0;
	unsigned int Ps_sys_header_len = 0;
	unsigned char pheader[20] = {0}; //pack header buffer
	unsigned char Sys_Header[20] = {0};
	unsigned char Pes_Header[20] = {0}; // pes header buffer
	unsigned char Ps_End[4] = {0x00,0x00,0x01,0xB9};
	unsigned int Pes_Header_Len = 0;
	//struct PSM_tag Psm_Header;
	
    psm_Tag(&Psm_Header);


	

	if (Nalu->type == SPS)
	{
		Packet_Tpye = VIDEO_PACK;
		PS_Header_Len = make_ps_packet_header( pheader , Frame_Index , Packet_Tpye , Nalu->Fps);
		Ps_sys_header_len = make_sys_packet_header( Sys_Header);
		Pes_Header_Len = make_pes_packet_header( Pes_Header , Frame_Index , Nalu->len , Packet_Tpye , Nalu->Fps);	
		
		
		memcpy( pPacket , pheader , PS_Header_Len );
		memcpy( pPacket+PS_Header_Len , Sys_Header , Ps_sys_header_len );
		memcpy( pPacket+PS_Header_Len+Ps_sys_header_len , &Psm_Header , sizeof(PSM_tag) );
		memcpy( pPacket+PS_Header_Len+Ps_sys_header_len+sizeof(PSM_tag) , Pes_Header , Pes_Header_Len );
		memcpy( pPacket+PS_Header_Len +Ps_sys_header_len+sizeof(PSM_tag)+Pes_Header_Len , Nalu->buf,Nalu->len);

		Ps_Packet_Len = PS_Header_Len +Ps_sys_header_len+sizeof(PSM_tag)+Pes_Header_Len+(Nalu->len);


	} 
	else if (Nalu->type == P_B_frame )
	{
		Packet_Tpye = VIDEO_PACK;
		PS_Header_Len = make_ps_packet_header( pheader , Frame_Index , Packet_Tpye , Nalu->Fps);
		Pes_Header_Len = make_pes_packet_header( Pes_Header , Frame_Index , Nalu->len , Packet_Tpye , Nalu->Fps);

		
		memcpy( pPacket , pheader , PS_Header_Len );
		memcpy( pPacket+PS_Header_Len , Pes_Header , Pes_Header_Len );
		memcpy( pPacket+PS_Header_Len+Pes_Header_Len , Nalu->buf,Nalu->len);

		Ps_Packet_Len=PS_Header_Len+Pes_Header_Len+Nalu->len;

	}
	
	else if (Nalu->type == AUDIO)
	{

		Packet_Tpye = AUDIO_PACK;
		PS_Header_Len = make_ps_packet_header( pheader , Frame_Index ,Packet_Tpye,Nalu->Fps);
		Pes_Header_Len = make_pes_packet_header( Pes_Header , Frame_Index , Nalu->len , Packet_Tpye , Nalu->Fps);
		
		
		memcpy( pPacket , pheader , PS_Header_Len );
		memcpy( pPacket+PS_Header_Len , Pes_Header , Pes_Header_Len );
		memcpy( pPacket+PS_Header_Len+Pes_Header_Len , Nalu->buf,Nalu->len);
		
		Ps_Packet_Len=PS_Header_Len+Pes_Header_Len+Nalu->len;

	}
	else if (Nalu->type == First_Frame)
	{
		Packet_Tpye = AUDIO_PACK;
		PS_Header_Len = make_ps_packet_header( pheader , Frame_Index , Packet_Tpye , Nalu->Fps);
		Ps_sys_header_len = make_sys_packet_header( Sys_Header);
		Pes_Header_Len = make_pes_packet_header( Pes_Header , Frame_Index , Nalu->len , Packet_Tpye , Nalu->Fps);	
		
		
		memcpy( pPacket , pheader , PS_Header_Len );
		memcpy( pPacket+PS_Header_Len , Sys_Header , Ps_sys_header_len );
		memcpy( pPacket+PS_Header_Len+Ps_sys_header_len , &Psm_Header , sizeof(PSM_tag) );
		memcpy( pPacket+PS_Header_Len+Ps_sys_header_len+sizeof(PSM_tag) , Pes_Header , Pes_Header_Len );
		memcpy( pPacket+PS_Header_Len +Ps_sys_header_len+sizeof(PSM_tag)+Pes_Header_Len , Nalu->buf,Nalu->len);
		
		Ps_Packet_Len = PS_Header_Len +Ps_sys_header_len+sizeof(PSM_tag)+Pes_Header_Len+(Nalu->len);

	}
	else
	{
		printf( "undef frame\n" );
	}

	if ( 1 == Stop_Flag )
	{
		memcpy(pPacket+PS_Header_Len+Ps_Packet_Len , Ps_End,4);
		Ps_Packet_Len += Ps_Packet_Len+4;
	}
    return Ps_Packet_Len;
}	

void ps_package(FILE* fd,NALU_t nalu,char* pbuf,int length,unsigned int *Frame_Index)
{
    unsigned char Ps_Packet_Buf[2000000] = {0};//一个ps  包数据
    int Ps_Packet_Len = 0;//ps  打包长度
    int stop =0;
    BYTE buf[2000000];
    int len=length;
    int file_start_flag = 0;

    unsigned char AUD[7] ={0x00,0x00,0x00,0x01,0x09,0x70};
    int k=0;
    nalu.type= UNDEF;
    memcpy(nalu.buf,AUD,7);
    memcpy(buf,pbuf,len);
    
    //SPS、PPS、I帧默认为一组，找到此包
	if ((buf[0] == 0x00) && (buf[1] == 0x00) && (buf[2] == 0x00) && (buf[3] == 0x01) && ((buf[4]&0x0F) == 0x07))
	{
	     /*
	      nalu.type = SPS;
		nalu.Fps = 60;
		memcpy(nalu.buf+7,buf,len);
		nalu.len=len+7;
		Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);//根据帧类别区别封装
		write(fd,Ps_Packet_Buf,Ps_Packet_Len);
		*Frame_Index=*Frame_Index+1;
		//printf("SPS paksize=%d\n",len);
	     */
	      if(len<60000)
	      {
		nalu.type = SPS;
		nalu.Fps = 60;
		memcpy(nalu.buf+7,buf,len);
		nalu.len=len+7;
		Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);//根据帧类别区别封装
		fwrite(Ps_Packet_Buf,Ps_Packet_Len,1,fd);
		*Frame_Index=*Frame_Index+1;
		//printf("SPS paksize=%d\n",len);
	       }
		else
		{
		   	k=0;
			//printf("\nSPS paksize>60000,totalsize=%d\n",len);
			while (len>60000)
			{
				//printf("k=%d\n",k);
				nalu.type = SPS;
				nalu.Fps = 60;
				if (k==0)//帧前加AUD
				{
				     memcpy(nalu.buf+7,buf+k*60000,60000);
				     nalu.len=60007;
				}
				else
				{
					memcpy(nalu.buf,buf+k*60000,60000);
					nalu.len=60000;
				}
				len=len-60000;
				//printf("SPS paksize=%d\n",nalu.len);
				Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);
				fwrite(Ps_Packet_Buf,Ps_Packet_Len,1,fd);
				*Frame_Index=*Frame_Index+1;
				k++;
			}
			//printf("k=%d\n",k);
			nalu.type = SPS;
			nalu.Fps = 60;
			memcpy(nalu.buf,buf+k*60000,len);
			nalu.len=len;
			//printf("SPS paksize=%d\n",nalu.len);
			Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);
			fwrite(Ps_Packet_Buf,Ps_Packet_Len,1,fd);
			*Frame_Index=*Frame_Index+1;
		}
	}
	//找到P帧			
	else if ((buf[0] == 0x00) && (buf[1] == 0x00) && (buf[2] == 0x00) && (buf[3] == 0x01) && ((buf[4]&0x0F) == 0x01))
	{
	     /*
             nalu.type = P_B_frame;
		nalu.Fps = 60;
		memcpy(nalu.buf+7,buf,len);
		nalu.len=len+7;
		Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);//根据帧类别区别封装
		write(fd,Ps_Packet_Buf,Ps_Packet_Len);
		*Frame_Index=*Frame_Index+1;
		//printf("P_B_frame paksize=%d\n",len);*/
		if(len<60000)
	      {
	       nalu.type = P_B_frame;
		nalu.Fps = 60;
		memcpy(nalu.buf+7,buf,len);
		nalu.len=len+7;
		Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);//根据帧类别区别封装
		fwrite(Ps_Packet_Buf,Ps_Packet_Len,1,fd);
		*Frame_Index=*Frame_Index+1;
		//printf("P_B_frame paksize=%d\n",len);
	      	}
	      else
	      	{
	      	  k=0;
		 // printf("\nP_B_frame paksize>60000,totalsize=%d\n",len);
		    while (len>60000)
		    {
			//printf("k=%d\n",k);
			nalu.type = P_B_frame;
			nalu.Fps = 60;
                    if (k==0)//帧前加AUD
			{
				memcpy(nalu.buf+7,buf+k*60000,60000);
				nalu.len=60007;
			}
			else
			{
				memcpy(nalu.buf,buf+k*60000,60000);
				nalu.len=60000;
			}
			len=len-60000;
			//printf("P_B_frame paksize=%d\n",nalu.len);
			Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);
			fwrite(Ps_Packet_Buf,Ps_Packet_Len,1,fd);
			*Frame_Index=*Frame_Index+1;
			k++;
		    }
		//printf("k=%d\n",k);
		nalu.type = P_B_frame;
		nalu.Fps = 60;
		memcpy(nalu.buf,buf+k*60000,len);
		nalu.len=len;
		//printf("P_B_frame paksize=%d\n",nalu.len);
		Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);
		fwrite(Ps_Packet_Buf,Ps_Packet_Len,1,fd);
		*Frame_Index=*Frame_Index+1;
	      	}
	}
	else
	{
		nalu.type = AUDIO;
	       nalu.Fps = 60;
		if (file_start_flag == 0 )
		{
			file_start_flag = 1;
			nalu.type = First_Frame;
		}
		memcpy(nalu.buf,buf,len);
		nalu.len=len;
		Ps_Packet_Len = make_ps_packet(Ps_Packet_Buf,&nalu,*Frame_Index,stop);//根据帧类别区别封装
		fwrite(Ps_Packet_Buf,Ps_Packet_Len,1,fd);
		*Frame_Index=*Frame_Index+1;
		//printf("AUDIO paksize=%d\n",len);
	}
}

