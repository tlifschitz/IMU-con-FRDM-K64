/////////////////////////////////////////////////////////////////////////////////
//                     XXXXXXXX_GENERIC_XXXXXXXXX                              //
//          Grupo 3 - Laboratorio de Microprocesadores - ITBA - 2018           //
//	                                                                           //
/////////////////////////////////////////////////////////////////////////////////

/**
 * @file App.c
 * @brief
 */

/////////////////////////////////////////////////////////////////////////////////
//                             Included header files                           //
/////////////////////////////////////////////////////////////////////////////////
#include "CANCommunications.h"
#include "CAN.h"

/////////////////////////////////////////////////////////////////////////////////
//                       Constants and macro definitions                       //
/////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
//                    Enumerations, structures and typedefs                    //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//                         Global variables definition                         //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//                   Local variable definitions ('static')                     //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//                   Local function prototypes ('static')                      //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
//                         Global function prototypes                          //
/////////////////////////////////////////////////////////////////////////////////

void App_Init (void)
{
	initCANCommunications();

}
static int i=0;
void App_Run (void)
{
	Measurement m;

	if(readMeasurement(&m) == true)
	{
		if(m.boardID == MY_BOARD_ID)
			i+=1;
		else
			i+=2;
	}
}
