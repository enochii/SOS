// #include "stdlib.h"
// #include "stdarg.h"
// #include "stdio.h"

// #include "string.h"
// #include <ctype.h>
// #include <errno.h> //for returning error codes to compare with test_strtol
#include <limits.h> //for LONG_MAX & LONG_MIN
#include "stdbool.h"

#include "type.h"
#include "stdio.h"
#include "const.h"


#define EOS_MATCHER_CHAR	'\f'

static int n_isspace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int islower(char c)
{
  return c>='a'&&c<='z';
}

static int isdigit(char c)
{
  return c>='0'&&c<='9';
}

static int isalpha(char c)
{
  return islower(c)|| (c>='A'&&c<='Z');
}

static long _strtol (const char *nPtr, char **endPtr, int base);

int nscanf(const char *str, const char *fmt, va_list arg)
{
    int cSuccess = 0;
    const char *rp = str;
    const char *fp = fmt;
    // va_list ap;
    char *ep;
    char fc;
    long v;
    
    // va_start(ap, fmt);
    // va_list arg = (va_list)((char*)(&fmt) + 4);
    va_list p_next_arg=arg;

    while(*rp && *fp){
      fc = *fp;
#ifdef EOS_MATCHER_CHAR
      if(fc == EOS_MATCHER_CHAR)
        break;
#endif
      if(n_isspace(fc)){
        /* do nothing */
      } else if(fc != '%'){
        while(n_isspace(*rp)) rp++;
        if(*rp == 0)
          break;
        else if(fc != *rp) 
          break;
        else
          rp++;
      } else {  /* fc == '%' */
      //attention: we should use operator twice, different from printf
      // first get the pointer given by user
      //then access it
        fc = *++fp;
        if(fc == 'd' || fc == 'x'){
          // int *ip = va_arg(ap, int *);
          int *ip=p_next_arg;
        //   printf("ip: %d\n", (int)ip);
          p_next_arg+=4;
          v = _strtol(rp, &ep, fc == 'd' ? 10 : 16);
          if(rp == ep) break;
          rp = ep;
          *(int*)(*ip) = v;
          cSuccess++;
        }else if(fc == 'c'){
            int *ip=p_next_arg;
            p_next_arg+=4;
            while(n_isspace(*rp)) rp++;
            *(char*)(*ip)=*rp++;
            cSuccess++;
        }else if(fc == 's'){
            //string format differs
            //we assume that rp(string by user) is zero-terminated
            const char** sp=p_next_arg;
            p_next_arg+=4;
            while(n_isspace(*rp)) rp++;
            //read str until meeting a space
            char* p=*sp;
            while(!n_isspace(*rp)){
                *p++=*rp++;
            }
            *p='\0';

            cSuccess++;
        }
    // else if(fc == 'f' || fc == 'g' || fc == 'e'){
    //     double fv = strtod(rp, &ep);
    //     if(ep == rp)
    //       break;
    //     float *vp = va_arg(ap, float *);
    //     *vp = fv;
    //     cSuccess++;
    //     rp = ep;
    //     }
      }
      fp++;
    }
#ifdef EOS_MATCHER_CHAR
    while(n_isspace(*rp)) rp++;
    if(*rp == 0 && *fp == EOS_MATCHER_CHAR)
      cSuccess++;
#endif
    
    // va_end(ap);
    return cSuccess;
}


#define NUL '\0'

static long _strtol (const char *nPtr, char **endPtr, int base) {
    //checking if the base value is correct
    if((base < 2 || base > 36) && base != 0) {
        // errno = EINVAL;
        return 0;
    }

    long number = 0;
    const char * divider;
    int currentdigit,
        sign,
        cutlim;
    enum sign {NEGATIVE, POSITIVE};
    unsigned long cutoff;
    bool correctconversion = true;

    divider = nPtr;

    //looking for a space if the beggining of the string is moved further
    while (n_isspace(* divider))
        divider++;

    //detecting the sign, positive by default
    if (* divider == '+') {
        sign = POSITIVE;
        divider++;
    } else if (* divider == '-') {
        sign = NEGATIVE;
        divider++;
    } else
        sign = POSITIVE;

    if (* divider == NUL) {
        * endPtr = (char *) divider;
        return 0;
    }

    if (* divider < '0' || (* divider > '9' && * divider < 'A') || (* divider > 'z'))
        return 0;

    if ((base == 8) && (* divider == '0')) {
        divider++;
        if (* divider == 'o' || * divider == 'O') //if the input includes 'o', it's skipped
            divider++;
    }
    else if ((base == 16)) {
        if (* divider == '0') {
            divider++;
            if (* divider == 'x' || * divider == 'X') {
                divider++;
                if (* divider > 'f' || * divider > 'F') {
                    divider--;
                    *endPtr = (char *) divider;
                    return 0;
                }
            }
            else
                divider--;
        }
    //basically the system-detecting algorithm
    } else if (base == 0) {
        if (* divider == '0') {
            divider++;
            if (* divider == 'o' || * divider == 'O') {
                base = 8;
                divider++;
                if (* divider > '7') {
                    divider--;
                    * endPtr = (char *) divider;
                    return 0;
                }
            } else if (* divider == 'x' || * divider == 'X') {
                base = 16;
                divider++;
                if (* divider > 'f' || * divider > 'F') {
                    divider--;
                    * endPtr = (char *) divider;
                    return 0;
                }
            } else if (* divider <= '7') {
                base = 8;
            } else {
                * endPtr = (char *) divider;
                return 0;
            }
        } else if (* divider >= '1' && * divider <= '9') {
                base = 10;
        }
    }

    //two conditions just for clarity --> |LONG_MIN| = LONG_MAX + 1
    if (sign)
        cutoff = LONG_MAX / (unsigned long) base;
    else
        cutoff = (unsigned long) LONG_MIN / (unsigned long) base;
    cutlim = cutoff % (unsigned long) base;

    //looping until the end of the input string
    //searching for convertable characters
    while (* divider != NUL) {
    	if (isdigit(* divider))
    		currentdigit = * divider - '0'; //converting to the actual integer
    	else {
    		if (isalpha(* divider)) {
    			if (islower(* divider) && (* divider - 'a') + 10 < base)
    				currentdigit = (* divider - 'a') + 10;
    			else if (!islower(* divider) && (* divider - 'A') + 10 < base)
                    currentdigit = (* divider - 'A') + 10;
                else
                    break;
    		} else
    			break;
    	}
    	if (!correctconversion ||
            number > cutoff ||
            (number == cutoff && (int) currentdigit > cutlim)) {
    		  correctconversion = false;
    		  divider++;
    	} else { //the actual conversion to decimal
    		correctconversion = true;
    		number = (number * base) + currentdigit;
    		divider++;
    	}
    }
    if (!correctconversion) {
    	if (sign)
    		number = LONG_MAX;
    	else
    		number = LONG_MIN;
    	// errno = ERANGE;
    }
    if (sign == NEGATIVE)
    	number *= -1;
    if (endPtr != NUL) {
        if (n_isspace(* divider)) //checking if the number is separated
            divider++;          //from the rest of the string
    	* endPtr = (char *) divider;
    }
    return number;
}
