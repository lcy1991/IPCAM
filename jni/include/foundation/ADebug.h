/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef A_DEBUG_H_

#define A_DEBUG_H_

#include <string.h>
#include "foundation/ABase.h"
#include "foundation/AString.h"
#ifdef ANDROID
#include <android/log.h>
#else
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#endif
#define MTU_SIZE     1464

#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define DEBUG  //open log
#ifdef DEBUG
#ifdef ANDROID
#define LOGF(tag, format, args...) __android_log_print(ANDROID_LOG_FATAL,tag,"[%s:%d]"format, __func__,__LINE__,##args)
#define LOGE(tag, format, args...) __android_log_print(ANDROID_LOG_ERROR,tag,"[%s:%d]"format, __func__, __LINE__,##args)
#define LOGW(tag, format, args...) __android_log_print(ANDROID_LOG_WARN,tag,"[%s:%d]"format,  __func__,__LINE__,##args)
#define LOGI(tag, format, args...) __android_log_print(ANDROID_LOG_INFO,tag,"[%s:%d]"format,  __func__,__LINE__,##args)
#else
/*
void PrintLog(const char *format, ...)
{
	FILE *logfp = NULL;

	if(logfp == NULL)
	{
		logfp = fopen("log.txt", "w");
	}
	va_list ap;
	va_start(ap, format);
	if(logfp) vfprintf(logfp,format,ap);
	va_start(ap, format);
	vprintf(format,ap);

	va_end(ap);
	fflush(logfp);
}	*/
#define LOGF(tag, format, args...) printf("[%s@%s,%d]"format"\n",__func__, __FILE__, __LINE__,##args)
#define LOGE(tag, format, args...) printf("[%s@%s,%d]"format"\n",__func__, __FILE__, __LINE__,##args)
#define LOGW(tag, format, args...) printf("[%s@%s,%d]"format"\n",__func__, __FILE__, __LINE__,##args)
#define LOGI(tag, format, args...) printf("[%s@%s,%d]"format"\n",__func__, __FILE__, __LINE__,##args)

#endif
#else
#define LOGF(tag, format, args...)
#define LOGE(tag, format, args...)
#define LOGW(tag, format, args...)
#define LOGI(tag, format, args...)
#endif
#define LOG_IF(con,format, args...) do{if(con)LOGF(__FILE__, format, ##args);}while(0)
#define CHECK(condition)                                \
    LOG_IF(                                \
            !(condition),                               \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)    \
            " CHECK(" #condition ") failed.")

#define MAKE_COMPARATOR(suffix,op)                          \
    template<class A, class B>                              \
    AString Compare_##suffix(const A &a, const B &b) {      \
        AString res;                                        \
        if (!(a op b)) {                                    \
            res.append(a);                                  \
            res.append(" vs. ");                            \
            res.append(b);                                  \
        }                                                   \
        return res;                                         \
    }

MAKE_COMPARATOR(EQ,==)
MAKE_COMPARATOR(NE,!=)
MAKE_COMPARATOR(LE,<=)
MAKE_COMPARATOR(GE,>=)
MAKE_COMPARATOR(LT,<)
MAKE_COMPARATOR(GT,>)

#define CHECK_OP(x,y,suffix,op)                                         \
    do {                                                                \
        AString ___res = Compare_##suffix(x, y);                        \
        if (!___res.empty()) {                                          \
            LOGF(   __FILE__,                                           \
                    __FILE__ ":" LITERAL_TO_STRING(__LINE__)            \
                    " CHECK_" #suffix "( " #x "," #y ") failed: %s",    \
                    ___res.c_str());                                    \
        }                                                               \
    } while (false)

#define CHECK_EQ(x,y)   CHECK_OP(x,y,EQ,==)
#define CHECK_NE(x,y)   CHECK_OP(x,y,NE,!=)
#define CHECK_LE(x,y)   CHECK_OP(x,y,LE,<=)
#define CHECK_LT(x,y)   CHECK_OP(x,y,LT,<)
#define CHECK_GE(x,y)   CHECK_OP(x,y,GE,>=)
#define CHECK_GT(x,y)   CHECK_OP(x,y,GT,>)

#define TRESPASS()      LOGF(__FILE__,"Should not be here.")


#endif  // A_DEBUG_H_

