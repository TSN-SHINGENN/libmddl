/** 
 *      Copyright 2018 TSN-SHINGENN All Rights Reserved.
 * 
 *      Basic Author: Seiichi Takeda  '2018-May-25 Active 
 *              Last Alteration $Author: takeda $
 *	Dual License :
 *	non-commercial ... MIT Licence
 *	    commercial ... Requires permission from the author
 */

/**
 * @file mddl_vsprintf.c
 * @brief オリジナル実装の軽い文字列処理ライブラリ。
 */

#if defined(WIN32)
/* Microsoft Windows Series */
#define _CRT_SECURE_NO_WARNINGS
#if _MSC_VER >= 1400            /* VC++2005 */
#pragma warning ( disable:4996 )
#pragma warning ( disable:4819 )
#endif
#endif

/* CRL */
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#ifdef __GNUC__
__attribute__ ((unused))
#endif
#ifdef DEBUG
static int debuglevel = 1;
#else
static const int debuglevel = 0;
#endif
#include "dbms.h"

/* this */
#include "mddl_vsprintf.h"

// #define USE_FLOAT_FORMAT

#define EOL_CRLF "\n\r"

#define _isnumc(x) ( (x) >= '0' && (x) <= '9' )
#define _ctoi(x)   ( (char)((int64_t)(x) - '0') )


typedef enum _enum_mddl_stdarg_integer_type {
    _IS_Ptr =  3,    // for pointer size of cpu type
    _IS_LL  =  2,    // Long Long
    _IS_Long  =  1,    // Long int
    _IS_Normal=  0,    // Integer
    _IS_Short = -1,    // Short
    _IS_Char  = -2,    // Char
    _IS_eot = INT8_MAX,  // end of enum integer type table
} enum_mddl_stdarg_integer_type_t;

typedef union union_xtoa_attr_t {
    uint8_t flags;
    struct {
	uint8_t   zero_padding:1;
	uint8_t    alternative:1;
	uint8_t thousand_group:1;
	uint8_t   str_is_lower:1;
	uint8_t with_sign_char:1;
	uint8_t left_justified:1;
	uint8_t no_assign_terminate:1;
    } f;
} union_xtoa_attr_t;


#define _prelength_is_over(max, neederslength)  ((max) <= (neederslength))

typedef int (*putchar_callback_ptr_t)(const int);

typedef struct _xtoa_output_method {
    putchar_callback_ptr_t _putchar_cb;
    size_t maxlenofstring;
    char *buf;
} xtoa_output_method_t;

static xtoa_output_method_t set_area_method(xtoa_output_method_t m, char *const buf, const size_t max_len)
{
    if(NULL == m._putchar_cb) {
	m.maxlenofstring = max_len;
	m.buf = buf;
    }
    return m;
}
#define _xtoa_output_method_initializer_set_callback_func(f, m) { (f), (m), NULL }
#define _xtoa_output_method_initializer_set_buffer( b, m) { NULL, (m), (b) }

/**
 * @fn void *get_pointer(va_list *const ap_p)
 * @brief va_list からポインタ値を取得します。
 * @return 指定可変長引数のポインタ
 **/
void *get_va_pointer(va_list *const ap_p)
{
    void *const ptr = (void*)va_arg( *ap_p,void*);
    return ptr;
}

static unsigned long long get_unsigned(va_list *const ap_p, const enum_mddl_stdarg_integer_type_t type)
{
    enum_mddl_stdarg_integer_type_t t = type;

    if(t >=  _IS_Ptr) {
	t = _IS_Ptr;
    } else if(t <= _IS_Char) {
	t = _IS_Char;
    }
    switch(t) {
        case _IS_Ptr   : return (unsigned long long)va_arg(*ap_p, uintptr_t); break;
	case _IS_LL    : return (unsigned long long)va_arg(*ap_p, unsigned long long); break;
	case _IS_Long  : return (unsigned long long)va_arg(*ap_p, unsigned long);      break;
	case _IS_Normal: return (unsigned long long)va_arg(*ap_p, unsigned int);       break;
	case _IS_Short : return (unsigned long long)va_arg(*ap_p, unsigned short); break;
	case _IS_Char  : return (unsigned long long)va_arg(*ap_p, unsigned char);  break;
	case _IS_eot: break;
    }

    return (unsigned long long) va_arg(*ap_p, unsigned int);
}


static long long int get_signed(va_list *const ap_p, const enum_mddl_stdarg_integer_type_t type)
{
    enum_mddl_stdarg_integer_type_t t = type;

    if(t >=  _IS_Ptr) {
	t = _IS_Ptr;
    } else if(t <= _IS_Char) {
	t = _IS_Char;
    }

    switch(t) {
        case _IS_Ptr   : return (long long)va_arg(*ap_p, intptr_t); break;
	case _IS_LL     : return (long long)va_arg(*ap_p, long long); break;
	case _IS_Long   : return (long long)va_arg(*ap_p, long);      break;
	case _IS_Normal : return (long long)va_arg(*ap_p, int);       break;
	case _IS_Short  : return (long long)va_arg(*ap_p, short); break;
	case _IS_Char   : return (long long)va_arg(*ap_p, char);  break;
	case _IS_eot: break;
    }

    return (unsigned long long int) va_arg(*ap_p, int);
}


static const char * const own_x2a_lower_str = "0123456789abcdefghijklmnopqrstuvwxyz";
static const char * const own_x2a_upper_str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";


#if defined(USE_FLOAT_FORMAT)
static double PRECISION = 0.000001;
/**
 * Double to ASCII
 */
static size_t ftoa(char *const buf, const size_t bufsz, const float f)
{
    char *c;
    double n;
    size_t l;

    c = buf;
    n = f;
    l = 0;

    // handle special cases
    if (isnan(n)) {
        strcpy(c, "nan");
	return 3;
    } else if (isinf(n)) {
        strcpy(c, "inf");
	return 3;
    } else if (n == 0.0) {
        strcpy(c, "0");
	return 1;
    } else {
        int digit, m, m1;
        int8_t useExp;
        uint8_t neg;
        neg = (n < 0);
        if (neg) {
            n = -n;
	}
        // calculate magnitude
        m = (int)log10(n);
        useExp = (m >= 14 || (neg && m >= 9) || m <= -9);

	if (neg) {
            *(c++) = '-';
	    l++;
	}
        // set up for scientific notation
        if (useExp) {
            if (m < 0) {
               m -= 1;
	    }
            n = n / pow(10.0, (double)m);
            m1 = m;
            m = 0;
        }
        if (m < 1) {
            m = 0;
        }
        // convert the number
        while (n > PRECISION || m >= 0) {
            double weight = pow(10.0, m);
            if (weight > 0 && !isinf(weight)) {
                digit = (int)floor(n / weight);
                n -= (digit * weight);
                *(c++) = '0' + digit;
		l++;
            }
            if (m == 0 && n > 0) {
                *(c++) = '.';
		l++;
	    }
            m--;
        }
        if (useExp) {
            // convert the exponent
            int i, j;
            *(c++) = 'e';
            if (m1 > 0) {
                *(c++) = '+';
		l++;
            } else {
                *(c++) = '-';
		l++;
                m1 = -m1;
            }
            m = 0;
            while (m1 > 0) {
                *(c++) = '0' + m1 % 10;
		l++;
                m1 /= 10;
                m++;
            }
            c -= m;
	    l -= m;
            for (i = 0, j = m-1; i<j; i++, j--) {
                // swap without temporary
                c[i] ^= c[j];
                c[j] ^= c[i];
                c[i] ^= c[j];
            }
            c += m;
	    l += m;
        }
        *(c) = '\0';
    }
    return l;
}

static int put_float(const xtoa_output_method_t *const method_p, const float f, long long int field_length, const char sign, const union_xtoa_attr_t *const attr_p)
{
    char fstr[80];
    size_t i, n;
    const putchar_callback_ptr_t _putchar_cb =  method_p->_putchar_cb;
    char *bufp = method_p->buf;

    if(!(field_length < 80) ) {
	return -1;
    }

    i = ftoa(fstr, 80, f);
    for(n=0; ( i > 0 ); ++n) {
	i--;
	if( NULL != _putchar_cb ) {
	    int result;
	    result = _putchar_cb(fstr[n]);
	    if(result<0) {
		return result;
	    }
	}
	if( NULL !=  bufp ) {
	    *bufp = fstr[n];
	    ++bufp;
	}
    }

    return (int)n;
}
#endif /* end of USE_FLOAT_FORMAT */


/**
 * @fn int static integer_to_string_with_format(const xtoa_output_method_t *const method_p, char sign, unsigned long long value, const int radix, long long int field_length, const union_xtoa_attr_t *const attr_p, int *const retlen_p)
 * @brief 引数の情報から整数値を変換してコールバックにASCIIコードで送ります。
 * @param method_p 文字出力（出力先コールバック・バッファ・最大文字長)
 * @param sign 符号(ASCIIコードで、0は符号なし）
 * @param value 文字列変換値
 * @param radix 基数
 * @param field_length パディング文字を含めたフィールド文字数
 * @param opts_p 文字変換時のオプション属性フラグポインタ
 * @param retlen_p 書き出し文字数取得用変数用ポインタ
 * @retval 0 成功
 * @retval 0以外　エラー番号
 */
static int integer_to_string_with_format(const xtoa_output_method_t *const method_p, char sign, unsigned long long value, const int radix, long long int field_length, const union_xtoa_attr_t *const attr_p, int *const retlen_p)
{
    const char *const symbols = (attr_p->f.str_is_lower) ? own_x2a_lower_str : own_x2a_upper_str;
    const size_t szoflimit = method_p->maxlenofstring;
    const putchar_callback_ptr_t _putchar_cb =  method_p->_putchar_cb;
    // const int estimate_field_length = field_length;

    char pad = ' ';
    int strpoint = 0;
    char stack_buf[64+2]; /* 64bitが表せる2進の最大数+a */
//    int lenofneeders = (attr_p->f.no_assign_terminate) ? 0 : 1;  /* \0のため */
    char *bufp = method_p->buf;
    int result;
    int retlen=0;

    if( NULL == method_p ) {
	DBMS1("%s : methos_p is NULL" EOL_CRLF, __func__);
	return EINVAL;
    }

    /* value値の文字列変換および,のスタック追加 */
    do {
	const uint8_t digi = (uint8_t)(value % radix);
	stack_buf[strpoint] = symbols[digi];
	++strpoint;
	if( (attr_p->f.thousand_group) && ((strpoint & 0x3) == 0x3)) {
	    stack_buf[strpoint] = ',';
	    ++strpoint;
	}
    } while ( value /= radix );
    if( field_length > 0 ) {
	field_length -= strpoint;
    }

    /* 右寄せの際のPadding設定 */
    if (attr_p->f.left_justified) {
        if(attr_p->f.zero_padding) {
	    pad = '0'; /* pad 変更 */
	}

        for(;field_length > 0; --field_length) {
	    stack_buf[strpoint] = pad;
	    ++strpoint;
	}
    }

    if (( sign != 0 ) && (radix == 10)) { 
	stack_buf[strpoint] = sign;
	++strpoint;
	--field_length;
    } else if (attr_p->f.alternative) {
	/* 基数ヘッドの処理 */
	if (radix == 8) {
	    stack_buf[strpoint] = '0';
	    ++strpoint;
	    --field_length;
	} else if (radix == 16) {
           stack_buf[strpoint] = 'x';
	    ++strpoint;
           stack_buf[strpoint] = '0';
	    ++strpoint;
	    field_length -= 2;
        }
    }

    if(1) {
  	const size_t total_length = (field_length > 0 ) ? (strpoint + (int)field_length) : (size_t)strpoint;
	if( retlen_p != NULL ) {
	    *retlen_p = total_length;
	}
	if((szoflimit != 0) && !( szoflimit > total_length)) {
	    errno = ENOSPC;
	    return -1;
	}
    }

    while(strpoint > 0) {
	const char c = stack_buf[--strpoint];
	if( NULL != _putchar_cb ) {
	    result = _putchar_cb(c);
	    if(result<0) {
		return result;
	    }
	}
	if( NULL !=  bufp ) {
	    *bufp = c;
	    ++bufp;
	}
	++retlen;
    }
    while(field_length > 0) {
	if( NULL != _putchar_cb ) {
	    result = _putchar_cb(pad);
	    if(result<0) {
		return result;
	    }
	}
	if( NULL !=  bufp ) {
	    *bufp = pad;
	    ++bufp;
	}
	--field_length;
	++retlen;
    }

    if(!(attr_p->f.no_assign_terminate)) {
	if( NULL !=  bufp ) {
	    *bufp = '\0';
	}
    }

    return retlen;
}


/**
 * @fn static int _own_vsnprintf(const multios_xtoa_output_method_t *const method_p, const char *const fmt, va_list ap)
 * @brief fmt に従った文字列を生成しながら pufc_cbに指定されたコールバック関数を用いて出力します
 *	max_lenを超える場合は、処理を中止して -1 を戻します。
 * @param putc_cb 処理結果の文字列を出力するためのコールバック関数
 * @param max_len 処理結果を受ける最大長(\0)を含む。 0:最大長は無視
 * @param fmt 文字列出力表記フォーマット ANSI C/POSIX を確認してね。
 * @param ap va_list 構造体
 * @retval 0以上
 * @retval -1 失敗
 **/
static int _own_vsnprintf(const xtoa_output_method_t *const method_p, const char *const fmt, va_list ap)
{
    int result;
    int lenofneeders = 0;
    const char *fptr;
    int sign;
    long long int precision;	/* 整数精度 */
    long long int length;	/* フィールド幅 */
    const putchar_callback_ptr_t _putchar_cb = method_p->_putchar_cb;
    const size_t max_len = method_p->maxlenofstring;
    char *bufp = method_p->buf;

    union_xtoa_attr_t specs = { 0 };
    enum_mddl_stdarg_integer_type_t int_type;

#if 0
    DBMS5("%s : method_p=%p _putchar_cb=%p max_len=%d bufp=%p fmt=%s" EOL_CRLF, __func__,
		    method_p, _putc_cb, max_len, bufp, fmt);
#endif

    if( NULL == method_p ) {
	errno = EINVAL;
	return -1;
    }
    // errno = 0;

    for(fptr=fmt;;++fptr) {
	/* Initialize */
	sign = 0;
	length = 0;
	precision = 0;
	specs.flags = 0;
	int_type = _IS_Normal;

	/* 先方のフォーマット指定されていない部分の内容確認 */
	if(max_len) {
	    const char *t;
	    size_t prelength;

	    for( prelength=0, t=fptr; ((*t != 0) && (*t != '%')); ++t, ++prelength); // 1 line loop
	    if( _prelength_is_over(max_len, prelength) ) {
//		DBMS5( "%s : max_len=%d prelength=%d is over" EOL_CRLF, __func__, (int)max_len,  (int)prelength);
		errno = ENOSPC;
		return -1;
	    }
//	    DBMS5( "%s : max_len=%d prelength=%d" EOL_CRLF, __func__, (int)max_len,  (int)prelength);
	}

	/* 先方のフォーマット指定されていない部分の出力 */
	for(;(*fptr != '\0') && (*fptr != '%');++fptr, ++lenofneeders) {
//	    DBMS5( "%s : lenofneeders=%d *fptr=%d bufp=0x%p" EOL_CRLF, __func__, (int)lenofneeders, (int)*fptr, bufp);
	    if( NULL != _putchar_cb) {
		result = _putchar_cb(*fptr);
		if(result<0) {
		    return result;
		}
	    }
	    if( NULL != method_p->buf) {
		*bufp = *fptr;
		++bufp;
	    }
	}
	if (*fptr == '\0') {
//	    xil_printf("%s : pre strings fini." EOL_CRLF, __func__);
		if( NULL != method_p->buf) {
		if( _prelength_is_over(max_len, (size_t)(lenofneeders)) ) {
		    errno = ENOSPC;
		    return -1;
		}
		*bufp = '\0';
		++bufp;
	    }
	    goto out;
	} else {
	    ++fptr;
	}

	if(1) { 
	    const char *const printf_flag_chars = "'-+ #0";

	    for( specs.flags = 0; (NULL != strchr( printf_flag_chars, *fptr)); ++fptr) {
		switch (*fptr) {
		    case '\'': 
			specs.f.thousand_group = 1;
			break;
		    case  '-': 
			specs.f.left_justified = 1;
			break;
		    case  '+': 
			specs.f.with_sign_char = 1;
			sign = '+';
			break;
		    case  '#': 
			specs.f.alternative = 1;
			break;
 		    case  '0': 
			specs.f.left_justified = 1;
			specs.f.zero_padding = 1;
			break;
		    case  ' ': 
			specs.f.with_sign_char = 1;
			sign = ' ';
			break;
		}
            }
	    specs.f.no_assign_terminate = 1;
	}

	if(*fptr == '*') {
	    length = get_signed( &ap, _IS_Normal);
	    ++fptr;
	} else {
	    for(;isdigit(*fptr); ++fptr) {
		length *= 10;
		length += (long long int)_ctoi(*fptr);
	    }
	}

        if (*fptr == '.') {
            ++fptr;
            if (*fptr == '*') {
		++fptr;
		precision = get_signed(&ap, _IS_Normal);
	    } else {
		while (isdigit(*fptr) ) {
		    precision *= 10;
		    precision += _ctoi(*fptr);
		    ++fptr;
		}
	    }
	}
	if(1) {
	    const char *const fmt_length_modifier="hlz";
	    const int remain = (int)(max_len - lenofneeders);
	    xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);

	    while (strchr(fmt_length_modifier,*fptr)) {
		switch (*fptr++) {
		case 'h': 
		    --int_type;
		    break;
		case 'l': 
		    ++int_type;
		    break;
		case 'z': /* size : long long     */
		    int_type=_IS_LL;
		    break;
		default: 
		    errno = EINVAL;
		    return -1;
		}
	    }

	    switch (*fptr) {
	    case 'd':
	    case 'i': {
		const long long int lli = get_signed(&ap, int_type);
		unsigned long long int ulli = 0;
		int retlen = 0;

		if (lli < 0) {
		    ulli = -lli;
		    sign = '-';
		} else {
		    ulli = (unsigned long long int)lli;
		}

		result = integer_to_string_with_format(&m, sign, ulli, 10, length, &specs, &retlen);
		if(result<0) {
		    return result;
		}
		lenofneeders += retlen;
		if(NULL != bufp) { 
		    bufp += retlen;
		}
//		xil_printf("%s : d i lenofneeders=%d retlen=%d" EOL_CRLF, __func__, lenofneeders, retlen); 
	    } break;

	    case 'u': {
//		const int remain = (int)(max_len - lenofneeders);
		const unsigned long long int ulli = get_unsigned(&ap, int_type);
//		multios_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);
		int retlen = 0;

		result = integer_to_string_with_format(&m, sign, ulli, 10, length, &specs, &retlen);
		if(result<0) {
		    return -1;
		}
		lenofneeders += retlen;
		if(NULL != bufp) { 
		    bufp += retlen;
		}
//		xil_printf("%s : u lenofneeders=%d retlen=%d" EOL_CRLF, __func__, lenofneeders, retlen); 
	    } break;

	    case 'o': {
//		const int remain = (int)(max_len - lenofneeders);
		const unsigned long long int ulli = get_unsigned(&ap, int_type);
//		multios_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);
		int retlen = 0;

		result = integer_to_string_with_format(&m, sign, ulli, 8, length, &specs, &retlen);
		if(result<0) {
		    return result;
		}
		lenofneeders += retlen;
		if(NULL != bufp) { 
		    bufp += retlen;
		}
	
//		xil_printf("%s : o lenofneeders=%d retlen=%d" EOL_CRLF, __func__, lenofneeders, retlen); 
	    } break;

	    case 'p':
		length = sizeof(uintptr_t) * 2;
		int_type = _IS_Ptr;
		specs.f.left_justified = 1;
		specs.f.zero_padding = 1;
//		specs.f.alternative = 1;
		specs.f.str_is_lower = 1;
	    case 'x': 
		specs.f.str_is_lower = 1;
	    case 'X': {
//		const int remain = (int)(max_len - lenofneeders);
		const unsigned long long int ulli = get_unsigned(&ap, int_type);
//		multios_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);
		int retlen = 0;
	    
		result = integer_to_string_with_format(&m, 0, ulli, 16, length, &specs, &retlen);
		if(result<0) {
		    return result;
		}
		lenofneeders += retlen;
		if(NULL != bufp) { 
		    bufp += retlen;
		}
//		xil_printf("%s : pxX lenofneeders=%d retlen=%d" EOL_CRLF, __func__, lenofneeders, retlen); 
	    } break;
	    case 'c': {
//		const int remain = (int)(max_len - lenofneeders);
		const unsigned long long int ulli = get_unsigned(&ap, int_type);
		const char asc = (char)(0xff & ulli);
//		multios_xtoa_output_method_t m = set_area_method( *method_p, bufp, remain);

		if((max_len != 0 ) && _prelength_is_over(max_len, ((size_t)lenofneeders+1)) ) {
		    errno = ENOSPC;
		    return -1;
		}
		if( NULL != _putchar_cb ) {
		    result = _putchar_cb(asc);
		    if(result<0) {
			return result;
		    }
		}
		++lenofneeders;
		if( NULL != method_p->buf) {
		    *bufp = asc;
		    ++bufp;
		}
	    } break;
#if defined(USE_FLOAT_FORMAT)
	    case 'f': {
		float f;
		int retlen;
                f = (float)va_arg(ap, double);
		retlen = put_float(&m, f, length, sign, &specs);
		if(retlen<0) {
		    return retlen;
		}
		lenofneeders += retlen;
	    } break;
#endif /* end of USE_FLOAT_FORMAT */
	    case 's': {
		const char *s = (char*)get_va_pointer(&ap);
		size_t tmp;

		if (NULL == s) {
		    s = "(null)";
		}
		tmp = strlen(s);

		if ( precision && ((size_t)precision < tmp) ) {
		    tmp = (size_t)precision;
		}
		length -= tmp;
		if (specs.f.left_justified /* !(flags & LEFT_JUSTIFIED) */) {
		    if( (max_len != 0 ) && (length > 0) && _prelength_is_over(max_len, (size_t)(lenofneeders+length)) ) {
			errno = ENOSPC;
			return -1;
		    } else {
			const char strpad = specs.f.zero_padding ? '0' : ' ';
			for(;length > 0; --length, ++lenofneeders ) {
			    if( NULL != _putchar_cb ) {
				result = _putchar_cb(strpad);
				if(result<0) {
				    return result;
				}
			    }
			    if( NULL != method_p->buf) {
				*bufp = strpad;
				++bufp;
			    }
			}
		    }
		}
		if( (max_len != 0) && ( tmp != 0) && _prelength_is_over(max_len, ((size_t)lenofneeders+tmp)) ) {
		    errno = ENOSPC;
		    return -1;
		} else {
		    for(;tmp; ++s, --tmp, ++lenofneeders) {
			if( NULL != _putchar_cb ) {
			    result = _putchar_cb(*s);
			    if(result<0) {
				return result;
			    }
			}
			if( NULL != method_p->buf) {
			    *bufp = *s;
			    ++bufp;
			}
		    }
		}
		if( (max_len != 0) && (length != 0) && _prelength_is_over(max_len, (size_t)(lenofneeders+length)) ) {
		    errno = ENOSPC;
		    return -1;
		} else {
		    const char strpad = ' ';
		    for(;length > 0; --length, ++lenofneeders) {
			if( NULL != _putchar_cb ) {
			    result = _putchar_cb(strpad);
			    if(result<0) {
				return result;
			    }
			}
			if( NULL != method_p->buf) {
			    *bufp = strpad;
			    ++bufp;
			}
		    }
		}
//		xil_printf("%s : s lenofneeders=%d" EOL_CRLF, __func__, lenofneeders); 
	    } break;
	    case '%': {
		const char strper = '%';
		if( (max_len != 0) && (length != 0) && _prelength_is_over(max_len, ((size_t)lenofneeders+1)) ) {
		    errno = ENOSPC;
		} else {
		    if( NULL != _putchar_cb ) {
			result = _putchar_cb(strper);
			if(result<0) {
			    return result;
			}
		    }
		    if( NULL != method_p->buf) {
			*bufp = strper;
			++bufp;
		    }
		    ++lenofneeders;
		}
            } break;
	    default: 
		if(1) {
		    errno = EINVAL;
		    return -1;
		} else {
		    for(;*fptr != '%';--fptr) {
			(void)fptr;
		    }
		}
	    }
	} 
    }
out:
    // xil_printf( " %s : function final lenofneeders=%d" EOL_CRLF, __func__,lenofneeders);

    return lenofneeders;
}

int mddl_vsnprintf(char *const buf, const size_t max_length, const char *const fmt, va_list ap)
{
    const xtoa_output_method_t _method = 
	_xtoa_output_method_initializer_set_buffer( buf, max_length);

    const int retval = _own_vsnprintf( &_method, fmt, ap);

    return retval;
}

/**
 * @fn int mddl_vsprintf(char *const buf, const char *const fmt, va_list ap)
 * @brief 可変長幕をを使ったbufの配列長を指定しない機能限定版vsprintf()
 * @param buf 生成文字列を書き出すバッファ
 * @param fmt 文字出力フォーマット
 * @param ap va_listマクロ構造体
 * @retval -1 失敗(errno参照)
 * @retval 0以上 bufに設定した文字数
 **/
int mddl_vsprintf(char *const buf, const char *const fmt, va_list ap)
{
    const xtoa_output_method_t _method = 
	_xtoa_output_method_initializer_set_buffer( buf, 0);

    const int retval = _own_vsnprintf( &_method, fmt, ap);

    return retval;
}

/**
 * @fn int mddl_vsnprintf_putchar(void (*putchar_cbfunc)(int), const size_t max_len, const char *const fmt, va_list ap)
 * @brief fmt に従った文字列を生成しながら putchar_cbfuncに指定されたコールバック関数を用いて出力します
 *	max_lenを超える場合は、処理を中止して -1 を戻します。
 * @param _putchar_cbfunc 処理結果の文字列を出力するためのコールバック関数
 * @param max_len 処理結果を受ける最大長(\0)を含む。 0:最大長は無視
 * @param fmt 文字列出力表記フォーマット ANSI C/POSIX を確認してね。
 * @param ap va_list 構造体
 * @retval 0以上 文字数？
 * @retval -1 失敗
 **/
int mddl_vsnprintf_putchar(int (*putchar_cbfunc)(int), const size_t max_len, const char *const fmt, va_list ap)
{
    const xtoa_output_method_t _method = _xtoa_output_method_initializer_set_callback_func( putchar_cbfunc, max_len);
    const int retval = _own_vsnprintf( &_method, fmt, ap);
    return retval;
}

/**
 * @fn int mddl_vsprintf_putchar(void (*putchar_cbfunc)(int), const char *const fmt, va_list ap)
 * @brief fmt に従った文字列を生成しながら _putchar_cbfuncに指定されたコールバック関数を用いて出力します。
 *	最大文字数無し。
 * @param _putchar_cbfunc 処理結果の文字列を出力するためのコールバック関数
 * @param fmt 文字列出力表記フォーマット ANSI C/POSIX を確認してね。
 * @param ap va_list 構造体
 * @retval 0以上 文字数？
 * @retval -1 失敗
 **/
int mddl_vsprintf_putchar(int (*putchar_cbfunc)(int), const char *const fmt, va_list ap)
{
    const xtoa_output_method_t _method = _xtoa_output_method_initializer_set_callback_func( putchar_cbfunc, 0);
    const int retval = _own_vsnprintf( &_method, fmt, ap);
    return retval;
}


