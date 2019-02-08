#include "ets.h"
#include "AnyMem.h"
#include "ps2_codes.h"
#include "ovl.h"
#include "ffs.h"

#include "fileman_u.h"
#include "ui_u.h"
#include "str_u.h"
#include "xprintf_u.h"

#include "../FfsUi/ffs_fu.h"
#include "../FfsUi/str_fu.h"

#include "Debug.h"

#define AT_OVL __attribute__((section(".ovl3_u.text")))

static void AT_OVL del (uint16_t n)
{
    AT_IROM static const char str_DelFile [] = "Удалить файл %s?";
    char str [64];

    xsprintf (str, str_DelFile, ffs_name (n));

    if (ui_yes_no (str) == 1)
    {
        ui_Suspend ();

        OVL_CALL (ffs_remove, n);

        ui_Resume ();
    }
}

static void AT_OVL rename (uint16_t n)
{
    AT_IROM static const char str_EnterNewFileName [] = "Введите новое имя файла:";
    const char *pInputText;
    char Name [16 + 1];
    int_fast16_t Res;

again:

    pInputText = ui_input_text (str_EnterNewFileName, ffs_name (n), 16);

    if (pInputText == NULL) return;

    AnyMem_strcpy (Name, pInputText);

    if (Name [0] == 0) return;

    ui_Suspend ();

    // Ищем - вдруг такой файл уже есть
    if (OVL_CALL (ffs_find, &Name [0]) >= 0)
    {
        AT_IROM static const char str_FileExist [] = "Файл с таким именем уже есть !";
        // Уже есть такой файл

        ui_Resume ();

        ui_draw_text (0, 8, str_FileExist);
        ui_sleep     (2000);

        goto again;
    }
    
    // Переименовываем
    Res = OVL_CALL (ffs_rename, n, &Name [0]);

    ui_Resume ();

    if (Res < 0)
    {
        AT_IROM static const char str_RenameFileError [] = "Ошибка переименования файла !";
        // Ошибка
        ui_draw_text (0, 8, str_RenameFileError);
        ui_sleep     (2000);
    }
}

static char AT_OVL CvtKoi8ForCmp (char Ch)
{
    // E E E E E E E E E E E E E E E E F F F F F F F F F F F F F F F F
    // 0 1 2 3 4 5 6 7 8 9 A B C D E F 0 1 2 3 4 5 6 7 8 9 A B C D E F
    // А Б В Г Д Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я
    AT_IROM static const char Tab [32] =
    {
        0xFE, 0xE0, 0xE1, 0xF6, // Ю А Б Ц
        0xE4, 0xE5, 0xF4, 0xE3, // Д Е Ф Г
        0xF5, 0xE8, 0xE9, 0xEA, // Х И Й К
        0xEB, 0xEC, 0xED, 0xEE, // Л М Н О
        0xEF, 0xFF, 0xF0, 0xF1, // П Я Р С
        0xF2, 0xF3, 0xE6, 0xE2, // Т У Ж В
        0xFC, 0xFB, 0xE7, 0xF8, // Ь Ы З Ш
        0xFD, 0xF9, 0xF7, 0xFA  // Э Щ Ч Ъ
    };

    Ch = to_upper (Ch);

    if (Ch >= 0xE0) Ch = AnyMem_r_c (&Tab [Ch - 0xE0]);

    return Ch;
}

static int_fast16_t AT_OVL CmpNames (const char *pName1, const char *pName2)
{
    char Ch1;
    char Ch2;

    do
    {
        Ch1 = CvtKoi8ForCmp (*pName1++);
        Ch2 = CvtKoi8ForCmp (*pName2++);

        if      (Ch1 > Ch2) return  1;
        else if (Ch1 < Ch2) return -1;
    }
    while (Ch1);

    return 0;
}

static int_fast16_t AT_OVL GetNextFile (uint_fast8_t Order, uint_fast8_t Type, int_fast16_t iCurFile)
{
    int_fast16_t iFile;
    int_fast16_t iNextFile = -1;
    char         NextName [17];
    char         CurName  [17];

    if (iCurFile >= 0) AnyMem_memcpy (CurName, ffs_name (iCurFile), 17);

    for (iFile = 0; iFile < FAT_SIZE; iFile++)
    {
        ffs_GetFile (iFile);

        if (ffs_File.type == Type)
        {
            const char *pName = ffs_name (iFile);

            if ((iCurFile  < 0 || ((Order == 0 && CmpNames (pName, CurName ) > 0) || (Order != 0 && CmpNames (pName, CurName ) < 0))) &&
                (iNextFile < 0 || ((Order == 0 && CmpNames (pName, NextName) < 0) || (Order != 0 && CmpNames (pName, NextName) > 0)))    )
            {
                iNextFile = iFile;
                AnyMem_memcpy (NextName, pName, 17);
            }
        }
    }

    return iNextFile;
}

int_fast16_t AT_OVL fileman (uint8_t type, uint_fast8_t Mode)
{
    #define MAX_FILES   16

    uint16_t     filelist [MAX_FILES];
    int_fast16_t PrevFile = -1;
    int_fast16_t n        =  0;
    int_fast16_t prev     =  0;
    int_fast16_t n_files;

reread_all:

    // Собираем каталог файлов
    // Сортируем каталог по имени

    {
        int_fast16_t File = PrevFile;

        n_files = 0;

        do
        {
            File = GetNextFile (0, type, File);

            if (File < 0) break;

            filelist [n_files++] = (uint16_t) File;
        }
        while (n_files < MAX_FILES);
    }

reread_up:
reread_down:

    while ((n_files < MAX_FILES || n < 0) && PrevFile >= 0)
    {
        uint_fast8_t Count;

        for (Count = MAX_FILES - 1; Count; Count--) filelist [Count] = filelist [Count - 1];

        filelist [0] = (uint16_t) PrevFile;
        PrevFile     = GetNextFile (1, type, PrevFile);

        n_files++;
        n++;
    }

    if (n_files > MAX_FILES) n_files = MAX_FILES;

    while (n_files >= MAX_FILES && n >= MAX_FILES)
    {
        uint_fast8_t Count;
        int_fast16_t File = GetNextFile (0, type, filelist [MAX_FILES - 1]);

        if (File < 0) break;

        PrevFile = filelist [0];

        for (Count = 0; Count < MAX_FILES - 1; Count++) filelist [Count] = filelist [Count + 1];

        filelist [Count] = (uint16_t) File;

        n--;
    }

    if (n > n_files - 1) n = n_files - 1;
    if (n <           0) n = 0;

    // Рисуем
    ui_clear();
    ui_header_default();

    if (n_files==0)
    {
        AT_IROM static const char str_NoFiles [] = "Нет файлов !";

        ui_draw_text(0, 4, str_NoFiles);
        ui_sleep(2000);
        return -1;
    }

    {
        AT_IROM static const char str_SelecFileForLoad [] = "Выберите файл для загрузки:";
        AT_IROM static const char str_Files            [] = "Файлы:";
        AT_IROM static const char str_EscDelete        [] = "ESC   - отмена  DELETE - удалить";
        AT_IROM static const char str_Enter            [] = "ENTER - выбор";
        AT_IROM static const char str_Button           [] = "ПРОБЕЛ - переим.";

        ui_draw_text( 0,  3, Mode ? str_SelecFileForLoad : str_Files);
        ui_draw_text( 0, 23, str_EscDelete);
        if (Mode) ui_draw_text(0, 24, str_Enter);
        ui_draw_text(16, 24, str_Button);
    }
    
    #define PX  0
    #define PY  5
    #define PAGE 15
    
    // Рисуем список файлов
    {
        uint_fast8_t Count;

        for (Count = 0; Count < n_files; Count++)
        {
            AT_IROM static const char str_5d [] = "%5d";
            char str[16];
            int  y = PY + Count;
        
            // Имя файла
            ui_draw_text (PX + 3, y, ffs_name (filelist [Count]));
            // Размер
            ffs_GetFile (filelist [Count]);
            xsprintf (str, str_5d, ffs_File.size);
            ui_draw_text (PX + 3 + 16 + 2, y, str);
        }
    }

    while (1)
    {
        // Стираем курсор с предыдущего файла
        ui_draw_text (PX, PY + prev, "   ");
        // Рисуем курсор на новом месте
        ui_draw_text (PX, PY + n,    "-->");
    
        // Запоминаем текущую позицию
        prev = n;
    
        // Обрабатываем нажатия кнопок
        while (1)
        {
            uint_fast16_t c = ui_GetKey ();

            if (c == KEY_MENU_UP)
            {
                // Вверх
                n--;
                break;
            }
            else if (c == KEY_MENU_DOWN)
            {
                // Вниз
                n++;
                break;
            }
            else if (c == KEY_MENU_LEFT)
            {
                // Влево
                n -= PAGE;
                break;
            }
            else if (c == KEY_MENU_RIGHT)
            {
                // Вправо
                n += PAGE;
                break;
            }
            else if (c == KEY_MENU_DELETE)
            {
                // Удалить
                del (filelist [n]);

                goto reread_all;
            }
            else if (c == KEY_MENU_SPACE)
            {
                // Переименовать
                rename (filelist [n]);

                goto reread_all;
            }
            else if (c == KEY_MENU_ENTER && Mode)
            {
                // Выбрать
                n = filelist [n];

                goto done;
            }
            else if (c == KEY_MENU_ESC)
            {
                // Отмена
                n = -1;

                goto done;
            }
        }

        if (n <           0) goto reread_up;
        if (n > n_files - 1) goto reread_down;
    }
    
done:
    
    return n;
}
