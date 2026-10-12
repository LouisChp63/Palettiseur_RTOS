#ifndef INC_MAIN_H_
#define INC_MAIN_H_
// Device header
#include "stm32f0xx.h"
// BSP functions
#include "bsp.h"
// FreeRTOS headers
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "stream_buffer.h"

#define	FRAME_LENGTH		14
#define FRAME_LENGTH_ACT	7
#define FRAME_LENGTH_SENS	7

#define	TAG_SENSORS			0xA8
#define	TAG_ACTUATORS		0xAD
#define TAG_UPDATE			0xA3

#define nb_task 	3
#define nb_cartons 	2
#define nb_poussees	3

extern uint8_t	rx_dma_buffer[FRAME_LENGTH];
extern uint8_t	tx_dma_buffer[FRAME_LENGTH_ACT];

//Define actuators
#define ACT_DISTRIBUTION_CARTONS 	                    1             //(0)
#define ACT_TAPIS_DISTRIBUTION_CARTONS 	                (1 << 1)
#define ACT_BLOCAGE_ENTREE_PALETTISEUR  	            (1 << 2)
#define ACT_PORTE 	                                    (1 << 3)
#define ACT_POUSSOIR 	                                (1 << 4)
#define ACT_CLAMP 	                                    (1 << 5)
#define ACT_MONTER_ASCENSEUR 	                        (1 << 6)        //(6)
//#define skip one bit each 7 sensors or actuators
#define ACT_DESCENDRE_ASCENSEUR 	                    (1 << 8)        //(7)
#define ACT_ASCENSEUR_TO_LIMIT                          (1 << 9)
#define ACT_DISTRIBUTION_PALETTE                        (1 << 10)
#define ACT_CHARGER_PALETTE		                        (1 << 11)
#define ACT_TAPIS_CARTON_VERS_PALETTISEUR		        (1 << 12)
#define ACT_TOURNER_CARTON		                        (1 << 13)
#define ACT_DECHARGER_PALETTISEUR		                (1 << 14)     //(13)
//#define skip one bit each 7 sensors or actuators
#define ACT_CHARGER_PALETTISEUR                         (1 << 16)     //(14)
#define ACT_DECHARGER_PALETTE                           (1 << 17)
#define ACT_TAPIS_PALETTE_VERS_ASCENSEUR                (1 << 18)
#define ACT_TAPIS_DISTRIBUTION_PALETTE                  (1 << 19)
#define ACT_TAPIS_FIN                                   (1 << 20)
#define ACT_REMOVER                                     (1 << 21)     //(19)


//Define sensors
#define SEN_CARTON_DISTRIBUE 	                        1           //(0)		//0 quand carton
#define SEN_CARTON_ENVOYE 	                            (1 << 1)				//0 quand carton
#define SEN_ENTREE_PALETTISEUR  	                    (1 << 2)				//0 quand carton
#define SEN_PORTE_OUVERTE 	                            (1 << 3)				//1 quand ouvert
#define SEN_LIMITE_POUSSOIR 	                        (1 << 4)				//1 quand a la limite
#define SEN_CLAMPED 	                                (1 << 5)				//1 quand a clamp
#define SEN_ASCENSEUR_ETAGE_RDC 	                    (1 << 6)      //(6)		//1 quand rdc
//#define skip one bit each 7 sensors or actuators
#define SEN_ASCENSEUR_ETAGE_1 	                        (1 << 8)      //(7)		//1 quand ascenseur
#define SEN_ASCENSEUR_ETAGE_2                           (1 << 9)				//1 quand ascenseur
#define SEN_SORTIE_PALETTE                              (1 << 10)				//1 quand palette
#define SEN_LIMITE_PORTE		                        (1 << 11)				//1 quand a la limite
#define SEN_ASCENSEUR_EN_MOUVEMENT		                (1 << 12)				//1 quand mouvement
#define SEN_ENTREE_PALETTE		                        (1 << 13)				//1 quand palette
#define SEN_BUTEE_CARTON		                        (1 << 14)     //(13)	//1 quand carton



// Global functions
int my_printf	(const char *format, ...);
int my_sprintf	(char *out, const char *format, ...);

#endif /* INC_MAIN_H_ */
