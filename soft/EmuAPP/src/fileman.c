#include "fileman.h"
#include "ui.h"
#include "ets.h"
#include "ps2_codes.h"
#include "str.h"
#include "xprintf.h"
#include "ffs.h"

static void OVL_SEC (fileman) del (uint16_t n)
{
    char str[64];
    xsprintf(str, "������� ���� %s?", (char *) OVL_CALL (OVL_NUM (fileman), ffs_name, n));
    if (ui_yes_no(str)==1)
    {
        OVL_CALLV (OVL_NUM (fileman), ffs_remove, n);
    }
}

static void OVL_SEC (fileman) rename(uint16_t n)
{
    const char *name;
    
again:
    name=ui_input_text("������� ����� ��� �����:", (char *) OVL_CALL (OVL_NUM (fileman), ffs_name, n), 16);
    if ( (! name) || (! name[0]) ) return;
    
    // ���� - ����� ����� ���� ��� ����
    if (OVL_CALL (OVL_NUM (fileman), ffs_find, name) >= 0)
    {
        // ��� ���� ����� ����
        ui_draw_text(0, 8, "���� � ����� ������ ��� ���� !");
        ui_sleep(2000);
        goto again;
    }
    
    // ���������������
    if (OVL_CALL (OVL_NUM (fileman), ffs_rename, n, name) < 0)
    {
        // ������
        ui_draw_text(0, 8, "������ �������������� ����� !");
        ui_sleep(2000);
    }
}

static char OVL_SEC (fileman) CvtKoi8ForCmp (char Ch)
{
    // E E E E E E E E E E E E E E E E F F F F F F F F F F F F F F F F
    // 0 1 2 3 4 5 6 7 8 9 A B C D E F 0 1 2 3 4 5 6 7 8 9 A B C D E F
    // � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � �
    static const char Tab [32] =
    {
        0xFE, 0xE0, 0xE1, 0xF6, // � � � �
        0xE4, 0xE5, 0xF4, 0xE3, // � � � �
        0xF5, 0xE8, 0xE9, 0xEA, // � � � �
        0xEB, 0xEC, 0xED, 0xEE, // � � � �
        0xEF, 0xFF, 0xF0, 0xF1, // � � � �
        0xF2, 0xF3, 0xE6, 0xE2, // � � � �
        0xFC, 0xFB, 0xE7, 0xF8, // � � � �
        0xFD, 0xF9, 0xF7, 0xFA  // � � � �
    };

    Ch = to_upper (Ch);

    if (Ch >= 0xE0) Ch = Tab [Ch - 0xE0];

    return Ch;
}

static int_fast16_t OVL_SEC (fileman) CmpNames (const char *pName1, const char *pName2)
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

static int_fast16_t OVL_SEC (fileman) GetNextFile (uint_fast8_t Order, uint_fast8_t Type, int_fast16_t iCurFile)
{
    int_fast16_t iFile;
    int_fast16_t iNextFile = -1;
    char         NextName [17];
    char         CurName  [17];

    if (iCurFile >= 0) ets_memcpy (CurName, (char *) OVL_CALL (OVL_NUM (fileman), ffs_name , (uint16_t) iCurFile), 17);

    for (iFile = 0; iFile < FAT_SIZE; iFile++)
    {
        if (fat[iFile].type == Type)
        {
            const char *pName = (char *) OVL_CALL (OVL_NUM (fileman), ffs_name , (uint16_t) iFile);

            if ((iCurFile  < 0 || ((Order == 0 && CmpNames (pName, CurName ) > 0) || (Order != 0 && CmpNames (pName, CurName ) < 0))) &&
                (iNextFile < 0 || ((Order == 0 && CmpNames (pName, NextName) < 0) || (Order != 0 && CmpNames (pName, NextName) > 0)))    )
            {
                iNextFile = iFile;
                ets_memcpy (NextName, pName, 17);
            }
        }
    }

    return iNextFile;
}

int_fast16_t OVL_SEC (fileman) fileman (uint8_t type, uint_fast8_t Mode)
{
    #define MAX_FILES   16

    uint16_t     filelist [MAX_FILES];
    int_fast16_t PrevFile = -1;
    int_fast16_t n        =  0;
    int_fast16_t prev     =  0;
    int_fast16_t n_files;

reread_all:

    // �������� ������� ������
    // ��������� ������� �� �����

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

    // ������
    ui_clear();
    ui_header_default();

    if (n_files==0)
    {
        ui_draw_text(0, 4, "��� ������ !");
        ui_sleep(2000);
        return -1;
    }

    ui_draw_text( 0,  3, Mode ? "�������� ���� ��� ��������:" : "�����:");
    ui_draw_text( 0, 23,           "ESC   - ������  DELETE - �������");
    if (Mode) ui_draw_text(0, 24,  "ENTER - �����");
    ui_draw_text(16, 24,                           "������ - ������.");
    
    #define PX  0
    #define PY  5
    #define PAGE 15
    
    // ������ ������ ������
    {
        uint_fast8_t Count;

        for (Count = 0; Count < n_files; Count++)
        {
            char str[16];
            int  y = PY + Count;
        
            // ��� �����
            ui_draw_text (PX + 3, y, (char *) OVL_CALL (OVL_NUM (fileman), ffs_name, filelist [Count]));
            // ������
            xsprintf (str, "%5d", fat [filelist [Count]].size);
            ui_draw_text (PX + 3 + 16 + 2, y, str);
        }
    }

    while (1)
    {
        // ������� ������ � ����������� �����
        ui_draw_text (PX, PY + prev, "   ");
        // ������ ������ �� ����� �����
        ui_draw_text (PX, PY + n,    "-->");
    
        // ���������� ������� �������
        prev = n;
    
        // ������������ ������� ������
        while (1)
        {
            uint_fast16_t c = OVL_CALL0 (OVL_NUM (fileman), ui_GetKey);

            if (c == KEY_MENU_UP)
            {
                // �����
                n--;
                break;
            }
            else if (c == KEY_MENU_DOWN)
            {
                // ����
                n++;
                break;
            }
            else if (c == KEY_MENU_LEFT)
            {
                // �����
                n -= PAGE;
                break;
            }
            else if (c == KEY_MENU_RIGHT)
            {
                // ������
                n += PAGE;
                break;
            }
            else if (c == KEY_MENU_DELETE)
            {
                // �������
                del (filelist [n]);

                goto reread_all;
            }
            else if (c == KEY_MENU_SPACE)
            {
                // �������������
                rename(filelist[n]);

                goto reread_all;
            }
            else if (c == KEY_MENU_ENTER && Mode)
            {
                // �������
                n = filelist [n];

                goto done;
            }
            else if (c == KEY_MENU_ESC)
            {
                // ������
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
