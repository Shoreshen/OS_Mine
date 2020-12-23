#include"printf.h"

char *DigitsCptl = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *DigitsSmal = "0123456789abcdefghijklmnopqrstuvwxyz";

char TempStr[50];

int skip_atoi(char **s) //Passing in pointer => fmt, so that the increasing will have affect on fmt
{
    int val = 0;
    while(is_digit(**s)){
        val = val * 10 + *((*s)++) - '0';
    }
    return val;
}

char* number(char *str, long num, int base, int field_width, int precision, int type)
{
    char *TempDgt = DigitsCptl, c, sign;
    int i;

 
    #pragma region Pre-Operation                                 
    if(type & SMALL){ TempDgt = DigitsSmal; }                       //Determine Lower-Case/Capital index
    if(type & LEFT){ type = type & (~ZEROPAD); }                    //Clear ZEROPAD bit, if LEFT bit present.
    if(base < 2 || base > 36){ return 0; }                          //Check validation of base
    if(type & ZEROPAD){ c = '0'; } else{ c = ' '; }                 //Check ZEROPAD bit, determining pre-fix char c
    //Check & set sign
    sign = 0;
    if((type & SIGN) && (num < 0)){
        sign = '-';
        num = -num;
    }
    else{
        if(type & PLUS){
            sign = '+';
        }
        else if(type & SPACE){
            sign = ' ';
        }
    }
    //Adjust field_width
    if(sign){ field_width--; }
    if(type & SPECIAL){
        if(base == 8) { field_width--; }
    }
    //Calculate reverse string
    i = 0;
    if( num == 0){ 
        TempStr[i++] = '0';
    }
    else{
        while(num != 0){
            TempStr[i++]=TempDgt[do_div(num,base)];
        }
    }
    //Adjust precision
    if(i > precision){ precision = i; }
    field_width = field_width - precision;
    #pragma endregion

    #pragma region Filling string
    if(!(type & (ZEROPAD + LEFT))){ while(field_width-- > 0){ *str++ = ' '; } } //Filling left side space
    if(sign){ *str++ = sign; }                                                  //Filling sign
    //Filling special symbol for OCT or HEX
    if(type & SPECIAL){
        if(base == 8){
            *str++ = '0';
        }
        else if(base == 16){
            *str++ = '0';
            *str++ = 'x';
        }
    }
    if (!(type & LEFT)) { while(field_width-- > 0){ *str++ = c; } }             //Filling prefix
    while(i < precision--){ *str++ = '0'; }                                     //Filling '0' to meet requirement on precision
    while(i-- > 0){ *str++ = TempStr[i];  }                                     //Filling string
    while(field_width-- > 0){ *str++ = ' '; }                                   //Filling tail space
    #pragma endregion

    return str;
}

int vsprintf(char * buf, char *fmt, va_list args)
{
    char *str = buf, *tempstr;
    int  flag, field_width, precision, qualifier, len, i;

    for(; *fmt; fmt++){
        if(*fmt != '%'){
            *str = *fmt;
            str ++;
        }
        else{
            #pragma region Read flag
            flag = 0;
            repeat:
            fmt ++;
            switch (*fmt)
            {
                case '-': flag |= LEFT;
                    goto repeat;
                case '+': flag |= PLUS;
                    goto repeat;
                case ' ': flag |= SPACE;
                    goto repeat;
                case '#': flag |= SPECIAL;
                    goto repeat;
                case '0': flag |= ZEROPAD;
                    goto repeat;
            }
            #pragma endregion

            #pragma region Get field_width
            //field_width = The minimal length of the string, if not meet, printing extra space.
            field_width = -1;
            if(is_digit(*fmt)){
                field_width = skip_atoi(&fmt);
            }
            else if(*fmt == '*'){
                fmt ++;
                field_width = va_arg(args, int);
                if(field_width < 0){
                    field_width = -field_width;
                    flag |= LEFT;
                }
            }
            #pragma endregion

            #pragma region Get precision
            //precision for s = The maximum length of the string, if exceed, cut the rest.
            precision = -1;
            if(*fmt == '.'){
                fmt ++;
                if(is_digit(*fmt)){
                    precision = skip_atoi(&fmt);
                }
                else if(*fmt == '*'){
                    fmt ++;
                    precision = va_arg(args, int);
                }
            }
            #pragma endregion

            #pragma region Qualification
            qualifier = -1;
            if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'z'){
                qualifier = *fmt;
                fmt++;
            }
            #pragma endregion

            #pragma region Substitute '%'
            switch (*fmt)
            {
                case 'c':
                    if(!(flag & LEFT)){
                        while(field_width > 1){
                            *str = ' ';
                            str++;
                            field_width --;
                        }
                    }
                    *str = (unsigned char) va_arg(args, int);
                    str++;
                    while(field_width > 1){
                        *str = ' ';
                        str++;
                        field_width --;
                    }
                    break;
                case 's':
                    //Calculate length of string
                    tempstr = va_arg(args, char *);
                    len = strlen(tempstr);
                    if(precision < 0){
                        precision = len;
                    }
                    else if(len > precision){
                        len = precision;
                    }
                    //Print string
                    if(!(flag & LEFT)){
                        while(field_width > len){
                            *str = ' ';
                            str++;
                            field_width --;
                        }
                    }
                    for(i = 0; i < len; i++){
                        *str = *tempstr;
                        str++;
                        tempstr++;
                    }
                    while(field_width > len){
                        *str = ' ';
                        str++;
                        field_width --;
                    }
                    break;
                case 'o':
                    if(qualifier == 'l'){
                        str = number(str, va_arg(args, unsigned long), 8, field_width, precision, flag);
                    }
                    else{
                        str = number(str, va_arg(args, unsigned int), 8, field_width, precision, flag);
                    }
                    break;
                case 'p':
                    flag |= SPECIAL;
                    if(field_width == -1){
                        field_width = 2 * sizeof(void *);
						flag |= ZEROPAD;
                    }
                    str = number(str, (unsigned long)va_arg(args,void *), 16, field_width, precision, flag);
                    break;
                case 'x':
                    flag = flag | SMALL;
                case 'X':
                    if(qualifier == 'l'){
                        str = number(str, va_arg(args,unsigned long), 16, field_width, precision, flag);
                    }
					else{
                        str = number(str, va_arg(args,unsigned int), 16, field_width, precision, flag);
                    }
					break;
                case 'd':
                    flag |= SIGN;
                	if(qualifier == 'l'){
                        str = number(str, va_arg(args,long), 10, field_width, precision, flag);
                    }
					else{
                        str = number(str, va_arg(args,int), 10, field_width, precision, flag);
                    }
					break;
                case 'i':
                    flag = flag | SIGN;
                case 'u':
					if(qualifier == 'l'){
                        str = number(str, va_arg(args,unsigned long), 10, field_width, precision, flag);
                    }
					else{
                        str = number(str, va_arg(args,unsigned int), 10, field_width, precision, flag);
                    }
					break;
                case 'n':
					if(qualifier == 'l')
					{
						long *ip = va_arg(args,long *);
						*ip = (str - buf);
					}
					else
					{
						int *ip = va_arg(args,int *);
						*ip = (str - buf);
					}
					break;
				case '%':
					*str++ = '%';
					break;
                default:
					*str++ = '%';	
					if(*fmt){
						*str++ = *fmt;
                    }
					else{
                        fmt--;          //Adjust fmt pointer if string end with '%'
                    }
					break;
            }
            #pragma endregion
        }
    }
    *str = '\0';
	return str - buf;
}

int printf(char *fmt, ...)
{
	char buf[1000];
	int count = 0;
    va_list args;

	va_start(args, fmt);
	count = vsprintf(buf, fmt, args);
	va_end(args);
	putstring(buf);

	return count;
}