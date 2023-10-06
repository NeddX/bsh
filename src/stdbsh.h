#ifndef BSH_STD_H
#define BSH_STD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libgen.h>
#include <signal.h>
#include <fcntl.h>

#define true 1
#define false 0
#define DEF_INPUT_COUNT 256
#define DEF_ARG_COUNT 64

typedef size_t usize;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uintptr_t uintptr;
typedef intptr_t intptr;
typedef uint8_t bool;

#endif // STD_BSH_H
