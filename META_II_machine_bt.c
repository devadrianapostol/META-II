/*
    META II machine.
    Load and execute a compiled META II program.

    This version implements backtracking as explained at the end of Schorre's
    paper ("Backup vs. No Backup").

    Backtracking is done with rule granularity. That is, when a syntax error
    occurs the whole current rule fails and returns.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include "asm.h"

#define MAXFRAMES   64  /* max # of stacked frames (CLL) at one given time */

enum {
    OP_TST, OP_ID, OP_NUM,
    OP_SR, OP_CLL, OP_R,
    OP_SET, OP_B, OP_BT,
    OP_BF, OP_BE, OP_CL,
    OP_CI, OP_GN1, OP_GN2,
    OP_LB, OP_OUT, OP_ADR,
    OP_END,
};

static IDescr opcode_table[] = {
    { "TST", OP_TST, ARG_STR  },
    { "ID",  OP_ID,  ARG_NONE },
    { "NUM", OP_NUM, ARG_NONE },
    { "SR",  OP_SR,  ARG_NONE },
    { "CLL", OP_CLL, ARG_ID   },
    { "R",   OP_R,   ARG_NONE },
    { "SET", OP_SET, ARG_NONE },
    { "B",   OP_B,   ARG_ID   },
    { "BT",  OP_BT,  ARG_ID   },
    { "BF",  OP_BF,  ARG_ID   },
    { "BE",  OP_BE,  ARG_NONE },
    { "CL",  OP_CL,  ARG_STR  },
    { "CI",  OP_CI,  ARG_NONE },
    { "GN1", OP_GN1, ARG_NONE },
    { "GN2", OP_GN2, ARG_NONE },
    { "LB",  OP_LB,  ARG_NONE },
    { "OUT", OP_OUT, ARG_NONE },
    { "ADR", OP_ADR, ARG_ID   },
    { "END", OP_END, ARG_NONE },
    { NULL,  0,      0        },
};

char *prog_name;
static char *file_path;
static IRec *instructions;
static int instr_counter;
static int line_counter = 1;

static struct {
    char *buf;
    int pos, siz;
} output_buffer;

static void outbuf_write(char *s)
{
    int n;

    n = strlen(s);
    if (output_buffer.pos+n > output_buffer.siz) {
        output_buffer.siz = output_buffer.siz*2+n;
        output_buffer.buf = realloc(output_buffer.buf, output_buffer.siz);
        assert(output_buffer.buf != NULL);
    }
    memcpy(output_buffer.buf+output_buffer.pos, s, n);
    output_buffer.pos += n;
}

static void outbuf_flush(void)
{
    if (output_buffer.pos == 0)
        return;
    fwrite(output_buffer.buf, 1, output_buffer.pos, stdout);
    output_buffer.pos = 0;
}

static void outbuf_set_pos(int pos)
{
    assert(pos>=0 && pos<=output_buffer.siz);
    output_buffer.pos = pos;
}

static char *skip_white(char *s)
{
    while (*s!='\0' && isspace(*s)) {
        if (*s == '\n')
            ++line_counter;
        ++s;
    }
    return s;
}

static void execute(char *pos)
{
    int i, res;
    IRec *ip, *lim;
    char *s, *t;
    char lastbuf[256], labbuf[32];
    int labcnt;
    int indent;
    struct {
        int lab1, lab2;
        int ret_addr;
        /* state upon entry to subroutine */
        char *in_pos;
        int out_pos;
        char lastbuf[256];
        int line_counter, labcnt, indent;
    } frames[MAXFRAMES];
    int top_frame;

#define SAVE_STATE()                                    \
    do {                                                \
        frames[top_frame].in_pos = pos;                 \
        frames[top_frame].out_pos = output_buffer.pos;  \
        strcpy(frames[top_frame].lastbuf, lastbuf);     \
        frames[top_frame].line_counter = line_counter;  \
        frames[top_frame].labcnt = labcnt;              \
        frames[top_frame].indent = indent;              \
    } while (0)
#define RESTORE_STATE()                                 \
    do {                                                \
        pos = frames[top_frame].in_pos;                 \
        outbuf_set_pos(frames[top_frame].out_pos);      \
        strcpy(lastbuf, frames[top_frame].lastbuf);     \
        line_counter = frames[top_frame].line_counter;  \
        labcnt = frames[top_frame].labcnt;              \
        indent = frames[top_frame].indent;              \
    } while (0)

    ip = &instructions[instructions[0].arg.loc];
    lim = &instructions[instr_counter];
    labcnt = 1;
    indent = 1;

    res = 1;
    top_frame = 0;
    frames[top_frame].lab1 = -1;
    frames[top_frame].lab2 = -1;

    while (ip < lim) {
        switch (ip->opcode) {
        case OP_TST:
            i = 0;
            pos = skip_white(pos);
            for (s=pos, t=ip->arg.str; *t!='\0' && *s==*t; s++, t++)
                lastbuf[i++] = *t;
            if (*t == '\0') {
                pos = s;
                res = 1;
            } else {
                res = 0;
            }
            lastbuf[i] = '\0';
            break;
        case OP_ID:
            i = 0;
            s = pos = skip_white(pos);
            if (isalpha(*s)) {
                lastbuf[i++] = *s++;
                while (isalnum(*s))
                    lastbuf[i++] = *s++;
                pos = s;
                res = 1;
            } else {
                res = 0;
            }
            lastbuf[i] = '\0';
            break;
        case OP_NUM:
            i = 0;
            s = pos = skip_white(pos);
            if (isdigit(*s)) {
                lastbuf[i++] = *s++;
                while (isdigit(*s))
                    lastbuf[i++] = *s++;
                pos = s;
                res = 1;
            } else {
                res = 0;
            }
            lastbuf[i] = '\0';
            break;
        case OP_SR:
            i = 0;
            s = pos = skip_white(pos);
            if (*s == '\'') {
                lastbuf[i++] = *s++;
                while (*s!='\'' && *s!='\0' && *s!='\n')
                    lastbuf[i++] = *s++;
            }
            if (*s == '\'') {
                lastbuf[i++] = *s++;
                pos = s;
                res = 1;
            } else {
                res = 0;
            }
            lastbuf[i] = '\0';
            break;
        case OP_CLL:
            ++top_frame;
            frames[top_frame].ret_addr = (ip-instructions)+1;
            frames[top_frame].lab1 = -1;
            frames[top_frame].lab2 = -1;
            SAVE_STATE();
            ip = &instructions[ip->arg.loc];
            continue;
        case OP_R:
            if (top_frame == 0)
                goto done;
            ip = &instructions[frames[top_frame].ret_addr];
            --top_frame;
            continue;
        case OP_SET:
            res = 1;
            break;
        case OP_B:
            ip = &instructions[ip->arg.loc];
            continue;
        case OP_BT:
            if (res) {
                ip = &instructions[ip->arg.loc];
                continue;
            }
            break;
        case OP_BF:
            if (!res) {
                ip = &instructions[ip->arg.loc];
                continue;
            }
            break;
        case OP_BE:
            if (!res) {
                if (top_frame == 0) {
                    outbuf_flush();
                    printf("%s: %s:%d: syntax error\n", prog_name, file_path, line_counter);
                    goto done;
                }
                RESTORE_STATE();
                ip = &instructions[frames[top_frame].ret_addr];
                --top_frame;
                res = 0;
                continue;
            }
            break;
        case OP_CL:
            if (indent)
                outbuf_write("\t");
            outbuf_write(ip->arg.str);
            indent = 0;
            break;
        case OP_CI:
            if (indent)
                outbuf_write("\t");
            outbuf_write(lastbuf);
            indent = 0;
            break;
        case OP_GN1:
            if (indent)
                outbuf_write("\t");
            if (frames[top_frame].lab1 == -1)
                frames[top_frame].lab1 = labcnt++;
            sprintf(labbuf, "L%d", frames[top_frame].lab1);
            outbuf_write(labbuf);
            indent = 0;
            break;
        case OP_GN2:
            if (indent)
                outbuf_write("\t");
            if (frames[top_frame].lab2 == -1)
                frames[top_frame].lab2 = labcnt++;
            sprintf(labbuf, "L%d", frames[top_frame].lab2);
            outbuf_write(labbuf);
            indent = 0;
            break;
        case OP_LB:
            indent = 0;
            break;
        case OP_OUT:
            outbuf_write("\n");
            indent = 1;
            break;
        case OP_END:
        case OP_ADR:
        default:
            assert(0);
        }
        ++ip;
    }
done:
    outbuf_flush();
#undef SAVE_STATE
#undef RESTORE_STATE
}

int main(int argc, char *argv[])
{
    char *inbuf;
    unsigned len;
    FILE *fp;

    prog_name = argv[0];
    if (argc < 3) {
        fprintf(stderr, "usage: %s <code> <input>\n", prog_name);
        exit(EXIT_SUCCESS);
    }
    file_path = argv[1];
    if ((instructions=read_program(file_path, opcode_table, &instr_counter)) == NULL)
        exit(EXIT_FAILURE);
    if (instructions[0].opcode != OP_ADR) {
        fprintf(stderr, "%s: code file `%s' does not begin with ADR instruction\n",
        prog_name, file_path);
        exit(EXIT_FAILURE);
    }

#if 0
    {
        int i;
        for (i = 0; i < instr_counter; i++) {
            IRec *ir;

            ir = &instructions[i];
            printf("(%d) ", i);
            print_instr(ir);
        }
    }
#endif

    file_path = argv[2];
    if ((fp=fopen(file_path, "rb")) == NULL)
        fprintf(stderr, "%s: cannot read input file `%s'\n", prog_name, file_path);
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);
    inbuf = malloc(len+1);
    len = fread(inbuf, 1, len, fp);
    inbuf[len] = '\0';
    fclose(fp);

    output_buffer.siz = 256;
    output_buffer.buf = malloc(output_buffer.siz);
    output_buffer.pos = 0;
    execute(inbuf);
    free(inbuf);
    free(output_buffer.buf);

    return 0;
}
