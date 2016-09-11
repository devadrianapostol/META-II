/*
    VALGOL I machine.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include "asm.h"

#define STACK_MAX     64
#define PNT_AREA_SIZ  128

enum {
    OP_LD, OP_LDL, OP_ST,
    OP_ADD, OP_SUB, OP_MLT,
    OP_EQU, OP_B, OP_BFP,
    OP_BTP, OP_EDT, OP_PNT,
    OP_HLT, OP_SP, OP_BLK,
    OP_END,
};

static IDescr opcode_table[] = {
    { "LD",  OP_LD,  ARG_ID   },
    { "LDL", OP_LDL, ARG_NUM  },
    { "ST",  OP_ST,  ARG_ID   },
    { "ADD", OP_ADD, ARG_NONE },
    { "SUB", OP_SUB, ARG_NONE },
    { "MLT", OP_MLT, ARG_NONE },
    { "EQU", OP_EQU, ARG_NONE },
    { "B",   OP_B,   ARG_ID   },
    { "BFP", OP_BFP, ARG_ID   },
    { "BTP", OP_BTP, ARG_ID   },
    { "EDT", OP_EDT, ARG_STR  },
    { "PNT", OP_PNT, ARG_NONE },
    { "HLT", OP_HLT, ARG_NONE },
    { "SP",  OP_SP,  ARG_NUM  },
    { "BLK", OP_BLK, ARG_NBLK },
    { "END", OP_END, ARG_NONE },
    { NULL,  0,      0        },
};

char *prog_name;
static char *file_path;
static IRec *instructions;
static int instr_counter;

static void execute(void)
{
    int i;
    IRec *ip, *lim;
    int stack[STACK_MAX], tos;
    char pntar[PNT_AREA_SIZ];

    ip = &instructions[instructions[0].arg.loc];
    lim = &instructions[instr_counter];
    tos = -1;
    for (i = 0; i < PNT_AREA_SIZ-1; i++)
        pntar[i] = ' ';
    pntar[i] = '\0';

    while (ip < lim) {
        switch (ip->opcode) {
        case OP_LD:
            stack[++tos] = *(int *)&instructions[ip->arg.loc];
            break;
        case OP_LDL:
            stack[++tos] = ip->arg.val;
            break;
        case OP_ST:
            *(int *)&instructions[ip->arg.loc] = stack[tos--];
            break;
        case OP_ADD:
            stack[tos-1] += stack[tos];
            --tos;
            break;
        case OP_SUB:
            stack[tos-1] -= stack[tos];
            --tos;
            break;
        case OP_MLT:
            stack[tos-1] *= stack[tos];
            --tos;
            break;
        case OP_EQU:
            stack[tos-1] = stack[tos-1]==stack[tos];
            --tos;
            break;
        case OP_B:
            ip = &instructions[ip->arg.loc];
            continue;
        case OP_BFP:
            if (stack[tos--] == 0) {
                ip = &instructions[ip->arg.loc];
                continue;
            }
            break;
        case OP_BTP:
            if (stack[tos--] != 0) {
                ip = &instructions[ip->arg.loc];
                continue;
            }
            break;
        case OP_EDT:
            memcpy(pntar+stack[tos], ip->arg.str, strlen(ip->arg.str));
            --tos;
            break;
        case OP_PNT:
            printf("%s\n", pntar);
            for (i = 0; i < PNT_AREA_SIZ-1; i++)
                pntar[i] = ' ';
            pntar[i] = '\0';
            break;
        case OP_HLT:
            return;
        case OP_SP:
        case OP_BLK:
        case OP_END:
        default:
            assert(0);
        }
        ++ip;
    }
}

int main(int argc, char *argv[])
{
    prog_name = argv[0];
    if (argc < 2) {
        fprintf(stderr, "usage: %s <code>\n", prog_name);
        exit(EXIT_SUCCESS);
    }
    file_path = argv[1];
    if ((instructions=read_program(file_path, opcode_table, &instr_counter)) == NULL)
        exit(EXIT_FAILURE);

#if 0
    {
        int i;
        for (i = 0; i < instr_counter; i++) {
            IRec *ir;

            ir = &instructions[i];
            printf("(%d) ", i);
            if (ir->opcode >= 0)
                print_instr(ir);
            else
                printf("\n");
        }
    }
#endif

    execute();

    return 0;
}
