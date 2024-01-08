extern "C" {
#include "emulator.h"
#include "manager.h"
}

#include <pico/time.h>
#include <pico/multicore.h>
#include <hardware/pwm.h>
#include "hardware/clocks.h"
#include <pico/stdlib.h>
#include <hardware/vreg.h>
#include <pico/stdio.h>

#include "psram_spi.h"
#include "nespad.h"

extern "C" {
#include "vga.h"
#include "ps2.h"
#include "usb.h"
//#include "CPU_bk.h"
#include "main_i.h"
#include "emu_e.h"
}

bool PSRAM_AVAILABLE = false;
bool SD_CARD_AVAILABLE = false;
uint32_t DIRECT_RAM_BORDER = PSRAM_AVAILABLE ? RAM_SIZE : (SD_CARD_AVAILABLE ? RAM_PAGE_SIZE : RAM_SIZE);

uint8_t __aligned(4096) TEXT_VIDEO_RAM[VIDEORAM_SIZE] = { 0 };
uint8_t __aligned(4096) RAM[RAM_SIZE] = { 0 };

pwm_config config = pwm_get_default_config();
#define PWM_PIN0 (26)
#define PWM_PIN1 (27)

void PWM_init_pin(uint8_t pinN) {
    gpio_set_function(pinN, GPIO_FUNC_PWM);
    pwm_config_set_clkdiv(&config, 1.0);
    pwm_config_set_wrap(&config, (1 << 12) - 1); // MAX PWM value
    pwm_init(pwm_gpio_to_slice_num(pinN), &config, true);
}
#if NESPAD_ENABLED
int timer_period = 54925;
#endif
struct semaphore vga_start_semaphore;
/* Renderer loop on Pico's second core */
void __time_critical_func(render_core)() {
    graphics_init();
    graphics_set_buffer(&RAM[0x10000], 512, 256);
    graphics_set_textbuffer(TEXT_VIDEO_RAM);
    graphics_set_bgcolor(0x80808080);
    graphics_set_offset(0, 0);
    graphics_set_flashmode(true, true);
    sem_acquire_blocking(&vga_start_semaphore);
#if NESPAD_ENABLED
    uint8_t tick50ms_counter = 0;
    while (true) {
        busy_wait_us(timer_period);
        if (tick50ms_counter % 2 == 0 && nespad_available) {
            nespad_read();
            if (nespad_state) {
                sermouseevent(nespad_state & DPAD_B ? 1 : nespad_state & DPAD_A ? 2 : 0,
                              nespad_state & DPAD_LEFT ? -3 : nespad_state & DPAD_RIGHT ? 3 : 0,
                              nespad_state & DPAD_UP ? -3 : nespad_state & DPAD_DOWN ? 3 : 0);
            }
        }
        if (tick50ms_counter < 20) {
            tick50ms_counter++;
        }
        else {
            tick50ms_counter = 0;
        }
    }
#endif
}

void inInit(uint gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

static FATFS fat_fs;

#include "Board.h"

CMotherBoard mb_test;
static std::unique_ptr<CMotherBoard> m_pBoard(nullptr);

static inline void StopTimer() {
	///		KillTimer(BKTIMER_UI_REFRESH);      // остановить таймер для OnMainLoop
	///		KillTimer(BKTIMER_UI_TIME);         // остановить таймер для OnMainLoopTimer
}

static void StopAll()
{
	StopTimer();
	if (m_pBoard)
	{
		m_pBoard->StopCPU(); // остановка CPU - там прекращается обработка команд и поток работает вхолостую
		m_pBoard->StopTimerThread(); // остановка и завершение потока.
	}
}

static void StartAll()
{
	if (m_pBoard)
	{
		m_pBoard->StartTimerThread();
		m_pBoard->RunCPU();
	}
//	StartTimer();
}

// выносим это в отдельную процедуру, т.к. есть вероятность неудачного прочтения MSF,
// при этом надо восстановить всё как было раньше
static void AttachObjects()
{/**
	auto vw = DYNAMIC_DOWNCAST(CBKView, GetActiveView());

	if (vw)
	{
		vw->ReCreateSCR(); // пересоздаём экран с новыми параметрами.
		m_nScreen_X = g_Config.m_nScreenW;
		m_pScreen->SetScreenViewport(m_nScreen_X);
		InitScrViewPort(m_nScreen_X);
	}

	int nMtc = m_pSound->ReInit(g_Config.m_nSoundSampleRate); // пересоздаём звук с новыми параметрами, на выходе - длина медиафрейма в сэмплах
	m_speaker.ReInit();     // ещё надо переинициализирвоать устройства, там
	m_speaker.ConfigureTapeBuffer(nMtc);// переопределяем буферы в зависимости от текущей частоты дискретизации
	m_covox.ReInit();       // есть вещи, зависящие от частоты дискретизации,
	m_menestrel.ReInit();   //
	m_aySnd.ReInit();       // которая теперь величина переменная. Но требует перезапуска конфигурации.
	m_paneOscillatorView.ReCreateOSC(); // пересоздаём осциллограф с новыми параметрами
	// при необходимости откорректируем размер приёмного буфера.
	m_paneOscillatorView.SetBuffer(nMtc); //SendMessage(WM_OSC_SETBUFFER, WPARAM(nMtc));
**/
	if (m_pBoard)
	{
	///	m_pBoard->AttachWindow(this);  // цепляем к MotherBoard этот класс
		// порядок имеет значение. сперва нужно делать обязательно AttachWindow(this)
		// и только затем m_pBoard->SetMTC(). И эта функция обязательна, там звуковой буфер вычисляется
		// и выделяется
	///	m_pBoard->SetMTC(nMtc); // и здесь ещё. тройная работа получается.
		// Присоединяем к новосозданному чипу устройства
	///	m_pBoard->AttachSound(m_pSound.get());
	///	m_pBoard->AttachSpeaker(&m_speaker);
	///	m_pBoard->AttachMenestrel(&m_menestrel);
	///	m_pBoard->AttachCovox(&m_covox);
	///	m_pBoard->AttachAY8910(&m_aySnd);
		// если в ини файле задана частота, то применим её, вместо частоты по умолчанию.
	///	m_pBoard->NormalizeCPU();
		// Цепляем к новому чипу отладчик, т.е. наоборот, к отладчику чип
	///	m_pDebugger->AttachBoard(GetBoard());
	///	m_paneRegistryDumpViewCPU.SetFreqParam();
		// Цепляем обработчик скриптов
	///	m_Script.AttachBoard(GetBoard());
	}
}

static void InitEmulator()
{
///	CString str = CString(MAKEINTRESOURCE(g_mstrConfigBKModelParameters[static_cast<int>(g_Config.GetBKModel())].nIDBKModelName));
///	UpdateFrameTitleForDocument(str);
/***
	switch (g_Config.m_nVKBDType)
	{
		default:
		case 0:
			m_paneBKVKBDView.SetKeyboardView(IDB_BITMAP_SOFT);
			break;

		case 1:
			m_paneBKVKBDView.SetKeyboardView(IDB_BITMAP_PLEN);
			break;
	}

	auto pSlider = DYNAMIC_DOWNCAST(CSliderButton, m_wndToolBarSound.GetButton(m_wndToolBarSound.CommandToIndex(ID_OPTIONS_SOUND_VOLUME)));

	if (pSlider)
	{
		pSlider->SetValue(g_Config.m_nSoundVolume);
	}

	m_pSound->SoundGen_SetVolume(g_Config.m_nSoundVolume); // продублируем для надёжности
	// инициализация текущих настроек
	m_speaker.EnableSound(g_Config.m_bSpeaker);
	m_speaker.SetFilter(g_Config.m_bSpeakerFilter);
	m_speaker.SetDCOffsetCorrect(g_Config.m_bSpeakerDCOffset);
	m_covox.EnableSound(g_Config.m_bCovox);
	m_covox.SetFilter(g_Config.m_bCovoxFilter);
	m_covox.SetDCOffsetCorrect(g_Config.m_bCovoxDCOffset);
	m_covox.SetStereo(g_Config.m_bStereoCovox);
	m_menestrel.EnableSound(g_Config.m_bMenestrel);
	m_menestrel.SetFilter(g_Config.m_bMenestrelFilter);
	m_menestrel.SetDCOffsetCorrect(g_Config.m_bMenestrelDCOffset);
	m_menestrel.SetStereo(true); // всегда стерео
	m_aySnd.EnableSound(g_Config.m_bAY8910);
	m_aySnd.SetFilter(g_Config.m_bAY8910Filter);
	m_aySnd.SetDCOffsetCorrect(g_Config.m_bAY8910DCOffset);
	m_aySnd.SetStereo(true); // всегда стерео
	m_tape.SetWaveParam(g_Config.m_nSoundSampleRate, BUFFER_CHANNELS);
    ***/
	// параметры экрана
	m_pBoard->ChangePalette();
/**	m_pScreen->SetSmoothing(g_Config.m_bSmoothing);
	m_pScreen->SetColorMode(g_Config.m_bColorMode);
	m_pScreen->SetAdaptMode(g_Config.m_bAdaptBWMode);
	m_pScreen->SetLuminoforeEmuMode(g_Config.m_bLuminoforeEmulMode);
	g_Config.m_bFullscreenMode ? m_pScreen->SetFullScreenMode() : m_pScreen->SetWindowMode();

	// обновим адрес дампа памяти
	for (int i = 0; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		m_arPaneMemoryDumpView[i].SetDumpWindows(m_pBoard->GetWndVectorPtr(), g_Config.m_arDumper[i].nPageListPos, g_Config.m_arDumper[i].nAddrListPos);
		m_arPaneMemoryDumpView[i].SetAddress(g_Config.m_arDumper[i].nAddr);
	}

	// обновим адрес дизассемблера
	m_paneDisassembleView.SetAddr(g_Config.m_nDisasmAddr);
	// Настройка панели управления записью
	m_paneTapeCtrlView.InitParams(&m_tape);
	// наглядно отобразим, что и в каком дисководе находится
	UpdateToolbarDriveIcons();
///	UpdateData(FALSE);**/
}

static bool ConfigurationConstructor(CONF_BKMODEL nConf, bool bStart = true)
{
	bool bReopenMemMap = false;

	if (m_pBoard)
	{
		// Если конфигурация уже существует, удалим её
		StopAll();  // сперва всё остановим
	///	bReopenMemMap = CheckDebugMemmap(); // флаг переоткрытия карты памяти
		// перед сохранением настройки флагов заберём из диалога
	///	StoreIniParams();
		g_Config.SaveConfig();

		if (m_pBoard)   // удалим конфигурацию
		{
			m_pBoard.reset();
		}
	}

	g_Config.SetBKModel(nConf);

	// создадим новую конфигурацию
	switch (g_Config.m_BKBoardModel)
	{
		case MSF_CONF::BK1001:
			m_pBoard = std::make_unique<CMotherBoard>();
			break;
            /**
		case MSF_CONF::BK10:
			m_pBoard = std::make_unique<CMotherBoard_10>();
			break;

		case MSF_CONF::BK1001_MSTD:
			m_pBoard = std::make_unique<CMotherBoard_MSTD>();
			break;

		case MSF_CONF::BK1001_EXT32:
			m_pBoard = std::make_unique<CMotherBoard_EXT32>();
			break;

		case MSF_CONF::BK1001_FDD:
			m_pBoard = std::make_unique<CMotherBoard_10_FDD>();
			break;

		case MSF_CONF::BK11:
			m_pBoard = std::make_unique<CMotherBoard_11>();
			break;

		case MSF_CONF::BK11_FDD:
			m_pBoard = std::make_unique<CMotherBoard_11_FDD>();
			break;

		case MSF_CONF::BK11M:
			m_pBoard = std::make_unique<CMotherBoard_11M>();
			break;

		case MSF_CONF::BK11M_FDD:
			m_pBoard = std::make_unique<CMotherBoard_11M_FDD>();
			break;
**/
		default:
			ASSERT(false);
			return false;
	}

	if (m_pBoard)
	{
		m_pBoard->SetFDDType(g_Config.m_BKFDDModel);
		g_Config.LoadConfig(false); // Читаем из ини файла параметры
		AttachObjects();    // пересоздадим и присоединим необходимые устройства.

		if (m_pBoard->InitBoard(g_Config.m_nCPURunAddr))
		{
			InitEmulator();         // переинициализируем модель

			if (bReopenMemMap)      // если надо
			{
	///			OnDebugMemmap();    // заново откроем карту памяти
			}

			if (bStart)
			{
				// Запускаем CPU
				StartAll();

				// если не установлен флаг остановки после создания
				if (g_Config.m_bPauseCPUAfterStart)
				{
					m_pBoard->BreakCPU();
				}
			}

		///	SetFocusToBK();
			return true;
		}

		// если ресет не удался - значит не удалось проинициализировать
		// память - значит не удалось загрузить какие-то дампы прошивок -
		// значит дальше работать невозможно.
		if (m_pBoard)
		{
			m_pBoard.reset();
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return false;
}

static void SetupConfiguration(CONF_BKMODEL nConf)
{
	///m_Script.StopScript();

	if (ConfigurationConstructor(nConf))
	{
	///	SetFocusToBK();
	}
	else
	{
		// надо вообще закрыть прогу. нечего тут делать.
	///	CString str;
	///	str.Format(IDS_BK_ERROR_MFBKBOARDERROR, g_Config.GetBKConfName());
	///	g_BKMsgBox.Show(str, MB_OK);
		// сбросим модель на дефолтную
	///	g_Config.SetBKModel(CONF_BKMODEL::BK_0010_01);
	///	DestroyWindow(); // не создалось - ничего не можем сделать. выходим.
	}
}

int main() {
#if (OVERCLOCKING > 270)
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_ms(33);
    set_sys_clock_khz(OVERCLOCKING * 1000, true);
#else
    vreg_set_voltage(VREG_VOLTAGE_1_15);
    sleep_ms(33);
    set_sys_clock_khz(270000, true);
#endif
    gpio_set_function(BEEPER_PIN, GPIO_FUNC_PWM);
    pwm_init(pwm_gpio_to_slice_num(BEEPER_PIN), &config, true);

#ifdef SOUND_SYSTEM
    PWM_init_pin(PWM_PIN0);
    PWM_init_pin(PWM_PIN1);
#endif

#if LOAD_WAV_PIO
    //пин ввода звука
    inInit(LOAD_WAV_PIO);
#endif

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    for (int i = 0; i < 6; i++) {
        sleep_ms(23);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(23);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
    }

    nespad_begin(clock_get_hz(clk_sys) / 1000, NES_GPIO_CLK, NES_GPIO_DATA, NES_GPIO_LAT);
    keyboard_init();

    sem_init(&vga_start_semaphore, 0, 1);
    multicore_launch_core1(render_core);
    sem_release(&vga_start_semaphore);

    sleep_ms(50);

    memset(RAM, 0, sizeof RAM);
    memset(TEXT_VIDEO_RAM, 0, sizeof TEXT_VIDEO_RAM);
    graphics_set_mode(color_mode ? BK_256x256x2 : BK_512x256x1);

    init_psram();

    FRESULT result = f_mount(&fat_fs, "", 1);
    if (FR_OK != result) {
        char tmp[80];
        sprintf(tmp, "Unable to mount SD-card: %s (%d)", FRESULT_str(result), result);
        logMsg(tmp);
    } else {
        SD_CARD_AVAILABLE = true;
    }

    DIRECT_RAM_BORDER = PSRAM_AVAILABLE ? RAM_SIZE : (SD_CARD_AVAILABLE ? RAM_PAGE_SIZE : RAM_SIZE);

SetupConfiguration(CONF_BKMODEL::BK_0010);

    while(1) {
        // TODO: remove it
    }

    if (SD_CARD_AVAILABLE) {
        insertdisk(0, fdd0_sz(), fdd0_rom(), "\\BK\\fdd0.img");
        insertdisk(1, fdd1_sz(), fdd1_rom(), "\\BK\\fdd1.img");
        insertdisk(2, 0, 0, "\\BK\\hdd0.img");
        insertdisk(3, 0, 0, "\\BK\\hdd1.img");
    }

    main_init();
    emu_start();
    return 0;
}

/*
177662 Разряд 14 (БК11М) - управляет включением системного таймера. (вектор 100) При значении 0 таймер выключен, при 1 - включен.
F таймера = 48.5 Hz
? Manwe@ - 17.02.2020 15:51
Можно и без потрошения видеоконтроллера посчитать экспериментально.
Таймер процессора насчитывает 640 тиков за кадр на БК 0011м. Это 640*128 тактов процессора.
Я проводил эксперимент: 320 тиков таймера показывал одну страницу памяти, 320 другую (обе страницы раскрашены в разные цвета).
 В таком эксперименте экран «разрезан» на две части. Эта линия разреза стабильно держится на месте и никуда не уплывает.
  Если бы между кадрами проходило не ровно 640 тиков таймера, граница постепенно уплывала.
Теперь считаем кадровую частоту: 4 МГц / (640*128) = 48.828125
t = 1 / 48,828125 Hz = 0,02048 s = 20,48 ms == 20480 mks (us)
*/
//#include "hardware/timer.h"
static repeating_timer_t system_timer_50_Hz = { 0 };
#define SYSTEM_TIMER_US (int64_t)20480

extern "C" void TickIRQ2();

static bool system_timer_50_Hz_cb(repeating_timer_t *rt) {
    if ((*(uint16_t*)rt->user_data) & (1 << 14)) {  // timer is not masked by system register
        TickIRQ2();
        return true;
    }
    return true;
}

extern "C" void init_system_timer(uint16_t* pReg, bool enable) {
    if (enable) {
 //       add_repeating_timer_us (SYSTEM_TIMER_US, system_timer_50_Hz_cb, pReg, &system_timer_50_Hz);
    } else if (system_timer_50_Hz.user_data) {
        cancel_repeating_timer(&system_timer_50_Hz);
        system_timer_50_Hz.user_data = 0;
    }
}
