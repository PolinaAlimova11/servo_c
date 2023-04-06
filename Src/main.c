
#include <stdint.h>
#include "main.h"

bool PWM_state = true;	// "1" или "0" шим сигнала
uint8_t Servo_state = 0;	// угол поворота сервопривода
uint16_t PWM_period_0, PWM_period_1;	//длительность сигналов шим

char RxBuffer[256];	//создаем буффер для принятых данных.
char TxBuffer[256];	//создаем буффер для отправляемых данных.
bool CommandReceived = false;	//булевай переменная, означающая прием строки


void TIM2_IRQHandler(void)
{
	PWM_period_1 = 30+Servo_state/0.9;		// задаем период единицы в шим
	PWM_period_0 = 2000-PWM_period_1;		// задаем период нуля в шим


	//Смена модуля счета для шим
	if (PWM_state)
	{
		PWM_0(); // меняем состояние выхода на шим
		TIM2->ARR = PWM_period_0 - 1;	//меняем модуль счета
		PWM_state = false;
	}

	else
	{
		PWM_1(); // меняем состояние выхода на шим
		TIM2->ARR = PWM_period_1 - 1;
		PWM_state = true;
	}

	TIM2->CNT = 0;		//зануляем счетный регистр таймера
	TIM2->SR &=~ TIM_SR_UIF; // сбрасываем флаг прерывания

}

void USART2_IRQHandler (void)
{
	if ((USART2->SR & USART_SR_RXNE)!= 0)	//проверяем, про прерывание произошло по приему данных
	{
		uint8_t ptr = strlen(RxBuffer);		//вычисляем позицию первой свободной ячейки
		RxBuffer[ptr] = USART2->DR;			//записываем в найденную ячейку полученный байт данных (символ)

		if (ptr>0){		//проверка, чтобы не было ошибки при чтении индекса -1
			if (RxBuffer[ptr]==0x0D)		//проверяем последний
			{
				CommandReceived = true;		//если они подходят, то "поднимаем" флаг
				return;
			}
		}
	}
}

void delay(uint32_t delay_val)		// функция задержки
{
	for(uint32_t i = 0; i < delay_val; i++); // пустой цикл для задержки
}

void EXTI15_10_IRQHandler(void)
{
	// сначала проверим, от какого источника произошло прерывание
	if (EXTI->PR & EXTI_PR_PR13)	// проверяем, записана ли "1" в 13 бит, то есть произошло ли
		// прерывание по 13 линии
	{
		if (Servo_state == 0){		// меняем положение серво
			Servo_state = 180;
		}
		else
		{
			Servo_state = 0;
		}
		delay(1000);	// задержка от дребезга кнопки
		EXTI->PR |= EXTI_PR_PR13;	//сбрасываем флаг прерывания, записывая 1.
	}
}

void init_port(void)		// инициализация порта на сервопривод
{
	//выход шим на PA6
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; //включаем тактирование на шину

	GPIOA->CRL &=~ GPIO_CRL_CNF6|GPIO_CRL_MODE6;	//зануляем поле CNF
	GPIOA->CRL |= GPIO_CRL_MODE6;	// записываем все 11 в поле mode
	// настраиваем на выход 00-11, push-pull на 50 КГц
}

void init_button(void)		// инициализация порта на кнопку
{
	//PC13
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN; //включаем тактирование на шину
	// также включаем тактирование альтернативной функции для прерывания по кнопке


	GPIOC->CRH &=~ (GPIO_CRH_CNF13|GPIO_CRH_MODE13);	// зануляем все биты кнопки
	GPIOC->CRH |= GPIO_CRH_CNF13_1;		// выставляем первый бит в 1
	// режим кнопки 10-00 вход с подтяжкой
	GPIOC->BSRR = GPIO_BSRR_BS13; // выставляем "1" в ODR с помощью регистра BSRR, чтобы обозначить, что
	// подтяжка к земле

	//EXTI13

	EXTI->IMR |= EXTI_IMR_MR13;	// выставляем маску на 13 бит
	EXTI->FTSR |= EXTI_FTSR_TR13;	// выставляем срабатывание по спаду импульса

	AFIO->EXTICR[3] |= AFIO_EXTICR4_EXTI13_PC;	// в 4 регистр (в массиве из 4 элементов он имеет 3 номер
		// записываем необходимый бит с помощью констант
		// какой порт нам необходимо считывать по 13 линии, порт С

	NVIC_EnableIRQ(EXTI15_10_IRQn);	// разрешаем прерывание по линии 10-15, где у нас обработчик прерываний
	NVIC_SetPriority(EXTI15_10_IRQn, 0);	// выставляем приоритет прерывнию

}
void initClk(void)
{
	// Enable HSI
	RCC->CR |= RCC_CR_HSION;
	while(!(RCC->CR & RCC_CR_HSIRDY)){};

	// Enable Prefetch Buffer
	FLASH->ACR |= FLASH_ACR_PRFTBE;

	// Flash 2 wait state
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	FLASH->ACR |= FLASH_ACR_LATENCY_2;

	// HCLK = SYSCLK
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

	// PCLK2 = HCLK
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

	// PCLK1 = HCLK
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

	// PLL configuration: PLLCLK = HSI/2 * 16 = 64 MHz
	RCC->CFGR &= ~RCC_CFGR_PLLSRC;
	RCC->CFGR |= RCC_CFGR_PLLMULL16;

	// Enable PLL
	RCC->CR |= RCC_CR_PLLON;

	// Wait till PLL is ready
	while((RCC->CR & RCC_CR_PLLRDY) == 0) {};

	// Select PLL as system clock source
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;

	// Wait till PLL is used as system clock source
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){};
}

void init_tim2(void)	// инициализируем таймер
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;	// включаем тактирование шины

	//Частота APB1 для таймеров = APB1Clk * 2 = 32МГц * 2 = 64МГц
	TIM2-> PSC = 640 - 1;	// выставляем предделитель частота 10 КГц
	TIM2 -> ARR = 50-1;	// выставляем значение, до которого считает таймер

	TIM2->DIER |= TIM_DIER_UIE;		// сначала разрешаем прерывание
	TIM2->CR1 |= TIM_CR1_CEN; // включаем таймер

	NVIC_EnableIRQ(TIM2_IRQn); // разрешаем прерывание по таймеру в системе
	NVIC_SetPriority(TIM2_IRQn, 1);		// настраиваем приоритет, он ниже, чем у кнопки

}

void init_uart2(void)		//инициализация usart2
{
	// PA2, PA3 USART - синхронный/асинхронный, но мы используем асинхронный
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;	//разрешили тактирование на шине
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;		//включаем тактирование альтернативных функций
	//порта для пинов USART2
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;		//включили тактирование порта А

	//PA2 - выход, PA3 - вход

	GPIOA->CRL &=~ (GPIO_CRL_CNF2 | GPIO_CRL_MODE2);		//занулили биты 2 пина, чтобы избежать ошибки
	GPIOA->CRL |= (GPIO_CRL_CNF2_1 | GPIO_CRL_MODE2_1);		//выставляем единицы в первые биты
	// то есть выбираем 10-10, что означает альтернативный выход пуш-пул и частота 2 МГц

	GPIOA->CRL &=~ (GPIO_CRL_CNF3 | GPIO_CRL_MODE3);		//занулили биты 3 пина, чтобы избежать ошибки
	GPIOA->CRL |= GPIO_CRL_CNF3_0;		//выставляем единицу в нулевой бит
		// то есть выбираем 01-00, что означает плавающий вход

	USART2->BRR = 0x8B;		//записали скорость 230400 бот
	//это число вычисляется по референц мануалу

	USART2->CR1 |= USART_CR1_UE |USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
	//разрешаем работу usart2, работу приемника, работу передатчика и прерывание по приему данных

	NVIC_EnableIRQ(USART2_IRQn); // разрешаем системе прерывания по линии usart2
}

void txStr(char *str, bool crlf)	//функция отправки данных по usart, имеет указатель на строку,
//которую нужно отправить и булевую переменную, отвечающую за символы конца строки
{
	if (crlf)
		strcat(str, "\r");	//добавляем в конец символы конца строки
	for(uint8_t i = 0; i<strlen(str); i++)
	{
		USART2->DR = str[i];		//передаем байт данных
		while((USART2->SR & USART_SR_TC)== 0){};	//ждём, когда закончится передача символа
		// когда в регистре выставится флаг, что отправка завершена

	}
}

void ExecuteCom(void)		//функция обработки команд
{
	memset(TxBuffer, 0, 256);	//зануляем буфер выходных данных

	if((strncmp(RxBuffer, "*IDN?", 5))==0)		//сравниваем, что в буффере с командой
	{
		strcpy(TxBuffer, "Alimova P. D. Obrubov S. I. IU4-71B");		//хаписываем в буфер вывода строку
	}

	else if ((strncmp(RxBuffer, "ANGLE ", 6))==0)
	{
		//если пришла эта команда, то должны поменять период таймера на заданное число
		//ANGLE 0
		//ANGLE 180
		//нужно вытащить значение числовое

		uint16_t value = -1;		//переменная для заданного поворота
		sscanf(RxBuffer, "%*s %hu", &value);	//преобразуем строку в число

		if ((0<=value)&&(value<=180))		//проверяем, правильное ли число ввел пользователь
		{
			Servo_state = value;		// присваиваем значение угла
			strcpy(TxBuffer, "OK");		//возвращаем строку, подтвержающую выполнение команды
		}
		else
		{
			strcpy(TxBuffer, "Invalid angle");		//возвращаем строку, подтвержающую выполнение команды
		}

	}
	else if ((strncmp(RxBuffer, "ANGLE?", 6))==0)
	{
		char servo_state_string[5];		//переменная для символьной записи числа
		itoa(Servo_state, servo_state_string, 10);		// изменяем тип с численного на символьный
		// 10 - система счисления
		strcpy(TxBuffer, servo_state_string);	//записываем в буфер отправки
	}
	else if ((strncmp(RxBuffer, "SET LEFT", 8))==0)
	{
		Servo_state = 0;		//выставляем угол поворота 0
		strcpy(TxBuffer, "OK");
	}
	else if ((strncmp(RxBuffer, "SET RIGHT", 9))==0)
	{
		Servo_state = 180;		// выставляем угол поворота 180
		strcpy(TxBuffer, "OK");
	}
	else	//если пришла неизвестная команда
	{
		strcpy(TxBuffer, "Unknown command");
	}

	txStr(TxBuffer, true);	//программа вывода данных из буфера выходных значений

	memset(RxBuffer, 0, 256);	//зануляем буфер входных данных
	CommandReceived = false;	//убираем флаг работы этой функции, чтобы повторно не отправлять данные

}






int main(void)
{
	initClk();
	init_uart2();
	init_button();
	init_port();	//порт шим
	init_tim2();	//таймер на шим
	PWM_1();		//выставляем 1 на шиме в самом начале


    while(true)
    {
    	if (CommandReceived)	//если пришли символы конца строки
    	{
    		ExecuteCom();	//функция обработки строки
    	}
    }
}
