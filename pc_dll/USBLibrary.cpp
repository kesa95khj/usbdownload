// USBLibrairie.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "USBLibrary.h"
#include <stdio.h>
#include <stdlib.h>


#include "GUID829.h"
LPGUID pGuid = (LPGUID) &GUID_CLASS_I82930_BULK;


//*----------------------------------------------------------------------------
//* \fn    DllMain
//* \brief 
//*----------------------------------------------------------------------------
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

//*----------------------------------------------------------------------------
//* \fn    CFCPipeUSB::CFCPipeUSB
//* \brief CFCPipeUSB class constructor
//*----------------------------------------------------------------------------
CFCPipeUSB::CFCPipeUSB()
{
  m_hPipeIn = INVALID_HANDLE_VALUE;
  m_hPipeOut = INVALID_HANDLE_VALUE;
}

//*----------------------------------------------------------------------------
//* \fn    CFCPipeUSB::Open
//* \brief Open handel to device
//*----------------------------------------------------------------------------
short CFCPipeUSB::Open(char* sDeviceName)
{
  char sPipeNameIn[MAX_PATH];
  char sPipeNameOut[MAX_PATH];

  ::strcpy(sPipeNameIn, sDeviceName);
  ::strcat(sPipeNameIn, "\\PIPE01");

  ::strcpy(sPipeNameOut, sDeviceName);
  ::strcat(sPipeNameOut, "\\PIPE00");

  m_hPipeIn = ::CreateFile(sPipeNameIn, 
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ,
						   NULL,
						   OPEN_EXISTING, 
                           0, 
						   NULL);

  m_hPipeOut = ::CreateFile(sPipeNameOut, 
                            GENERIC_WRITE,
                            FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING, 
                            0,
							NULL);


  if(    (m_hPipeIn == INVALID_HANDLE_VALUE)
      || (m_hPipeOut == INVALID_HANDLE_VALUE))
  {
    Close();
	int i =GetLastError();
    return i;
  }

  return FC_OK;
}

//*----------------------------------------------------------------------------
//* \fn    CFCPipeUSB::Close
//* \brief Close handel
//*----------------------------------------------------------------------------
short CFCPipeUSB::Close()
{
  if(m_hPipeIn != INVALID_HANDLE_VALUE)
    ::CloseHandle(m_hPipeIn);
  if(m_hPipeOut != INVALID_HANDLE_VALUE)
    ::CloseHandle(m_hPipeOut);

  m_hPipeIn = INVALID_HANDLE_VALUE;
  m_hPipeOut = INVALID_HANDLE_VALUE;;

  return FC_OK;
}

//*----------------------------------------------------------------------------
//* \fn    CFCPipeUSB::ReadPipe
//* \brief Read data
//*----------------------------------------------------------------------------
short CFCPipeUSB::ReadPipe(LPVOID pBuffer, ULONG ulBufferSize)
{
  int timeout = 5;
  if(m_hPipeIn == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  if(!pBuffer || !ulBufferSize)
    return FC_NOT_INITIALIZED;

  ULONG dwBytesRead;
  ULONG dwBytesToRead = ulBufferSize;

  do
  {
    if( ! ::ReadFile(m_hPipeIn, pBuffer, dwBytesToRead, &dwBytesRead, NULL)) {
      DWORD dwErr = ::GetLastError();
      return FC_DRIVER_ERROR;
    }
	pBuffer =  (char *) pBuffer + dwBytesRead;
    dwBytesToRead -= dwBytesRead;
  } while(dwBytesToRead && (--timeout));

  
	if ((!timeout) || dwBytesToRead)
		return FC_DRIVER_ERROR;

  return FC_OK;
}

//*----------------------------------------------------------------------------
//* \fn    CFCPipeUSB::WritePipe
//* \brief Write data
//*----------------------------------------------------------------------------
short CFCPipeUSB::WritePipe(LPVOID pBuffer, ULONG ulBufferSize, ULONG *pBytesWritten)
{
  if(m_hPipeOut == INVALID_HANDLE_VALUE)
    return FC_ERROR;

  if(!pBuffer)
    return FC_NOT_INITIALIZED;

 
  if(! ::WriteFile(m_hPipeOut, pBuffer, ulBufferSize, pBytesWritten, NULL))
  {
    DWORD dwErr = ::GetLastError();
    return FC_DRIVER_ERROR;
  }

  return FC_OK;
}

//*----------------------------------------------------------------------------
//* \fn    GetDeviceName
//* \brief 
//*----------------------------------------------------------------------------
BOOL GetDeviceName (HDEVINFO HardwareDeviceInfo,
                                  PSP_INTERFACE_DEVICE_DATA DeviceInfoData,
                                  char **pDevName)
{
  PSP_INTERFACE_DEVICE_DETAIL_DATA functionClassDeviceData = NULL;
  ULONG                            predictedLength = 0;
  ULONG                            requiredLength = 0;

  HANDLE hOut = INVALID_HANDLE_VALUE;

  //
  // allocate a function class device data structure to receive the
  // goods about this particular device.
  //
  SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
                                  DeviceInfoData,
                                  NULL,  // probing so no output buffer yet
                                  0,     // probing so output buffer length of zero
                                  &requiredLength,
                                  NULL); // not interested in the specific dev-node

  predictedLength = requiredLength;

  functionClassDeviceData = (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc (predictedLength);
  functionClassDeviceData->cbSize = sizeof (SP_INTERFACE_DEVICE_DETAIL_DATA);

  //
  // Retrieve the information from Plug and Play.
  //
  if (! SetupDiGetInterfaceDeviceDetail(HardwareDeviceInfo,
                                        DeviceInfoData,
                                        functionClassDeviceData,
                                        predictedLength,
                                        &requiredLength,
                                        NULL))
  {
    free(functionClassDeviceData);
    return FALSE;
  }

  *pDevName = strdup(functionClassDeviceData->DevicePath) ;

  free(functionClassDeviceData);
	
  return TRUE;
}

//*----------------------------------------------------------------------------
//* \fn    GetUsbDeviceListName
//* \brief Make list of device Name connected for current GUID
//*----------------------------------------------------------------------------
USBLIBRARY_API int GetUsbDeviceListName(char** pDeviceList[])
{

  ULONG                    NumberDevices;
  HDEVINFO                 hardwareDeviceInfo;
  SP_INTERFACE_DEVICE_DATA deviceInfoData;
   
   // Open a handle to the plug and play dev node.
  // SetupDiGetClassDevs() returns a device information set that contains info on all
  // installed devices of a specified class.
  //
  hardwareDeviceInfo = SetupDiGetClassDevs (
                         pGuid,
                         NULL,            // Define no enumerator (global)
                         NULL,            // Define no
                         (DIGCF_PRESENT | // Only Devices present
                         DIGCF_INTERFACEDEVICE)); // Function class devices.
  NumberDevices = 0;
   deviceInfoData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);
  *pDeviceList = (char **) malloc (sizeof(char *) * 128);

  do 
  {
      

      // SetupDiEnumDeviceInterfaces() returns information about device interfaces
      // exposed by one or more devices. Each call returns information about one interface;
      // the routine can be called repeatedly to get information about several interfaces
      // exposed by one or more devices.
      if(SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                      0, // We don't care about specific PDOs
                                      pGuid,
                                      NumberDevices,
                                      &deviceInfoData)) {
		GetDeviceName(hardwareDeviceInfo,&deviceInfoData,&((*pDeviceList)[NumberDevices++]));
      } 
   } while (ERROR_NO_MORE_ITEMS != GetLastError()) ;



  // SetupDiDestroyDeviceInfoList() destroys a device information set
  // and frees all associated memory.
  SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

  return NumberDevices;

}


