
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

//int clear=0;
int color=1;
unsigned int last_cursor[100][100]={0};
unsigned int size[100]={0};
unsigned int count=0;
unsigned int lastTab[100]={0};
unsigned int tabSize=0;
int tab=0;

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);

PUBLIC void set_color(int i){
    if(i>=1&&i<=5)
        color=i;
}

PUBLIC void set_Tab(){
    tab=1;
}

PRIVATE int isTab(CONSOLE* p_con){
     if(p_con->cursor==lastTab[tabSize-1]){
            return 1;
     }
     return 0;
}



/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}
       
	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}

/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
        
        /*if(clear){
               p_con->cursor=p_con->original_addr;
               clear=0;
        }*/

         u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
       // last_cursor[0]=p_con->original_addr;
        int i=0;
        for(i;i<count;i++){
             if(last_cursor[i][0]==p_con->original_addr)
                     break;
        }
        if(i==count){
             int j=0;
             for(;j<100;j++){
                    last_cursor[i][j]=0;
             }
             size[i]=0;
             last_cursor[i][0]=p_con->original_addr;
             count++;
        }

/*
	unsigned int	current_start_addr;	 当前显示到了什么位置	  
	unsigned int	original_addr;		 当前控制台对应显存位置 
	unsigned int	v_mem_limit;		 当前控制台占的显存大小 
	unsigned int	cursor;			 当前光标位置 
*/
        //last_cursor[i][++size[i]]=p_con->cursor;

        // disp_str(shit+'0');
       // last_cursor[i][shit]=i;

        int j=0;
	switch(ch) {
	case '\n':
		if (p_con->cursor < p_con->original_addr +
		    p_con->v_mem_limit - SCREEN_WIDTH) {
                       last_cursor[i][++size[i]]=p_con->cursor;
			p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
				((p_con->cursor - p_con->original_addr) /
				 SCREEN_WIDTH + 1);
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr) {
                        if(isTab(p_con)){
                            p_con->cursor=p_con->cursor-4;
                            *(p_vmem-2) = ' ';
			    *(p_vmem-1) = DEFAULT_CHAR_COLOR;
                            tabSize--;
                        }
                        else{ 
                          if((p_con->cursor-p_con->original_addr)%SCREEN_WIDTH==0){
                              if(size[i]>=1&&size[i]<=99){
                                     p_con->cursor=last_cursor[i][size[i]--];
                               }else
                                  p_con->cursor=last_cursor[i][0];
                              
                           }else
			       p_con->cursor--;
                        
			*(p_vmem-2) = ' ';
			*(p_vmem-1) = DEFAULT_CHAR_COLOR;
                        }
		}
		break;
        case '\t':
                   p_con->cursor+=4;
                   lastTab[tabSize]=p_con->cursor;
                   tabSize++;
               break;
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
                        //*p_vmem++ = DEFAULT_CHAR_COLOR;
                        if(color==2){
			   *p_vmem++ = F2_COLOR; 
                         }else if(color==3){
			   *p_vmem++ = F3_COLOR; //blue
                         }else if(color==4){
			   *p_vmem++ = F4_COLOR;                            
                         }else if(color==5){//green
			   *p_vmem++ = F5_COLOR;                          
                         }else
			   *p_vmem++=DEFAULT_CHAR_COLOR;   
                        //last_cursor= p_con->cursor;                     
                        p_con->cursor++;
		}
		break;
	}

       /* if(tab){
              lastTab[tabSize]=p_con->cursor;
              tabSize++;
              tab=0;
        }*/

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

