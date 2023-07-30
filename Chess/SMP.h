#pragma once

#ifdef DEBUG
constexpr int maxNumThreads = 1;
#else
#ifdef W32_BUILD
constexpr int maxNumThreads = 32;  // mustn't exceed 32
#else
constexpr int maxNumThreads = 256;
#endif
#endif

// Windows-specific
#include <windows.h>
typedef HANDLE mutex_t;
typedef HANDLE event_t;

#define log_msg(...)
#define error_msg(format, ...)                                              \
    do {																	\
        log_msg("error_msg: " format "\n", ##__VA_ARGS__);                  \
        fprintf(stderr, "error_msg: " format "\n", ##__VA_ARGS__);          \
        abort();															\
    } while (false)


#ifdef VERBOSE
#define VSAY Say
static int debug_loc = 0;
#define HERE debug_loc = __LINE__;
#else
#define VSAY(x)
#define HERE3(line) Say("info reached "  #line);
#define HERE2(x) HERE3(x)
#define HERE HERE2(__LINE__)
#endif

