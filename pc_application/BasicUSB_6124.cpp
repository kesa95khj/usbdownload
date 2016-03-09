#include <windows.h>
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>
#include "USBLibrary.h"
#include "comm_idd.h"

static const unsigned short crc16_tab[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

typedef struct {
	char *device_name;

	FILE *file;
	char file_name[64];
	unsigned long file_size;
	
	unsigned int frame_count;
	unsigned int frame_length;
	unsigned char *frame_buffer;

	unsigned char tx_buffer[1024];
	unsigned char rx_buffer[1024];
} t_download_info;

static int usb_download_start (t_download_info *info);
static int usb_download_end (t_download_info *info);
static int usb_download_file (t_download_info *info);

static unsigned short crc_ccitt_calcurate(unsigned char *buf, unsigned int len);

CFCPipeUSB g_usb_pipe;

int main(int argc, char* argv[])
{
	unsigned int device_cnt;
	char ** strConnectedDevices;

	t_download_info *download_info;

	printf("==========================================================\n");
	printf("   Chestnut USB Download Program\n");
	printf("==========================================================\n");

	if(argc <= 1) {
		printf(" Please write file name to download.\n");
		return 0;
	}

	device_cnt = GetUsbDeviceListName(&strConnectedDevices);
	if(device_cnt == 0) {
		printf(" Chestnut is not ready.\n");
		return 0;
	}
	
	download_info = (t_download_info*) malloc(sizeof(t_download_info));
	memset((char*)download_info, 0, sizeof(t_download_info));

	char *str = argv[1];
	unsigned int idx=0;
	unsigned int i=0;

	printf(" str = %s\n", str);

	
	for(;;) {	
		if(str[i] == 0) 
			break;

		if((str[i] == 0x5c) || (str[i] == '/'))
			idx = i+1;
		i++;
	}

	
	strcpy(download_info->file_name, str + idx);
	printf("File Name : %s\n", download_info->file_name);
	

	download_info->device_name = _strdup(strConnectedDevices[0]);
	download_info->frame_count = 0;
	download_info->file = fopen(argv[1], "rb");
	if(download_info->file == NULL) {
		printf(" Can not open file.\n");
		free(download_info);
		return 0;
	}

	fseek (download_info->file , 0 , SEEK_END); 
	download_info->file_size = ftell (download_info->file); 
	rewind (download_info->file); 

	if(download_info->file_size == 0) {
		printf(" File size is zero.\n");
		fclose(download_info->file);
		free(download_info);
		return 0;
	}

	// open the device
	if (g_usb_pipe.Open(download_info->device_name)) {
		printf("-Error : Can not open device\n%s\n",download_info->device_name);
		fclose(download_info->file);
		free(download_info);
		return 0;
	}
	
	////////////////////////////////////////////////////////////////////////////////

	if(usb_download_start(download_info)) {
		printf(" download start failed\n");
		goto EXIT;
	}

	////////////////////////////////////////////////////////////////////////////////

	download_info->frame_buffer = (unsigned char*)malloc(COMM_FRAME_SIZE);
	printf("file size = %lu\n", download_info->file_size);
	for(;;) {		
		download_info->frame_count++;
		download_info->frame_length = fread(download_info->frame_buffer, 1, COMM_FRAME_SIZE, download_info->file);

		//printf("%u - %u\n", download_info->frame_count, download_info->frame_length);
		printf(".");

		if(download_info->frame_length == 0)
			break;
		
		if(usb_download_file(download_info)) {
			printf(" download file failed\n");
			free(download_info->frame_buffer);
			goto EXIT;
		}
	}
	printf("\n");
	free(download_info->frame_buffer);

	////////////////////////////////////////////////////////////////////////////////

	if(usb_download_end(download_info)) {
		printf(" download end failed\n");
		goto EXIT;
	}

	////////////////////////////////////////////////////////////////////////////////

	// close handle
EXIT:
	g_usb_pipe.Close();
	fclose(download_info->file);
	free(download_info);
	
	return 1;
}


static int usb_download_start (t_download_info *info)
{
	unsigned short crc;
	unsigned long tx_cnt;
	t_comm_download_start_cmd *cmd = (t_comm_download_start_cmd*)malloc(sizeof(t_comm_download_start_cmd));

	memset((char*)cmd, 0, sizeof(t_comm_download_start_cmd));

	cmd->opcode = COMM_DOWNLOAD_START_CMD;
	
	cmd->length[0] = (sizeof(t_comm_download_start_cmd)-5)/256;
	cmd->length[1] = (sizeof(t_comm_download_start_cmd)-5)%256;

	cmd->file_size[0] = (unsigned char)((info->file_size>>24) & 0xff);
	cmd->file_size[1] = (unsigned char)((info->file_size>>16) & 0xff);
	cmd->file_size[2] = (unsigned char)((info->file_size>>8) & 0xff);
	cmd->file_size[3] = (unsigned char)(info->file_size & 0xff);

	sprintf((char*)cmd->file_name, info->file_name);
	sprintf((char*)cmd->date, "%s", __DATE__);
	sprintf((char*)cmd->time, "%s", __TIME__);
	

	crc = crc_ccitt_calcurate((unsigned char*)cmd, sizeof(t_comm_download_start_cmd)-2);
	cmd->crc[0] = (crc>>8) & 0xff;
	cmd->crc[1] = crc & 0xff;

	if(g_usb_pipe.WritePipe(cmd, sizeof(t_comm_download_start_cmd), &tx_cnt) || (tx_cnt != sizeof(t_comm_download_start_cmd))) {
		printf("-Error : Write operation to USB device failed\n");
		free(cmd);
		return -1;
	}
	free(cmd);

	t_comm_download_start_rsp *rsp = (t_comm_download_start_rsp*)info->rx_buffer;

	if (g_usb_pipe.ReadPipe (info->rx_buffer, sizeof(t_comm_download_start_rsp))) {
		printf("-Error : Read operation to USB device failed\n");
		return -2;
	}
	
	if((rsp->opcode != COMM_DOWNLOAD_START_RSP) || (rsp->ack != COMM_ACK)) {
		printf("-Error : Chestnut protocol failed\n");
		return -3;
	}
	
	unsigned short rx_crc = (rsp->crc[0]<<8) | rsp->crc[1];
	crc = crc_ccitt_calcurate((unsigned char*)rsp, sizeof(t_comm_download_start_rsp)-2);
	if(crc != rx_crc) {
		printf("-Error : CRC failed\n");
		return -4;
	}

	return 0;
}

static int usb_download_end (t_download_info *info)
{
	unsigned short crc;
	unsigned long tx_cnt;
	t_comm_download_end_cmd *cmd = (t_comm_download_end_cmd*)malloc(sizeof(t_comm_download_end_cmd));

	memset((char*)cmd, 0, sizeof(t_comm_download_end_cmd));

	cmd->opcode = COMM_DOWNLOAD_END_CMD;
	
	cmd->length[0] = (sizeof(t_comm_download_end_cmd)-5)/256;
	cmd->length[1] = (sizeof(t_comm_download_end_cmd)-5)%256;

	crc = crc_ccitt_calcurate((unsigned char*)cmd, sizeof(t_comm_download_end_cmd)-2);
	cmd->crc[0] = (crc>>8) & 0xff;
	cmd->crc[1] = crc & 0xff;

	//memcpy((char*)info->tx_buffer, (char*)cmd, sizeof(t_comm_download_end_cmd));

	if(g_usb_pipe.WritePipe(cmd, sizeof(t_comm_download_end_cmd), &tx_cnt) || (tx_cnt != sizeof(t_comm_download_end_cmd))) {
		printf("-Error : Write operation to USB device failed\n");
		free(cmd);
		return -1;
	}
	free(cmd);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	t_comm_download_end_rsp *rsp = (t_comm_download_end_rsp*)info->rx_buffer;

	if (g_usb_pipe.ReadPipe (info->rx_buffer, sizeof(t_comm_download_end_rsp))) {
		printf("-Error : Read operation to USB device failed\n");
		return -2;
	}
	
	if((rsp->opcode != COMM_DOWNLOAD_END_RSP) || (rsp->ack != COMM_ACK)) {
		printf("-Error : Chestnut protocol failed\n");
		return -3;
	}
	
	unsigned short rx_crc = (rsp->crc[0]<<8) | rsp->crc[1];
	crc = crc_ccitt_calcurate((unsigned char*)rsp, sizeof(t_comm_download_end_rsp)-2);
	
	if(crc != rx_crc) {
		printf("-Error : CRC failed\n");
		return -4;
	}

	return 0;	
}

static int usb_download_file (t_download_info *info)
{
	unsigned short crc;
	unsigned long tx_cnt;
	t_comm_download_file_cmd *cmd = (t_comm_download_file_cmd*)malloc(sizeof(t_comm_download_file_cmd));

	memset((char*)cmd, 0, sizeof(t_comm_download_file_cmd));

	cmd->opcode = COMM_DOWNLOAD_FILE_CMD;
	
	cmd->length[0] = (sizeof(t_comm_download_file_cmd)-5)/256;
	cmd->length[1] = (sizeof(t_comm_download_file_cmd)-5)%256;

	cmd->frame_num[0] = (info->frame_count>>8) & 0xff;
	cmd->frame_num[1] = info->frame_count & 0xff;

	cmd->frame_length[0] = (info->frame_length>>8) & 0xff;
	cmd->frame_length[1] = info->frame_length & 0xff;

	memcpy(cmd->frame, info->frame_buffer, info->frame_length);

	crc = crc_ccitt_calcurate((unsigned char*)cmd, sizeof(t_comm_download_file_cmd)-2);
	cmd->crc[0] = (crc>>8) & 0xff;
	cmd->crc[1] = crc & 0xff;

	if(g_usb_pipe.WritePipe(cmd, sizeof(t_comm_download_file_cmd), &tx_cnt) || (tx_cnt != sizeof(t_comm_download_file_cmd))) {
		printf("-Error : Write operation to USB device failed\n");
		free(cmd);
		return -1;
	}
	free(cmd);

	/////////////////////////////////////////////////////////////////////////////////////////////

	t_comm_download_file_rsp *rsp = (t_comm_download_file_rsp*)info->rx_buffer;

	if (g_usb_pipe.ReadPipe (info->rx_buffer, sizeof(t_comm_download_file_rsp))) {
		printf("-Error : Read operation to USB device failed\n");
		return -2;
	}
	
	if((rsp->opcode != COMM_DOWNLOAD_FILE_RSP) || (rsp->ack != COMM_ACK)) {
		printf("-Error : Chestnut protocol failed\n");
		return -3;
	}
	
	unsigned short rx_crc = (rsp->crc[0]<<8) | rsp->crc[1];
	crc = crc_ccitt_calcurate((unsigned char*)rsp, sizeof(t_comm_download_file_rsp)-2);
	
	if(crc != rx_crc) {
		printf("-Error : CRC failed\n");
		return -4;
	}

	return 0;	
}



static unsigned short crc_ccitt_calcurate(unsigned char *buf, unsigned int len)
{
	unsigned int i;
	unsigned short crc;
	
	crc = 0;
	for (i = 0;  i < len;  i++)  {
		crc = crc16_tab[((crc>>8) ^ *buf++) & 0xFF] ^ (crc << 8);
	}

	return crc;
}

