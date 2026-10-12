#include "stm32f0xx.h"
#include "main.h"
#include "bsp.h"

/*
 * Global variables
 */
uint8_t	rx_dma_buffer[FRAME_LENGTH];
uint8_t	tx_dma_buffer[FRAME_LENGTH_ACT];

// Static functions
//static void SystemClock_Config(void);

// FreeRTOS tasks
void vTask_1 			(void *pvParameters);
void vTask_2			(void *pvParameters);
void vTask_3			(void *pvParameters);
void vTask_Write	 	(void *pvParameters);
void vTask_Read	 		(void *pvParameters);

/*
* Kernel objects
*/
xQueueHandle 	xSubscribeQueue;
xQueueHandle	xCommandQueue;

xTaskHandle			vTask1_handle;
xTaskHandle			vTask2_handle;
xTaskHandle			vTask3_handle;
xTaskHandle			vTask_Write_handle;
xTaskHandle			vTask_Read_handle;

typedef struct
{
	xTaskHandle handle;
	uint32_t sensor_msk;
	uint32_t sensor_state;
} subscribe_message_t;

typedef struct {
    uint32_t mask;
    uint32_t value;
} actuator_cmd_t;

// Main function
int main()
{
	// Configure System Clock
//    SystemClock_Config();

	// Initialize LED pin
	BSP_LED_Init();

	// Initialize the user Push-Button
	BSP_PB_Init();

	// Initialize Debug Console
	BSP_Console_Init();
	my_printf("\r\nConsole Ready!\r\n");
	my_printf("SYSCLK = %d Hz\r\n", SystemCoreClock);

	// Start Trace Recording
	xTraceEnable(TRC_START);

	// Message queue
	xSubscribeQueue = xQueueCreate(nb_task, sizeof(subscribe_message_t));
	xCommandQueue = xQueueCreate(nb_task, sizeof(actuator_cmd_t));

	// Give a nice name to the Queue in the trace recorder
	vTraceSetQueueName(xSubscribeQueue, "Subscribe Queue");
	vTraceSetQueueName(xCommandQueue, "Command Queue");

	// Create Tasks
	xTaskCreate(vTask_1, 		"Task_1", 		128, NULL, 3, &vTask1_handle);
	xTaskCreate(vTask_2, 		"Task_2", 		128, NULL, 2, &vTask2_handle);
	xTaskCreate(vTask_3, 		"Task_3", 		128, NULL, 4, &vTask3_handle);
	xTaskCreate(vTask_Write, 	"Task_Write", 	128, NULL, 1, &vTask_Write_handle);
	xTaskCreate(vTask_Read, 	"Task_Read", 	128, NULL, 5, &vTask_Read_handle);

	my_printf("Appuyer sur le bouton PB pour commencer\r\n");
	while(BSP_PB_GetState() == 0);
	my_printf("Demarrage du palletizer\r\n");

	// Start the Scheduler
	vTaskStartScheduler();

	while(1)
	{
		// The program should never be here...
	}
}

/*
 *	Task_1
 *	Gestion de l'approvisionnement des cartons
 *	Groupement par groupe de 2 cartons
 */
void vTask_1 (void *pvParameters)
{
	actuator_cmd_t command = {0};

	subscribe_message_t sub_task1;
	sub_task1.handle = vTask1_handle;

	command.mask = ACT_DISTRIBUTION_CARTONS | ACT_BLOCAGE_ENTREE_PALETTISEUR;

	ulTaskNotifyTake(pdTRUE, 0);
	while(1)
	{
		command.value |= ACT_BLOCAGE_ENTREE_PALETTISEUR;

		for(uint8_t i = 0; i < nb_cartons; i++)
		{
			command.value |= ACT_DISTRIBUTION_CARTONS;
			xQueueSendToBack(xCommandQueue, &command, 0);

	        sub_task1.sensor_msk = SEN_CARTON_DISTRIBUE;
	        sub_task1.sensor_state = 0x00000000;
	        xQueueSendToBack(xSubscribeQueue, &sub_task1, 0);

	        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			command.value &= ~ACT_DISTRIBUTION_CARTONS;
			xQueueSendToBack(xCommandQueue, &command, 0);

			sub_task1.sensor_msk = SEN_CARTON_DISTRIBUE;
			sub_task1.sensor_state = SEN_CARTON_DISTRIBUE;
			xQueueSendToBack(xSubscribeQueue, &sub_task1, 0);

			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			sub_task1.sensor_msk = SEN_CARTON_ENVOYE;
			sub_task1.sensor_state = 0x00000000;
			xQueueSendToBack(xSubscribeQueue, &sub_task1, 0);

			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			sub_task1.sensor_msk = SEN_CARTON_ENVOYE;
			sub_task1.sensor_state = SEN_CARTON_ENVOYE;
			xQueueSendToBack(xSubscribeQueue, &sub_task1, 0);

			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		}

		sub_task1.sensor_msk = SEN_ENTREE_PALETTISEUR;
		sub_task1.sensor_state = 0x00000000;
		xQueueSendToBack(xSubscribeQueue, &sub_task1, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		sub_task1.sensor_msk = SEN_ENTREE_PALETTISEUR;
		sub_task1.sensor_state = SEN_ENTREE_PALETTISEUR;
		xQueueSendToBack(xSubscribeQueue, &sub_task1, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		sub_task1.sensor_msk = SEN_ENTREE_PALETTISEUR;
		sub_task1.sensor_state = 0x00000000;
		xQueueSendToBack(xSubscribeQueue, &sub_task1, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		command.value &= ~ACT_BLOCAGE_ENTREE_PALETTISEUR;
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task1.sensor_msk = SEN_ENTREE_PALETTISEUR;
		sub_task1.sensor_state = SEN_ENTREE_PALETTISEUR;
		xQueueSendToBack(xSubscribeQueue, &sub_task1, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

/*************************************************************************/
	/* Task2 notifie que de nouveaux cartons peuvent être reçus */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}

/*
 *	Task_2
 *	Gestion de la partie regroupement des 6 cartons sur la porte
 *	Transmition des groupes à la tâche de palettisation (Task3)
 */
void vTask_2 (void *pvParameters)
{
	actuator_cmd_t command = {0};

	subscribe_message_t sub_task2;
	sub_task2.handle = vTask2_handle;

	command.mask = ACT_TOURNER_CARTON | ACT_POUSSOIR | ACT_CLAMP | ACT_PORTE | ACT_DECHARGER_PALETTISEUR;

	ulTaskNotifyTake(pdTRUE, 0);
	while(1)
	{
		for(uint8_t i = 0; i < nb_poussees; i++)
		{
			sub_task2.sensor_msk = SEN_BUTEE_CARTON | SEN_LIMITE_POUSSOIR;
			sub_task2.sensor_state = SEN_BUTEE_CARTON | SEN_LIMITE_POUSSOIR;
			xQueueSendToBack(xSubscribeQueue, &sub_task2, 0);

			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			command.value |= ACT_POUSSOIR;
			xQueueSendToBack(xCommandQueue, &command, 0);

/***************************************************************************/
	/* Notifie à Task1 que poussoir a eu lieu et que de nouveaux cartons peuvent être reçus */
			xTaskNotifyGive(vTask1_handle);

			sub_task2.sensor_msk = SEN_BUTEE_CARTON | SEN_LIMITE_POUSSOIR;
			sub_task2.sensor_state = SEN_LIMITE_POUSSOIR;
			xQueueSendToBack(xSubscribeQueue, &sub_task2, 0);

			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			if(i < nb_poussees-1)
			{
				command.value &= ~ACT_POUSSOIR;
				xQueueSendToBack(xCommandQueue, &command, 0);

				sub_task2.sensor_msk = SEN_LIMITE_POUSSOIR;
				sub_task2.sensor_state = 0x00000000;
				xQueueSendToBack(xSubscribeQueue, &sub_task2, 0);

				ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			}
		}

		command.value &= ~ACT_POUSSOIR;
		command.value |= ACT_CLAMP;
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task2.sensor_msk = SEN_LIMITE_PORTE | SEN_CLAMPED;
		sub_task2.sensor_state = SEN_LIMITE_PORTE | SEN_CLAMPED;
		xQueueSendToBack(xSubscribeQueue, &sub_task2, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		command.value |= ACT_PORTE;
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task2.sensor_msk = SEN_LIMITE_PORTE | SEN_CLAMPED;
		sub_task2.sensor_state = SEN_CLAMPED;
		xQueueSendToBack(xSubscribeQueue, &sub_task2, 0);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		sub_task2.sensor_msk = SEN_LIMITE_PORTE | SEN_CLAMPED;
		sub_task2.sensor_state = SEN_LIMITE_PORTE | SEN_CLAMPED;
		xQueueSendToBack(xSubscribeQueue, &sub_task2, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		xTaskNotifyGive(vTask3_handle);
		vTaskDelay(500); // Délai pour que les cartons aient le temps de tomber sur la palette et que l'ascenseur descende avant de fermer la porte

		command.value &= ~(ACT_PORTE | ACT_CLAMP);
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task2.sensor_msk = SEN_BUTEE_CARTON | SEN_LIMITE_POUSSOIR;
		sub_task2.sensor_state = SEN_LIMITE_POUSSOIR;
		xQueueSendToBack(xSubscribeQueue, &sub_task2, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}

/*
 *	Task_3
 *	Gestion du parcours des palettes
 *	Réception des cartons par groupe de 6 (Task2)
 *	Acheminement des palettes à la sortie du palettiseur
 *
 */
void vTask_3 (void *pvParameters)
{
	actuator_cmd_t command = {0};

	subscribe_message_t sub_task3;
	sub_task3.handle = vTask3_handle;

	command.mask = ACT_DISTRIBUTION_PALETTE | ACT_CHARGER_PALETTE | ACT_MONTER_ASCENSEUR | ACT_DESCENDRE_ASCENSEUR |
				   ACT_ASCENSEUR_TO_LIMIT | ACT_DECHARGER_PALETTE;

	ulTaskNotifyTake(pdTRUE, 0);
	while(1)
	{
		command.value |= ACT_DISTRIBUTION_PALETTE | ACT_CHARGER_PALETTE;
		xQueueSendToBack(xCommandQueue, &command, 0);

		// Délai pour désactiver l'actionneur après envoie de la palette
		vTaskDelay(50);
		// Ce délai aurait pû être remplacer par un capteur judicieusement placé au niveau de l'arrivée des palettes

		command.value &= ~ACT_DISTRIBUTION_PALETTE;
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task3.sensor_msk = SEN_ENTREE_PALETTE;
		sub_task3.sensor_state = SEN_ENTREE_PALETTE;
		xQueueSendToBack(xSubscribeQueue, &sub_task3, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		sub_task3.sensor_msk = SEN_ENTREE_PALETTE;
		sub_task3.sensor_state = 0x00000000;
		xQueueSendToBack(xSubscribeQueue, &sub_task3, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		command.value &= ~ACT_CHARGER_PALETTE;
		command.value |= ACT_ASCENSEUR_TO_LIMIT | ACT_MONTER_ASCENSEUR;
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task3.sensor_msk = SEN_ASCENSEUR_ETAGE_1 | SEN_ASCENSEUR_EN_MOUVEMENT;
		sub_task3.sensor_state = SEN_ASCENSEUR_ETAGE_1;
		xQueueSendToBack(xSubscribeQueue, &sub_task3, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		command.value &= ~(ACT_ASCENSEUR_TO_LIMIT | ACT_MONTER_ASCENSEUR);
		xQueueSendToBack(xCommandQueue, &command, 0);

/***************************************************************************/
	/* Task2 notifie que la porte est ouverte */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		command.value |= ACT_DESCENDRE_ASCENSEUR;
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task3.sensor_msk = SEN_ASCENSEUR_ETAGE_1 | SEN_ASCENSEUR_EN_MOUVEMENT;
		sub_task3.sensor_state = 0x00000000;
		xQueueSendToBack(xSubscribeQueue, &sub_task3, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

/***************************************************************************/
	/* Task2 notifie que la porte est ouverte */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		command.value |= ACT_ASCENSEUR_TO_LIMIT;
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task3.sensor_msk = SEN_ASCENSEUR_ETAGE_RDC | SEN_ASCENSEUR_EN_MOUVEMENT;
		sub_task3.sensor_state = SEN_ASCENSEUR_ETAGE_RDC;
		xQueueSendToBack(xSubscribeQueue, &sub_task3, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		command.value &= ~(ACT_ASCENSEUR_TO_LIMIT | ACT_DESCENDRE_ASCENSEUR);
		command.value |= ACT_CHARGER_PALETTE;
		xQueueSendToBack(xCommandQueue, &command, 0);

		sub_task3.sensor_msk = SEN_SORTIE_PALETTE;
		sub_task3.sensor_state = 0x00000000;
		xQueueSendToBack(xSubscribeQueue, &sub_task3, 0);

		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	}
}

/*
 * Task_Write
 * Fait le force update
 * Active tous les actionneurs non commandes par les tâches
 * Traite les commandes envoyées par les tâches et les transmet à Factory_IO
 */
void vTask_Write (void *pvParameters)
{
    static uint32_t actuators_state = 0;
    actuator_cmd_t command;

    NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 2);
	NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);

	tx_dma_buffer[0] = TAG_UPDATE;
	tx_dma_buffer[1] = '\n';

	DMA1_Channel4->CNDTR = 2;
	DMA1_Channel4->CCR |= DMA_CCR_EN;
	USART2->CR3 |= USART_CR3_DMAT;

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	DMA1_Channel4->CCR &= ~DMA_CCR_EN;
	USART2->CR3 &= ~USART_CR3_DMAT;

	actuators_state = ACT_TAPIS_DISTRIBUTION_CARTONS | ACT_TAPIS_CARTON_VERS_PALETTISEUR | ACT_CHARGER_PALETTISEUR |
					  ACT_TAPIS_DISTRIBUTION_PALETTE | ACT_TAPIS_PALETTE_VERS_ASCENSEUR | ACT_TAPIS_FIN | ACT_REMOVER;

	tx_dma_buffer[0] = TAG_ACTUATORS;
	tx_dma_buffer[1] = (uint8_t)(actuators_state & 0x000000FF);
	tx_dma_buffer[2] = (uint8_t)((actuators_state & 0x0000FF00) >> 8);
	tx_dma_buffer[3] = (uint8_t)((actuators_state & 0x00FF0000) >> 16);
	tx_dma_buffer[4] = (uint8_t)((actuators_state & 0xFF000000) >> 24);
	tx_dma_buffer[5] = 0x00;
	tx_dma_buffer[6] = '\n';

	DMA1_Channel4->CNDTR = FRAME_LENGTH_ACT;
	DMA1_Channel4->CCR |= DMA_CCR_EN;
	USART2->CR3 |= USART_CR3_DMAT;

	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	DMA1_Channel4->CCR &= ~DMA_CCR_EN;
	USART2->CR3 &= ~USART_CR3_DMAT;

    while(1)
    {
    	// Réception des commandes
        xQueueReceive(xCommandQueue, &command, portMAX_DELAY);

        actuators_state = (actuators_state & ~command.mask) | (command.value & command.mask);

        // Préparation du buffer pour envoie DMA à Factory_IO
        tx_dma_buffer[0] = TAG_ACTUATORS;
        tx_dma_buffer[1] = (uint8_t) (actuators_state & 0x000000FF);
        tx_dma_buffer[2] = (uint8_t)((actuators_state & 0x0000FF00) >>8U );
        tx_dma_buffer[3] = (uint8_t)((actuators_state & 0x00FF0000) >>16U);
        tx_dma_buffer[4] = (uint8_t)((actuators_state & 0xFF000000) >>24U);
        tx_dma_buffer[5] = 0x00;
        tx_dma_buffer[6] = '\n';

        DMA1_Channel4->CNDTR = FRAME_LENGTH_ACT;
        DMA1_Channel4->CCR |= DMA_CCR_EN;
        USART2->CR3 |= USART_CR3_DMAT;

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        DMA1_Channel4->CCR &= ~DMA_CCR_EN;
        USART2->CR3 &= ~USART_CR3_DMAT;
    }
}

/*
 * Task_Read
 * Gère la réception des abonnements
 * Envoie une notification quand les abonnemnts et l'état réel des capteurs matchent
 */
void vTask_Read (void *pvParameters)
{
    subscribe_message_t msg;
    subscribe_message_t subscription_table[nb_task] = {0};

    uint32_t sensor_buffer = 0;

    NVIC_SetPriority(USART2_IRQn, 1);
	NVIC_EnableIRQ(USART2_IRQn);

    while(1)
    {
        // Traitement de tous les abonnements
        while(xQueueReceive(xSubscribeQueue, &msg, 0) == pdTRUE)
        {
            uint8_t free_slot = nb_task;
            for(uint8_t i = 0; i < nb_task; i++)
            {
                if(subscription_table[i].handle == NULL)
                {
                    free_slot = i;
                    break;
                }
            }

            if(free_slot < nb_task)
            {
                subscription_table[free_slot].handle = msg.handle;
                subscription_table[free_slot].sensor_msk = msg.sensor_msk;
                subscription_table[free_slot].sensor_state = msg.sensor_state;
            }
            else
            {
                my_printf("ERREUR : Table pleine !\r\n");
            }
        }

        // Attente de la notification UART IDLE
        if(ulTaskNotifyTake(pdTRUE, 100) == pdTRUE)
        {
        	// Lecture du buffer rx
			sensor_buffer = (uint32_t)(rx_dma_buffer[1]) |
							(uint32_t)(rx_dma_buffer[2] <<8U) |
							(uint32_t)(rx_dma_buffer[3] <<16U) |
							(uint32_t)(rx_dma_buffer[4] <<24U);

			// Vérification des abonnements
			for (uint8_t i = 0; i < nb_task; i++)
			{
				if (subscription_table[i].handle != NULL)
				{
					if ((sensor_buffer & subscription_table[i].sensor_msk) ==
						(subscription_table[i].sensor_state & subscription_table[i].sensor_msk))
					{
						xTaskNotifyGive(subscription_table[i].handle);

						// Suppression des abonnments après envoie de la notif
						subscription_table[i].handle = NULL;
						subscription_table[i].sensor_msk = 0;
						subscription_table[i].sensor_state = 0;
					}
				}
			}
    	}
    }
}

/*
 * Clock configuration for the Nucleo STM32F072RB board
 * HSE input Bypass Mode            -> 8MHz
 * SYSCLK, AHB, APB1                -> 48MHz
 * PA8 as MCO with /16 prescaler    -> 3MHz
 */

//static void SystemClock_Config()
//{
//	uint32_t	HSE_Status;
//	uint32_t	PLL_Status;
//	uint32_t	SW_Status;
//	uint32_t	timeout = 0;
//	timeout = 1000000;
//
//	// Start HSE in Bypass Mode
//	RCC->CR |= RCC_CR_HSEBYP;
//	RCC->CR |= RCC_CR_HSEON;
//
//	// Wait until HSE is ready
//	do
//	{
//		HSE_Status = RCC->CR & RCC_CR_HSERDY_Msk;
//		timeout--;
//	} while ((HSE_Status == 0) && (timeout > 0));
//
//	// Select HSE as PLL input source
//	RCC->CFGR &= ~RCC_CFGR_PLLSRC_Msk;
//	RCC->CFGR |= (0x02 <<RCC_CFGR_PLLSRC_Pos);
//
//	// Set PLL PREDIV to /1
//	RCC->CFGR2 = 0x00000000;
//
//	// Set PLL MUL to x6
//	RCC->CFGR &= ~RCC_CFGR_PLLMUL_Msk;
//	RCC->CFGR |= (0x04 <<RCC_CFGR_PLLMUL_Pos);
//
//	// Enable the main PLL
//	RCC-> CR |= RCC_CR_PLLON;
//	// Wait until PLL is ready
//	do
//	{
//		PLL_Status = RCC->CR & RCC_CR_PLLRDY_Msk;
//		timeout--;
//	} while ((PLL_Status == 0) && (timeout > 0));
//
//	// Set AHB prescaler to /1
//	RCC->CFGR &= ~RCC_CFGR_HPRE_Msk;
//	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
//// RCC_CFGR_HPRE_DIV1 -> 48MHz		RCC_CFGR_HPRE_DIV16 -> 3MHz		RCC_CFGR_HPRE_DIV64 -> 750kHz
//// RCC_CFGR_HPRE_DIV128 -> 375kHz	RCC_CFGR_HPRE_DIV256 -> 187.5kHz => ne marche plus à cette fréquence
//
//	//Set APB1 prescaler to /1
//	RCC->CFGR &= ~RCC_CFGR_PPRE_Msk;
//	RCC->CFGR |= RCC_CFGR_PPRE_DIV1;
//
//	// Enable FLASH Prefetch Buffer and set Flash Latency
//	FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
//
//	// Select the main PLL as system clock source
//	RCC->CFGR &= ~RCC_CFGR_SW;
//	RCC->CFGR |= RCC_CFGR_SW_PLL;
//
//	// Wait until PLL becomes main switch input
//	do
//	{
//		SW_Status = (RCC->CFGR & RCC_CFGR_SWS_Msk);
//		timeout--;
//	} while ((SW_Status != RCC_CFGR_SWS_PLL) && (timeout > 0));
//
//	// Update SystemCoreClock global variable
//	SystemCoreClockUpdate();
//}
