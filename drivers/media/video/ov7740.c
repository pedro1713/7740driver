#include "generic_sensor.h"
/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*/
static int version = KERNEL_VERSION(0,1,3);
module_param(version, int, S_IRUGO);

static int debug = 2;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_OV7740
#define SENSOR_V4L2_IDENT V4L2_IDENT_OV7740
#define SENSOR_ID 0x7742
#define SENSOR_BUS_PARAM                     (SOCAM_MASTER |\
                                             SOCAM_PCLK_SAMPLE_FALLING|SOCAM_HSYNC_ACTIVE_HIGH| SOCAM_VSYNC_ACTIVE_LOW|\
                                             SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_8  |SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W                     640
#define SENSOR_PREVIEW_H                     480
#define SENSOR_PREVIEW_FPS                   30000     // 15fps
#define SENSOR_FULLRES_L_FPS                 7500      // 7.5fps
#define SENSOR_FULLRES_H_FPS                 30000     // 30fps
#define SENSOR_720P_FPS                      0
#define SENSOR_1080P_FPS                     0

#define SENSOR_REGISTER_LEN                  1         // sensor register address bytes
#define SENSOR_VALUE_LEN                     1         // sensor register value bytes
                                    
static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect|CFG_Scene);
static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;
	unsigned short int preview_maxlines;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;

};

/*
*  The follow setting need been filled.
*  
*  Must Filled:
*  sensor_init_data :               Sensor initial setting;
*  sensor_fullres_lowfps_data :     Sensor full resolution setting with best quality, recommend for video;
*  sensor_preview_data :            Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :          Sensor software reset register;
*  sensor_check_id_data :           Sensor chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data:     Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p:                     Sensor 720p setting, it is for video;
*  sensor_1080p:                    Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

#define ENDMARKER {0xff, 0xff}
#define SENSOR_RST {0x12, 0x80}
#define REG55_PLLx4 {0x55, 0x40}
#define CLK_DIV {0x11, 0x01}
#define HOUT_START {0x17, 0x25}
#define HOUT_SIZE {0x18, 0xa0}
#define VOUT_START {0x19, 0x03}
#define VOUT_SIZE {0x1a, 0xf0}
#define PIX_SHIFT {0x1b, 0x89}

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] ={
	SENSOR_RST,
	SensorWaitMs(5),
	{0x13, 0x00},//Deactivate everything on REG13
	REG55_PLLx4,
	CLK_DIV,

	{0x12, 0x10},
	{0xd5, 0x10},
	{0x0c, 0x02},
	{0x0d, 0x34},//Analog setting

	HOUT_START,
	HOUT_SIZE,
	VOUT_START,
	VOUT_SIZE,
	PIX_SHIFT,

	{0x22, 0x03},
	{0x29, 0x17},
	{0x2b, 0xf0},//Row Counter End Point LSB
	{0x2c, 0x01},
	{0x31, 0xa0},//HSIZE MSB
	{0x32, 0xf0},//VSIZE MSB
	{0x33, 0xc4},//HV OFFSET
	{0x35, 0x05},//Analog setting
	{0x36, 0x3f},//Analog setting

	{0x28, 0x02},
	{0x04, 0x60},//Analog setting
	{0x27, 0x80},
	{0x3d, 0x0f},//Analog setting
	{0x3e, 0x81},//Analog setting
	{0x3f, 0x40},//Analog setting
	{0x40, 0x7f},//Analog setting
	{0x41, 0x6a},//Analog setting
	{0x42, 0x29},//Analog setting
	{0x44, 0xe5},//Analog setting
	{0x45, 0x41},//Analog setting
	{0x47, 0x42},//Analog setting
	{0x48, 0x00},//Analog setting
	{0x49, 0x61},//Analog setting
	{0x4a, 0xa1},
	{0x4b, 0x5e},
	{0x4c, 0x18},
	{0x4d, 0x50},
	{0x4e, 0x13},
	{0x64, 0x00},
	{0x67, 0x88},
	{0x68, 0x1a},
	{0x14, 0x38},
	{0x24, 0x3c},
	{0x25, 0x30},
	{0x26, 0x72},
	{0x50, 0x97},
	{0x51, 0x7e},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x20, 0x00},
	{0x21, 0x23},
	{0x38, 0x14},
	{0xe9, 0x00},
	{0x56, 0x55},
	{0x57, 0xff},
	{0x58, 0xff},
	{0x59, 0xff},
	{0x5f, 0x04},//Analog setting
	{0xec, 0x00},
	{0x13, 0xdf},//Reactivated everything in register REG13
	{0x80, 0x7d},
	{0x81, 0x3f},
	{0x82, 0x32},
	{0x83, 0x01},
	{0x38, 0x11},
	{0x84, 0x70},
	{0x85, 0x00},
	{0x86, 0x03},
	{0x87, 0x01},
	{0x88, 0x05},
	{0x89, 0x30},
	{0x8d, 0x30},
	{0x8f, 0x85},
	{0x93, 0x30},
	{0x95, 0x85},
	{0x99, 0x30},
	{0x9b, 0x85},
	{0x9c, 0x08},
	{0x9d, 0x12},
	{0x9e, 0x23},
	{0x9f, 0x45},
	{0xa0, 0x55},
	{0xa1, 0x64},
	{0xa2, 0x72},
	{0xa3, 0x7f},
	{0xa4, 0x8b},
	{0xa5, 0x95},
	{0xa6, 0xa7},
	{0xa7, 0xb5},
	{0xa8, 0xcb},
	{0xa9, 0xdd},
	{0xaa, 0xec},
	{0xab, 0x1a},
	{0xce, 0x78},
	{0xcf, 0x6e},
	{0xd0, 0x0a},
	{0xd1, 0x0c},
	{0xd2, 0x84},
	{0xd3, 0x90},
	{0xd4, 0x1e},
	{0xd9, 0x00},
	{0x5a, 0x24},
	{0x5b, 0x1f},
	{0x5c, 0x88},
	{0x5d, 0x60},
	{0xac, 0x6e},
	{0xbe, 0xff},
	{0xbf, 0x00},
	{0x70, 0x00},//50/60Hz auto detection is XCLK dependent
	{0x71, 0x34},//the following is based on XCLK = 24MHz
	{0x74, 0x28},
	{0x75, 0x98},
	{0x76, 0x00},
	{0x77, 0x08},
	{0x78, 0x01},
	{0x79, 0xc2},
	{0x7d, 0x02},
	{0x7a, 0x4e},
	{0x7b, 0x1f},
	{0xEC, 0x00},//00/80 for manual/auto
	{0x7c, 0x0c},
	ENDMARKER,
	SensorEnd
};

/* Sensor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
	//7.5 fps with input clock 24 MHz
	{0x11, 0x01},
	{0x55, 0x00},
	{0x15, 0x00},
	ENDMARKER,
	SensorEnd
};

/* Sensor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
	SensorEnd
};

/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] =
{
	{0x0e, 0xe4},//Sleep the sensor
	SENSOR_RST,
	SensorWaitMs(5),
	{0x13, 0x00},//Deactivate everything on REG13
	//Registers related to CLK
	REG55_PLLx4,
	CLK_DIV,
	
	{0x12, 0x00},
	{0xd5, 0x10},
	{0x0c, 0xd2},
	{0x0d, 0x34},//Analog setting

	//Following registers are related to output size
	HOUT_START,
	HOUT_SIZE,
	VOUT_START,
	VOUT_SIZE,
	PIX_SHIFT,

	{0x22, 0x03},
	{0x29, 0x17},
	{0x2b, 0xf0},//Row Counter End Point LSB
	{0x2c, 0x01},
	{0x31, 0xa0},//HSIZE MSB
	{0x32, 0xf0},//VSIZE MSB
	{0x33, 0xc4},//HV Offset
	{0x35, 0x05},//Analog setting
	{0x36, 0x3f},//Analog setting

	{0x28, 0x02},//VSYNC inverse	
	{0x04, 0x60},//Analog setting
	{0x27, 0x80},
	{0x3d, 0x0f},//Analog setting
	{0x3e, 0x81},//Analog setting
	{0x3f, 0x40},//Analog setting
	{0x40, 0x7f},//Analog setting
	{0x41, 0x6a},//Analog setting
	{0x42, 0x29},//Analog setting
	{0x44, 0xe5},//Analog setting
	{0x45, 0x41},//Analog setting
	{0x47, 0x42},//Analog setting
	{0x48, 0x00},//Analog setting
	{0x49, 0x61},//Analog setting
	{0x4a, 0xa1},
	{0x4b, 0x5e},
	{0x4c, 0x18},
	{0x4d, 0x50},
	{0x4e, 0x13},
	{0x64, 0x00},
	{0x67, 0x88},
	{0x68, 0x1a},
	{0x14, 0x38},
	{0x24, 0x3c},
	{0x25, 0x30},
	{0x26, 0x72},
	{0x50, 0x97},
	{0x51, 0x7e},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x20, 0x00},
	{0x21, 0x23},
	{0x38, 0x14},
	{0xe9, 0x00},
	{0x56, 0x55},
	{0x57, 0xff},
	{0x58, 0xff},
	{0x59, 0xff},
	{0x5f, 0x04},//Analog setting
	{0xec, 0x00},
	{0x13, 0xdf},//Reactivated everything in register REG13
	{0x80, 0x7d},
	{0x81, 0x3f},
	{0x82, 0x32},
	{0x83, 0x01},
	{0x38, 0x11},
	{0x84, 0x70},
	{0x85, 0x00},
	{0x86, 0x03},
	{0x87, 0x01},
	{0x88, 0x05},
	{0x89, 0x30},
	{0x8d, 0x30},
	{0x8f, 0x85},
	{0x93, 0x30},
	{0x95, 0x85},
	{0x99, 0x30},
	{0x9b, 0x85},
	{0x9c, 0x08},
	{0x9d, 0x12},
	{0x9e, 0x23},
	{0x9f, 0x45},
	{0xa0, 0x55},
	{0xa1, 0x64},
	{0xa2, 0x72},
	{0xa3, 0x7f},
	{0xa4, 0x8b},
	{0xa5, 0x95},
	{0xa6, 0xa7},
	{0xa7, 0xb5},
	{0xa8, 0xcb},
	{0xa9, 0xdd},
	{0xaa, 0xec},
	{0xab, 0x1a},
	{0xce, 0x78},
	{0xcf, 0x6e},
	{0xd0, 0x0a},
	{0xd1, 0x0c},
	{0xd2, 0x84},
	{0xd3, 0x90},
	{0xd4, 0x1e},
	{0xd9, 0x00},
	{0x5a, 0x24},
	{0x5b, 0x1f},
	{0x5c, 0x88},
	{0x5d, 0x60},
	{0xac, 0x6e},
	{0xbe, 0xff},
	{0xbf, 0x00},
	{0x70, 0x00},//50/60Hz auto detection is XCLK dependent
	{0x71, 0x34},//the following is based on XCLK = 24MHz
	{0x74, 0x28},
	{0x75, 0x98},
	{0x76, 0x00},
	{0x77, 0x08},
	{0x78, 0x01},
	{0x79, 0xc2},
	{0x7d, 0x02},
	{0x7a, 0x4e},
	{0x7b, 0x1f},
	{0xEC, 0x00},//00/80 for manual/auto
	{0x7c, 0x0c},
	{0x38, 0x17},
	{0x84, 0x02},//DSP outputs colorbar testing purposes
	{0x0e, 0xe0},
	ENDMARKER,
	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
	SensorEnd	
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={	
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
	SensorRegVal(0x12,0x80),
	SensorWaitMs(5),
	ENDMARKER,
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
	SensorRegVal(0x0a,0),
	SensorRegVal(0x0b,0),
	ENDMARKER,
	SensorEnd
};
/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
	{0x13, 0xdf},
	{0x15, 0x00},
	{0x2d, 0x00},
	{0x2e, 0x00},
	ENDMARKER,
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
	{0x13, 0xdd},
	{0x01, 0x58},
	{0x02, 0x60},
	{0x03, 0x40},
	{0x15, 0x00},
	{0x2d, 0x00},
	{0x2e, 0x00},
	ENDMARKER,
	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
	//Sunny
	{0x13, 0xdd},
	{0x01, 0x5a},
	{0x02, 0x5c},
	{0x03, 0x42},
	{0x15, 0x00},
	{0x2d, 0x00},
	{0x2e, 0x00},
	ENDMARKER,
	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
	//Office
	{0x13, 0xdd},
	{0x01, 0x84},
	{0x02, 0x4c},
	{0x03, 0x40},
	{0x15, 0x00},
	{0x2d, 0x00},
	{0x2e, 0x00},
	ENDMARKER,
	SensorEnd
};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
	//Home
	{0x13, 0xdd},
	{0x01, 0x96},
	{0x02, 0x40},
	{0x03, 0x4a},
	{0x15, 0x00},
	{0x2d, 0x00},
	{0x2e, 0x00},
	ENDMARKER,
	SensorEnd
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
	// Brightness -2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
	// Brightness -1
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
	//Brightness 0
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
	// Brightness +1
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
	//Brightness +2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
	//Brightness +3
	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
	{0x81, 0x3F},
	{0xda, 0x00},
	{0xdf, 0x80},
	{0xe0, 0x80},
	ENDMARKER,
	SensorEnd
};


static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
	//Negative
	{0x81, 0x3F},
	{0xda, 0x40},
	{0xdf, 0x80},
	{0xe0, 0x80},
	ENDMARKER,
	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
	//Bluish
	{0x81, 0x3F},
	{0xda, 0x18},
	{0xdf, 0xa0},
	{0xe0, 0x40},
	ENDMARKER,
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
	//Greenish
	{0x81, 0x3F},
	{0xda, 0x18},
	{0xdf, 0x60},
	{0xe0, 0x60},
	ENDMARKER,
	SensorEnd
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_Negative,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

static	struct rk_sensor_reg sensor_Exposure0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
	SensorEnd
};

static struct rk_sensor_reg *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
	sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,
};

static	struct rk_sensor_reg sensor_Saturation0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation2[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

static	struct rk_sensor_reg sensor_Contrast0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
	sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
	//3.75 fps night mode for 60 Hz environment 24MHz input clock
	{0x11, 0x03},
	{0x55, 0x00},
	{0x15, 0x00},
	ENDMARKER,
	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

static struct rk_sensor_reg sensor_Zoom0[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] =
{
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL,};

/*
* User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
*/
static struct v4l2_querymenu sensor_menus[] =
{
};
/*
* User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
*/
static struct sensor_v4l2ctrl_usr_s sensor_controls[] =
{
};

//MUST define the current used format as the first item   
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{ V4L2_MBUS_FMT_UYVY8_2X8,V4L2_COLORSPACE_SRGB },
	{ V4L2_MBUS_FMT_YUYV8_2X8,V4L2_COLORSPACE_SRGB }
};

static struct soc_camera_ops sensor_ops;


/*
**********************************************************
* Following is local code:
* 
* Please codeing your program here 
**********************************************************
*/
static int sensor_parameter_record(struct i2c_client *client)
{
	u8 ret_l,ret_h;
	int tp_l,tp_h;
	
	struct generic_sensor *sensor = to_generic_sensor(client);
	struct specific_sensor *spsensor = to_specific_sensor(sensor);

	sensor_write(client,0x13,0x82);	//stop AE/AG

	//Read AEC Gain for preview
	sensor_read(client,0x0F,&ret_h);
	sensor_read(client,0x10, &ret_l);
	tp_l = ret_l;
	tp_h = ret_h;
	spsensor->parameter.preview_exposure = ((tp_h << 8) & 0xFF00) | ((tp_l) & 0x00FF);


	//Read back AGC Gain for preview
	sensor_read(client,0x00, &ret_l);
	spsensor->parameter.preview_gain = ret_l;

	spsensor->parameter.CapturePclk = 24000;
	spsensor->parameter.PreviewPclk = 24000;
	spsensor->parameter.PreviewDummyPixels = 0;
	spsensor->parameter.CaptureDummyPixels = 0;
	SENSOR_DG("Read 0x00=0x%02x  PreviewExposure:%d 0x0F=0x%02x  0x10=0x%02x",
		ret_l, spsensor->parameter.preview_exposure, tp_h, tp_l);
	return 0;
}
#define OV7740_FULL_PERIOD_PIXEL_NUMS	(656)  // default pixel#(w/o dummy pixels) in VGA mode
#define OV7740_FULL_PERIOD_LINE_NUMS	(496)  // default line#(w/o dummy lines) in VGA mode
#define OV7740_PV_PERIOD_PIXEL_NUMS	(656)  // default pixel#(w/o dummy pixels) in QVGA mode
#define OV7740_PV_PERIOD_LINE_NUMS	(496)  // default line#(w/o dummy lines) in QVGA mode

/* SENSOR EXPOSURE LINE LIMITATION */
#define OV7740_FULL_EXPOSURE_LIMITATION   (488)
#define OV7740_PV_EXPOSURE_LIMITATION	  (488)

// SENSOR VGA SIZE
#define OV7740_IMAGE_SENSOR_FULL_WIDTH	  (640)
#define OV7740_IMAGE_SENSOR_FULL_HEIGHT   (480)

#define OV7740_FULL_GRAB_WIDTH				(OV7740_IMAGE_SENSOR_FULL_WIDTH - 16)
#define OV7740_FULL_GRAB_HEIGHT 			(OV7740_IMAGE_SENSOR_FULL_HEIGHT - 16)

static void OV7740WriteShutter(struct i2c_client *client,bool is_preview, unsigned int shutter)
{
	unsigned int extra_exposure_lines = 0;

	if (shutter < 1)
	{
		shutter = 1;
	}
	
	if (is_preview) 
	{
		if (shutter <= OV7740_PV_EXPOSURE_LIMITATION) 
		{
			extra_exposure_lines = 0;
		}
		else 
		{
			extra_exposure_lines=shutter - OV7740_PV_EXPOSURE_LIMITATION;
		}
		
	}
	else 
	{
		if (shutter <= OV7740_FULL_EXPOSURE_LIMITATION) 
		{
			extra_exposure_lines = 0;
		}
		else 
		{
			extra_exposure_lines = shutter - OV7740_FULL_EXPOSURE_LIMITATION;
		}
		
	}
	
	//AEC PK EXPOSURE
	shutter*=16;
	sensor_write(client,0x10, (shutter & 0x00FF));  //AEC[7:0]
	sensor_write(client,0x0F, ((shutter & 0xFF00) >>8));  //AEC[15:8]

	if(extra_exposure_lines>0)
	{
		// set extra exposure line [aec add vts]
		printk(KERN_ERR "ah doesn't knows whut t'do coach\n");
	}
	else
	{
		// set extra exposure line [aec add vts]
		printk(KERN_WARNING "that was easy\n");
	}
	
}

static int sensor_ae_transfer(struct i2c_client *client)
{
	unsigned int prev_line_len,cap_line_len,shutter;
	struct generic_sensor *sensor = to_generic_sensor(client);
	struct specific_sensor *spsensor = to_specific_sensor(sensor);

	mdelay(100);
	shutter = spsensor->parameter.preview_exposure;

	//OV2659SetDummy(client,600,0);	
	
	prev_line_len = OV7740_PV_PERIOD_PIXEL_NUMS + spsensor->parameter.PreviewDummyPixels;
	cap_line_len = OV7740_FULL_PERIOD_PIXEL_NUMS + spsensor->parameter.CaptureDummyPixels;
	shutter = (shutter * spsensor->parameter.CapturePclk) / spsensor->parameter.PreviewPclk;
	shutter = (shutter * prev_line_len) / cap_line_len;
	shutter*=2;

	OV7740WriteShutter(client,0,shutter);
	return 0;
}
/*
**********************************************************
* Following is callback
* If necessary, you could coding these callback
**********************************************************
*/
/*
* the function is called in open sensor  
*/
static int sensor_activate_cb(struct i2c_client *client)
{
//    u8 reg_val;

    SENSOR_DG("%s",__FUNCTION__);
    return 0;
}
/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
//	u8 reg_val;
	struct generic_sensor *sensor = to_generic_sensor(client);

    SENSOR_DG("%s",__FUNCTION__);
    
	/* ddl@rock-chips.com : all sensor output pin must switch into Hi-Z */
	if (sensor->info_priv.funmodule_state & SENSOR_INIT_IS_OK) {
	
	}
	
	return 0;
}
/*
* the function is called before sensor register setting in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
    if (capture) {
        sensor_parameter_record(client);
    }

    return 0;
}
static int sensor_softrest_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	
	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	return 0;
}
/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT  
*/
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
    if (capture) {
        sensor_ae_transfer(client);
    }
    msleep(400);
    return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));
		
	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");
		
	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",pm_msg.event);
		return -EINVAL;
	}
	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{

	SENSOR_DG("Resume");

	return 0;

}
static int sensor_mirror_cb (struct i2c_client *client, int mirror)
{
	//char val;
	int err = 0;
    
    SENSOR_DG("mirror: %d",mirror);
/*	if (mirror) {
		err = sensor_read(client, 0x0C, &val);
		if (err == 0) {
			val |= 0x40;
			err = sensor_write(client, 0x0C, val);
		}
	} else {
		err = sensor_read(client, 0x0C, &val);
		if (err == 0) {
			val &= 0xbf;
			err = sensor_write(client, 0x0C, val);
		}
	} */

	return err;    
}
/*
* the function is v4l2 control V4L2_CID_HFLIP callback  
*/
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
                                                     struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if (sensor_mirror_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%02x",ext_ctrl->value);
	
	SENSOR_DG("sensor_mirror success, value:0x%02x",ext_ctrl->value);
	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	//char val;
	int err = 0;	

    SENSOR_DG("flip: %d",flip);
	/*if (flip) {
		err = sensor_read(client, 0x0C, &val);
		if (err == 0) {
			val |= 0x80;
			err = sensor_write(client, 0x0C, val);
		}
	} else {
		err = sensor_read(client, 0x0C, &val);
		if (err == 0) {
			val &= 0x7f;
			err = sensor_write(client, 0x0C, val);
		}
	}*/

	return err;
}
/*
* the function is v4l2 control V4L2_CID_VFLIP callback  
*/
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info,
                                                     struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

    if (sensor_flip_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%02x",ext_ctrl->value);
	
	SENSOR_DG("sensor_flip success, value:0x%02x",ext_ctrl->value);
	return 0;
}
/*
* the functions are focus callbacks
*/
static int sensor_focus_init_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_near_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_far_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client,int pos){
	return 0;
}

static int sensor_focus_af_const_usr_cb(struct i2c_client *client){
	return 0;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
    return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client, int *zone_tm_pos)
{
    return 0;
}

/*
face defect call back
*/
static int 	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
*   The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function. 
*/
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
    return;
}

/*
* :::::WARNING:::::
* It is not allowed to modify the following code
*/

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();

