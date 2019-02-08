#include "Debug.h"

#ifdef DEBUG

#include "ets.h"
#include "esp8266.h"
#include "AnyMem.h"

#define ZEROPAD 1               // Pad with zero
#define SIGN    2               // Unsigned/signed long
#define PLUS    4               // Show plus
#define SPACE   8               // Space if plus
#define LEFT    16              // Left justified
#define SPECIAL 32              // 0x
#define LARGE   64              // Use 'ABCDEF' instead of 'abcdef'

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static AT_IROM const char str_lower_digits [] = "0123456789abcdefghijklmnopqrstuvwxyz";
static AT_IROM const char str_upper_digits [] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void Debug_PrintChar (char Ch)
{
    while (((ESP8266_UART0->STATUS >> 16) & 0xFF) > 125);

    ESP8266_UART0->FIFO = Ch;
}

static int str_nlen (const char *s, int maxlen)
{
    int l=0;
    for ( ; AnyMem_r_c (s) && (maxlen > 0); maxlen--, l++, s++);
    return l;
}

static int skip_atoi (const char **s)
{
    int  i = 0;
    char Ch;

    while ((Ch = AnyMem_r_c (*s), is_digit (Ch))) {i = i*10 + Ch - '0'; (*s)++;}

    return i;
}

static void number (long num, int base, int size, int precision, int type)
{
    char c, sign, tmp[66];
    const char *dig = str_lower_digits;
    int i;

    if (type & LARGE)  dig = str_upper_digits;
    if (type & LEFT) type &= ~ZEROPAD;
    if (base < 2 || base > 36) return;

    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
            }
        else if (type & PLUS) {
            sign = '+';
            size--;
            }
        else if (type & SPACE) {
            sign = ' ';
            size--;
            }
        }

    if (type & SPECIAL) {
        if (base == 16) size -= 2;
        else if (base == 8) size--;
        }

    i = 0;

    if (num == 0) tmp[i++] = '0';
    else {
        while (num != 0) {
            tmp[i++] = AnyMem_r_c (&dig[((unsigned long) num) % (unsigned) base]);
            num = ((unsigned long) num) / (unsigned) base;
            }
        }

    if (i > precision) precision = i;
    size -= precision;
    if (!(type & (ZEROPAD | LEFT))) while (size-- > 0) Debug_PrintChar (' ');
    if (sign) Debug_PrintChar (sign);

    if (type & SPECIAL) {
        if (base == 8) Debug_PrintChar ('0');
        else if (base == 16) {
            Debug_PrintChar ('0');
            Debug_PrintChar ('x');
            }
        }

    if (!(type & LEFT)) while (size-- > 0) Debug_PrintChar (c);
    while (i < precision--) Debug_PrintChar ('0');
    while (i-- > 0) Debug_PrintChar (tmp [i]);
    while (size-- > 0) Debug_PrintChar (' ');
}

void Debug_Printf (const char *fmt, ...)
{
    va_list args;
    int len;
    unsigned long num;
    int i, base;
    const char *s;
    char Ch;

    int flags;            // Flags to number()

    int field_width;      // Width of output field
    int precision;        // Min. # of digits for integers; max number of chars for from string
    int qualifier;        // 'h', 'l', or 'L' for integer fields

    va_start(args, fmt);

    for (; (Ch = AnyMem_r_c (fmt), Ch); fmt++)
    {
        if (Ch != '%')
        {
            Debug_PrintChar (Ch);
            continue;
        }

    // Process flags
        flags = 0;
repeat:
        Ch = AnyMem_r_c (++fmt); // This also skips first '%'
        switch (Ch)
        {
            case '-': flags |= LEFT; goto repeat;
            case '+': flags |= PLUS; goto repeat;
            case ' ': flags |= SPACE; goto repeat;
            case '#': flags |= SPECIAL; goto repeat;
            case '0': flags |= ZEROPAD; goto repeat;
        }

    // Get field width
        field_width = -1;
        if (is_digit (Ch))
        {
            field_width = skip_atoi (&fmt);
        }
        else if (Ch == '*')
        {
            ++fmt;
            field_width = va_arg (args, int);
            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT;
            }
        }
        Ch = AnyMem_r_c (fmt);

    // Get the precision
        precision = -1;
        if (Ch == '.')
        {
            Ch = AnyMem_r_c (++fmt);

            if (is_digit(Ch))
            {
                precision = skip_atoi (&fmt);
                Ch = AnyMem_r_c (fmt);
            }
            else if (Ch == '*')
            {
                Ch = AnyMem_r_c (++fmt);
                precision = va_arg(args, int);
            }

            if (precision < 0) precision = 0;
        }

    // Get the conversion qualifier
        qualifier = -1;
        if (Ch == 'h' || Ch == 'l' || Ch == 'L')
        {
            qualifier = Ch;
            Ch = AnyMem_r_c (++fmt);
        }

        // Default base
        base = 10;

        switch (Ch)
        {
          case 'c':
            if (!(flags & LEFT)) while (--field_width > 0) Debug_PrintChar (' ');
            Debug_PrintChar ((unsigned char) va_arg(args, int));
            while (--field_width > 0) Debug_PrintChar (' ');
            continue;

          case 's':
            {
                static AT_IROM const char str_NULL [] = "<NULL>";
                s = va_arg(args, char *);
                if (s == NULL) s = str_NULL;
                len = str_nlen(s, precision);
                if (!(flags & LEFT)) while (len < field_width--) Debug_PrintChar (' ');
                for (i = 0; i < len; ++i) Debug_PrintChar (AnyMem_r_c (s++));
                while (len < field_width--) Debug_PrintChar (' ');
            }
            continue;

          case 'p':
            if (field_width == -1)
            {
              field_width = 2 * sizeof(void *);
              flags |= ZEROPAD;
            }
            number((unsigned long) va_arg(args, void *), 16, field_width, precision, flags);
            continue;

          // Integer number formats - set up the flags and "break"
          case 'o':
            base = 8;
            break;

          case 'X':
            flags |= LARGE;
          case 'x':
            base = 16;
            break;

          case 'd':
          case 'i':
            flags |= SIGN;

          case 'u':
            break;

          default:
            if (Ch != '%') Debug_PrintChar ('%');
            if (Ch)        Debug_PrintChar (Ch);
            else --fmt;
            continue;
        }

        if (qualifier == 'l')
        {
            num = va_arg (args, unsigned long);
        }
        else if (qualifier == 'h')
        {
            if  (flags & SIGN) num = va_arg (args, int);
            else               num = va_arg (args, unsigned int);
        }
        else if (flags & SIGN) num = va_arg (args, int);
        else                   num = va_arg (args, unsigned int);

        number (num, base, field_width, precision, flags);
    }
}

#endif
