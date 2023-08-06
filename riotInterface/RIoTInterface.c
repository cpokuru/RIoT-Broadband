/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtConnection.h"
#include "rtLog.h"
#include "rtMessage.h"

#define MAX_DEVICES 100
#define MAX_ID_LENGTH 20
#define MAX_DESC_LENGTH 50
#define MAX_CLASS_LENGTH 20

#define MAX_BUF_SIZE 5000
#define MAX_CMD_SIZE 128

#if 0
typedef struct {
    char deviceID[MAX_LINE_LENGTH];
    char deviceName[MAX_LINE_LENGTH];
    char deviceClass[MAX_CLASS_LENGTH];
} Device;
#endif


typedef struct {
    char id[MAX_ID_LENGTH];
    char description[MAX_DESC_LENGTH];
    char class[MAX_CLASS_LENGTH];
} Device;


typedef struct {
    char id[17];
    char name[50];
    char deviceClass[20];
    char userPassword[100];
    char userUserId[100];
    char ipAddress[100];
    char macAddress[100];
    int portNumber;
} rDevice;


Device devices[MAX_DEVICES];

int numDevices;

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


// Assume the definitions for rtConnection, rtMessageHeader,
// and rtMessage functions are available in the C code.
//CKP ready for test
void onDeviceProperty1(rtMessageHeader const *rtHeader, uint8_t const *rtMsg, uint32_t rtMsgLength, void *userData)
{
    char buf2[2048] = {0};	
    rtConnection con = (rtConnection)userData;
    rtMessage req;
    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);

    if (rtMessageHeader_IsRequest(rtHeader))
    {
        rtMessage res;
        rtMessage_Create(&res);

        const char *uuid, *property;
        rtMessage_GetString(req, "uuid", &uuid);
        rtMessage_GetString(req, "property", &property);
        printf("Device identifier is %s, Property requested: %s\n", uuid, property);
        onDeviceProperty(uuid,property,buf2,sizeof(buf2));

        rtMessage_SetString(res, "value",buf2);
        rtConnection_SendResponse(con, rtHeader, res, 1000);
        rtMessage_Release(res);
    }
    rtMessage_Release(req);
}

void getuuidClass(char *uid,char *output,int output_size)
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
         printf("Method 1:\n");
        for (int i = 0; buf[i] != '\0'; i++) {
          //putchar(buf[i]);
          printf("buf[%d] = %c\n ",i, buf[i]);
         }
        printf("Method 2:\n");
        for (int j = 0; str1[j] != '\0'; j++) {
          //putchar(str1[j]);
          printf("str1[%d] = %c\n ",j, str1[j]);
         // printf("\n");
         }


         removeTrailingSpaces(buf);


                  printf("AMethod 1:\n");
        for (int i = 0; buf[i] != '\0'; i++) {
          //putchar(buf[i]);
          printf("buf[%d] = %c\n ",i, buf[i]);
         }
        printf("AMethod 2:\n");
        for (int j = 0; str1[j] != '\0'; j++) {
          //putchar(str1[j]);
          printf("str1[%d] = %c\n ",j, str1[j]);
         // printf("\n");
         }
	snprintf(output, output_size, "%s", buf);


}
void onDeviceProperty(char *param,char *proName ,char *output, int output_size)
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
	getuuidClass(param,bufw,sizeof(bufw));
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

// Assume the definitions for rtConnection, rtMessageHeader,
// and rtMessage functions are available in the C code.
//CKP ready for test
void onSendCommand1(rtMessageHeader const *rtHeader, uint8_t const *rtMsg, uint32_t rtMsgLength, void *userData)
{
    rtConnection con = (rtConnection)userData;
    rtMessage req;
    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);

    if (rtMessageHeader_IsRequest(rtHeader))
    {
        
	char cmd[1000]={'\0'};    
	char buf[1024]={'\0'};
	char bufa[2048]={'\0'};
	char bufb[2048]={'\0'};
        int ret;
	rtMessage res;
        rtMessage_Create(&res);

        const char *uuid, *command;
        rtMessage_GetString(req, "uuid", &uuid);
        rtMessage_GetString(req, "command", &command);

        printf("Device identifier is %s, command requested: %s\n", uuid, command);
        //ckp start
        printf("Read the existing state of bulb .. \n");
        onDeviceProperty(uuid,"isOn",bufa,sizeof(bufa));
        printf("bulb exis state is %s\n",bufa);
        if(strcmp(bufa,command) ==0)
        {
            printf("state is same \n");
            return 1;
        }
	else
	{
		sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --wr /%s/ep/1/r/isOn %s",uuid,command);
        	printf("command is %s\n",cmd);
        	ret = _syscmd(cmd, buf, sizeof(buf));
        	if ((ret != 0) && (strlen(buf) == 0))
        		 return -1;

       		// snprintf(output, output_size, "%s", buf);
		onDeviceProperty(uuid,"isOn",bufb,sizeof(bufb));
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

	}		//ckp end
    }
    rtMessage_Release(req);
}

void onSendCommand(char *param,char *val,char *output, int output_size)
{
    char cmd[1000]={'\0'};
    char buf[1024]={'\0'};
    char bufa[2048]={'\0'};
    char bufb[2048]={'\0'};
    int ret;
    printf("Read the existing state of bu;b .. \n");
    onDeviceProperty(param,"isOn",bufa,sizeof(bufa));
    printf("bulb exis state is %s\n",bufa);
    if(strcmp(bufa,val) ==0)
    {
	    printf("state is same \n");
	    return 1;
    }
    else
    {    
    printf("Inside %s UUID is %s,val %s\n",__FUNCTION__,param,val);
        sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --wr /%s/ep/1/r/isOn %s",param,val);
	printf("command is %s\n",cmd);
    ret = _syscmd(cmd, buf, sizeof(buf));
    if ((ret != 0) && (strlen(buf) == 0))
        return -1;
    snprintf(output, output_size, "%s", buf);
    onDeviceProperty(param,"isOn",bufb,sizeof(bufb));
    printf("bulb new  state is %s\n",bufb);
    if(strcmp(bufb,bufa) == 0)
    {
	    printf("state did not change \n");
	    return -1;
    }
    else
    {
	    printf("state change happened \n");
	    return 1;
    }
   }
    //printf("send comm is %s\n",buf);
}

void extractAfterLastSlash(const char *str, char *ps ,int size) {
    int lastSlashPos = -1;
    int strLength = strlen(str);

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


// Function to remove the device-specific prefix from the key
void removeDevicePrefix(char *uuid,char* key) {
    //char* prefix = "/000d6f000ef0a7a8/ep/1/r/";
    char* prefix;
    sprintf(prefix, "/%s/ep/1/r/",uuid);
    printf("prefix is %s\n",prefix);
    size_t prefixLen = strlen(prefix);
    if (strncmp(key, prefix, prefixLen) == 0) {
        memmove(key, key + prefixLen, strlen(key) - prefixLen + 1);
    }
}

// Function to tokenize the buffer based on newlines and equal signs
void tokenizeBuffer(char *uuid,char buffer[], rtMessage props) {
    const char* delimiter = "\n";
    char* token;
    char* bufferCopy = strdup(buffer); // Create a copy of the buffer as strtok modifies the original string

    token = strtok(bufferCopy, delimiter);
    while (token != NULL) {
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
	    printf("------\n");
	    printf("keyval is %s\n", keyValue1);
            // Use rtMessage_AddString to add the key-value pair to the rtMessage
            rtMessage_AddString(props, "properties", keyValue1);

            // Remove the device-specific prefix from the key, if present
            removeDevicePrefix(uuid,key);
        }

        token = strtok(NULL, delimiter);
    }

    free(bufferCopy); // Free the allocated buffer copy
}

// Assume the definitions for rtConnection, rtMessageHeader,
// and rtMessage functions are available in the C code.

void onDeviceProperties1(rtMessageHeader const *rtHeader, uint8_t const *rtMsg, uint32_t rtMsgLength, void *userData)
{
    char buf[5000]={'\0'};
    rtConnection con = (rtConnection)userData;
    rtMessage req;
    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);

    if (rtMessageHeader_IsRequest(rtHeader))
    {
        rtMessage res;
        rtMessage_Create(&res);

        const char *uuid;
        rtMessage_GetString(req, "uuid", &uuid);

        printf("Device identifier is %s\n", uuid);
        rtMessage props;
        rtMessage_Create(&props);
	
	onDeviceProperties(uuid,buf,sizeof(buf));
	getuuidClass(uuid,buf,sizeof(buf));
	if(strcmp(buf,"light") == 0)
	{
        	tokenizeBuffer(uuid,buf,props);
		printf("class is light -- \n");
	}
	else
	{
		tokenizeBuffer(uuid,buf,props);
		printf("class is camera -- \n");
	}

        rtMessage_AddString(props, "properties", "Prop1=Josekutty");
        rtMessage_AddString(props, "properties", "Prop2=Kottarathil");
        rtMessage_AddString(props, "properties", "Prop3=Comcast");
        rtMessage_AddString(props, "properties", "Prop4=Engineer");

        rtMessage_SetMessage(res, "properties", props);
        rtConnection_SendResponse(con, rtHeader, res, 1000);
        rtMessage_Release(res);
    }
    rtMessage_Release(req);
}

void onDeviceProperties(char *param, char *output, int output_size)
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

void discoverDeviceRaw(char *output, int output_size)
{
    char buf[MAX_BUF_SIZE];
    char cmd[MAX_CMD_SIZE];
    int ret = 0;

    sprintf(cmd, "/opt/icontrol/bin/xhDeviceUtil --pa | grep -i class");
    ret = _syscmd(cmd, buf, sizeof(buf));
    if ((ret != 0) && (strlen(buf) == 0))
        return -1;
    snprintf(output, output_size, "%s", buf);

    return 0;
}
void retrieveCameraDeviceInfo(rDevice *rdevice) {
#if 0
      	char buffer[] = "944a0c25412c: My Camera 1, Class: camera\n"
                    "        /944a0c25412c/r/communicationFailure = false\n"
                    "        /944a0c25412c/r/dateAdded = 1689201734867\n"
                    "        /944a0c25412c/r/dateLastContacted = 1686779769305\n"
                    "        /944a0c25412c/r/firmwareUpdateStatus = (null)\n"
                    "        /944a0c25412c/r/firmwareVersion = 3.0.02.51\n"
                    "        /944a0c25412c/r/hardwareVersion = 1\n"
                    "        /944a0c25412c/r/ipAddress = 10.0.0.228\n"
                    "        /944a0c25412c/r/macAddress = 94:4a:0c:25:41:2c\n"
                    "        /944a0c25412c/r/manufacturer = iControl\n"
                    "        /944a0c25412c/r/model = iCamera2-C\n"
                    "        /944a0c25412c/r/ping = (null)\n"
                    "        /944a0c25412c/r/portNumber = 443\n"
                    "        /944a0c25412c/r/reboot = (null)\n"
                    "        /944a0c25412c/r/resetToFactoryDefaults = (null)\n"
                    "        /944a0c25412c/r/serialNumber = 1509AXN046181\n"
                    "        /944a0c25412c/r/setWifiCredentials = (null)\n"
                    "        /944a0c25412c/r/signalStrength = (null)\n"
                    "        /944a0c25412c/r/timezone =\n"
                    "        Endpoint camera: Profile: camera\n"
                    "                /944a0c25412c/ep/camera/r/adminPassword = (encrypted)\n"
                    "                /944a0c25412c/ep/camera/r/adminUserId = (encrypted)\n"
                    "                /944a0c25412c/ep/camera/r/apiVersion = 3.3\n"
                    "                /944a0c25412c/ep/camera/r/aspectRatio = 16:9\n"
                    "                /944a0c25412c/ep/camera/r/createMediaTunnel = (null)\n"
                    "                /944a0c25412c/ep/camera/r/destroyMediaTunnel = (null)\n"
                    "                /944a0c25412c/ep/camera/r/getPicture = (null)\n"
                    "                /944a0c25412c/ep/camera/r/label = My Camera 1\n"
                    "                /944a0c25412c/ep/camera/r/motionCapable = false\n"
                    "                /944a0c25412c/ep/camera/r/pictureURL = https://10.0.0.228:443/openhome/streaming/channels/0/picture\n"
                    "                /944a0c25412c/ep/camera/r/recordable = true\n"
                    "                /944a0c25412c/ep/camera/r/resolution = 1280:720\n"
                    "                /944a0c25412c/ep/camera/r/uploadVideoClip = (null)\n"
                    "                /944a0c25412c/ep/camera/r/userPassword = (encrypted)\n"
                    "                /944a0c25412c/ep/camera/r/userUserId = (encrypted)\n"
                    "                /944a0c25412c/ep/camera/r/videoInfo = {\"videoFormats\":[\"MJPEG\",\"FLV\",\"RTSP\"],\"videoCodecs\":[\"H264\",\"MPEG4\"],\"formatURLs\":{\"MJPEG\":\"https:}\n"
                    "        Endpoint sensor: Profile: sensor\n"
                    "                /944a0c25412c/ep/sensor/r/bypassed = true\n"
                    "                /944a0c25412c/ep/sensor/r/faulted = false\n"
                    "                /944a0c25412c/ep/sensor/r/motionSensitivity = low\n"
                    "                /944a0c25412c/ep/sensor/r/tampered = false\n"
                    "                /944a0c25412c/ep/sensor/r/type = motion";
#endif
    char command[200];
    char output[100];

        printf("%s function \n",__FUNCTION__);

    // Retrieve userUserId using xhDeviceUtil command
    // xhDeviceUtil --list -i camera
    //
    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --list -i camera");
   // printf("command to get camera uuid is %s\n",command);
    _syscmd(command, output, sizeof(output));
   // printf("Camera UUID is %s\n",output);
    output[strcspn(output, "\n")] = '\0';
    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --rr /%s/ep/camera/r/userUserId",output);
   // printf("command to get userid is %s\n",command);
    _syscmd(command, output, sizeof(output));
    sscanf(output, "%s", rdevice->userUserId);
   // printf("UserId is  %s\n",output,rdevice->userUserId);
    //1
    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --list -i camera");
    //printf("1command to get camera uuid is %s\n",command);
    _syscmd(command, output, sizeof(output));
    //printf("1Camera UUID is %s\n",output);
    output[strcspn(output, "\n")] = '\0';

    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --rr /%s/ep/camera/r/userPassword",output);
    _syscmd(command, output, sizeof(output));
    sscanf(output, "%s", rdevice->userPassword);
    //printf("UserPass is %s\n",rdevice->userPassword);
   
    //2
    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --list -i camera");
    //printf("2command to get camera uuid is %s\n",command);
    _syscmd(command, output, sizeof(output));
    //printf("2Camera UUID is %s\n",output);
    output[strcspn(output, "\n")] = '\0';

    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --rr /%s/r/ipAddress ",output);
    _syscmd(command, output, sizeof(output));
    sscanf(output, "%s", rdevice->ipAddress);
    //printf("Ipaddress is %s\n",rdevice->ipAddress);

    //3
    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --list -i camera");
    //printf("3command to get camera uuid is %s\n",command);
    _syscmd(command, output, sizeof(output));
   // printf("3Camera UUID is %s\n",output);
    output[strcspn(output, "\n")] = '\0';

     //snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --rr /%s/r/macAddress ",output);
    _syscmd(command, output, sizeof(output));
    sscanf(output, "%s", rdevice->macAddress);
   // printf("Macaddress  is %s\n",rdevice->macAddress);
    //4
    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --list -i camera");
   // printf("4command to get camera uuid is %s\n",command);
    _syscmd(command, output, sizeof(output));
   // printf("4Camera UUID is %s\n",output);
    output[strcspn(output, "\n")] = '\0';

    snprintf(command, sizeof(command), "/opt/icontrol/bin/xhDeviceUtil --rr /%s/r/portNumber ",output);
   // printf("port num command is %s\n",command);
    _syscmd(command, output, sizeof(output));
    
    sscanf(output, "%d", &rdevice->portNumber);
    //printf("portNumber is %d\n",rdevice->portNumber);
}
//CKP ready for test
void onAvailableDevices1(rtMessageHeader const *rtHeader, uint8_t const *rtMsg, uint32_t rtMsgLength, void *userData)
{
    rtConnection con = (rtConnection)userData;
    rtMessage req;
    rtMessage_FromBytes(&req, rtMsg, rtMsgLength);

    if (rtMessageHeader_IsRequest(rtHeader))
    {
        rtMessage res;
        rtMessage_Create(&res);

        rtMessage device;
        //rtMessage_Create(&device);
        numDevices = onAvailableDevices(devices);
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
    		// Set Device ID
    		rtMessage_SetString(device, "id", id_str);

    		// Set Device Name
    		rtMessage_SetString(device, "name", devices[i].description);

   		// Set Device Class
    		rtMessage_SetString(device, "class", devices[i].class);
                rtMessage_AddMessage(res, "devices", device);
                rtMessage_Release(device);
	}
        //rtMessage_SetString(device, "name", "Philips");
        //rtMessage_SetString(device, "uuid", "1234-PHIL-LIGHT-BULB");
        //rtMessage_SetString(device, "devType", "1");
        //rtMessage_AddMessage(res, "devices", device);
        //rtMessage_Release(device);

        //rtMessage_Create(&device);
        //rtMessage_SetString(device, "name", "Hewei-HDCAM-1234");
        //rtMessage_SetString(device, "devType", "0");

        //rtMessage_AddMessage(res, "devices", device);
        //rtMessage_Release(device);

        rtConnection_SendResponse(con, rtHeader, res, 1000);
        rtMessage_Release(res);
    }
    rtMessage_Release(req);
}

/*Cleaned up & Tested code */
int onAvailableDevices(Device devices[]) {
    
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
               devices[DeviceCount].id, devices[DeviceCount].description, devices[DeviceCount].class);
        DeviceCount++;
        token = strtok(NULL, "\n");
    }

    return DeviceCount;
}

#ifdef CKP
int discoverDevices(Device devices[]) {
    FILE *pipe;
    char line[MAX_LINE_LENGTH];
    int numDevices = 0;
    char buf[1024];
    char cmd[128];
    int count = 0;
    int returnValue;
        printf("%s function \n",__FUNCTION__);
    returnValue= _syscmd("/opt/icontrol/bin/xhDeviceUtil --pa | grep -i class",buf,sizeof(buf));
    if (result != 0) 
    {
    	printf("Error executing _syscmd \n");
    	return -1; // Or any other appropriate error code
    }
    // printf("data is %s\n",buf);
     // Tokenize the buffer and copy data into structures
    char* token = strtok(buf, "\n");
    while (token != NULL && count < MAX_DEVICES) {
        sscanf(token, "%16[^:]: %49[^,], Class: %19s",
               devices[count].id, devices[count].description, devices[count].class);
        count++;
        token = strtok(NULL, "\n");
    }

    return count;
}

#endif
int main() {
//    Device devices[MAX_DEVICES];
    rDevice rdevice;
    char buf[5000] = {0};
    char buf1[2048] = {0};
    char buf2[2048] = {0};
    char buf3[2048] = {0};
   // char *PropName = "isOn" ;
    char *PropName = "videoInfo";
    char *lUUID = "000d6f000ef0a7a8";
    char *UUID = "944a0c25412c";
    //light char *Class = "  light";
    char *Class = "  camera";
    char *Value = "false";
    rtError err;
    printf("Test discovered device details ..\n ");
#ifdef RDK
    numDevices = onAvailableDevices(devices);

   if (numDevices < 0) {
         printf("No devices discovered\n");
        return 1;
    }

    // Print the extracted device class information
    for (int i = 0; i < numDevices; i++) {
        printf("Device ID: %s\n", devices[i].id);
        printf("Device Name: %s\n", devices[i].description);
        printf("Device Class: %s\n", devices[i].class);
        printf("----------------------\n");
    }

#ifdef RAW
    /*If the rbus does not accept struct then pass data in buffer*/
    discoverDeviceRaw(buf1,sizeof(buf1));
    printf("discover raw device is %s\n",buf1);
#endif

    printf("Read properties based on UUID,for testing we are passing camera UUID \n");
    //readDevProperty(devices[0].id,buf,sizeof(buf));
    onDeviceProperties(UUID,buf,sizeof(buf));
    printf("Device property is %s\n",buf);
    printf("-------Read device property value based on UUID and property name -------");
    //readDevPropertyVal(UUID,PropName,Class,buf2,sizeof(buf2));
    onDeviceProperty(UUID,PropName,buf2,sizeof(buf2));
    printf("Send command to light device -wr /000d6f000ef0a7a8/ep/1/r/isOn true or false\n");
    onSendCommand(lUUID,Value,buf3,sizeof(buf3));
    sleep(5);
#if 0
    printf("Retrieve Camera device details ..\n ");
    retrieveCameraDeviceInfo(&rdevice);
      // Print the retrieved data
    //printf("Name: %s\n", rdevice.name);
    printf("User User ID: %s\n", rdevice.userUserId);
    printf("User Password: %s\n", rdevice.userPassword);
    printf("ipaddr: %s\n", rdevice.ipAddress);
    printf("macaddr: %s\n", rdevice.macAddress);
    printf("port: %d\n", rdevice.portNumber);
    return 0;
#endif
//#ifdef RT
    
  //  rtError err;
  //int i;
#endif
  printf("rtMessage \n");
  rtLog_SetLevel(RT_LOG_DEBUG);

  rtConnection con;
  rtConnection_Create(&con, "IOTGateway", "tcp://10.0.0.1:10001");
  rtConnection_AddListener(con, "getAvailableDevices", onAvailableDevices1, con);
  rtConnection_AddListener(con, "getDeviceProperties", onDeviceProperties1, con);
  rtConnection_AddListener(con, "getDeviceProperty", onDeviceProperty1, con);
  rtConnection_AddListener(con, "sendCommand", onSendCommand1, con); 
  pause();
  rtConnection_Destroy(con);
  //JK star
#if 0  
  rtLog_SetLevel(RT_LOG_DEBUG);
    cout<<"Version 1.0 "<<endl;

   rtConnection con;
    rtConnection_Create(&con, "IOTGateway", argv[1]);
        rtConnection_AddListener(con, "getAvailableDevices", onAvailableDevices, con);
    rtConnection_AddListener(con, "getDeviceProperties", onDeviceProperties, con);
    rtConnection_AddListener(con, "getDeviceProperty", onDeviceProperty, con);
    rtConnection_AddListener(con, "sendCommand", onSendCommand, con);
    waitForTermSignal();
    rtConnection_Destroy(con);
#endif
//#endif
    return 0;
}

