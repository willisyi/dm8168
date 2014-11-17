/*
* writer:Kevin.liu
* Date:2010-08-06
*/
#ifndef __HEADER_PACK_HEADER_DEF_H__
#define __HEADER_PACK_HEADER_DEF_H__

//#include "NAL_H.H"


//#ifdef _WIN32
//typedef unsigned __int64 UINT64;
//#else
typedef long long UINT64;
//#endif
typedef unsigned char BYTE;
#include <fcntl.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>

#define  SPS 0
#define  PPS 1
#define  I_frame 2
#define  P_B_frame 3
#define  AUDIO 4
#define  First_Frame 5
#define  UNDEF 6

#define  VIDEO_PACK 0
#define  AUDIO_PACK 1


//#define AUDIO_STREAM 
//#define VIDEO_STREAM 
#define AUDIO_VIDEO_STREAM 
/*
typedef enum
{
	Frame_SPS = 0,
	Frame_PPS,
	Frame_I,
	Frame_P_B,
	Frame_AUDIO

} Frame_Type;

typedef enum
{
	Stream_AUDIO = 0,
	Stream_VIDEO,
	Stream_AUDIO_VIDEO
		
} Stream_Type;
*/


typedef struct NALU_t 
//struct NALU_t
{
	char StartCodePrefixLen;
	unsigned int len;
	char *buf;
	unsigned char type;
	unsigned int Fps;
//}*PNALU_t;
}NALU_t;
/*
* PS_HEADER_tag for ps header,14Bytes
*the Program Stream Pack Header contains a 14 bytes length
*/

typedef struct PS_HEADER_tag
//struct PS_HEADER_tag
{
	unsigned char pack_start_code[4];		//'0x000001BA'
	
	unsigned char system_clock_reference_base21:2;
	unsigned char marker_bit:1;
	unsigned char system_clock_reference_base1:3;
	unsigned char fix_bit:2;				//'01'
	
	unsigned char system_clock_reference_base22;
	
	unsigned char system_clock_reference_base31:2;
	unsigned char marker_bit1:1;
	unsigned char system_clock_reference_base23:5;
	
	unsigned char system_clock_reference_base32;
	
	unsigned char system_clock_reference_extension1:2;
	unsigned char marker_bit2:1;
	unsigned char system_clock_reference_base33:5;	//system_clock_reference_base 33bit
	
	unsigned char marker_bit3:1;
	unsigned char system_clock_reference_extension2:7; //system_clock_reference_extension 9bit
	
	unsigned char program_mux_rate1;// may vary from pack to pack
	
	unsigned char program_mux_rate2;
	
	unsigned char marker_bit5:1;
	unsigned char marker_bit4:1;
	unsigned char program_mux_rate3:6;
	
	unsigned char pack_stuffing_length:3;
	unsigned char reserved:5;
	
//};	
//}*pPS_HEADER_tag;
}PS_HEADER_tag;



/*
* ps system header
*/

typedef struct Ps_System_Header_tag
//struct Ps_System_Header_tag
{
	unsigned char system_header_start_head[4];
	unsigned char header_length[2];

	unsigned char rate_bound_1:7; // maximum value of the program_mux_rate
	unsigned char maker_bit_1:1; // always '1';

	unsigned char rate_bound_2;

	unsigned char maker_bit_2:1; // always set '1';
	unsigned char rate_bound_3:7;

	unsigned char CSPS_flag:1;
	unsigned char fixed_flag:1;
	unsigned char audio_bound:6;
	
	unsigned char video_bound:5;
	unsigned char maker_bit_3:1; // always set '1'
	unsigned char system_video_local_flag:1;
	unsigned char system_audio_local_flag:1;
	
	unsigned char reserved:7;// 7f
	unsigned char packet_rate_restriction_flag:1;

	unsigned char streamid_1;

	unsigned char pstd_buf_size_bound_11:5; // greater than or equal to the maximum P-STD input buffer size
	unsigned char pstd_buf_bound_scale_1:1;
	unsigned char res_1:2; //'11'

	unsigned char pstd_buf_size_bound_12;

#ifdef AUDIO_VIDEO_STREAM
	unsigned char streamid_2;

	unsigned char pstd_buf_size_bound_21:5;
	unsigned char pstd_buf_bound_scale_2:1;
	unsigned char res_2:2;
	
	unsigned char pstd_buf_size_bound_22;
#endif
	
//};
//}*pPs_System_Header_tag;
}Ps_System_Header_tag;



/*
*PES_HEADER_tag
*Program Stream PES HEADER
*
*/

typedef struct PES_HEADER_tag
//struct PES_HEADER_tag
{
	unsigned char	packet_start_code_prefix[3];
	unsigned char	stream_id;
	unsigned char	PES_packet_length[2];

	unsigned char	original_or_copy:1;
	unsigned char	copyright:1;
	unsigned char	data_alignment_indicator:1;
	unsigned char	PES_priority:1;
	unsigned char	PES_scrambling_control:2;
	unsigned char	fix_bit:2;

	unsigned char	PES_extension_flag:1;
	unsigned char	PES_CRC_flag:1;
	unsigned char	additional_copy_info_flag:1;
	unsigned char	DSM_trick_mode_flag:1; // trick mode control values
	unsigned char	ES_rate_flag:1;
	unsigned char	ESCR_flag:1;
	unsigned char	PTS_DTS_flags:2;

	unsigned char	PES_header_data_length;  // pes_extension_flag
//};
//}*pPES_HEADER_tag;
}PES_HEADER_tag;


/*
*struct: PTS_tag
*Presentation_time_stamp
*/

typedef struct PTS_tag
//struct PTS_tag
{
	unsigned char marker_bit:1;//表示只占1位
	unsigned char PTS1:3;//位域，位段
	unsigned char fix_bit:4;

	unsigned char PTS21;

	unsigned char marker_bit1:1;
	unsigned char PTS22:7;

	unsigned char PTS31; 

	unsigned char marker_bit2:1;
	unsigned char PTS32:7;
//};	
//}*pPTS_tag;
}PTS_tag;



/*
*struct:PSM_tag
*Program Stream Map
*/


typedef struct PSM_tag
//struct PSM_tag
{
	unsigned char packet_start_code_prefix[3];
	unsigned char map_stream_id;
	unsigned char program_stream_map_length[2];
	
	unsigned char program_stream_map_version:5;
	unsigned char reserved1:2;
	unsigned char current_next_indicator:1;

	unsigned char marker_bit:1;
	unsigned char reserved2:7;

	unsigned char program_stream_info_length[2];
	unsigned char elementary_stream_map_length[2];
	unsigned char stream_type_1;
	unsigned char elementary_stream_id_1; // the type of the elementary stream according to the following table
	unsigned char elementary_stream_info_length_1[2];

#ifdef AUDIO_VIDEO_STREAM
	unsigned char stream_type_2;
	unsigned char elementary_stream_id_2;
	unsigned char elementary_stream_info_length_2[2];
#endif

	unsigned char CRC_32[4];

//};	
//}*pPSM_tag;
	}PSM_tag;



void ps_header_tag(PS_HEADER_tag *PSHeader_tag);
void getSystem_clock_reference_base(PS_HEADER_tag *PSHeader_tag, UINT64 _ui64SCR);
void setSystem_clock_reference_base(PS_HEADER_tag *PSHeader_tag, UINT64 _ui64SCR);
void getProgram_mux_rate(PS_HEADER_tag *PSHeader_tag, unsigned int _uiMux_rate);
void setProgram_mux_rate(PS_HEADER_tag *PSHeader_tag, unsigned int _uiMux_rate);

void ps_System_Header_tag(Ps_System_Header_tag *System_Header_tag);
void get_rate_bound(Ps_System_Header_tag *System_Header_tag,unsigned int _ui_rate_bound);
void set_rate_bound(Ps_System_Header_tag *System_Header_tag,unsigned int _ui_rate_bound);
void set_pstd_buf_size_bound_1(Ps_System_Header_tag *System_Header_tag,unsigned int _ui_pstd_buf_size_bound);
void set_pstd_buf_size_bound_2(Ps_System_Header_tag *System_Header_tag,unsigned int _ui_pstd_buf_size_bound);


void pes_HEADER_tag(PES_HEADER_tag *pes_header_tag);

void pts_Tag(PTS_tag *pts_tag);
void getPTS(PTS_tag *pts_tag,UINT64 _ui64PTS);
void setPTS(PTS_tag *pts_tag,UINT64 _ui64PTS);

void psm_Tag(PSM_tag *psm_tag);


int make_ps_packet_header(unsigned char *_pHeader,unsigned int _iFrameIndex,unsigned char Packet_Type , unsigned int fps);
int make_pes_packet_header(unsigned char *_pHeader,unsigned int _iFrameIndex,unsigned int _ui_data_length,unsigned char Packet_Type,unsigned int fps);
int make_sys_packet_header(unsigned char *pHeader);
unsigned int make_ps_packet(unsigned char *pPacket, NALU_t *Nalu,unsigned int Frame_Index,unsigned int Stop_Flag);
void ps_package(FILE* fd,NALU_t nalu,char* pbuf,int length,unsigned int *Frame_Index);

#endif /*__HEADER_PACK_HEADER_DEF_H__*/
