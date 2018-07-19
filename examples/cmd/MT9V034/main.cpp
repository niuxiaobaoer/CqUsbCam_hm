
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termio.h>
#include <libusb-1.0/libusb.h>

#include  <sys/select.h>
#include  <sys/time.h>
#include <stdio.h>
#include <unistd.h>

#include <opencv2/highgui/highgui.hpp>
#include <pthread.h>
#include "../../../CqUsbCam/CqUsbCam.h"
#include "../../../CqUsbCam/SensorCapbablity.h"
#include "../../../CqUsbCam/debugsw.h"


#define MAIN_RESOLU_SELECT	 		'a'
#define MAIN_RESOLU_752_480 		'0'
#define MAIN_RESOLU_640_480 		'1'
#define MAIN_RESOLU_640_480SKIP 		'2'
#define MAIN_RESOLU_640_480BIN		'3'

#define MAIN_BITDEPTH_SELECT 	'b'
#define MAIN_BITDEPTH_8		 	'0'
#define MAIN_BITDEPTH_16		 	'1'
#define MAIN_BITDEPTH_L8		 	'2'

#define MAIN_PROCTYPE_SELECT 	'c'
#define MAIN_PROCTYPE_N		 	'0'
#define MAIN_PROCTYPE_X		 	'1'
#define MAIN_PROCTYPE_Y		 	'2'
#define MAIN_PROCTYPE_XY		 	'3'

#define MAIN_CHECK_SPEED 		'd'
#define MAIN_TRIGMODE_AUTO 		'e'
#define MAIN_TRIGMODE_FPGA 		'f'
#define MAIN_TRIGMODE_SOFT 		'N'

#define MAIN_FPGA_TRIG_FREQ_SELECT	'g'
#define MAIN_EXPO_VALUE_SELECT	'h'
#define MAIN_GAIN_VALUE_SELECT	'i'
#define MAIN_AUTO_GAIN_EXPO_SELECT	'j'
#define MAIN_ROI			'k'
#define MAIN_CAPTURE			'l'
#define MAIN_ROI_BOX			'm'
#define MAIN_ANALOG_GAIN_AUTO_TRIG	'n'
#define MAIN_ANALOG_GAIN_FPGA_TRIG	'o'
#define MAIN_ANALOG_GAIN_1X			'1'
#define MAIN_ANALOG_GAIN_2X			'2'
#define MAIN_ANALOG_GAIN_4X			'4'
#define MAIN_ANALOG_GAIN_8X			'8'
#define MAIN_ROI_X1			'p'
#define MAIN_ROI_X2			'q'
#define MAIN_ROI_X3			'r'
#define MAIN_ROI_X4			's'
#define MAIN_ROI_X5			't'
#define MAIN_ROI_Y1			'u'
#define MAIN_ROI_Y2			'v'
#define MAIN_ROI_Y3			'w'
#define MAIN_ROI_Y4			'x'
#define MAIN_ROI_Y5			'y'
#define MAIN_ROI_WIDTH		'A'
#define MAIN_ROI_HEIGHT		'B'
#define MAIN_SAVE_VEDIO		'D'
#define MAIN_SAVE_ALL		'E'
#define MAIN_WRITE_SENSOR_REG	'F'
#define MAIN_READ_SENSOR_REG		'G'
#define MAIN_WRITE_FPGA_REG		'H'
#define MAIN_READ_FPGA_REG		'I'
#define MAIN_SELECT_SENSOR		'J'
#define MAIN_EXIT_NORMAL		'z'
#define MAIN_STOP_CAPTURE		'L'
#define MAIN_SOFT_TRIG		'M'

string sensor = "MT9V034";
unsigned int g_width=752;
unsigned int g_height=480;

pthread_mutex_t mutexDisp;
pthread_mutex_t mutexCam;

void Disp(void* frameData)
{
	printf("endter Disp\n");
	unsigned char* new_frame_data = (unsigned char*)malloc(g_width * g_height);
	memcpy(new_frame_data, frameData, g_width * g_height);
	pthread_mutex_lock(&mutexDisp);
	cv::Mat frame(g_height, g_width, CV_8UC1, /*(unsigned char*)frameData*/new_frame_data);	
	printf("before imshow\n");
	cv::imshow("disp",frame);
	printf("after imshow\n");
	cv::waitKey(5);
	pthread_mutex_unlock(&mutexDisp);
	free(new_frame_data);
	printf("exiting from Disp\n");
}



CCqUsbCam cam0,  *pCamInUse;


unsigned short hex2dec(char *hex)

{

	unsigned short  number=0;

	char *p=hex;

	for(p=hex;*p;++p)
	{
		if((hex[p-hex]<='z')&&(hex[p-hex]>='a'))
			hex[p-hex]=hex[p-hex]-32;
		number=number*16+(hex[p-hex]>='A'?hex[p-hex]-'A'+10:hex[p-hex]-'0');
	}

	return number;

}

void checkspeed()
{
	unsigned int speed = 0;
	cam0.GetUsbSpeed(speed);
	if(speed==LIBUSB_SPEED_SUPER)
	{
		printf("USB 3.0 device found on cam0!\n");
		cam0.SendUsbSpeed2Fpga(LIBUSB_SPEED_SUPER);
	}
	else if(speed==LIBUSB_SPEED_HIGH)
	{
		printf("USB 2.0 device found on cam0!\n");
		cam0.SendUsbSpeed2Fpga(LIBUSB_SPEED_HIGH);
	}
	else
	{
		printf("Device speed unknown on cam0!\n");
	}

}

void timerFunction(int sig)
{

	unsigned long iByteCntPerSec=0;
	unsigned long iFrameCntPerSec=0;

	pthread_mutex_lock(&mutexCam);

	cam0.GetRecvByteCnt(iByteCntPerSec);
	cam0.ClearRecvByteCnt();
	cam0.GetRecvFrameCnt(iFrameCntPerSec);
	cam0.ClearRecvFrameCnt();

	printf("cam0: %ld Fps     %0.4f MBs\n", iFrameCntPerSec, float(iByteCntPerSec)/1024.0/1024.0);


	alarm(1);

	pthread_mutex_unlock(&mutexCam);

}

int LIBUSB_CALL usb_arrived_callback(struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event event, void *userdata)
{

	//libusb_handle_events_completed(ctx, NULL);

	struct libusb_device_handle *handle;
	struct libusb_device_descriptor desc;
	printf("enter usb_arrieved\n");
	libusb_get_device_descriptor(dev, &desc);
	printf("Add usb device: \n");

	int usbCnt=CCqUsbCam::OpenUSB();
	printf("%d usb device(s) found!\n", usbCnt);
	if(usbCnt<=0)
	{
		printf("exiting ...\n");
		return -1;
	}
	printf("ret of claiminterface is %d\n",	cam0.ClaimInterface(0));//cam0.ClaimInterface(0);
	checkspeed();

	//libusb_handle_events_completed(ctx, NULL);
	//g_w->on_pushButton_OpenUSB_clicked();
	return 0;
}

int LIBUSB_CALL usb_left_callback(struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event event, void *userdata)
{

	//libusb_handle_events_completed(ctx, NULL);


	struct libusb_device_descriptor desc;
	libusb_get_device_descriptor(dev, &desc);
	printf("enter usb_left\n");
	printf("Remove usb device: CLASS(0x%x) SUBCLASS(0x%x) iSerialNumber(0x%x)\n", desc.bDeviceClass, desc.bDeviceSubClass, desc.iSerialNumber);

	pthread_mutex_lock(&mutexCam);
	//alarm(0);
	printf("ret of stopcap is %d\n",	cam0.StopCap());//;cam0.StopCap();
	pthread_mutex_unlock(&mutexCam);

	pthread_mutex_lock(&mutexDisp);
	cv::destroyWindow("disp");
	cv::waitKey(1);
	cv::waitKey(1);
	cv::waitKey(1);
	cv::waitKey(1);
	pthread_mutex_unlock(&mutexDisp);

	printf("ret of releaseinterface is %d \n",	cam0.ReleaseInterface());
	//	CCqUsbCam::CloseUSB();


	return 0;
}

void *ThreadFunc(void*arg)
{
	libusb_hotplug_callback_handle usb_arrived_handle;
	libusb_hotplug_callback_handle usb_left_handle;
	libusb_context *ctx;
	int rc;

	libusb_init(&ctx);


	rc = libusb_hotplug_register_callback(ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, LIBUSB_HOTPLUG_NO_FLAGS, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, 			            usb_arrived_callback, NULL, &usb_arrived_handle);
	if (LIBUSB_SUCCESS != rc)
	{ printf("Error to register usb arrived callback\n");
		goto failure;
	}


	rc = libusb_hotplug_register_callback(ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_NO_FLAGS, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, 
			usb_left_callback, NULL, &usb_left_handle);
	if (LIBUSB_SUCCESS != rc)
	{
		printf("Error to register usb left callback\n");
		goto failure;
	}


	while (1)
	{
		libusb_handle_events_completed(ctx, NULL);
		usleep(1000);
	}

	libusb_hotplug_deregister_callback(ctx, usb_arrived_handle);
	libusb_hotplug_deregister_callback(ctx, usb_left_handle);
	libusb_exit(ctx);
	return NULL;

failure:
	libusb_exit(ctx);
	return NULL;

}
pthread_t tidp;
pthread_t timerthread;
int b_timerfunc=false;
void *ThreadTimerFunc(void *arg)
{
	while(b_timerfunc)
	{
		cam0.SoftTrig();
	//	printf("timerfunc");
		usleep(20000);
	}
	return NULL;
}
void opentimer()
{
	if(b_timerfunc==true)
	{
		b_timerfunc=false;
		//stop thread;
		printf("stop soft trig timer \n");
	}
	else
	{
		printf("start soft trig timer \n");
		b_timerfunc=true;
		pthread_create( &timerthread, NULL, ThreadTimerFunc, NULL);
	}
}

int main(int argc, char *argv[])
{
	cq_int32_t ret;
	ret =pthread_mutex_init(&mutexDisp, NULL);
	if(ret!=0)
		printf("pthread_mutex_init failed");
	ret =pthread_mutex_init(&mutexCam, NULL);
	if(ret!=0)
		printf("pthread_mutex_init failed");

	cam0.SelectSensor(sensor);
	int usbCnt=CCqUsbCam::OpenUSB();
	printf("%d usb device(s) found!\n", usbCnt);
	if(usbCnt>0)
	{
		printf("ret of claiminterface is %d\n",	cam0.ClaimInterface(0));
		checkspeed();
	}
	pthread_create( &tidp, NULL, ThreadFunc, NULL);


	//	CCqUsbCam::CloseUSB();
	cam0.SetResolution(RESOLU_640_480);
	g_height = 480;
	g_width = 640;

	cam0.SetAutoGainExpo(false, false);
	cam0.SetExpoValue(10);
	cam0.SetGainValue(64);
	cam0.SetTrigMode(TRIGMODE_SOFT);
#if 0
	cv::namedWindow("disp",CV_WINDOW_AUTOSIZE | CV_GUI_NORMAL);

	printf("ret of startcap is %d\n",	cam0.StartCap(480, 640,  &Disp));

	signal(SIGALRM, timerFunction);
	alarm(1);
#endif

	
	while(1)
	{

		printf("Please input your choice ...\n");
		printf("\
				\'a\':	Select resolution\n\
				\'c\':	Select proc type\n\
				\'d\':	Check speed\n\
				\'e\':	Set auto trig mode\n\
				\'N\':	Set soft trig mode\n\
				\'f\':	Set fpga trig mode\n\
				\'g\':	Set fpga trig frequency\n\
				\'h\':	Set expo value\n\
				\'i\':	Set gain value\n\
				\'j\':	Set auto-gain-expo function\n\
				\n\
				\'l\':	Start capture\n\
				\'L\':	Stop capture\n\
				\n\
				\'F\':	Write sensor\n\
				\'G\':	Read sensor\n\
				\'H\':	Write FPGA\n\
				\'I\':	Read FPGA\n\
				\n\
				\'M\':	Soft trig\n\
				\n\
				\'N\':	Timer soft trig\n\
				\n\
				\'z\':	Exit\n"\
				);
				
		char ch=getchar();
		getchar();
		printf("Your choice is %c\n", ch);


		switch(ch)
		{
			case MAIN_RESOLU_SELECT:
				{
					printf("Please input your choice ...\n");
					printf("\
							\'0\':	752x480\n\
							\'1\':	640x480\n\
							"\
					      );
					char ch=getchar();
					getchar();	
					printf("Your choice is %c\n", ch);
					switch(ch)
					{
						case MAIN_RESOLU_752_480:
							cam0.SetResolution(RESOLU_752_480);
							{
								g_width=752;
								g_height=480;
							}
							break;
						case MAIN_RESOLU_640_480:
							cam0.SetResolution(RESOLU_640_480);
							{
								g_width=640;
								g_height=480;
							}
							break;

						default:
							printf("Bad inut ...");
					}
					break;
				}

			case MAIN_PROCTYPE_SELECT:
				{
					printf("Please input your choice ...\n");
					printf("\
							\'0\':	Normal\n\
							\'1\':	X mirror\n\
							\'2\':	Y mirror\n\
							\'3\':	XY mirror\n"\
					      );

					char ch=getchar();
					getchar();
					printf("Your choice is %c", ch);
					switch(ch)
					{
						case MAIN_PROCTYPE_N:
							cam0.SetMirrorType(MIRROR_NORMAL);
							break;
						case MAIN_PROCTYPE_X:
							cam0.SetMirrorType(MIRROR_X);
							break;
						case MAIN_PROCTYPE_Y:
							cam0.SetMirrorType(MIRROR_Y);
							break;
						case MAIN_PROCTYPE_XY:
							cam0.SetMirrorType(MIRROR_XY);
							break;
						default:
							printf("Bad inut ...\n");
					}
					break;
				}
			case MAIN_CHECK_SPEED:
				{
					unsigned int speed = 0;
					cam0.GetUsbSpeed(speed);
					if(speed==LIBUSB_SPEED_SUPER)
					{
						printf("USB 3.0 device found!\n");
						cam0.SendUsbSpeed2Fpga(USB_SPEED_SUPER);
					}
					else if(speed==LIBUSB_SPEED_HIGH)
					{
						printf("USB 2.0 device found!\n");
						cam0.SendUsbSpeed2Fpga(USB_SPEED_HIGH);
					}
					else
					{
						printf("Device speed unknown!\n");
					}
					break;
				}
			case MAIN_TRIGMODE_AUTO:
				cam0.SetTrigMode(TRIGMODE_AUTO);
				break;
			case MAIN_TRIGMODE_SOFT:
				cam0.SetTrigMode(TRIGMODE_SOFT);
				break;	
			case MAIN_TRIGMODE_FPGA:
				cam0.SetTrigMode(TRIGMODE_FPGA);
				break;
			case MAIN_FPGA_TRIG_FREQ_SELECT:
				{
					printf("Please input the trig frequency (Dec, 0~45), make sure the device is in FPGA trig mode\n");
					char str[10];
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned int freq=atoi(str);
					printf("Your input is %d \n", freq);
					cam0.SetFpgaTrigFreq(freq);	
					break;				
				}
			case MAIN_AUTO_GAIN_EXPO_SELECT:
				{
					printf("Please input your choice ...\n");
					printf("\
							\'0\':	Manual 	gain, Manual 	expo\n\
							\'1\':	Manual 	gain, Auto 	expo\n\
							\'2\':	Auto	gain, Manual 	expo\n\
							\'3\':	Auto 	gain, Auto	expo\n"\
					      );
					char ch=getchar();
					getchar();
					printf("Your choice is %c\n", ch);
					switch(ch)
					{
						case '0':
							cam0.SetAutoGainExpo(false, false);
							break;
						case '1':
							cam0.SetAutoGainExpo(false, true);
							break;
						case '2':
							cam0.SetAutoGainExpo(true, false);
							break;
						case '3':
							cam0.SetAutoGainExpo(true, true);
							break;
						default:
							printf("Bad inut ...\n");					
					}
					break;
				}

			case MAIN_EXPO_VALUE_SELECT:
				{
					printf("Please input the expo value (Dec, 0~65536)\n");
					char str[10];
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned int expoValue=atoi(str);
					printf("Your input is %d \n", expoValue);
					cam0.SetExpoValue(expoValue);
					break;
				}
			case MAIN_GAIN_VALUE_SELECT:
				{
					printf("Please input the gain value (Dec, 0~64)\n");
					char str[10];
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned int gainValue=atoi(str);
					printf("Your input is %d \n", gainValue);
					cam0.SetGainValue(gainValue);
					break;
				}		
			case MAIN_CAPTURE:
				{
					cv::namedWindow("disp",CV_WINDOW_AUTOSIZE | CV_GUI_NORMAL);

					printf("ret of startcap is %d\n",	cam0.StartCap(g_height,  g_width, Disp));

					signal(SIGALRM, timerFunction);
					alarm(1);
					break;
				}
			case MAIN_STOP_CAPTURE:
				{
					pthread_mutex_lock(&mutexCam);
					alarm(0);
					printf("ret of stopcap is %d\n",	cam0.StopCap());
					pthread_mutex_unlock(&mutexCam);

					pthread_mutex_lock(&mutexDisp);
					cv::destroyWindow("disp");
					cv::waitKey(1);
					cv::waitKey(1);
					cv::waitKey(1);
					cv::waitKey(1);
					pthread_mutex_unlock(&mutexDisp);
					break;
				}

			case MAIN_WRITE_SENSOR_REG:
				{
					printf("Please input the reg address (Hex. Just number, no \'0x\' needed))\n");
					char str[10];
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned int iDecRegAddr=hex2dec(str);
					printf("Your input is %x \n", iDecRegAddr);
					printf("Please input the reg value (Hex. Just number, no \'0x\' needed))\n");
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned int iDecRegValue=hex2dec(str);
					printf("Your input is %x \n", iDecRegValue);
					cam0.WrSensorReg(iDecRegAddr, iDecRegValue);
					//goto Badinput;
					break;
				}
			case MAIN_READ_SENSOR_REG:
				{
					printf("Please input the reg address (Hex. Just number, no \'0x\' needed))\n");
					char str[10];
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned int iDecRegAddr=hex2dec(str);
					printf("Your input is %x \n", iDecRegAddr);
					unsigned int iValue=0;
					cam0.RdSensorReg(iDecRegAddr, iValue);
					printf("reg value is %04x\n", iValue&0xffff);
					break;
				}
			case MAIN_WRITE_FPGA_REG:
				{
					printf("Please input the reg address (Hex. Just number, no \'0x\' needed))\n");
					char str[10];
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned char iDecRegAddr=hex2dec(str);
					printf("Your input is %x \n", iDecRegAddr);
					printf("Please input the reg value (Hex. Just number, no \'0x\' needed))\n");
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned char iDecRegValue=hex2dec(str);
					printf("Your input is %x \n", iDecRegValue);
					cam0.WrFpgaReg(iDecRegAddr, iDecRegValue);
					break;
				}
			case MAIN_READ_FPGA_REG:
				{
					printf("Please input the reg address (Hex. Just number, no \'0x\' needed))\n");
					char str[10];
					memset(str,0,sizeof(str));
					fgets(str, 9,stdin);
					unsigned int iDecRegAddr=hex2dec(str);
					printf("Your input is %x \n", iDecRegAddr);
					unsigned int iValue=0;
					cam0.RdFpgaReg(iDecRegAddr, iValue);
					printf("reg value is %02x\n", iValue&0xff);
					break;
				}
			case MAIN_SOFT_TRIG:
				{
					opentimer();
					break;
				}

			case MAIN_EXIT_NORMAL:
				printf("ret of releaseinterface is %d\n",cam0.ReleaseInterface());
				CCqUsbCam::CloseUSB();

				printf("Exiting ...\n");
				exit(0);
				break;
			default:
				printf("Bad inut ...\n");
		}
	}


	return 0;

}