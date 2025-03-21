/**
 * @file print.c
 * @author 胡博文 (@921576434@qq.com)
 * @brief component层lib库串口打印相关源文件
 * @version 2.0
 * @date 2023-10-28
 *
 * @copyright Copyright (c) 2022
 *
 * @par 修订历史
 *     <table>
 *         <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 *         <tr><td>v1.0 <td>胡博文 <td>2022-09-21 <td>增加注释
 *         <tr><td>v2.0 <td>胡博文 <td>2023-10-28 <td>解决多核打印
 */
#include <print.h>
#include <lsched.h>
#include <spinlock.h>
typedef struct params_s {
    s32 len;
    s32 num1;
    s32 num2;
    char8 pad_character;
    s32 do_padding;
    s32 left_flag;
    s32 unsigned_flag;
} params_t;
#ifdef CFG_SMP
acoral_spinlock_t print_lock;
#endif
/*---------------------------------------------------*/
/* The purpose of this routine is to output data the */
/* same as the standard printf function without the  */
/* overhead most run-time libraries involve. Usually */
/* the printf brings in many kilobytes of code and   */
/* that is unacceptable in most embedded systems.    */
/*---------------------------------------------------*/


/*---------------------------------------------------*/
/*                                                   */
/* This routine puts pad characters into the output  */
/* buffer.                                           */
/*                                                   */
static void padding( const s32 l_flag, const struct params_s *par)
{
    s32 i;

    if ((par->do_padding != 0) && (l_flag != 0) && (par->len < par->num1)) {
        i=(par->len);
        for (; i<(par->num1); i++) {
#ifdef STDOUT_BASEADDRESS
            outbyte( par->pad_character);
#endif
        }
    }
}

/*---------------------------------------------------*/
/*                                                   */
/* This routine moves a string to the output buffer  */
/* as directed by the padding and positioning flags. */
/*                                                   */
static void outs(const charptr lp, struct params_s *par)
{
    charptr LocalPtr;
    LocalPtr = lp;
    /* pad on left if needed                         */
    if(LocalPtr != NULL) {
        par->len = (s32)strlen( LocalPtr);
    }
    padding( !(par->left_flag), par);

    /* Move string to the buffer                     */
    while (((*LocalPtr) != (char8)0) && ((par->num2) != 0)) {
        (par->num2)--;
#ifdef STDOUT_BASEADDRESS
        outbyte(*LocalPtr);
#endif
        LocalPtr += 1;
}

    /* Pad on right if needed                        */
    /* CR 439175 - elided next stmt. Seemed bogus.   */
    /* par->len = strlen( lp)                      */
    padding( par->left_flag, par);
}

/*---------------------------------------------------*/
/*                                                   */
/* This routine moves a number to the output buffer  */
/* as directed by the padding and positioning flags. */
/*                                                   */

static void outnum( const s32 n, const s32 base, struct params_s *par)
{
    s32 negative;
    s32 i;
    char8 outbuf[32];
    const char8 digits[] = "0123456789ABCDEF";
    u32 num;
    for(i = 0; i<32; i++) {
    outbuf[i] = '0';
    }

    /* Check if number is negative                   */
    if ((par->unsigned_flag == 0) && (base == 10) && (n < 0L)) {
        negative = 1;
        num =(-(n));
    }
    else{
        num = n;
        negative = 0;
    }

    /* Build number (backwards) in outbuf            */
    i = 0;
    do {
        outbuf[i] = digits[(num % base)];
        i++;
        num /= base;
    } while (num > 0);

    if (negative != 0) {
        outbuf[i] = '-';
        i++;
    }

    outbuf[i] = 0;
    i--;

    /* Move the converted number to the buffer and   */
    /* add in the padding where needed.              */
    par->len = (s32)strlen(outbuf);
    padding( !(par->left_flag), par);
    while (&outbuf[i] >= outbuf) {
#ifdef STDOUT_BASEADDRESS
    outbyte( outbuf[i] );
#endif
        i--;
}
    padding( par->left_flag, par);
}
/*---------------------------------------------------*/
/*                                                   */
/* This routine gets a number from the format        */
/* string.                                           */
/*                                                   */
static s32 getnum( charptr* linep)
{
    s32 n;
    s32 ResultIsDigit = 0;
    charptr cptr;
    n = 0;
    cptr = *linep;
    if(cptr != NULL){
        ResultIsDigit = isdigit(((s32)*cptr));
    }
    while (ResultIsDigit != 0) {
        if(cptr != NULL){
            n = ((n*10) + (((s32)*cptr) - (s32)'0'));
            cptr += 1;
            if(cptr != NULL){
                ResultIsDigit = isdigit(((s32)*cptr));
            }
        }
        ResultIsDigit = isdigit(((s32)*cptr));
    }
    *linep = ((charptr )(cptr));
    return(n);
}

void smp_print( const char8 *ctrl1, ...)
{
#ifdef CFG_SMP
    static int print_init = 0;
    if(print_init == 0)
    {
        print_init = 1;
        acoral_spin_init(&print_lock);
    }
#endif
    acoral_enter_critical();
#ifdef CFG_SMP
    acoral_spin_lock(&print_lock);
#endif
    s32 Check;
#if defined (__aarch64__) || defined (__arch64__)
    s32 long_flag;
#endif
    s32 dot_flag;

    params_t par;

    char8 ch;
    va_list argp;
    char8 *ctrl = (char8 *)ctrl1;

    va_start( argp, ctrl1);

    while ((ctrl != NULL) && (*ctrl != (char8)0)) {

        /* move format string chars to buffer until a  */
        /* format control is found.                    */
        if (*ctrl != '%') {
#ifdef STDOUT_BASEADDRESS
            outbyte(*ctrl);
#endif
            ctrl += 1;
            continue;
        }

        /* initialize all the flags for this format.   */
        dot_flag = 0;
#if defined (__aarch64__) || defined (__arch64__)
        long_flag = 0;
#endif
        par.unsigned_flag = 0;
        par.left_flag = 0;
        par.do_padding = 0;
        par.pad_character = ' ';
        par.num2=32767;
        par.num1=0;
        par.len=0;

 try_next:
        if(ctrl != NULL) {
            ctrl += 1;
        }
        if(ctrl != NULL) {
            ch = *ctrl;
        }
        else {
            ch = *ctrl;
        }

        if (isdigit((s32)ch) != 0) {
            if (dot_flag != 0) {
                par.num2 = getnum(&ctrl);
            }
            else {
                if (ch == '0') {
                    par.pad_character = '0';
                }
                if(ctrl != NULL) {
            par.num1 = getnum(&ctrl);
                }
                par.do_padding = 1;
            }
            if(ctrl != NULL) {
            ctrl -= 1;
            }
            goto try_next;
        }

        switch (tolower((s32)ch)) {
            case '%':
#ifdef STDOUT_BASEADDRESS
                outbyte( '%');
#endif
                Check = 1;
                break;

            case '-':
                par.left_flag = 1;
                Check = 0;
                break;

            case '.':
                dot_flag = 1;
                Check = 0;
                break;

            case 'l':
            #if defined (__aarch64__) || defined (__arch64__)
                long_flag = 1;
            #endif
                Check = 0;
                break;

            case 'u':
                par.unsigned_flag = 1;
                /* fall through */
            case 'i':
            case 'd':
                #if defined (__aarch64__) || defined (__arch64__)
                if (long_flag != 0){
                    outnum1((s64)va_arg(argp, s64), 10L, &par);
                }
                else {
                    outnum( va_arg(argp, s32), 10L, &par);
                }
                #else
                    outnum( va_arg(argp, s32), 10L, &par);
                #endif
                Check = 1;
                break;
            case 'p':
                #if defined (__aarch64__) || defined (__arch64__)
                par.unsigned_flag = 1;
                outnum1((s64)va_arg(argp, s64), 16L, &par);
                Check = 1;
                break;
                #endif
            case 'X':
            case 'x':
                par.unsigned_flag = 1;
                #if defined (__aarch64__) || defined (__arch64__)
                if (long_flag != 0) {
                    outnum1((s64)va_arg(argp, s64), 16L, &par);
                }
                else {
                    outnum((s32)va_arg(argp, s32), 16L, &par);
                }
                #else
                outnum((s32)va_arg(argp, s32), 16L, &par);
                #endif
                Check = 1;
                break;

            case 's':
                outs( va_arg( argp, char *), &par);
                Check = 1;
                break;

            case 'c':
#ifdef STDOUT_BASEADDRESS
                outbyte( va_arg( argp, s32));
#endif
                Check = 1;
                break;

            case '\\':
                switch (*ctrl) {
                    case 'a':
#ifdef STDOUT_BASEADDRESS
                        outbyte( ((char8)0x07));
#endif
                        break;
                    case 'h':
#ifdef STDOUT_BASEADDRESS
                        outbyte( ((char8)0x08));
#endif
                        break;
                    case 'r':
#ifdef STDOUT_BASEADDRESS
                        outbyte( ((char8)0x0D));
#endif
                        break;
                    case 'n':
#ifdef STDOUT_BASEADDRESS
                        outbyte( ((char8)0x0D));
                        outbyte( ((char8)0x0A));
#endif
                        break;
                    default:
#ifdef STDOUT_BASEADDRESS
                        outbyte( *ctrl);
#endif
                        break;
                }
                ctrl += 1;
                Check = 0;
                break;

            default:
        Check = 1;
        break;
        }
        if(Check == 1) {
            if(ctrl != NULL) {
                ctrl += 1;
            }
                continue;
        }
        goto try_next;
    }
    va_end( argp);
#ifdef CFG_SMP
    acoral_spin_unlock(&print_lock);
#endif
    acoral_exit_critical();
}
