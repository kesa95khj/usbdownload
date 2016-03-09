#ifndef __COMM_IDD_H__
#define __COMM_IDD_H__

#define COMM_FRAME_SIZE				512

#define COMM_ACK					0x00
#define COMM_NACK					0x01

#define COMM_DOWNLOAD_START_CMD		0x10
#define COMM_DOWNLOAD_START_RSP		0x11

#define COMM_DOWNLOAD_END_CMD		0x12
#define COMM_DOWNLOAD_END_RSP		0x13

#define COMM_DOWNLOAD_FILE_CMD		0x14
#define COMM_DOWNLOAD_FILE_RSP		0x15


typedef struct {
	unsigned char opcode;
	unsigned char length[2];
	unsigned char file_size[4];
	unsigned char file_name[64];
	unsigned char date[16];
	unsigned char time[16];
	unsigned char crc[2];
} t_comm_download_start_cmd;

typedef struct {
	unsigned char opcode;
	unsigned char length[2];
	unsigned char ack;
	unsigned char crc[2];
} t_comm_download_start_rsp;

typedef struct {
	unsigned char opcode;
	unsigned char length[2];
	unsigned char crc[2];
} t_comm_download_end_cmd;

typedef struct {
	unsigned char opcode;
	unsigned char length[2];
	unsigned char ack;
	unsigned char crc[2];
} t_comm_download_end_rsp;

typedef struct {
	unsigned char opcode;
	unsigned char length[2];
	unsigned char frame_num[2];
	unsigned char frame_length[2];
	unsigned char frame[COMM_FRAME_SIZE];
	unsigned char crc[2];
} t_comm_download_file_cmd;

typedef struct {
	unsigned char opcode;
	unsigned char length[2];
	unsigned char frame_num[2];
	unsigned char ack;
	unsigned char crc[2];
} t_comm_download_file_rsp;




#endif

