/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2011 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

#ifndef _DISPLAY_PROCESS_H_
#define _DISPLAY_PROCESS_H_

#ifdef TI814X_DVR
#define DISABLE_DISPLAY_PROCESS_THREAD
#endif

struct video_timings {
	/* Unit: pixels */
	UInt16 x_res;
	/* Unit: pixels */
	UInt16 y_res;
	/* Unit: KHz */
	UInt32 pixel_clock;
	/* Unit: pixel clocks */
	UInt16 hsw;	/* Horizontal synchronization pulse width */
	/* Unit: pixel clocks */
	UInt16 hfp;	/* Horizontal front porch */
	/* Unit: pixel clocks */
	UInt16 hbp;	/* Horizontal back porch */
	/* Unit: line clocks */
	UInt16 vsw;	/* Vertical synchronization pulse width */
	/* Unit: line clocks */
	UInt16 vfp;	/* Vertical front porch */
	/* Unit: line clocks */
	UInt16 vbp;	/* Vertical back porch */
};

enum extension_edid_db {
	DATABLOCK_AUDIO	= 1,
	DATABLOCK_VIDEO	= 2,
	DATABLOCK_VENDOR = 3,
	DATABLOCK_SPEAKERS = 4,
};

struct img_edid {
	UInt8 pref;
	UInt32 code;
};

struct image_format {
	UInt32 length;
	struct img_edid fmt[20];
};

struct audio_edid {
	UInt32 num_of_ch;
	UInt32 format;
};

struct audio_format {
	UInt32 length;
	struct audio_edid fmt[20];
};

struct latency {
	/* vid: if indicated, value=1+ms/2 with a max of 251 meaning 500ms */
	UInt32 vid_latency;

	UInt32 aud_latency;
	UInt32 int_vid_latency;
	UInt32 int_aud_latency;
};

struct deep_color {
	UInt8 bit_30;
	UInt8 bit_36;
	UInt32 max_tmds_freq;
};

/*  Video Descriptor Block  */
struct HDMI_EDID_DTD_VIDEO {
	UInt16	pixel_clock;		/* 54-55 */
	UInt8	horiz_active;		/* 56 */
	UInt8	horiz_blanking;		/* 57 */
	UInt8	horiz_high;		/* 58 */
	UInt8	vert_active;		/* 59 */
	UInt8	vert_blanking;		/* 60 */
	UInt8	vert_high;		/* 61 */
	UInt8	horiz_sync_offset;	/* 62 */
	UInt8	horiz_sync_pulse;	/* 63 */
	UInt8	vert_sync_pulse;	/* 64 */
	UInt8	sync_pulse_high;	/* 65 */
	UInt8	horiz_image_size;	/* 66 */
	UInt8	vert_image_size;	/* 67 */
	UInt8	image_size_high;	/* 68 */
	UInt8	horiz_border;		/* 69 */
	UInt8	vert_border;		/* 70 */
	UInt8	misc_settings;		/* 71 */
};

/*	Monitor Limits Descriptor Block	*/
struct HDMI_EDID_DTD_MONITOR {
	UInt16	pixel_clock;		/* 54-55*/
	UInt8	_reserved1;		/* 56 */
	UInt8	block_type;		/* 57 */
	UInt8	_reserved2;		/* 58 */
	UInt8	min_vert_freq;		/* 59 */
	UInt8	max_vert_freq;		/* 60 */
	UInt8	min_horiz_freq;		/* 61 */
	UInt8	max_horiz_freq;		/* 62 */
	UInt8	pixel_clock_mhz;	/* 63 */
	UInt8	GTF[2];			/* 64 -65 */
	UInt8	start_horiz_freq;	/* 66	*/
	UInt8	C;			/* 67 */
	UInt8	M[2];			/* 68-69 */
	UInt8	K;			/* 70 */
	UInt8	J;			/* 71 */

} __attribute__ ((packed));

/* Text Descriptor Block */
struct HDMI_EDID_DTD_TEXT {
	UInt16	pixel_clock;		/* 54-55 */
	UInt8	_reserved1;		/* 56 */
	UInt8	block_type;		/* 57 */
	UInt8	_reserved2;		/* 58 */
	UInt8	text[13];		/* 59-71 */
} __attribute__ ((packed));

struct HDMI_EDID {
	UInt8   header[8];		/* 00-07 */
	UInt16	manufacturerID;		/* 08-09 */
	UInt16	product_id;		/* 10-11 */
	UInt32	serial_number;		/* 12-15 */
	UInt8	week_manufactured;	/* 16 */
	UInt8	year_manufactured;	/* 17 */
	UInt8	edid_version;		/* 18 */
	UInt8	edid_revision;		/* 19 */
	UInt8	video_in_definition;	/* 20 */
	UInt8	max_horiz_image_size;	/* 21 */
	UInt8	max_vert_image_size;	/* 22 */
	UInt8	display_gamma;		/* 23 */
	UInt8	power_features;		/* 24 */
	UInt8	chroma_info[10];	/* 25-34 */
	UInt8	timing_1;		/* 35 */
	UInt8	timing_2;		/* 36 */
	UInt8	timing_3;		/* 37 */
	UInt8	std_timings[16];	/* 38-53 */

	struct HDMI_EDID_DTD_VIDEO	video[4]; /* 54-125 */

	UInt8	extension_edid;		/* 126 */
	UInt8	checksum;		/* 127 */
	UInt8	extension_tag;		/* 00 (extensions follow EDID) */
	UInt8	extention_rev;		/* 01 */
	UInt8	offset_dtd;		/* 02 */
	UInt8	num_dtd;		/* 03 */

	UInt8	data_block[123];	/* 04 - 126 */
	UInt8	extension_checksum;	/* 127 */

	UInt8	ext_datablock[256];
} __attribute__ ((packed));





void display_process_init();

void display_process_deinit();

#endif /*   _DISPLAY_PROCESS_H_ */

