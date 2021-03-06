/////////////////////////////////////////////////////////////////////////////////
//                             Included header files                           //
/////////////////////////////////////////////////////////////////////////////////
#include "FXOS8700CQDriver.h"
#include "FXOS8700CQRegisterAddressMap.h"
#include "GPIO.h"
#include <stdlib.h>
#include "I2C_Library.h"
/////////////////////////////////////////////////////////////////////////////////
//                       Constants and macro definitions                       //
/////////////////////////////////////////////////////////////////////////////////

#define DEF_SLAVE_ADDR 0x1D
#define DEF_SLAVE_NAME 0xC7

#define MAG_SENSITIVITY 0.1
#define ACC_SENSITIVITY(x) (1.0/(float)(4096>>x))/**recieves a FX_accScales and gives the current sensitivity*/

#define MAG_TOTAL_REGISTERS 6
#define ACC_TOTAL_REGISTERS 6

/*for XYZ_DATA_CONFIG*/
#define XYZ_HPF_MASK(x) (x<<5)
#define XYZ_SCALE_MASK(x) (x)
/*for CTRL_REG1*/
#define CTRL1_ACTIVE_MASK(x) (x)
#define CTRL1_LOW_NOISE_MASK(x) (x<<2)
#define CTRL1_ODR_MASK(x) (x<<3)

/*for CTRL_REG4*/
#define CTRL4_DATA_READY_IE_MASK(x) (x)
/*for CTRL_REG5*/
#define CTRL5_DATA_READY_PIN_MASK(x) (x)
#define CTRL5_INT1_INTERRUPT 1
#define CTRL5_INT2_INTERRUPT 0
/*for M_CTRL_REG1*/
#define MCTRL1_ACAL_MASK(x) (x<<7)
#define MCTRL1_RST_MASK(x) (x<<6)
#define MCTRL1_OST_MASK(x) (x<<5)
#define MCTRL1_OSR_MASK(x) (x<<2)
#define MCTRL1_HMS_MASK(x) (x)
#define MAX_OSR 7
/*for M_CTRL_REG2*/
#define MCTRL2_AUTOINC_MASK(x) (x<<5)
/////////////////////////////////////////////////////////////////////////////////
//                   Local variable definitions ('static')                     //
/////////////////////////////////////////////////////////////////////////////////
static sData accData;
static sData magData;
static FX_config currentConf;///Current driver configuration


static uint8_t dataBuff[12];///Buffer for I2C data
static I2C_CONTROL_T i2cConfig;///I2C handling struct
static bool dataFlag;///flag to indicate new data gathered from the sensor

/////////////////////////////////////////////////////////////////////////////////
//                 Local function prototypes and definitions('static')         //
/////////////////////////////////////////////////////////////////////////////////

static void readData(void);
static void sDataReady(void);

/**
 * @brief Callback for the sensor's data ready interruption
 * */
static void sDataReady(void)
{
	static uint32_t counter=0;
	counter++;
	//I2C_SetDefaultConfig(&i2cConfig, DEF_SLAVE_ADDR,I2C_FREQ_48K8,readData);

	switch(currentConf.mode)
	{
		case FX_HYBRID:
			i2cConfig.address_reg=OUT_X_MSB;
			i2cConfig.dataSize=MAG_TOTAL_REGISTERS+ACC_TOTAL_REGISTERS;
			break;
		case FX_ACC_ONLY:
			i2cConfig.address_reg=OUT_X_MSB;
			i2cConfig.dataSize=ACC_TOTAL_REGISTERS;
			break;
		case FX_MAG_ONLY:
			i2cConfig.address_reg=M_OUT_X_MSB;
			i2cConfig.dataSize=MAG_TOTAL_REGISTERS;
			break;
	}

	if(I2C_ReadData(&i2cConfig)==I2C_BUS_BUSY){}
}


/**
 * @brief This is the callback for I2C_Read, processes the data from I2C.
 * */
static void readData(void)
{
	if(i2cConfig.fault==I2C_NO_FAULT)
	{	switch(i2cConfig.address_reg)
		{
			case WHO_AM_I:
				//assert(i2cConfig.data[0] != DEF_SLAVE_NAME)
				break;
			case OUT_X_MSB:
				if(i2cConfig.dataSize>=ACC_TOTAL_REGISTERS)
				{
					/*			Debug lines
									uint16_t xMSB = (uint16_t)( i2cConfig.data[4] << 8);
									uint16_t a= ((uint16_t)(i2cConfig.data[5] & 0x00FF));
									uint16_t xLSB = xMSB | a;
									accData.z = (xLSB>>2);
					 */
					int16_t ax=0,ay=0,az=0;
					/*Transform accelerometer data to a 14bit number in an int16_t */
					ax=(int16_t)(((i2cConfig.data[0] << 8) | i2cConfig.data[1]))>> 2;
					ay=(int16_t)(((i2cConfig.data[2] << 8) | i2cConfig.data[3]))>> 2;
					az=(int16_t)(((i2cConfig.data[4] << 8) | i2cConfig.data[5]))>> 2;

					/*Make accelerometer units [g]*/
					accData.x=(float) ax * ACC_SENSITIVITY(currentConf.scale);
					accData.y=(float) ay * ACC_SENSITIVITY(currentConf.scale);
					accData.z=(float) az * ACC_SENSITIVITY(currentConf.scale);
					dataFlag=true;
				}
				if(i2cConfig.dataSize==MAG_TOTAL_REGISTERS+ACC_TOTAL_REGISTERS && currentConf.mode==FX_HYBRID)
				{
					int16_t	mx=0,my=0,mz=0;
					/*Transform mag data to an int16_t*/
					mx=(int16_t)(((i2cConfig.data[6] << 8) | i2cConfig.data[7]));
					my=(int16_t)(((i2cConfig.data[8] << 8) | i2cConfig.data[9]));
					mz=(int16_t)(((i2cConfig.data[10] << 8) | i2cConfig.data[11]));
					/*Make magnetometer units [uT]*/
					magData.x=(float)mx*MAG_SENSITIVITY;
					magData.y=(float)my*MAG_SENSITIVITY;
					magData.z=(float)mz*MAG_SENSITIVITY;


				}
				break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////
//                 				SERVICES								       //
/////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Checking function for new data from the sensor
 * @return true if new data was collected, false if not
 * */
bool FX_newData()
{
	return dataFlag;
}

/**
 * @brief returns a default configuration for the sensor, which can be used in most cases
 * @return FX_config containing {FX_HYBRID, FX_SCALE2, FX_ODR_200}
 * */
FX_config FX_GetDefaultConfig(void)
{
	FX_config defConf={FX_HYBRID,FX_SCALE2,FX_ODR_200};
	return defConf;
}
/**
 * @brief Initialises the driver with the given configuration
 * @param config configuration for the driver containing scale, ODR, and mode
 * @return true if the initialization was ok, false if not
 * */
bool FX_Init(FX_config conf)
{
	bool retVal = true;

	I2C_SetDefaultConfig(&i2cConfig, DEF_SLAVE_ADDR,I2C_FREQ_48K8,readData);

	i2cConfig.data = dataBuff;


	I2C_init(&i2cConfig);


	/**Slave Name check*/
	i2cConfig.address_reg=WHO_AM_I;
	i2cConfig.dataSize=1;

	if(I2C_Blocking_ReadData(&i2cConfig) != I2C_NO_FAULT)
		digitalWrite(PIN_LED_RED,1);

	/**Sensor disable using CTRL_REG1*/
	i2cConfig.address_reg=CTRL_REG1;
	i2cConfig.data[0]=0;

	if(I2C_Blocking_WriteData(&i2cConfig) != I2C_NO_FAULT)
		retVal=false;

	/** Out data configuration register for scale and no highpass filter*/
	if(conf.mode!=FX_MAG_ONLY)
	{
		i2cConfig.address_reg=XYZ_DATA_CFG;
		i2cConfig.data[0]=XYZ_HPF_MASK(0)|XYZ_SCALE_MASK(conf.scale); //i2cConfig.data[0]=0b00000000;
		if(I2C_Blocking_WriteData(&i2cConfig) != I2C_NO_FAULT)
			retVal=false;
	}

	/**Enable data ready interrupt in the interrupt enable register*/
	i2cConfig.address_reg=CTRL_REG4;
	i2cConfig.data[0]=CTRL4_DATA_READY_IE_MASK(1); //i2cConfig.data[0]=0b00000001;

	if(I2C_Blocking_WriteData(&i2cConfig) != I2C_NO_FAULT)
		retVal=false;

	/*Configure interruption signal to be in pin INT2*/
	i2cConfig.address_reg=CTRL_REG5;
	/**/
	i2cConfig.data[0]=CTRL5_DATA_READY_PIN_MASK(CTRL5_INT2_INTERRUPT); //i2cConfig.data[0]=0x00;

	if(I2C_Blocking_WriteData(&i2cConfig) != I2C_NO_FAULT)
		retVal=false;



	/**/
	i2cConfig.address_reg=M_CTRL_REG1;
	i2cConfig.data[0]= MCTRL1_ACAL_MASK(0) | MCTRL1_RST_MASK(0)| MCTRL1_OST_MASK(0)
			| MCTRL1_OSR_MASK(MAX_OSR) | MCTRL1_HMS_MASK(conf.mode);//i2cConfig.data[0]&=//i2cConfig.data[0]=0b00011111;

	if(I2C_Blocking_WriteData(&i2cConfig) != I2C_NO_FAULT)
		retVal=false;;

	/*Hybrid autoincrement mode configuration to read the 12 bytes in burst*/
	if(conf.mode==FX_HYBRID)
	{
		i2cConfig.address_reg=M_CTRL_REG2;
		/*Enable Hybrid auto-increase mode for burst read*/
		i2cConfig.data[0]=MCTRL2_AUTOINC_MASK(1);//i2cConfig.data[0]=0b00010000;
		if(I2C_Blocking_WriteData(&i2cConfig) != I2C_NO_FAULT)
			retVal=false;
	}

	/*IRQ pin configuration for data ready interrupt*/
	pinMode(PORTNUM2PIN(PC,13),INPUT);
	pinConfigureIRQ(PORTNUM2PIN(PC,13),IRQC_INTERRUPT_FALLING,sDataReady);


	/*The init function activates the sensor, to start measuring*/
	i2cConfig.address_reg=CTRL_REG1;
	i2cConfig.data[0]= CTRL1_ACTIVE_MASK(1) | CTRL1_LOW_NOISE_MASK(1)| CTRL1_ODR_MASK(conf.ODR);//i2cConfig.data[0]=0b00001101;
	if(I2C_Blocking_WriteData(&i2cConfig) != I2C_NO_FAULT)
		retVal=false;;

	currentConf=conf;

	return retVal;
}

/**
 * @brief returns the last data retrieved from the sensor if in hybrid mode
 * @param *acc pointer to the struct were to save the accelerometer data
 * @param *mag pointer to the struct were to save the magnetometer data
 * */
bool FX_GetData(sData *acc, sData *mag)
{
	if(acc!=NULL && mag!=NULL && currentConf.mode==FX_HYBRID)
	{
		*acc=accData;
		*mag=magData;
		dataFlag=false;
	}
	return true;//<--- LE PUSE TRUE (tobi) ver si se usa el bool para manejo de errores

}

/**
 * @brief returns the currently configured mode
 * @return FX_ACC_ONLY,FX_MAG_ONLY or FX_HYBRID
 * */
FX_modes FX_GetMode()
{
	return currentConf.mode;
}
/**
 * @brief returns the currently configured scale
 * @return FX_SCALE2,FX_SCALE4 or FX_SCALE8
 * */
FX_accScales FX_GetScale()
{
	return currentConf.scale;
}
/**
 * @brief returns the last data retrieved from the accelerometer if in acc only mode
 * @param *acc pointer to the struct were to save the accelerometer data
 * */
bool FX_GetAccData(sData *acc)
{
	if(acc!=NULL && currentConf.mode==FX_ACC_ONLY)
	{
		*acc=accData;
		dataFlag=false;
	}
	return 0;//ver si se usa el bool para manejo de errores
}

/**
 * @brief returns the last data retrieved from the accelerometer if in mag only mode
 * @param *mag pointer to the struct were to save the accelerometer data
 * */
bool FX_GetMagData(sData *mag)
{
	if(mag!=NULL && currentConf.mode==FX_MAG_ONLY)
	{
		*mag=magData;
		dataFlag=false;
	}
	return 0;//ver si se usa el bool para manejo de errores
}
/**
 * @brief deactivates the FXOS8700CQ
 * */
void FX_Disable(void)
{
	i2cConfig.address_reg=CTRL_REG1;
	i2cConfig.data[0]=0;

	if(I2C_WriteData(&i2cConfig) != I2C_NO_FAULT)
		digitalWrite(PIN_LED_RED,0);
}
