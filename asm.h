#ifndef ASM_H_
#define ASM_H_

typedef int OpCode;
typedef struct IRec IRec;
typedef struct IDescr IDescr;

typedef enum {
    ARG_NONE,
    ARG_ID,
    ARG_STR,
    ARG_NUM,
    ARG_NBLK, /* # of cells to reserve; each cell has sizeof(IRec) bytes */
} ArgKind;

struct IRec {
    OpCode opcode;
    union {
        char *str;
        int loc;
        int val;
    } arg;
};

struct IDescr {
    char *mne;
    OpCode opc;
    ArgKind arg_kind;
};

IRec *read_program(char *file_path, IDescr *opcode_table, int *instr_counter);
void print_instr(IRec *ir);

#endif
