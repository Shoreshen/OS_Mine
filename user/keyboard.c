#include "keyboard.h"
#include "stdio.h"

unsigned char pausebreak_scode[]={0xE1,0x1D,0x45,0xE1,0x9D,0xC5};
int shift_l=0,shift_r=0,ctrl_l=0,ctrl_r=0,alt_l=0,alt_r=0;

unsigned char get_scancode(int fd)
{
	unsigned char ret  = 0;
	read(fd,&ret,1);
	return ret;
}

int analysis_keycode(int fd)
{
    unsigned char x = 0;
    int i, key = 0, make = 0;
    x = get_scancode(fd);
    if(x == 0xE1){
		key = PAUSEBREAK;
		for(i = 1;i<6;i++){
			if(get_scancode(fd) != pausebreak_scode[i]){
				key = 0;
				break;
			}
        }
    }
    else if(x == 0xE0){
        x = get_scancode(fd);
		switch(x)
		{
			case 0x2A: // press printscreen
				if(get_scancode(fd) == 0xE0)
					if(get_scancode(fd) == 0x37)
					{
						key = PRINTSCREEN;
						make = 1;
					}
				break;
			case 0xB7: // UNpress printscreen
				if(get_scancode(fd) == 0xE0)
					if(get_scancode(fd) == 0xAA)
					{
						key = PRINTSCREEN;
						make = 0;
					}
				break;
			case 0x1d: // press right ctrl
				ctrl_r = 1;
				key = OTHERKEY;
				break;
			case 0x9d: // UNpress right ctrl
				ctrl_r = 0;
				key = OTHERKEY;
				break;
			case 0x38: // press right alt
				alt_r = 1;
				key = OTHERKEY;
				break;
			case 0xb8: // UNpress right alt
				alt_r = 0;
				key = OTHERKEY;
				break;		
			default:
				key = OTHERKEY;
				break;
		}
    }
    if(key == 0){
		if(x & 0x80){//Bit 7 of all scan code of break is 1b
			make = 0;
		}else{
			make = 1;	
		}
			
		if(shift_l || shift_r){
			key = keycode_map_normal[(x & 0x7f)][1];
		}else{
			key = keycode_map_normal[(x & 0x7f)][0];
		}
		switch(x & 0x7F)
		{
			case 0x2a:	//SHIFT_L:
				shift_l = make;
				key = 0;
				break;
			case 0x36:	//SHIFT_R:
				shift_r = make;
				key = 0;
				break;
			case 0x1d:	//CTRL_L:
				ctrl_l = make;
				key = 0;
				break;
			case 0x38:	//ALT_L:
				alt_l = make;
				key = 0;
				break;
			case 0x01:	//ESC
				key = 0;
				break;
			case 0x0e:	//BACKSPACE
                if(make){
                    key = '\b';
                }
				break;
			case 0x0f:	//TAB
                if(make){
                    key = '\t';
                }
				break;
			case 0x1c:	//ENTER
                if(make){
                    key = '\n';
                }
				break;
			default:
				if(!make){
					key = 0;
				}
				break;
		}
        return key;
    }
    return 0;
}

int read_line(int fd, char* buf)
{
	int key = 0, count = 0;

	while(count < BUF_SIZE){
		key = analysis_keycode(fd);
		if(key == '\n'){
			return count;
		}else if(key){
			if(key == '\b'){
				if(count>0){
					buf[count] = 0;
					count--;
					printf("\b%c_", key);
				}
			}else{
				buf[count] = key;
				count ++;
				printf("\b%c_", key);
			}
		}
	}

	printf("Command is too long (greater than 256 character)!\n");
	
	return 0;
}

unsigned long* parse_cmd(char *cmd, int *arg_count)
{
	int i, st = 0, pos = 0;
	unsigned long *arg_pos;
	char *buf = NULL;

	if(strlen(cmd)){
		buf = (char *)malloc(strlen(cmd) + 2);
		strcpy(buf, cmd);
	}else{
		return NULL;
	}

	while(buf[st] == ' '){
		st++;
	}
	for(i = st; i < BUF_SIZE; i++){
		if(!buf[i]){
			break;
		}
		if(buf[i] != ' ' && (buf[i+1] == ' ' || buf[i+1] == '\0')){
			(*arg_count)++;
		}
	}
	if(!(*arg_count)){
		return 0;
	}
	arg_pos = (unsigned long*)malloc(sizeof(unsigned long) * (*arg_count));
	// printf("malloc:%p\n",arg_pos);
	i = st;	
	for(pos = 0; pos < *arg_count; pos++){
		arg_pos[pos] = (unsigned long)&buf[i];
		while(buf[i] && buf[i] != ' '){
			i++;
		}
		buf[i++] = 0;
		while(buf[i] == ' '){
			i++;
		}
		printf("%s\n",(char *)arg_pos[pos]);
	}
	return arg_pos;
}