#include "Bsp_COMP/Bsp_COMP.h"

COMP_HandleTypeDef  hcomp2;


/**
 * @brief       ADC 初始化配置
 * @param       htim:定时器句柄
 * @retval      无
 */
void COMP_Init(void)
{
	hcomp2.Instance = COMP2;                                            /* COMP2 */
  hcomp2.Init.InputMinus      = COMP_INPUT_MINUS_IO2;                 /* Minus Input PA4 */
  hcomp2.Init.InputPlus       = COMP_INPUT_PLUS_IO2;                  /* Plus Input VrefCmp */
  hcomp2.Init.OutputPol       = COMP_OUTPUTPOL_NONINVERTED;           /* Output None Inverted */
  hcomp2.Init.WindowMode      = COMP_WINDOWMODE_DISABLE;              /* Window mode Disable */
  hcomp2.Init.VrefSrc         = COMP_VREFCMP_SOURCE_VCC;              /* VrefCmp Select VCC */
  hcomp2.Init.VrefDiv         = COMP_VREFCMP_DIV_8_16VREFCMP;         /* Vrefbuf 8/16  */
  hcomp2.Init.DigitalFilter   = 0x0;                                  /* Filter Disable */
  hcomp2.Init.TriggerMode     = COMP_TRIGGERMODE_IT_RISING_FALLING;   /* Trigger on Rising/Falling */
	
	HAL_COMP_Init(&hcomp2);
	
	HAL_COMP_Start(&hcomp2);
}
/**
 * @brief       HAL_COMP_MspInit
 * @param       htim:定时器句柄
 * @retval      无
 */
void HAL_COMP_MspInit(COMP_HandleTypeDef *hcomp)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();                 /* Enable GPIOA Clock */
  __HAL_RCC_COMP1_CLK_ENABLE();                 /* Enable COMP1 Clock */    
  __HAL_RCC_COMP2_CLK_ENABLE();                 /* Enable COMP2 Clock */  

  /* Configure PA3 Analog */
  GPIO_InitTypeDef COMPINPUT;
  COMPINPUT.Pin = GPIO_PIN_3;
  COMPINPUT.Mode = GPIO_MODE_ANALOG;            /* Analog Mode */
  COMPINPUT.Speed = GPIO_SPEED_FREQ_HIGH;
  COMPINPUT.Pull = GPIO_PULLDOWN;               /* Pulldown */
  HAL_GPIO_Init(GPIOA,  &COMPINPUT);            /* GPIO Init */
  
  HAL_NVIC_SetPriority(ADC_COMP_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC_COMP_IRQn);       
}



/**
  * @brief  COMP Trigger CallBack
  * @param  None
  * @retval None
  */
void HAL_COMP_TriggerCallback(COMP_HandleTypeDef *hcomp)
{
  if(HAL_COMP_GetOutputLevel(hcomp)==1)
  {
    LED4(LED_OFF);
  }
  else
  {
    LED4(LED_ON);
  }
}
