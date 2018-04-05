#include "ets.h"

#include "tv.h"
#include "vg75.h"
#include "vv55_i.h"
#include "ps2.h"
#include "keymap.h"
#include "i8080.h"
#include "i8080_hal.h"
#include "tape.h"
#include "timer0.h"
#include "ui.h"
#include "menu.h"
#include "ffs.h"
#include "zkg.h"
#include "reboot.h"
#include "board.h"


void main_program(void)
{
    // ������ �������� �������
    ffs_init();
    
    // ������ ���������
    i8080_hal_init();
    i8080_init();
    i8080_jump(0xF800);
    
    // ������ �����
    tv_init();
    vg75_init((uint8_t*)i8080_hal_memory());
    tv_start();
    
    // ������ ����������
    kbd_init();
    ps2_init();
    keymap_init();
    
    // ������ ����������
    tape_init();
    
    // ��������� ��������
    uint32_t prev_T=getCycleCount();
    uint32_t sec_T=prev_T;
    uint32_t cycles=0, sec_cycles=0;
    bool turbo=false, win=false;
    while (1)
    {
        uint32_t T=getCycleCount();
        int32_t dT=T-prev_T;
	
        if ( (dT > 0) || (turbo) )
        {
            // ����� ��������� �������� �����
            uint8_t n=turbo ? 200 : 20;
            while (n--)
            {
        	uint16_t c=i8080_instruction();
                cycles+=c;
                i8080_cycles+=c;
            }
	    
            if (! turbo)
        	prev_T+=cycles*90; else
        	prev_T=T;
            sec_cycles+=cycles;
            cycles=0;
        }
	
        if ( ((uint32_t)(T-sec_T)) >= 160000000)
        {
            // ������ �������
            ets_printf("Speed=%d rtc=0x%08x\n", (int)sec_cycles, READ_PERI_REG(0x60001200));
            //kbd_dump();
            sec_cycles=0;
            sec_T=T;
        }
	
	// ��� ���������
	
	if (tape_periodic())
	{
	    // ��������� ������ �� ���������� - ���� ���������� ��������� ����
	    ui_start();
		tape_save();
	    ui_stop();
	    
	    // ���������� ����� ������
	    sec_T=prev_T=getCycleCount();
	    sec_cycles=0;
	}
	
	if (win)
	{
	    // Win ������ - ������������ ����-�������
	    uint16_t c=ps2_read();
	    switch (c)
	    {
		case PS2_LEFT:
		    // ����� �����
		    if (screen.x_offset > 0) screen.x_offset--;
		    break;
		    
		case PS2_RIGHT:
		    // ����� ������
		    if (screen.x_offset < 16) screen.x_offset++;
		    break;
		    
		case PS2_UP:
		    // ����� �����
		    if (screen.y_offset > 8) screen.y_offset-=8; else screen.y_offset=0;
		    break;
		    
		case PS2_DOWN:
		    // ����� ����
		    if (screen.y_offset < 8*8) screen.y_offset+=8;
		    break;
		    
		case PS2_L_WIN | 0x8000:
		case PS2_R_WIN | 0x8000:
		    // ������ Win
		    win=false;
		    break;
	    }
	} else
	{
	    // Win �� ������
	    uint16_t c;
	    bool rst=false;
	    
    	    ps2_leds(kbd_rus(), false, turbo);
    	    ps2_periodic();
    	    c=keymap_periodic();
    	    switch (c)
    	    {
    		case 0:
    		    break;
    		
    		case PS2_ESC:
    		    // ����
		    ui_start();
			menu();
		    ui_stop();
		    rst=true;
		    break;
		
		case PS2_F5:
		    // �������� ��������
		    ui_start();
			menu_fileman();
		    ui_stop();
		    rst=true;
		    break;
		
		case PS2_F9:
		    // ����� � �������
		    i8080_jump(0xF800);
		    break;
		
		case PS2_F10:
		    // �����
        	    ets_memset(i8080_hal_memory(), 0x00, 0x8000);
        	    i8080_init();
        	    i8080_jump(0xF800);
        	    ets_memcpy(zkg, zkg_rom, 1024);
        	    break;
		
		case PS2_F12:
		    // WiFi
		    reboot(0x55AA55AA);
		    break;
		
		case PS2_SCROLL:
		    // ������������� �����
		    turbo=!turbo;
		    break;
		
		case PS2_L_WIN:
		case PS2_R_WIN:
		    // ������ Win
		    win=true;
		    break;
		
		/*case PS2_F12:
		    // ���� ������
		    {
			ets_printf("VRAM w=%d h=%d:\n", screen.screen_w, screen.screen_h);
			int i,j;
			uint8_t *vram=screen.vram;
			for (i=0; i<screen.screen_h; i++)
			{
			    ets_printf("%2d: ", i);
			    for (j=0; j<screen.screen_w; j++)
			    {
				ets_printf(" %02X", *vram++);
			    }
			    ets_printf("\n");
			}
		    }
		    break;*/
		
		default:
		    ets_printf("PS2: %04X\n", c);
		    break;
    	    }
    	    
    	    if (rst)
    	    {
	        // ���������� ����� ������
		sec_T=prev_T=getCycleCount();
		sec_cycles=0;
	    }
    	}
    }
}