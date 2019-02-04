#include "xprintf_u.h"
#include "str_u.h"
#include "ets.h"
#include "AnyMem.h"

#define AT_OVL __attribute__((section(".ovl3_u.text")))

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

static int AT_OVL skip_atoi(const char **s)
{
    int  i = 0;
    char Ch;

    while ((Ch = AnyMem_r_c (*s), is_digit (Ch))) {i = i*10 + Ch - '0'; (*s)++;}

    return i;
}

static char * AT_OVL number(char *str, long num, int base, int size, int precision, int type)
{
    char c, sign, tmp[66];
    const char *dig = str_lower_digits;
    int i;

    if (type & LARGE)  dig = str_upper_digits;
    if (type & LEFT) type &= ~ZEROPAD;
    if (base < 2 || base > 36) return NULL;

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
    if (!(type & (ZEROPAD | LEFT))) while (size-- > 0) AnyMem_w_c (str++, ' ');
    if (sign) AnyMem_w_c (str++, sign);

    if (type & SPECIAL) {
        if (base == 8) AnyMem_w_c (str++, '0');
        else if (base == 16) {
            AnyMem_w_c (str++, '0');
            AnyMem_w_c (str++, 'x');
            }
        }

    if (!(type & LEFT)) while (size-- > 0) AnyMem_w_c (str++, c);
    while (i < precision--) AnyMem_w_c (str++, '0');
    while (i-- > 0) AnyMem_w_c (str++, tmp [i]);
    while (size-- > 0) AnyMem_w_c (str++, ' ');

    return str;
}

static char * AT_OVL eaddr(char *str, unsigned char *addr, int size, int precision, int type)
{
    char tmp[24];
    const char *dig = str_lower_digits;
    int i, len;

    if (type & LARGE)  dig = str_upper_digits;
    len = 0;
    for (i = 0; i < 6; i++)
    {
        unsigned char Ch;
        if (i != 0) tmp[len++] = ':';
        Ch = AnyMem_r_uc (&addr [i]);
        tmp[len++] = AnyMem_r_c (&dig [Ch >> 4]);
        tmp[len++] = AnyMem_r_c (&dig [Ch & 0x0F]);
    }

    if (!(type & LEFT)) while (len < size--) AnyMem_w_c (str++, ' ');
    for (i = 0; i < len; ++i) AnyMem_w_c (str++, tmp[i]);
    while (len < size--) AnyMem_w_c (str++, ' ');
    return str;
}

static char * AT_OVL iaddr(char *str, unsigned char *addr, int size, int precision, int type)
{
    char tmp[24];
    int i, len;
    unsigned int n;

    len = 0;
    for (i = 0; i < 4; i++)
    {
        if (i != 0) tmp[len++] = '.';

        n = AnyMem_r_uc (&addr[i]);

        if (n == 0)
        {
            tmp[len++] = '0';
        }
        else
        {
            if (n >= 100)
            {
                tmp[len++] = '0' + n / 100;
                n = n % 100;
                tmp[len++] = '0' + n / 10;
                n = n % 10;
            }
            else if (n >= 10)
            {
                tmp[len++] = '0' + n / 10;
                n = n % 10;
            }

            tmp[len++] = '0' + n;
        }
    }

    if (!(type & LEFT)) while (len < size--) AnyMem_w_c (str++, ' ');
    for (i = 0; i < len; ++i) AnyMem_w_c (str++, tmp[i]);
    while (len < size--) AnyMem_w_c (str++, ' ');
    return str;
}

int AT_OVL xvsprintf(char *buf, const char *fmt, va_list args)
{
    int len;
    unsigned long num;
    int i, base;
    char *str;
    const char *s;
    char Ch;

    int flags;            // Flags to number()

    int field_width;      // Width of output field
    int precision;        // Min. # of digits for integers; max number of chars for from string
    int qualifier;        // 'h', 'l', or 'L' for integer fields

    for (str = buf; (Ch = AnyMem_r_c (fmt), Ch); fmt++)
    {
        if (Ch != '%')
        {
            AnyMem_w_c (str++, Ch);
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
            if (!(flags & LEFT)) while (--field_width > 0) AnyMem_w_c (str++, ' ');
            AnyMem_w_c (str++, (unsigned char) va_arg(args, int));
            while (--field_width > 0) AnyMem_w_c (str++, ' ');
            continue;

          case 's':
            {
                static AT_IROM const char str_NULL [] = "<NULL>";
                s = va_arg(args, char *);
                if (s == NULL) s = str_NULL;
                len = str_nlen(s, precision);
                if (!(flags & LEFT)) while (len < field_width--) AnyMem_w_c (str++, ' ');
                for (i = 0; i < len; ++i) AnyMem_w_c (str++, AnyMem_r_c (s++));
                while (len < field_width--) AnyMem_w_c (str++, ' ');
            }
            continue;

          case 'p':
            if (field_width == -1)
            {
              field_width = 2 * sizeof(void *);
              flags |= ZEROPAD;
            }
            str = number(str, (unsigned long) va_arg(args, void *), 16, field_width, precision, flags);
            continue;

          case 'n':
            if (qualifier == 'l')
            {
                long *ip = va_arg(args, long *);
                *ip = (str - buf);
            }
            else
            {
                int *ip = va_arg(args, int *);
                *ip = (str - buf);
            }
            continue;

          case 'A':
            flags |= LARGE;

          case 'a':
            if (qualifier == 'l') str = eaddr(str, va_arg(args, unsigned char *), field_width, precision, flags);
            else                  str = iaddr(str, va_arg(args, unsigned char *), field_width, precision, flags);
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
            if (Ch != '%') AnyMem_w_c (str++, '%');
            if (Ch)        AnyMem_w_c (str++, Ch);
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

        str = number (str, num, base, field_width, precision, flags);
    }

    AnyMem_w_c (str, '\0');
    return str - buf;
}

int AT_OVL xsprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    int n;

    va_start(args, fmt);
    n = xvsprintf(buf, fmt, args);

    return n;
}
