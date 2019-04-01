/* $Id$ */
//
// ������ HF Dream Receiver (�� ������� �����)
// ����� ���� ����������� mgs2001@mail.ru
// UA1ARN
//

#include "hardware.h"
#include "board.h"
#include "audio.h"

#include "display.h"
#include "formats.h"

#include <string.h>
#include <math.h>

#define WITHPLACEHOLDERS 1	//  ����������� ������ � ��� ���������� ������

// todo: switch off -Wunused-function

#if WITHDIRECTFREQENER
struct editfreq
{
	uint_fast32_t freq;
	uint_fast8_t blinkpos;		// ������� (������� 10) �������������� �������
	uint_fast8_t blinkstate;	// � ����� �������������� ������� ������������ ������������� (0 - ������)
};
#endif /* WITHDIRECTFREQENER */

// ������������ ������ ������� ��� ������������ �����������
// ������� ��� ��������
static void dsp_latchwaterfall(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	);
static void display2_spectrum(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	);
static void display2_waterfall(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	);
static void display2_colorbuff(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	);
// ����������� ����� S-����� � ������ �����������
static void display2_legend(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	);
// ����������� ����� S-�����
static void display2_legend_rx(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	);
// ����������� ����� SWR-����� � ������ ����������
static void display2_legend_tx(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	);

//#define WIDEFREQ (TUNE_TOP > 100000000L)

static const FLASHMEM int32_t vals10 [] =
{
	1000000000UL,
	100000000UL,
	10000000UL,
	1000000UL,
	100000UL,
	10000UL,
	1000UL,
	100UL,
	10UL,
	1UL,
};

// ����������� ���� � ���� "������� ����" - ��������� �������� ������� ��������� ��������.
static void 
NOINLINEAT
display_value_big(
	uint_fast32_t freq,
	uint_fast8_t width, // = 8;	// full width
	uint_fast8_t comma, // = 2;	// comma position (from right, inside width)
	uint_fast8_t comma2,	// = comma + 3;		// comma position (from right, inside width)
	uint_fast8_t rj,	// = 1;		// right truncated
	uint_fast8_t blinkpos,		// �������, ��� ������ ������ ��������
	uint_fast8_t blinkstate,	// 0 - ������, 1 - ������
	uint_fast8_t withhalf,		// 0 - ������ ������� �����
	uint_fast8_t lowhalf		// lower half
	)
{
	//const uint_fast8_t comma2 = comma + 3;		// comma position (from right, inside width)
	const uint_fast8_t j = (sizeof vals10 /sizeof vals10 [0]) - rj;
	uint_fast8_t i = (j - width);
	uint_fast8_t z = 1;	// only zeroes
	uint_fast8_t half = 0;	// ���������� ����� ������ ������� - ��������� �������

	display_wrdatabig_begin();
	for (; i < j; ++ i)
	{
		const ldiv_t res = ldiv(freq, vals10 [i]);
		const uint_fast8_t g = (j - i);		// ���������� ������� �������� ������� �� �����������

		// ����������� �������� ��������
		if (comma2 == g)
		{
			display_put_char_big((z == 0) ? '.' : '#', lowhalf);	// '#' - ����� ������. ����� ������ �����
		}
		else if (comma == g)
		{
			z = 0;
			half = withhalf;
			display_put_char_big('.', lowhalf);
		}

		if (blinkpos == g)
		{
			const uint_fast8_t bc = blinkstate ? '_' : ' ';
			// ��� ������� �������������� �������. ������ �� �� �������� ��� ����
			z = 0;
			if (half)
				display_put_char_half(bc, lowhalf);

			else
				display_put_char_big(bc, lowhalf);
		}
		else if (z == 1 && (i + 1) < j && res.quot == 0)
			display_put_char_big(' ', lowhalf);	// supress zero
		else
		{
			z = 0;
			if (half)
				display_put_char_half('0' + res.quot, lowhalf);

			else
				display_put_char_big('0' + res.quot, lowhalf);
		}
		freq = res.rem;
	}
	display_wrdatabig_end();
}


static void
NOINLINEAT
display_value_small(
	int_fast32_t freq,
	uint_fast8_t width,	// full width (if >= 128 - display with sign)
	uint_fast8_t comma,		// comma position (from right, inside width)
	uint_fast8_t comma2,
	uint_fast8_t rj,		// right truncated
	uint_fast8_t lowhalf
	)
{
	const uint_fast8_t wsign = (width & WSIGNFLAG) != 0;
	const uint_fast8_t wminus = (width & WMINUSFLAG) != 0;
	const uint_fast8_t j = (sizeof vals10 /sizeof vals10 [0]) - rj;
	uint_fast8_t i = j - (width & WWIDTHFLAG);	// ����� ����� �� �������
	uint_fast8_t z = 1;	// only zeroes

	display_wrdata_begin();
	if (wsign || wminus)
	{
		// ����������� �� ������.
		z = 0;
		if (freq < 0)
		{
			display_put_char_small('-', lowhalf);
			freq = - freq;
		}
		else if (wsign)
			display_put_char_small('+', lowhalf);
		else
			display_put_char_small(' ', lowhalf);
	}
	for (; i < j; ++ i)
	{
		const ldiv_t res = ldiv(freq, vals10 [i]);
		const uint_fast8_t g = (j - i);
		// ����������� �������� ��������
		if (comma2 == g)
		{
			display_put_char_small((z == 0) ? '.' : ' ', lowhalf);
		}
		else if (comma == g)
		{
			z = 0;
			display_put_char_small('.', lowhalf);
		}

		if (z == 1 && (i + 1) < j && res.quot == 0)
			display_put_char_small(' ', lowhalf);	// supress zero
		else
		{
			z = 0;
			display_put_char_small('0' + res.quot, lowhalf);
		}
		freq = res.rem;
	}
	display_wrdata_end();
}


// ����������� �������. ����� ��� �� ������� �������.
static void display_freqXbig_a(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	uint_fast8_t rj;
	uint_fast8_t fullwidth = display_getfreqformat(& rj);
	const uint_fast8_t comma = 3 - rj;

	display_setcolors3(BIGCOLOR, BGCOLOR, BIGCOLORHALF);
	if (pv != NULL)
	{
#if WITHDIRECTFREQENER
		const struct editfreq * const efp = (const struct editfreq *) pv;


		uint_fast8_t lowhalf = HALFCOUNT_FREQA - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);
			display_value_big(efp->freq, fullwidth, comma, comma + 3, rj, efp->blinkpos, efp->blinkstate, 0, lowhalf);	// ������������ ������� ����� ������
		} while (lowhalf --);
#endif /* WITHDIRECTFREQENER */
	}
	else
	{
		enum { blinkpos = 255, blinkstate = 0 };

		const uint_fast32_t freq = hamradio_get_freq_a();

		uint_fast8_t lowhalf = HALFCOUNT_FREQA - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);
			display_value_big(freq, fullwidth, comma, comma + 3, rj, blinkpos, blinkstate, 0, lowhalf);	// ������������ ������� ����� ������
		} while (lowhalf --);
	}
}

// ����������� �������. ����� ��������� �������.
static void display_freqX_a(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	uint_fast8_t rj;
	uint_fast8_t fullwidth = display_getfreqformat(& rj);
	const uint_fast8_t comma = 3 - rj;

	display_setcolors3(BIGCOLOR, BGCOLOR, BIGCOLORHALF);
	if (pv != NULL)
	{
#if WITHDIRECTFREQENER
		const struct editfreq * const efp = (const struct editfreq *) pv;

		uint_fast8_t lowhalf = HALFCOUNT_FREQA - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);
			display_value_big(efp->freq, fullwidth, comma, comma + 3, rj, efp->blinkpos, efp->blinkstate, 1, lowhalf);	// ������������ ������� ����� ������
		} while (lowhalf --);
#endif /* WITHDIRECTFREQENER */
	}
	else
	{
		enum { blinkpos = 255, blinkstate = 0 };

		const uint_fast32_t freq = hamradio_get_freq_a();

		uint_fast8_t lowhalf = HALFCOUNT_FREQA - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);
			display_value_big(freq, fullwidth, comma, comma + 3, rj, blinkpos, blinkstate, 1, lowhalf);	// ������������ ������� ����� ������
		} while (lowhalf --);
	}
}

// ������� ����������� ��� ����� ����� ����������� � ������� �������� (��� ��������� ���������)
// FREQ B
static void display_freqchr_a(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	uint_fast8_t rj;
	uint_fast8_t fullwidth = display_getfreqformat(& rj);
	const uint_fast8_t comma = 3 - rj;

	display_setcolors3(BIGCOLOR, BGCOLOR, BIGCOLORHALF);
	if (pv != NULL)
	{
#if WITHDIRECTFREQENER
		const struct editfreq * const efp = (const struct editfreq *) pv;

		uint_fast8_t lowhalf = HALFCOUNT_FREQA - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);
			display_value_big(efp->freq, fullwidth, comma, 255, rj, efp->blinkpos, efp->blinkstate, 1, lowhalf);	// ������������ ������� ����� ������
		} while (lowhalf --);
#endif /* WITHDIRECTFREQENER */
	}
	else
	{
		enum { blinkpos = 255, blinkstate = 0 };

		const uint_fast32_t freq = hamradio_get_freq_a();

		uint_fast8_t lowhalf = HALFCOUNT_FREQA - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);
			display_value_big(freq, fullwidth, comma, 255, rj, blinkpos, blinkstate, 1, lowhalf);	// ������������ ������� ����� ������
		} while (lowhalf --);
	}
}

// ������� ����������� ��� ����� ����� ����������� � ������� �������� (��� ��������� ���������)
// FREQ B
static void display_freqchr_b(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	uint_fast8_t rj;
	uint_fast8_t fullwidth = display_getfreqformat(& rj);
	const uint_fast8_t comma = 3 - rj;

	display_setcolors(BIGCOLOR, BGCOLOR);
#if 0
	if (pv != NULL)
	{
#if WITHDIRECTFREQENER
		const struct editfreq * const efp = (const struct editfreq *) pv;

		uint_fast8_t lowhalf = HALFCOUNT_FREQA - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);
			display_value_big(efp->freq, fullwidth, comma, 255, rj, efp->blinkpos, efp->blinkstate, 1, lowhalf);	// ������������ ������� ����� ������
		} while (lowhalf --);
#endif /* WITHDIRECTFREQENER */
	}
	else
#endif
	{
		enum { blinkpos = 255, blinkstate = 0 };

		const uint_fast32_t freq = hamradio_get_freq_b();

		uint_fast8_t lowhalf = HALFCOUNT_FREQA - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);
			display_value_big(freq, fullwidth, comma, 255, 1, blinkpos, blinkstate, 1, lowhalf);	// ������������ ������� ����� ������
		} while (lowhalf --);
	}
}

static void display_freqX_b(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	uint_fast8_t rj;
	uint_fast8_t fullwidth = display_getfreqformat(& rj);
	const uint_fast8_t comma = 3 - rj;

	const uint_fast32_t freq = hamradio_get_freq_b();

	display_setcolors(FRQCOLOR, BGCOLOR);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x, y + lowhalf);
		display_value_small(freq, fullwidth, comma, comma + 3, rj, lowhalf);
	} while (lowhalf --);
}

// ���������� ������� ���������� ������� �������
static void display_freqmeter10(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHFQMETER
	char buffer [32];
	local_snprintf_P(buffer, sizeof buffer / sizeof buffer [0], PSTR("%10lu"), board_get_fqmeter());

	display_setcolors(FRQCOLOR, BGCOLOR);
	display_at(x, y, buffer);
#endif /* WITHFQMETER */
}


// ����������� ������ (�� FLASH) � ���������� �� ���������
static void 
NOINLINEAT
display2_text_P(
	uint_fast8_t x, 
	uint_fast8_t y, 
	const FLASHMEM char * const * labels,	// ������ ���������� �� �����
	const COLOR_T * colorsfg,			// ������ ������ face
	const COLOR_T * colorsbg,			// ������ ������ background
	uint_fast8_t state
	)
{
	#if LCDMODE_COLORED
	#else /* LCDMODE_COLORED */
	#endif /* LCDMODE_COLORED */

	display_setcolors(colorsfg [state], colorsbg [state]);
	display_at_P(x, y, labels [state]);
}

// ����������� ������ � ���������� �� ���������
static void 
NOINLINEAT
display2_text(
	uint_fast8_t x, 
	uint_fast8_t y, 
	const char * const * labels,	// ������ ���������� �� �����
	const COLOR_T * colorsfg,			// ������ ������ face
	const COLOR_T * colorsbg,			// ������ ������ background
	uint_fast8_t state
	)
{
	#if LCDMODE_COLORED
	#else /* LCDMODE_COLORED */
	#endif /* LCDMODE_COLORED */

	display_setcolors(colorsfg [state], colorsbg [state]);
	display_at(x, y, labels [state]);
}

// ����������� ������� TX / RX
static void display_txrxstatecompact(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
	const uint_fast8_t tx = hamradio_get_tx();
	display_setcolors(TXRXMODECOLOR, tx ? MODECOLORBG_TX : MODECOLORBG_RX);
	display_at_P(x, y, tx ? PSTR("T") : PSTR(" "));
#endif /* WITHTX */
}

// todo: ������ LCDMODE_COLORED

// ��������� ����������� ��������� �����/��������
static const COLOR_T colorsfg_2alarm [2] = { COLOR_BLACK, COLOR_BLACK, };
static const COLOR_T colorsbg_2alarm [2] = { COLOR_GREEN, COLOR_RED, };

// ��������� ����������� ��������� �� ���� ���������
static const COLOR_T colorsfg_4state [4] = { COLOR_BLACK, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, };
static const COLOR_T colorsbg_4state [4] = { COLOR_GREEN, COLOR_DARKGREEN, COLOR_DARKGREEN, COLOR_DARKGREEN, };

// ��������� ����������� ��������� �� ���� ���������
static const COLOR_T colorsfg_2state [2] = { COLOR_BLACK, COLOR_WHITE, };
static const COLOR_T colorsbg_2state [2] = { COLOR_GREEN, COLOR_DARKGREEN, };

// ��������� ����������� ������� ��� ���������
static const COLOR_T colorsfg_1state [1] = { COLOR_GREEN, };
static const COLOR_T colorsbg_1state [1] = { COLOR_BLACK, };	// ��������������� � ���� ���� �� �������

// ��������� ����������� ������� ��� ���������
static const COLOR_T colorsfg_1gold [1] = { COLOR_YELLOW, };
static const COLOR_T colorsbg_1gold [1] = { COLOR_BLACK, };	// ��������������� � ���� ���� �� �������

// ����������� ������� TX / RX
static void display_txrxstate2(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
	const uint_fast8_t state = hamradio_get_tx();

	static const FLASHMEM char text0 [] = "RX";
	static const FLASHMEM char text1 [] = "TX";
	const FLASHMEM char * const labels [2] = { text0, text1 };
	display2_text_P(x, y, labels, colorsfg_2alarm, colorsbg_2alarm, state);
#endif /* WITHTX */
}

static void display_recstatus(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHUSEAUDIOREC
	unsigned long hamradio_get_recdropped(void);
	int hamradio_get_recdbuffered(void);

	char buffer [12];
	local_snprintf_P(
		buffer,
		sizeof buffer / sizeof buffer [0],
		PSTR("%08lx %2d"), 
		(unsigned long) hamradio_get_recdropped(),
		(int) hamradio_get_recdbuffered()
		);
		
	display_setcolors(MODECOLOR, BGCOLOR);
	display_at(x, y, buffer);

#endif /* WITHUSEAUDIOREC */
}
// ����������� ������ ������ ����� ���������
static void display_rec3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHUSEAUDIOREC

	const uint_fast8_t state = hamradio_get_rec_value();

	static const FLASHMEM char text_pau [] = "PAU";
	static const FLASHMEM char text_rec [] = "REC";
	const FLASHMEM char * const labels [2] = { text_pau, text_rec };
	display2_text_P(x, y, labels, colorsfg_2state, colorsbg_2state, state);

#endif /* WITHUSEAUDIOREC */
}

// ����������� ��������� USB HOST
static void display_usb1(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if defined (WITHUSBHW_HOST)
	const uint_fast8_t active = hamradio_get_usbh_active();
	#if LCDMODE_COLORED
		display_setcolors(TXRXMODECOLOR, active ? MODECOLORBG_TX : MODECOLORBG_RX);
		display_at_P(x, y, PSTR("U"));
	#else /* LCDMODE_COLORED */
		display_at_P(x, y, active ? PSTR("U") : PSTR("X"));
	#endif /* LCDMODE_COLORED */
#endif /* defined (WITHUSBHW_HOST) */
}

void display_2states(
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t state,
	const char * state1,	// ��������
	const char * state0
	)
{
	#if LCDMODE_COLORED
		const char * const labels [2] = { state1, state1, };
	#else /* LCDMODE_COLORED */
		const char * const labels [2] = { state0, state1, };
	#endif /* LCDMODE_COLORED */
	display2_text(x, y, labels, colorsfg_2state, colorsbg_2state, state);
}

void display_2states_P(
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t state,
	const FLASHMEM char * state1,	// ��������
	const FLASHMEM char * state0
	)
{
	#if LCDMODE_COLORED
		const FLASHMEM char * const labels [2] = { state1, state1, };
	#else /* LCDMODE_COLORED */
		const FLASHMEM char * const labels [2] = { state0, state1, };
	#endif /* LCDMODE_COLORED */
	display2_text_P(x, y, labels, colorsfg_2state, colorsbg_2state, state);
}

// ���������, �� �������� ��������� ������
void display_1state_P(
	uint_fast8_t x, 
	uint_fast8_t y, 
	const FLASHMEM char * label
	)
{
	display2_text_P(x, y, & label, colorsfg_1state, colorsbg_1state, 0);
}

// ���������, �� �������� ��������� ������
void display_1state(
	uint_fast8_t x, 
	uint_fast8_t y, 
	const char * label
	)
{
	display2_text(x, y, & label, colorsfg_1state, colorsbg_1state, 0);
}


static const FLASHMEM char text_nul1_P [] = " ";
static const FLASHMEM char text_nul3_P [] = "   ";
static const FLASHMEM char text_nul4_P [] = "    ";
static const FLASHMEM char text_nul5_P [] = "     ";
//static const FLASHMEM char text_nul9_P [] = "         ";
static const char text_nul3 [] = "   ";
static const char text_nul5 [] = "     ";

// ����������� ������ NOCH ON/OFF
static void display_notch5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHNOTCHONOFF || WITHNOTCHFREQ
	int_fast32_t freq;
	const uint_fast8_t state = hamradio_get_notchvalue(& freq);
	display_2states_P(x, y, state, PSTR("NOTCH"), text_nul5_P);
#endif /* WITHNOTCHONOFF || WITHNOTCHFREQ */
}

// ����������� ������� NOCH
static void display_notchfreq5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHNOTCHONOFF || WITHNOTCHFREQ
	int_fast32_t freq;
	const uint_fast8_t state = hamradio_get_notchvalue(& freq);
	char s [6];
	local_snprintf_P(s, sizeof s / sizeof s [0], PSTR("%5lu"), freq);
	display_2states(x, y, state, s, text_nul5);
#endif /* WITHNOTCHONOFF || WITHNOTCHFREQ */
}

// ����������� ������ NOCH ON/OFF
static void display_notch3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHNOTCHONOFF || WITHNOTCHFREQ
	int_fast32_t freq;
	const uint_fast8_t state = hamradio_get_notchvalue(& freq);
	static const FLASHMEM char text_nch [] = "NCH";
	display_2states_P(x, y, state, PSTR("NCH"), text_nul3_P);
#endif /* WITHNOTCHONOFF || WITHNOTCHFREQ */
}


// VFO mode
static void display_vfomode3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	uint_fast8_t state;
	const char * const labels [1] = { hamradio_get_vfomode3_value(& state), };
	display2_text(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
}


// VFO mode
static void display_vfomode5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	uint_fast8_t state;
	const char * const labels [1] = { hamradio_get_vfomode5_value(& state), };
	display2_text(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
}

static void display_XXXXX3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHPLACEHOLDERS
	const uint_fast8_t state = 0;
	display_2states_P(x, y, state, text_nul3_P, text_nul3_P);
#endif /* WITHPLACEHOLDERS */
}

static void display_XXXXX5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHPLACEHOLDERS
	const uint_fast8_t state = 0;
	display_2states_P(x, y, state, text_nul5_P, text_nul5_P);
#endif /* WITHPLACEHOLDERS */
}

// ����������� ������ �������� ����� � USB
static void display_datamode4(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
	#if WITHIF4DSP && WITHUSBUAC && WITHDATAMODE
		const uint_fast8_t state = hamradio_get_datamode();
		display_2states_P(x, y, state, PSTR("DATA"), text_nul4_P);
	#endif /* WITHIF4DSP && WITHUSBUAC && WITHDATAMODE */
#endif /* WITHTX */
}

// ����������� ������ �������� ����� � USB
static void display_datamode3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
	#if WITHIF4DSP && WITHUSBUAC && WITHDATAMODE
		const uint_fast8_t state = hamradio_get_datamode();
		display_2states_P(x, y, state, PSTR("DAT"), text_nul3_P);
	#endif /* WITHIF4DSP && WITHUSBUAC && WITHDATAMODE */
#endif /* WITHTX */
}

// ����������� ������ �������������
static void display_atu3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
	#if WITHAUTOTUNER
		const uint_fast8_t state = hamradio_get_atuvalue();
		display_2states_P(x, y, state, PSTR("ATU"), text_nul3_P);
	#endif /* WITHAUTOTUNER */
#endif /* WITHTX */
}


// ����������� ������ General Coverage / HAM bands
static void display_genham1(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHBCBANDS

	const uint_fast8_t state = hamradio_get_genham_value();

	display_2states_P(x, y, state, PSTR("G"), PSTR("H"));

#endif /* WITHBCBANDS */
}

// ����������� ������ ������ ������
static void display_byp3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
	#if WITHAUTOTUNER
		const uint_fast8_t state = hamradio_get_bypvalue();
		display_2states_P(x, y, state, PSTR("BYP"), text_nul3_P);
	#endif /* WITHAUTOTUNER */
#endif /* WITHTX */
}

// ����������� ������ VOX
static void display_vox3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
	#if WITHVOX
		const uint_fast8_t state = hamradio_get_voxvalue();
		display_2states_P(x, y, state, PSTR("VOX"), text_nul3_P);
	#endif /* WITHVOX */
#endif /* WITHTX */
}

// ����������� ������� VOX � TUNE
// ���� VOX �� ������������, ������ TUNE
static void display_voxtune3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX

	static const FLASHMEM char text_vox [] = "VOX";
	static const FLASHMEM char text_tun [] = "TUN";
	static const FLASHMEM char text_nul [] = "   ";

#if WITHVOX

	const uint_fast8_t tunev = hamradio_get_tunemodevalue();
	const uint_fast8_t voxv = hamradio_get_voxvalue();

	#if LCDMODE_COLORED
		const FLASHMEM char * const labels [4] = { text_vox, text_vox, text_tun, text_tun, };
	#else /* LCDMODE_COLORED */
		const FLASHMEM char * const labels [4] = { text_nul, text_vox, text_tun, text_tun, };
	#endif /* LCDMODE_COLORED */

	display2_text_P(x, y, labels, colorsfg_4state, colorsbg_4state, tunev * 2 + voxv);

#else /* WITHVOX */

	const uint_fast8_t state = hamradio_get_tunemodevalue();

	display_2states_P(x, y, state, PSTR("TUN"), text_nul3_P);

#endif /* WITHVOX */
#endif /* WITHTX */
}

// ����������� ������� VOX � TUNE
// ������� �����
// ���� VOX �� ������������, ������ TUNE
static void display_voxtune4(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
#if WITHVOX

	const uint_fast8_t tunev = hamradio_get_tunemodevalue();
	const uint_fast8_t voxv = hamradio_get_voxvalue();
	static const FLASHMEM char text0 [] = "VOX ";
	static const FLASHMEM char text1 [] = "TUNE";
	const FLASHMEM char * const labels [4] = { text0, text0, text1, text1, };
	display2_text_P(x, y, labels, colorsfg_4state, colorsbg_4state, tunev * 2 + voxv);

#else /* WITHVOX */

	const uint_fast8_t state = hamradio_get_tunemodevalue();
		display_2states_P(x, y, state, PSTR("TUNE"), text_nul4_P);

#endif /* WITHVOX */
#endif /* WITHTX */
}

// ����������� ������� VOX � TUNE
// ������������� �����������
// ���� VOX �� ������������, ������ TUNE
static void display_voxtune1(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTX
#if WITHVOX

	const uint_fast8_t tunev = hamradio_get_tunemodevalue();
	const uint_fast8_t voxv = hamradio_get_voxvalue();
	static const FLASHMEM char textx [] = " ";
	static const FLASHMEM char text0 [] = "V";
	static const FLASHMEM char text1 [] = "U";
	const FLASHMEM char * const labels [4] = { textx, text0, text1, text1, };
	display2_text_P(x, y, labels, colorsfg_4state, colorsbg_4state, tunev * 2 + voxv);

#else /* WITHVOX */

	const uint_fast8_t state = hamradio_get_tunemodevalue();
	display_2states_P(x, y, state, PSTR("U"), text_nul1_P);

#endif /* WITHVOX */
#endif /* WITHTX */
}


static void display_lockstate4(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	const uint_fast8_t lockv = hamradio_get_lockvalue();
	const uint_fast8_t fastv = hamradio_get_usefastvalue();

	static const FLASHMEM char text0 [] = "    ";
	static const FLASHMEM char text1 [] = "LOCK";
	static const FLASHMEM char text2 [] = "FAST";
#if LCDMODE_COLORED
	const FLASHMEM char * const labels [4] = { text1, text2, text1, text1, };
#else /* LCDMODE_COLORED */
	const FLASHMEM char * const labels [4] = { text0, text2, text1, text1, };
#endif
	display2_text_P(x, y, labels, colorsfg_4state, colorsbg_4state, lockv * 2 + fastv);
}


static void display_lockstate1(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	display_setcolors(LOCKCOLOR, BGCOLOR);
	display_at_P(x, y, hamradio_get_lockvalue() ? PSTR("*") : PSTR(" "));
}

// ����������� PBT
static void display_pbt(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHPBT
	const int_fast32_t pbt = hamradio_get_pbtvalue();

	//display_setcolors(LOCKCOLOR, BGCOLOR);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x + 0, y + lowhalf);		
		display_string_P(PSTR("PBT "), lowhalf);
		display_gotoxy(x + 4, y + lowhalf);		
		display_menu_value(pbt, 4 | WSIGNFLAG, 2, 1, lowhalf);
	} while (lowhalf --);
#endif /* WITHPBT */
}

// RX path bandwidth
static void display_rxbw3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	const char FLASHMEM * const labels [1] = { hamradio_get_rxbw_value_P(), };
	display2_text_P(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
}


// ������� ��������� DUAL WATCH
static void display_mainsub3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHUSEDUALWATCH
	const char FLASHMEM * const labels [1] = { hamradio_get_mainsubrxmode3_value_P(), };
	display2_text_P(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
#endif /* WITHUSEDUALWATCH */
}


// RX preamplifier
static void display_pre3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	const char FLASHMEM * const labels [1] = { hamradio_get_pre_value_P(), };
	display2_text_P(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
}

// ������������ ��� (���� ���������� ��� REDRM_BARS - � ��������� �����������)
static void display_ovf3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHDSPEXTDDC
	//const char FLASHMEM * const labels [1] = { hamradio_get_pre_value_P(), };
	//display2_text_P(x, y, labels, colorsfg_1state, colorsbg_1state, 0);

	if (boad_fpga_adcoverflow() != 0)
	{
		display_setcolors(BGCOLOR, OVFCOLOR);
		display_at_P(x, y, PSTR("OVF"));
	}
	else if (boad_mike_adcoverflow() != 0)
	{
		display_setcolors(BGCOLOR, OVFCOLOR);
		display_at_P(x, y, PSTR("MIK"));
	}
	else
	{
		display_setcolors(BGCOLOR, BGCOLOR);
		display_at_P(x, y, PSTR("   "));
	}
#endif /* WITHDSPEXTDDC */
}

// RX preamplifier ��� ������������ ��� (���� ���������� ��� REDRM_BARS - � ��������� �����������)
static void display_preovf3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	if (boad_fpga_adcoverflow() != 0)
	{
		display_setcolors(OVFCOLOR, BGCOLOR);
		display_at_P(x, y, PSTR("OVF"));
	}
	else if (boad_mike_adcoverflow() != 0)
	{
		display_setcolors(BGCOLOR, OVFCOLOR);
		display_at_P(x, y, PSTR("MIK"));
	}
	else
	{
		display_setcolors(MODECOLOR, BGCOLOR);
		display_at_P(x, y, hamradio_get_pre_value_P());
	}
}

// display antenna
static void display_ant5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHANTSELECT
	const char FLASHMEM * const labels [1] = { hamradio_get_ant5_value_P(), };
	display2_text_P(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
#endif /* WITHANTSELECT */
}

// RX att (or att/pre)
static void display_att4(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	const char FLASHMEM * const labels [1] = { hamradio_get_att_value_P(), };
	display2_text_P(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
}

// HP/LP
static void display_hplp2(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHPOWERLPHP
	const char FLASHMEM * const labels [1] = { hamradio_get_hplp_value_P(), };
	display2_text_P(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
#endif /* WITHPOWERLPHP */
}

// RX att, ��� �������� ���������� TX
static void display_att_tx3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	const uint_fast8_t tx = hamradio_get_tx();
	const FLASHMEM char * text = tx ? PSTR("TX  ") : hamradio_get_att_value_P();

	display_setcolors(MODECOLOR, BGCOLOR);
	display_at_P(x, y, text);
}

// RX agc
static void display_agc3(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	display_1state_P(x, y, hamradio_get_agc3_value_P());
}

// RX agc
static void display_agc4(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	display_1state_P(x, y, hamradio_get_agc4_value_P());
}

// VFO mode - ����� �������� (������ �� ����� SPLIT ��� ��������)
static void display_vfomode1(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	uint_fast8_t state;
	const char * const label = hamradio_get_vfomode3_value(& state);

	display_setcolors(MODECOLOR, BGCOLOR);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x, y + lowhalf);
		display_wrdata_begin();
		display_put_char_small(label [0], lowhalf);
		display_wrdata_end();
	} while (lowhalf --);
}

// SSB/CW/AM/FM/...
static void display_mode3_a(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	const char FLASHMEM * const labels [1] = { hamradio_get_mode_a_value_P(), };
	display2_text_P(x, y, labels, colorsfg_1gold, colorsbg_1gold, 0);
}


// SSB/CW/AM/FM/...
static void display_mode3_b(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	const char FLASHMEM * const labels [1] = { hamradio_get_mode_b_value_P(), };
	display2_text_P(x, y, labels, colorsfg_1gold, colorsbg_1gold, 0);
}

// dd.dV - 5 places
static void display_voltlevelV5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHVOLTLEVEL && WITHCPUADCHW
	uint_fast8_t volt = hamradio_get_volt_value();	// ���������� � ������ ���������� �.�. 151 = 15.1 ������

	display_setcolors(colorsfg_1state [0], colorsbg_1state [0]);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x + CHARS2GRID(0), y + lowhalf);	
		display_value_small(volt, 3, 1, 255, 0, lowhalf);
		display_gotoxy(x + CHARS2GRID(4), y + lowhalf);	
		display_string_P(PSTR("V"), lowhalf);
	} while (lowhalf --);
#endif /* WITHVOLTLEVEL && WITHCPUADCHW */
}

// dd.d - 4 places
static void display_voltlevel4(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHVOLTLEVEL && WITHCPUADCHW
	const uint_fast8_t volt = hamradio_get_volt_value();	// ���������� � ������ ���������� �.�. 151 = 15.1 ������

	display_setcolors(colorsfg_1state [0], colorsbg_1state [0]);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x, y + lowhalf);	
		display_value_small(volt, 3, 1, 255, 0, lowhalf);
	} while (lowhalf --);
#endif /* WITHVOLTLEVEL && WITHCPUADCHW */
}

// ����������� ����� ��������
static void display_thermo4(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHTHERMOLEVEL && WITHCPUADCHW
	int_fast16_t tempv = hamradio_get_temperature_value() / 10;	// ������� � ������� ����� � ����� �������

	display_setcolors(colorsfg_1state [0], colorsbg_1state [0]);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x + CHARS2GRID(0), y + lowhalf);	
		display_value_small(tempv, 3 | WSIGNFLAG, 0, 255, 0, lowhalf);
		//display_gotoxy(x + CHARS2GRID(4), y + lowhalf);	
		//display_string_P(PSTR("�"), lowhalf);
	} while (lowhalf --);
#endif /* WITHTHERMOLEVEL && WITHCPUADCHW */
}

// +d.ddA - 6 places (with "A")
// amphermeter with "A"
static void display_currlevelA6(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHCURRLEVEL && WITHCPUADCHW
	int_fast16_t drain = hamradio_get_pacurrent_value();	// ��� � �������� ��������� (�� 2.55 ������), ����� ���� �������������

	display_setcolors(colorsfg_1state [0], colorsbg_1state [0]);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x + CHARS2GRID(0), y + lowhalf);	
		display_value_small(drain, 3 | WMINUSFLAG, 2, 255, 0, lowhalf);
		display_gotoxy(x + CHARS2GRID(5), y + lowhalf);	
		display_string_P(PSTR("A"), lowhalf);
	} while (lowhalf --);
#endif /* WITHCURRLEVEL && WITHCPUADCHW */
}

// d.dd - 5 places (without "A")
static void display_currlevel5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHCURRLEVEL && WITHCPUADCHW
	int_fast16_t drain = hamradio_get_pacurrent_value();	// ��� � �������� ��������� (�� 2.55 ������), ����� ���� �������������

	display_setcolors(colorsfg_1state [0], colorsbg_1state [0]);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x + CHARS2GRID(0), y + lowhalf);	
		display_value_small(drain, 3 | WMINUSFLAG, 2, 255, 0, lowhalf);
		//display_gotoxy(x + CHARS2GRID(4), y + lowhalf);	
		//display_string_P(PSTR("A"), lowhalf);
	} while (lowhalf --);
#endif /* WITHCURRLEVEL && WITHCPUADCHW */
}
// dd.d - 5 places (without "A")
static void display_currlevel5alt(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHCURRLEVEL && WITHCPUADCHW
	int_fast16_t drain = hamradio_get_pacurrent_value();	// ��� � �������� ��������� (�� 2.55 ������), ����� ���� �������������

	display_setcolors(colorsfg_1state [0], colorsbg_1state [0]);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x + CHARS2GRID(0), y + lowhalf);	
		display_value_small(drain, 3 | WMINUSFLAG, 1, 255, 1, lowhalf);
		//display_gotoxy(x + CHARS2GRID(4), y + lowhalf);	
		//display_string_P(PSTR("A"), lowhalf);
	} while (lowhalf --);
#endif /* WITHCURRLEVEL && WITHCPUADCHW */
}

// ����������� ������ ������� � dBm
static void display_siglevel7(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHIF4DSP
	uint_fast8_t tracemax;
	uint_fast8_t v = board_getsmeter(& tracemax, 0, UINT8_MAX, 0);

	char buff [32];
	local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("%-+4d" "dBm"), tracemax - UINT8_MAX);
	(void) v;
	const char * const labels [1] = { buff, };
	display2_text(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
#endif /* WITHIF4DSP */
}

// ����������� ������ ������� � ������ ����� S
// S9+60
static void display_siglevel5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHIF4DSP
	uint_fast8_t tracemax;
	uint_fast8_t v = board_getsmeter(& tracemax, 0, UINT8_MAX, 0);

	char buff [32];
	const int s9level = - 73;
	const int s9step = 6;
	const int alevel = tracemax - UINT8_MAX;

	(void) v;
	if (alevel < (s9level - s9step * 9))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S0   "));
	}
	else if (alevel < (s9level - s9step * 7))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S1   "));
	}
	else if (alevel < (s9level - s9step * 6))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S2   "));
	}
	else if (alevel < (s9level - s9step * 5))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S3   "));
	}
	else if (alevel < (s9level - s9step * 4))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S4   "));
	}
	else if (alevel < (s9level - s9step * 3))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S5   "));
	}
	else if (alevel < (s9level - s9step * 2))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S6   "));
	}
	else if (alevel < (s9level - s9step * 1))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S7   "));
	}
	else if (alevel < (s9level - s9step * 0))
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S8   "));
	}
	else
	{
		local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("S9+%02d"), alevel - s9level);
	}
	const char * const labels [1] = { buff, };
	display2_text(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
#endif /* WITHIF4DSP */
}

static void display_freqdelta8(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHINTEGRATEDDSP
	int_fast32_t deltaf;
	const uint_fast8_t f = dsp_getfreqdelta10(& deltaf, 0);		/* �������� �������� ���������� ������� � ��������� 0.1 ����� ��� ��������� A */
	deltaf = - deltaf;	// ������ �� ������� ������������� � ����������
	display_setcolors(colorsfg_1state [0], colorsbg_1state [0]);
	if (f != 0)
	{
		uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);	
			display_value_small(deltaf, 6 | WSIGNFLAG, 1, 255, 0, lowhalf);
		} while (lowhalf --);
	}
	else
	{
		display_at_P(x, y, PSTR("        "));
	}
#endif /* WITHINTEGRATEDDSP */
}

/* �������� ���������� �� ������ ��������� � ������ SAM */
static void display_samfreqdelta8(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHINTEGRATEDDSP
	int_fast32_t deltaf;
	const uint_fast8_t f = hamradio_get_samdelta10(& deltaf, 0);		/* �������� �������� ���������� ������� � ��������� 0.1 ����� ��� ��������� A */
	deltaf = - deltaf;	// ������ �� ������� ������������� � ����������
	display_setcolors(colorsfg_1state [0], colorsbg_1state [0]);
	if (f != 0)
	{
		uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
		do
		{
			display_gotoxy(x, y + lowhalf);	
			display_value_small(deltaf, 6 | WSIGNFLAG, 1, 255, 0, lowhalf);
		} while (lowhalf --);
	}
	else
	{
		display_at_P(x, y, PSTR("        "));
	}
#endif /* WITHINTEGRATEDDSP */
}

// d.d - 3 places
// ������� �������� ������� ������� ����� �� ������� ��/��
static void display_amfmhighcut4(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if WITHAMHIGHKBDADJ
	uint_fast8_t flag;
	const uint_fast8_t v = hamradio_get_amfm_highcut10_value(& flag);	// ������� �������� ������� ������� ����� �� ������� ��/�� (� �������� ����)

	display_setcolors(colorsfg_2state [flag], colorsbg_2state [flag]);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x, y + lowhalf);	
		display_value_small(v, 3, 2, 255, 0, lowhalf);
	} while (lowhalf --);
#endif /* WITHAMHIGHKBDADJ */
}

// ������ ������� - ����, ������ � �������
static void display_time8(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if defined (RTC1_TYPE)
	uint_fast8_t hour, minute, secounds;
	char buff [9];

	board_rtc_gettime(& hour, & minute, & secounds);
	local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("%02d:%02d:%02d"), 
		hour, minute, secounds
		);
	
	const char * const labels [1] = { buff, };
	display2_text(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
#endif /* WITHNMEA */
}

// ������ ������� - ������ ���� � ������, ��� ������
static void display_time5(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if defined (RTC1_TYPE)
	uint_fast8_t hour, minute, secounds;
	char buff [6];

	board_rtc_gettime(& hour, & minute, & secounds);
	local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("%02d%c%02d"), 
		hour, 
		((secounds & 1) ? ' ' : ':'),	// �������� ��������� � �������� ��� �������
		minute
		);

	const char * const labels [1] = { buff, };
	display2_text(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
#endif /* WITHNMEA */
}

// ������ ������� - ������ ���� � ������, ��� ������
// Jan-01 13:40
static void display_datetime12(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if defined (RTC1_TYPE)
	char buff [13];

	uint_fast16_t year;
	uint_fast8_t month, day;
	uint_fast8_t hour, minute, secounds;
	static const char months [13] [4] = 
	{
		"JAN",
		"FEB",
		"MAR",
		"APR",
		"MAY",
		"JUN",
		"JUL",
		"AUG",
		"SEP",
		"OCT",
		"NOV",
		"DEC",
	};

	board_rtc_getdatetime(& year, & month, & day, & hour, & minute, & secounds);

	local_snprintf_P(buff, sizeof buff / sizeof buff [0], PSTR("%s %2d %02d%c%02d"), 
		months [month - 1],
		day,
		hour, 
		((secounds & 1) ? ' ' : ':'),	// �������� ��������� � �������� ��� �������
		minute
		);

	const char * const labels [1] = { buff, };
	display2_text(x, y, labels, colorsfg_1state, colorsbg_1state, 0);
#endif /* WITHNMEA */
}

struct dzone
{
	uint_fast8_t x; // ����� ������� ����
	uint_fast8_t y;
	void (* redraw)(uint_fast8_t x, uint_fast8_t y, void * pv);	// ������� ����������� ��������
	uint_fast8_t key;		// ��� ����� ����������� ���������������� ���� �������
	uint_fast8_t subset;
};

/* struct dzone subset field values */

#define PAGESLEEP 7

#define REDRSUBSET(page)		(1U << (page))	// ������ ������������� ������ ������������� ������ ���������

#define REDRSUBSET_ALL ( \
		REDRSUBSET(0) | \
		REDRSUBSET(1) | \
		REDRSUBSET(2) | \
		REDRSUBSET(3) | \
		REDRSUBSET(4) | \
		0)

#define REDRSUBSET_MENU		REDRSUBSET(5)
#define REDRSUBSET_MENU2	REDRSUBSET(6)
#define REDRSUBSET_SLEEP	REDRSUBSET(PAGESLEEP)

enum
{
	REDRM_MODE,		// ���� ���������� ��� ��������� ������� ������, LOCK state
	REDRM_FREQ,		// ���������� �������
	REDRM_FRQB,	// ���������� �������
	REDRM_BARS,		// S-meter, SWR-meter, voltmeter
	REDRM_VOLT,		// ��������� (����� ���������� ���������)

	REDRM_MFXX,		// ��� �������������� ���������
	REDRM_MLBL,		// �������� �������������� ���������
	REDRM_MVAL,		// �������� ��������� ����
	REDRM_count
};

/* �������� ������������ ��������� �� �������� */

#if DSTYLE_T_X20_Y4

	enum
	{
		BDTH_ALLRX = 14,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_RIGHTRX = 5,	// ������ ���������� ������
		BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
		//
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 9,
		BDTH_SPACESWR = 1,
		BDTH_ALLPWR = 4,
		BDTH_SPACEPWR = 0
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX
	#endif /* WITHSHOWSWRPWR */
	};
	#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

	enum {
		DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		DPAGE1,					// ��������, � ������� ������������ �������� (��� ���) 
		DISPLC_MODCOUNT,
	};
	enum {
		DPAGEEXT = REDRSUBSET(DPAGE0) | REDRSUBSET(DPAGE1)
	};
	#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
	#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
	static const FLASHMEM struct dzone dzones [] =
	{
		{	0, 0,	display_freqchr_a,	REDRM_FREQ, DPAGEEXT, },	// ������� ��� ���������� ��������
		//{	0, 0,	display_txrxstate2,	REDRM_MODE, DPAGEEXT, },
		{	9, 0,	display_mode3_a,	REDRM_MODE,	DPAGEEXT, },	// SSB/CW/AM/FM/...
		{	12, 0,	display_lockstate1,	REDRM_MODE, DPAGEEXT, },
		{	13, 0,	display_rxbw3,		REDRM_MODE, DPAGEEXT, },
		{	17, 0,	display_vfomode3,	REDRM_MODE, DPAGEEXT, },	// SPLIT

		{	0, 1,	display_att_tx3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
		{	4, 1,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
	#if defined (RTC1_TYPE)
		{	8, 1,	display_time8,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME
	#endif /* defined (RTC1_TYPE) */
	#if WITHUSEDUALWATCH
		{	0, 1,	display_freqchr_b,	REDRM_FREQ, REDRSUBSET(DPAGE1), },	// ������� ��� ���������� ��������
		{	17, 1,	display_mainsub3, REDRM_MODE, DPAGEEXT, },	// main/sub RX
	#endif /* WITHUSEDUALWATCH */

		{	0, 2,	display2_bars,		REDRM_BARS, DPAGEEXT, },	// S-METER, SWR-METER, POWER-METER
		{	17, 2,	display_voxtune3,	REDRM_MODE, DPAGEEXT, },

	#if WITHVOLTLEVEL && WITHCURRLEVEL
		{	0, 3,	display_voltlevel4, REDRM_VOLT, DPAGEEXT, },	// voltmeter with "V"
		{	5, 3,	display_currlevel5, REDRM_VOLT, DPAGEEXT, },	// without "A"
	#endif /* WITHVOLTLEVEL && WITHCURRLEVEL */
#if WITHIF4DSP
	#if WITHUSEAUDIOREC
		{	13, 3,	display_rec3,		REDRM_BARS, DPAGEEXT, },	// ����������� ������ ������ ����� ���������
	#endif /* WITHUSEAUDIOREC */
#endif /* WITHIF4DSP */
		{	17, 3,	display_agc3,		REDRM_MODE, DPAGEEXT, },

	#if WITHMENU 
		{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
	#endif /* WITHMENU */
	};

#elif DSTYLE_T_X20_Y2
	/*
		��� ��, ��� ������ ������ ������� 2*20, ��������� ������ � ��� 
		��������� (������ ��������� ��������) - ��� ����� ���������, 
		���� ������������ ������.
		����� ���������, ����� ������� � ����� �� ������� ������������ �������� ���������. 
		���� ���� ������� ��� ����� � �������� ������� ������ �������� - ����� ��� � �����������, 
		� �������. ������ ��� ���������� ���� ����� �������� ������ �������� ���������� 
		����� �� ������, ������� ������ ����������� ��� ������������ ������ �����������. 
		��, ��� ������������� �� MENU (REDRSUBSET(0)/REDRSUBSET(1)) - ���������������� �� ��������� ������.

	*/
	enum
	{
		BDTH_ALLRX = 12,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_RIGHTRX = 4,	// ������ ���������� ������
		BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
		BDTH_SPACERX = 1,		/* ���������� �������, ���������� ������ �� ������ S-����� */
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 6,
		BDTH_SPACESWR = 1,	/* ���������� �������, ���������� ������ �� ������ SWR-����� */
		BDTH_ALLPWR = 5,
		BDTH_SPACEPWR = 0	/* ���������� �������, ���������� ������ �� ������ PWR-����� */
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX
	#endif /* WITHSHOWSWRPWR */
	};
	#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

	enum {
		DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		DPAGE_SMETER,
		DPAGE_TIME,
		DISPLC_MODCOUNT
	};
	#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
	#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
	static const FLASHMEM struct dzone dzones [] =
	{
		// ������ 0
		{	0, 0,	display_vfomode3,	REDRM_MODE, REDRSUBSET_ALL, },	// SPLIT
		{	4, 0,	display_freqchr_a,	REDRM_FREQ, REDRSUBSET_ALL, },	// ������� ��� ���������� ��������
		{	12, 0,	display_lockstate1,	REDRM_MODE, REDRSUBSET_ALL, },
		{	13, 0,	display_mode3_a,	REDRM_MODE,	REDRSUBSET_ALL, },	// SSB/CW/AM/FM/...
		{	17, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET_ALL, },
		// ������ 1 - ���������� ��������
		{	0, 1,	display_voxtune3,	REDRM_MODE, REDRSUBSET_ALL, },
		// ������ 1
		{	4, 1,	display_att_tx3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
	#if WITHDSPEXTDDC
		{	9, 1,	display_preovf3,	REDRM_BARS, REDRSUBSET(DPAGE0), },	// ovf/pre
	#else /* WITHDSPEXTDDC */
		{	9, 1,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// pre
	#endif /* WITHDSPEXTDDC */
		// ������ 1
		{	4, 1,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE_SMETER), },	// S-METER, SWR-METER, POWER-METER
		{	4, 1,	display_time8,		REDRM_BARS, REDRSUBSET(DPAGE_TIME), },	// TIME
		// ������ 1 - ���������� ��������
		{	17, 1,	display_agc3,		REDRM_MODE, REDRSUBSET_ALL, },

		//{	0, 0,	display_txrxstate2,	REDRM_MODE, REDRSUBSET_ALL, },
	#if WITHMENU
		{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
	#endif /* WITHMENU */
	};

#elif DSTYLE_T_X20_Y2_IGOR

	enum
	{
		BDTH_ALLRX = 12,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_RIGHTRX = 4,	// ������ ���������� ������
		BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
		BDTH_SPACERX = 1,		/* ���������� �������, ���������� ������ �� ������ S-����� */
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 6,
		BDTH_SPACESWR = 1,	/* ���������� �������, ���������� ������ �� ������ SWR-����� */
		BDTH_ALLPWR = 5,
		BDTH_SPACEPWR = 0	/* ���������� �������, ���������� ������ �� ������ PWR-����� */
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX
	#endif /* WITHSHOWSWRPWR */
	};
	#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

	enum {
		DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		DPAGE1 = 0,					// ��������, � ������� ������������ �������� (��� ���) 
		DISPLC_MODCOUNT
	};
	#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
	#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
	static const FLASHMEM struct dzone dzones [] =
	{
		{	0, 0,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0) | REDRSUBSET(DPAGE1), },	// SPLIT
		{	4, 0,	display_freqchr_a,	REDRM_FREQ, REDRSUBSET(DPAGE0) | REDRSUBSET(DPAGE1), },	// ������� ��� ���������� ��������
		{	12, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0) | REDRSUBSET(DPAGE1), },
		{	4, 1,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
		//{	0, 1, display_pbt,		REDRM_BARS, REDRSUBSET(DPAGE1), },	// PBT +00.00
	#if WITHMENU
		{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
	#endif /* WITHMENU */
	};

#elif DSTYLE_T_X16_Y2 && DSTYLE_SIMPLEFREQ

	enum
	{
		BDTH_ALLRX = 12,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_RIGHTRX = 4,	// ������ ���������� ������
		BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
		//
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 7,
		BDTH_SPACESWR = 1,
		BDTH_ALLPWR = 4,
		BDTH_SPACEPWR = 0
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX
	#endif /* WITHSHOWSWRPWR */
	};
	#define SWRMAX	40	// 4.0 - �������� �� ������ �����
	enum {
		DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		//
		DISPLC_MODCOUNT
	};
	#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
	#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
	static const FLASHMEM struct dzone dzones [] =
	{
		{	0, 0,	display_freqchr_a,	REDRM_FREQ, REDRSUBSET_ALL, },	// ������� ��� ���������� ��������
	};

#elif DSTYLE_T_X16_Y2 && CTLSTYLE_RA4YBO_AM0

	enum
	{
		BDTH_ALLRX = 15,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_RIGHTRX = 5,	// ������ ���������� ������
		BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
		//
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 7,
		BDTH_SPACESWR = 1,
		BDTH_ALLPWR = 7,
		BDTH_SPACEPWR = 0
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX
	#endif /* WITHSHOWSWRPWR */
	};
	#define SWRMAX	40	// 4.0 - �������� �� ������ �����
	enum {
		DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		//
		DISPLC_MODCOUNT
	};

	#define DISPLC_WIDTH	6	// ���������� ���� � ����������� �������
	#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
	static const FLASHMEM struct dzone dzones [] =
	{
		{	0, 0,	display_freqchr_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },	// ������� ��� ���������� ��������
		{	8, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
		{	13, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
		{	1, 1,	display2_bars_amv0,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
	#if WITHMENU 
		{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
	#endif /* WITHMENU */
	};

#elif DSTYLE_T_X16_Y2

	enum
	{
		BDTH_ALLRX = 16,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_RIGHTRX = 5,	// ������ ���������� ������
		BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
		//
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 7,
		BDTH_SPACESWR = 1,
		BDTH_ALLPWR = 8,
		BDTH_SPACEPWR = 0
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX
	#endif /* WITHSHOWSWRPWR */
	};
	#define SWRMAX	40	// 4.0 - �������� �� ������ �����
	enum {
		DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		DPAGE_SMETER,			// �������� � ������������ S-�����, SWR-�����
	#if WITHUSEAUDIOREC
		//DPAGE_SDCARD,
	#endif /* WITHUSEAUDIOREC */
	#if WITHUSEDUALWATCH
		DPAGE_SUBRX,
	#endif /* WITHUSEDUALWATCH */
	#if defined (RTC1_TYPE)
		DPAGE_TIME,
	#endif /* defined (RTC1_TYPE) */
	#if WITHMODEM
		DPAGE_BPSK,
	#endif /* WITHMODEM */
	#if WITHVOLTLEVEL && WITHCURRLEVEL 
		DPAGE_VOLTS,
	#endif /* WITHVOLTLEVEL && WITHCURRLEVEL */
		//
		DISPLC_MODCOUNT
	};
	#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
	#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
	static const FLASHMEM struct dzone dzones [] =
	{
		{	0, 0,	display_freqchr_a,	REDRM_FREQ, REDRSUBSET_ALL, },	// ������� ��� ���������� ��������
		{	8, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET_ALL, },
		{	9, 0,	display_mode3_a,	REDRM_MODE,	REDRSUBSET_ALL, },	// SSB/CW/AM/FM/...
		{	13, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET_ALL, },

		{	0, 1,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
		{	4, 1,	display_att_tx3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// ��� ������� s-metre, ��� �������� ���������� TX
	#if WITHDSPEXTDDC
		{	9, 1,	display_preovf3,	REDRM_BARS, REDRSUBSET(DPAGE0), },	// ovf/pre
	#else /* WITHDSPEXTDDC */
		{	9, 1,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// pre
	#endif /* WITHDSPEXTDDC */
#if WITHIF4DSP
	#if WITHUSEAUDIOREC
		{	13, 1,	display_rec3,		REDRM_BARS, REDRSUBSET(DPAGE0) /*| REDRSUBSET(DPAGE_SMETER)*/, },	// ����������� ������ ������ ����� ���������
	#endif /* WITHUSEAUDIOREC */
#else /* WITHIF4DSP */
		{	13, 1,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0) /*| REDRSUBSET(DPAGE_SMETER)*/, },
#endif /* WITHIF4DSP */

		{	0, 1,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE_SMETER), },	// S-METER, SWR-METER, POWER-METER

	#if WITHVOLTLEVEL && WITHCURRLEVEL
		{	0, 1,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE_VOLTS), },	// voltmeter with "V"
		{	6, 1,	display_currlevelA6, REDRM_VOLT, REDRSUBSET(DPAGE_VOLTS), },	// amphermeter with "A"
	#endif /* WITHVOLTLEVEL && WITHCURRLEVEL */
	#if WITHMODEM
		{	0, 1,	display_freqdelta8, REDRM_BARS, REDRSUBSET(DPAGE_BPSK), },	// ����� �� ������������
	#endif /* WITHMODEM */
	#if WITHUSEDUALWATCH
		{	0, 1,	display_freqchr_b,	REDRM_FREQ, REDRSUBSET(DPAGE_SUBRX), },	// FREQ B
		{	9, 1,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE_SUBRX), },	// SPLIT
		{	13, 1,	display_mainsub3, REDRM_MODE, REDRSUBSET(DPAGE_SUBRX), },	// main/sub RX
	#endif /* WITHUSEDUALWATCH */

	//#if WITHUSEAUDIOREC
	//	{	0, 1,	display_recstatus,	REDRM_BARS, REDRSUBSET(DPAGE_SDCARD), },	// recording debug information
	//	{	13, 1,	display_rec3,		REDRM_BARS, REDRSUBSET(DPAGE_SDCARD), },	// ����������� ������ ������ ����� ���������
	//#endif /* WITHUSEAUDIOREC */

	#if defined (RTC1_TYPE)
		{	0, 1,	display_time8,		REDRM_BARS, REDRSUBSET(DPAGE_TIME), },	// TIME
	#if WITHUSEDUALWATCH
		{	9, 1,	display_mainsub3,	REDRM_BARS, REDRSUBSET(DPAGE_TIME), },	// main/sub RX
	#endif /* WITHUSEDUALWATCH */
	#if WITHUSEAUDIOREC
		{	13, 1,	display_rec3,		REDRM_BARS, REDRSUBSET(DPAGE_TIME), },	// ����������� ������ ������ ����� ���������
	#endif /* WITHUSEAUDIOREC */
	#endif /* defined (RTC1_TYPE) */


		//{	0, 0,	display_txrxstate2,	REDRM_MODE, REDRSUBSET_ALL, },
		//{	0, 0,	display_voxtune3,	REDRM_MODE, REDRSUBSET_ALL, },
	#if WITHMENU
		{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	15, 0,	display_lockstate1,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
	#endif /* WITHMENU */
	};

#elif DSTYLE_T_X16_Y4

	enum
	{
		BDTH_ALLRX = 14,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_RIGHTRX = 5,	// ������ ���������� ������
		BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
		//
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 9,
		BDTH_SPACESWR = 1,
		BDTH_ALLPWR = 4,
		BDTH_SPACEPWR = 0
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX
	#endif /* WITHSHOWSWRPWR */
	};
	#define SWRMAX	40	// 4.0 - �������� �� ������ �����

	enum {
		DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		DISPLC_MODCOUNT
	};
	#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
	#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
	static const FLASHMEM struct dzone dzones [] =
	{
		{	0, 0,	display_freqchr_a,	REDRM_FREQ, REDRSUBSET_ALL, },	// ������� ��� ���������� ��������
		{	8, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET_ALL, },
		{	9, 0,	display_mode3_a,	REDRM_MODE,	REDRSUBSET_ALL, },	// SSB/CW/AM/FM/...
		{	13, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET_ALL, },

		{	0, 1,	display_vfomode3,	REDRM_MODE, REDRSUBSET_ALL, },	// SPLIT
		{	4, 1,	display_att_tx3,	REDRM_MODE, REDRSUBSET_ALL, },		// ��� �������� ���������� TX
	#if WITHDSPEXTDDC
		{	9, 1,	display_preovf3,	REDRM_BARS, REDRSUBSET_ALL, },	// ovf/pre
	#else /* WITHDSPEXTDDC */
		{	9, 1,	display_pre3,		REDRM_MODE, REDRSUBSET_ALL, },	// pre
	#endif /* WITHDSPEXTDDC */

		{	0, 2,	display2_bars,		REDRM_BARS, REDRSUBSET_ALL, },	// S-METER, SWR-METER, POWER-METER

		{	0, 3,	display_voxtune3,	REDRM_MODE, REDRSUBSET_ALL, },
	#if defined (RTC1_TYPE)
		{	4, 3,	display_time8,		REDRM_BARS, REDRSUBSET_ALL, },	// TIME
	#endif /* defined (RTC1_TYPE) */
		{	13, 3,	display_agc3,		REDRM_MODE, REDRSUBSET_ALL, },

		//{	0, 0,	display_txrxstate2,	REDRM_MODE, REDRSUBSET_ALL, },
		//{	0, 0,	display_voxtune3,	REDRM_MODE, REDRSUBSET_ALL, },

	#if WITHMENU
		{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ��������� ���������
		{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	15, 0,	display_lockstate1,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
	#endif /* WITHMENU */
	};

#elif DSTYLE_G_X64_Y32
	// RDX0120
	#define CHAR_W	6
	#define CHAR_H	8
	#define SMALLCHARH 8 /* Font height */

	enum
	{
		BDTH_ALLRX = 9,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_RIGHTRX = 3,	// ������ ���������� ������
		BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
		//
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX
	};
	enum
	{
		PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
		PATTERN_BAR_FULL = 0x7e,	//0x7C,	// ������ ��� - ������
		PATTERN_BAR_HALF = 0x1e,	//0x38,	// ������ ��� - ������
		PATTERN_BAR_EMPTYFULL = 0x42,	//0x00
		PATTERN_BAR_EMPTYHALF = 0x12	//0x00
	};
	#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

	enum {
		DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		DISPLC_MODCOUNT
	};
	#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
	#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
	static const FLASHMEM struct dzone dzones [] =
	{
		{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
		{	3, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
		{	7, 0,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

		{	0, 1,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },

		{	9, 2,	display_vfomode1,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT - ����� ��������
		{	0, 2,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER

		{	6, 3,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
		{	9, 3,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },
		{	0, 3,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
	#if WITHMENU 
		{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0, 3,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	0, 2,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	9, 3,	display_lockstate1,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
	#endif /* WITHMENU */
	};

#elif DSTYLE_G_X128_Y64
	// RDX0077
	#define CHAR_W	6
	#define CHAR_H	8
	#define SMALLCHARH 8 /* Font height */

	#if DSTYLE_UR3LMZMOD && WITHONEATTONEAMP

		enum
		{
			BDTH_ALLRX = 21,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 11,	// ������ ���������� ������ 
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 11,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		//					  "012345678901234567890"
		#define SMETERMAP 	  " 1 3 5 7 9 + 20 40 60"	//
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			#define SWRPWRMAP "1 | 2 | 3 0%   | 100%" 
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#else
			#define POWERMAP  " 0 10 20 40 60 80 100"
			#define SWRMAP    "1    |    2    |   3 "	// 
			#define SWRMAX	(SWRMIN * 31 / 10)	// 3.1 - �������� �� ������ �����
		#endif
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
		#if 1
			PATTERN_BAR_FULL = 0x7e,
			PATTERN_BAR_HALF = 0x7e,
			PATTERN_BAR_EMPTYFULL = 0x42,
			PATTERN_BAR_EMPTYHALF = 0x42
		#else
			PATTERN_BAR_FULL = 0x7e,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x42,
			PATTERN_BAR_EMPTYHALF = 0x24
		#endif
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },	// TX/RX
			{	3, 0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },		// VOX/TUN
			{	7, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	10, 0,	display_hplp2,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	13, 0,	display_voltlevel4, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter without "V"
			{	18, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },


			{	0,	1,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	18, 2,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	2, 4,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	7, 4,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	18, 4,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...


			{	2, 5,	display_lockstate4, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	7, 5,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	11, 5,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },

			{	0, 6,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
			{	0, 7,	display2_legend,	REDRM_MODE, REDRSUBSET(DPAGE0), },// ����������� ��������� ����� S-�����
		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#else /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */

		enum
		{
			BDTH_ALLRX = 17,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 6,	// ������ ���������� ������ 
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 10,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 6,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		//#define SMETERMAP "1 3 5 7 9 + 20 40 60"
		//#define POWERMAP  "0 10 20 40 60 80 100"
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0x7e,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x42,
			PATTERN_BAR_EMPTYHALF = 0x24
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0,	0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3,	0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	7,	0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	16, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	18, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	12, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	0,	2,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },		// col = 1 for !WIDEFREQ
			{	18, 3,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			{	18, 5,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			{	0,	5,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	7,	5,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },		// x=8 then !WIDEFREQ
			{	18, 7,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	0,	7,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#endif /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */

#elif DSTYLE_G_X132_Y64
	// RDX0154
	// �� ����������� 22 ���������� (x=0..21)
	#define CHAR_W	6
	#define CHAR_H	8
	#define SMALLCHARH 8 /* Font height */

	#if DSTYLE_UR3LMZMOD && WITHONEATTONEAMP
		// SW20XXX 
		enum
		{
			BDTH_ALLRX = 21,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 11,	// ������ ���������� ������ 
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 11,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		//					  "0123456789012345678901"
		#define SMETERMAP 	  " 1 3 5 7 9 + 20 40 60 "	//
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			#define SWRPWRMAP "1 | 2 | 3 0%   | 100% " 
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#else
			#define POWERMAP  " 0 10 20 40 60 80 100 "
			#define SWRMAP    "1    |    2    |   3  "	// 
			#define SWRMAX	(SWRMIN * 32 / 10)	// 3.2 - �������� �� ������ �����
		#endif
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
		#if 1
			PATTERN_BAR_FULL = 0x7e,
			PATTERN_BAR_HALF = 0x7e,
			PATTERN_BAR_EMPTYFULL = 0x42,
			PATTERN_BAR_EMPTYHALF = 0x42
		#else
			PATTERN_BAR_FULL = 0x7e,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x42,
			PATTERN_BAR_EMPTYHALF = 0x24
		#endif
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		// �� ����������� 22 ���������� (x=0..21)
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0,	0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },	// TX/RX
			{	3,	0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// VOX/TUN
			{	7,	0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// PRE/ATT/___
			{	11, 0,	display_lockstate4, REDRM_MODE, REDRSUBSET(DPAGE0), },	// LOCK
			{	19, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// 3.1

			{	0,	1,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	19, 2,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/..
			//
			{	2,	4,	display_vfomode5,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	8,	4,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	19, 4,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0,	5,	display_hplp2,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// HP/LP
			{	3,	5,	display_voltlevelV5,REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter with "V"
		#if WITHANTSELECT
			{	9,	5,	display_ant5,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// ANTENNA
		#endif /* WITHANTSELECT */
		#if defined (RTC1_TYPE)
			{	9,	5,	display_time5,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME
		#endif /* defined (RTC1_TYPE) */
		#if WITHTX && WITHAUTOTUNER
			{	15, 5,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// ATU
			{	19, 5,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// BYP
		#endif /* WITHTX && WITHAUTOTUNER */

			{	0,	6,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
			{	0,	7,	display2_legend,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// ����������� ��������� ����� S-�����
		#if WITHMENU
			{	0,	0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0,	1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4,	1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#else /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */

		enum
		{
			BDTH_ALLRX = 17,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 6,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 8,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 8,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x00,	//0x00
			PATTERN_BAR_EMPTYHALF = 0x00	//0x00
		};
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3, 0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	7, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	12, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	16, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	19, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },

			{	0, 2,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },	// x=1 then !WIDEFREQ
			{	19, 3,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0, 5,	display_voltlevel4, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter without "V"
			{	0, 6,	display_currlevelA6, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// amphermeter with "A"
			{	13, 5,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	16, 5,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },	// x=9 then !WIDEFREQ
			{	19, 5,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0, 6,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	4, 6,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },

	#if WITHIF4DSP
		#if WITHUSEAUDIOREC
			{	19, 7,	display_rec3,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// ����������� ������ ������ ����� ���������
		#endif /* WITHUSEAUDIOREC */
	#else /* WITHIF4DSP */
			{	19, 7,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
	#endif /* WITHIF4DSP */
	#if WITHUSEDUALWATCH
			{	19, 6,	display_mainsub3, REDRM_BARS, REDRSUBSET(DPAGE0), },	// main/sub RX
	#endif /* WITHUSEDUALWATCH */

			{	0, 7,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER

		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 1,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 1,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#endif /* DSTYLE_UR3LMZMOD */

#elif DSTYLE_G_X160_Y128
	// TFT-1.8, LCDMODE_ST7735 for example
	// CHAR_W=8, CHAR_H=8, SMALLCHARH=16
	#define CHAR_W	8
	#define CHAR_H	8
	#define SMALLCHARH 16 /* Font height */

	#if DSTYLE_UR3LMZMOD && WITHONEATTONEAMP
		// ������������ � �������� ������� sw2013 mini - CTLSTYLE_SW2012CN � CTLSTYLE_SW2012CN5

		enum
		{
			BDTH_ALLRX = 20,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 11,	// ������ ���������� ������ 
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 10,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		#define SMETERMAP "1 3 5 7 9 + 20 40 60"	//
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			//                "01234567890123456789"
			#define SWRPWRMAP "1 | 2 | 3 0%  | 100%"
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#else /* WITHSHOWSWRPWR */
			#define POWERMAP  "0 10 20 40 60 80 100"
			#define SWRMAP    "1    |    2    |   3"	// 
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#endif /* WITHSHOWSWRPWR */
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
		#if 1
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0xFF,
			PATTERN_BAR_EMPTYFULL = 0x81,
			PATTERN_BAR_EMPTYHALF = 0x81
		#else
			PATTERN_BAR_FULL = 0x7e,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x42,
			PATTERN_BAR_EMPTYHALF = 0x24
		#endif
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3, 0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	7, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	12, 0,	display_lockstate4, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	17, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			//{	0, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			//{	0, 0,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },

			{	0,	3,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	17, 5,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0,	8,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	7,	8,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	17, 8,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

		#if defined (RTC1_TYPE)
			{	0,	11,	display_time5,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME
			//{	0,	11,	display_time8,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME
		#endif /* defined (RTC1_TYPE) */
			{	0,	11,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	4,	11,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	15, 11,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter with "V"

			{	0, 13,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
			{	0, 14,	display2_legend,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// ����������� ��������� ����� S-�����

		#if WITHMENU
			{	4, 0,	display_menu_group,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� ������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblng,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	0, 4,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */

		};

	#else /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */

		enum
		{
			BDTH_ALLRX = 15,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 5,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 7,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 7,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x81,
			PATTERN_BAR_EMPTYHALF = 0x24
		};
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0,	0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3,	0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	7,	0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	12, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	17, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	16, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },

			{	0,	4,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	17, 6,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0,	9,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	7,	9,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	17, 9,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

		#if defined (RTC1_TYPE)
			{	0,	12,	display_time5,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME
			//{	0,	12,	display_time8,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME
		#endif /* defined (RTC1_TYPE) */
			{	0,	12,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	4,	12,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	8,	12,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter with "V"

			{	16, 13,	display_agc4,		REDRM_MODE, REDRSUBSET(DPAGE0), },

			{	0, 14,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
		#if WITHMENU
			{	4, 0,	display_menu_group,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� ������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblng,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	0, 4,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#endif /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */

#elif DSTYLE_G_X176_Y132
	// Siemens S65 LS020, Siemens S65 LPH88
	#define CHAR_W	8
	#define CHAR_H	8
	#define SMALLCHARH 16 /* Font height */

	#if DSTYLE_UR3LMZMOD && WITHONEATTONEAMP
		// SW20XXX - ������ ���� ��������� ����������� � ��� - "�� �����"
		enum
		{
			BDTH_ALLRX = 20,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 11,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 10,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		#define SMETERMAP     "1 3 5 7 9 + 20 40 60"
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			//                "01234567890123456789"
			#define SWRPWRMAP "1 | 2 | 3 0%  | 100%"
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#else /* WITHSHOWSWRPWR */
			#define SWRMAP    "1    |    2    |   3"	// 
			#define POWERMAP  "0 10 20 40 60 80 100"
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#endif /* WITHSHOWSWRPWR */
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0x3c,
			//PATTERN_BAR_FULL = 0x7e,
			//PATTERN_BAR_HALF = 0x7e /* 0x3c */,
			PATTERN_BAR_EMPTYFULL = 0x00,
			PATTERN_BAR_EMPTYHALF = 0x00
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			//{	0, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3, 0,	display_hplp2,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	6, 0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	10, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	14, 0,	display_lockstate4, REDRM_MODE, REDRSUBSET(DPAGE0), },
			//{	0, 0,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	19, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	0, 2,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter with "V"
			{	6, 2,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	10, 2,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	17, 3,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			{	0,	5,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	0, 10,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	17, 10,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			{	5, 10,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	0, 13,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },		// S-METER, SWR-METER, POWER-METER
			{	1, 14,	display2_legend,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// ����������� ��������� ����� S-�����
		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#elif DSTYLE_UR3LMZMOD
		// ���������� ��� � ������������� ������������
		// ����������� ���
		// ��� CTLSTYLE_RA4YBO_V1

		enum
		{
			BDTH_ALLRX = 20,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 11,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 10,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		#define SMETERMAP     "1 3 5 7 9 + 20 40 60"
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			//                "01234567890123456789"
			#define SWRPWRMAP "1 | 2 | 3 0%  | 100%"
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#else /* WITHSHOWSWRPWR */
			#define SWRMAP    "1    |    2    |   3"	// 
			#define POWERMAP  "0 10 20 40 60 80 100"
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#endif /* WITHSHOWSWRPWR */
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0x3c,
			//PATTERN_BAR_FULL = 0x7e,
			//PATTERN_BAR_HALF = 0x7e /* 0x3c */,
			PATTERN_BAR_EMPTYFULL = 0x00,
			PATTERN_BAR_EMPTYHALF = 0x00
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3, 0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	7, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },		// 4 chars - 12dB, 18dB
			{	12, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	16, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	18, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },

			{	0, 2,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter with "V"
			{	6, 2,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	10, 2,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	18, 2,	display_agc4,		REDRM_MODE, REDRSUBSET(DPAGE0), },

			{	0,	6,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	18, 4,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0, 10,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	4, 10,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	16, 10,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0, 13,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },		// S-METER, SWR-METER, POWER-METER
			{	1, 14,	display2_legend,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// ����������� ��������� ����� S-�����
		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};


	#else /* DSTYLE_UR3LMZMOD */

		enum
		{
			BDTH_ALLRX = 17,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 6,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 8,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 8,
			BDTH_SPACEPWR = 0,
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX,
		#endif /* WITHSHOWSWRPWR */
			BDCV_ALLRX = ROWS2GRID(1),		// ���������� ������, ���������� ��� S-����, ��������, ���� �����������
			BDCV_SPMRX = BDCV_ALLRX,	// ������������ ������ ������� � �������		};
			BDCV_WFLRX = BDCV_ALLRX,	// ������������ ������ �������� � �������		};
			BDCO_SPMRX = 0,	// �������� ������� �� ��������� � ������� �� ������ ������ ����
			BDCO_WFLRX = 0	// �������� �������� �� ��������� � ������� �� ������ ������ ����
		};
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x00,	//0x00
			PATTERN_BAR_EMPTYHALF = 0x00	//0x00
		};
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0,	0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3,	0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	7,	0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	12, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	16, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	19, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	0,	2,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter with "V"
			{	6,	2,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	10, 2,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	0,	4,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	19, 4,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			{	0,	9,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	8,	9,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	19, 9,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			{	0,	14,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
			{	18, 14,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#endif /* DSTYLE_UR3LMZMOD */

#elif DSTYLE_G_X220_Y176
	// ��������� 220*176 SF-TC220H-9223A-N_IC_ILI9225C_2011-01-15 � ������������ ILI9225�
	// x= 27 characters
	#define CHAR_W	8
	#define CHAR_H	8
	#define SMALLCHARH 16 /* Font height */

	#if DSTYLE_UR3LMZMOD && WITHONEATTONEAMP

		enum
		{
			BDTH_ALLRX = 27,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 14,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 16,
			BDTH_SPACEPWR = 1
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		// 27 horisontal places
		//                    "012345678901234567890123456"
			#define SMETERMAP "1  3  5  7  9 + 20  40  60 "
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
			#define SWRPWRMAP "1 | 2 | 3 0%     |    100% "
		#else
			#define POWERMAP  "0  10  20  40  60  80  100"
			#define SWRMAP    "1    -    2    -    3    -"	// 
			#define SWRMAX	(SWRMIN * 36 / 10)	// 3.6 - �������� �� ������ �����
		#endif
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0xFF,
			PATTERN_BAR_EMPTYFULL = 0x81,	//0x00
			PATTERN_BAR_EMPTYHALF = 0x81	//0x00
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3, 0,	display_hplp2,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			//{	6, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	10, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			//{	0, 0,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	20, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	24, 0,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			
			{	0, 5,	display_freqXbig_a, REDRM_FREQ, REDRSUBSET(DPAGE0), },

			{	0, 11,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	11, 11, display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	24, 11,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0, 14,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
			{	0, 17,	display2_legend,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// ����������� ��������� ����� S-�����

			{	0, 20,	display_lockstate4, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	9, 20,	display_voxtune4,	REDRM_MODE, REDRSUBSET(DPAGE0), },
		#if defined (RTC1_TYPE)
			{	14, 20,	display_time5,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME HH:MM
		#endif /* defined (RTC1_TYPE) */
			{	14, 20,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	18, 20,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	22, 20,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter
		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#elif DSTYLE_UR3LMZMOD
		// KEYBSTYLE_SW2013SF_US2IT
		enum
		{
			BDTH_ALLRX = 27,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 14,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 16,
			BDTH_SPACEPWR = 1
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		// 27 horisontal places
		//                    "012345678901234567890123456"
			#define SMETERMAP "1  3  5  7  9 + 20  40  60 "
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
			#define SWRPWRMAP "1 | 2 | 3 0%     |    100% "
		#else
			#define POWERMAP  "0  10  20  40  60  80  100"
			#define SWRMAP    "1    -    2    -    3    -"	// 
			#define SWRMAX	(SWRMIN * 36 / 10)	// 3.6 - �������� �� ������ �����
		#endif
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0xFF,
			PATTERN_BAR_EMPTYFULL = 0x81,	//0x00
			PATTERN_BAR_EMPTYHALF = 0x81	//0x00
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3, 0,	display_hplp2,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	6, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	10, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			//{	0, 0,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	20, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	24, 0,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			
			{	0, 5,	display_freqXbig_a, REDRM_FREQ, REDRSUBSET(DPAGE0), },

			{	0, 11,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	11, 11, display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	24, 11,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0, 14,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
			{	0, 16,	display2_legend,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// ����������� ��������� ����� S-�����

		#if defined (RTC1_TYPE)
			{	0, 18,	display_datetime12,	REDRM_BARS, REDRSUBSET(DPAGE0), },	// DATE&TIME Jan 01 13:40
			//{	0, 18,	display_time5,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME HH:MM
		#endif /* defined (RTC1_TYPE) */
			{	0, 20,	display_lockstate4, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	9, 20,	display_voxtune4,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	14, 20,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	18, 20,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	22, 20,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter
		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#else /* DSTYLE_UR3LMZMOD */

		enum
		{
			BDTH_ALLRX = 17,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 6,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 8,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 8,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x00,	//0x00
			PATTERN_BAR_EMPTYHALF = 0x00	//0x00
		};
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	7	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	4, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	10, 0,	display_pre3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	19, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	15, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	23, 0,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0, 4,	display_freqX_a,	REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	19, 10,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
			{	0, 10,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	8, 10,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	19, 13,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	0, 14,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER

		#if defined (RTC1_TYPE)
			{	0, 20,	display_time8,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME
		#endif /* defined (RTC1_TYPE) */
			{	10, 20,	display_voxtune4,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	15, 20,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	19, 20,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	23, 20,	display_voltlevel4, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter without "V"
		#if WITHMENU
			{	0, 0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#endif /* DSTYLE_UR3LMZMOD */

#elif DSTYLE_G_X240_Y128
	// WO240128A
	#define CHAR_W	8
	#define CHAR_H	8
	#define SMALLCHARH 16 /* Font height */

	#if DSTYLE_UR3LMZMOD && WITHONEATTONEAMP
		// x=30, y=16

		enum
		{
			BDTH_ALLRX = 30,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_LEFTRX = 15,	// ������ ���������� ������
			BDTH_RIGHTRX = BDTH_ALLRX - BDTH_LEFTRX,	// ������ ���������� ������ 
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 20,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		//					  "012345678901234567890123456789"
		#define SMETERMAP 	  "S 1  3  5  7  9  +20  +40  +60"	//
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			#define SWRPWRMAP "1 | 2 | 3 0%      50%     100%" 
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		#else
			#error Not designed for work DSTYLE_UR3LMZMOD without WITHSHOWSWRPWR
			//#define POWERMAP  " 0 10 20 40 60 80 100"
			//#define SWRMAP    "1    |    2    |   3 "	// 
			//#define SWRMAX	(SWRMIN * 31 / 10)	// 3.1 - �������� �� ������ �����
		#endif
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0xFF,
			PATTERN_BAR_EMPTYFULL = 0x81,
			PATTERN_BAR_EMPTYHALF = 0x81
		};
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
		/* ---------------------------------- */
			{	0,	0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3,	0,	display_ant5,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	9,	0,	display_hplp2,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	16, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },	// attenuator state
			{	21, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	25,	0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	27, 0,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
		/* ---------------------------------- */
			{	0,	2,	display_freqXbig_a, REDRM_FREQ, REDRSUBSET(DPAGE0), },	// fullwidth = 8 constantly
		/* ---------------------------------- */
			//{	0,	8,	display_mainsub3, REDRM_MODE, REDRSUBSET(DPAGE0), },	// main/sub RX
			{	4,	8,	display_vfomode3,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPL
			{	12, 8,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	27, 8,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...
		/* ---------------------------------- */
			{	0,	11,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
			{	0,	12,	display2_legend,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// ����������� ��������� ����� S-�����
			/* ---------------------------------- */
			{	0,	14,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
	#if WITHNOTCHONOFF || WITHNOTCHFREQ
			// see next {	4,	14,	display_notch5,		REDRM_MODE, REDRSUBSET(DPAGE0), },
	#endif /* WITHNOTCHONOFF || WITHNOTCHFREQ  */
	#if WITHAUTOTUNER
			{	4,	14,	display_atu3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	8,	14,	display_byp3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
	#endif /* WITHAUTOTUNER  */
		/* ---------------------------------- */
	#if WITHVOLTLEVEL
			{	12,	14,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET(DPAGE0), },	// voltmeter with "V"
	#endif /* WITHVOLTLEVEL  */
	#if defined (RTC1_TYPE)
			//{	18,	14,	display_time8,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// TIME
			{	18,	14,	display_datetime12,	REDRM_BARS, REDRSUBSET(DPAGE0), },	// DATE&TIME Jan 01 13:40
	#endif /* defined (RTC1_TYPE) */
		#if WITHNOTCHONOFF || WITHNOTCHFREQ
			{	27, 14,	display_notch3, REDRM_MODE, PG0, },	// 3.7
		#endif /* WITHNOTCHONOFF || WITHNOTCHFREQ */
		/* ---------------------------------- */
	#if WITHMENU
			{	0,	0,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0,	2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4,	2,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#if WITHVOLTLEVEL && WITHCURRLEVEL
			{	0,	9,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET_MENU, },	// voltmeter with "V"
			{	6,	9,	display_currlevelA6, REDRM_VOLT, REDRSUBSET_MENU, },	// amphermeter with "A"
		#endif /* WITHVOLTLEVEL && WITHCURRLEVEL */
	#endif /* WITHMENU */
		};

	#elif DSTYLE_UR3LMZMOD
		// Versopn with specreum display
		// x=30, y=16
		enum
		{
			BDCV_ALLRX = 7,			// ���������� ������, ���������� ��� S-����, ��������, ���� �����������
			BDCV_SPMRX = BDCV_ALLRX,	// ������������ ������ ������� � �������		};
			BDCV_WFLRX = BDCV_ALLRX,	// ������������ ������ �������� � �������		};
			BDCO_SPMRX = 0,	// �������� ������� �� ��������� � ������� �� ������ ������ ����
			BDCO_WFLRX = 0,	// �������� �������� �� ��������� � ������� �� ������ ������ ����

			BDTH_ALLRX = 30,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_LEFTRX = 15,	// ������ ���������� ������
			BDTH_RIGHTRX = BDTH_ALLRX - BDTH_LEFTRX,	// ������ ���������� ������ 
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 20,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		//					  "012345678901234567890123456789"
		#define SMETERMAP 	  "S 1  3  5  7  9  +20  +40  +60"	//
		//#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			#define SWRPWRMAP "1 | 2 | 3 0%      50%     100%" 
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		//#else
		//	#error Not designed for work DSTYLE_UR3LMZMOD without WITHSHOWSWRPWR
			//#define POWERMAP  " 0 10 20 40 60 80 100"
			//#define SWRMAP    "1    |    2    |   3 "	// 
			//#define SWRMAX	(SWRMIN * 31 / 10)	// 3.1 - �������� �� ������ �����
		//#endif
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0xFF,
			PATTERN_BAR_EMPTYFULL = 0x81,
			PATTERN_BAR_EMPTYHALF = 0x81
		};
		//#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DPAGE1,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		enum
		{
			PG0 = REDRSUBSET(DPAGE0),
			PG1 = REDRSUBSET(DPAGE1),
			PGALL = PG0 | PG1 | REDRSUBSET_MENU,
			PGLATCH = PGALL,
			PGunused
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
		/* ---------------------------------- */
			{	0,	0,	display_txrxstate2, REDRM_MODE, PGALL, },
			{	3,	0,	display_ant5,		REDRM_MODE, PGALL, },
			{	9,	0,	display_ovf3,		REDRM_BARS, PGALL, },	// ovf
			{	14,	0,	display_pre3,		REDRM_MODE, PGALL, },	// pre
			{	18, 0,	display_att4,		REDRM_MODE, PGALL, },	// attenuator state
			{	22,	0,	display_lockstate1, REDRM_MODE, PGALL, },
			{	23, 0,	display_rxbw3,		REDRM_MODE, PGALL, },
			{	27, 0,	display_mode3_a,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
		/* ---------------------------------- */
			{	0,	2,	display_freqXbig_a, REDRM_FREQ, PGALL, },	// fullwidth = 8 constantly
		#if ! WITHDSPEXTDDC
			{	27, 2,	display_agc3,		REDRM_MODE, PGALL, },
		#endif /* ! WITHDSPEXTDDC */
			{	27, 4,	display_voxtune3,	REDRM_MODE, PGALL, },
		/* ---------------------------------- */
		#if WITHUSEAUDIOREC
			{	0,	7,	display_rec3,		REDRM_BARS, PGALL, },	// ����������� ������ ������ ����� ���������
		#endif /* WITHUSEAUDIOREC */
			{	4,	7,	display_mainsub3, REDRM_MODE, PGALL, },	// main/sub RX
			{	8,	7,	display_vfomode3,	REDRM_MODE, PGALL, },	// SPL
			{	16, 7,	display_freqX_b,	REDRM_FRQB, PGALL, },
			{	27, 7,	display_mode3_b,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
		/* ---------------------------------- */
			{	0,	9,	display2_bars,		REDRM_BARS, PG0, },	// S-METER, SWR-METER, POWER-METER
			{	0,	10,	display2_legend,	REDRM_MODE, PG0, },	// ����������� ��������� ����� S-�����
			/* ---------------------------------- */
			{	0,	9,	dsp_latchwaterfall,	REDRM_BARS,	PGLATCH, },	// ������������ ������ ������� ��� ������������ ����������� ������� ��� ��������
			{	0,	9,	display2_spectrum,	REDRM_BARS, PG1, },// ���������� ����������� �������
			{	0,	9,	display2_colorbuff,	REDRM_BARS,	PG1, },// ����������� �������� �/��� �������
			/* ---------------------------------- */
		#if defined (RTC1_TYPE)
			{	0,	14,	display_time5,		REDRM_BARS, PG0, },	// TIME
		#endif /* defined (RTC1_TYPE) */
		#if WITHVOLTLEVEL
			{	6,	14,	display_voltlevelV5, REDRM_VOLT, PG0 | REDRSUBSET_MENU, },	// voltmeter with "V"
		#endif /* WITHVOLTLEVEL  */
		#if WITHCURRLEVEL
			{	11, 14,	display_currlevelA6, REDRM_VOLT, PG0 | REDRSUBSET_MENU, },	// amphermeter with "A"
		#endif /*  WITHCURRLEVEL */
		#if WITHAMHIGHKBDADJ
			{	6, 14,	display_amfmhighcut4,REDRM_MODE, PG0, },	// 3.70
		#endif /* WITHAMHIGHKBDADJ */
			{	18, 14,	display_samfreqdelta8, REDRM_BARS, PG0 | REDRSUBSET_MENU, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
		#if WITHNOTCHONOFF || WITHNOTCHFREQ
			{	27, 14,	display_notch3, REDRM_MODE, PG0, },	// 3.7
		#endif /* WITHNOTCHONOFF || WITHNOTCHFREQ */
		/* ---------------------------------- */
	#if WITHMENU
			{	0,	9,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0,	11,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4,	11,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			//{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#if WITHVOLTLEVEL && WITHCURRLEVEL
			//{	0,	9,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET_MENU, },	// voltmeter with "V"
			//{	6,	9,	display_currlevelA6, REDRM_VOLT, REDRSUBSET_MENU, },	// amphermeter with "A"
		#endif /* WITHVOLTLEVEL && WITHCURRLEVEL */
	#endif /* WITHMENU */
		};

	#else /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */
		// RA1AGO version
		// x=30, y=16

		enum
		{
			BDCV_ALLRX = 7,			// ���������� ������, ���������� ��� S-����, ��������, ���� �����������
			BDCV_SPMRX = BDCV_ALLRX,	// ������������ ������ ������� � �������		};
			BDCV_WFLRX = BDCV_ALLRX,	// ������������ ������ �������� � �������		};
			BDCO_SPMRX = 0,	// �������� ������� �� ��������� � ������� �� ������ ������ ����
			BDCO_WFLRX = 0,	// �������� �������� �� ��������� � ������� �� ������ ������ ����
			BDTH_ALLRX = 30,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_LEFTRX = 15,	// ������ ���������� ������
			BDTH_RIGHTRX = BDTH_ALLRX - BDTH_LEFTRX,	// ������ ���������� ������ 
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 9,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 20,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		//					  "012345678901234567890123456789"
		#define SMETERMAP 	  "S 1  3  5  7  9  +20  +40  +60"	//
		//#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			#define SWRPWRMAP "1 | 2 | 3 0%      50%     100%" 
			#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		//#else
		//	#error Not designed for work DSTYLE_UR3LMZMOD without WITHSHOWSWRPWR
			//#define POWERMAP  " 0 10 20 40 60 80 100"
			//#define SWRMAP    "1    |    2    |   3 "	// 
			//#define SWRMAX	(SWRMIN * 31 / 10)	// 3.1 - �������� �� ������ �����
		//#endif
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0xFF,
			PATTERN_BAR_EMPTYFULL = 0x81,
			PATTERN_BAR_EMPTYHALF = 0x81
		};
		//#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		#if WITHDSPEXTDDC
			DPAGE1,					// ��������, � ������� ������������ �������� (��� ���) 
		#endif /* WITHDSPEXTDDC */
			DISPLC_MODCOUNT
		};
		enum
		{
			PG0 = REDRSUBSET(DPAGE0),
		#if WITHDSPEXTDDC
			PG1 = REDRSUBSET(DPAGE1),
			PGALL = PG0 | PG1 | REDRSUBSET_MENU,
		#else /* WITHDSPEXTDDC */
			PGALL = PG0 | REDRSUBSET_MENU,
		#endif /* WITHDSPEXTDDC */
			PGLATCH = PGALL,
			PGunused
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
		/* ---------------------------------- */
			{	0,	0,	display_txrxstate2, REDRM_MODE, PGALL, },
			{	3,	0,	display_ant5,		REDRM_MODE, PGALL, },
		#if WITHDSPEXTDDC
			{	9,	0,	display_ovf3,		REDRM_BARS, PGALL, },	// ovf
			{	14,	0,	display_pre3,		REDRM_MODE, PGALL, },	// pre
			{	18, 0,	display_att4,		REDRM_MODE, PGALL, },	// attenuator state
			{	22,	0,	display_lockstate1, REDRM_MODE, PGALL, },
		#else /* WITHDSPEXTDDC */
			{	9,	0,	display_pre3,		REDRM_MODE, PGALL, },	// pre
			{	13, 0,	display_att4,		REDRM_MODE, PGALL, },	// attenuator state
			{	18,	0,	display_lockstate4, REDRM_MODE, PGALL, },
		#endif /* WITHDSPEXTDDC */
			{	23, 0,	display_rxbw3,		REDRM_MODE, PGALL, },
			{	27, 0,	display_mode3_a,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
		/* ---------------------------------- */
			{	0,	2,	display_freqXbig_a, REDRM_FREQ, PGALL, },	// fullwidth = 8 constantly
		#if ! WITHDSPEXTDDC
			{	27, 2,	display_agc3,		REDRM_MODE, PGALL, },
		#endif /* ! WITHDSPEXTDDC */
			{	27, 4,	display_voxtune3,	REDRM_MODE, PGALL, },
		/* ---------------------------------- */
		#if WITHUSEAUDIOREC
			{	0,	7,	display_rec3,		REDRM_BARS, PGALL, },	// ����������� ������ ������ ����� ���������
		#endif /* WITHUSEAUDIOREC */
			{	4,	7,	display_mainsub3, REDRM_MODE, PGALL, },	// main/sub RX
			{	8,	7,	display_vfomode3,	REDRM_MODE, PGALL, },	// SPL
			{	16, 7,	display_freqX_b,	REDRM_FRQB, PGALL, },
			{	27, 7,	display_mode3_b,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
		/* ---------------------------------- */
			{	0,	9,	display2_bars,		REDRM_BARS, PG0 | REDRSUBSET_MENU, },	// S-METER, SWR-METER, POWER-METER
			{	0,	10,	display2_legend,	REDRM_MODE, PG0 | REDRSUBSET_MENU, },	// ����������� ��������� ����� S-�����
			/* ---------------------------------- */
		#if WITHDSPEXTDDC

			{	0,	9,	dsp_latchwaterfall,	REDRM_BARS,	PGLATCH, },	// ������������ ������ ������� ��� ������������ ����������� ������� ��� ��������
			{	0,	9,	display2_spectrum,	REDRM_BARS, PG1, },// ���������� ����������� �������
			{	0,	9,	display2_colorbuff,	REDRM_BARS,	PG1, },// ����������� �������� �/��� �������
		#else /* WITHDSPEXTDDC */
			{	27, 12,	display_atu3,		REDRM_MODE, PGALL, },	// ATU
			{	27, 14,	display_byp3,		REDRM_MODE, PGALL, },	// BYP
		#endif /* WITHDSPEXTDDC */
			/* ---------------------------------- */
		#if defined (RTC1_TYPE)
			{	0,	14,	display_time5,		REDRM_BARS, PG0, },	// TIME
		#endif /* defined (RTC1_TYPE) */
		#if WITHVOLTLEVEL
			{	6,	14,	display_voltlevelV5, REDRM_VOLT, PG0, },	// voltmeter with "V"
		#endif /* WITHVOLTLEVEL  */
		#if WITHCURRLEVEL
			{	11, 14,	display_currlevelA6, REDRM_VOLT, PG0, },	// amphermeter with "A"
		#endif /*  WITHCURRLEVEL */
		#if WITHAMHIGHKBDADJ
			{	6, 14,	display_amfmhighcut4,REDRM_MODE, PG0, },	// 3.70
		#endif /* WITHAMHIGHKBDADJ */
			{	18, 14,	display_samfreqdelta8, REDRM_BARS, PG0 | REDRSUBSET_MENU, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
		#if WITHNOTCHONOFF || WITHNOTCHFREQ
			{	27, 14,	display_notch3, REDRM_MODE, PG0, },	// 3.7
		#endif /* WITHNOTCHONOFF || WITHNOTCHFREQ */
		/* ---------------------------------- */
	#if WITHMENU
			{	0,	12,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0,	14,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4,	14,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			//{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#if WITHVOLTLEVEL && WITHCURRLEVEL
			//{	0,	14,	display_voltlevelV5, REDRM_VOLT, REDRSUBSET_MENU, },	// voltmeter with "V"
			//{	6,	14,	display_currlevelA6, REDRM_VOLT, REDRSUBSET_MENU, },	// amphermeter with "A"
		#endif /* WITHVOLTLEVEL && WITHCURRLEVEL */
	#endif /* WITHMENU */
		};

	#endif /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */

#elif DSTYLE_G_X320_Y240
	// TFT ������ 320 * 240 ADI_3.2_AM-240320D4TOQW-T00H(R)
	// 320*240 SF-TC240T-9370-T � ������������ ILI9341
	// 32*15 ��������� 10*16
	#define CHAR_W	10
	#define CHAR_H	8
	#define SMALLCHARH 16 /* Font height */

	#if DSTYLE_UR3LMZMOD && WITHONEATTONEAMP
		// TFT ������ 320 * 240
		enum
		{
			BDTH_ALLRX = 20,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 11,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 5,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 10,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		#define SMETERMAP "1 3 5 7 9 + 20 40 60"
		#define SWRPWRMAP "1 2 3  0%   |   100%" 
		#define POWERMAP  "0 10 20 40 60 80 100"
		#define SWRMAX	(SWRMIN * 30 / 10)	// 3.0 - �������� �� ������ �����
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0xFF,
			PATTERN_BAR_HALF = 0xFF,
			PATTERN_BAR_EMPTYFULL = 0x00,
			PATTERN_BAR_EMPTYHALF = 0x00
		};

		enum {
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0, 0,	display_txrxstate2, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	3, 0,	display_voxtune3,	REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	7, 0,	display_att4,		REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	12, 0,	display_preovf3,	REDRM_BARS, REDRSUBSET(DPAGE0), },
			{	16, 0,	display_lockstate1, REDRM_MODE, REDRSUBSET(DPAGE0), },
			{	19, 0,	display_rxbw3,		REDRM_MODE, REDRSUBSET(DPAGE0), },

			{	0, 8,	display_freqXbig_a, REDRM_FREQ, REDRSUBSET(DPAGE0), },
			{	19, 8,	display_mode3_a,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	0, 18,	display_vfomode5,	REDRM_MODE, REDRSUBSET(DPAGE0), },	// SPLIT
			{	5, 18,	display_freqX_b,	REDRM_FRQB, REDRSUBSET(DPAGE0), },
			{	19, 18,	display_mode3_b,	REDRM_MODE,	REDRSUBSET(DPAGE0), },	// SSB/CW/AM/FM/...

			{	1, 24,	display2_bars,		REDRM_BARS, REDRSUBSET(DPAGE0), },	// S-METER, SWR-METER, POWER-METER
		#if defined (RTC1_TYPE)
			{	0, 28,	display_time8,		REDRM_BARS, REDRSUBSET_MENU | REDRSUBSET(DPAGE0), },	// TIME
		#endif /* defined (RTC1_TYPE) */
			{	18, 28,	display_agc3,		REDRM_MODE, REDRSUBSET(DPAGE0), },
		#if WITHMENU
			{	4, 0,	display_menu_group,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� ������
			{	0, 2,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 2,	display_menu_lblng,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			{	0, 4,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	16, 0,	display_lockstate4,	REDRM_MODE, REDRSUBSET_MENU, },	// ��������� ���������� ���������
		#endif /* WITHMENU */
		};

	#else /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */
		// TFT ������ 320 * 240
		// ��� �����
		enum
		{
			BDCV_ALLRX = 9,	// ����� ������, ���������� ��� S-����, ��������, ������� � ���� �����������

			BDCO_SPMRX = 0,	// �������� ������� �� ��������� � ������� �� ������ ������ ����
			BDCV_SPMRX = 4,	// ������������ ������ ������� � �������

			BDCO_WFLRX = 4,	// �������� �������� �� ��������� � ������� �� ������ ������ ����
			BDCV_WFLRX = 5,	// ������������ ������ �������� � �������

			BDTH_ALLRX = 26,	// ������ ���� ��� ����������� ������ �� ����������
			BDTH_RIGHTRX = 16,	// ������ ���������� ������
			BDTH_LEFTRX = BDTH_ALLRX - BDTH_RIGHTRX,	// ������ ���������� ������
			BDTH_SPACERX = 0,
		#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
			BDTH_ALLSWR = 8,
			BDTH_SPACESWR = 1,
			BDTH_ALLPWR = 8,
			BDTH_SPACEPWR = 0
		#else /* WITHSHOWSWRPWR */
			BDTH_ALLSWR = BDTH_ALLRX,
			BDTH_SPACESWR = BDTH_SPACERX,
			BDTH_ALLPWR = BDTH_ALLRX,
			BDTH_SPACEPWR = BDTH_SPACERX
		#endif /* WITHSHOWSWRPWR */
		};
		enum
		{
			PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
			PATTERN_BAR_FULL = 0x7e,
			PATTERN_BAR_HALF = 0x3c,
			PATTERN_BAR_EMPTYFULL = 0x00,
			PATTERN_BAR_EMPTYHALF = 0x00
		};
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����

		enum 
		{
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
		#if WITHIF4DSP
			DPAGE1,					// ��������, � ������� ������������ ������ ��������
		#endif /* WITHIF4DSP */
			DISPLC_MODCOUNT
		};
		enum {
			PG0 = REDRSUBSET(DPAGE0),
		#if WITHIF4DSP
			PG1 = REDRSUBSET(DPAGE1),
			PGALL = PG0 | PG1 | REDRSUBSET_MENU,
		#else /* WITHIF4DSP */
			PGALL = PG0 | REDRSUBSET_MENU,
		#endif /* WITHIF4DSP */
			PGLATCH = PGALL,	// ��������, �� ������� �������� ����������� �������� ��� ��������.
			PGunused
		};
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
		#define DISPLC_RJ		1	// ���������� ������� ������ ���� � ����������� �������
		static const FLASHMEM struct dzone dzones [] =
		{
			{	0,	0,	display_txrxstate2, REDRM_MODE, PGALL, },
			{	3,	0,	display_voxtune3,	REDRM_MODE, PGALL, },
			{	7,	0,	display_att4,		REDRM_MODE, PGALL, },
			{	12, 0,	display_pre3,		REDRM_MODE, PGALL, },
		#if WITHDSPEXTDDC
			{	16, 0,	display_ovf3,		REDRM_BARS, PGALL, },	// ovf/pre
		#endif /* WITHDSPEXTDDC */
			{	20, 0,	display_lockstate4, REDRM_MODE, PGALL, },
			{	25, 0,	display_agc3,		REDRM_MODE, PGALL, },
			{	29, 0,	display_rxbw3,		REDRM_MODE, PGALL, },

			{	0,	8,	display_freqXbig_a, REDRM_FREQ, PGALL, },
			{	29, 8,	display_mode3_a,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
			//---
			{	0,	15,	display_vfomode5,	REDRM_MODE, PGALL, },	// SPLIT
			{	6,	15,	display_freqX_b,	REDRM_FRQB, PGALL, },
		#if WITHUSEDUALWATCH
			{	25, 15,	display_mainsub3,	REDRM_MODE, PGALL, },	// main/sub RX
		#endif /* WITHUSEDUALWATCH */
			{	29, 15,	display_mode3_b,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
			//---
			{	0,	18,	display2_bars,		REDRM_BARS, PG0, },	// S-METER, SWR-METER, POWER-METER
		#if WITHIF4DSP
			{	0,	18,	dsp_latchwaterfall,	REDRM_BARS,	PGLATCH, },	// ������������ ������ ������� ��� ������������ ����������� ������� ��� ��������
			{	0,	18,	display2_spectrum,	REDRM_BARS, PG1, },// ���������� ����������� �������
			{	0,	18,	display2_waterfall,	REDRM_BARS, PG1, },// ���������� ����������� ��������
			{	0,	18,	display2_colorbuff,	REDRM_BARS,	PG1, },// ����������� �������� �/��� �������

			{	27, 18,	display_siglevel5,	REDRM_BARS, PGALL, },	// signal level
		#endif /* WITHIF4DSP */
			//---
			//{	22, 25,	display_samfreqdelta8, REDRM_BARS, PGALL, },	/* �������� ���������� �� ������ ��������� � ������ SAM */

		#if WITHVOLTLEVEL
			{	0, 28,	display_voltlevelV5, REDRM_VOLT, PGALL, },	// voltmeter with "V"
		#endif /* WITHVOLTLEVEL */
		#if WITHCURRLEVEL
			{	6, 28,	display_currlevelA6, REDRM_VOLT, PGALL, },	// amphermeter with "A"
		#endif /* WITHCURRLEVEL */
		#if defined (RTC1_TYPE)
			{	13, 28,	display_time8,		REDRM_BARS, PGALL, },	// TIME
		#endif /* defined (RTC1_TYPE) */
		#if WITHUSEAUDIOREC
			{	25, 28,	display_rec3,		REDRM_BARS, PGALL, },	// ����������� ������ ������ ����� ���������
		#endif /* WITHUSEAUDIOREC */
		#if WITHMENU
			{	4, 19,	display_menu_group,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� ������
			{	0, 21,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
			{	0, 24,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
			{	4, 24,	display_menu_lblst,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
			//{	9,	27,	display_freqmeter10,	REDRM_VOLT, REDRSUBSET_MENU, },	// ���������� ������� ���������� ������� �������
			{	9, 27,	display_samfreqdelta8, REDRM_BARS, REDRSUBSET_MENU, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
		#endif /* WITHMENU */
		};

		/* �������� ���������� ���� � ��������� �/��� ���������. */
		void display2_getpipparams(pipparams_t * p)
		{
			p->x = GRID2X(0);	// ������� �������� ������ ���� � ��������
			p->y = GRID2Y(18);	// ������� �������� ������ ���� � ��������
			p->w = GRID2X(CHARS2GRID(BDTH_ALLRX));	// ������ �� ����������� � ��������
			p->h = GRID2Y(BDCV_ALLRX);				// ������ �� ��������� � ��������
		}

	#endif /* DSTYLE_UR3LMZMOD && WITHONEATTONEAMP */

#elif DSTYLE_G_X480_Y272 && WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192)

	// TFT ������ SONY PSP-1000
	// 272/5 = 54, 480/16=30

	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		//					"012345678901234567890123"
		#define SWRPWRMAP	"1   2   3   4  0% | 100%" 
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����
	#else
		//					"012345678901234567890123"
		#define POWERMAP	"0    25    50   75   100"
		#define SWRMAP		"1   |   2  |   3   |   4"	// 
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����
	#endif
	//						"012345678901234567890123"
	#define SMETERMAP		"1  3  5  7  9 +20 +40 60"
	enum
	{
		BDTH_ALLRX = 24,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_LEFTRX = 12,	// ������ ���������� ������
		BDTH_RIGHTRX = BDTH_ALLRX - BDTH_LEFTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 13,
		BDTH_SPACESWR = 2,
		BDTH_ALLPWR = 9,
		BDTH_SPACEPWR = 0,
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX,
	#endif /* WITHSHOWSWRPWR */

		BDCV_ALLRX = ROWS2GRID(23),	// ���������� ������, ���������� ��� S-����, ��������, ���� �����������

	#if WITHSEPARATEWFL
		/* ��� ���������� �� ����� ������� �������� � �������� */
		BDCO_SPMRX = ROWS2GRID(0),	// �������� ������� �� ��������� � ������� �� ������ ������ ����
		BDCO_WFLRX = ROWS2GRID(0)	// �������� �������� �� ��������� � ������� �� ������ ������ ����
		BDCV_SPMRX = BDCV_ALLRX,	// ������������ ������ ������� � �������		};
		BDCV_WFLRX = BDCV_ALLRX,	// ������������ ������ �������� � �������		};
	#else /* WITHSEPARATEWFL */
		/* ���������� �� ����� ������� �������� � �������� */
		BDCO_SPMRX = ROWS2GRID(0),	// �������� ������� �� ��������� � ������� �� ������ ������ ����
		BDCV_SPMRX = ROWS2GRID(12),	// ������������ ������ ������� � �������		};
		BDCO_WFLRX = BDCV_SPMRX,	// �������� �������� �� ��������� � ������� �� ������ ������ ����
		BDCV_WFLRX = BDCV_ALLRX - BDCO_WFLRX	// ������������ ������ �������� � �������		};
	#endif /* WITHSEPARATEWFL */
	};

	enum
	{
		PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
		PATTERN_BAR_FULL = 0xFF,
		PATTERN_BAR_HALF = 0x3c,
		PATTERN_BAR_EMPTYFULL = 0x00,	//0x00
		PATTERN_BAR_EMPTYHALF = 0x00	//0x00
	};

	#if WITHSEPARATEWFL
		/* ��� ���������� �� ����� ������� �������� � �������� */
		enum 
		{
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DPAGE1,					// ��������, � ������� ������������ ������
			DPAGE2,					// ��������, � ������� ������������ �������
			DISPLC_MODCOUNT
		};

		enum
		{
			PG0 = REDRSUBSET(DPAGE0),
			PG1 = REDRSUBSET(DPAGE1),
			PG2 = REDRSUBSET(DPAGE2),
			PGSLP = REDRSUBSET_SLEEP,
			PGALL = PG0 | PG1 | PG2 | REDRSUBSET_MENU,
			PGLATCH = PGALL,	// ��������, �� ������� �������� ����������� �������� ��� ��������.
			PGSWR = PG0,	// �������� ����������� S-meter � SWR-meter
			PGWFL = PG2,
			PGSPE = PG1,
			PGunused
		};
	#else /* WITHSEPARATEWFL */
		/* ���������� �� ����� ������� �������� � �������� */
		enum 
		{
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DPAGE1,					// ��������, � ������� ������������ ������ � �������
			DISPLC_MODCOUNT
		};

		enum
		{
			PG0 = REDRSUBSET(DPAGE0),
			PG1 = REDRSUBSET(DPAGE1),
			PGALL = PG0 | PG1 | REDRSUBSET_MENU,
			PGWFL = PG0,	// �������� ����������� ��������
			PGSPE = PG0,	// �������� ����������� ��������
			PGSWR = PG1,	// �������� ����������� S-meter � SWR-meter
			PGLATCH = PGALL,	// ��������, �� ������� �������� ����������� �������� ��� ��������.
			PGSLP = REDRSUBSET_SLEEP,
			PGunused
		};
	#endif /* WITHSEPARATEWFL */

	#if TUNE_TOP > 100000000uL
		#define DISPLC_WIDTH	9	// ���������� ���� � ����������� �������
	#else
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
	#endif
	#define DISPLC_RJ		0	// ���������� ������� ������ ���� � ����������� �������

	static const FLASHMEM struct dzone dzones [] =
	{
		{	0,	0,	display_txrxstate2, REDRM_MODE, PGALL, },
		{	3,	0,	display_ant5,		REDRM_MODE, PGALL, },
		{	9,	0,	display_att4,		REDRM_MODE, PGALL, },
		{	14,	0,	display_preovf3,	REDRM_BARS, PGALL, },
		{	18,	0,	display_genham1,	REDRM_BARS, PGALL, },	// ����������� ������ General Coverage / HAM bands

	#if WITHENCODER2
		{	21, 0,	display_fnlabel9,	REDRM_MODE, PGALL, },	// FUNC item label
		{	21,	4,	display_fnvalue9,	REDRM_MODE, PGALL, },	// FUNC item value
		{	25, 15,	display_notch5,		REDRM_MODE, PGALL, },	// NOTCH on/off
	#else /* WITHENCODER2 */
		{	25, 0,	display_notch5,		REDRM_MODE, PGALL, },	// FUNC item label
		{	25,	4,	display_notchfreq5,	REDRM_BARS, PGALL, },	// FUNC item value
	#endif /* WITHENCODER2 */

		{	26, 20,	display_agc3,		REDRM_MODE, PGALL, },	// AGC mode
		{	26, 25,	display_voxtune3,	REDRM_MODE, PGALL, },	// VOX
		{	26, 30,	display_datamode3,	REDRM_MODE, PGALL, },	// DATA mode indicator
		{	26, 35,	display_atu3,		REDRM_MODE, PGALL, },	// TUNER state (optional)
		{	26, 40,	display_byp3,		REDRM_MODE, PGALL, },	// TUNER BYPASS state (optional)
		{	26, 45,	display_rec3,		REDRM_BARS, PGALL, },	// ����������� ������ ������ ����� ���������
		
		{	0,	7,	display_freqX_a,	REDRM_FREQ, PGALL, },	// MAIN FREQ ������� (������� �����)
		{	21, 10,	display_mode3_a,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
		{	26, 10,	display_rxbw3,		REDRM_MODE, PGALL, },	// 3.1 / 0,5 / WID / NAR
		{	21, 15,	display_mainsub3,	REDRM_MODE, PGALL, },	// main/sub RX: A/A, A/B, B/A, etc

		{	0,	20,	display_lockstate4, REDRM_MODE, PGALL, },	// LOCK
		{	5,	20,	display_vfomode3,	REDRM_MODE, PGALL, },	// SPLIT
		{	9,	20,	display_freqX_b,	REDRM_FRQB, PGALL, },	// SUB FREQ
		{	21, 20,	display_mode3_b,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...

#if 1
		{	0,	25,	display2_legend_rx,	REDRM_MODE, PGSWR, },	// ����������� ��������� ����� S-�����
		{	0,	30,	display2_bars_rx,	REDRM_BARS, PGSWR, },	// S-METER, SWR-METER, POWER-METER
		{	0,	35,	display2_legend_tx,	REDRM_MODE, PGSWR, },	// ����������� ��������� ����� PWR & SWR-�����
		{	0,	40,	display2_bars_tx,	REDRM_BARS, PGSWR, },	// S-METER, SWR-METER, POWER-METER
#else
		{	0,	25,	display2_adctest,	REDRM_BARS, PGSWR, },	// ADC raw data print
#endif

	#if WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192)
		{	0,	25,	dsp_latchwaterfall,	REDRM_BARS,	PGLATCH, },	// ������������ ������ ������� ��� ������������ ����������� ������� ��� ��������
		{	0,	25,	display2_spectrum,	REDRM_BARS, PGSPE, },// ���������� ����������� �������
		{	0,	25,	display2_waterfall,	REDRM_BARS, PGWFL, },// ���������� ����������� ��������
		{	0,	25,	display2_colorbuff,	REDRM_BARS,	PGWFL | PGSPE, },// ����������� �������� �/��� �������
	#endif /* WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192) */

	
		//{	0,	51,	display_samfreqdelta8, REDRM_BARS, PGALL, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
		{	0,	51,	display_time8,		REDRM_BARS, PGALL,	},	// TIME
		{	9,	51,	display_siglevel5,	REDRM_BARS, PGALL, },	// signal level in S points
		{	15, 51,	display_thermo4,	REDRM_VOLT, PGALL, },	// thermo sensor
	#if CTLSTYLE_RA4YBO || CTLSTYLE_RA4YBO_V3
		{	19, 51,	display_currlevel5alt, REDRM_VOLT, PGALL, },	// PA drain current dd.d without "A"
	#else
		{	19, 51,	display_currlevel5, REDRM_VOLT, PGALL, },	// PA drain current d.dd without "A"
	#endif
		{	25, 51,	display_voltlevelV5, REDRM_VOLT, PGALL, },	// voltmeter with "V"
	#if WITHAMHIGHKBDADJ
		{	25, 51,	display_amfmhighcut4,REDRM_MODE, PGALL, },	// 3.70
	#endif /* WITHAMHIGHKBDADJ */

		// sleep mode display
		{	5,	24,	display_datetime12,	REDRM_BARS, PGSLP, },	// DATE & TIME // DATE&TIME Jan 01 13:40
		{	20, 24,	display_voltlevelV5, REDRM_VOLT, PGSLP, },	// voltmeter with "V"

	#if WITHMENU
		{	4,	25,	display_menu_group,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� ������
		{	0,	30,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4,	30,	display_menu_lblng,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	0,	35,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0,	40,	display_samfreqdelta8, REDRM_BARS, REDRSUBSET_MENU, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
	#endif /* WITHMENU */
	};

	/* �������� ���������� ���� � ��������� �/��� ���������. */
	void display2_getpipparams(pipparams_t * p)
	{
		p->x = GRID2X(0);	// ������� �������� ������ ���� � ��������
		p->y = GRID2Y(25);	// ������� �������� ������ ���� � ��������
		p->w = GRID2X(CHARS2GRID(BDTH_ALLRX));	// ������ �� ����������� � ��������
		p->h = GRID2Y(BDCV_ALLRX);				// ������ �� ��������� � ��������
	}

#elif DSTYLE_G_X480_Y272

	// TFT ������ SONY PSP-1000
	// 272/5 = 54, 480/16=30
	// ��� �������� � ��������

	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		//					"012345678901234567890123"
		#define SWRPWRMAP	"1   2   3   4  0% | 100%" 
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����
	#else
		//					"012345678901234567890123"
		#define POWERMAP	"0    25    50   75   100"
		#define SWRMAP		"1   |   2  |   3   |   4"	// 
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����
	#endif
	//						"012345678901234567890123"
	#define SMETERMAP		"1  3  5  7  9 +20 +40 60"
	enum
	{
		BDTH_ALLRX = 24,	// ������ ���� ��� ����������� ������ �� ����������
		BDTH_LEFTRX = 12,	// ������ ���������� ������
		BDTH_RIGHTRX = BDTH_ALLRX - BDTH_LEFTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 13,
		BDTH_SPACESWR = 2,
		BDTH_ALLPWR = 9,
		BDTH_SPACEPWR = 0,
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX,
	#endif /* WITHSHOWSWRPWR */

		BDCV_ALLRX = ROWS2GRID(20),	// ���������� ������, ���������� ��� S-����, ��������, ���� �����������
	};

	enum
	{
		PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
		PATTERN_BAR_FULL = 0xFF,
		PATTERN_BAR_HALF = 0x3c,
		PATTERN_BAR_EMPTYFULL = 0x00,	//0x00
		PATTERN_BAR_EMPTYHALF = 0x00	//0x00
	};

		/* ���������� �� ����� ������� �������� � �������� */
		enum 
		{
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DISPLC_MODCOUNT
		};

		enum
		{
			PG0 = REDRSUBSET(DPAGE0),
			PGSWR = PG0,	// �������� ����������� S-meter � SWR-meter
			PGALL = PG0 | REDRSUBSET_MENU,
			PGSLP = REDRSUBSET_SLEEP,
			PGunused
		};

	#if TUNE_TOP > 100000000uL
		#define DISPLC_WIDTH	9	// ���������� ���� � ����������� �������
	#else
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
	#endif
	#define DISPLC_RJ		0	// ���������� ������� ������ ���� � ����������� �������

	static const FLASHMEM struct dzone dzones [] =
	{
		{	0,	0,	display_txrxstate2, REDRM_MODE, PGALL, },
		{	3,	0,	display_ant5,		REDRM_MODE, PGALL, },
		{	9,	0,	display_att4,		REDRM_MODE, PGALL, },
		{	14,	0,	display_preovf3,	REDRM_BARS, PGALL, },
		{	18,	0,	display_genham1,	REDRM_BARS, PGALL, },	// ����������� ������ General Coverage / HAM bands

	#if WITHENCODER2
		{	21, 0,	display_fnlabel9,	REDRM_MODE, PGALL, },	// FUNC item label
		{	21,	4,	display_fnvalue9,	REDRM_MODE, PGALL, },	// FUNC item value
		{	25, 15,	display_notch5,		REDRM_MODE, PGALL, },	// NOTCH on/off
	#else /* WITHENCODER2 */
		{	25, 0,	display_notch5,		REDRM_MODE, PGALL, },	// FUNC item label
		{	25,	4,	display_notchfreq5,	REDRM_BARS, PGALL, },	// FUNC item value
	#endif /* WITHENCODER2 */

		{	26, 20,	display_agc3,		REDRM_MODE, PGALL, },	// AGC mode
		{	26, 22,	display_voxtune3,	REDRM_MODE, PGALL, },	// VOX
		{	25, 30,	display_lockstate4, REDRM_MODE, PGALL, },	// LOCK
		{	26, 35,	display_atu3,		REDRM_MODE, PGALL, },	// TUNER state (optional)
		{	26, 40,	display_byp3,		REDRM_MODE, PGALL, },	// TUNER BYPASS state (optional)
		{	26, 45,	display_rec3,		REDRM_BARS, PGALL, },	// ����������� ������ ������ ����� ���������
		
		{	0,	7,	display_freqX_a,	REDRM_FREQ, PGALL, },	// MAIN FREQ ������� (������� �����)
		{	21, 10,	display_mode3_a,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
		{	26, 10,	display_rxbw3,		REDRM_MODE, PGALL, },	// 3.1 / 0,5 / WID / NAR
		{	21, 15,	display_mainsub3,	REDRM_MODE, PGALL, },	// main/sub RX: A/A, A/B, B/A, etc

		{	5,	22,	display_vfomode3,	REDRM_MODE, PGALL, },	// SPLIT
		{	8,	22,	display_freqX_b,	REDRM_FRQB, PGALL, },	// SUB FREQ
		{	21, 22,	display_mode3_b,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...

		{	0,	30,	display2_legend_rx,	REDRM_MODE, PGSWR, },	// ����������� ��������� ����� S-�����
		{	0,	35,	display2_bars_rx,	REDRM_BARS, PGSWR, },	// S-METER, SWR-METER, POWER-METER
		{	0,	40,	display2_legend_tx,	REDRM_MODE, PGSWR, },	// ����������� ��������� ����� PWR & SWR-�����
		{	0,	45,	display2_bars_tx,	REDRM_BARS, PGSWR, },	// S-METER, SWR-METER, POWER-METER

		//{	0,	51,	display_samfreqdelta8, REDRM_BARS, PGALL, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
		{	0,	51,	display_time8,		REDRM_BARS, PGALL,	},	// TIME
		{	9,	51,	display_siglevel5,	REDRM_BARS, PGALL, },	// signal level in S points
		{	15, 51,	display_thermo4,	REDRM_VOLT, PGALL, },	// thermo sensor
	#if CTLSTYLE_RA4YBO || CTLSTYLE_RA4YBO_V3
		{	19, 51,	display_currlevel5alt, REDRM_VOLT, PGALL, },	// PA drain current dd.d without "A"
	#else
		{	19, 51,	display_currlevel5, REDRM_VOLT, PGALL, },	// PA drain current d.dd without "A"
	#endif
		{	25, 51,	display_voltlevelV5, REDRM_VOLT, PGALL, },	// voltmeter with "V"

		// sleep mode display
		{	5,	24,	display_datetime12,	REDRM_BARS, PGSLP, },	// DATE & TIME // DATE&TIME Jan 01 13:40
		{	20, 24,	display_voltlevelV5, REDRM_VOLT, PGSLP, },	// voltmeter with "V"

	#if WITHMENU
		{	4,	25,	display_menu_group,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� ������
		{	0,	30,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4,	30,	display_menu_lblng,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	14,	30,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0,	35,	display_samfreqdelta8, REDRM_BARS, REDRSUBSET_MENU, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
	#endif /* WITHMENU */
	};

	/* �������� ���������� ���� � ��������� �/��� ���������. */
	void display2_getpipparams(pipparams_t * p)
	{
		p->x = GRID2X(0);	// ������� �������� ������ ���� � ��������
		p->y = GRID2Y(30);	// ������� �������� ������ ���� � ��������
		p->w = GRID2X(CHARS2GRID(BDTH_ALLRX));	// ������ �� ����������� � ��������
		p->h = GRID2Y(BDCV_ALLRX);				// ������ �� ��������� � ��������
	}

#elif DSTYLE_G_X800_Y480 && WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192)

	// TFT ������ AT070TN90
	// 480/5 = 96, 800/16=50

	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		//					"012345678901234567890123"
		#define SWRPWRMAP	"1   2   3   4  0% | 100%" 
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����
	#else
		//					"012345678901234567890123"
		#define POWERMAP	"0    25    50   75   100"
		#define SWRMAP		"1   |   2  |   3   |   4"	// 
		#define SWRMAX	(SWRMIN * 40 / 10)	// 4.0 - �������� �� ������ �����
	#endif
	//						"012345678901234567890123"
	#define SMETERMAP		"1  3  5  7  9 +20 +40 60"
	enum
	{
		BDTH_ALLRX = 40,	// ������ ���� ��� ����������� ������ �� ����������

		BDTH_LEFTRX = 12,	// ������ ���������� ������
		BDTH_RIGHTRX = BDTH_ALLRX - BDTH_LEFTRX,	// ������ ���������� ������
		BDTH_SPACERX = 0,
	#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
		BDTH_ALLSWR = 13,
		BDTH_SPACESWR = 2,
		BDTH_ALLPWR = 9,
		BDTH_SPACEPWR = 0,
	#else /* WITHSHOWSWRPWR */
		BDTH_ALLSWR = BDTH_ALLRX,
		BDTH_SPACESWR = BDTH_SPACERX,
		BDTH_ALLPWR = BDTH_ALLRX,
		BDTH_SPACEPWR = BDTH_SPACERX,
	#endif /* WITHSHOWSWRPWR */

		BDCV_ALLRX = ROWS2GRID(60),	// ���������� ������, ���������� ��� S-����, ��������, ���� �����������
	#if WITHSEPARATEWFL
		/* ��� ���������� �� ����� ������� �������� � �������� */
		BDCO_SPMRX = ROWS2GRID(0),	// �������� ������� �� ��������� � ������� �� ������ ������ ����
		BDCO_WFLRX = ROWS2GRID(0)	// �������� �������� �� ��������� � ������� �� ������ ������ ����
		BDCV_SPMRX = BDCV_ALLRX,	// ������������ ������ ������� � �������		};
		BDCV_WFLRX = BDCV_ALLRX,	// ������������ ������ �������� � �������		};
	#else /* WITHSEPARATEWFL */
		/* ���������� �� ����� ������� �������� � �������� */
		BDCO_SPMRX = ROWS2GRID(0),	// �������� ������� �� ��������� � ������� �� ������ ������ ����
		BDCV_SPMRX = ROWS2GRID(22),	// ������������ ������ ������� � �������		};
		BDCO_WFLRX = BDCV_SPMRX,	// �������� �������� �� ��������� � ������� �� ������ ������ ����
		BDCV_WFLRX = BDCV_ALLRX - BDCV_SPMRX	// ������������ ������ �������� � �������		};
	#endif /* WITHSEPARATEWFL */
	};

	enum
	{
		PATTERN_SPACE = 0x00,	/* ������� ����� �� SWR � PWR ������ ���� �������� */
		PATTERN_BAR_FULL = 0xFF,
		PATTERN_BAR_HALF = 0x3c,
		PATTERN_BAR_EMPTYFULL = 0x00,	//0x00
		PATTERN_BAR_EMPTYHALF = 0x00	//0x00
	};

	#if WITHSEPARATEWFL
		/* ��� ���������� �� ����� ������� �������� � �������� */
		enum 
		{
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DPAGE1,					// ��������, � ������� ������������ ������
			DPAGE2,					// ��������, � ������� ������������ �������
			DISPLC_MODCOUNT
		};

		enum
		{
			PG0 = REDRSUBSET(DPAGE0),
			PG1 = REDRSUBSET(DPAGE1),
			PG2 = REDRSUBSET(DPAGE2),
			PGSLP = REDRSUBSET_SLEEP,
			PGALL = PG0 | PG1 | PG2 | REDRSUBSET_MENU,
			PGLATCH = PGALL,	// ��������, �� ������� �������� ����������� �������� ��� ��������.
			PGSWR = PG0,	// �������� ����������� S-meter � SWR-meter
			PGWFL = PG2,
			PGSPE = PG1,
			PGunused
		};
	#else /* WITHSEPARATEWFL */
		/* ���������� �� ����� ������� �������� � �������� */
		enum 
		{
			DPAGE0,					// ��������, � ������� ������������ �������� (��� ���) 
			DPAGE1,					// ��������, � ������� ������������ ������ � �������
			DISPLC_MODCOUNT
		};

		enum
		{
			PG0 = REDRSUBSET(DPAGE0),
			PG1 = REDRSUBSET(DPAGE1),
			PGALL = PG0 | PG1 | REDRSUBSET_MENU,
			PGWFL = PG0,	// �������� ����������� ��������
			PGSPE = PG0,	// �������� ����������� ��������
			PGSWR = PG1,	// �������� ����������� S-meter � SWR-meter
			PGLATCH = PGALL,	// ��������, �� ������� �������� ����������� �������� ��� ��������.
			PGSLP = REDRSUBSET_SLEEP,
			PGunused
		};
	#endif /* WITHSEPARATEWFL */

		enum {
		DLE0 = 88,
		DLE1 = 93
		};

	#if 1//TUNE_TOP > 100000000uL
		#define DISPLC_WIDTH	9	// ���������� ���� � ����������� �������
	#else
		#define DISPLC_WIDTH	8	// ���������� ���� � ����������� �������
	#endif
	#define DISPLC_RJ		0	// ���������� ������� ������ ���� � ����������� �������

	// 480/5 = 96, 800/16=50
	// 272/5 = 54, 480/16=30 (old)
	//#define GRID2X(cellsx) ((cellsx) * 16)	/* ������� ����� ����� �������� � ����� ������� �� ����������� */
	//#define GRID2Y(cellsy) ((cellsy) * 5)	/* ������� ����� ����� �������� � ����� ������� �� ��������� */
	//#define SMALLCHARH 15 /* Font height */
	//#define SMALLCHARW 16 /* Font width */
	static const FLASHMEM struct dzone dzones [] =
	{
		{	0,	0,	display_txrxstate2, REDRM_MODE, PGALL, },
		{	3,	0,	display_ant5,		REDRM_MODE, PGALL, },
		{	9,	0,	display_att4,		REDRM_MODE, PGALL, },
		{	14,	0,	display_preovf3,	REDRM_BARS, PGALL, },
		{	18,	0,	display_genham1,	REDRM_BARS, PGALL, },	// ����������� ������ General Coverage / HAM bands

	#if WITHENCODER2
		{	21, 0,	display_fnlabel9,	REDRM_MODE, PGALL, },	// FUNC item label
		{	21,	4,	display_fnvalue9,	REDRM_MODE, PGALL, },	// FUNC item value
		{	45, 15,	display_notch5,		REDRM_MODE, PGALL, },	// NOTCH on/off
	#else /* WITHENCODER2 */
		{	45, 0,	display_notch5,		REDRM_MODE, PGALL, },	// FUNC item label
		{	45,	4,	display_notchfreq5,	REDRM_BARS, PGALL, },	// FUNC item value
	#endif /* WITHENCODER2 */

		{	46, 20,	display_agc3,		REDRM_MODE, PGALL, },	// AGC mode
		{	46, 25,	display_voxtune3,	REDRM_MODE, PGALL, },	// VOX
		{	46, 30,	display_datamode3,	REDRM_MODE, PGALL, },	// DATA mode indicator
		{	46, 35,	display_atu3,		REDRM_MODE, PGALL, },	// TUNER state (optional)
		{	46, 40,	display_byp3,		REDRM_MODE, PGALL, },	// TUNER BYPASS state (optional)
		{	46, 45,	display_rec3,		REDRM_BARS, PGALL, },	// ����������� ������ ������ ����� ���������
		
		{	0,	7,	display_freqX_a,	REDRM_FREQ, PGALL, },	// MAIN FREQ ������� (������� �����)
		{	21, 10,	display_mode3_a,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...
		{	26, 10,	display_rxbw3,		REDRM_MODE, PGALL, },	// 3.1 / 0,5 / WID / NAR
		{	21, 15,	display_mainsub3,	REDRM_MODE, PGALL, },	// main/sub RX: A/A, A/B, B/A, etc

		{	5,	20,	display_vfomode3,	REDRM_MODE, PGALL, },	// SPLIT
		{	9,	20,	display_freqX_b,	REDRM_FRQB, PGALL, },	// SUB FREQ
		{	21, 20,	display_mode3_b,	REDRM_MODE,	PGALL, },	// SSB/CW/AM/FM/...

#if 1
		{	0,	25,	display2_legend_rx,	REDRM_MODE, PGSWR, },	// ����������� ��������� ����� S-�����
		{	0,	30,	display2_bars_rx,	REDRM_BARS, PGSWR, },	// S-METER, SWR-METER, POWER-METER
		{	0,	35,	display2_legend_tx,	REDRM_MODE, PGSWR, },	// ����������� ��������� ����� PWR & SWR-�����
		{	0,	40,	display2_bars_tx,	REDRM_BARS, PGSWR, },	// S-METER, SWR-METER, POWER-METER
#else
		{	0,	25,	display2_adctest,	REDRM_BARS, PGSWR, },	// ADC raw data print
#endif

	#if WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192)
		{	0,	25,	dsp_latchwaterfall,	REDRM_BARS,	PGLATCH, },	// ������������ ������ ������� ��� ������������ ����������� ������� ��� ��������
		{	0,	25,	display2_spectrum,	REDRM_BARS, PGSPE, },// ���������� ����������� �������
		{	0,	25,	display2_waterfall,	REDRM_BARS, PGWFL, },// ���������� ����������� ��������
		{	0,	25,	display2_colorbuff,	REDRM_BARS,	PGWFL | PGSPE, },// ����������� �������� �/��� �������
	#endif /* WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192) */

	
		{	0,	DLE0,	display_lockstate4, REDRM_MODE, PGALL, },	// LOCK
		{	5,	DLE0,	display_samfreqdelta8, REDRM_BARS, PGALL, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
		{	14,	DLE0,	display_siglevel5,	REDRM_BARS, PGALL, },	// signal level in S points

		{	0,	DLE1,	display_datetime12,	REDRM_BARS, PGALL,	},	// TIME
		{	13, DLE1,	display_thermo4,	REDRM_VOLT, PGALL, },	// thermo sensor

		{	39, DLE1,	display_currlevel5, REDRM_VOLT, PGALL, },	// PA drain current d.dd without "A"
		{	45, DLE1,	display_voltlevelV5, REDRM_VOLT, PGALL, },	// voltmeter with "V"
	#if WITHAMHIGHKBDADJ
		{	45, DLE1,	display_amfmhighcut4,REDRM_MODE, PGALL, },	// 3.70
	#endif /* WITHAMHIGHKBDADJ */

		// sleep mode display
		{	5,	24,	display_datetime12,	REDRM_BARS, PGSLP, },	// DATE & TIME // DATE&TIME Jan 01 13:40
		{	20, 24,	display_voltlevelV5, REDRM_VOLT, PGSLP, },	// voltmeter with "V"

	#if WITHMENU
		{	4,	25,	display_menu_group,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� ������
		{	0,	30,	display_menu_lblc3,	REDRM_MFXX, REDRSUBSET_MENU, },	// ��� �������������� ���������
		{	4,	30,	display_menu_lblng,	REDRM_MLBL, REDRSUBSET_MENU, },	// �������� �������������� ���������
		{	0,	35,	display_menu_valxx,	REDRM_MVAL, REDRSUBSET_MENU, },	// �������� ���������
		{	0,	40,	display_samfreqdelta8, REDRM_BARS, REDRSUBSET_MENU, },	/* �������� ���������� �� ������ ��������� � ������ SAM */
	#endif /* WITHMENU */
	};

	/* �������� ���������� ���� � ��������� �/��� ���������. */
	void display2_getpipparams(pipparams_t * p)
	{
		p->x = GRID2X(0);	// ������� �������� ������ ���� � ��������
		p->y = GRID2Y(25);	// ������� �������� ������ ���� � ��������
		p->w = GRID2X(CHARS2GRID(BDTH_ALLRX));	// ������ �� ����������� � ��������
		p->h = GRID2Y(BDCV_ALLRX);				// ������ �� ��������� � ��������
	}

#else
	#error TODO: to be implemented
#endif /* LCDMODE_LS020 */

void
//NOINLINEAT
display_menu_value(
	int_fast32_t value,
	uint_fast8_t width,	// full width (if >= 128 - display with sign)
	uint_fast8_t comma,		// comma position (from right, inside width)
	uint_fast8_t rj,		// right truncated
	uint_fast8_t lowhalf
	)
{
	display_value_small(value, width, comma, 255, rj, lowhalf);
}

//+++ bars
		
static uint_fast8_t display_mapbar(
	uint_fast8_t val, 
	uint_fast8_t bottom, uint_fast8_t top, 
	uint_fast8_t mapleft, 
	uint_fast8_t mapinside, 
	uint_fast8_t mapright
	)
{
	if (val < bottom)
		return mapleft;
	if (val < top)
		return mapinside;
	return mapright;
}
#if WITHBARS

// ���������� ����� � ����������� �������� �� ������
static uint_fast16_t display_getpwrfullwidth(void)
{
	return GRID2X(CHARS2GRID(BDTH_ALLPWR));
}

#if LCDMODE_HD44780
	// �� HD44780 ������������ �������������

#elif LCDMODE_S1D13781 && ! LCDMODE_LTDC


#else /* LCDMODE_HD44780 */

// ������ ���� ������� (��� ������ �������) ��������� "��������" ����� �������
// display_wrdatabar_begin() � display_wrdatabar_end().
//
void 
//NOINLINEAT
display_dispbar(
	uint_fast8_t width,	/* ���������� ���������, ���������� ����������� */
	uint_fast8_t value,		/* ��������, ������� ���� ���������� */
	uint_fast8_t tracevalue,		/* �������� �������, ������� ���� ���������� */
	uint_fast8_t topvalue,	/* ��������, ��������������� ��������� ������������ ���������� */
	uint_fast8_t pattern,	/* DISPLAY_BAR_HALF ��� DISPLAY_BAR_FULL */
	uint_fast8_t patternmax,	/* DISPLAY_BAR_HALF ��� DISPLAY_BAR_FULL - ��� ����������� ������������ �������� */
	uint_fast8_t emptyp			/* ������� ��� ���������� ����� �������� */
	)
{
	//enum { DISPLAY_BAR_LEVELS = 6 };	// ���������� �������� � ����� ����������

	value = value < 0 ? 0 : value;
	const uint_fast16_t wfull = GRID2X(width);
	const uint_fast16_t wpart = (uint_fast32_t) wfull * value / topvalue;
	const uint_fast16_t wmark = (uint_fast32_t) wfull * tracevalue / topvalue;
	uint_fast8_t i = 0;

	for (; i < wpart; ++ i)
	{
		if (i == wmark)
		{
			display_barcolumn(patternmax);
			continue;
		}
#if (DSTYLE_G_X132_Y64 || DSTYLE_G_X128_Y64) && DSTYLE_UR3LMZMOD
		display_barcolumn(pattern);
#elif DSTYLE_G_X64_Y32
		display_barcolumn((i % 6) != 5 ? pattern : emptyp);
#else
		display_barcolumn((i % 2) == 0 ? pattern : PATTERN_SPACE);
#endif
	}

	for (; i < wfull; ++ i)
	{
		if (i == wmark)
		{
			display_barcolumn(patternmax);
			continue;
		}
#if (DSTYLE_G_X132_Y64 || DSTYLE_G_X128_Y64) && DSTYLE_UR3LMZMOD
		display_barcolumn(emptyp);
#elif DSTYLE_G_X64_Y32
		display_barcolumn((i % 6) == 5 ? pattern : emptyp);
#else
		display_barcolumn((i % 2) == 0 ? emptyp : PATTERN_SPACE);
#endif
	}
}
#endif /* LCDMODE_HD44780 */

#endif /* WITHBARS */

// ��������� ��� s-meter
static void
display_bars_address_rx(
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t xoffs	// grid
	)
{
	display_gotoxy(x + xoffs, y);
}

// ��������� ��� swr-meter
static void
display_bars_address_swr(
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t xoffs	// grid
	)
{
	display_bars_address_rx(x, y, xoffs);
}

// ��������� ��� pwr-meter
static void
display_bars_address_pwr(
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t xoffs	// grid
	)
{
#if WITHSHOWSWRPWR	/* �� ������� ������������ ������������ SWR-meter � PWR-meter */
	display_bars_address_rx(x, y, xoffs + CHARS2GRID(BDTH_ALLSWR + BDTH_SPACESWR));
#else
	display_bars_address_rx(x, y, xoffs);
#endif
}

// ���������� ��� ������ ����� PWR & SWR
void display_swrmeter(  
	uint_fast8_t x, 
	uint_fast8_t y, 
	adcvalholder_t f,	// forward, 
	adcvalholder_t r,	// reflected (�����������������)
	uint_fast16_t minforward
	)
{
#if WITHBARS
	// SWRMIN - �������� 10 - ������������� SWR = 1.0, �������� = 0.1
	// SWRMAX - ����� ����� ����� � ����� ����� SWR-����� (30 = ��� 3.0)
	const uint_fast16_t fullscale = SWRMAX - SWRMIN;
	uint_fast16_t swr10;		// ������������  ��������
	if (f < minforward)
		swr10 = 0;	// SWR=1
	else if (f <= r)
		swr10 = fullscale;		// SWR is infinite
	else
		swr10 = (f + r) * SWRMIN / (f - r) - SWRMIN;
	// v = 10..40 for swr 1..4
	// swr10 = 0..30 for swr 1..4
	const uint_fast8_t mapleftval = display_mapbar(swr10, 0, fullscale, 0, swr10, fullscale);

	//debug_printf_P(PSTR("swr10=%d, mapleftval=%d, fs=%d\n"), swr10, mapleftval, display_getmaxswrlimb());

	display_bars_address_swr(x, y, CHARS2GRID(0));

	display_setcolors(SWRCOLOR, BGCOLOR);

	display_wrdatabar_begin();
	display_dispbar(BDTH_ALLSWR, mapleftval, fullscale, fullscale, PATTERN_BAR_FULL, PATTERN_BAR_FULL, PATTERN_BAR_EMPTYFULL);
	display_wrdatabar_end();

	if (BDTH_SPACESWR != 0)
	{
		// ��������� ������ ����� �� ���������� ���
		display_bars_address_swr(x, y, CHARS2GRID(BDTH_ALLSWR));
		display_wrdatabar_begin();
		display_dispbar(BDTH_SPACESWR, 0, 1, 1, PATTERN_SPACE, PATTERN_SPACE, PATTERN_SPACE);
		display_wrdatabar_end();
	}

#endif /* WITHBARS */
}

// ���������� �� display2_bars_amv0 (������ ��� CTLSTYLE_RA4YBO_AM0)
// ���������� ��� ������ ����� PWR & SWR
// ������������ ����� ��� SWR
void display_modulationmeter_amv0(  
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t value,
	uint_fast8_t fullscale
	)
{
#if WITHBARS
	const uint_fast8_t mapleftval = display_mapbar(value, 0, fullscale, 0, value, fullscale);

	//debug_printf_P(PSTR("swr10=%d, mapleftval=%d, fs=%d\n"), swr10, mapleftval, display_getmaxswrlimb());

	display_setcolors(SWRCOLOR, BGCOLOR);
	display_bars_address_swr(x - 1, y, CHARS2GRID(0));
	display_string_P(PSTR("M"), 0);

	display_bars_address_swr(x, y, CHARS2GRID(0));

	display_setcolors(SWRCOLOR, BGCOLOR);

	display_wrdatabar_begin();
	display_dispbar(BDTH_ALLSWR, mapleftval, fullscale, fullscale, PATTERN_BAR_FULL, PATTERN_BAR_FULL, PATTERN_BAR_EMPTYFULL);
	display_wrdatabar_end();

	if (BDTH_SPACESWR != 0)
	{
		// ��������� ������ ����� �� ���������� ���
		display_bars_address_swr(x, y, CHARS2GRID(BDTH_ALLSWR));
		display_wrdatabar_begin();
		display_dispbar(BDTH_SPACESWR, 0, 1, 1, PATTERN_SPACE, PATTERN_SPACE, PATTERN_SPACE);
		display_wrdatabar_end();
	}

#endif /* WITHBARS */
}
// ���������� ��� ������ ����� PWR & SWR
// ���������� �� display2_bars_amv0 (������ ��� CTLSTYLE_RA4YBO_AM0)
void display_pwrmeter_amv0(  
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t value,			// ������� ��������
	uint_fast8_t tracemax,		// max hold ��������
	uint_fast8_t maxpwrcali		// �������� ��� ���������� �� ��� �����
	)
{
#if WITHBARS
	const uint_fast16_t fullscale = display_getpwrfullwidth();	// ���������� ����� � ����������� �������� �� ������
#if WITHPWRLIN
	uint_fast8_t v = (uint_fast32_t) value * fullscale / ((uint_fast32_t) maxpwrcali);
	uint_fast8_t t = (uint_fast32_t) tracemax * fullscale / ((uint_fast32_t) maxpwrcali);
#else /* WITHPWRLIN */
	uint_fast8_t v = (uint_fast32_t) value * value * fullscale / ((uint_fast32_t) maxpwrcali * maxpwrcali);
	uint_fast8_t t = (uint_fast32_t) tracemax * tracemax * fullscale / ((uint_fast32_t) maxpwrcali * maxpwrcali);
#endif /* WITHPWRLIN */
	const uint_fast8_t mapleftval = display_mapbar(v, 0, fullscale, 0, v, fullscale);
	const uint_fast8_t mapleftmax = display_mapbar(t, 0, fullscale, fullscale, t, fullscale); // fullscale - invisible

	display_setcolors(PWRCOLOR, BGCOLOR);
	display_bars_address_pwr(x - 1, y, CHARS2GRID(0));
	display_string_P(PSTR("P"), 0);

	display_bars_address_pwr(x, y, CHARS2GRID(0));

	display_setcolors(PWRCOLOR, BGCOLOR);

	display_wrdatabar_begin();
	display_dispbar(BDTH_ALLPWR, mapleftval, mapleftmax, fullscale, PATTERN_BAR_HALF, PATTERN_BAR_FULL, PATTERN_BAR_EMPTYHALF);
	display_wrdatabar_end();

	if (BDTH_SPACEPWR != 0)
	{
		// ��������� ������ ����� �� ���������� ��������
		display_bars_address_pwr(x, y, CHARS2GRID(BDTH_ALLPWR));
		display_wrdatabar_begin();
		display_dispbar(BDTH_SPACEPWR, 0, 1, 1, PATTERN_SPACE, PATTERN_SPACE, PATTERN_SPACE);
		display_wrdatabar_end();
	}

#endif /* WITHBARS */
}

void display_smeter_amv0(
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t value,		// ������� ��������
	uint_fast8_t tracemax,	// ����� ������������ ���������
	uint_fast8_t level9,	// s9 level
	uint_fast8_t delta1,	// s9 - s0 delta
	uint_fast8_t delta2)	// s9+50 - s9 delta
{
#if WITHBARS
	tracemax = value > tracemax ? value : tracemax;	// ������ �� ��������������� ��������
	//delta1 = delta1 > level9 ? level9 : delta1;
	
	const uint_fast8_t leftmin = level9 - delta1;
	const uint_fast8_t mapleftval = display_mapbar(value, leftmin, level9, 0, value - leftmin, delta1);
	const uint_fast8_t mapleftmax = display_mapbar(tracemax, leftmin, level9, delta1, tracemax - leftmin, delta1); // delta1 - invisible
	const uint_fast8_t maprightval = display_mapbar(value, level9, level9 + delta2, 0, value - level9, delta2);
	const uint_fast8_t maprightmax = display_mapbar(tracemax, level9, level9 + delta2, delta2, tracemax - level9, delta2); // delta2 - invisible

	display_setcolors(LCOLOR, BGCOLOR);
	display_bars_address_rx(x - 1, y, CHARS2GRID(0));
	display_string_P(PSTR("S"), 0);

	display_bars_address_rx(x, y, CHARS2GRID(0));
	display_setcolors(LCOLOR, BGCOLOR);
	display_wrdatabar_begin();
	display_dispbar(BDTH_LEFTRX, mapleftval, mapleftmax, delta1, PATTERN_BAR_HALF, PATTERN_BAR_FULL, PATTERN_BAR_EMPTYHALF);		//���� 9 ������ ������
	display_wrdatabar_end();
	//
	display_bars_address_rx(x, y, CHARS2GRID(BDTH_LEFTRX));
	display_setcolors(RCOLOR, BGCOLOR);
	display_wrdatabar_begin();
	display_dispbar(BDTH_RIGHTRX, maprightval, maprightmax, delta2, PATTERN_BAR_FULL, PATTERN_BAR_FULL, PATTERN_BAR_EMPTYFULL);		// ���� 9 ������ ������ ���.
	display_wrdatabar_end();

	if (BDTH_SPACERX != 0)
	{
		display_bars_address_pwr(x, y, CHARS2GRID(BDTH_ALLRX));
		display_wrdatabar_begin();
		display_dispbar(BDTH_SPACERX, 0, 1, 1, PATTERN_SPACE, PATTERN_SPACE, PATTERN_SPACE);
		display_wrdatabar_end();
	}

#endif /* WITHBARS */
}

// ���������� ��� ������ ����� PWR & SWR
void display_pwrmeter(  
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t value,			// ������� ��������
	uint_fast8_t tracemax,		// max hold ��������
	uint_fast8_t maxpwrcali		// �������� ��� ���������� �� ��� �����
	)
{
#if WITHBARS
	const uint_fast16_t fullscale = display_getpwrfullwidth();	// ���������� ����� � ����������� �������� �� ������
#if WITHPWRLIN
	uint_fast8_t v = (uint_fast32_t) value * fullscale / ((uint_fast32_t) maxpwrcali);
	uint_fast8_t t = (uint_fast32_t) tracemax * fullscale / ((uint_fast32_t) maxpwrcali);
#else /* WITHPWRLIN */
	uint_fast8_t v = (uint_fast32_t) value * value * fullscale / ((uint_fast32_t) maxpwrcali * maxpwrcali);
	uint_fast8_t t = (uint_fast32_t) tracemax * tracemax * fullscale / ((uint_fast32_t) maxpwrcali * maxpwrcali);
#endif /* WITHPWRLIN */
	const uint_fast8_t mapleftval = display_mapbar(v, 0, fullscale, 0, v, fullscale);
	const uint_fast8_t mapleftmax = display_mapbar(t, 0, fullscale, fullscale, t, fullscale); // fullscale - invisible

	display_bars_address_pwr(x, y, CHARS2GRID(0));

	display_setcolors(PWRCOLOR, BGCOLOR);

	display_wrdatabar_begin();
	display_dispbar(BDTH_ALLPWR, mapleftval, mapleftmax, fullscale, PATTERN_BAR_HALF, PATTERN_BAR_FULL, PATTERN_BAR_EMPTYHALF);
	display_wrdatabar_end();

	if (BDTH_SPACEPWR != 0)
	{
		// ��������� ������ ����� �� ���������� ��������
		display_bars_address_pwr(x, y, CHARS2GRID(BDTH_ALLPWR));
		display_wrdatabar_begin();
		display_dispbar(BDTH_SPACEPWR, 0, 1, 1, PATTERN_SPACE, PATTERN_SPACE, PATTERN_SPACE);
		display_wrdatabar_end();
	}

#endif /* WITHBARS */
}

void display_smeter(
	uint_fast8_t x, 
	uint_fast8_t y, 
	uint_fast8_t value,		// ������� ��������
	uint_fast8_t tracemax,	// ����� ������������ ���������
	uint_fast8_t level9,	// s9 level
	uint_fast8_t delta1,	// s9 - s0 delta
	uint_fast8_t delta2)	// s9+50 - s9 delta
{
#if WITHBARS
	tracemax = value > tracemax ? value : tracemax;	// ������ �� ��������������� ��������
	//delta1 = delta1 > level9 ? level9 : delta1;
	
	const uint_fast8_t leftmin = level9 - delta1;
	const uint_fast8_t mapleftval = display_mapbar(value, leftmin, level9, 0, value - leftmin, delta1);
	const uint_fast8_t mapleftmax = display_mapbar(tracemax, leftmin, level9, delta1, tracemax - leftmin, delta1); // delta1 - invisible
	const uint_fast8_t maprightval = display_mapbar(value, level9, level9 + delta2, 0, value - level9, delta2);
	const uint_fast8_t maprightmax = display_mapbar(tracemax, level9, level9 + delta2, delta2, tracemax - level9, delta2); // delta2 - invisible

	display_bars_address_rx(x, y, CHARS2GRID(0));
	display_setcolors(LCOLOR, BGCOLOR);
	display_wrdatabar_begin();
	display_dispbar(BDTH_LEFTRX, mapleftval, mapleftmax, delta1, PATTERN_BAR_HALF, PATTERN_BAR_FULL, PATTERN_BAR_EMPTYHALF);		//���� 9 ������ ������
	display_wrdatabar_end();
	//
	display_bars_address_rx(x, y, CHARS2GRID(BDTH_LEFTRX));
	display_setcolors(RCOLOR, BGCOLOR);
	display_wrdatabar_begin();
	display_dispbar(BDTH_RIGHTRX, maprightval, maprightmax, delta2, PATTERN_BAR_FULL, PATTERN_BAR_FULL, PATTERN_BAR_EMPTYFULL);		// ���� 9 ������ ������ ���.
	display_wrdatabar_end();

	if (BDTH_SPACERX != 0)
	{
		display_bars_address_pwr(x, y, CHARS2GRID(BDTH_ALLRX));
		display_wrdatabar_begin();
		display_dispbar(BDTH_SPACERX, 0, 1, 1, PATTERN_SPACE, PATTERN_SPACE, PATTERN_SPACE);
		display_wrdatabar_end();
	}

#endif /* WITHBARS */
}
//--- bars

// ����������� ����� S-����� � ������ �����������
static void display2_legend_rx(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if defined(SMETERMAP)
	display_setcolors(MODECOLOR, BGCOLOR);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x, y + lowhalf);
		display_string_P(PSTR(SMETERMAP), lowhalf);

	} while (lowhalf --);
#endif /* defined(SMETERMAP) */
#if LCDMODE_LTDC_PIP16
	arm_hardware_ltdc_pip_off();
#endif /* LCDMODE_LTDC_PIP16 */
}

// ����������� ����� SWR-����� � ������ �����������
static void display2_legend_tx(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
#if defined(SWRPWRMAP) && WITHTX && (WITHSWRMTR || WITHSHOWSWRPWR)
	display_setcolors(MODECOLOR, BGCOLOR);
	uint_fast8_t lowhalf = HALFCOUNT_SMALL - 1;
	do
	{
		display_gotoxy(x, y + lowhalf);
		#if WITHSWRMTR
			#if WITHSHOWSWRPWR /* �� ������� ������������ ������������ SWR-meter � PWR-meter */
					display_string_P(PSTR(SWRPWRMAP), lowhalf);
			#else
					if (swrmode) 	// ���� TUNE �� ���������� ����� ���
						display_string_P(PSTR(SWRMAP), lowhalf);
					else
						display_string_P(PSTR(POWERMAP), lowhalf);
			#endif
		#elif WITHPWRMTR
					display_string_P(PSTR(POWERMAP), lowhalf);
		#else
			#warning No TX indication
		#endif
	} while (lowhalf --);

	#if LCDMODE_LTDC_PIP16
		arm_hardware_ltdc_pip_off();
	#endif /* LCDMODE_LTDC_PIP16 */

#endif /* defined(SWRPWRMAP) && WITHTX && (WITHSWRMTR || WITHSHOWSWRPWR) */
}


// ����������� ����� S-����� � ������ �����������
static void display2_legend(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
	if (hamradio_get_tx())
		display2_legend_tx(x, y, pv);
	else
		display2_legend_rx(x, y, pv);
}


#if WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192) && ! LCDMODE_HD44780

enum 
{
	ALLDX = GRID2X(CHARS2GRID(BDTH_ALLRX)),	// ������ �� ����������� � ��������
	ALLDY = GRID2Y(BDCV_ALLRX),				// ������ �� ��������� � �������� ����� ���������� ��������
	WFDY = GRID2Y(BDCV_WFLRX),				// ������ �� ��������� � �������� ����� ���������� ��������
	WFY0 = GRID2Y(BDCO_WFLRX),				// �������� �� ��������� � �������� ����� ���������� ��������
	SPDY = GRID2Y(BDCV_SPMRX),				// ������ �� ��������� � �������� ����� ���������� �������
	SPY0 = GRID2Y(BDCO_SPMRX)				// �������� �� ��������� � �������� ����� ���������� �������
};

#if LCDMODE_LTDC_PIP16

	// ���� ����� ���������� ��� �����������, ������ ��� ������������. ������ ��������� ����� ������������.
	enum { NPIPS = 3 };
	static RAMNOINIT_D1 ALIGNX_BEGIN PACKEDCOLOR565_T colorpips [NPIPS] [GXSIZE(ALLDX, ALLDY)] ALIGNX_END;
	static int pipphase;

	static void nextpip(void)
	{
		pipphase = (pipphase + 1) % NPIPS;
	}

#else /* LCDMODE_LTDC_PIP16 */

	static ALIGNX_BEGIN PACKEDCOLOR565_T colorpip0 [GXSIZE(ALLDX, ALLDY)] ALIGNX_END;

#endif /* LCDMODE_LTDC_PIP16 */

static PACKEDCOLOR565_T * getscratchpip(void)
{
#if LCDMODE_LTDC_PIP16
	return colorpips [pipphase];
#else /* LCDMODE_LTDC_PIP16 */
	return colorpip0;
#endif /* LCDMODE_LTDC_PIP16 */
}

static const FLOAT_t spectrum_beta = 0.25;					// incoming value coefficient
static const FLOAT_t spectrum_alpha = 1 - (FLOAT_t) 0.25;	// old value coefficient

static const FLOAT_t waterfall_beta = 0.75;					// incoming value coefficient
static const FLOAT_t waterfall_alpha = 1 - (FLOAT_t) 0.75;	// old value coefficient

static FLOAT_t spavgarray [ALLDX];	// ������ ������� ������ ��� ����������� (����� �������).

static FLOAT_t filter_waterfall(
	uint_fast16_t x
	)
{
	const FLOAT_t val = spavgarray [x];
	static FLOAT_t Yold [ALLDX];
	const FLOAT_t Y = Yold [x] * waterfall_alpha + waterfall_beta * val;
	Yold [x] = Y;
	return Y;
}

static FLOAT_t filter_spectrum(
	uint_fast16_t x
	)
{
	const FLOAT_t val = spavgarray [x];
	static FLOAT_t Yold [ALLDX];
	const FLOAT_t Y = Yold [x] * spectrum_alpha + spectrum_beta * val;
	Yold [x] = Y;
	return Y;
}

#if 1
	static uint8_t wfarray [WFDY][ALLDX];	// ������ "��������"
	static uint_fast16_t wfrow;				// ������, � ������� ��������� �������� ������
	static uint_fast32_t wffreq;			// ������� ������ �������, ��� ������� � ��������� ��� ����������.
#else
	static uint8_t wfarray [1][ALLDX];	// ������ "��������"
	enum { wfrow = 0 };				// ������, � ������� ��������� �������� ������
#endif
enum { PALETTESIZE = 256 };
static PACKEDCOLOR565_T wfpalette [PALETTESIZE];

#define COLOR565_GRIDCOLOR		TFTRGB565(128, 128, 0)		//COLOR_GRAY - center marker
#define COLOR565_GRIDCOLOR2		TFTRGB565(128, 0, 0x00)		//COLOR_DARKRED - other markers
#define COLOR565_SPECTRUMBG		TFTRGB565(0, 0, 0)			//COLOR_BLACK
#define COLOR565_SPECTRUMBG2	TFTRGB565(0, 64, 64)		//COLOR_XXX - ������ ����������� ���������
//#define COLOR565_SPECTRUMBG2	TFTRGB565(0x80, 0x80, 0x00)	//COLOR_OLIVE - ������ ����������� ���������
#define COLOR565_SPECTRUMFG		TFTRGB565(0, 255, 0)		//COLOR_GREEN
#define COLOR565_SPECTRUMFENCE	TFTRGB565(255, 255, 255)	//COLOR_WHITE

// ��� ���� �� ������� Malamute
static void wfpalette_initialize(void)
{
	int type = 0;
	int i;
	int a = 0;

	if (type)
	{
		for (i = 0; i < 42; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(0, 0, (int) (powf((float) 0.095 * i, 4)));
		}
		a += i;
		for (i = 0; i < 42; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(0, i * 6, 255);
		}
		a += i;
		for (i = 0; i < 42; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(0, 255, (int)(((float) 0.39 * (41 - i )) * ((float) 0.39 * (41 - i))) );
		}
		a += i;
		for (i = 0; i < 42; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(i * 6, 255, 0);
		}
		a += i;
		for (i = 0; i < 42; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(255, (41 - i) * 6, 0);
		}
		a += i;
		for (i = 0; i < 42; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(255, 0, i * 6);
		}
		a += i;
		// a = 252
	}
	else
	{

		// a = 0
		for (i = 0; i < 64; ++ i)
		{
			// ��� i = 0..15 ��������� ������� = ����
			wfpalette [a + i] = TFTRGB565(0, 0, (int) (powf((float) 0.0625 * i, 4)));	// ��������� ��������� ����� �������� ��������� ������������� ����������!
		}
		a += i;
		// a = 64
		for (i = 0; i < 32; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(0, i * 8, 255);
		}
		a += i;
		// a = 96
		for (i = 0; i < 32; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(0, 255, 255 - i * 8);
		}
		a += i;
		// a = 128
		for (i = 0; i < 32; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(i * 8, 255, 0);
		}
		a += i;
		// a = 160
		for (i = 0; i<64; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(255, 255 - i * 4, 0);
		}
		a += i;
		// a = 224
		for (i = 0; i < 32; ++ i)
		{
			wfpalette [a + i] = TFTRGB565(255, 0, i * 8);
		}
		a += i;
		// a = 256
	}
}

// ������������ ������ ������� ��� ������������ �����������
// ������� ��� ��������
static void dsp_latchwaterfall(
	uint_fast8_t x0, 
	uint_fast8_t y0, 
	void * pv
	)
{
	uint_fast16_t x;
	(void) x0;
	(void) y0;
	(void) pv;


	// ����������� ���������� ������� ��� �������������
	dsp_getspectrumrow(spavgarray, ALLDX);

	wfrow = (wfrow == 0) ? (WFDY - 1) : (wfrow - 1);

	// ����������� ���������� ������� ��� ��������
	for (x = 0; x < ALLDX; ++ x)
	{
		// ��� ���������� ��� ��������
		const int val = dsp_mag2y(filter_waterfall(x), PALETTESIZE - 1); // ���������� �������� �� 0 �� dy ������������
		// ������ � ����� ��������
		wfarray [wfrow] [x] = val;
	}
}

// ��������� ����������� ������� � ��������

static int_fast16_t glob_gridstep = 10000;	// 10 kHz - ��� �����

#if WITHDISPLAYNOFILLSPECTRUM
	/* �� �������� ����������� ������� ��� �������� ������� */
	static uint_fast8_t glob_nofill = 1;
#else /* WITHDISPLAYNOFILLSPECTRUM */
	static uint_fast8_t glob_nofill;
#endif /* WITHDISPLAYNOFILLSPECTRUM */

// �������� �������������� ������� ��� ��������� ���������� � ������
static uint_fast16_t
deltafreq2x(
	int_fast16_t delta,	// ���������� �� ����������� ������� � ������
	int_fast32_t bw,	// ������ ������
	uint_fast16_t width	// ������ ������
	)
{
	//const int_fast32_t dp = (delta + bw / 2) * width / bw;
	const int_fast32_t dp = (delta + bw / 2) * (width - 1) / bw;
	return dp;
}

// ��������� ������� �����.
// ������ RGB565
void display_colorbuffer_xor_vline(
	PACKEDCOLOR565_T * buffer,
	uint_fast16_t col,	// �������������� ���������� ������� (0..dx-1) ����� �������
	uint_fast16_t row0,	// ������������ ���������� ������� (0..dy-1) ������ ����
	uint_fast16_t h,	// ������
	COLOR565_T color
	)
{
	while (h --)
		display_colorbuffer_xor(buffer, ALLDX, ALLDY, col, row0 ++, color);
}

// ��������� ������� �����.
// ������ RGB565
static void 
display_colorbuffer_set_vline(
	PACKEDCOLOR565_T * buffer,
	uint_fast16_t col,	// �������������� ���������� ���������� ������� (0..dx-1) ����� �������
	uint_fast16_t row0,	// ������������ ���������� ���������� ������� (0..dy-1) ������ ����
	uint_fast16_t h,	// ������
	COLOR565_T color
	)
{
	while (h --)
		display_colorbuffer_set(buffer, ALLDX, ALLDY, col, row0 ++, color);
}

// ��������� �������� ������
static void 
display_colorgrid(
	PACKEDCOLOR565_T * buffer,
	uint_fast16_t row0,	// ������������ ���������� ������ ���������� ������� (0..dy-1) ������ ����
	uint_fast16_t h,			// ������
	int_fast32_t bw
	)
{
	COLOR565_T color0 = COLOR565_GRIDCOLOR;	// ������ �� ������
	COLOR565_T color = COLOR565_GRIDCOLOR2;
	// 
	const int_fast32_t gs = glob_gridstep;	// ��� �����
	const int_fast32_t halfbw = bw / 2;
	int_fast32_t df;	// ������� ����� ��������
	for (df = - halfbw / gs * gs; df < halfbw; df += gs)
	{
		uint_fast16_t xmarker;

		// ������ ����������� ������� ������ - XOR �����
		xmarker = deltafreq2x(df, bw, ALLDX);
		display_colorbuffer_xor_vline(buffer, xmarker, row0, h, df == 0 ? color0 : color);
	}
}

// ������ �� ����������� �������� 
// ��� �� �������,��� ���� ����������� �������� ��������� ��������.

#define HHWMG ((! LCDMODE_S1D13781_NHWACCEL && LCDMODE_S1D13781) || LCDMODE_UC1608 || LCDMODE_UC1601)

#if HHWMG
static ALIGNX_BEGIN GX_t spectrmonoscr [MGSIZE(ALLDX, SPDY)] ALIGNX_END;
#endif /* HHWMG */
// ���������� ����������� �������
static void display2_spectrum(
	uint_fast8_t x0, 
	uint_fast8_t y0, 
	void * pv
	)
{
#if HHWMG
	// ������ �� ����������� �������� 
	// ��� �� �������,��� ���� ����������� �������� ��������� ��������.

	if (hamradio_get_tx() == 0)
	{
		const int_fast32_t bw = dsp_get_samplerateuacin_rts();
		uint_fast16_t xleft = deltafreq2x(hamradio_getleft_bp(0), bw, ALLDX);	// ����� ���� ������
		uint_fast16_t xright = deltafreq2x(hamradio_getright_bp(0), bw, ALLDX);	// ������ ���� ������
		uint_fast16_t x;
		uint_fast16_t y;

		if (xright == xleft)
			xright = xleft + 1;

		// ������������ ������
		// ������ ����������� ������� ������
		memset(spectrmonoscr, 0xFF, sizeof spectrmonoscr);			// ��������� �������� �������� �����
		// ����������� �������
		uint_fast16_t xmarker = deltafreq2x(0, bw, ALLDX);
		for (y = 0; y < SPDY; ++ y)
		{
			display_pixelbuffer(spectrmonoscr, ALLDX, SPDY, xmarker, SPY0 + y);	// �������� �����
		}

		// ����������� �������
		for (x = 0; x < ALLDX; ++ x)
		{
			// �������� - � ������������ ����������
			const int val = dsp_mag2y(filter_spectrum(x), SPDY);	// ���������� �������� �� 0 �� SPDY ������������
			// ������������ ����������� ������.
			if (x >= xleft && x <= xright)
			{
				for (y = 0; y < SPDY; ++ y)
					display_pixelbuffer_xor(spectrmonoscr, ALLDX, SPDY, x, SPY0 + y);	// xor �����
			}
			// ������������ �������
			const int yv = SPDY - val;	//������������ �������, yv = 0..SPDY
			if (glob_nofill != 0)
			{
				if (yv < SPDY)
					display_pixelbuffer_xor(spectrmonoscr, ALLDX, SPDY, x, SPY0 + yv);	// xor �����
			}
			else
			{
				for (y = yv; y < SPDY; ++ y)
					display_pixelbuffer_xor(spectrmonoscr, ALLDX, SPDY, x, SPY0 + y);	// xor �����
			}
		}
	}
	else
	{
		memset(spectrmonoscr, 0xFF, sizeof spectrmonoscr);			// ��������� �������� �������� �����
	}
	display_setcolors(COLOR565_SPECTRUMBG, COLOR565_SPECTRUMFG);

#else /* */
	PACKEDCOLOR565_T * const colorpip = getscratchpip();
	(void) x0;
	(void) y0;
	(void) pv;
	// ������ �� ������� ��������, �� �������������� ����������� 
	// ���������� ����������� �� bitmap � ��������������
	if (hamradio_get_tx() == 0)
	{
		const int_fast32_t bw = dsp_get_samplerateuacin_rts();
		uint_fast16_t xleft = deltafreq2x(hamradio_getleft_bp(0), bw, ALLDX);	// ����� ���� ������
		uint_fast16_t xright = deltafreq2x(hamradio_getright_bp(0), bw, ALLDX);	// ������ ���� ������

		if (xright == xleft)
			xright = xleft + 1;

		uint_fast16_t x;
		uint_fast16_t y;
		// ����������� �������
		for (x = 0; x < ALLDX; ++ x)
		{
			// �������� - � ������������ ����������
			const int val = dsp_mag2y(filter_spectrum(x), SPDY);	// ���������� �������� �� 0 �� SPDY ������������
			// ������������ �������
			const int yv = SPDY - val;	// ������������ �������, yv = 0..SPDY

			// ������������ ���� ������ - ������� ����� �������
			//display_colorbuffer_set_vline(colorpip, x, SPY0, yv, COLOR565_SPECTRUMBG);
			display_colorbuffer_set_vline(colorpip, x, SPY0, yv, (x >= xleft && x <= xright) ? COLOR565_SPECTRUMBG2 : COLOR565_SPECTRUMBG);
			// ����� �� �������
			if (yv < SPDY)
			{
				display_colorbuffer_set(colorpip, ALLDX, ALLDY, x, yv + SPY0, COLOR565_SPECTRUMFENCE);	

				// ������ ����� ������
				const int yb = yv + 1;
				if (yb < SPDY)
				{
					if (glob_nofill != 0)
					{
						// ��� �������� ������ ���� �������
						display_colorbuffer_set_vline(colorpip, x, yb + SPY0, SPDY - yb, COLOR565_SPECTRUMBG);
					}
					else
					{
						// ������������ ������� ������� ������
						display_colorbuffer_set_vline(colorpip, x, yb + SPY0, SPDY - yb, COLOR565_SPECTRUMFG);
					}
				}
			}
		}
		display_colorgrid(colorpip, SPY0, SPDY, bw);	// ��������� �������� ������
	}

#endif
}

// ������� ������� ������ ����������� ��������
// � ������ wfrow - �����
static void wflclear(void)
{
	const size_t rowsize = sizeof wfarray [0];
	uint_fast16_t y;

	for (y = 0; y < sizeof wfarray / sizeof wfarray [0]; ++ y)
	{
		if (y == wfrow)
			continue;
		memset(wfarray [y], 0x00, rowsize);
	}
}

// ������� ����������� - ���� �������� �������� �����
// ����� ��������� ����� ������� �����������
// � ������ wfrow - �����
static void wflshiftleft(uint_fast16_t pixels)
{
	const size_t rowsize = sizeof wfarray [0];
	uint_fast16_t y;

	if (pixels == 0)
		return;
	for (y = 0; y < sizeof wfarray / sizeof wfarray [0]; ++ y)
	{
		if (y == wfrow)
			continue;
		memmove(wfarray [y] + 0, wfarray [y] + pixels, rowsize - pixels);
		memset(wfarray [y] + rowsize - pixels, 0x00, pixels);
	}
}

// ������� ����������� - ���� �������� �������� ������
// ����� ��������� ����� ������� �����������
// � ������ wfrow - �����
static void wflshiftright(uint_fast16_t pixels)
{
	const size_t rowsize = sizeof wfarray [0];
	uint_fast16_t y;

	if (pixels == 0)
		return;
	for (y = 0; y < sizeof wfarray / sizeof wfarray [0]; ++ y)
	{
		if (y == wfrow)
			continue;
		memmove(wfarray [y] + pixels, wfarray [y] + 0, rowsize - pixels);
		memset(wfarray [y] + 0, 0x00, pixels);
	}
}



// ��������� ����� ����������� ������ �� �������� (� ������ ������������� ����������� scroll �����������).
static void display_wfputrow(uint_fast16_t x, uint_fast16_t y, const uint8_t * p)
{
	enum { dx = ALLDX, dy = 1 };
	static ALIGNX_BEGIN PACKEDCOLOR565_T b [GXSIZE(dx, dy)] ALIGNX_END;
	uint_fast16_t xp; 
	for (xp = 0; xp < dx; ++ xp)
		display_colorbuffer_set(b, dx, dy, xp, 0, wfpalette [p [xp]]);

	// ������ ����������� ������� ������
	//display_colorbuffer_xor(b, dx, dy, dx / 2, 0, COLOR565_GRIDCOLOR);

	display_colorbuffer_show(b, dx, dy, x, y);
}

// ���������� ����������� ��������
static void display2_waterfall(
	uint_fast8_t x0, 
	uint_fast8_t y0, 
	void * pv
	)
{
#if (! LCDMODE_S1D13781_NHWACCEL && LCDMODE_S1D13781)

		const uint_fast32_t freq = hamradio_get_freq_a();	/* frequecy at midle of spectrum */
		const int_fast32_t bw = dsp_get_samplerateuacin_rts();
		uint_fast16_t x, y;
		const uint_fast16_t xm = deltafreq2x(0, bw, ALLDX);
		int_fast16_t hshift = 0;

	#if 1
		// ����� ������� ("�������")
		// �������� ����, ������������ ������ ������� ������
		display_scroll_down(GRID2X(x0), GRID2Y(y0) + WFY0, ALLDX, WFDY, 1, hshift);
		while (display_getreadystate() == 0)
			;
		x = 0;
		display_wfputrow(GRID2X(x0) + x, GRID2Y(y0) + 0 + WFY0, & wfarray [wfrow] [0]);	// display_plot inside for one row
	#elif 0
		// ����� ������� ("������")
		// �������� �����, ������������ ������ ������ ������
		display_scroll_up(GRID2X(x0), GRID2Y(y0) + WFY0, ALLDX, WFDY, 1, hshift);
		while (display_getreadystate() == 0)
			;
		x = 0;
		display_wfputrow(GRID2X(x0) + x, GRID2Y(y0) + WFDY - 1 + WFY0, & wfarray [wfrow] [0]);	// display_plot inside for one row
	#else
		// ����� ������� ("�������")
		// ������������ ���� �����
		while (display_getreadystate() == 0)
			;
		for (y = 0; y < WFDY; ++ y)
		{
			// ��������� ��������������� �������
			x = 0;
			display_wfputrow(GRID2X(x0) + x, GRID2Y(y0) + y + WFY0, & wfarray [(wfrow + y) % WFDY] [0]);	// display_plot inside for one row
		}
	#endif

#elif HHWMG
	// ������ �� ����������� �������� 
	// ��� �� �������,��� ���� ����������� �������� ��������� ��������.

	// ����� ������� ("�������") �� ����������� ��������

#else /* */
	// ����� ������� ("�������") �� ������� ��������
	(void) x0;
	(void) y0;
	(void) pv;

	if (hamradio_get_tx() == 0)
	{
		PACKEDCOLOR565_T * const colorpip = getscratchpip();
		const uint_fast32_t freq = hamradio_get_freq_a();	/* frequecy at midle of spectrum */
		const int_fast32_t bw = dsp_get_samplerateuacin_rts();
		uint_fast16_t x, y;
		const uint_fast16_t xm = deltafreq2x(0, bw, ALLDX);

#if ! WITHSEPARATEWFL
		if (wffreq == 0)
		{
			// ������� ������� ������ ����������� ��������
			// � ������ wfrow - �����
			wflclear();
		}
		else if (wffreq == freq)
		{
			// �� �������� �������
		}
		else if (wffreq > freq)
		{
			// ������� ����������� - ���� �������� �������� ������
			const uint_fast32_t delta = wffreq - freq;
			if (delta < bw / 2)
			{
				// ����� ��������� ����� ������� �����������
				// � ������ wfrow - �����
				wflshiftright(xm - deltafreq2x(0 - delta, bw, ALLDX));
			}
			else
			{
				// ������� ������� ������ ����������� ��������
				// � ������ wfrow - �����
				wflclear();
			}
		}
		else
		{
			// ������� ����������� - ���� �������� �������� �����
			const uint_fast32_t delta = freq - wffreq;
			if (delta < bw / 2)
			{
				// ����� ��������� ����� ������� �����������
				// � ������ wfrow - �����
				wflshiftleft(deltafreq2x(delta, bw, ALLDX) - xm);
			}
			else
			{
				// ������� ������� ������ ����������� ��������
				// � ������ 0 - �����
				wflclear();
			}
		}
#endif /* ! WITHSEPARATEWFL */

		// ������������ ������
		// ����� ������� ("�������")
		for (x = 0; x < ALLDX; ++ x)
		{
			for (y = 0; y < WFDY; ++ y)
			{
				display_colorbuffer_set(colorpip, ALLDX, ALLDY, x, y + WFY0, wfpalette [wfarray [(wfrow + y) % WFDY] [x]]);
			}
		}
#if 0
		if (1)
		{
			// ������ ����������� ������� ������
			// XOR �����
			uint_fast16_t xmarker = xm;
			display_colorbuffer_xor_vline(colorpip, xmarker, WFY0, WFDY, COLOR565_GRIDCOLOR);
		}
		else
		{
			display_colorgrid(colorpip, WFY0, WFDY, bw);	// ��������� �������� ������
		}
#endif
		wffreq = freq;
	}
	else
	{
		//wffreq = 0;
	}

#endif /* LCDMODE_S1D13781 */
}

// ����������� ����� ��������������� ������
// ����������� �������� �/��� �������
static void display2_colorbuff(
	uint_fast8_t x0, 
	uint_fast8_t y0, 
	void * pv
	)
{
#if HHWMG
	// ������ �� ����������� �������� 
	// ��� �� �������,��� ���� ����������� �������� ��������� ��������.
	display_showbuffer(spectrmonoscr, ALLDX, SPDY, x0, y0);

#else /* */

	PACKEDCOLOR565_T * const colorpip = getscratchpip();

	if (hamradio_get_tx() == 0)
	{
	}
	else
	{
		display_colorbuffer_fill(colorpip, ALLDX, ALLDY, COLOR_GRAY);
	}

#if LCDMODE_LTDC_PIP16
	display_colorbuffer_pip(colorpip, ALLDX, ALLDY);
	nextpip();
#else /* LCDMODE_LTDC_PIP16 */
	display_colorbuffer_show(colorpip, ALLDX, ALLDY, GRID2X(x0), GRID2Y(y0));
#endif /* LCDMODE_LTDC_PIP16 */
#endif /* LCDMODE_S1D13781 */
}

#else /* WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192) */

static void dsp_latchwaterfall(
	uint_fast8_t x0, 
	uint_fast8_t y0, 
	void * pv
	)
{
}

static void display2_spectrum(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
}

static void display2_waterfall(
	uint_fast8_t x, 
	uint_fast8_t y, 
	void * pv
	)
{
}

// ����������� ����� ��������������� ������
static void display2_colorbuff(
	uint_fast8_t x0, 
	uint_fast8_t y0, 
	void * pv
	)
{
}

#endif /* WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192) */

#define STMD 1

#if STMD

// ��������� state machine �����������
static uint_fast8_t reqs [REDRM_count];		// ������� �� �����������
static uint_fast8_t subsets [REDRM_count];	// �������� ������� �� ������ �����������.
static uint_fast8_t walkis [REDRM_count];	// ������ � ������ ���������� ����������� � ������ �������
static uint_fast8_t keyi;					// ������ �� �����������, ������������� ������.

#endif /* STMD */

static uint_fast8_t
getsubset(
	uint_fast8_t menuset,	/* ������ ������ ����������� (0..DISPLC_MODCOUNT - 1) */
	uint_fast8_t extra		/* ��������� � ������ ����������� �������� */
	)
{
	return extra ? REDRSUBSET_MENU : REDRSUBSET(menuset);
}

// ���������� ��������� ���� ��������� �� ���.
static void 
display_walktrough(
	uint_fast8_t key,
	uint_fast8_t subset,
	void * pv
	)
{
	enum { WALKCOUNT = sizeof dzones / sizeof dzones [0] };
	uint_fast8_t i;

	for (i = 0; i < WALKCOUNT; ++ i)
	{
		const FLASHMEM struct dzone * const p = & dzones [i];

		if ((p->key != key) || (p->subset & subset) == 0)
			continue;
		(* p->redraw)(p->x, p->y, pv);
	}
}


// ����� �� ���������� ��������� ���� ��������� ����� state machine.
static void 
display_walktroughsteps(
	uint_fast8_t key,
	uint_fast8_t subset
	)
{
#if STMD
	reqs [key] = 1;
	subsets [key] = subset;
	walkis [key] = 0;
#else /* STMD */

	display_walktrough(key, subset, NULL);

#endif /* STMD */
}

// Interface functions
// ���������� ����� state machine ����������� �������
void display2_bgprocess(void)
{
#if STMD
	enum { WALKCOUNT = sizeof dzones / sizeof dzones [0] };
	const uint_fast8_t keyi0 = keyi;

	for (;;)
	{
		if (reqs [keyi] != 0)
			break;
		keyi = (keyi == (REDRM_count - 1)) ? 0 : (keyi + 1);
		if (keyi == keyi0)
			return;			// �� ����� �� ������ �������
	}

	//return;
	for (; walkis [keyi] < WALKCOUNT; ++ walkis [keyi])
	{
		const FLASHMEM struct dzone * const p = & dzones [walkis [keyi]];

		if ((p->key != keyi) || (p->subset & subsets [keyi]) == 0)
			continue;
		(* p->redraw)(p->x, p->y, NULL);
		walkis [keyi] += 1;
		break;
	}
	if (walkis [keyi] >= WALKCOUNT)
	{
		reqs [keyi] = 0;	// ����� ������ �� ����������� ������� ���� ���������
		keyi = (keyi == (REDRM_count - 1)) ? 0 : (keyi + 1);
	}
#endif /* STMD */
}

// Interface functions
// ����� state machine ����������� ������� � �������� �������
void display2_bgreset(void)
{
	uint_fast8_t i;

	// �������� �������.
	display_clear();	

#if STMD
	// ����� state machine ����������� �������
	for (i = 0; i < REDRM_count; ++ i)
	{
		reqs [i] = 0;
		//walkis [keyi] = 0;
	}
	keyi = 0;
#endif /* STMD */

#if WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192) && ! LCDMODE_HD44780
	// ������������� ������� ��������
	wfpalette_initialize();
#endif /* WITHINTEGRATEDDSP && (WITHRTS96 || WITHRTS192) */
}

// Interface functions
void display_mode_subset(
	uint_fast8_t menuset	/* ������ ������ ����������� (0..DISPLC_MODCOUNT - 1) */
	)
{
	display_walktroughsteps(REDRM_MODE, getsubset(menuset, 0));
}

void display_barmeters_subset(
	uint_fast8_t menuset,	/* ������ ������ ����������� (0..3) */
	uint_fast8_t extra		/* ��������� � ������ ����������� �������� */
	)
{
	display_walktroughsteps(REDRM_BARS, getsubset(menuset, extra));
}

void display_dispfreq_ab(
	uint_fast8_t menuset	/* ������ ������ ����������� (0..DISPLC_MODCOUNT - 1) */
	)
{
	display_walktroughsteps(REDRM_FREQ, getsubset(menuset, 0));
	display_walktroughsteps(REDRM_FRQB, getsubset(menuset, 0));
}

void display_dispfreq_a2(
	uint_fast32_t freq,
	uint_fast8_t blinkpos,		// ������� (������� 10) �������������� �������
	uint_fast8_t blinkstate,	// � ����� �������������� ������� ������������ ������������� (0 - ������)
	uint_fast8_t menuset	/* ������ ������ ����������� (0..DISPLC_MODCOUNT - 1) */
	)
{
#if WITHDIRECTFREQENER
	struct editfreq ef;

	ef.freq = freq;
	ef.blinkpos = blinkpos;
	ef.blinkstate = blinkstate;

	display_walktrough(REDRM_FREQ,  getsubset(menuset, 0), & ef);
#else	/* WITHDIRECTFREQENER */
	display_walktroughsteps(REDRM_FREQ,  getsubset(menuset, 0));
#endif /* WITHDIRECTFREQENER */
}

void display_volts(
	uint_fast8_t menuset,	/* ������ ������ ����������� (0..DISPLC_MODCOUNT - 1) */
	uint_fast8_t extra		/* ��������� � ������ ����������� �������� */
	)
{
	display_walktroughsteps(REDRM_VOLT, getsubset(menuset, extra));
}

// ����������� �������� ��������� ��� ������
void display_menuitemlabel(
	void * pv,
	uint_fast8_t byname			/* ��� �������� ������ ���� � ���� */
	)
{
	display_walktrough(REDRM_FREQ, REDRSUBSET_MENU, NULL);
	display_walktrough(REDRM_FRQB, REDRSUBSET_MENU, NULL);
	display_walktrough(REDRM_MODE, REDRSUBSET_MENU, NULL);
	if (byname == 0)
	{
		display_walktrough(REDRM_MFXX, REDRSUBSET_MENU, pv);
	}
	display_walktrough(REDRM_MLBL, REDRSUBSET_MENU, pv);
	display_walktrough(REDRM_MVAL, REDRSUBSET_MENU, pv);
}

// ����������� �������� ���������
void display_menuitemvalue(
	void * pv
	)
{
	display_walktrough(REDRM_MVAL, REDRSUBSET_MENU, pv);
}


// ��������� ����� �������� ����������� (menuset)
uint_fast8_t display_getpagesmax(void)
{
	return DISPLC_MODCOUNT - 1;
}

// ����� �������� ����������� ��� "���"
uint_fast8_t display_getpagesleep(void)
{
	return PAGESLEEP;
}

// �������� ��������� ����������� ������� (��� ������� ������� �����)
uint_fast8_t display_getfreqformat(
	uint_fast8_t * prjv
	)
{
	* prjv = DISPLC_RJ;
	return DISPLC_WIDTH;
}
