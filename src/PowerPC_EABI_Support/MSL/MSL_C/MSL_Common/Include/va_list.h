#ifndef _MSL_VA_LIST_H
#define _MSL_VA_LIST_H

typedef struct __va_list_struct {
    char gpr;
    char fpr;
    char reserved[2];
    char *input_arg_area;
    char *reg_save_area;
} __va_list[1];

typedef __va_list va_list;

#endif
