
#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f1xx.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define DELAY_VAL 100000


#define PWM_1() GPIOA->BSRR = GPIO_BSRR_BS6; // макрос для включения 1 шима
#define PWM_0() GPIOA->BSRR = GPIO_BSRR_BR6; // макрос для включения 0 шима
#define PWM_SWAP() GPIOA->ODR ^= GPIO_ODR_ODR6; // макрос изменения состояния шим

//прототипы функций
void delay(uint32_t delay_val);
void init_port(void);
void init_button(void);
void initClk(void);
void init_tim2(void);
void txStr(char *str, bool crlf);
void ExecuteCom(void);


#endif /* MAIN_H_ */
