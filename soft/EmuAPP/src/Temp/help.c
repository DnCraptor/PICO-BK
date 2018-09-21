#include "help.h"

#include "ets.h"
#include "vg75.h"
#include "ui.h"
#include "ps2.h"
#include "ps2_codes.h"


#define HELP_AT	0x50000


static int n_lines=-1, line=0;


static void display(void)
{
    int addr=0;
    
    // ������ ����� ������ ������� ������
    if (line != 0)
    {
	uint16_t tmp[2];
	SPIRead(HELP_AT+(line & 0xFFFE)*2, (uint32_t*)tmp, 4);	// ������ ������ ���� ��������� �� ������� 4 ����
	addr=tmp[line & 1];
    }
    
    // ��������� �������� ������� ����� � ����� ������ �� �����
    addr+=n_lines*2 + HELP_AT;
    //ets_printf("HELP: display start 0x%05X\n", addr);
    
    // ������ ������
    int i;
    for (i=0; i<30; i++)
    {
	// ��������� �� ����� ������
	if (line+i >= n_lines) break;
	
	//ets_printf("HELP: line %d at 0x%05X\n", line+i, addr);
	
	// ������ ����� � �����
	uint8_t buf[80];	// ����. ����� - 72 ������� + �� ������������ �� ������� 4 ����
	SPIRead(addr & ~0x03, (uint32_t*)buf, sizeof(buf));
	uint8_t o=addr & 0x03;
	uint8_t x=8;
	while (buf[o])
	{
	    ui_scr[4+i][x++]=buf[o++];
	    addr++;
	}
	
	// ������� ������� ������
	while (x < 80)
	    ui_scr[4+i][x++]=0;
	
	addr++;	// ����� ������
    }
}


void help_display(void)
{
    ui_start();
    
    // ������ ���-�� ����� (���� �� ������ ���)
    if (n_lines < 0)
    {
	uint16_t tmp[2];
	
	SPIRead(HELP_AT, (uint32_t*)tmp, 4);
	n_lines=tmp[0];
	//ets_printf("HELP: n_lines=%d\n", n_lines);
    }
    
draw:
    // ������ �� ������
    display();
    
    // ������������ ������
    while (1)
    {
	uint16_t c=ps2_read();
	
	switch (c)
	{
	    case PS2_ESC:
	    case PS2_MENU:
		// �����
		goto done;
	    
	    case PS2_UP:
		// �����
		if (line > 0)
		{
		    line--;
		    goto draw;
		}
		break;
	    
	    case PS2_DOWN:
		// ����
		if (line+30 < n_lines)
		{
		    line++;
		    goto draw;
		}
		break;
	    
	    case PS2_PGUP:
		// �������� �����
		if (line > 0)
		{
		    line-=29;
		    if (line < 0) line=0;
		    goto draw;
		}
		break;
	    
	    case PS2_PGDN:
		// �������� ����
		if (line+30 < n_lines)
		{
		    line+=29;
		    if (line+30 >= n_lines)
			line=n_lines-30;
		    goto draw;
		}
		break;
	    
	    case PS2_HOME:
		// ������
		if (line > 0)
		{
		    line=0;
		    goto draw;
		}
		break;
	    
	    case PS2_END:
		// �����
		if (line+30 < n_lines)
		{
		    line=n_lines-30;
		    goto draw;
		}
		break;
	}
    }
    
done:
    ui_stop();
}
