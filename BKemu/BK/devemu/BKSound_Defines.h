#pragma once

// !!!а алгоритмы не универсальные. Они рассчитаны на исключительно 2 канала
constexpr int DEFAULT_SOUND_SAMPLE_RATE = 44100;

constexpr short BUFFER_CHANNELS = 2;  // кол-во каналов 2, всегда и везде;

using SAMPLE_INT = double; // разрядность сэмплов для внутренней обработки
constexpr int SAMPLE_INT_SIZE = sizeof(SAMPLE_INT); // размер сэмпла в байтах в одном канале
constexpr short SAMPLE_INT_BLOCKALIGN = (BUFFER_CHANNELS *SAMPLE_INT_SIZE);     // длина сэмпла в байтах во всех каналах
constexpr short SAMPLE_INT_BPS = (8 * SAMPLE_INT_SIZE); // размер сэмпла в битах в одном канале

#define OSL 0
#define OSR 1
struct sOneSample
{
	SAMPLE_INT s[BUFFER_CHANNELS];
};

using SAMPLE_IO = short; // разрядность сэмплов 16 бит для ввода/вывода и обмена с внешним миром
constexpr int SAMPLE_IO_SIZE = sizeof(SAMPLE_IO);   // размер сэмпла в байтах в одном канале
constexpr short SAMPLE_IO_BLOCKALIGN = (BUFFER_CHANNELS *SAMPLE_IO_SIZE);   // длина сэмпла в байтах во всех каналах
constexpr short SAMPLE_IO_BPS = (8 * SAMPLE_IO_SIZE); // размер сэмпла в битах в одном канале

constexpr SAMPLE_INT FLOAT_BASE = 32768.0;

constexpr short MIN_SAMPLE = 0; // вот они 16 битные сэмплы
constexpr short AVG_SAMPLE = 0x4000;
constexpr short MAX_SAMPLE = 0x7fff;

