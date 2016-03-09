// BasicUSB_6124.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <process.h>
#include "USBLibrary.h"

typedef struct _DEVICE {
	char *name;
	unsigned int number;
	unsigned int txrate;
	struct _DEVICE *next;
	char active;
} *PDEVICE, DEVICE;


#define MAX_MSG	1000
unsigned char txMsg[MAX_MSG];
unsigned char rxMsg[MAX_MSG];

int main(int argc, char* argv[])
{
	unsigned int device_cnt;
	char ** strConnectedDevices;
	PDEVICE ptDevice;

	printf("==========================================================\n");
	printf("   Chestnut USB Download Program\n");
	printf("==========================================================\n");

	device_cnt = GetUsbDeviceListName(&strConnectedDevices);
	
	if(nbConnectedDevices == 0)
		return 0;
	
	ptDevice = (PDEVICE) malloc(sizeof(DEVICE));
	ptDevice->name = _strdup(strConnectedDevices[0]);
	ptDevice->txrate = 0;
	
	// open the device
	if (pipe.Open(ptDevice->name))
		printf("-E- Can not open device %d\n%s\n", ptDevice->number, ptDevice->name);
	printf("-I- New device connected %d \n", pDevice->number); 
	
	while (1) {
		memset(txMsg, 0xaa, 300);
				
		printf(" pipeWritePipe \n");
		if (pipe.WritePipe(txMsg, msgLength, &msgWritten) || (msgLength != msgWritten)) {
			printf("-E- Write operation to device %d failed\n", pDevice->number);
			break;
		}
		printf(" pipeReadPipe \n");
		if (pipe.ReadPipe (rxMsg, msgLength)) {
			printf("-E- Read operation to device %d failed\n", pDevice->number);
			break;
		}
		
		printf(" Read / Write Complete \n");
		if (memcmp(txMsg, rxMsg, msgLength))
			printf("-E- Device %d Message received & transmitted are different\n");

		WaitForSingleObject( threadMutex, INFINITE );
		pDevice->txrate += msgLength;

		Sleep(1000L);
	}

	// close handle
	pipe.Close();
	free(ptDevice);
	
	return 1;
}

