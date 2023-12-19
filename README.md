# Raspberry БК-0011M (К1801ВМ1 - PDP-11 based) Emulator for MURMULATOR devboard
Based on:

# БК-0011М https://github.com/konst-st/BK8266/tree/BK0011M
Эмулятор БК-0011М на ESP8266<br/>
Сделан на основе эмулятора БК-0010-01 на ESP8266 https://github.com/konst-st/BK8266

# Hardware needed
To get it working you should have an Murmulator (development) board with VGA output. Schematics available here at https://github.com/AlexEkb4ever/MURMULATOR_classical_scheme
![Murmulator Schematics](https://github.com/javavi/pico-infonesPlus/blob/main/assets/Murmulator-1_BSchem.JPG)

Extra PSRAM support on pi pico pins:
* PSRAM_PIN_CS=18
* PSRAM_PIN_SCK=19
* PSRAM_PIN_MOSI=20
* PSRAM_PIN_MISO=21

![RAM extention](/psram.jpg)

# Эмулятор К1801ВМ1
Эмулятор цельнотянутый с https://github.com/konst-st/BK8266/tree/BK0011M.<br/>
Кое чего подсмотрено из эмулятора Юрия Калмыкова http://gid.pdp-11.ru<br/>

Эмулятор использует только ОЗУ RP2040 (Raspberry Pi Pico) (из которыз 128 кБ выделено под ОЗУ БК0011М).<br/>
ПЗУ БК11М содержится во флэш (используется встроеннное кэширование пики).<br/>

Пока не реализовано:
<ul>
<li>Прерывание по вектору 14 после выполнения каждой команды при установленном в PSW бите T (трассировка).</li>
<li>Прерывание от таймера 50 Гц.</li>
<li>Чтение и запись файлов (планирую эмулировать дисковод).</li>
<li>Так же планирую реализовать эмуляцию звукового сопроцессора AY-3-8910.</li>
</ul>

# Переферия
<ul>
<li>PS/2 клавиатура с автоматической перекодировкой русских букв и спец.символов</li>
<li>Звук от пищалки и моно Covox на порту 177714 выводится в виде сигма-дельта модуляции (типа ШИМ).</li>
<li>Реализована эмуляция таймера (регистры 177706, 177710, 177712).</li>
</ul>

# Раскладка клавиатуры
Соответствие клавишам БК-0011М:
<ul>
<li>любой Alt - АР2</li>
<li>любой Shift - Нижний регистр</li>
<li>любой Ctrl - СУ</li>
<li>Caps Lock - Переключение ЗАГЛ / СТР</li>
<li>левый Win - РУС</li>
<li>правый Win - ЛАТ</li>
<li>Pause - СТОП</li>
<li>F1 - ПОВТ</li>
<li>F2 - КТ</li>
<li>F3 - =|=>|</li>
<li>F4 - |<==</li>
<li>F5 - |==></li>
<li>F6 - ИНД СУ</li>
<li>F7 - БЛОК РЕД</li>
<li>F8 - ШАГ</li>
<li>F9 - СБР</li>
</ul>
Клавиши джойстика (не тестировалось):
<ul>
<li>Up - Вверх (разряд 0 порта)</li>
<li>Right - Вправо (разряд 2 порта)</li>
<li>Down - Вниз (разряд 3 порта)</li>
<li>Left - Влево (разряд 4 порта)</li>
<li>Пробел - Кнопка 1 (разряд 1 порта)</li>
<li>A - Кнопка 2 (разряд 5 порта)</li>
<li>S - Кнопка 3 (разряд 6 порта)</li>
<li>D - Кнопка 4 (разряд 7 порта)</li>
<li>F - Кнопка 5 (разряд 8 порта)</li>
</ul>
Клавиши эмулятора:
<ul>
<li>Scroll Lock - Переключение режима Турбо (не работает)</li>
<li><s>Print Screen - выход в режим WiFi. Это временное решение. В дальнейшем планируется переходить в режим WiFi из меню по Esc</s></li>
<li>Esc - выход в файловый менеджер эмулятора (недоделан)</li>
<li>Num Lock - переключение клавиш в режим джойстика</li>
</ul>
