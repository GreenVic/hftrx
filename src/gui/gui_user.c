/* $Id$ */
//
// Проект HF Dream Receiver (КВ приёмник мечты)
// автор Гена Завидовский mgs2001@mail.ru
// UA1ARN
//

#include "hardware.h"
#include "board.h"
#include "audio.h"

#include "src/display/display.h"
#include "src/display/fontmaps.h"
#include "formats.h"

#include <string.h>
#include <math.h>

#include "keyboard.h"
#include "codecs.h"

#include "src/gui/gui.h"

#if WITHTOUCHGUI

#include "src/gui/gui_user.h"
#include "src/gui/gui_system.h"
#include "src/gui/gui_structs.h"
#include "src/gui/gui_settings.h"

static void gui_main_process(void);
static void window_mode_process(void);
static void window_af_process(void);
static void window_freq_process (void);
static void window_swrscan_process(void);
static void window_tx_process(void);
static void window_tx_vox_process(void);
static void window_tx_power_process(void);
static void window_audiosettings_process(void);
static void window_ap_reverb_process(void);
static void window_ap_mic_eq_process(void);
static void window_ap_mic_process(void);
static void window_ap_mic_prof_process(void);
static void window_menu_process(void);
static void window_uif_process(void);
static void window_options_process(void);
static void window_utilites_process(void);
static void window_bands_process(void);
static void window_memory_process(void);
static void window_display_process(void);
static void window_receive_process(void);
static void window_notch_process(void);
static void window_gui_settings_process(void);

static window_t windows [] = {
//     window_id,   		 parent_id, 			align_mode,     x1, y1, w, h,   title,     		is_show, first_call, is_close, onVisibleProcess
	{ WINDOW_MAIN, 			 NO_PARENT_WINDOW, 		ALIGN_LEFT_X,	0, 0, 0, 0, "",  	   	   			 NON_VISIBLE, 0, 0, gui_main_process, },
	{ WINDOW_MODES, 		 WINDOW_RECEIVE, 		ALIGN_CENTER_X, 0, 0, 0, 0, "Select mode", 			 NON_VISIBLE, 0, 1, window_mode_process, },
	{ WINDOW_AF,    		 WINDOW_RECEIVE,		ALIGN_CENTER_X, 0, 0, 0, 0, "AF settings",    		 NON_VISIBLE, 0, 1, window_af_process, },
	{ WINDOW_FREQ,  		 WINDOW_BANDS,			ALIGN_CENTER_X, 0, 0, 0, 0, "Freq:", 	   			 NON_VISIBLE, 0, 1, window_freq_process, },
	{ WINDOW_MENU,  		 WINDOW_OPTIONS,		ALIGN_CENTER_X, 0, 0, 0, 0, "Settings",	   		 	 NON_VISIBLE, 0, 1, window_menu_process, },
	{ WINDOW_UIF, 			 NO_PARENT_WINDOW, 		ALIGN_LEFT_X, 	0, 0, 0, 0, "",   		   	 		 NON_VISIBLE, 0, 0, window_uif_process, },
	{ WINDOW_SWR_SCANNER,	 WINDOW_UTILS, 			ALIGN_CENTER_X, 0, 0, 0, 0, "SWR band scanner",		 NON_VISIBLE, 0, 0, window_swrscan_process, },
	{ WINDOW_AUDIOSETTINGS,  WINDOW_OPTIONS,		ALIGN_CENTER_X, 0, 0, 0, 0, "Audio settings", 		 NON_VISIBLE, 0, 1, window_audiosettings_process, },
	{ WINDOW_AP_MIC_EQ, 	 WINDOW_AUDIOSETTINGS, 	ALIGN_CENTER_X, 0, 0, 0, 0, "MIC TX equalizer",		 NON_VISIBLE, 0, 1, window_ap_mic_eq_process, },
	{ WINDOW_AP_REVERB_SETT, WINDOW_AUDIOSETTINGS, 	ALIGN_CENTER_X, 0, 0, 0, 0, "Reverberator settings", NON_VISIBLE, 0, 1, window_ap_reverb_process, },
	{ WINDOW_AP_MIC_SETT, 	 WINDOW_AUDIOSETTINGS, 	ALIGN_CENTER_X, 0, 0, 0, 0, "Microphone settings", 	 NON_VISIBLE, 0, 1, window_ap_mic_process, },
	{ WINDOW_AP_MIC_PROF, 	 WINDOW_AUDIOSETTINGS, 	ALIGN_CENTER_X, 0, 0, 0, 0, "Microphone profiles", 	 NON_VISIBLE, 0, 1, window_ap_mic_prof_process, },
	{ WINDOW_TX_SETTINGS, 	 WINDOW_OPTIONS, 		ALIGN_CENTER_X, 0, 0, 0, 0, "Transmit settings", 	 NON_VISIBLE, 0, 1, window_tx_process, },
	{ WINDOW_TX_VOX_SETT, 	 WINDOW_TX_SETTINGS, 	ALIGN_CENTER_X, 0, 0, 0, 0, "VOX settings", 	 	 NON_VISIBLE, 0, 1, window_tx_vox_process, },
	{ WINDOW_TX_POWER, 		 WINDOW_TX_SETTINGS, 	ALIGN_CENTER_X, 0, 0, 0, 0, "TX power", 	 	 	 NON_VISIBLE, 0, 1, window_tx_power_process, },
	{ WINDOW_OPTIONS, 		 NO_PARENT_WINDOW, 		ALIGN_CENTER_X,	0, 0, 0, 0, "Options",  	   	   	 NON_VISIBLE, 0, 1, window_options_process, },
	{ WINDOW_UTILS, 		 WINDOW_OPTIONS,		ALIGN_CENTER_X,	0, 0, 0, 0, "Utilites",  	   	   	 NON_VISIBLE, 0, 1, window_utilites_process, },
	{ WINDOW_BANDS, 		 NO_PARENT_WINDOW,		ALIGN_CENTER_X,	0, 0, 0, 0, "Bands",  	   	   	 	 NON_VISIBLE, 0, 1,	window_bands_process, },
	{ WINDOW_MEMORY, 		 NO_PARENT_WINDOW,		ALIGN_CENTER_X,	0, 0, 0, 0, "Memory",  	   	   	 	 NON_VISIBLE, 0, 1, window_memory_process, },
	{ WINDOW_DISPLAY, 		 WINDOW_OPTIONS,		ALIGN_CENTER_X,	0, 0, 0, 0, "Display settings",  	 NON_VISIBLE, 0, 1, window_display_process, },
	{ WINDOW_RECEIVE, 		 NO_PARENT_WINDOW, 		ALIGN_CENTER_X, 0, 0, 0, 0, "Receive settings", 	 NON_VISIBLE, 0, 1, window_receive_process, },
	{ WINDOW_NOTCH, 		 NO_PARENT_WINDOW, 		ALIGN_CENTER_X, 0, 0, 0, 0, "Notch", 	 	 		 NON_VISIBLE, 0, 1, window_notch_process, },
	{ WINDOW_GUI_SETTINGS, 	 WINDOW_OPTIONS, 		ALIGN_CENTER_X, 0, 0, 0, 0, "GUI settings",	 		 NON_VISIBLE, 0, 1, window_gui_settings_process, },
};

static enc2_menu_t gui_enc2_menu = { "", "", 0, 0, };
static menu_by_name_t menu_uif;

enc2step_t enc2step [] = {
	{ 100, "100 Hz", },
	{ 500, "500 Hz", },
};

struct gui_nvram_t gui_nvram;

enum { enc2step_vals = ARRAY_SIZE(enc2step) };

enum {
	enc2step_default = 1,
};

/* Возврат ссылки на окно */
window_t * get_win(uint8_t window_id)
{
	ASSERT(window_id < WINDOWS_COUNT);
	return & windows [window_id];
}

void gui_user_actions_after_close_window(void)
{
	hamradio_disable_encoder2_redirect();
}

void load_settings(void)
{
	hamradio_load_gui_settings(& gui_nvram);

	if (gui_nvram.enc2step_pos == 255)
		gui_nvram.enc2step_pos = enc2step_default;
}

void save_settings(void)
{
	hamradio_save_gui_settings(& gui_nvram);
}

// *********************************************************************************************************************************************************************

static void gui_main_process(void)
{
	window_t * win = get_win(WINDOW_MAIN);

	PACKEDCOLORMAIN_T * const fr = colmain_fb_draw();
	char buf [TEXT_ARRAY_SIZE];
	const uint_fast8_t buflen = ARRAY_SIZE(buf);
	uint_fast16_t y1 = 125, y2 = 145, current_place = 0, xx;
	const uint_fast8_t num_places = 8;
	const uint_fast8_t lbl_place_width = 100;
	uint_fast8_t update = 0;

	if (win->first_call)
	{
		uint_fast8_t interval_btn = 3;
		uint_fast16_t x = 0;
		ASSERT(win != NULL);
		win->first_call = 0;
		gui_enc2_menu.updated = 1;
		update = 1;

		static const button_t buttons [] = {
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 1, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_txrx", 	 "RX", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_Bands", 	 "Bands", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_Memory",  "Memory", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_Receive", "Receive|options", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 1, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_notch",   "", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_speaker", "Speaker|on air", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_1",  	 "", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_2", 		 "", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MAIN, NON_VISIBLE, INT32_MAX, "btn_Options", "Options", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		for (uint_fast8_t id = 0; id < win->bh_count; id ++)
		{
			button_t * bh = & win->bh_ptr [id];
			bh->x1 = x;
			bh->y1 = WITHGUIMAXY - bh->h - 1;
			bh->visible = VISIBLE;
			x = x + interval_btn + bh->w;
		}

		load_settings();
		elements_state(win);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)	// обработка короткого нажатия кнопок
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_txrx = find_gui_element(TYPE_BUTTON, win, "btn_txrx");
			button_t * btn_notch = find_gui_element(TYPE_BUTTON, win, "btn_notch");
			button_t * btn_Bands = find_gui_element(TYPE_BUTTON, win, "btn_Bands");
			button_t * btn_Memory = find_gui_element(TYPE_BUTTON, win, "btn_Memory");
			button_t * btn_Options = find_gui_element(TYPE_BUTTON, win, "btn_Options");
			button_t * btn_speaker = find_gui_element(TYPE_BUTTON, win, "btn_speaker");
			button_t * btn_Receive = find_gui_element(TYPE_BUTTON, win, "btn_Receive");


			if (bh == btn_notch)
			{
				hamradio_set_gnotch(! hamradio_get_gnotch());
				update = 1;
			}
#if WITHSPKMUTE
			else if (bh == btn_speaker)
			{
				hamradio_set_gmutespkr(! hamradio_get_gmutespkr());
				update = 1;
			}
#endif /* #if WITHSPKMUTE */
			else if (bh == btn_Bands)
			{
				if (check_for_parent_window() != NO_PARENT_WINDOW)
				{
					close_window(OPEN_PARENT_WINDOW);
					footer_buttons_state(CANCELLED);
				}
				else
				{
					window_t * win = get_win(WINDOW_BANDS);
					open_window(win);
					footer_buttons_state(DISABLED, btn_Bands);
				}
			}
			else if (bh == btn_Memory)
			{
				window_t * win = get_win(WINDOW_MEMORY);
				if (win->state == NON_VISIBLE)
				{
					open_window(win);
					footer_buttons_state(DISABLED, btn_Memory);
				}
				else
				{
					close_window(OPEN_PARENT_WINDOW);
					footer_buttons_state(CANCELLED);
				}
			}
			else if (bh == btn_Options)
			{
				if (check_for_parent_window() != NO_PARENT_WINDOW)
				{
					close_window(OPEN_PARENT_WINDOW);
					footer_buttons_state(CANCELLED);
					hamradio_set_lockmode(0);
					hamradio_disable_keyboard_redirect();
				}
				else
				{
					window_t * win = get_win(WINDOW_OPTIONS);
					open_window(win);
					footer_buttons_state(DISABLED, btn_Options);
				}
			}
			else if (bh == btn_Receive)
			{
				if (check_for_parent_window() != NO_PARENT_WINDOW)
				{
					close_window(OPEN_PARENT_WINDOW);
					footer_buttons_state(CANCELLED);
					hamradio_set_lockmode(0);
					hamradio_disable_keyboard_redirect();
				}
				else
				{
					window_t * win = get_win(WINDOW_RECEIVE);
					open_window(win);
					footer_buttons_state(DISABLED, btn_Receive);
				}
			}
#if WITHTX
			else if (bh == btn_txrx)
			{
				hamradio_moxmode(1);
				update = 1;
			}
#endif /* WITHTX */
		}
		else if (IS_BUTTON_LONG_PRESS)			// обработка длинного нажатия
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_txrx = find_gui_element(TYPE_BUTTON, win, "btn_txrx");
			button_t * btn_notch = find_gui_element(TYPE_BUTTON, win, "btn_notch");
#if WITHTX
			if (bh == btn_txrx)
			{
				hamradio_tunemode(1);
				update = 1;
			}
			else
#endif /* WITHTX */
			if (bh == btn_notch)
			{
				window_t * win = get_win(WINDOW_NOTCH);
				if (win->state == NON_VISIBLE)
				{
					open_window(win);
					footer_buttons_state(DISABLED, btn_notch);
				}
				else
				{
					close_window(OPEN_PARENT_WINDOW);
					footer_buttons_state(CANCELLED);
				}
			}
		}
		break;

	case WM_MESSAGE_UPDATE:

		update = 1;
		break;

	case WM_MESSAGE_ENC2_ROTATE:	// если не открыто 2-е окно, 2-й энкодер подстраивает частоту с округлением 500 гц от текущего значения
	{
		uint_fast16_t step = enc2step [gui_nvram.enc2step_pos].step;
		uint32_t freq = hamradio_get_freq_rx();
		uint16_t f_rem = freq % step;

		if (action > 0)
		{
			hamradio_set_freq(freq + (step - f_rem) * abs(action));
		}
		else if (action < 0)
		{
			hamradio_set_freq(freq - (f_rem ? f_rem : step) * abs(action));
		}
	}
		break;

	default:

		break;
	}

	if (update)											// обновление состояния элементов при действиях с ними, а также при запросах из базовой системы
	{
		button_t * btn_notch = find_gui_element(TYPE_BUTTON, win, "btn_notch");
		btn_notch->is_locked = hamradio_get_gnotch() ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		uint_fast8_t notch_type = hamradio_get_gnotchtype();
		if (notch_type == 1)
			local_snprintf_P(btn_notch->text, ARRAY_SIZE(btn_notch->text), PSTR("Notch|manual"));
		else if (notch_type == 2)
			local_snprintf_P(btn_notch->text, ARRAY_SIZE(btn_notch->text), PSTR("Notch|auto"));

		button_t * btn_speaker = find_gui_element(TYPE_BUTTON, win, "btn_speaker");
#if WITHSPKMUTE
		btn_speaker->is_locked = hamradio_get_gmutespkr() ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		local_snprintf_P(btn_speaker->text, ARRAY_SIZE(btn_speaker->text), PSTR("Speaker|%s"), btn_speaker->is_locked ? "muted" : "on air");
#else
		btn_speaker->state = DISABLED;
		local_snprintf_P(btn_speaker->text, ARRAY_SIZE(btn_speaker->text), PSTR("Speaker|on air"));
#endif /* #if WITHSPKMUTE */

		button_t * btn_txrx = find_gui_element(TYPE_BUTTON, win, "btn_txrx");
#if WITHTX
		uint_fast8_t tune = hamradio_tunemode(0);
		uint_fast8_t mox = hamradio_moxmode(0);

		if (tune)
		{
			btn_txrx->is_locked = BUTTON_LOCKED;
			local_snprintf_P(btn_txrx->text, ARRAY_SIZE(btn_txrx->text), PSTR("TX|tune"));
		}
		else if (! tune && mox)
		{
			btn_txrx->is_locked = BUTTON_LOCKED;
			local_snprintf_P(btn_txrx->text, ARRAY_SIZE(btn_txrx->text), PSTR("TX"));
		}
		else if (! tune && ! mox)
		{
			btn_txrx->is_locked = BUTTON_NON_LOCKED;
			local_snprintf_P(btn_txrx->text, ARRAY_SIZE(btn_txrx->text), PSTR("RX"));
		}
#else
		btn_txrx->state = DISABLED;
		local_snprintf_P(btn_txrx->text, ARRAY_SIZE(btn_txrx->text), PSTR("RX"));
#endif /* WITHTX */
	}

	// разметка инфобара
	for(uint_fast8_t i = 1; i < num_places; i++)
	{
		uint_fast16_t x = lbl_place_width * i;
		colmain_line(fr, DIM_X, DIM_Y, x, y1, x, y2 + SMALLCHARH2, COLORMAIN_GREEN, 0);
	}

	// параметры полосы пропускания фильтра
	{
		static uint_fast8_t bp_type, bp_low, bp_high;
		if (update)
		{
			bp_high = hamradio_get_high_bp(0);
			bp_low = hamradio_get_low_bp(0) * 10;
			bp_type = hamradio_get_bp_type();
			bp_high = bp_type ? (bp_high * 100) : (bp_high * 10);
		}
		local_snprintf_P(buf, buflen, PSTR("AF"));
		xx = current_place * lbl_place_width + 7;
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx, y1 + (y2 - y1) / 2, buf, COLORMAIN_WHITE);
		xx += SMALLCHARW2 * 3;
		local_snprintf_P(buf, buflen, bp_type ? (PSTR("L %d")) : (PSTR("W %d")), bp_low);
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx, y1, buf, COLORMAIN_WHITE);
		local_snprintf_P(buf, buflen, bp_type ? (PSTR("H %d")) : (PSTR("P %d")), bp_high);
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx, y2, buf, COLORMAIN_WHITE);
	}

	current_place ++;	// 2

	// значение сдвига частоты
	{
		static int_fast16_t if_shift;
		if (update)
			if_shift = hamradio_if_shift(0);
		xx = current_place * lbl_place_width + lbl_place_width / 2;
		if (if_shift)
		{
			local_snprintf_P(buf, buflen, PSTR("IF shift"));
			colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y1, buf, COLORMAIN_WHITE);
			local_snprintf_P(buf, buflen, if_shift == 0 ? PSTR("%d") : PSTR("%+d Hz"), if_shift);
			colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y2, buf, COLORMAIN_WHITE);
		}
		else
		{
			local_snprintf_P(buf, buflen, PSTR("IF shift"));
			colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y1 + (y2 - y1) / 2, buf, COLORMAIN_GRAY);
		}
	}

	current_place ++;	// 3

	// AGC
	{
		static int_fast8_t agc;
		if (update)
			agc = hamradio_get_agc_type();
		xx = current_place * lbl_place_width + lbl_place_width / 2;
		local_snprintf_P(buf, buflen, PSTR("AGC"));
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y1, buf, COLORMAIN_WHITE);
		local_snprintf_P(buf, buflen, agc ? PSTR("fast") : PSTR("slow"));
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y2, buf, COLORMAIN_WHITE);
	}

	current_place ++;	// 4


	// ширина панорамы
	{
#if WITHIF4DSP
		static int_fast32_t z;
		if (update)
			z = display_zoomedbw() / 1000;
		local_snprintf_P(buf, buflen, PSTR("SPAN"));
		xx = current_place * lbl_place_width + lbl_place_width / 2;
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y1, buf, COLORMAIN_WHITE);
		local_snprintf_P(buf, buflen, PSTR("%dk"), z);
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y2, buf, COLORMAIN_WHITE);
#endif /* WITHIF4DSP */
	}

	current_place ++;	// 5

	// напряжение питания
	{
#if WITHVOLTLEVEL
		static ldiv_t v;
		if (update)
			v = ldiv(hamradio_get_volt_value(), 10);
		local_snprintf_P(buf, buflen, PSTR("%d.%1dV"), v.quot, v.rem);
		xx = current_place * lbl_place_width + lbl_place_width / 2;
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, hamradio_get_tx() ? y1 : (y1 + (y2 - y1) / 2), buf, COLORMAIN_WHITE);
#endif /* WITHVOLTLEVEL */
	}

	// ток PA (при передаче)
	{
#if WITHCURRLEVEL
		if (hamradio_get_tx())
		{
			static int_fast16_t drain;
			if (update)
			{
				drain = hamradio_get_pacurrent_value();	// Ток в десятках милиампер (может быть отрицательным)
				if (drain < 0)
				{
					drain = 0;	// FIXME: без калибровки нуля (как у нас сейчас) могут быть ошибки установки тока
				}
			}

	#if (WITHCURRLEVEL_ACS712_30A || WITHCURRLEVEL_ACS712_20A)
			// для больших токов (более 9 ампер)
			ldiv_t t = ldiv(drain / 10, 10);
			local_snprintf_P(buf, buflen, PSTR("%2d.%01dA"), t.quot, t.rem);

	#else /* (WITHCURRLEVEL_ACS712_30A || WITHCURRLEVEL_ACS712_20A) */
			// Датчик тока до 5 ампер
			ldiv_t t = ldiv(drain, 100);
			local_snprintf_P(buf, buflen, PSTR("%d.%02dA"), t.quot, t.rem);

	#endif /* (WITHCURRLEVEL_ACS712_30A || WITHCURRLEVEL_ACS712_20A) */

			colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y2, buf, COLORMAIN_WHITE);
	}
#endif /* WITHCURRLEVEL */
	}

	current_place ++;	// 6

	// текущее время
	{
#if defined (RTC1_TYPE)
		static uint_fast16_t year;
		static uint_fast8_t month, day, hour, minute, secounds;
		board_rtc_getdatetime(& year, & month, & day, & hour, & minute, & secounds);
		local_snprintf_P(buf, buflen, PSTR("%02d.%02d"), day, month);
		xx = current_place * lbl_place_width + lbl_place_width / 2;
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y1, buf, COLORMAIN_WHITE);
		local_snprintf_P(buf, buflen, PSTR("%02d%c%02d"), hour, ((secounds & 1) ? ' ' : ':'), minute);
		colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y2, buf, COLORMAIN_WHITE);
#endif 	/* defined (RTC1_TYPE) */
	}

	current_place ++;	// 7

	// пусто

	current_place ++;	// 8

	// быстрое меню 2-го энкодера
	{
		if (gui_enc2_menu.state)
		{
			local_snprintf_P(buf, buflen, PSTR("%s"), gui_enc2_menu.param);
			remove_end_line_spaces(buf);
			xx = current_place * lbl_place_width + lbl_place_width / 2;
			colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y1, buf, COLORMAIN_WHITE);
			local_snprintf_P(buf, buflen, PSTR("%s"), gui_enc2_menu.val);
			remove_end_line_spaces(buf);
			COLORPIP_T color_lbl = gui_enc2_menu.state == 2 ? COLORMAIN_YELLOW : COLORMAIN_WHITE;
			colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y2, buf, color_lbl);
		}
		else
		{
			local_snprintf_P(buf, buflen, PSTR("------"));
			xx = current_place * lbl_place_width + lbl_place_width / 2;
			colpip_string2_tbg(fr, DIM_X, DIM_Y, xx - strwidth2(buf) / 2, y1 + (y2 - y1) / 2, buf, COLORMAIN_WHITE);
		}
	}

	{
	#if WITHTHERMOLEVEL	// температура выходных транзисторов (при передаче)
		static ldiv_t t;
		if (hamradio_get_tx())// && get_gui_1sec_timer())
		{
			t = ldiv(hamradio_get_temperature_value(), 10);
			local_snprintf_P(buf, buflen, PSTR("%d.%dC "), t.quot, t.rem);
			PRINTF("%s\n", buf);		// пока вывод в консоль
		}
#endif /* WITHTHERMOLEVEL */
	}
}

// *********************************************************************************************************************************************************************

static void window_memory_process(void)
{
	window_t * win = get_win(WINDOW_MEMORY);
	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 5;
		win->first_call = 0;

		win->bh_count = memory_cells_count;
		uint_fast16_t buttons_size = win->bh_count * sizeof (button_t);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);

		static const label_t labels [] = {
			{ 0, 0, WINDOW_MEMORY, DISABLED,  0, NON_VISIBLE, "lbl_note1", "",  FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0, WINDOW_MEMORY, DISABLED,  0, NON_VISIBLE, "lbl_note2", "",  FONT_MEDIUM, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);


		x = col1_int;
		y = row1_int;
		button_t * bh = NULL;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;
			bh->w = 100;
			bh->h = 44;
			bh->state = CANCELLED;
			bh->parent = WINDOW_MEMORY;
			bh->index = i;
			bh->is_long_press = 1;
			bh->is_locked = BUTTON_NON_LOCKED;
			local_snprintf_P(bh->name, ARRAY_SIZE(bh->name), PSTR("btn_memory_%02d"), i);

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}

			uint_fast32_t freq = hamradio_load_memory_cells(i, 0);
			if (freq > 0)
			{
				local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("%dk"), freq / 1000);
				bh->payload = 1;
			}
			else
			{
				local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("---"));
				bh->payload = 0;
			}
		}

		ASSERT(bh != NULL);
		label_t * lbl_note1 = find_gui_element(TYPE_LABEL, win, "lbl_note1");
		lbl_note1->x = col1_int;
		lbl_note1->y = bh->y1 + bh->h + get_label_height(lbl_note1);
		lbl_note1->visible = VISIBLE;
		local_snprintf_P(lbl_note1->text, ARRAY_SIZE(lbl_note1->text), PSTR("Long press on empty cell - sa"));

		label_t * lbl_note2 = find_gui_element(TYPE_LABEL, win, "lbl_note2");
		lbl_note2->x = lbl_note1->x + get_label_width(lbl_note1);
		lbl_note2->y = lbl_note1->y;
		lbl_note2->visible = VISIBLE;
		local_snprintf_P(lbl_note2->text, ARRAY_SIZE(lbl_note2->text), PSTR("ve, on saved cell - clean"));

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (type == TYPE_BUTTON)
		{
			if (IS_BUTTON_PRESS)
			{
				button_t * bh = (button_t *) ptr;
				uint_fast8_t cell_id = bh->index;

				if (bh->payload)
				{
					hamradio_load_memory_cells(cell_id, 1);
					close_window(DONT_OPEN_PARENT_WINDOW);
					footer_buttons_state(CANCELLED);
					return;
				}
			}
			else if (IS_BUTTON_LONG_PRESS)
			{
				button_t * bh = (button_t *) ptr;
				uint_fast8_t cell_id = bh->index;

				if (bh->payload)
				{
					bh->payload = 0;
					hamradio_clean_memory_cells(cell_id);
					local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("---"));
				}
				else
				{
					bh->payload = 1;
					uint_fast32_t freq = hamradio_get_freq_rx();
					local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("%dk"), freq / 1000);
					hamradio_save_memory_cells(cell_id);
				}
			}
		}

		break;

	default:

		break;
	}
}

// *********************************************************************************************************************************************************************

static void window_bands_process(void)
{
	window_t * win = get_win(WINDOW_BANDS);
	static band_array_t bands [30];

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0, max_x = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 3, i = 0;
		button_t * bh = NULL;
		win->first_call = 0;

		static const label_t labels [] = {
			{ 0, 0, WINDOW_BANDS, DISABLED,  0, NON_VISIBLE, "lbl_ham",   "HAM bands",		 FONT_LARGE, COLORMAIN_WHITE, },
			{ 0, 0, WINDOW_BANDS, DISABLED,  0, NON_VISIBLE, "lbl_bcast", "Broadcast bands", FONT_LARGE, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		win->bh_count = hamradio_get_bands(bands) + 1;
		uint_fast16_t buttons_size = win->bh_count * sizeof (button_t);
		win->bh_ptr = calloc(win->bh_count, sizeof (button_t));
		GUI_MEM_ASSERT(win->bh_ptr);

		label_t * lh1 = find_gui_element(TYPE_LABEL, win, "lbl_ham");
		lh1->x = col1_int;
		lh1->y = row1_int;
		lh1->visible = VISIBLE;

		x = col1_int;
		y = lh1->y + get_label_height(lh1) * 2;

		for (uint_fast8_t r = 1; i < win->bh_count; i ++, r ++)
		{
			bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			bh->w = 86;
			bh->h = 44;
			bh->state = CANCELLED;
			bh->parent = WINDOW_BANDS;

			max_x = (bh->x1 + bh->w > max_x) ? (bh->x1 + bh->w) : max_x;

			if (bands [i].type != BAND_TYPE_HAM)
			{
				// кнопка прямого ввода частоты
				local_snprintf_P(bh->name, ARRAY_SIZE(bh->name), PSTR("btn_freq"));
				local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("Freq|enter"));
				i ++;

				break;
			}

			bh->payload = bands [i].init_freq;

			char * div = strchr(bands [i].name, ' ');
			if(div)
				memcpy(div, "|", 1);

			local_snprintf_P(bh->name, ARRAY_SIZE(bh->name), PSTR("btn_ham_%d"), i);
			strcpy(bh->text, bands [i].name);

			if (hamradio_check_current_freq_by_band(bands [i].index))
				bh->is_locked = BUTTON_LOCKED;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
		}

		label_t * lh2 = find_gui_element(TYPE_LABEL, win, "lbl_bcast");
		lh2->x = max_x + 50;
		lh2->y = row1_int;
		lh2->visible = VISIBLE;

		x = lh2->x;
		y = lh1->y + get_label_height(lh1) * 2;

		for (uint_fast8_t r = 1; i < win->bh_count; i ++, r ++)
		{
			bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			bh->w = 86;
			bh->h = 44;
			bh->state = CANCELLED;
			bh->parent = WINDOW_BANDS;
			bh->payload = bands [i - 1].init_freq;
			local_snprintf_P(bh->name, ARRAY_SIZE(bh->name), PSTR("btn_bcast_%d"), i);
			strcpy(bh->text, bands [i - 1].name);

			if (hamradio_check_current_freq_by_band(bands [i - 1].index))
				bh->is_locked = BUTTON_LOCKED;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = lh2->x;
				y = y + bh->h + interval;
			}
		}

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_Freq = find_gui_element(TYPE_BUTTON, win, "btn_freq");

			if (bh == btn_Freq)
			{
				window_t * win = get_win(WINDOW_FREQ);
				open_window(win);
				hamradio_set_lockmode(1);
				hamradio_enable_keyboard_redirect();
			}
			else
			{
				hamradio_goto_band_by_freq(bh->payload);
				close_all_windows();
			}
		}
		break;

	default:

		break;
	}

}

// *********************************************************************************************************************************************************************

static void window_options_process(void)
{
	window_t * win = get_win(WINDOW_OPTIONS);

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 3;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_OPTIONS, NON_VISIBLE, INT32_MAX, "btn_SysMenu", "System|settings", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_OPTIONS, NON_VISIBLE, INT32_MAX, "btn_AUDsett", "Audio|settings", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_OPTIONS, NON_VISIBLE, INT32_MAX, "btn_TXsett",  "Transmit|settings", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_OPTIONS, NON_VISIBLE, INT32_MAX, "btn_Display", "Display|settings", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_OPTIONS, NON_VISIBLE, INT32_MAX, "btn_gui",   	 "GUI|settings", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_OPTIONS, NON_VISIBLE, INT32_MAX, "btn_Utils",   "Utils", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			button_t * bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
		}

		hamradio_disable_keyboard_redirect();
		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_gui = find_gui_element(TYPE_BUTTON, win, "btn_gui");
			button_t * btn_Utils = find_gui_element(TYPE_BUTTON, win, "btn_Utils");
			button_t * btn_TXsett = find_gui_element(TYPE_BUTTON, win, "btn_TXsett");
			button_t * btn_AUDsett = find_gui_element(TYPE_BUTTON, win, "btn_AUDsett");
			button_t * btn_SysMenu = find_gui_element(TYPE_BUTTON, win, "btn_SysMenu");
			button_t * btn_Display = find_gui_element(TYPE_BUTTON, win, "btn_Display");

			if (bh == btn_Utils)
			{
				window_t * win = get_win(WINDOW_UTILS);
				open_window(win);
			}
			else if (bh == btn_gui)
			{
				window_t * win = get_win(WINDOW_GUI_SETTINGS);
				open_window(win);
			}
			else if (bh == btn_TXsett)
			{
				window_t * win = get_win(WINDOW_TX_SETTINGS);
				open_window(win);
			}
			else if (bh == btn_AUDsett)
			{
				window_t * win = get_win(WINDOW_AUDIOSETTINGS);
				open_window(win);
			}
			else if (bh == btn_SysMenu)
			{
				window_t * win = get_win(WINDOW_MENU);
				open_window(win);
				hamradio_enable_encoder2_redirect();
			}
			else if (bh == btn_Display)
			{
				window_t * win = get_win(WINDOW_DISPLAY);
				open_window(win);
			}
		}
		break;

	default:

		break;
	}
}

// *********************************************************************************************************************************************************************

static void window_display_process(void)
{
	window_t * win = get_win(WINDOW_DISPLAY);
	static display_var_t display_t;

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 50, col1_int = 20, row1_int = window_title_height + 20;
		win->first_call = 0;
		display_t.change = 0;
		display_t.updated = 1;

		static const button_t buttons [] = {
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, INT32_MAX, "btn_zoom", "", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, INT32_MAX, "btn_view", "", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, INT32_MAX, "btn_params", "", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, -1, "btn_topSP_m", 	"-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, 1,  "btn_topSP_p", 	"+", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, -1, "btn_bottomSP_m", "-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, 1,  "btn_bottomSP_p", "+", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, -1, "btn_topWF_m", 	"-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, 1,  "btn_topWF_p", 	"+", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, -1, "btn_bottomWF_m", "-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_DISPLAY, NON_VISIBLE, 1,  "btn_bottomWF_p", "+", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_DISPLAY, CANCELLED, 0, NON_VISIBLE, "lbl_topSP",    "Top    : xxx db", FONT_MEDIUM, COLORMAIN_WHITE, TYPE_DISPLAY_SP_TOP, },
			{ 0, 0,	WINDOW_DISPLAY, CANCELLED, 0, NON_VISIBLE, "lbl_bottomSP", "Bottom : xxx db", FONT_MEDIUM, COLORMAIN_WHITE, TYPE_DISPLAY_SP_BOTTOM, },
			{ 0, 0,	WINDOW_DISPLAY, CANCELLED, 0, NON_VISIBLE, "lbl_topWF",    "Top    : xxx db", FONT_MEDIUM, COLORMAIN_WHITE, TYPE_DISPLAY_WF_TOP, },
			{ 0, 0,	WINDOW_DISPLAY, CANCELLED, 0, NON_VISIBLE, "lbl_bottomWF", "Bottom : xxx db", FONT_MEDIUM, COLORMAIN_WHITE, TYPE_DISPLAY_WF_BOTTOM, },
			{ 0, 0,	WINDOW_DISPLAY, DISABLED,  0, NON_VISIBLE, "lbl_sp",	   "Spectrum", 		  FONT_LARGE,  COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_DISPLAY, DISABLED,  0, NON_VISIBLE, "lbl_wf", 	   "Waterfall", 	  FONT_LARGE,  COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		label_t * lbl_sp = find_gui_element(TYPE_LABEL, win, "lbl_sp");
		label_t * lbl_wf = find_gui_element(TYPE_LABEL, win, "lbl_wf");
		label_t * lbl_topSP = find_gui_element(TYPE_LABEL, win, "lbl_topSP");
		label_t * lbl_bottomSP = find_gui_element(TYPE_LABEL, win, "lbl_bottomSP");
		label_t * lbl_topWF = find_gui_element(TYPE_LABEL, win, "lbl_topWF");
		label_t * lbl_bottomWF = find_gui_element(TYPE_LABEL, win, "lbl_bottomWF");

		button_t * btn_bottomSP_m = find_gui_element(TYPE_BUTTON, win, "btn_bottomSP_m");
		button_t * btn_bottomSP_p = find_gui_element(TYPE_BUTTON, win, "btn_bottomSP_p");
		button_t * btn_topSP_m = find_gui_element(TYPE_BUTTON, win, "btn_topSP_m");
		button_t * btn_topSP_p = find_gui_element(TYPE_BUTTON, win, "btn_topSP_p");
		button_t * btn_bottomWF_m = find_gui_element(TYPE_BUTTON, win, "btn_bottomWF_m");
		button_t * btn_bottomWF_p = find_gui_element(TYPE_BUTTON, win, "btn_bottomWF_p");
		button_t * btn_topWF_m = find_gui_element(TYPE_BUTTON, win, "btn_topWF_m");
		button_t * btn_topWF_p = find_gui_element(TYPE_BUTTON, win, "btn_topWF_p");

		button_t * btn_zoom = find_gui_element(TYPE_BUTTON, win, "btn_zoom");
		button_t * btn_view = find_gui_element(TYPE_BUTTON, win, "btn_view");
		button_t * btn_params = find_gui_element(TYPE_BUTTON, win, "btn_params");

		lbl_sp->y = row1_int;
		lbl_sp->visible = VISIBLE;
		lbl_topSP->x = col1_int;
		lbl_topSP->y = lbl_sp->y + interval;
		lbl_topSP->visible = VISIBLE;
		lbl_bottomSP->x = lbl_topSP->x;
		lbl_bottomSP->y = lbl_topSP->y + interval;
		lbl_bottomSP->visible = VISIBLE;
		btn_topSP_m->x1 = lbl_topSP->x + get_label_width(lbl_topSP) + 10;
		btn_topSP_m->y1 = lbl_topSP->y + get_label_height(lbl_topSP) / 2 - btn_topSP_m->h / 2;
		btn_topSP_m->visible = VISIBLE;
		btn_topSP_p->x1 = btn_topSP_m->x1 + btn_topSP_m->w + 10;
		btn_topSP_p->y1 = btn_topSP_m->y1;
		btn_topSP_p->visible = VISIBLE;
		btn_bottomSP_m->x1 = btn_topSP_m->x1;
		btn_bottomSP_m->y1 = lbl_bottomSP->y + get_label_height(lbl_bottomSP) / 2 - btn_bottomSP_m->h / 2;
		btn_bottomSP_m->visible = VISIBLE;
		btn_bottomSP_p->x1 = btn_bottomSP_m->x1 + btn_bottomSP_m->w + 10;
		btn_bottomSP_p->y1 = btn_bottomSP_m->y1;
		btn_bottomSP_p->visible = VISIBLE;
		lbl_sp->x = (lbl_topSP->x + btn_topSP_p->x1 + btn_topSP_p->w) / 2 - get_label_width(lbl_sp) / 2;

		lbl_topWF->x = btn_topSP_p->x1 + btn_topSP_p->w + interval / 2;
		lbl_topWF->y = lbl_topSP->y;
		lbl_topWF->visible = VISIBLE;
		lbl_bottomWF->x = lbl_topWF->x;
		lbl_bottomWF->y = lbl_topWF->y + interval;
		lbl_bottomWF->visible = VISIBLE;
		btn_topWF_m->x1 = lbl_topWF->x + get_label_width(lbl_topWF) + 10;
		btn_topWF_m->y1 = lbl_topWF->y + get_label_height(lbl_topWF) / 2 - btn_topWF_m->h / 2;
		btn_topWF_m->visible = VISIBLE;
		btn_topWF_p->x1 = btn_topWF_m->x1 + btn_topWF_m->w + 10;
		btn_topWF_p->y1 = btn_topWF_m->y1;
		btn_topWF_p->visible = VISIBLE;
		btn_bottomWF_m->x1 = btn_topWF_m->x1;
		btn_bottomWF_m->y1 = lbl_bottomWF->y + get_label_height(lbl_bottomWF) / 2 - btn_bottomWF_m->h / 2;
		btn_bottomWF_m->visible = VISIBLE;
		btn_bottomWF_p->x1 = btn_bottomWF_m->x1 + btn_bottomWF_m->w + 10;
		btn_bottomWF_p->y1 = btn_bottomWF_m->y1;
		btn_bottomWF_p->visible = VISIBLE;
		lbl_wf->x = (lbl_topWF->x + btn_topWF_p->x1 + btn_topWF_p->w) / 2 - get_label_width(lbl_wf) / 2;
		lbl_wf->y = lbl_sp->y;
		lbl_wf->visible = VISIBLE;

		btn_view->x1 = (lbl_sp->x + lbl_wf->x + get_label_width(lbl_wf)) / 2 - btn_view->w / 2;
		btn_view->y1 = btn_bottomWF_p->y1 + btn_bottomWF_p->h + interval / 2;
		btn_view->visible = VISIBLE;
		btn_params->x1 = btn_view->x1 + btn_view->w + interval / 2;
		btn_params->y1 = btn_view->y1;
		btn_params->visible = VISIBLE;
		btn_zoom->x1 = btn_view->x1 - btn_zoom->w - interval / 2;
		btn_zoom->y1 = btn_view->y1;
		btn_zoom->visible = VISIBLE;

		hamradio_enable_encoder2_redirect();
		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_bottomSP_m = find_gui_element(TYPE_BUTTON, win, "btn_bottomSP_m");
			button_t * btn_bottomSP_p = find_gui_element(TYPE_BUTTON, win, "btn_bottomSP_p");
			button_t * btn_topSP_m = find_gui_element(TYPE_BUTTON, win, "btn_topSP_m");
			button_t * btn_topSP_p = find_gui_element(TYPE_BUTTON, win, "btn_topSP_p");
			button_t * btn_bottomWF_m = find_gui_element(TYPE_BUTTON, win, "btn_bottomWF_m");
			button_t * btn_bottomWF_p = find_gui_element(TYPE_BUTTON, win, "btn_bottomWF_p");
			button_t * btn_topWF_m = find_gui_element(TYPE_BUTTON, win, "btn_topWF_m");
			button_t * btn_topWF_p = find_gui_element(TYPE_BUTTON, win, "btn_topWF_p");
			button_t * btn_zoom = find_gui_element(TYPE_BUTTON, win, "btn_zoom");
			button_t * btn_view = find_gui_element(TYPE_BUTTON, win, "btn_view");
			button_t * btn_params = find_gui_element(TYPE_BUTTON, win, "btn_params");

			if (bh == btn_view)
			{
				hamradio_change_view_style(1);
				display_t.updated = 1;
			}
			else if (bh == btn_zoom)
			{
				uint_fast8_t z = (hamradio_get_gzoomxpow2() + 1) % (BOARD_FFTZOOM_POW2MAX + 1);
				hamradio_set_gzoomxpow2(z);
				display_t.updated = 1;
			}
			else if (bh == btn_params)
			{
				hamradio_set_gwflevelsep(! hamradio_get_gwflevelsep());
				display_t.updated = 1;
			}
			else if (bh == btn_topSP_m || bh == btn_topSP_p)
			{
				hamradio_gtopdbsp(bh->payload);
				display_t.updated = 1;
			}
			else if (bh == btn_bottomSP_m || bh == btn_bottomSP_p)
			{
				hamradio_gbottomdbsp(bh->payload);
				display_t.updated = 1;
			}
			else if (bh == btn_topWF_m || bh == btn_topWF_p)
			{
				hamradio_gtopdbwf(bh->payload);
				display_t.updated = 1;
			}
			else if (bh == btn_bottomWF_m || bh == btn_bottomWF_p)
			{
				hamradio_gbottomdbwf(bh->payload);
				display_t.updated = 1;
			}
		}
		else if (IS_LABEL_PRESS)
		{
			label_t * lh = (label_t *) ptr;

			display_t.select = lh->index;
			display_t.change = 0;
			display_t.updated = 1;
		}

		break;

	case WM_MESSAGE_ENC2_ROTATE:

		display_t.change = action;
		display_t.updated = 1;

		break;

	case WM_MESSAGE_UPDATE:

		display_t.updated = 1;
		display_t.change = 0;
		break;

	default:

		break;
	}

	if (display_t.updated)
	{
		display_t.updated = 0;

		button_t * btn_view = find_gui_element(TYPE_BUTTON, win, "btn_view");
		local_snprintf_P(btn_view->text, ARRAY_SIZE(btn_view->text), PSTR("View|%s"), hamradio_change_view_style(0));
		remove_end_line_spaces(btn_view->text);

		button_t * btn_zoom = find_gui_element(TYPE_BUTTON, win, "btn_zoom");
		local_snprintf_P(btn_zoom->text, ARRAY_SIZE(btn_zoom->text), PSTR("Zoom|x%d"), 1 << hamradio_get_gzoomxpow2());

		button_t * btn_params = find_gui_element(TYPE_BUTTON, win, "btn_params");
		local_snprintf_P(btn_params->text, ARRAY_SIZE(btn_params->text), PSTR("WF params|%s"), hamradio_get_gwflevelsep() ? "separate" : "from SP");

		for(uint_fast8_t i = 0; i < win->lh_count; i ++)
			win->lh_ptr [i].color = COLORMAIN_WHITE;

		ASSERT(display_t.select < win->lh_count);
		win->lh_ptr [display_t.select].color = COLORMAIN_YELLOW;

		label_t * lbl_topSP = find_gui_element(TYPE_LABEL, win, "lbl_topSP");
		label_t * lbl_bottomSP = find_gui_element(TYPE_LABEL, win, "lbl_bottomSP");
		label_t * lbl_topWF = find_gui_element(TYPE_LABEL, win, "lbl_topWF");
		label_t * lbl_bottomWF = find_gui_element(TYPE_LABEL, win, "lbl_bottomWF");

		local_snprintf_P(lbl_topSP->text, ARRAY_SIZE(lbl_topSP->text), PSTR("Top    : %3d db"),
				hamradio_gtopdbsp(display_t.select == TYPE_DISPLAY_SP_TOP ? display_t.change : 0));

		local_snprintf_P(lbl_bottomSP->text, ARRAY_SIZE(lbl_bottomSP->text), PSTR("Bottom : %3d db"),
				hamradio_gbottomdbsp(display_t.select == TYPE_DISPLAY_SP_BOTTOM ? display_t.change : 0));

		local_snprintf_P(lbl_topWF->text, ARRAY_SIZE(lbl_topWF->text), PSTR("Top    : %3d db"),
				hamradio_gtopdbwf(display_t.select == TYPE_DISPLAY_WF_TOP ? display_t.change : 0));

		local_snprintf_P(lbl_bottomWF->text, ARRAY_SIZE(lbl_bottomWF->text), PSTR("Bottom : %3d db"),
				hamradio_gbottomdbwf(display_t.select == TYPE_DISPLAY_WF_BOTTOM ? display_t.change : 0));

		display_t.change = 0;
	}
}

// *********************************************************************************************************************************************************************

static void window_utilites_process(void)
{
	window_t * win = get_win(WINDOW_UTILS);
	uint_fast8_t update = 0;

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 4;
		win->first_call = 0;
		update = 1;

		static const button_t buttons [] = {
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_UTILS, NON_VISIBLE, INT32_MAX, "btn_SWRscan", "SWR|scanner", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			button_t * bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
		}

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_SWRscan = find_gui_element(TYPE_BUTTON, win, "btn_SWRscan");
#if WITHTX
			if (bh == btn_SWRscan)
			{
				window_t * winSWR = get_win(WINDOW_SWR_SCANNER);
				open_window(winSWR);
			}
#endif /* WITHTX */
		}
		break;

	default:

		break;
	}

	if (update)
	{
		update = 0;

		button_t * btn_SWRscan = find_gui_element(TYPE_BUTTON, win, "btn_SWRscan");
#if WITHTX
		btn_SWRscan->state = CANCELLED;
#else
		btn_SWRscan->state = DISABLED;
#endif /* WITHTX */
	}
}

// *********************************************************************************************************************************************************************

static void window_mode_process(void)
{
	window_t * win = get_win(WINDOW_MODES);

	if (win->first_call)
	{
		uint_fast16_t x, y;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 4;
		uint_fast8_t id_start, id_end;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MODES, NON_VISIBLE, SUBMODE_LSB, "btn_ModeLSB", "LSB", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MODES, NON_VISIBLE, SUBMODE_CW,  "btn_ModeCW",  "CW", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MODES, NON_VISIBLE, SUBMODE_AM,  "btn_ModeAM",  "AM", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MODES, NON_VISIBLE, SUBMODE_DGL, "btn_ModeDGL", "DGL", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MODES, NON_VISIBLE, SUBMODE_USB, "btn_ModeUSB", "USB", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MODES, NON_VISIBLE, SUBMODE_CWR, "btn_ModeCWR", "CWR", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MODES, NON_VISIBLE, SUBMODE_NFM, "btn_ModeNFM", "NFM", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MODES, NON_VISIBLE, SUBMODE_DGU, "btn_ModeDGU", "DGU", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			button_t * bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;
			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = row1_int + bh->h + interval;
			}
		}

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;

			if (bh->payload != INT32_MAX)
				hamradio_change_submode(bh->payload, 1);

			close_window(DONT_OPEN_PARENT_WINDOW);
			footer_buttons_state(CANCELLED);
			return;
		}
		break;

	default:

		break;
	}
}

// *********************************************************************************************************************************************************************

static void window_af_process(void)
{
	window_t * win = get_win(WINDOW_AF);
	static bp_var_t bp_t;

	if (win->first_call)
	{
		win->first_call = 0;
		uint_fast8_t interval = 50, col1_int = 20;
		bp_t.change = 0;
		bp_t.updated = 1;

		static const button_t buttons [] = {
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AF, NON_VISIBLE, -1, "btn_low_m", 	"-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AF, NON_VISIBLE, 1,  "btn_low_p", 	"+", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AF, NON_VISIBLE, -1, "btn_high_m", 	"-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AF, NON_VISIBLE, 1,  "btn_high_p", 	"+", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AF, NON_VISIBLE, -1, "btn_afr_m", 	"-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AF, NON_VISIBLE, 1,  "btn_afr_p", 	"+", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AF, NON_VISIBLE, -1, "btn_ifshift_m", "-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AF, NON_VISIBLE, 1,  "btn_ifshift_p", "+", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0, WINDOW_AF, CANCELLED, 0, NON_VISIBLE, "lbl_low",     "Low  cut : **** ", FONT_MEDIUM, COLORMAIN_WHITE, TYPE_BP_LOW, },
			{ 0, 0, WINDOW_AF, CANCELLED, 0, NON_VISIBLE, "lbl_high",    "High cut : **** ", FONT_MEDIUM, COLORMAIN_WHITE, TYPE_BP_HIGH, },
			{ 0, 0, WINDOW_AF, CANCELLED, 0, NON_VISIBLE, "lbl_afr",     "AFR      : **** ", FONT_MEDIUM, COLORMAIN_WHITE, TYPE_AFR, },
			{ 0, 0, WINDOW_AF, CANCELLED, 0, NON_VISIBLE, "lbl_ifshift", "IF shift : **** ", FONT_MEDIUM, COLORMAIN_WHITE, TYPE_IF_SHIFT, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		label_t * lbl_low = find_gui_element(TYPE_LABEL, win, "lbl_low");
		label_t * lbl_high = find_gui_element(TYPE_LABEL, win, "lbl_high");
		label_t * lbl_afr = find_gui_element(TYPE_LABEL, win, "lbl_afr");
		label_t * lbl_ifshift = find_gui_element(TYPE_LABEL, win, "lbl_ifshift");

		button_t * bh = & win->bh_ptr [0];

		lbl_low->x = col1_int;
		lbl_low->y = interval;
		lbl_low->visible = VISIBLE;

		lbl_high->x = lbl_low->x;
		lbl_high->y = lbl_low->y + interval;
		lbl_high->visible = VISIBLE;

		lbl_afr->x = lbl_high->x;
		lbl_afr->y = lbl_high->y + interval;
		lbl_afr->visible = VISIBLE;

		lbl_ifshift->x = lbl_afr->x;
		lbl_ifshift->y = lbl_afr->y + interval;
		lbl_ifshift->visible = VISIBLE;

		uint_fast16_t x = lbl_low->x + get_label_width(lbl_low);
		uint_fast16_t y = lbl_low->y + get_label_height(lbl_low) / 2 - bh->h / 2;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + bh->w + 10;
			if (r >= 2)
			{
				r = 0;
				x = lbl_low->x + get_label_width(lbl_low);
				y = y + interval;
			}
		}

		hamradio_enable_encoder2_redirect();
		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_low_m = find_gui_element(TYPE_BUTTON, win, "btn_low_m");
			button_t * btn_low_p = find_gui_element(TYPE_BUTTON, win, "btn_low_p");
			button_t * btn_high_m = find_gui_element(TYPE_BUTTON, win, "btn_high_m");
			button_t * btn_high_p = find_gui_element(TYPE_BUTTON, win, "btn_high_p");
			button_t * btn_afr_m = find_gui_element(TYPE_BUTTON, win, "btn_afr_m");
			button_t * btn_afr_p = find_gui_element(TYPE_BUTTON, win, "btn_afr_p");
			button_t * btn_ifshift_m = find_gui_element(TYPE_BUTTON, win, "btn_ifshift_m");
			button_t * btn_ifshift_p = find_gui_element(TYPE_BUTTON, win, "btn_ifshift_p");

			if (bh == btn_low_m || bh == btn_low_p)
			{
				bp_t.select = TYPE_BP_LOW;
				bp_t.change = bh->payload;
				bp_t.updated = 1;
			}
			else if (bh == btn_high_m || bh == btn_high_p)
			{
				bp_t.select = TYPE_BP_HIGH;
				bp_t.change = bh->payload;
				bp_t.updated = 1;
			}
			else if (bh == btn_afr_m || bh == btn_afr_p)
			{
				bp_t.select = TYPE_AFR;
				bp_t.change = bh->payload;
				bp_t.updated = 1;
			}
			else if (bh == btn_ifshift_m || bh == btn_ifshift_p)
			{
				bp_t.select = TYPE_IF_SHIFT;
				bp_t.change = bh->payload;
				bp_t.updated = 1;
			}
		}
		else if (IS_LABEL_PRESS)
		{
			label_t * lh = (label_t *) ptr;
			bp_t.select = lh->index;
			bp_t.change = 0;
			bp_t.updated = 1;
		}
		break;

	case WM_MESSAGE_ENC2_ROTATE:

		bp_t.change = action;
		bp_t.updated = 1;
		break;

	case WM_MESSAGE_UPDATE:

		bp_t.change = 0;
		bp_t.updated = 1;
		break;

	default:

		break;
	}

	if (bp_t.updated)
	{
		bp_t.updated = 0;

		char str_low [TEXT_ARRAY_SIZE], str_high [TEXT_ARRAY_SIZE];
		uint_fast8_t bp_type = hamradio_get_bp_type();
		if (bp_type)						// BWSET_WIDE
		{
			strcpy(str_low,  "Low  cut ");
			strcpy(str_high, "High cut ");
		}
		else								// BWSET_NARROW
		{
			strcpy(str_low,  "Width    ");
			strcpy(str_high, "Pitch    ");
		}

		for(uint_fast8_t i = 0; i < win->lh_count; i ++)
			win->lh_ptr [i].color = COLORMAIN_WHITE;

		ASSERT(bp_t.select < win->lh_count);
		win->lh_ptr [bp_t.select].color = COLORMAIN_YELLOW;

		label_t * lbl_low = find_gui_element(TYPE_LABEL, win, "lbl_low");
		label_t * lbl_high = find_gui_element(TYPE_LABEL, win, "lbl_high");
		label_t * lbl_afr = find_gui_element(TYPE_LABEL, win, "lbl_afr");
		label_t * lbl_ifshift = find_gui_element(TYPE_LABEL, win, "lbl_ifshift");

		uint_fast8_t val_low = hamradio_get_low_bp(bp_t.select == TYPE_BP_LOW ? (bp_t.change * 5) : 0);
		local_snprintf_P(lbl_low->text, ARRAY_SIZE(lbl_low->text), PSTR("%s: %4d"), str_low, val_low * 10);

		uint_fast8_t val_high = hamradio_get_high_bp(bp_t.select == TYPE_BP_HIGH ? bp_t.change : 0) * (bp_type ? 100 : 10);
		local_snprintf_P(lbl_high->text, ARRAY_SIZE(lbl_high->text), PSTR("%s: %4d"), str_high, val_high);

		local_snprintf_P(lbl_afr->text, ARRAY_SIZE(lbl_afr->text), PSTR("AFR      : %+4d "),
				hamradio_afresponce(bp_t.select == TYPE_AFR ? bp_t.change : 0));

		int16_t shift = hamradio_if_shift(bp_t.select == TYPE_IF_SHIFT ? bp_t.change : 0);
		if (shift)
			local_snprintf_P(lbl_ifshift->text, ARRAY_SIZE(lbl_ifshift->text), PSTR("IF shift :%+5d"), shift);
		else
			local_snprintf_P(lbl_ifshift->text, ARRAY_SIZE(lbl_ifshift->text), PSTR("IF shift :  OFF"));

		bp_t.change = 0;
	}
}

// *********************************************************************************************************************************************************************

static void window_freq_process (void)
{
	static label_t * lbl_freq;
	static editfreq_t editfreq;
	window_t * win = get_win(WINDOW_FREQ);

	if (win->first_call)
	{
		uint_fast16_t x, y;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 4;
		win->first_call = 0;
		button_t * bh = NULL;

		static const button_t buttons [] = {
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 1, 		 		"btn_Freq1",  "1", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 2, 		 		"btn_Freq2",  "2", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 3, 		 		"btn_Freq3",  "3", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, BUTTON_CODE_BK, 	"btn_FreqBK", "<-", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 4, 	 			"btn_Freq4",  "4", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 5, 				"btn_Freq5",  "5", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 6, 				"btn_Freq6",  "6", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, BUTTON_CODE_OK, 	"btn_FreqOK", "OK", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 7, 				"btn_Freq7",  "7", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 8,  				"btn_Freq8",  "8", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 9, 		 		"btn_Freq9",  "9", },
			{ 0, 0, 50, 50, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_FREQ, NON_VISIBLE, 0, 	 			"btn_Freq0",  "0", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_FREQ, DISABLED, 0, NON_VISIBLE, "lbl_freq_val", "", FONT_LARGE, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
		}

		bh = find_gui_element(TYPE_BUTTON, win, "btn_FreqOK");
		bh->is_locked = BUTTON_LOCKED;

		lbl_freq = find_gui_element(TYPE_LABEL, win, "lbl_freq_val");
		lbl_freq->x = strwidth(win->name) + strwidth(" ") + 20;
		lbl_freq->y = 5;
		strcpy(lbl_freq->text, "    0 k");
		lbl_freq->color = COLORMAIN_WHITE;
		lbl_freq->visible = VISIBLE;

		editfreq.val = 0;
		editfreq.num = 0;
		editfreq.key = BUTTON_CODE_DONE;

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS && editfreq.key == BUTTON_CODE_DONE)
		{
			button_t * bh = (button_t *) ptr;
			editfreq.key = bh->payload;
		}
		break;

	default:

		break;
	}

	if (editfreq.key != BUTTON_CODE_DONE)
	{
		if (lbl_freq->color == COLORMAIN_RED)
		{
			editfreq.val = 0;
			editfreq.num = 0;
		}

		lbl_freq->color = COLORMAIN_WHITE;

		switch (editfreq.key)
		{
		case BUTTON_CODE_BK:
			if (editfreq.num > 0)
			{
				editfreq.val /= 10;
				editfreq.num --;
			}
			break;

		case BUTTON_CODE_OK:
			if (hamradio_set_freq(editfreq.val * 1000) || editfreq.val == 0)
			{
				close_all_windows();
			}
			else
				lbl_freq->color = COLORMAIN_RED;

			break;

		default:
			if (editfreq.num < 6)
			{
				editfreq.val  = editfreq.val * 10 + editfreq.key;
				if (editfreq.val)
					editfreq.num ++;
			}
		}
		editfreq.key = BUTTON_CODE_DONE;

		local_snprintf_P(lbl_freq->text, ARRAY_SIZE(lbl_freq->text), PSTR("%5d k"), editfreq.val);
	}
}

// *********************************************************************************************************************************************************************

static void window_swrscan_process(void)
{
#if WITHTX
	PACKEDCOLORMAIN_T * const fr = colmain_fb_draw();
	uint_fast16_t gr_w = 500, gr_h = 250;												// размеры области графика
	uint_fast8_t interval = 20, col1_int = 20, row1_int = 30;
	uint_fast16_t x0 = col1_int + interval * 2, y0 = row1_int + gr_h - interval * 2;	// нулевые координаты графика
	uint_fast16_t x1 = x0 + gr_w - interval * 4, y1 = gr_h - y0 + interval * 2;			// размеры осей графика
	static uint_fast16_t mid_w = 0, freq_step = 0;
	static uint_fast16_t i, current_freq_x;
	static uint_fast32_t lim_bottom, lim_top, swr_freq, backup_freq;
	static label_t * lbl_swr_error;
	static button_t * btn_swr_start, * btn_Options, * btn_swr_OK;
	static uint_fast8_t backup_power;
	static uint_fast8_t swr_scan_done = 0, is_swr_scanning = 0;
	static uint_fast8_t swr_scan_enable = 0;		// флаг разрешения сканирования КСВ
	static uint_fast8_t swr_scan_stop = 0;			// флаг нажатия кнопки Stop во время сканирования
	static uint_fast8_t * y_vals;					// массив КСВ в виде отсчетов по оси Y графика
	window_t * win = get_win(WINDOW_SWR_SCANNER);
	uint_fast8_t averageFactor = 3;

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0, xmax = 0, ymax = 0;
		win->first_call = 0;
		button_t * bh = NULL;

		static const button_t buttons [] = {
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_SWR_SCANNER, 	NON_VISIBLE, INT32_MAX,  "btn_swr_start", "Start", },
			{ 0, 0, 86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_SWR_SCANNER, 	NON_VISIBLE, INT32_MAX,  "btn_swr_OK", 	  "OK", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_SWR_SCANNER, DISABLED,  0, NON_VISIBLE, "lbl_swr_bottom", "", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_SWR_SCANNER, DISABLED,  0, NON_VISIBLE, "lbl_swr_top", 	 "", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_SWR_SCANNER, DISABLED,  0, NON_VISIBLE, "lbl_swr_error",  "", FONT_MEDIUM, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		mid_w = col1_int + gr_w / 2;
		btn_swr_start = find_gui_element(TYPE_BUTTON, win, "btn_swr_start");
		btn_swr_start->x1 = mid_w - btn_swr_start->w - interval;
		btn_swr_start->y1 = row1_int + gr_h + col1_int;
		strcpy(btn_swr_start->text, "Start");
		btn_swr_start->visible = VISIBLE;

		btn_swr_OK = find_gui_element(TYPE_BUTTON, win, "btn_swr_OK");
		btn_swr_OK->x1 = mid_w + interval;
		btn_swr_OK->y1 = btn_swr_start->y1;
		btn_swr_OK->visible = VISIBLE;

		lbl_swr_error = find_gui_element(TYPE_LABEL, win, "lbl_swr_error");
		btn_Options = find_gui_element(TYPE_BUTTON, get_win(WINDOW_MAIN), "btn_Options");

		backup_freq = hamradio_get_freq_rx();
		if (hamradio_verify_freq_bands(backup_freq, & lim_bottom, & lim_top))
		{
			label_t * lbl_swr_bottom = find_gui_element(TYPE_LABEL, win, "lbl_swr_bottom");
			local_snprintf_P(lbl_swr_bottom->text, ARRAY_SIZE(lbl_swr_bottom->text), PSTR("%dk"), lim_bottom / 1000);
			lbl_swr_bottom->x = x0 - get_label_width(lbl_swr_bottom) / 2;
			lbl_swr_bottom->y = y0 + get_label_height(lbl_swr_bottom) * 2;
			lbl_swr_bottom->visible = VISIBLE;

			label_t * lbl_swr_top = find_gui_element(TYPE_LABEL, win, "lbl_swr_top");
			local_snprintf_P(lbl_swr_top->text, ARRAY_SIZE(lbl_swr_top->text), PSTR("%dk"), lim_top / 1000);
			lbl_swr_top->x = x1 - get_label_width(lbl_swr_bottom) / 2;
			lbl_swr_top->y = lbl_swr_bottom->y;
			lbl_swr_top->visible = VISIBLE;

			btn_swr_start->state = CANCELLED;
			swr_freq = lim_bottom;
			freq_step = (lim_top - lim_bottom) / (x1 - x0);
			current_freq_x = normalize(backup_freq / 1000, lim_bottom / 1000, lim_top / 1000, x1 - x0);
//			backup_power = hamradio_get_tx_power();
		}
		else
		{	// если текущая частота не входит ни в один из любительских диапазонов, вывод сообщения об ошибке
			local_snprintf_P(lbl_swr_error->text, ARRAY_SIZE(lbl_swr_error->text), PSTR("%dk not into HAM bands"), backup_freq / 1000);
			lbl_swr_error->x = mid_w - get_label_width(lbl_swr_error) / 2;
			lbl_swr_error->y = (row1_int + gr_h) / 2;
			lbl_swr_error->visible = VISIBLE;
			btn_swr_start->state = DISABLED;
		}

		xmax = col1_int + gr_w;
		ymax = btn_swr_OK->y1 + btn_swr_OK->h;

		calculate_window_position(win, WINDOW_POSITION_MANUAL, xmax, ymax);

		i = 0;
		y_vals = calloc(x1 - x0, sizeof(uint_fast8_t));
		swr_scan_done = 0;
		is_swr_scanning = 0;
		swr_scan_stop = 0;
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_swr_start = find_gui_element(TYPE_BUTTON, win, "btn_swr_start");
			button_t * btn_swr_OK = find_gui_element(TYPE_BUTTON, win, "btn_swr_OK");

			if (bh == btn_swr_start && ! strcmp(btn_swr_start->text, "Start"))
			{
				swr_scan_enable = 1;
			}
			else if (bh == btn_swr_start && ! strcmp(btn_swr_start->text, "Stop"))
			{
				swr_scan_stop = 1;
			}
			else if (bh == btn_swr_OK)
			{
				close_all_windows();
				free(y_vals);
				return;
			}
		}
		break;

	default:

		break;
	}

	if (swr_scan_enable)						// нажата кнопка Start
	{
		swr_scan_enable = 0;
		strcpy(btn_swr_start->text, "Stop");
		btn_Options->state = DISABLED;
		btn_swr_OK->state = DISABLED;
//		hamradio_set_tx_power(50);
		hamradio_set_tune(1);
		is_swr_scanning = 1;
		i = 0;
		swr_freq = lim_bottom;
		memset(y_vals, 0, x1 - x0);
	}

	if (is_swr_scanning)						// сканирование
	{
		hamradio_set_freq(swr_freq);
		swr_freq += freq_step;
		if (swr_freq >= lim_top || swr_scan_stop)
		{										// нажата кнопка Stop или сканируемая частота выше границы диапазона
			swr_scan_done = 1;
			is_swr_scanning = 0;
			swr_scan_stop = 0;
			strcpy(btn_swr_start->text, "Start");
			btn_Options->state = CANCELLED;
			btn_swr_OK->state = CANCELLED;
			hamradio_set_tune(0);
			hamradio_set_freq(backup_freq);
//			hamradio_set_tx_power(backup_power);
		}

		const uint_fast16_t swr_fullscale = (SWRMIN * 40 / 10) - SWRMIN;	// количество рисок в шкале индикатора
		y_vals [i] = normalize(get_swr(swr_fullscale), 0, swr_fullscale, y0 - y1);
		if (i)
			y_vals [i] = (y_vals [i - 1] * (averageFactor - 1) + y_vals [i]) / averageFactor;
		i++;
	}

	if (! win->first_call)
	{
		// отрисовка фона графика и разметки
		uint_fast16_t gr_x = win->x1 + x0, gr_y = win->y1 + y0;
		colpip_fillrect(fr, DIM_X, DIM_Y, win->x1 + col1_int, win->y1 + row1_int, gr_w, gr_h, COLORMAIN_BLACK);
		colmain_line(fr, DIM_X, DIM_Y, gr_x, gr_y, gr_x, win->y1 + y1, COLORMAIN_WHITE, 0);
		colmain_line(fr, DIM_X, DIM_Y, gr_x, gr_y, win->x1 + x1, gr_y, COLORMAIN_WHITE, 0);

		char buf [5];
		uint_fast8_t l = 1, row_step = round((y0 - y1) / 3);
		local_snprintf_P(buf, ARRAY_SIZE(buf), PSTR("%d"), l++);
		colpip_string3_tbg(fr, DIM_X, DIM_Y, gr_x - SMALLCHARW3 * 2, gr_y - SMALLCHARH3 / 2, buf, COLORMAIN_WHITE);
		for(int_fast16_t yy = y0 - row_step; yy > y1; yy -= row_step)
		{
			if (yy < 0)
				break;

			colmain_line(fr, DIM_X, DIM_Y, gr_x, win->y1 + yy, win->x1 + x1, win->y1 + yy, COLORMAIN_DARKGREEN, 0);
			local_snprintf_P(buf, ARRAY_SIZE(buf), PSTR("%d"), l++);
			colpip_string3_tbg(fr, DIM_X, DIM_Y, gr_x - SMALLCHARW3 * 2, win->y1 + yy - SMALLCHARH3 / 2, buf, COLORMAIN_WHITE);
		}

		if (lbl_swr_error->visible)				// фон сообщения об ошибке
		{
			colpip_fillrect(fr, DIM_X, DIM_Y, win->x1 + col1_int, win->y1 + lbl_swr_error->y - 5, gr_w, get_label_height(lbl_swr_error) + 5, COLORMAIN_RED);
		}
		else									// маркер текущей частоты
		{
			colmain_line(fr, DIM_X, DIM_Y, gr_x + current_freq_x, gr_y, gr_x + current_freq_x, win->y1 + y1, COLORMAIN_RED, 0);
		}

		if (is_swr_scanning || swr_scan_done)	// вывод графика во время сканирования и по завершении
		{
			for(uint_fast16_t j = 2; j <= i; j ++)
				colmain_line(fr, DIM_X, DIM_Y, gr_x + j - 2, gr_y - y_vals [j - 2], gr_x + j - 1, gr_y - y_vals [j - 1], COLORMAIN_YELLOW, 1);
		}
	}
#endif /* WITHTX */
}

// *********************************************************************************************************************************************************************

static void window_tx_process(void)
{
	window_t * win = get_win(WINDOW_TX_SETTINGS);
	uint_fast8_t update = 0;

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 3;
		button_t * bh = NULL;
		win->first_call = 0;
		update = 1;

		static const button_t buttons [] = {
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_TX_SETTINGS, 	NON_VISIBLE, INT32_MAX, "btn_tx_vox", 	 	 	"VOX|OFF", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_TX_SETTINGS, 	NON_VISIBLE, INT32_MAX, "btn_tx_vox_settings", 	"VOX|settings", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_TX_SETTINGS, 	NON_VISIBLE, INT32_MAX, "btn_tx_power", 	 	"TX power", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			button_t * bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
		}

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			window_t * winTX = get_win(WINDOW_TX_SETTINGS);
			window_t * winPower = get_win(WINDOW_TX_POWER);
			window_t * winVOX = get_win(WINDOW_TX_VOX_SETT);
			button_t * btn_tx_vox = find_gui_element(TYPE_BUTTON, winTX, "btn_tx_vox");
			button_t * btn_tx_power = find_gui_element(TYPE_BUTTON, winTX, "btn_tx_power");
			button_t * btn_tx_vox_settings = find_gui_element(TYPE_BUTTON, winTX, "btn_tx_vox_settings");

#if WITHPOWERTRIM
			if (bh == btn_tx_power)
			{
				open_window(winPower);
				return;
			}
#endif /* WITHPOWERTRIM */
#if WITHVOX
			if (bh == btn_tx_vox)
			{
				hamradio_set_gvoxenable(! hamradio_get_gvoxenable());
				update = 1;
			}
			else if (bh == btn_tx_vox_settings)
			{
				open_window(winVOX);
				return;
			}
#endif /* WITHVOX */
		}
		break;

	default:

		break;
	}

	if (update)
	{
#if ! WITHPOWERTRIM
		button_t * btn_tx_power= find_gui_element(TYPE_BUTTON, win, "btn_tx_power");
		btn_tx_power->state = DISABLED;
#endif /* ! WITHPOWERTRIM */

		button_t * btn_tx_vox= find_gui_element(TYPE_BUTTON, win, "btn_tx_vox"); 						// vox on/off
#if WITHVOX
		btn_tx_vox->is_locked = hamradio_get_gvoxenable() ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		local_snprintf_P(btn_tx_vox->text, ARRAY_SIZE(btn_tx_vox->text), PSTR("VOX|%s"), btn_tx_vox->is_locked ? "ON" : "OFF");
#else
		btn_tx_vox->state = DISABLED;
		local_snprintf_P(btn_tx_vox->text, ARRAY_SIZE(btn_tx_vox->text), PSTR("VOX"));
#endif /* WITHVOX */

		button_t * btn_tx_vox_settings = find_gui_element(TYPE_BUTTON, win, "btn_tx_vox_settings");		// vox settings
#if WITHVOX
		btn_tx_vox_settings->state = hamradio_get_gvoxenable() ? CANCELLED : DISABLED;
#else
		btn_tx_vox_settings->state = DISABLED;
#endif /* WITHVOX */
	}
}

// *********************************************************************************************************************************************************************

static void window_tx_vox_process(void)
{
#if WITHVOX
	window_t * win = get_win(WINDOW_TX_VOX_SETT);

	static slider_t * sl_vox_delay = NULL, * sl_vox_level = NULL, * sl_avox_level = NULL;
	static label_t * lbl_vox_delay = NULL, * lbl_vox_level = NULL, * lbl_avox_level = NULL;
	static uint_fast16_t delay_min, delay_max, level_min, level_max, alevel_min, alevel_max;
	slider_t * sl;

	if (win->first_call)
	{
		uint_fast8_t interval = 50, col1_int = 20;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0,  44, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_TX_VOX_SETT, NON_VISIBLE, INT32_MAX, "btn_tx_vox_OK", "OK", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const slider_t sliders [] = {
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_TX_VOX_SETT, "sl_vox_delay",  CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_TX_VOX_SETT, "sl_vox_level",  CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_TX_VOX_SETT, "sl_avox_level", CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
		};
		win->sh_count = ARRAY_SIZE(sliders);
		uint_fast16_t sliders_size = sizeof(sliders);
		win->sh_ptr = malloc(sliders_size);
		GUI_MEM_ASSERT(win->sh_ptr);
		memcpy(win->sh_ptr, sliders, sliders_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_vox_delay",    	 "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_vox_level",    	 "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_avox_level",   	 "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_vox_delay_min",  "", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_vox_delay_max",  "", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_vox_level_min",  "", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_vox_level_max",  "", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_avox_level_min", "", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_VOX_SETT, DISABLED,  0, NON_VISIBLE, "lbl_avox_level_max", "", FONT_SMALL, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		hamradio_get_vox_delay_limits(& delay_min, & delay_max);
		hamradio_get_vox_level_limits(& level_min, & level_max);
		hamradio_get_antivox_delay_limits(& alevel_min, & alevel_max);

		sl_vox_delay = find_gui_element(TYPE_SLIDER, win, "sl_vox_delay");
		sl_vox_level = find_gui_element(TYPE_SLIDER, win, "sl_vox_level");
		sl_avox_level = find_gui_element(TYPE_SLIDER, win, "sl_avox_level");
		lbl_vox_delay = find_gui_element(TYPE_LABEL, win, "lbl_vox_delay");
		lbl_vox_level = find_gui_element(TYPE_LABEL, win, "lbl_vox_level");
		lbl_avox_level = find_gui_element(TYPE_LABEL, win, "lbl_avox_level");

		ldiv_t d = ldiv(hamradio_get_vox_delay(), 100);
		lbl_vox_delay->x = col1_int;
		lbl_vox_delay->y = interval;
		lbl_vox_delay->visible = VISIBLE;
		local_snprintf_P(lbl_vox_delay->text, ARRAY_SIZE(lbl_vox_delay->text), PSTR("Delay: %d.%d"), d.quot, d.rem / 10);

		lbl_vox_level->x = lbl_vox_delay->x;
		lbl_vox_level->y = lbl_vox_delay->y + interval;
		lbl_vox_level->visible = VISIBLE;
		local_snprintf_P(lbl_vox_level->text, ARRAY_SIZE(lbl_vox_level->text), PSTR("Level: %3d"), hamradio_get_vox_level());

		lbl_avox_level->x = lbl_vox_level->x;
		lbl_avox_level->y = lbl_vox_level->y + interval;
		lbl_avox_level->visible = VISIBLE;
		local_snprintf_P(lbl_avox_level->text, ARRAY_SIZE(lbl_avox_level->text), PSTR("AVOX : %3d"), hamradio_get_antivox_level());

		sl_vox_delay->x = lbl_vox_delay->x + interval * 2 + interval / 2;
		sl_vox_delay->y = lbl_vox_delay->y;
		sl_vox_delay->visible = VISIBLE;
		sl_vox_delay->size = 300;
		sl_vox_delay->step = 3;
		sl_vox_delay->value = normalize(hamradio_get_vox_delay(), delay_min, delay_max, 100);

		sl_vox_level->x = sl_vox_delay->x;
		sl_vox_level->y = lbl_vox_level->y;
		sl_vox_level->visible = VISIBLE;
		sl_vox_level->size = 300;
		sl_vox_level->step = 3;
		sl_vox_level->value = normalize(hamradio_get_vox_level(), level_min, level_max, 100);

		sl_avox_level->x = sl_vox_delay->x;
		sl_avox_level->y = lbl_avox_level->y;
		sl_avox_level->visible = VISIBLE;
		sl_avox_level->size = 300;
		sl_avox_level->step = 3;
		sl_avox_level->value = normalize(hamradio_get_antivox_level(), alevel_min, alevel_max, 100);

		button_t * bh = find_gui_element(TYPE_BUTTON, win, "btn_tx_vox_OK");
		bh->x1 = (sl_vox_delay->x + sl_vox_delay->size + col1_int * 2) / 2 - (bh->w / 2);
		bh->y1 = lbl_avox_level->y + interval;
		bh->visible = VISIBLE;

		d = ldiv(delay_min, 100);
		label_t * lbl_vox_delay_min = find_gui_element(TYPE_LABEL, win, "lbl_vox_delay_min");
		local_snprintf_P(lbl_vox_delay_min->text, ARRAY_SIZE(lbl_vox_delay_min->text), PSTR("%d.%d sec"), d.quot, d.rem / 10);
		lbl_vox_delay_min->x = sl_vox_delay->x - get_label_width(lbl_vox_delay_min) / 2;
		lbl_vox_delay_min->y = sl_vox_delay->y + get_label_height(lbl_vox_delay_min) * 3;
		lbl_vox_delay_min->visible = VISIBLE;

		d = ldiv(delay_max, 100);
		label_t * lbl_vox_delay_max = find_gui_element(TYPE_LABEL, win, "lbl_vox_delay_max");
		local_snprintf_P(lbl_vox_delay_max->text, ARRAY_SIZE(lbl_vox_delay_max->text), PSTR("%d.%d sec"), d.quot, d.rem / 10);
		lbl_vox_delay_max->x = sl_vox_delay->x + sl_vox_delay->size - get_label_width(lbl_vox_delay_max) / 2;
		lbl_vox_delay_max->y = sl_vox_delay->y + get_label_height(lbl_vox_delay_max) * 3;
		lbl_vox_delay_max->visible = VISIBLE;

		label_t * lbl_vox_level_min = find_gui_element(TYPE_LABEL, win, "lbl_vox_level_min");
		local_snprintf_P(lbl_vox_level_min->text, ARRAY_SIZE(lbl_vox_level_min->text), PSTR("%d"), level_min);
		lbl_vox_level_min->x = sl_vox_level->x - get_label_width(lbl_vox_level_min) / 2;
		lbl_vox_level_min->y = sl_vox_level->y + get_label_height(lbl_vox_level_min) * 3;
		lbl_vox_level_min->visible = VISIBLE;

		label_t * lbl_vox_level_max = find_gui_element(TYPE_LABEL, win, "lbl_vox_level_max");
		local_snprintf_P(lbl_vox_level_max->text, ARRAY_SIZE(lbl_vox_level_max->text), PSTR("%d"), level_max);
		lbl_vox_level_max->x = sl_vox_level->x + sl_vox_level->size - get_label_width(lbl_vox_level_max) / 2;
		lbl_vox_level_max->y = sl_vox_level->y + get_label_height(lbl_vox_level_max) * 3;
		lbl_vox_level_max->visible = VISIBLE;

		label_t * lbl_avox_level_min = find_gui_element(TYPE_LABEL, win, "lbl_avox_level_min");
		local_snprintf_P(lbl_avox_level_min->text, ARRAY_SIZE(lbl_avox_level_min->text), PSTR("%d"), alevel_min);
		lbl_avox_level_min->x = sl_avox_level->x - get_label_width(lbl_avox_level_min) / 2;
		lbl_avox_level_min->y = sl_avox_level->y + get_label_height(lbl_avox_level_min) * 3;
		lbl_avox_level_min->visible = VISIBLE;

		label_t * lbl_avox_level_max = find_gui_element(TYPE_LABEL, win, "lbl_avox_level_max");
		local_snprintf_P(lbl_avox_level_max->text, ARRAY_SIZE(lbl_avox_level_max->text), PSTR("%d"), alevel_max);
		lbl_avox_level_max->x = sl_avox_level->x + sl_vox_level->size - get_label_width(lbl_avox_level_max) / 2;
		lbl_avox_level_max->y = sl_avox_level->y + get_label_height(lbl_avox_level_max) * 3;
		lbl_avox_level_max->visible = VISIBLE;

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;

			button_t * btn_tx_vox_OK = find_gui_element(TYPE_BUTTON, win, "btn_tx_vox_OK");
			if (bh == btn_tx_vox_OK)
			{
				close_all_windows();
			}
		}
		else if (IS_SLIDER_MOVE)
		{
			slider_t * sl = (slider_t *) ptr;

			if (sl == sl_vox_delay)
			{
				uint_fast16_t delay = delay_min + normalize(sl->value, 0, 100, delay_max - delay_min);
				ldiv_t d = ldiv(delay, 100);
				local_snprintf_P(lbl_vox_delay->text, ARRAY_SIZE(lbl_vox_delay->text), PSTR("Delay: %d.%d"), d.quot, d.rem / 10);
				hamradio_set_vox_delay(delay);
			}
			else if (sl == sl_vox_level)
			{
				uint_fast16_t level = level_min + normalize(sl->value, 0, 100, level_max - level_min);
				local_snprintf_P(lbl_vox_level->text, ARRAY_SIZE(lbl_vox_level->text), PSTR("Level: %3d"), level);
				hamradio_set_vox_level(level);
			}
			else if (sl == sl_avox_level)
			{
				uint_fast16_t alevel = alevel_min + normalize(sl->value, 0, 100, alevel_max - alevel_min);
				local_snprintf_P(lbl_avox_level->text, ARRAY_SIZE(lbl_avox_level->text), PSTR("AVOX : %3d"), alevel);
				hamradio_set_antivox_level(alevel);
			}
		}
		break;

	default:

		break;
	}
#endif /* WITHVOX */
}

// *********************************************************************************************************************************************************************

static void window_tx_power_process(void)
{
#if WITHPOWERTRIM
	window_t * win = get_win(WINDOW_TX_POWER);

	static slider_t * sl_pwr_level = NULL, * sl_pwr_tuner_level = NULL;
	static label_t * lbl_tx_power = NULL, * lbl_tune_power = NULL;
	static uint_fast16_t power_min, power_max;
	slider_t * sl;

	if (win->first_call)
	{
		uint_fast8_t interval = 50, col1_int = 20;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0,  44, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_TX_POWER,  NON_VISIBLE, INT32_MAX, "btn_tx_pwr_OK", "OK", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const slider_t sliders [] = {
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_TX_POWER, "sl_pwr_level",   	   CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_TX_POWER, "sl_pwr_tuner_level", CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
		};
		win->sh_count = ARRAY_SIZE(sliders);
		uint_fast16_t sliders_size = sizeof(sliders);
		win->sh_ptr = malloc(sliders_size);
		GUI_MEM_ASSERT(win->sh_ptr);
		memcpy(win->sh_ptr, sliders, sliders_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_TX_POWER, DISABLED,  0, NON_VISIBLE, "lbl_tx_power",   "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_TX_POWER, DISABLED,  0, NON_VISIBLE, "lbl_tune_power", "", FONT_MEDIUM, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		sl_pwr_level = find_gui_element(TYPE_SLIDER, win, "sl_pwr_level");
		sl_pwr_tuner_level = find_gui_element(TYPE_SLIDER, win, "sl_pwr_tuner_level");

		lbl_tx_power = find_gui_element(TYPE_LABEL, win, "lbl_tx_power");
		lbl_tune_power = find_gui_element(TYPE_LABEL, win, "lbl_tune_power");

		hamradio_get_tx_power_limits(& power_min, & power_max);
		uint_fast8_t power = hamradio_get_tx_power();
		uint_fast8_t tune_power = hamradio_get_tx_tune_power();

		lbl_tx_power->x = col1_int;
		lbl_tx_power->y = interval;
		lbl_tx_power->visible = VISIBLE;
		local_snprintf_P(lbl_tx_power->text, ARRAY_SIZE(lbl_tx_power->text), PSTR("TX power  : %3d"), power);

		lbl_tune_power->x = lbl_tx_power->x;
		lbl_tune_power->y = lbl_tx_power->y + interval;
		lbl_tune_power->visible = VISIBLE;
		local_snprintf_P(lbl_tune_power->text, ARRAY_SIZE(lbl_tune_power->text), PSTR("Tune power: %3d"), tune_power);

		sl_pwr_level->x = lbl_tx_power->x + interval * 3 + interval / 2;
		sl_pwr_level->y = lbl_tx_power->y;
		sl_pwr_level->visible = VISIBLE;
		sl_pwr_level->size = 300;
		sl_pwr_level->step = 3;
		sl_pwr_level->value = normalize(power, power_min, power_max, 100);

		sl_pwr_tuner_level->x = sl_pwr_level->x;
		sl_pwr_tuner_level->y = lbl_tune_power->y;
		sl_pwr_tuner_level->visible = VISIBLE;
		sl_pwr_tuner_level->size = 300;
		sl_pwr_tuner_level->step = 3;
		sl_pwr_tuner_level->value = normalize(tune_power, power_min, power_max, 100);

		button_t * bh = find_gui_element(TYPE_BUTTON, win, "btn_tx_pwr_OK");
		bh->x1 = (sl_pwr_level->x + sl_pwr_level->size + col1_int * 2) / 2 - (bh->w / 2);
		bh->y1 = lbl_tune_power->y + interval;
		bh->visible = VISIBLE;

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_tx_pwr_OK = find_gui_element(TYPE_BUTTON, win, "btn_tx_pwr_OK");

			if (bh == btn_tx_pwr_OK)
			{
				close_all_windows();
			}
		}
		else if (IS_SLIDER_MOVE)
		{
			slider_t * sl = (slider_t *) ptr;

			if (sl == sl_pwr_level)
			{
				uint_fast8_t power = power_min + normalize(sl->value, 0, 100, power_max - power_min);
				local_snprintf_P(lbl_tx_power->text, ARRAY_SIZE(lbl_tx_power->text), PSTR("TX power  : %3d"),power);
				hamradio_set_tx_power(power);
			}
			else if (sl == sl_pwr_tuner_level)
			{
				uint_fast8_t power = power_min + normalize(sl->value, 0, 100, power_max - power_min);
				local_snprintf_P(lbl_tune_power->text, ARRAY_SIZE(lbl_tune_power->text), PSTR("Tune power: %3d"),power);
				hamradio_set_tx_tune_power(power);
			}
		}
		break;

	default:

		break;
	}
#endif /* WITHPOWERTRIM */
}

// *********************************************************************************************************************************************************************

static void window_audiosettings_process(void)
{
	window_t * win = get_win(WINDOW_AUDIOSETTINGS);
	uint_fast8_t update = 0;

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 4;
		button_t * bh = NULL;
		win->first_call = 0;
		update = 1;

		static const button_t buttons [] = {
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AUDIOSETTINGS, NON_VISIBLE, INT32_MAX, "btn_reverb", 		 "Reverb|OFF", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AUDIOSETTINGS, NON_VISIBLE, INT32_MAX, "btn_mic_eq", 		 "MIC EQ|OFF", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AUDIOSETTINGS, NON_VISIBLE, INT32_MAX, "btn_mic_profiles", 	 "MIC|profiles", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AUDIOSETTINGS, NON_VISIBLE, INT32_MAX, "btn_monitor", 		 "Monitor|disabled", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AUDIOSETTINGS, NON_VISIBLE, INT32_MAX, "btn_reverb_settings", "Reverb|settings", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AUDIOSETTINGS, NON_VISIBLE, INT32_MAX, "btn_mic_eq_settings", "MIC EQ|settings", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AUDIOSETTINGS, NON_VISIBLE, INT32_MAX, "btn_mic_settings", 	 "MIC|settings", },

		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			button_t * bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
		}

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			window_t * winAP = get_win(WINDOW_AUDIOSETTINGS);
			window_t * winEQ = get_win(WINDOW_AP_MIC_EQ);
			window_t * winRS = get_win(WINDOW_AP_REVERB_SETT);
			window_t * winMIC = get_win(WINDOW_AP_MIC_SETT);
			window_t * winMICpr = get_win(WINDOW_AP_MIC_PROF);
			button_t * btn_reverb = find_gui_element(TYPE_BUTTON, winAP, "btn_reverb");						// reverb on/off
			button_t * btn_reverb_settings = find_gui_element(TYPE_BUTTON, winAP, "btn_reverb_settings");	// reverb settings
			button_t * btn_monitor = find_gui_element(TYPE_BUTTON, winAP, "btn_monitor");					// monitor on/off
			button_t * btn_mic_eq = find_gui_element(TYPE_BUTTON, winAP, "btn_mic_eq");						// MIC EQ on/off
			button_t * btn_mic_eq_settings = find_gui_element(TYPE_BUTTON, winAP, "btn_mic_eq_settings");	// MIC EQ settingss
			button_t * btn_mic_settings = find_gui_element(TYPE_BUTTON, winAP, "btn_mic_settings");			// mic settings
			button_t * btn_mic_profiles = find_gui_element(TYPE_BUTTON, winAP, "btn_mic_profiles");			// mic profiles

			if (bh == btn_reverb_settings)
			{
				open_window(winRS);
			}
			else if (bh == btn_monitor)
			{
				hamradio_set_gmoniflag(! hamradio_get_gmoniflag());
				update = 1;
			}
			else if (bh == btn_mic_eq)
			{
				hamradio_set_gmikeequalizer(! hamradio_get_gmikeequalizer());
				update = 1;
			}
			else if (bh == btn_mic_eq_settings)
			{
				open_window(winEQ);
				return;
			}
			else if (bh == btn_mic_settings)
			{
				open_window(winMIC);
				return;
			}
			else if (bh == btn_mic_profiles)
			{
				open_window(winMICpr);
				return;
			}
#if WITHREVERB
			else if (bh == btn_reverb)
			{
				hamradio_set_greverb(! hamradio_get_greverb());
				update = 1;
			}
#endif /* WITHREVERB */
		}
		break;

	default:

		break;
	}

	if (update)
	{
		button_t * bh = find_gui_element(TYPE_BUTTON, win, "btn_reverb"); 			// reverb on/off
#if WITHREVERB
		bh->is_locked = hamradio_get_greverb() ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("Reverb|%s"), hamradio_get_greverb() ? "ON" : "OFF");
#else
		bh->state = DISABLED;
		local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("Reverb|OFF"));
#endif /* WITHREVERB */

		bh = find_gui_element(TYPE_BUTTON, win, "btn_reverb_settings");				// reverb settings
#if WITHREVERB
		bh->state = hamradio_get_greverb() ? CANCELLED : DISABLED;
#else
		bh->state = DISABLED;
#endif /* WITHREVERB */

		bh = find_gui_element(TYPE_BUTTON, win, "btn_monitor");						// monitor on/off
		bh->is_locked = hamradio_get_gmoniflag() ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("Monitor|%s"), bh->is_locked ? "enabled" : "disabled");

		bh = find_gui_element(TYPE_BUTTON, win, "btn_mic_eq");						// MIC EQ on/off
		bh->is_locked = hamradio_get_gmikeequalizer() ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("MIC EQ|%s"), bh->is_locked ? "ON" : "OFF");

		bh = find_gui_element(TYPE_BUTTON, win, "btn_mic_eq_settings");				// MIC EQ settings
		bh->state = hamradio_get_gmikeequalizer() ? CANCELLED : DISABLED;
	}
}

// *********************************************************************************************************************************************************************

static void window_ap_reverb_process(void)
{
#if WITHREVERB
	window_t * win = get_win(WINDOW_AP_REVERB_SETT);

	static label_t * lbl_reverbDelay = NULL, * lbl_reverbLoss = NULL;
	static slider_t * sl_reverbDelay = NULL, * sl_reverbLoss = NULL;
	static uint_fast16_t delay_min, delay_max, loss_min, loss_max;

	if (win->first_call)
	{
		uint_fast8_t interval = 60, col1_int = 20;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0,  40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AP_REVERB_SETT,	NON_VISIBLE, INT32_MAX, "btn_REVs_ok", "OK", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_AP_REVERB_SETT,  DISABLED,  0, NON_VISIBLE, "lbl_reverbDelay",		"", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_REVERB_SETT,  DISABLED,  0, NON_VISIBLE, "lbl_reverbLoss", 		"", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_REVERB_SETT,  DISABLED,  0, NON_VISIBLE, "lbl_reverbDelay_min", 	"", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_REVERB_SETT,  DISABLED,  0, NON_VISIBLE, "lbl_reverbDelay_max", 	"", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_REVERB_SETT,  DISABLED,  0, NON_VISIBLE, "lbl_reverbLoss_min", 	"", FONT_SMALL, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_REVERB_SETT,  DISABLED,  0, NON_VISIBLE, "lbl_reverbLoss_max", 	"", FONT_SMALL, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		slider_t sliders [] = {
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_AP_REVERB_SETT, 	"reverbDelay", CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_AP_REVERB_SETT, 	"reverbLoss",  CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
		};
		win->sh_count = ARRAY_SIZE(sliders);
		uint_fast16_t sliders_size = sizeof(sliders);
		win->sh_ptr = malloc(sliders_size);
		GUI_MEM_ASSERT(win->sh_ptr);
		memcpy(win->sh_ptr, sliders, sliders_size);

		label_t * lbl_reverbDelay_min = find_gui_element(TYPE_LABEL, win, "lbl_reverbDelay_min");
		label_t * lbl_reverbDelay_max = find_gui_element(TYPE_LABEL, win, "lbl_reverbDelay_max");
		label_t * lbl_reverbLoss_min = find_gui_element(TYPE_LABEL, win, "lbl_reverbLoss_min");
		label_t * lbl_reverbLoss_max = find_gui_element(TYPE_LABEL, win, "lbl_reverbLoss_max");
		lbl_reverbDelay = find_gui_element(TYPE_LABEL, win, "lbl_reverbDelay");
		lbl_reverbLoss = find_gui_element(TYPE_LABEL, win, "lbl_reverbLoss");
		sl_reverbDelay = find_gui_element(TYPE_SLIDER, win, "reverbDelay");
		sl_reverbLoss = find_gui_element(TYPE_SLIDER, win, "reverbLoss");

		hamradio_get_reverb_delay_limits(& delay_min, & delay_max);
		hamradio_get_reverb_loss_limits(& loss_min, & loss_max);

		lbl_reverbDelay->x = col1_int;
		lbl_reverbDelay->y = interval;
		lbl_reverbDelay->visible = VISIBLE;
		local_snprintf_P(lbl_reverbDelay->text, ARRAY_SIZE(lbl_reverbDelay->text), PSTR("Delay: %3d ms"), hamradio_get_reverb_delay());

		lbl_reverbLoss->x = lbl_reverbDelay->x;
		lbl_reverbLoss->y = lbl_reverbDelay->y + interval;
		lbl_reverbLoss->visible = VISIBLE;
		local_snprintf_P(lbl_reverbLoss->text, ARRAY_SIZE(lbl_reverbLoss->text), PSTR("Loss :  %2d dB"), hamradio_get_reverb_loss());

		sl_reverbDelay->x = lbl_reverbDelay->x + interval * 3;
		sl_reverbDelay->y = lbl_reverbDelay->y;
		sl_reverbDelay->visible = VISIBLE;
		sl_reverbDelay->size = 300;
		sl_reverbDelay->step = 3;
		sl_reverbDelay->value = normalize(hamradio_get_reverb_delay(), delay_min, delay_max, 100);

		local_snprintf_P(lbl_reverbDelay_min->text, ARRAY_SIZE(lbl_reverbDelay_min->text), PSTR("%d ms"), delay_min);
		lbl_reverbDelay_min->x = sl_reverbDelay->x - get_label_width(lbl_reverbDelay_min) / 2;
		lbl_reverbDelay_min->y = sl_reverbDelay->y + get_label_height(lbl_reverbDelay_min) * 3;
		lbl_reverbDelay_min->visible = VISIBLE;

		local_snprintf_P(lbl_reverbDelay_max->text, ARRAY_SIZE(lbl_reverbDelay_max->text), PSTR("%d ms"), delay_max);
		lbl_reverbDelay_max->x = sl_reverbDelay->x + sl_reverbDelay->size - get_label_width(lbl_reverbDelay_max) / 2;
		lbl_reverbDelay_max->y = sl_reverbDelay->y + get_label_height(lbl_reverbDelay_max) * 3;
		lbl_reverbDelay_max->visible = VISIBLE;

		sl_reverbLoss->x = lbl_reverbLoss->x + interval * 3;
		sl_reverbLoss->y = lbl_reverbLoss->y;
		sl_reverbLoss->visible = VISIBLE;
		sl_reverbLoss->size = 300;
		sl_reverbLoss->step = 3;
		sl_reverbLoss->value = normalize(hamradio_get_reverb_loss(), loss_min, loss_max, 100);

		local_snprintf_P(lbl_reverbLoss_min->text, ARRAY_SIZE(lbl_reverbLoss_min->text), PSTR("%d dB"), loss_min);
		lbl_reverbLoss_min->x = sl_reverbLoss->x - get_label_width(lbl_reverbLoss_min) / 2;
		lbl_reverbLoss_min->y = sl_reverbLoss->y + get_label_height(lbl_reverbLoss_min) * 3;
		lbl_reverbLoss_min->visible = VISIBLE;

		local_snprintf_P(lbl_reverbLoss_max->text, ARRAY_SIZE(lbl_reverbLoss_max->text), PSTR("%d dB"), loss_max);
		lbl_reverbLoss_max->x = sl_reverbLoss->x + sl_reverbLoss->size - get_label_width(lbl_reverbLoss_max) / 2;
		lbl_reverbLoss_max->y = sl_reverbLoss->y + get_label_height(lbl_reverbLoss_max) * 3;
		lbl_reverbLoss_max->visible = VISIBLE;

		button_t * bh = find_gui_element(TYPE_BUTTON, win, "btn_REVs_ok");
		bh->x1 = sl_reverbLoss->x + sl_reverbLoss->size + interval / 2;
		bh->y1 = lbl_reverbLoss->y;
		bh->visible = VISIBLE;

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			close_window(OPEN_PARENT_WINDOW); // единственная кнопка
			return;
		}
		else if (IS_SLIDER_MOVE)
		{
			slider_t * sh = (slider_t *) ptr;

			if (sh == sl_reverbDelay)
			{
				uint_fast16_t delay = delay_min + normalize(sl_reverbDelay->value, 0, 100, delay_max - delay_min);
				local_snprintf_P(lbl_reverbDelay->text, ARRAY_SIZE(lbl_reverbDelay->text), PSTR("Delay: %3d ms"), delay);
				hamradio_set_reverb_delay(delay);
			}
			else if (sh == sl_reverbLoss)
			{
				uint_fast16_t loss = loss_min + normalize(sl_reverbLoss->value, 0, 100, loss_max - loss_min);
				local_snprintf_P(lbl_reverbLoss->text, ARRAY_SIZE(lbl_reverbLoss->text), PSTR("Loss :  %2d dB"), loss);
				hamradio_set_reverb_loss(loss);
			}
		}
		break;

	default:

		break;
	}
#endif /* WITHREVERB */
}

// *********************************************************************************************************************************************************************

static void window_ap_mic_eq_process(void)
{
	PACKEDCOLORMAIN_T * const fr = colmain_fb_draw();
	window_t * win = get_win(WINDOW_AP_MIC_EQ);

	label_t * lbl = NULL;
	static uint_fast8_t eq_limit, eq_base = 0;
	char buf [TEXT_ARRAY_SIZE];
	static int_fast16_t mid_y = 0;
	static uint_fast8_t id = 0;
	static button_t * btn_EQ_ok;

	if (win->first_call)
	{
		uint_fast16_t x, y, mid_w;
		uint_fast8_t interval = 70, col1_int = 70, row1_int = window_title_height + 20;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0,  40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AP_MIC_EQ, 	NON_VISIBLE, INT32_MAX, "btn_EQ_ok", "OK", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq0.08_val", 	"", FONT_LARGE, COLORMAIN_YELLOW, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq0.23_val", 	"", FONT_LARGE, COLORMAIN_YELLOW, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq0.65_val",  "", FONT_LARGE, COLORMAIN_YELLOW, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq1.8_val",   "", FONT_LARGE, COLORMAIN_YELLOW, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq5.3_val",   "", FONT_LARGE, COLORMAIN_YELLOW, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq0.08_name", "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq0.23_name", "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq0.65_name", "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq1.8_name",  "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_EQ, DISABLED,  0, NON_VISIBLE, "lbl_eq5.3_name",  "", FONT_MEDIUM, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		static const slider_t sliders [] = {
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_VERTICAL, WINDOW_AP_MIC_EQ, "eq0.08", CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, 0, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_VERTICAL, WINDOW_AP_MIC_EQ, "eq0.23", CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, 1, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_VERTICAL, WINDOW_AP_MIC_EQ, "eq0.65", CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, 2, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_VERTICAL, WINDOW_AP_MIC_EQ, "eq1.8",  CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, 3, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_VERTICAL, WINDOW_AP_MIC_EQ, "eq5.3",  CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, 4, },
		};
		win->sh_count = ARRAY_SIZE(sliders);
		uint_fast16_t sliders_size = sizeof(sliders);
		win->sh_ptr = malloc(sliders_size);
		GUI_MEM_ASSERT(win->sh_ptr);
		memcpy(win->sh_ptr, sliders, sliders_size);

		eq_base = hamradio_getequalizerbase();
		eq_limit = abs(eq_base) * 2;

		x = col1_int;
		y = row1_int;
		slider_t * sl = NULL;

		for (id = 0; id < win->sh_count; id++)
		{
			sl = & win->sh_ptr [id];

			sl->x = x;
			sl->size = 200;
			sl->step = 2;
			sl->value = normalize(hamradio_get_gmikeequalizerparams(id), eq_limit, 0, 100);
			sl->visible = VISIBLE;

			mid_w = sl->x + sliders_width / 2;		// центр шкалы слайдера по x

			local_snprintf_P(buf, ARRAY_SIZE(buf), PSTR("lbl_%s_name"), sl->name);
			lbl = find_gui_element(TYPE_LABEL, win, buf);
			local_snprintf_P(lbl->text, ARRAY_SIZE(lbl->text), PSTR("%sk"), strchr(sl->name, 'q') + 1);
			lbl->x = mid_w - get_label_width(lbl) / 2;
			lbl->y = y;
			lbl->visible = VISIBLE;

			y = lbl->y + get_label_height(lbl) * 2;

			local_snprintf_P(buf, ARRAY_SIZE(buf), PSTR("lbl_%s_val"), sl->name);
			lbl = find_gui_element(TYPE_LABEL, win, buf);
			local_snprintf_P(lbl->text, ARRAY_SIZE(lbl->text), PSTR("%d"), hamradio_get_gmikeequalizerparams(id) + eq_base);
			lbl->x = mid_w - get_label_width(lbl) / 2;
			lbl->y = y;
			lbl->visible = VISIBLE;

			sl->y = lbl->y + get_label_height(lbl) * 2 + 10;

			x = x + interval;
			y = row1_int;
		}

		btn_EQ_ok = find_gui_element(TYPE_BUTTON, win, "btn_EQ_ok");
		btn_EQ_ok->x1 = sl->x + sliders_width + btn_EQ_ok->w;
		btn_EQ_ok->y1 = sl->y + sl->size - btn_EQ_ok->h;
		btn_EQ_ok->visible = VISIBLE;

		calculate_window_position(win, WINDOW_POSITION_AUTO);
		mid_y = win->y1 + sl->y + sl->size / 2;						//todo: абсолютные координаты! переделать
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			close_all_windows();
			return;
		}
		else if (IS_SLIDER_MOVE)
		{
			slider_t * sh = (slider_t *) ptr;
			uint_fast8_t id = sh->index;

			hamradio_set_gmikeequalizerparams(id, normalize(sh->value, 100, 0, eq_limit));

			local_snprintf_P(buf, ARRAY_SIZE(buf), PSTR("lbl_%s_val"), sh->name);
			lbl = find_gui_element(TYPE_LABEL, win, buf);
			local_snprintf_P(lbl->text, ARRAY_SIZE(lbl->text), PSTR("%d"), hamradio_get_gmikeequalizerparams(id) + eq_base);
			lbl->x = sh->x + sliders_width / 2 - get_label_width(lbl) / 2;
		}
		break;

	default:

		break;
	}

	// разметка шкал
	for (uint_fast16_t i = 0; i <= abs(eq_base); i += 3)
	{
		uint_fast16_t yy = normalize(i, 0, abs(eq_base), 100);
		colmain_line(fr, DIM_X, DIM_Y, win->x1 + 50, mid_y + yy, win->x1 + win->w - (btn_EQ_ok->w << 1), mid_y + yy, GUI_SLIDERLAYOUTCOLOR, 0);
		local_snprintf_P(buf, ARRAY_SIZE(buf), i == 0 ? PSTR("%d") : PSTR("-%d"), i);
		colpip_string2_tbg(fr, DIM_X, DIM_Y, win->x1 + 50 - strwidth2(buf) - 5, mid_y + yy - SMALLCHARH2 / 2, buf, COLORMAIN_WHITE);

		if (i == 0)
			continue;
		colmain_line(fr, DIM_X, DIM_Y, win->x1 + 50, mid_y - yy, win->x1 + win->w - (btn_EQ_ok->w << 1), mid_y - yy, GUI_SLIDERLAYOUTCOLOR, 0);
		local_snprintf_P(buf, ARRAY_SIZE(buf), PSTR("%d"), i);
		colpip_string2_tbg(fr, DIM_X, DIM_Y, win->x1 + 50 - strwidth2(buf) - 5, mid_y - yy - SMALLCHARH2 / 2, buf, COLORMAIN_WHITE);
	}
}

// *********************************************************************************************************************************************************************

static void window_ap_mic_process(void)
{
	window_t * win = get_win(WINDOW_AP_MIC_SETT);

	static slider_t * sl_micLevel = NULL, * sl_micClip = NULL, * sl_micAGC = NULL;
	static label_t * lbl_micLevel = NULL, * lbl_micClip = NULL, * lbl_micAGC = NULL;
	static uint_fast16_t level_min, level_max, clip_min, clip_max, agc_min, agc_max;

	if (win->first_call)
	{
		uint_fast8_t interval = 50, col1_int = 20;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0,  86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AP_MIC_SETT, NON_VISIBLE, INT32_MAX, "btn_mic_agc",   "AGC|OFF", },
			{ 0, 0,  86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AP_MIC_SETT, NON_VISIBLE, INT32_MAX, "btn_mic_boost", "Boost|OFF", },
			{ 0, 0,  86, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_AP_MIC_SETT, NON_VISIBLE, INT32_MAX, "btn_mic_OK",    "OK", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micLevel", 	  "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micClip",  	  "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micAGC",   	  "", FONT_MEDIUM, COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micLevel_min", "", FONT_SMALL,  COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micLevel_max", "", FONT_SMALL,  COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micClip_min",  "", FONT_SMALL,  COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micClip_max",  "", FONT_SMALL,  COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micAGC_min",   "", FONT_SMALL,  COLORMAIN_WHITE, },
			{ 0, 0,	WINDOW_AP_MIC_SETT, DISABLED, 0, NON_VISIBLE, "lbl_micAGC_max",   "", FONT_SMALL,  COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		static const slider_t sliders [] = {
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_AP_MIC_SETT, "sl_micLevel", CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_AP_MIC_SETT, "sl_micClip",  CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
			{ 0, 0, 0, 0, 0, 0, ORIENTATION_HORIZONTAL, WINDOW_AP_MIC_SETT, "sl_micAGC",   CANCELLED, NON_VISIBLE, 0, 50, 255, 0, 0, },
		};
		win->sh_count = ARRAY_SIZE(sliders);
		uint_fast16_t sliders_size = sizeof(sliders);
		win->sh_ptr = malloc(sliders_size);
		GUI_MEM_ASSERT(win->sh_ptr);
		memcpy(win->sh_ptr, sliders, sliders_size);

		hamradio_get_mic_level_limits(& level_min, & level_max);
		hamradio_get_mic_clip_limits(& clip_min, & clip_max);
		hamradio_get_mic_agc_limits(& agc_min, & agc_max);

		sl_micLevel = find_gui_element(TYPE_SLIDER, win, "sl_micLevel");
		sl_micClip = find_gui_element(TYPE_SLIDER, win, "sl_micClip");
		sl_micAGC = find_gui_element(TYPE_SLIDER, win, "sl_micAGC");
		lbl_micLevel = find_gui_element(TYPE_LABEL, win, "lbl_micLevel");
		lbl_micClip = find_gui_element(TYPE_LABEL, win, "lbl_micClip");
		lbl_micAGC = find_gui_element(TYPE_LABEL, win, "lbl_micAGC");

		lbl_micLevel->x = col1_int;
		lbl_micLevel->y = interval;
		lbl_micLevel->visible = VISIBLE;
		local_snprintf_P(lbl_micLevel->text, ARRAY_SIZE(lbl_micLevel->text), PSTR("Level: %3d"), hamradio_get_mik1level());

		lbl_micClip->x = lbl_micLevel->x;
		lbl_micClip->y = lbl_micLevel->y + interval;
		lbl_micClip->visible = VISIBLE;
		local_snprintf_P(lbl_micClip->text, ARRAY_SIZE(lbl_micClip->text), PSTR("Clip : %3d"), hamradio_get_gmikehclip());

		lbl_micAGC->x = lbl_micClip->x;
		lbl_micAGC->y = lbl_micClip->y + interval;
		lbl_micAGC->visible = VISIBLE;
		local_snprintf_P(lbl_micAGC->text, ARRAY_SIZE(lbl_micAGC->text), PSTR("AGC  : %3d"), hamradio_get_gmikeagcgain());

		sl_micLevel->x = lbl_micLevel->x + interval * 2 + interval / 2;
		sl_micLevel->y = lbl_micLevel->y;
		sl_micLevel->visible = VISIBLE;
		sl_micLevel->size = 300;
		sl_micLevel->step = 3;
		sl_micLevel->value = normalize(hamradio_get_mik1level(), level_min, level_max, 100);

		sl_micClip->x = sl_micLevel->x;
		sl_micClip->y = lbl_micClip->y;
		sl_micClip->visible = VISIBLE;
		sl_micClip->size = 300;
		sl_micClip->step = 3;
		sl_micClip->value = normalize(hamradio_get_gmikehclip(), clip_min, clip_max, 100);

		sl_micAGC->x = sl_micLevel->x;
		sl_micAGC->y = lbl_micAGC->y;
		sl_micAGC->visible = VISIBLE;
		sl_micAGC->size = 300;
		sl_micAGC->step = 3;
		sl_micAGC->value = normalize(hamradio_get_gmikeagcgain(), agc_min, agc_max, 100);

		button_t * bh2 = find_gui_element(TYPE_BUTTON, win, "btn_mic_boost");
		bh2->x1 = (sl_micLevel->x + sl_micLevel->size + col1_int * 2) / 2 - (bh2->w / 2);
		bh2->y1 = lbl_micAGC->y + interval;
		bh2->is_locked = hamradio_get_gmikebust20db() ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		local_snprintf_P(bh2->text, ARRAY_SIZE(bh2->text), PSTR("Boost|%s"), bh2->is_locked ? "ON" : "OFF");
		bh2->visible = VISIBLE;

		button_t * bh1 = find_gui_element(TYPE_BUTTON, win, "btn_mic_agc");
		bh1->x1 = bh2->x1 - bh1->w - interval;
		bh1->y1 = bh2->y1;
		bh1->is_locked = hamradio_get_gmikeagc() ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		local_snprintf_P(bh1->text, ARRAY_SIZE(bh1->text), PSTR("AGC|%s"), bh1->is_locked ? "ON" : "OFF");
		bh1->visible = VISIBLE;

		bh1 = find_gui_element(TYPE_BUTTON, win, "btn_mic_OK");
		bh1->x1 = bh2->x1 + bh2->w + interval;
		bh1->y1 = bh2->y1;
		bh1->visible = VISIBLE;

		label_t * lbl_micLevel_min = find_gui_element(TYPE_LABEL, win, "lbl_micLevel_min");
		local_snprintf_P(lbl_micLevel_min->text, ARRAY_SIZE(lbl_micLevel_min->text), PSTR("%d"), level_min);
		lbl_micLevel_min->x = sl_micLevel->x - get_label_width(lbl_micLevel_min) / 2;
		lbl_micLevel_min->y = sl_micLevel->y + get_label_height(lbl_micLevel_min) * 3;
		lbl_micLevel_min->visible = VISIBLE;

		label_t * lbl_micLevel_max = find_gui_element(TYPE_LABEL, win, "lbl_micLevel_max");
		local_snprintf_P(lbl_micLevel_max->text, ARRAY_SIZE(lbl_micLevel_max->text), PSTR("%d"), level_max);
		lbl_micLevel_max->x = sl_micLevel->x + sl_micLevel->size - get_label_width(lbl_micLevel_max) / 2;
		lbl_micLevel_max->y = sl_micLevel->y + get_label_height(lbl_micLevel_max) * 3;
		lbl_micLevel_max->visible = VISIBLE;

		label_t * lbl_micClip_min = find_gui_element(TYPE_LABEL, win, "lbl_micClip_min");
		local_snprintf_P(lbl_micClip_min->text, ARRAY_SIZE(lbl_micClip_min->text), PSTR("%d"), clip_min);
		lbl_micClip_min->x = sl_micClip->x - get_label_width(lbl_micClip_min) / 2;
		lbl_micClip_min->y = sl_micClip->y + get_label_height(lbl_micClip_min) * 3;
		lbl_micClip_min->visible = VISIBLE;

		label_t * lbl_micClip_max = find_gui_element(TYPE_LABEL, win, "lbl_micClip_max");
		local_snprintf_P(lbl_micClip_max->text, ARRAY_SIZE(lbl_micClip_max->text), PSTR("%d"), clip_max);
		lbl_micClip_max->x = sl_micClip->x + sl_micClip->size - get_label_width(lbl_micClip_max) / 2;
		lbl_micClip_max->y = sl_micClip->y + get_label_height(lbl_micClip_max) * 3;
		lbl_micClip_max->visible = VISIBLE;

		label_t * lbl_micAGC_min = find_gui_element(TYPE_LABEL, win, "lbl_micAGC_min");
		local_snprintf_P(lbl_micAGC_min->text, ARRAY_SIZE(lbl_micAGC_min->text), PSTR("%d"), agc_min);
		lbl_micAGC_min->x = sl_micAGC->x - get_label_width(lbl_micAGC_min) / 2;
		lbl_micAGC_min->y = sl_micAGC->y + get_label_height(lbl_micAGC_min) * 3;
		lbl_micAGC_min->visible = VISIBLE;

		label_t * lbl_micAGC_max = find_gui_element(TYPE_LABEL, win, "lbl_micAGC_max");
		local_snprintf_P(lbl_micAGC_max->text, ARRAY_SIZE(lbl_micAGC_max->text), PSTR("%d"), agc_max);
		lbl_micAGC_max->x = sl_micAGC->x + sl_micClip->size - get_label_width(lbl_micAGC_max) / 2;
		lbl_micAGC_max->y = sl_micAGC->y + get_label_height(lbl_micAGC_max) * 3;
		lbl_micAGC_max->visible = VISIBLE;

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_mic_boost = find_gui_element(TYPE_BUTTON, win, "btn_mic_boost");
			button_t * btn_mic_agc = find_gui_element(TYPE_BUTTON, win, "btn_mic_agc");
			button_t * btn_mic_OK = find_gui_element(TYPE_BUTTON, win, "btn_mic_OK");

			if (bh == btn_mic_boost)
			{
				btn_mic_boost->is_locked = hamradio_get_gmikebust20db() ? BUTTON_NON_LOCKED : BUTTON_LOCKED;
				local_snprintf_P(btn_mic_boost->text, ARRAY_SIZE(btn_mic_boost->text), PSTR("Boost|%s"), btn_mic_boost->is_locked ? "ON" : "OFF");
				hamradio_set_gmikebust20db(btn_mic_boost->is_locked);
			}
			else if (bh == btn_mic_agc)
			{
				btn_mic_agc->is_locked = hamradio_get_gmikeagc() ? BUTTON_NON_LOCKED : BUTTON_LOCKED;
				local_snprintf_P(btn_mic_agc->text, ARRAY_SIZE(btn_mic_agc->text), PSTR("AGC|%s"), btn_mic_agc->is_locked ? "ON" : "OFF");
				hamradio_set_gmikeagc(btn_mic_agc->is_locked);
			}
			else if (bh == btn_mic_OK)
			{
				close_all_windows();
				return;
			}
		}
		else if (IS_SLIDER_MOVE)
		{
			slider_t * sh = (slider_t *) ptr;
			if (sh == sl_micLevel)
			{
				uint_fast16_t level = level_min + normalize(sh->value, 0, 100, level_max - level_min);
				local_snprintf_P(lbl_micLevel->text, ARRAY_SIZE(lbl_micLevel->text), PSTR("Level: %3d"), level);
				hamradio_set_mik1level(level);
			}
			else if (sh == sl_micClip)
			{
				uint_fast16_t clip = clip_min + normalize(sh->value, 0, 100, clip_max - clip_min);
				local_snprintf_P(lbl_micClip->text, ARRAY_SIZE(lbl_micClip->text), PSTR("Clip : %3d"), clip);
				hamradio_set_gmikehclip(clip);
			}
			else if (sh == sl_micAGC)
			{
				uint_fast16_t agc = agc_min + normalize(sh->value, 0, 100, agc_max - agc_min);
				local_snprintf_P(lbl_micAGC->text, ARRAY_SIZE(lbl_micAGC->text), PSTR("AGC  : %3d"), agc);
				hamradio_set_gmikeagcgain(agc);
			}
		}
		break;

	default:

		break;
	}
}

// *********************************************************************************************************************************************************************

static void window_ap_mic_prof_process(void)
{
	window_t * win = get_win(WINDOW_AP_MIC_PROF);

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 3;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 1, WINDOW_AP_MIC_PROF, 	NON_VISIBLE, INT32_MAX, "btn_mic_profile_1", "", 0, },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 1, WINDOW_AP_MIC_PROF, 	NON_VISIBLE, INT32_MAX, "btn_mic_profile_2", "", 1, },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 1, WINDOW_AP_MIC_PROF, 	NON_VISIBLE, INT32_MAX, "btn_mic_profile_3", "", 2, },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			button_t * bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
			uint_fast8_t cell_saved = hamradio_load_mic_profile(i, 0);
			local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("Profile %d|%s"), i + 1, cell_saved ? "saved" : "empty");
			bh->payload = cell_saved;
		}

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			uint_fast8_t profile_id = bh->index;
			if (bh->payload)
			{
				hamradio_load_mic_profile(profile_id, 1);
				close_window(DONT_OPEN_PARENT_WINDOW);
				footer_buttons_state(CANCELLED);
				return;
			}
		}
		else if (IS_BUTTON_LONG_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			uint_fast8_t profile_id = bh->index;
			if (bh->payload)
			{
				hamradio_clean_mic_profile(profile_id);
				local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("Profile %d|empty"), profile_id + 1);
				bh->payload = 0;
			}
			else
			{
				hamradio_save_mic_profile(profile_id);
				local_snprintf_P(bh->text, ARRAY_SIZE(bh->text), PSTR("Profile %d|saved"), profile_id + 1);
				bh->payload = 1;
			}
		}
		break;

	default:

		break;
	}
}

// *********************************************************************************************************************************************************************

static void window_menu_process(void)
{
	static uint_fast8_t menu_is_scrolling = 0;
	uint_fast8_t int_col2 = 180, int_col3 = 230, int_rows = 35, is_moving_label = 0;
	static button_t * button_up = NULL, * button_down = NULL;
	window_t * win = get_win(WINDOW_MENU);
	int_fast8_t move_x = 0, move_y = 0, rotate = 0;
	static label_t * selected_label = NULL;
	static uint_fast8_t menu_label_touched = 0;
	static uint_fast8_t menu_level, enc2_code = KBD_CODE_MAX;
	static menu_t menu [MENU_COUNT];

	if (win->first_call)
	{
		uint_fast16_t xmax = 0, ymax = 0;
		win->first_call = 0;
		win->align_mode = ALIGN_CENTER_X;						// выравнивание окна системных настроек всегда по центру

		uint_fast8_t col1_int = 20, row1_int = window_title_height + 20, i;
		uint_fast16_t xn, yn;
		label_t * lh;

		static const button_t buttons [] = {
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MENU, NON_VISIBLE, -1, "btn_SysMenu-", "-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_MENU, NON_VISIBLE, 1, "btn_SysMenu+", "+", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_group",  "", FONT_LARGE, COLORMAIN_WHITE, 0, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_group",  "", FONT_LARGE, COLORMAIN_WHITE, 1, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_group",  "", FONT_LARGE, COLORMAIN_WHITE, 2, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_group",  "", FONT_LARGE, COLORMAIN_WHITE, 3, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_group",  "", FONT_LARGE, COLORMAIN_WHITE, 4, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_group",  "", FONT_LARGE, COLORMAIN_WHITE, 5, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_params", "", FONT_LARGE, COLORMAIN_WHITE, 0, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_params", "", FONT_LARGE, COLORMAIN_WHITE, 1, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_params", "", FONT_LARGE, COLORMAIN_WHITE, 2, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_params", "", FONT_LARGE, COLORMAIN_WHITE, 3, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_params", "", FONT_LARGE, COLORMAIN_WHITE, 4, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 1, NON_VISIBLE, "lbl_params", "", FONT_LARGE, COLORMAIN_WHITE, 5, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 0, NON_VISIBLE, "lbl_vals",   "", FONT_LARGE, COLORMAIN_WHITE, 0, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 0, NON_VISIBLE, "lbl_vals",   "", FONT_LARGE, COLORMAIN_WHITE, 1, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 0, NON_VISIBLE, "lbl_vals",   "", FONT_LARGE, COLORMAIN_WHITE, 2, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 0, NON_VISIBLE, "lbl_vals",   "", FONT_LARGE, COLORMAIN_WHITE, 3, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 0, NON_VISIBLE, "lbl_vals",   "", FONT_LARGE, COLORMAIN_WHITE, 4, },
			{ 0, 0, WINDOW_MENU, CANCELLED, 0, NON_VISIBLE, "lbl_vals",   "", FONT_LARGE, COLORMAIN_WHITE, 5, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		button_up = find_gui_element(TYPE_BUTTON, win, "btn_SysMenu+");
		button_down = find_gui_element(TYPE_BUTTON, win, "btn_SysMenu-");
		button_up->visible = NON_VISIBLE;
		button_down->visible = NON_VISIBLE;

		menu [MENU_GROUPS].add_id = 0;
		menu [MENU_GROUPS].selected_str = 0;
		menu [MENU_GROUPS].selected_label = 0;
		menu [MENU_PARAMS].add_id = 0;
		menu [MENU_PARAMS].selected_str = 0;
		menu [MENU_PARAMS].selected_label = 0;
		menu [MENU_VALS].add_id = 0;
		menu [MENU_VALS].selected_str = 0;
		menu [MENU_VALS].selected_label = 0;

		menu [MENU_GROUPS].first_id = 0;
		for (i = 0; i < win->lh_count; i ++)
		{
			lh = & win->lh_ptr [i];
			if (strcmp(lh->name, "lbl_group"))
				break;
		}

		menu [MENU_GROUPS].last_id = -- i;
		menu [MENU_GROUPS].num_rows = menu [MENU_GROUPS].last_id - menu [MENU_GROUPS].first_id;

		menu [MENU_PARAMS].first_id = ++ i;
		for (; i < win->lh_count; i ++)
		{
			lh = & win->lh_ptr [i];
			if (strcmp(lh->name, "lbl_params"))
				break;
		}
		menu [MENU_PARAMS].last_id = -- i;
		menu [MENU_PARAMS].num_rows = menu [MENU_PARAMS].last_id - menu [MENU_PARAMS].first_id;

		menu [MENU_VALS].first_id = ++ i;
		for (; i < win->lh_count; i ++)
		{
			lh = & win->lh_ptr [i];
			if (strcmp(lh->name, "lbl_vals"))
				break;
		}
		menu [MENU_VALS].last_id = -- i;
		menu [MENU_VALS].num_rows = menu [MENU_VALS].last_id - menu [MENU_VALS].first_id;

		menu [MENU_GROUPS].count = hamradio_get_multilinemenu_block_groups(menu [MENU_GROUPS].menu_block) - 1;
		xn = col1_int;
		yn = row1_int;
		for(i = 0; i <= menu [MENU_GROUPS].num_rows; i ++)
		{
			lh = & win->lh_ptr [menu [MENU_GROUPS].first_id + i];
			strcpy(lh->text, menu [MENU_GROUPS].menu_block [i + menu [MENU_GROUPS].add_id].name);
			lh->visible = VISIBLE;
			lh->color = i == menu [MENU_GROUPS].selected_label ? COLORMAIN_BLACK : COLORMAIN_WHITE;
			lh->x = xn;
			lh->y = yn;
			yn += int_rows;
		}

		menu [MENU_PARAMS].count = hamradio_get_multilinemenu_block_params(menu [MENU_PARAMS].menu_block, menu [MENU_GROUPS].menu_block [menu [MENU_GROUPS].selected_str].index) - 1;
		xn += int_col2;
		yn = row1_int;
		for(i = 0; i <= menu [MENU_PARAMS].num_rows; i ++)
		{
			lh = & win->lh_ptr [menu [MENU_PARAMS].first_id + i];
			strcpy(lh->text, menu [MENU_PARAMS].menu_block [i + menu [MENU_PARAMS].add_id].name);
			lh->visible = VISIBLE;
			lh->color = COLORMAIN_WHITE;
			lh->x = xn;
			lh->y = yn;
			yn += int_rows;
		}

		menu [MENU_VALS].count = menu [MENU_PARAMS].count < menu [MENU_VALS].num_rows ? menu [MENU_PARAMS].count : menu [MENU_VALS].num_rows;
		hamradio_get_multilinemenu_block_vals(menu [MENU_VALS].menu_block, menu [MENU_PARAMS].menu_block [menu [MENU_PARAMS].selected_str].index, menu [MENU_VALS].count);
		xn += int_col3;
		yn = row1_int;
		for(lh = NULL, i = 0; i <= menu [MENU_VALS].num_rows; i ++)
		{
			lh = & win->lh_ptr [menu [MENU_VALS].first_id + i];
			lh->x = xn;
			lh->y = yn;
			yn += int_rows;
			lh->visible = NON_VISIBLE;
			lh->color = COLORMAIN_WHITE;
			if (menu [MENU_VALS].count < i)
				continue;
			strcpy(lh->text, menu [MENU_VALS].menu_block [i + menu [MENU_VALS].add_id].name);
			lh->visible = VISIBLE;
		}

		menu_level = MENU_GROUPS;

		ASSERT(lh != NULL);
		xmax = lh->x + 100;
		ymax = lh->y + get_label_height(lh);

		hamradio_enable_encoder2_redirect();
		calculate_window_position(win, WINDOW_POSITION_MANUAL, xmax, ymax);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			put_to_wm_queue(win, WM_MESSAGE_ENC2_ROTATE, bh->payload);
		}
		else if ((IS_LABEL_PRESS) || (IS_LABEL_MOVE))
		{
			selected_label = (label_t *) ptr;
			if (strcmp(selected_label->name, "lbl_group") == 0)
			{
				menu [MENU_GROUPS].selected_label = selected_label->index;
				menu_label_touched = 1;
				menu_level = MENU_GROUPS;
			}
			else if (strcmp(selected_label->name, "lbl_params") == 0)
			{
				menu [MENU_PARAMS].selected_label = selected_label->index;
				menu_label_touched = 1;
				menu_level = MENU_PARAMS;
			}
			else if (strcmp(selected_label->name, "lbl_vals") == 0)
			{
				menu [MENU_VALS].selected_label = selected_label->index;
				menu [MENU_PARAMS].selected_label = menu [MENU_VALS].selected_label;
				menu_label_touched = 1;
				menu_level = MENU_VALS;
			}

			is_moving_label = IS_LABEL_MOVE;
		}
		break;

	case WM_MESSAGE_ENC2_ROTATE:

		rotate = action;
		break;

	case WM_MESSAGE_KEYB_CODE:

		enc2_code = action;
		break;

	default:

		break;
	}

	get_gui_tracking(& move_x, & move_y);
	if (is_moving_label && move_y != 0)
	{
		static uint_fast8_t start_str_group = 0, start_str_params = 0;
		if (! menu_is_scrolling)
		{
			start_str_group = menu [MENU_GROUPS].add_id;
			start_str_params = menu [MENU_PARAMS].add_id;
		}
		ldiv_t r = ldiv(move_y, int_rows);
		if (strcmp(selected_label->name, "lbl_group") == 0)
		{
			int_fast8_t q = start_str_group - r.quot;
			menu [MENU_GROUPS].add_id = q <= 0 ? 0 : q;
			menu [MENU_GROUPS].add_id = (menu [MENU_GROUPS].add_id + menu [MENU_GROUPS].num_rows) > menu [MENU_GROUPS].count ?
					(menu [MENU_GROUPS].count - menu [MENU_GROUPS].num_rows) : menu [MENU_GROUPS].add_id;
			menu [MENU_GROUPS].selected_str = menu [MENU_GROUPS].selected_label + menu [MENU_GROUPS].add_id;
			menu_level = MENU_GROUPS;
			menu [MENU_PARAMS].add_id = 0;
			menu [MENU_PARAMS].selected_str = 0;
			menu [MENU_PARAMS].selected_label = 0;
			menu [MENU_VALS].add_id = 0;
			menu [MENU_VALS].selected_str = 0;
			menu [MENU_VALS].selected_label = 0;
		}
		else if (strcmp(selected_label->name, "lbl_params") == 0 && menu [MENU_PARAMS].count > menu [MENU_PARAMS].num_rows)
		{
			int_fast8_t q = start_str_params - r.quot;
			menu [MENU_PARAMS].add_id = q <= 0 ? 0 : q;
			menu [MENU_PARAMS].add_id = (menu [MENU_PARAMS].add_id + menu [MENU_PARAMS].num_rows) > menu [MENU_PARAMS].count ?
					(menu [MENU_PARAMS].count - menu [MENU_PARAMS].num_rows) : menu [MENU_PARAMS].add_id;
			menu [MENU_PARAMS].selected_str = menu [MENU_PARAMS].selected_label + menu [MENU_PARAMS].add_id;
			menu [MENU_VALS].add_id = menu [MENU_PARAMS].add_id;
			menu [MENU_VALS].selected_str = menu [MENU_PARAMS].selected_str;
			menu [MENU_VALS].selected_label = menu [MENU_PARAMS].selected_label;
			menu_level = MENU_PARAMS;
		}
		menu_is_scrolling = 1;
	}

	if (! is_moving_label && menu_is_scrolling)
	{
		menu_is_scrolling = 0;
		reset_tracking();
	}

	if (enc2_code != KBD_CODE_MAX || menu_label_touched || menu_is_scrolling)
	{
		// выход из режима редактирования параметра  - краткое или длинное нажатие на энкодер
		if (enc2_code == KBD_ENC2_PRESS && menu_level == MENU_VALS)
		{
			menu_level = MENU_PARAMS;
			enc2_code = KBD_CODE_MAX;
		}
		if (enc2_code == KBD_ENC2_PRESS)
			menu_level = ++menu_level > MENU_VALS ? MENU_VALS : menu_level;
		if (enc2_code == KBD_ENC2_HOLD)
		{
			menu_level = --menu_level == MENU_OFF ? MENU_OFF : menu_level;
			if (menu_level == MENU_GROUPS)
			{
				menu [MENU_PARAMS].add_id = 0;
				menu [MENU_PARAMS].selected_str = 0;
				menu [MENU_PARAMS].selected_label = 0;
				menu [MENU_VALS].add_id = 0;
				menu [MENU_VALS].selected_str = 0;
				menu [MENU_VALS].selected_label = 0;
			}
		}

		// при переходе между уровнями пункты меню выделяется цветом
		label_t * lh = NULL;
		if (menu_level == MENU_VALS)
		{
			menu [MENU_VALS].selected_label = menu [MENU_PARAMS].selected_label;
			lh = & win->lh_ptr [menu [MENU_VALS].first_id + menu [MENU_VALS].selected_label];

			button_down->visible = VISIBLE;
			button_down->x1 = lh->x - button_down->w - 10;
			button_down->y1 = (lh->y + get_label_height(lh) / 2) - (button_down->h / 2);

			button_up->visible = VISIBLE;
			button_up->x1 = lh->x + get_label_width(lh) + 10;
			button_up->y1 = button_down->y1;
			for (uint_fast8_t i = 0; i <= menu [MENU_GROUPS].num_rows; i++)
			{
				lh = & win->lh_ptr [menu [MENU_GROUPS].first_id + i];
				lh->color = i == menu [MENU_GROUPS].selected_label ? COLORMAIN_BLACK : COLORMAIN_GRAY;

				lh = & win->lh_ptr [menu [MENU_PARAMS].first_id + i];
				lh->color = i == menu [MENU_PARAMS].selected_label ? COLORMAIN_BLACK : COLORMAIN_GRAY;

				lh = & win->lh_ptr [menu [MENU_VALS].first_id + i];
				lh->color = i == menu [MENU_PARAMS].selected_label ? COLORMAIN_YELLOW : COLORMAIN_GRAY;
			}
			menu_label_touched = 0;
		}
		else if (menu_level == MENU_PARAMS)
		{
			button_down->visible = NON_VISIBLE;
			button_up->visible = NON_VISIBLE;
			for (uint_fast8_t i = 0; i <= menu [MENU_GROUPS].num_rows; i++)
			{
				lh = & win->lh_ptr [menu [MENU_GROUPS].first_id + i];
				lh->color = i == menu [MENU_GROUPS].selected_label ? COLORMAIN_BLACK : COLORMAIN_GRAY;

				lh = & win->lh_ptr [menu [MENU_PARAMS].first_id + i];
				lh->color = i == menu [MENU_PARAMS].selected_label ? COLORMAIN_BLACK : COLORMAIN_WHITE;

				lh = & win->lh_ptr [menu [MENU_VALS].first_id + i];
				lh->color = COLORMAIN_WHITE;
			}
		}
		else if (menu_level == MENU_GROUPS)
		{
			button_down->visible = NON_VISIBLE;
			button_up->visible = NON_VISIBLE;
			for (uint_fast8_t i = 0; i <= menu [MENU_GROUPS].num_rows; i++)
			{
				lh = & win->lh_ptr [menu [MENU_GROUPS].first_id + i];
				lh->color = i == menu [MENU_GROUPS].selected_label ? COLORMAIN_BLACK : COLORMAIN_WHITE;

				lh = & win->lh_ptr [menu [MENU_PARAMS].first_id + i];
				lh->color = COLORMAIN_WHITE;

				lh = & win->lh_ptr [menu [MENU_VALS].first_id + i];
				lh->color = COLORMAIN_WHITE;
			}
		}

		enc2_code = KBD_CODE_MAX;
	}

	if (menu_level == MENU_OFF)
	{
		if (win->parent_id != NO_PARENT_WINDOW)
		{
			close_all_windows();
			return;
		} else
			menu_level = MENU_GROUPS;
	}

	if (rotate != 0 && menu_level == MENU_VALS)
	{
		menu [MENU_PARAMS].selected_str = menu [MENU_PARAMS].selected_label + menu [MENU_PARAMS].add_id;
		label_t * lh = & win->lh_ptr [menu [MENU_VALS].first_id + menu [MENU_PARAMS].selected_label];
		strcpy(lh->text, hamradio_gui_edit_menu_item(menu [MENU_PARAMS].menu_block [menu [MENU_PARAMS].selected_str].index, rotate));

		lh = & win->lh_ptr [menu [MENU_VALS].first_id + menu [MENU_VALS].selected_label];
		button_up->x1 = lh->x + get_label_width(lh) + 10;
	}

	if ((menu_label_touched || menu_is_scrolling || rotate != 0) && menu_level != MENU_VALS)
	{

		if (rotate != 0)
		{
			menu [menu_level].selected_str = (menu [menu_level].selected_str + rotate) <= 0 ? 0 : (menu [menu_level].selected_str + rotate);
			menu [menu_level].selected_str = menu [menu_level].selected_str > menu [menu_level].count ? menu [menu_level].count : menu [menu_level].selected_str;
		}
		else if (menu_label_touched)
			menu [menu_level].selected_str = menu [menu_level].selected_label + menu [menu_level].add_id;

		menu [MENU_PARAMS].count = hamradio_get_multilinemenu_block_params(menu [MENU_PARAMS].menu_block, menu [MENU_GROUPS].menu_block [menu [MENU_GROUPS].selected_str].index) - 1;

		if (rotate > 0)
		{
			// указатель подошел к нижней границе списка
			if (++menu [menu_level].selected_label > (menu [menu_level].count < menu [menu_level].num_rows ? menu [menu_level].count : menu [menu_level].num_rows))
			{
				menu [menu_level].selected_label = (menu [menu_level].count < menu [menu_level].num_rows ? menu [menu_level].count : menu [menu_level].num_rows);
				menu [menu_level].add_id = menu [menu_level].selected_str - menu [menu_level].selected_label;
			}
		}
		if (rotate < 0)
		{
			// указатель подошел к верхней границе списка
			if (--menu [menu_level].selected_label < 0)
			{
				menu [menu_level].selected_label = 0;
				menu [menu_level].add_id = menu [menu_level].selected_str;
			}
		}

		if (menu_level == MENU_GROUPS)
			for(uint_fast8_t i = 0; i <= menu [MENU_GROUPS].num_rows; i++)
			{
				label_t * l = & win->lh_ptr [menu [MENU_GROUPS].first_id + i];
				strcpy(l->text, menu [MENU_GROUPS].menu_block [i + menu [MENU_GROUPS].add_id].name);
				l->color = i == menu [MENU_GROUPS].selected_label ? COLORMAIN_BLACK : COLORMAIN_WHITE;
			}

		menu [MENU_VALS].count = menu [MENU_PARAMS].count < menu [MENU_VALS].num_rows ? menu [MENU_PARAMS].count : menu [MENU_VALS].num_rows;
		hamradio_get_multilinemenu_block_vals(menu [MENU_VALS].menu_block,  menu [MENU_PARAMS].menu_block [menu [MENU_PARAMS].add_id].index, menu [MENU_VALS].count);

		for(uint_fast8_t i = 0; i <= menu [MENU_PARAMS].num_rows; i++)
		{
			label_t * lp = & win->lh_ptr [menu [MENU_PARAMS].first_id + i];
			label_t * lv = & win->lh_ptr [menu [MENU_VALS].first_id + i];

			lp->visible = NON_VISIBLE;
			lp->state = DISABLED;
			lv->visible = NON_VISIBLE;
			lv->state = DISABLED;
			if (i > menu [MENU_PARAMS].count)
				continue;
			strcpy(lp->text, menu [MENU_PARAMS].menu_block [i + menu [MENU_PARAMS].add_id].name);
			strncpy(lv->text, menu [MENU_VALS].menu_block [i].name, 15);
			lp->color = i == menu [MENU_PARAMS].selected_label && menu_level > MENU_GROUPS ? COLORMAIN_BLACK : COLORMAIN_WHITE;
			lp->visible = VISIBLE;
			lp->state = CANCELLED;
			lv->visible = VISIBLE;
			lv->state = CANCELLED;
		}
		menu_label_touched = 0;
	}

	label_t * lh;
	switch (menu_level)
	{
	case MENU_PARAMS:
	case MENU_VALS:
		lh = & win->lh_ptr [menu [MENU_PARAMS].first_id + menu [MENU_PARAMS].selected_label];
		colpip_rect(colmain_fb_draw(), DIM_X, DIM_Y, win->x1 + lh->x - 5, win->y1 + lh->y - 5, win->x1 + lh->x + int_col3 - 20,
				win->y1 + lh->y + get_label_height(lh) + 5, GUI_MENUSELECTCOLOR, 1);

	case MENU_GROUPS:
		lh = & win->lh_ptr [menu [MENU_GROUPS].first_id + menu [MENU_GROUPS].selected_label];
		colpip_rect(colmain_fb_draw(), DIM_X, DIM_Y, win->x1 + lh->x - 5, win->y1 + lh->y - 5, win->x1 + lh->x + int_col2 - 20,
				win->y1 + lh->y + get_label_height(lh) + 5, GUI_MENUSELECTCOLOR, 1);
	}
}

// *********************************************************************************************************************************************************************

static void window_receive_process(void)
{
	window_t * win = get_win(WINDOW_RECEIVE);
	uint_fast8_t update = 0;

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 3;
		win->first_call = 0;
		update = 1;

		static const button_t buttons [] = {
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_RECEIVE, NON_VISIBLE, INT32_MAX, "btn_att",     "", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_RECEIVE, NON_VISIBLE, INT32_MAX, "btn_AGC", 	 "", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_RECEIVE, NON_VISIBLE, INT32_MAX, "btn_mode", 	 "Mode", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_RECEIVE, NON_VISIBLE, INT32_MAX, "btn_preamp",  "", },
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_RECEIVE, NON_VISIBLE, INT32_MAX, "btn_AF",  	 "AF|filter", },

		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			button_t * bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
		}

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_att = find_gui_element(TYPE_BUTTON, win, "btn_att");
			button_t * btn_preamp = find_gui_element(TYPE_BUTTON, win, "btn_preamp");
			button_t * btn_AF = find_gui_element(TYPE_BUTTON, win, "btn_AF");
			button_t * btn_AGC = find_gui_element(TYPE_BUTTON, win, "btn_AGC");
			button_t * btn_mode = find_gui_element(TYPE_BUTTON, win, "btn_mode");

			if (bh == btn_att)
			{
				hamradio_change_att();
			}
			else if (bh == btn_preamp)
			{
				hamradio_change_preamp();
			}
			else if (bh == btn_AF)
			{
				window_t * win = get_win(WINDOW_AF);
				open_window(win);
			}
			else if (bh == btn_AGC)
			{
				btn_AGC->payload ? hamradio_set_agc_slow() : hamradio_set_agc_fast();
				btn_AGC->payload = hamradio_get_agc_type();
			}
			else if (bh == btn_mode)
			{
				window_t * win = get_win(WINDOW_MODES);
				open_window(win);
			}
		}
		break;

	case WM_MESSAGE_UPDATE:

		update = 1;
		break;

	default:

		break;
	}

	if (update)
	{
		button_t * btn_att = find_gui_element(TYPE_BUTTON, win, "btn_att");
		const char * a = remove_start_line_spaces(hamradio_get_att_value());
		local_snprintf_P(btn_att->text, ARRAY_SIZE(btn_att->text), PSTR("Att|%s"), a == NULL ? "off" : a);

		button_t * btn_preamp = find_gui_element(TYPE_BUTTON, win, "btn_preamp");
		const char * p = remove_start_line_spaces(hamradio_get_preamp_value());
		local_snprintf_P(btn_preamp->text, ARRAY_SIZE(btn_preamp->text), PSTR("Preamp|%s"), p == NULL ? "off" : "on");

		button_t * btn_AGC = find_gui_element(TYPE_BUTTON, win, "btn_AGC");
		local_snprintf_P(btn_AGC->text, ARRAY_SIZE(btn_AGC->text), PSTR("AGC|%s"), btn_AGC->payload ? "fast" : "slow");
	}
}

// *********************************************************************************************************************************************************************

static void window_notch_process(void)
{
	window_t * win = get_win(WINDOW_NOTCH);
	static notch_var_t notch;

	if (win->first_call)
	{
		win->first_call = 0;
		uint_fast8_t interval = 50, col1_int = 20;
		notch.change = 0;
		notch.updated = 1;

		static const button_t buttons [] = {
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_NOTCH, NON_VISIBLE, -1, "btn_freq-",  "-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_NOTCH, NON_VISIBLE, 1,  "btn_freq+",  "+", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_NOTCH, NON_VISIBLE, -1, "btn_width-", "-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_NOTCH, NON_VISIBLE, 1,  "btn_width+", "+", },
			{ 0, 0, 80, 35, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_NOTCH, NON_VISIBLE, 0,  "btn_Auto",   "Auto", },
			{ 0, 0, 80, 35, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_NOTCH, NON_VISIBLE, 1,  "btn_Manual", "Manual", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0, WINDOW_NOTCH, CANCELLED, 0, NON_VISIBLE, "lbl_freq",  "Freq:  *******",  FONT_MEDIUM, COLORMAIN_YELLOW, },
			{ 0, 0, WINDOW_NOTCH, CANCELLED, 0, NON_VISIBLE, "lbl_width", "Width: *******",  FONT_MEDIUM, COLORMAIN_WHITE,  },
			{ 0, 0, WINDOW_NOTCH, CANCELLED, 0, NON_VISIBLE, "lbl_type",  "Type: ",  		FONT_MEDIUM, COLORMAIN_WHITE,  },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		label_t * lbl_freq = find_gui_element(TYPE_LABEL, win, "lbl_freq");
		label_t * lbl_width = find_gui_element(TYPE_LABEL, win, "lbl_width");
		label_t * lbl_type = find_gui_element(TYPE_LABEL, win, "lbl_type");

		lbl_freq->x = col1_int;
		lbl_freq->y = interval;
		lbl_freq->visible = VISIBLE;

		lbl_width->x = lbl_freq->x;
		lbl_width->y = lbl_freq->y + interval;
		lbl_width->visible = VISIBLE;

		lbl_type->x = lbl_width->x;
		lbl_type->y = lbl_width->y + interval;
		lbl_type->visible = VISIBLE;

		button_t * btn_freqm = find_gui_element(TYPE_BUTTON, win, "btn_freq-");
		button_t * btn_freqp = find_gui_element(TYPE_BUTTON, win, "btn_freq+");
		button_t * btn_widthm = find_gui_element(TYPE_BUTTON, win, "btn_width-");
		button_t * btn_widthp = find_gui_element(TYPE_BUTTON, win, "btn_width+");
		button_t * btn_Auto = find_gui_element(TYPE_BUTTON, win, "btn_Auto");
		button_t * btn_Manual = find_gui_element(TYPE_BUTTON, win, "btn_Manual");

		uint_fast16_t x = lbl_freq->x + get_label_width(lbl_freq);
		uint_fast16_t y = lbl_freq->y + get_label_height(lbl_freq) / 2 - btn_freqm->h / 2;

		btn_freqm->x1 = x;
		btn_freqm->y1 = y;
		btn_freqm->visible = VISIBLE;

		btn_freqp->x1 = btn_freqm->x1 + btn_freqm->w + 10;
		btn_freqp->y1 = y;
		btn_freqp->visible = VISIBLE;

		btn_widthm->x1 = btn_freqm->x1;
		btn_widthm->y1 = btn_freqm->y1 + interval;
		btn_widthm->visible = VISIBLE;

		btn_widthp->x1 = btn_freqp->x1;
		btn_widthp->y1 = btn_widthm->y1;
		btn_widthp->visible = VISIBLE;

		btn_Auto->x1 = lbl_type->x + get_label_width(lbl_type);
		btn_Auto->y1 = lbl_type->y + get_label_height(lbl_type) / 2 - btn_Auto->h / 2;
		btn_Auto->visible = VISIBLE;

		btn_Manual->x1 = btn_Auto->x1 + btn_Auto->w + 10;
		btn_Manual->y1 = btn_Auto->y1;
		btn_Manual->visible = VISIBLE;

		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_freqm = find_gui_element(TYPE_BUTTON, win, "btn_freq-");
			button_t * btn_freqp = find_gui_element(TYPE_BUTTON, win, "btn_freq+");
			button_t * btn_widthm = find_gui_element(TYPE_BUTTON, win, "btn_width-");
			button_t * btn_widthp = find_gui_element(TYPE_BUTTON, win, "btn_width+");
			button_t * btn_Auto = find_gui_element(TYPE_BUTTON, win, "btn_Auto");
			button_t * btn_Manual = find_gui_element(TYPE_BUTTON, win, "btn_Manual");

			if (bh == btn_freqm || bh == btn_freqp)
			{
				notch.select = TYPE_NOTCH_FREQ;
				notch.change = bh->payload;
				notch.updated = 1;
			}
			else if (bh == btn_widthm || bh == btn_widthp)
			{
				notch.select = TYPE_NOTCH_WIDTH;
				notch.change = bh->payload;
				notch.updated = 1;
			}
			else if (bh == btn_Manual || bh == btn_Auto)
			{
				hamradio_set_gnotchtype(bh->payload);
				notch.change = 0;
				notch.updated = 1;
			}
		}
		else if (IS_LABEL_PRESS)
		{
			label_t * lh = (label_t *) ptr;
			label_t * lbl_freq = find_gui_element(TYPE_LABEL, win, "lbl_freq");
			label_t * lbl_width = find_gui_element(TYPE_LABEL, win, "lbl_width");

			if (lh == lbl_freq)
				notch.select = TYPE_NOTCH_FREQ;
			else if (lh == lbl_width)
				notch.select = TYPE_NOTCH_WIDTH;

			notch.change = 0;
			notch.updated = 1;
		}
		break;

	case WM_MESSAGE_ENC2_ROTATE:

		notch.change = action;
		notch.updated = 1;
		break;

	case WM_MESSAGE_UPDATE:

		notch.change = 0;
		notch.updated = 1;
		break;

	default:

		break;
	}

	if (notch.updated)
	{
		button_t * btn_Auto = find_gui_element(TYPE_BUTTON, win, "btn_Auto");
		button_t * btn_Manual = find_gui_element(TYPE_BUTTON, win, "btn_Manual");
		label_t * lbl_freq = find_gui_element(TYPE_LABEL, win, "lbl_freq");
		label_t * lbl_width = find_gui_element(TYPE_LABEL, win, "lbl_width");

		notch.updated = 0;

		for(uint_fast8_t i = 0; i < win->lh_count; i ++)
			win->lh_ptr [i].color = COLORMAIN_WHITE;

		ASSERT(notch.select < win->lh_count);
		win->lh_ptr [notch.select].color = COLORMAIN_YELLOW;

		uint_fast8_t type = hamradio_get_gnotchtype();
		btn_Auto->is_locked = type == BOARD_NOTCH_AUTO ? BUTTON_LOCKED : BUTTON_NON_LOCKED;
		btn_Manual->is_locked = type == BOARD_NOTCH_MANUAL ? BUTTON_LOCKED : BUTTON_NON_LOCKED;

		local_snprintf_P(lbl_freq->text, ARRAY_SIZE(lbl_freq->text), PSTR("Freq:%5d Hz"), hamradio_notch_freq(notch.select == TYPE_NOTCH_FREQ ? notch.change : 0));
		local_snprintf_P(lbl_width->text, ARRAY_SIZE(lbl_width->text), PSTR("Width:%4d Hz"), hamradio_notch_width(notch.select == TYPE_NOTCH_WIDTH ? notch.change : 0));
	}
}

// *********************************************************************************************************************************************************************

static void window_gui_settings_process(void)
{
	window_t * win = get_win(WINDOW_GUI_SETTINGS);
	uint_fast8_t update = 0;

	if (win->first_call)
	{
		uint_fast16_t x = 0, y = 0;
		uint_fast8_t interval = 6, col1_int = 20, row1_int = window_title_height + 20, row_count = 4;
		win->first_call = 0;

		static const button_t buttons [] = {
			{ 0, 0, 100, 44, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_GUI_SETTINGS, NON_VISIBLE, INT32_MAX, "btn_enc2_step", "", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		x = col1_int;
		y = row1_int;

		for (uint_fast8_t i = 0, r = 1; i < win->bh_count; i ++, r ++)
		{
			button_t * bh = & win->bh_ptr [i];
			bh->x1 = x;
			bh->y1 = y;
			bh->visible = VISIBLE;

			x = x + interval + bh->w;
			if (r >= row_count)
			{
				r = 0;
				x = col1_int;
				y = y + bh->h + interval;
			}
		}

		update = 1;
		calculate_window_position(win, WINDOW_POSITION_AUTO);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			button_t * btn_enc2_step = find_gui_element(TYPE_BUTTON, win, "btn_enc2_step");

			if (bh == btn_enc2_step)
			{
				gui_nvram.enc2step_pos = (gui_nvram.enc2step_pos + 1 ) % enc2step_vals;
				save_settings();
				update = 1;
			}
		}
		break;

	default:

		break;
	}

	if (update)
	{
		update = 0;
		button_t * btn_enc2_step = find_gui_element(TYPE_BUTTON, win, "btn_enc2_step");
		local_snprintf_P(btn_enc2_step->text, ARRAY_SIZE(btn_enc2_step->text), PSTR("Enc2 step|%s"), enc2step [gui_nvram.enc2step_pos].label);
	}
}

// *********************************************************************************************************************************************************************

void gui_uif_editmenu(const char * name, uint_fast16_t menupos, uint_fast8_t exitkey)
{
	window_t * win = get_win(WINDOW_UIF);
	if (win->state == NON_VISIBLE)
	{
		close_window(DONT_OPEN_PARENT_WINDOW);
		open_window(win);
		footer_buttons_state(DISABLED, NULL);
		strcpy(menu_uif.name, name);
		menu_uif.menupos = menupos;
		menu_uif.exitkey = exitkey;
		hamradio_enable_encoder2_redirect();
	}
	else if (win->state == VISIBLE)
	{
		close_window(DONT_OPEN_PARENT_WINDOW);
		footer_buttons_state(CANCELLED);
		hamradio_disable_encoder2_redirect();
	}
}

static void window_uif_process(void)
{
	static label_t * lbl_uif_val;
	static button_t * button_up, * button_down;
	static uint_fast16_t window_center_x;
	static uint_fast8_t reinit = 0;
	window_t * win = get_win(WINDOW_UIF);
	int_fast8_t rotate = 0;

	if (win->first_call)
	{
		win->first_call = 0;
		reinit = 1;
		static const uint_fast8_t win_width = 170;

		static const button_t buttons [] = {
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_UIF, NON_VISIBLE, -1, "btn_UIF-", "-", },
			{ 0, 0, 40, 40, CANCELLED, BUTTON_NON_LOCKED, 0, WINDOW_UIF, NON_VISIBLE, 1, "btn_UIF+",  "+", },
		};
		win->bh_count = ARRAY_SIZE(buttons);
		uint_fast16_t buttons_size = sizeof(buttons);
		win->bh_ptr = malloc(buttons_size);
		GUI_MEM_ASSERT(win->bh_ptr);
		memcpy(win->bh_ptr, buttons, buttons_size);

		static const label_t labels [] = {
			{ 0, 0,	WINDOW_UIF,  DISABLED,  0, NON_VISIBLE, "lbl_uif_val", 	 "", FONT_LARGE, COLORMAIN_WHITE, },
		};
		win->lh_count = ARRAY_SIZE(labels);
		uint_fast16_t labels_size = sizeof(labels);
		win->lh_ptr = malloc(labels_size);
		GUI_MEM_ASSERT(win->lh_ptr);
		memcpy(win->lh_ptr, labels, labels_size);

		label_t * lbl_uif_val = find_gui_element(TYPE_LABEL, win, "lbl_uif_val");
		calculate_window_position(win, WINDOW_POSITION_MANUAL, win_width, window_title_height + get_label_height(lbl_uif_val) * 4);
	}

	uint_fast8_t type;
	int_fast8_t action;
	uintptr_t ptr;
	switch (get_from_wm_queue(win, & type, & ptr, & action))
	{
	case WM_MESSAGE_ACTION:

		if (IS_BUTTON_PRESS)
		{
			button_t * bh = (button_t *) ptr;
			put_to_wm_queue(win, WM_MESSAGE_ENC2_ROTATE, bh->payload);
		}
		break;

	case WM_MESSAGE_ENC2_ROTATE:

		rotate = action;
		break;

	case WM_MESSAGE_KEYB_CODE:

		if (action == menu_uif.exitkey)
		{
			hamradio_disable_keyboard_redirect();
			close_window(DONT_OPEN_PARENT_WINDOW);
			footer_buttons_state(CANCELLED);
			return;
		}

		break;

	default:

		break;
	}

	if (reinit)
	{
		reinit = 0;
		uint_fast8_t col1_int = 20, row1_int = window_title_height + 20;
		strcpy(win->name, menu_uif.name);

		button_down = find_gui_element(TYPE_BUTTON, win, "btn_UIF-");
		button_up = find_gui_element(TYPE_BUTTON, win, "btn_UIF+");
		lbl_uif_val = find_gui_element(TYPE_LABEL, win, "lbl_uif_val");

		const char * v = hamradio_gui_edit_menu_item(menu_uif.menupos, 0);
		strcpy(lbl_uif_val->text, v);

		button_down->x1 = col1_int;
		button_down->y1 = row1_int;
		button_down->visible = VISIBLE;

		button_up->x1 = button_down->x1 + button_down->w + 30 + get_label_width(lbl_uif_val);
		button_up->y1 = button_down->y1;
		button_up->visible = VISIBLE;

		window_center_x = (col1_int + button_up->x1 + button_up->w) / 2;

		lbl_uif_val->x = window_center_x - get_label_width(lbl_uif_val) / 2;
		lbl_uif_val->y = row1_int + button_up->h / 2 - get_label_height(lbl_uif_val) / 2;
		lbl_uif_val->visible = VISIBLE;

		hamradio_enable_keyboard_redirect();
		return;
	}

	if (rotate != 0)
	{
		hamradio_gui_edit_menu_item(menu_uif.menupos, rotate);
		reinit = 1;
		gui_update(NULL);
	}
}

// *********************************************************************************************************************************************************************

void gui_encoder2_menu (enc2_menu_t * enc2_menu)
{
	memcpy(& gui_enc2_menu, enc2_menu, sizeof (gui_enc2_menu));
	gui_enc2_menu.updated = 1;
}

// *********************************************************************************************************************************************************************

void gui_open_sys_menu(void)
{
	window_t * win = get_win(WINDOW_MENU);
	static uint_fast8_t backup_parent = NO_PARENT_WINDOW;

	if (check_for_parent_window() == NO_PARENT_WINDOW && win->parent_id != NO_PARENT_WINDOW)
	{
		backup_parent = win->parent_id;		// для возможности открытия окна напрямую временно "обнулить" parent_id
		win->parent_id = NO_PARENT_WINDOW;
		open_window(win);
		footer_buttons_state(DISABLED, NULL);
	}
	else if (check_for_parent_window() == WINDOW_MENU && win->parent_id == NO_PARENT_WINDOW)
	{
		close_window(DONT_OPEN_PARENT_WINDOW);
		footer_buttons_state(CANCELLED);
		win->parent_id = backup_parent;
		backup_parent = NO_PARENT_WINDOW;
	}
}

// *********************************************************************************************************************************************************************

#endif /* WITHTOUCHGUI */
