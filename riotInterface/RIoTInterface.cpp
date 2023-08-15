/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
*/
#include <cstdlib>
#include <iostream>
#include <condition_variable>
#include <thread>
#include <cstring>
#include "rtConnection.h"
#include "rtLog.h"
#include "rtMessage.h"

#define MAX_BUF_SIZE 5000
#define MAX_CMD_SIZE 128
#define MAX_DEVICES 100
#define MAX_ID_LENGTH 20
#define MAX_DESC_LENGTH 50
#define MAX_CLASS_LENGTH 20
#define MAX_ITEMS 300
#define MAX_LENGTH 512

typedef struct {
    char id[MAX_ID_LENGTH];
    char description[MAX_DESC_LENGTH];
    char classs[MAX_CLASS_LENGTH];
} Device;

Device devices[MAX_DEVICES];

int numDevices;

int lightIndex = 0; // Index for the "light" category
int cameraIndex = 0; // Index for the "camera" category

int itemIndex =0;
using namespace std;
void onAvailableDevices(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);
void onDeviceProperties(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);
void onDeviceProperty(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);
void onSendCommand(rtMessageHeader const *hdr, uint8_t const *buff, uint32_t n, void *closure);

rtConnection con;
bool m_isActive = true;
std::condition_variable m_act_cv;
std::mutex m_lock;

int doesDeviceIDExist(Device devices[], int numDevices, char* deviceID) {
   int i;
    printf("Inside %s\n",__FUNCTION__);
        for (i = 0; i < numDevices; i++) {
        if (strcmp(devices[i].id, deviceID) == 0) {
            return 1; // Device ID found in the array
        }
    }
    return 0; // Device ID not found in the array
}

int _syscmd(char *cmd, char *retBuf, int retBufSize)
{
    FILE *f;
    char *ptr = retBuf;
    int bufSize=retBufSize, bufbytes=0, readbytes=0;

    if((f = popen(cmd, "r")) == NULL) {
        fprintf(stderr,"\npopen %s error\n", cmd);
        return -1;
    }

    while(!feof(f))
    {
        *ptr = 0;
                if(bufSize>=128) {
                bufbytes=128;
                } else {
                bufbytes=bufSize-1;
                }

        fgets(ptr,bufbytes,f);
                readbytes=strlen(ptr);

                if( readbytes== 0)
            break;

                bufSize-=readbytes;
        ptr += readbytes;
    }
    pclose(f);
   //ckp commeneted retBuf[retBufSize-1]=0;
   //ckp start
   // Remove newline character if present
    if (retBufSize > 0 && retBuf[retBufSize - 1] == '\n') {
        retBuf[retBufSize - 1] = '\0';
    }
   //ckp end
    return 0;
}
void removeTrailingSpaces(char* str) {
    int length = strlen(str);
    int i = length - 1;

    // Traverse from the end of the string to find the last non-space character
    while (i >= 0 && isspace(str[i])) {
        i--;
    }

    // Null-terminate the string at the last non-space character
    str[i + 1] = '\0';
}

void getuuidClasss(char *uid,char *output,int output_size)
{       
        char cmd[1000]={'\0'};
        char buf[2048]={'\0'};
        const char *str1 = "light";
        int ret;
        printf("Check if the UUID belongs to light or camera !! \n");
        //xhDeviceUtil --pd  944a0c25412c | awk -F': ' '/Class:/{print $3}'
        sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --pd  %s | awk -F': ' '/Class:/{print $3}'",uid);
        ret = _syscmd(cmd, buf, sizeof(buf));
        if ((ret != 0) && (strlen(buf) == 0))
           return -1;
         printf("write a code to check if the string is light or camera\n");
         
         
         removeTrailingSpaces(buf);
         
         
        snprintf(output, output_size, "%s", buf);


}

void registerForServices()
{
    rtConnection_AddListener(con, "getAvailableDevices", onAvailableDevices, con);
    rtConnection_AddListener(con, "getDeviceProperties", onDeviceProperties, con);
    rtConnection_AddListener(con, "getDeviceProperty", onDeviceProperty, con);
    rtConnection_AddListener(con, "sendCommand", onSendCommand, con);
}

int onAvailableDevices1(Device devices[]) {

    char buf[MAX_BUF_SIZE];
    char cmd[MAX_CMD_SIZE];
    int DeviceCount = 0,cStatus;

    printf("%s function \n",__FUNCTION__);
    cStatus = _syscmd("/opt/icontrol/bin/xhDeviceUtil --pa | grep -i class",buf,sizeof(buf));
    if (cStatus != 0)
    {
        printf("Error executing _syscmd \n");
        return -1;
    }
    // printf("data is %s\n",buf);

   /*Extract buffer data and copy the data to device structure*/
    char* token = strtok(buf, "\n");
    while (token != NULL && DeviceCount < MAX_DEVICES) {
        sscanf(token, "%16[^:]: %49[^,], Class: %19s",
               devices[DeviceCount].id, devices[DeviceCount].description, devices[DeviceCount].classs);
        DeviceCount++;
        token = strtok(NULL, "\n");
    }

    return DeviceCount;
}


void onAvailableDevices(rtMessageHeader const *rtHeader, uint8_t const *rtMsg, uint32_t rtMsgLength, void *userData)
{
    rtConnection con = (rtConnection)userData;
    rtMessage req;
    cout << "[onAvailableDevices]Received the message " << endl;
    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);
    rtMessage_ToString(req, (char **)&rtMsg, &rtMsgLength);
    rtLog_Info("req:%.*s", rtMsgLength, rtMsg);

    if (rtMessageHeader_IsRequest(rtHeader))
    {
        rtMessage res;
        rtMessage_Create(&res);

        rtMessage device;
        //ckp start
	numDevices = onAvailableDevices1(devices);
        if (numDevices < 0)
        {
                printf("No devices discovered\n");
                 return 1;
        }
        for (int i = 0; i < numDevices; i++)
        {
                char id_str[MAX_ID_LENGTH + 1];
                snprintf(id_str, sizeof(id_str), "%s", devices[i].id);
                rtMessage_Create(&device);
                printf("Device ID: %s\n", devices[i].id);
                // Set Device ID
                rtMessage_SetString(device, "id", id_str);

                printf("Device Name: %s\n", devices[i].description);
                // Set Device Name
                rtMessage_SetString(device, "name", devices[i].description);

                printf("Device Class: %s\n", devices[i].classs);
                // Set Device Class
#if 0
		if(strcmp(devices[i].classs,"camera") == 0)
		{
			printf("class is camera \n");
		        rtMessage_SetString(device, "class","0");
		}
		else
		{
			printf("class is light \n");
			rtMessage_SetString(device, "class","1");
		}
#endif
     		rtMessage_SetString(device, "class", devices[i].classs);
                printf("----------------------\n");
		rtMessage_AddMessage(res, "devices", device);
                
		rtMessage_Release(device);

        }

	//ckp end
//      	rtMessage_Create(&device);

  //      rtMessage_SetString(device, "name", "Philips");
    //    rtMessage_SetString(device, "uuid", "1234-PHIL-LIGHT-BULB");
      //  rtMessage_SetString(device, "devType", "1");
        //rtMessage_AddMessage(res, "devices", device);

        //char *output;
        //int outLen;

       // rtMessage_Create(&device);
       // rtMessage_SetString(device, "name", "Hewei-HDCAM-1234");
       // rtMessage_SetString(device, "devType", "0");

       // rtMessage_AddMessage(res, "devices", device);

        //rtMessage_ToString(res, &output, &outLen);
        //cout << "[onAvailableDevices]Returning the response " << output << endl;

        rtConnection_SendResponse(con, rtHeader, res, 1000);
        // rtMessage_Release(res);
    }
    else
    {
        cout << "[onAvailableDevices]Received  message not a request. Ignoring.." << endl;
    }
    // rtMessage_Release(req);
}
// Function to remove the device-specific prefix from the key
void removeDevicePrefix(char *uuid,char* key) {
    //char* prefix = "/000d6f000ef0a7a8/ep/1/r/";
    char prefix[1000];
   // printf("Entering %s\n",__FUNCTION__);
    sprintf(prefix, "/%s/ep/1/r/",uuid);
   // printf("prefix is %s\n",prefix);
    size_t prefixLen = strlen(prefix);
    if (strncmp(key, prefix, prefixLen) == 0) {
        memmove(key, key + prefixLen, strlen(key) - prefixLen + 1);
    }
}

void extractAfterLastSlash(const char *str, char *ps ,int size) {
    int lastSlashPos = -1;
    int strLength = strlen(str);
    //printf("Entering %s\n",__FUNCTION__);
    for (int i = 0; i < strLength; i++) {
        if (str[i] == '/') {
            lastSlashPos = i;
        }
    }

    if (lastSlashPos != -1 && lastSlashPos < strLength - 1) {
        //printf("Data after the last occurrence of / is  %s\n", &str[lastSlashPos + 1]);
       // snprintf(ps, size, , str);
        snprintf(ps, size, "%s", &str[lastSlashPos + 1]);
    } else {
        printf("No data found after the last occurrence of /.\n");
    }
}

// Function to tokenize the buffer based on newlines and equal signs
void tokenizeBuffer(char *uuid,char buffer[],char keyValue1Array[][512], int maxItems) {
    
    const char* delimiter = "\n";
    char* token;
    char buf1[5000]={'\0'};
    //int itemIndex = 0;
    printf("ckp1 \n");
    // Reset keyValue1Array
    for (int i = 0; i < maxItems; i++) {
        memset(keyValue1Array[i], 0, sizeof(keyValue1Array[i]));
    }
    char* bufferCopy = strdup(buffer); // Create a copy of the buffer as strtok modifies the original string
    printf("start if %s\n",__FUNCTION__);
    token = strtok(bufferCopy, delimiter);
    while (token != NULL && itemIndex < maxItems) {
    //while (token != NULL && (lightIndex < maxItems || cameraIndex < maxItems)) {
        // Find the equal sign in the token
        char* equalSign = strchr(token, '=');
        if (equalSign != NULL) {
            // Extract the key and value parts
            char* key = token;
            *equalSign = '\0'; // Replace the equal sign with a null terminator to separate the key and value
            char* value = equalSign + 1;

            // Create the key-value string in the format "key = value"
            char keyValue[256]; // Assuming the maximum length of key-value is 255 characters
             char keyValue1[512]; // Assuming the maximum length of key-value is 255 characters
            snprintf(keyValue, sizeof(keyValue), "%s = %s", key, value);
            extractAfterLastSlash(keyValue,keyValue1,sizeof(keyValue1));
            printf("%s = %s\n", key, value);
            //printf("------\n");
            printf("keyval is %s\n", keyValue1);
	//     getuuidClasss(uuid,buf1,sizeof(buf1));
          // if(strcmp(buf1,"light") == 0 && lightIndex < maxItems)
          // {
	//	   strcpy(keyValue1Array[lightIndex], keyValue1);
          //         removeDevicePrefix(uuid,key);
	//	   lightIndex++;
	  // }else
	  // {
	//	   strcpy(keyValue1Array[cameraIndex], keyValue1);
          //         removeDevicePrefix(uuid,key);
            //        cameraIndex++;
	  // }
	    strcpy(keyValue1Array[itemIndex], keyValue1);
            // Use rtMessage_AddString to add the key-value pair to the rtMessage
	    //rtMessage_AddString(res, "properties", "Prop1in=Josekutty");
	    //rtMessage_SetString(props, "properties", keyValue1);
            //printf("----------------------\n");
            //rtMessage_AddMessage(res, "properties", props);

              //  rtMessage_Release(props);
            //rtMessage_AddString(props, "properties", keyValue1);

            // Remove the device-specific prefix from the key, if present
            removeDevicePrefix(uuid,key);
	     itemIndex++;
        }

        token = strtok(NULL, delimiter);
    }

    free(bufferCopy); // Free the allocated buffer copy
}



void onDeviceProperties1(char *param, char *output, int output_size)
{
    char cmd[MAX_CMD_SIZE];
    char buf[MAX_BUF_SIZE];
    int ret = 0;

    sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --pd %s",param);
    ret = _syscmd(cmd, buf, sizeof(buf));
    if ((ret != 0) && (strlen(buf) == 0))
        return -1;
    snprintf(output, output_size, "%s", buf);

    return 0;
}

void onDeviceProperties(rtMessageHeader const *rtHeader, uint8_t const *rtMsg, uint32_t rtMsgLength, void *userData)
{
    char buf[5000]={'\0'};
    char buf1[5000]={'\0'};
    char keyValue1Array[MAX_ITEMS][512];
    memset(keyValue1Array, 0, sizeof(keyValue1Array));
    rtConnection con = (rtConnection)userData;
    rtMessage req;
    cout << "[onDeviceProperties]Received  message .." << endl;

    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);

    if (rtMessageHeader_IsRequest(rtHeader))
    {
        rtMessage res;
        rtMessage_Create(&res);

        char *uuid;
        rtMessage_GetString(req, "deviceId", &uuid);

        cout << "Device identifier is " << uuid << endl;
        //rtMessage props;
        //rtMessage_Create(&props);

     	//ckp start
	onDeviceProperties1(uuid,buf,sizeof(buf));
	printf("buf is %s\n",buf);
        getuuidClasss(uuid,buf1,sizeof(buf1));
       printf("before \n");
       	//if(strcmp(buf1,"light") == 0)
       // {

                tokenizeBuffer(uuid,buf,keyValue1Array, MAX_ITEMS);
		//for (int i = 0; i < lightIndex; i++) {
		for (int i = 0; i < itemIndex; i++) {
                   printf("keyValue1[%d]: %s\n", i, keyValue1Array[i]);
		   rtMessage_AddString(res, "properties", keyValue1Array[i]);
		   //rtMessage_SetString(res, "properties", keyValue1Array[i]);
               }
	//	memset(keyValue1Array, 0, sizeof(keyValue1Array));
		printf("before respo \n");
               int ret = rtConnection_SendResponse(con, rtHeader, res, 2000);
               printf("Response dd is %d -- \n",ret);
	       itemIndex=0;
       // }
       // else
        //{
          //      tokenizeBuffer(uuid,buf,keyValue1Array, MAX_ITEMS);
	//	for (int i = 0; i < cameraIndex; i++) {
          //         printf("keyValue2[%d]: %s\n", i, keyValue1Array[i]);
         //          rtMessage_AddString(res, "properties", keyValue1Array[i]);
            //    }
          //      memset(keyValue1Array, 0, sizeof(keyValue1Array));
       // rtConnection_SendResponse(con, rtHeader, res, 1000);
         //       printf("class is camera -- \n");
       // }

	//ckp end
#if 0
        rtMessage_AddString(res, "properties", "Prop1=Josekutty");
        rtMessage_AddString(res, "properties", "Prop2=Kottarathil");
        rtMessage_AddString(res, "properties", "Prop3=Comcast");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");
        rtMessage_AddString(res, "properties", "Prop4=Engineer");

#endif
	//rtConnection_SendResponse(con, rtHeader, res, 1000);
        rtMessage_Release(res);
//	printf("clear data1 \n");
//	for (int i = 0; i < MAX_ITEMS ; ++i) {
  //      memset(keyValue1Array[i], 0, MAX_LENGTH); // Reset the contents to 0
    //    }
    }
    else
    {
        cout << "[onDeviceProperties]Received  message not a request. Ignoring.." << endl;
    }
    rtMessage_Release(req);
}
void onDeviceProperty1(char *param,char *proName ,char *output, int output_size)
{
    char cmd[1000]={'\0'};
    char buf[2048]={'\0'};
    char bufx[3000]={'\0'};
    char bufw[3000]={'\0'};
    int ret = 0;
    char deviceClass[40];
    char *result;
    char *result1;
    char cls[20];
    const char *str1 = "light";
    printf("prop name is %s and UUID is %s and length of propname is %d\n",proName,param,strlen(proName));

    if (doesDeviceIDExist(devices,numDevices,param))
    {
        printf("Device ID: %s exists in the list.\n", param);
        /*Below code is required if we dont get a class name from UI start*/
        printf("Check if the UUID belongs to light or camera !! \n");
        getuuidClasss(param,bufw,sizeof(bufw));
        //xhDeviceUtil --pd  944a0c25412c | awk -F': ' '/Class:/{print $3}'

        if(strcmp(bufw,str1) == 0)
        {
             printf("light is the class\n");
             printf("Supported light params for UI to toggle are -- 1.isOn \n");
             printf("check if the light class consists of isOn \n");
             sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --pd %s",param);
             ret = _syscmd(cmd, buf, sizeof(buf));
             if ((ret != 0) && (strlen(buf) == 0))
              return -1;
            snprintf(output, output_size, "%s", buf);
            printf("PD buffer for light is %s\n",output);
            result = strstr(buf,proName);
if (result != NULL)
             {
                printf("light prop name exists in the buffer is %s\n",proName);
                printf("format is : /000d6f000ef0a7a8/ep/1/r/isOn \n");
                sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --rr /%s/ep/1/r/isOn",param);
                ret = _syscmd(cmd, buf, sizeof(buf));
                if ((ret != 0) && (strlen(buf) == 0))
                  return -1;
                snprintf(output, output_size, "%s", buf);
                printf("Light current state is %s\n",buf);
             } else 
             {
                printf("\"isOn\" does not exist in the buffer.\n");
             }
         }
         else
         {
             printf("camera is the class,copied userUserId temporarirly \n");
             //strcpy(proName,"userUserId");
             printf("Supported camera params for UI to read only -- 1.userUserId 2.userPassword 3.videoInfo \n");
             sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --pd %s",param);
             ret = _syscmd(cmd, bufx, sizeof(bufx));
             if ((ret != 0) && (strlen(bufx) == 0))
               return -1;
             snprintf(output, output_size, "%s", bufx);
             printf("PD buffer for camera is %s\n",output);
             printf("---check if the camera class consists of propertyname %s \n",proName);
             printf("--property format: /944a0c25412c/ep/camera/r/videoInfo proName %s\n",proName);
             result1 = strstr(bufx,proName);
             if (result1 != NULL)
             {
                     printf("camera prop name exists : %s\n",proName);
                    if(strcmp(proName,"videoInfo") == 0)
                    {
                      sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --rr /%s/ep/camera/r/%s | grep -o 'https://[^\"]*' | sed -n '1p'",param,proName);
                    }
                    else
                     {
                        sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --rr /%s/ep/camera/r/%s",param,proName);
                     }
                        ret = _syscmd(cmd, bufx, sizeof(bufx));
                     if ((ret != 0) && (strlen(bufx) == 0))
                     return -1;
                     snprintf(output, output_size, "%s", bufx);
                     printf("camera prop output  is %s\n",bufx);
  }
          }
          
         
    } else {
        printf("Device ID: %s does not exist in the list.\n", param);
    }        
#if 0 
    sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --pd %s",param);
    ret = _syscmd(cmd, buf, sizeof(buf));
    if ((ret != 0) && (strlen(buf) == 0))
        return -1;
    snprintf(output, output_size, "%s", buf);
#endif 
    return 0;
}   

void onDeviceProperty(rtMessageHeader const *rtHeader, uint8_t const *rtMsg, uint32_t rtMsgLength, void *userData)
{
    rtConnection con = (rtConnection)userData;
    rtMessage req;
    char buf2[2048] = {0};
    cout << "[onDeviceProperty]Received  message .." << endl;
    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);
    rtMessage_ToString(req, (char **)&rtMsg, &rtMsgLength);
    rtLog_Info("req:%.*s", rtMsgLength, rtMsg);

    if (rtMessageHeader_IsRequest(rtHeader))
    {
        rtMessage res;
        rtMessage_Create(&res);

        char *uuid, *property;
        rtMessage_GetString(req, "deviceId", &uuid);
        rtMessage_GetString(req, "property", &property);

        cout << "Device identifier is " << uuid << ", Property requested :" << property << endl;
        onDeviceProperty1(uuid,property,buf2,sizeof(buf2));
	printf("buf2x is %s\n",buf2);
//	rtMessage_SetString(res, "value1", "AhHooked.");
//	printf("buf21 is %s\n",buf2);
   //     rtConnection_SendResponse(con, rtHeader, res, 1000);
	rtMessage_SetString(res, "value", buf2);
        rtConnection_SendResponse(con, rtHeader, res, 1000);
        rtMessage_Release(res);
//	printf("buf22 is %s\n",buf2);
    }
    else
    {
        cout << "[onDeviceProperty]Received  message not a request. Ignoring.." << endl;
    }
    rtMessage_Release(req);
}
void onSendCommand(rtMessageHeader const *rtHeader, uint8_t const *rtMsg, uint32_t rtMsgLength, void *userData)
{
#if 0	
    rtConnection con = (rtConnection)userData;
    rtMessage req;

    cout << "[onSendCommand]Received  message .." << endl;
    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);

    if (rtMessageHeader_IsRequest(rtHeader))
    {
        rtMessage res;
        rtMessage_Create(&res);

        char *uuid, *property;
        rtMessage_GetString(req, "uuid", &uuid);
        rtMessage_GetString(req, "command", &property);

        cout << "Device identifier is " << uuid << ", command requested :" << property << endl;
        //free(uuid);
        //free(property);

        rtMessage_SetInt32(res, "result", 1);
        rtConnection_SendResponse(con, rtHeader, res, 1000);
        rtMessage_Release(res);
    }
    else
    {
        cout << "[onSendCommand]Received  message not a request. Ignoring.." << endl;
    }
    rtMessage_Release(req);
#endif

    rtConnection con = (rtConnection)userData;
    rtMessage req;
    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);
    printf("Entering %s\n",__FUNCTION__);
    if (rtMessageHeader_IsRequest(rtHeader))
    {

        char cmd[1000]={'\0'};
        char buf[1024]={'\0'};
        char bufa[2048]={'\0'};
        char bufb[2048]={'\0'};
        char pProperty[20]={'\0'};
        int ret;
        rtMessage res;
        rtMessage_Create(&res);

        char *uuid, *property;
        rtMessage_GetString(req, "deviceId", &uuid);
        rtMessage_GetString(req, "command", &property);

        printf("Entering11 %s\n",__FUNCTION__);
        printf("command requested: %s\n",property);
        printf("Device identifier is %s\n", uuid);
        printf("Entering12 %s\n",__FUNCTION__);
     	if (strcmp(property, "on=true") == 0) {
		strcpy(pProperty,"true");
        } 
	else
	{	
		strcpy(pProperty,"false");
        }
        //ckp start
        printf("command requested: %s\n",pProperty);
        printf("Read the existing state of bulb .. \n");
        onDeviceProperty1(uuid,"isOn",bufa,sizeof(bufa));
        printf("bulb exis state is %s\n",bufa);
        if(strcmp(bufa,pProperty) ==0)
        {
            printf("state is same \n");
            return 1;
        }
        else
        {
  sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --wr /%s/ep/1/r/isOn %s",uuid,pProperty);
                printf("command is %s\n",cmd);
                ret = _syscmd(cmd, buf, sizeof(buf));
                if ((ret != 0) && (strlen(buf) == 0))
                         return -1;

                // snprintf(output, output_size, "%s", buf);
                onDeviceProperty1(uuid,"isOn",bufb,sizeof(bufb));
                    printf("bulb new  state is %s\n",bufb);
                 if(strcmp(bufb,bufa) == 0)
                 {
                       printf("state did not change \n");
                       rtMessage_SetInt32(res, "result", 0);
                        rtConnection_SendResponse(con, rtHeader, res, 1000);
                        rtMessage_Release(res);
                        //return -1;
                 }
                else
                 {
                        printf("state change happened \n");
                        rtMessage_SetInt32(res, "result", 1);
                        rtConnection_SendResponse(con, rtHeader, res, 1000);
                        rtMessage_Release(res);
                        //return 1;
                 }

        }               //ckp end
    }
    rtMessage_Release(req);

}

void handleTermSignal(int _signal)
{
    cout << "Exiting from app.." << endl;

    unique_lock<std::mutex> ulock(m_lock);
    m_isActive = false;
    m_act_cv.notify_one();
}
void waitForTermSignal()
{
    cout << "Waiting for term signal.. " << endl;
    thread termThread([&]()
                      {
    while (m_isActive)
    {
        unique_lock<std::mutex> ulock(m_lock);
        m_act_cv.wait(ulock);
    }
    
    cout<<"[SmartMonitor::waitForTermSignal] Received term signal."<<endl; });
    termThread.join();
}
int main(int argc, char const *argv[])
{
    rtLog_SetLevel(RT_LOG_DEBUG);
    cout << "RIoT Sample Daemon 1.0" << endl;
    rtConnection con;
    cout<<" Usage is "<<argv[0]<<" tcp://<<ipaddress:port"<<endl;
    rtConnection_Create(&con, "IOTGateway", argc == 1 ? "tcp://127.0.0.1:10001" : argv[1]);
    rtConnection_AddListener(con, "GetAvailableDevices", onAvailableDevices, con);
    rtConnection_AddListener(con, "GetDeviceProperties", onDeviceProperties, con);
    rtConnection_AddListener(con, "GetDeviceProperty", onDeviceProperty, con);
    rtConnection_AddListener(con, "SendCommand", onSendCommand, con);
    waitForTermSignal();
    rtConnection_Destroy(con);
    return 0;
}
