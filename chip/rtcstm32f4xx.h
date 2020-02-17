/* $Id$ */
// Проект HF Dream Receiver (КВ приёмник мечты)
// автор Гена Завидовский mgs2001@mail.ru
// UA1ARN

//
// Поддержка stm32f4xx internal real time clock
//
#ifndef RTCSTM32F4XX1_H_INCLUDED
#define RTCSTM32F4XX1_H_INCLUDED

// input value 0x00..0x99, return value 0..99
static uint_fast8_t 
stm32f4xx_bcd2bin(uint_fast8_t v)
{
	const div_t d = div(v, 16);
	return d.quot * 10 + d.rem;
}

// input value: 0..99, return value 0x00..0x99
static uint_fast8_t 
stm32f4xx_bin2bcd(uint_fast8_t v)
{
	const div_t d = div(v, 10);
	return d.quot * 16 + d.rem;
}

// Разрешить запись в Backup domain
static void
stm32f4xx_rtc_bdenable(void)
{
#if CPUSTYLE_STM32F4XX || CPUSTYLE_STM32F0XX

	PWR->CR |= PWR_CR_DBP;  
	while ((PWR->CR & PWR_CR_DBP) == 0)
		;

#else /* CPUSTYLE_STM32F4XX || CPUSTYLE_STM32F0XX */

	PWR->CR1 |= PWR_CR1_DBP;  
	while ((PWR->CR1 & PWR_CR1_DBP) == 0)
		;

#endif /* CPUSTYLE_STM32F4XX || CPUSTYLE_STM32F0XX */
}

// Запретить запись в Backup domain
static void
stm32f4xx_rtc_bddisable(void)
{
#if CPUSTYLE_STM32F4XX || CPUSTYLE_STM32F0XX

	PWR->CR &= ~ PWR_CR_DBP;	
	while ((PWR->CR & PWR_CR_DBP) != 0)
		;

#else /* CPUSTYLE_STM32F4XX || CPUSTYLE_STM32F0XX */

	PWR->CR1 &= ~ PWR_CR1_DBP;	
	while ((PWR->CR1 & PWR_CR1_DBP) != 0)
		;

#endif /* CPUSTYLE_STM32F4XX || CPUSTYLE_STM32F0XX */
}

// Запретить запись в RTC
static void
stm32f4xx_rtc_wrdisable(void)	
{
	RTC->WPR = 0xFF;
}

// Разрешить запись в RTC
static void
stm32f4xx_rtc_wrenable(void)	
{
	/* Disable the write protection for RTC registers */
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;
}


static void
stm32f4xx_rtc_lsXstart(void)
{
#if WITHRTCLSI
	/* use LSI as clock source ~40 kHz */
	const uint_fast32_t bcdrrtcsel = 
		2 * RCC_BDCR_RTCSEL_0 |		/* 10: LSI oscillator clock used as the RTC clock */
		0;

	if ((RCC->BDCR & RCC_BDCR_RTCSEL) != bcdrrtcsel)
	{
		RCC->BDCR |= RCC_BDCR_BDRST;
		RCC->BDCR &= ~ RCC_BDCR_BDRST;

		RCC->BDCR = (RCC->BDCR & ~ (RCC_BDCR_RTCSEL)) |
			bcdrrtcsel |
			0;

		RCC->CSR |= RCC_CSR_LSION;
		while ((RCC->CSR & RCC_CSR_LSIRDY) == 0)
			;
	}

	RCC->BDCR |= RCC_BDCR_RTCEN;	/* RTC clock enabled */
	while ((RCC->BDCR & RCC_BDCR_RTCEN) == 0)
		;

#else /* WITHRTCLSI */
	/* use LSE as clock source 32.768 kHz xtal */

	#if CPUSTYLE_STM32F4XX

		const uint_fast32_t bcdrrtcsel = 
			1 * RCC_BDCR_RTCSEL_0 |		/* 01: LSE oscillator clock used as the RTC clock */
			0;

		if ((RCC->BDCR & RCC_BDCR_RTCSEL) != bcdrrtcsel)
		{
			RCC->BDCR |= RCC_BDCR_BDRST;
			RCC->BDCR &= ~ RCC_BDCR_BDRST;

			RCC->BDCR = (RCC->BDCR & ~ (RCC_BDCR_RTCSEL)) |
				bcdrrtcsel |
				0;

			RCC->BDCR = (RCC->BDCR & ~ (RCC_BDCR_LSEBYP)) |
				1 * RCC_BDCR_LSEON |
				0;

			while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0)
				;

		}

		RCC->BDCR |= RCC_BDCR_RTCEN;	/* RTC clock enabled */
		while ((RCC->BDCR & RCC_BDCR_RTCEN) == 0)
			;

	#else /* CPUSTYLE_STM32F4XX */

		const uint_fast32_t bcdrrtcsel = 
			1 * RCC_BDCR_RTCSEL_0 |		/* 01: LSE oscillator clock used as the RTC clock */
			0;

		if ((RCC->BDCR & RCC_BDCR_RTCSEL) != bcdrrtcsel)
		{
			RCC->BDCR |= RCC_BDCR_BDRST;
			RCC->BDCR &= ~ RCC_BDCR_BDRST;

			RCC->BDCR = (RCC->BDCR & ~ (RCC_BDCR_RTCSEL)) |
				bcdrrtcsel |
				0;

			RCC->BDCR = (RCC->BDCR & ~ (RCC_BDCR_LSEDRV | RCC_BDCR_LSEBYP)) |
				3 * RCC_BDCR_LSEDRV_0 |
				1 * RCC_BDCR_LSEON |
				0;

			while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0)
				;

		}

		RCC->BDCR |= RCC_BDCR_RTCEN;	/* RTC clock enabled */
		while ((RCC->BDCR & RCC_BDCR_RTCEN) == 0)
			;

	#endif /* CPUSTYLE_STM32F4XX */

#endif /* WITHRTCLSI */
}

void board_rtc_settime(
	uint_fast8_t hours,
	uint_fast8_t minutes,
	uint_fast8_t secounds
	)
{
	stm32f4xx_rtc_bdenable();	// Разрешить запись в Backup domain
	stm32f4xx_rtc_wrenable();	/* Disable the write protection for RTC registers */
	/* INIT mode ON */
	RTC->ISR |= RTC_ISR_INIT;	
	while ((RTC->ISR & RTC_ISR_INITF) == 0)
		;

	RTC->TR =
		stm32f4xx_bin2bcd(hours) * RTC_TR_HU_0 |
		stm32f4xx_bin2bcd(minutes) * RTC_TR_MNU_0 |
		stm32f4xx_bin2bcd(secounds) * RTC_TR_SU_0 |
		0;
	
	stm32f4xx_rtc_wrdisable();	/* Enable the write protection for RTC registers */
	RTC->ISR &= ~ RTC_ISR_INIT;	// INIT mode OFF
	stm32f4xx_rtc_bddisable();	// Запретить запись в Backup domain
}

void board_rtc_setdate(
	uint_fast16_t year,
	uint_fast8_t month,
	uint_fast8_t dayofmonth
	)
{
	stm32f4xx_rtc_bdenable();	// Разрешить запись в Backup domain
	stm32f4xx_rtc_wrenable();	/* Disable the write protection for RTC registers */
	/* INIT mode ON */
	RTC->ISR |= RTC_ISR_INIT;	
	while ((RTC->ISR & RTC_ISR_INITF) == 0)
		;

	RTC->DR =
		stm32f4xx_bin2bcd(year - 2000) * RTC_DR_YU_0 |
		stm32f4xx_bin2bcd(month) * RTC_DR_MU_0 |
		stm32f4xx_bin2bcd(dayofmonth) * RTC_DR_DU_0 |
		0;
	stm32f4xx_rtc_wrdisable();	/* Enable the write protection for RTC registers */
	RTC->ISR &= ~ RTC_ISR_INIT;	// INIT mode OFF
	stm32f4xx_rtc_bddisable();	// Запретить запись в Backup domain
}

void board_rtc_setdatetime(
	uint_fast16_t year,
	uint_fast8_t month,
	uint_fast8_t dayofmonth,
	uint_fast8_t hours,
	uint_fast8_t minutes,
	uint_fast8_t secounds
	)
{
	stm32f4xx_rtc_bdenable();	// Разрешить запись в Backup domain
	/* Disable the write protection for RTC registers */
	stm32f4xx_rtc_wrenable();
	/* INIT mode ON */
	RTC->ISR |= RTC_ISR_INIT;	
	while ((RTC->ISR & RTC_ISR_INITF) == 0)
		;

	RTC->DR =
		stm32f4xx_bin2bcd(year - 2000) * RTC_DR_YU_0 |
		stm32f4xx_bin2bcd(month) * RTC_DR_MU_0 |
		stm32f4xx_bin2bcd(dayofmonth) * RTC_DR_DU_0 |
		0;
	RTC->TR =
		stm32f4xx_bin2bcd(hours) * RTC_TR_HU_0 |
		stm32f4xx_bin2bcd(minutes) * RTC_TR_MNU_0 |
		stm32f4xx_bin2bcd(secounds) * RTC_TR_SU_0 |
		0;
	stm32f4xx_rtc_wrdisable();	/* Enable the write protection for RTC registers */
	RTC->ISR &= ~ RTC_ISR_INIT;	// INIT mode OFF
	stm32f4xx_rtc_bddisable();	// Запретить запись в Backup domain
}

void board_rtc_getdate(
	uint_fast16_t * year,
	uint_fast8_t * month,
	uint_fast8_t * dayofmonth
	)
{
	const uint32_t dr = RTC->DR;

	* year = stm32f4xx_bcd2bin((dr & (RTC_DR_YT | RTC_DR_YU)) / RTC_DR_YU_0) + 2000;
	* month = stm32f4xx_bcd2bin((dr & (RTC_DR_MT | RTC_DR_MU)) / RTC_DR_MU_0);
	* dayofmonth = stm32f4xx_bcd2bin((dr & (RTC_DR_DT | RTC_DR_DU)) / RTC_DR_DU_0);
}

void board_rtc_gettime(
	uint_fast8_t * hour,
	uint_fast8_t * minute,
	uint_fast8_t * secounds
	)
{
	const uint32_t tr = RTC->TR;

	* hour = stm32f4xx_bcd2bin((tr & (RTC_TR_HT | RTC_TR_HU)) / RTC_TR_HU_0);
	* minute = stm32f4xx_bcd2bin((tr & (RTC_TR_MNT | RTC_TR_MNU)) / RTC_TR_MNU_0);
	* secounds = stm32f4xx_bcd2bin((tr & (RTC_TR_ST | RTC_TR_SU)) / RTC_TR_SU_0);
}

void board_rtc_getdatetime(
	uint_fast16_t * year,
	uint_fast8_t * month,	// 01-12
	uint_fast8_t * dayofmonth,
	uint_fast8_t * hour,
	uint_fast8_t * minute,
	uint_fast8_t * secounds
	)
{
	uint32_t dr2;
	uint32_t tr2;
	uint32_t dr = RTC->DR;
	uint32_t tr = RTC->TR;
	/* wait for TR & DR registers coherent */
	do
	{
		dr2 = dr;
		tr2 = tr;
		dr = RTC->DR;
		tr = RTC->TR;
	} while ((dr != dr2) || (tr != tr2));

	* year = stm32f4xx_bcd2bin((dr & (RTC_DR_YT | RTC_DR_YU)) / RTC_DR_YU_0) + 2000;
	* month = stm32f4xx_bcd2bin((dr & (RTC_DR_MT | RTC_DR_MU)) / RTC_DR_MU_0);
	* dayofmonth = stm32f4xx_bcd2bin((dr & (RTC_DR_DT | RTC_DR_DU)) / RTC_DR_DU_0);
	* hour = stm32f4xx_bcd2bin((tr & (RTC_TR_HT | RTC_TR_HU)) / RTC_TR_HU_0);
	* minute = stm32f4xx_bcd2bin((tr & (RTC_TR_MNT | RTC_TR_MNU)) / RTC_TR_MNU_0);
	* secounds = stm32f4xx_bcd2bin((tr & (RTC_TR_ST | RTC_TR_SU)) / RTC_TR_SU_0);
}

/* возврат не-0 если требуется начальная загрузка значений */
uint_fast8_t board_rtc_chip_initialize(void)
{
	debug_printf_P(PSTR("rtc_stm32f4xx_initialize\n"));

	// RCC_APB1ENR_RTCAPBEN ???
#if defined(RCC_APB1ENR_RTCEN)
	RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_RTCEN;  // Включить тактирование
	__DSB();
#elif defined(RCC_APB1ENR_PWREN)
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;  // Включить тактирование
	__DSB();
#else

#endif /* defined(RCC_APB1ENR_RTCEN) */

	//RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;  // STM32F1xx: Включить тактирование
	//__DSB();

	stm32f4xx_rtc_bdenable();	// Разрешить запись в Backup domain

#if 0

	// Sample for STM32F207 from http://electronix.ru/forum/index.php?showtopic=134999&view=findpost&p=1421489

	/* PWR */

	PWR->CSR =
		PWR_CSR_EWUP * 0 |		// Enable WKUP pin
		PWR_CSR_BRE * 1 |		// Backup regulator enable
		0;

	/* Теперь можно включить НЧ генератор */
	RCC->BDCR =
		RCC_BDCR_LSEON * 1 |    // External low-speed oscillator enable
		// RCC_BDCR_LSERDY * 0 |
		RCC_BDCR_LSEBYP * 0 |   // External low-speed oscillator bypass
		RCC_BDCR_RTCSEL_0 * 1 |	// RTC clock source selection: LSE
		RCC_BDCR_RTCEN * 1 |    // RTC clock enable
		RCC_BDCR_BDRST * 0 |    // Backup domain software reset
		0;

#endif

	stm32f4xx_rtc_lsXstart();	/* use LSE as clock source 32.768 kHz xtal, test: use LSI as clock source */
	stm32f4xx_rtc_wrenable();	/* Disable the write protection for RTC registers */


	RTC->CR |= RTC_CR_BYPSHAD;	// 1 - direct accesss (0 - acess to shadow registers)
	local_delay_ms(10);
    /* Configure the RTC PRER */
    RTC->PRER =
		((256 - 1) & RTC_PRER_PREDIV_S) |
		(((128 - 1) << 16) & RTC_PRER_PREDIV_A) |
		0;

	stm32f4xx_rtc_wrdisable();	/* Enable the write protection for RTC registers */
	stm32f4xx_rtc_bddisable();	// Запретить запись в Backup domain

	/* Установка начальных значений времени и даты */
	const uint_fast8_t inits = (RTC->ISR & RTC_ISR_INITS) == 0;	// if year is zero
	debug_printf_P(PSTR("rtc_stm32f4xx_initialize: INITS=%d\n"), inits);
	debug_printf_P(PSTR("rtc_stm32f4xx_initialize: done\n"));

	return inits;	/* возврат не-0 если требуется начальная загрузка значений */
}

#endif /* RTCSTM32F4XX1_H_INCLUDED */
