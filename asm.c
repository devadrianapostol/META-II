#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <setjmp.h>
#include "asm.h"

#define LINEBUFSIZ  1024
#define LABTABSIZ   511

typedef struct LabSym LabSym;
typedef struct FixUp FixUp;

static struct LabSym {
    char *id;
    int val;
    LabSym *next;
} *label_table[LABTABSIZ];

static struct FixUp {
    int loc;
    FixUp *next;
} *fixup_list;

/* USER PROVIDED */
extern char *prog_name;
static IDescr *opcode_table;
static char *file_path;
/* ------------- */

static int line_counter = 1;
static IRec *instructions;
static int instr_counter, instr_max;
static jmp_buf env;

static void err(char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "%s: %s:%d: error: ", prog_name, file_path, line_counter);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    longjmp(env, 1);
}

static unsigned hash(char *s)
{
    unsigned hash_val;

    for (hash_val = 0; *s != '\0'; s++)
        hash_val = (unsigned)*s + 31*hash_val;
    return hash_val;
}

static int label_value(char *s)
{
    LabSym *np;
    unsigned h;

    h = hash(s)%LABTABSIZ;
    for (np = label_table[h]; np != NULL; np = np->next)
        if (strcmp(np->id, s) == 0)
            return np->val;
    return -1;
}

static char *get_identifier(char *s, char *buf)
{
    while (isblank(*s))
        ++s;
    if (!isalpha(*s))
        return NULL;
    *buf++ = *s++;
    while (isalnum(*s))
        *buf++ = *s++;
    *buf = '\0';
    return s;
}

static char *get_string(char *s, char *buf)
{
    while (isblank(*s))
        ++s;
    if (*s!='\'' || *(s+1)=='\'')
        return NULL;
    ++s;
    while (*s!='\'' && *s!='\0')
        *buf++ = *s++;
    if (*s != '\'')
        return NULL;
    ++s;
    *buf = '\0';
    return s;
}

static char *get_number(char *s, char *buf)
{
    while (isblank(*s))
        ++s;
    if (!isdigit(*s))
        return NULL;
    *buf++ = *s++;
    while (isdigit(*s))
        *buf++ = *s++;
    *buf = '\0';
    return s;
}

/* label = no_space ID */
static void parse_label(char *s)
{
    LabSym *np;
    unsigned h;
    char buf[LINEBUFSIZ];

    if ((s=get_identifier(s, buf)) == NULL)
        err("expecting identifier on label line");
    h = hash(buf)%LABTABSIZ;
    for (np = label_table[h]; np != NULL; np = np->next)
        if (strcmp(np->id, buf) == 0)
            break;
    if (np != NULL)
        err("label `%s' redefined", buf);
    np = malloc(sizeof(*np));
    np->id = strdup(buf);
    np->val = instr_counter;
    np->next = label_table[h];
    label_table[h] = np;
}

static IRec *new_instr(OpCode opcode)
{
    if (instr_counter >= instr_max) {
        instr_max = instr_max?instr_max*2:64;
        instructions = realloc(instructions, sizeof(instructions[0])*instr_max);
    }
    instructions[instr_counter].opcode = opcode;
    return &instructions[instr_counter++];
}

/* instruction = space MNE operand */
static void parse_instruction(char *s)
{
    int i;
    IRec *ir;
    OpCode opc;
    char mne[8], buf[LINEBUFSIZ];

    if ((s=get_identifier(s, buf)) == NULL)
        err("expecting mnemonic on instruction line");
    for (i = 0; opcode_table[i].mne != NULL; i++)
        if (strcmp(opcode_table[i].mne, buf) == 0)
            break;
    if (opcode_table[i].mne == NULL)
        err("unknown mnemonic `%s'", buf);
    strcpy(mne, buf);
    opc = opcode_table[i].opc;

    switch (opcode_table[i].arg_kind) {
    case ARG_ID: {
        FixUp *fp;

        if (get_identifier(s, buf) == NULL)
            err("instruction `%s' requires an identifier argument", mne);
        fp = malloc(sizeof(*fp));
        fp->loc = instr_counter;
        fp->next = fixup_list;
        fixup_list = fp;
        ir = new_instr(opc);
        ir->arg.str = strdup(buf);
    }
        break;
    case ARG_STR:
        if (get_string(s, buf) == NULL)
            err("instruction `%s' requires a string argument", mne);
        ir = new_instr(opc);
        ir->arg.str = strdup(buf);
        break;
    case ARG_NUM:
        if (get_number(s, buf) == NULL)
            err("instruction `%s' requires a number argument", mne);
        ir = new_instr(opc);
        ir->arg.val = (int)strtol(buf, NULL, 10);
        break;
    case ARG_NBLK:
        if (get_number(s, buf) == NULL)
            err("instruction `%s' requires a number argument", mne);
        i = (int)strtol(buf, NULL, 10);
        assert(i>=0 && i<=256); /* 256 is an arbitrary limit */
        while (i-- > 0)
            (void)new_instr(-1);
        break;
    default:
        ir = new_instr(opc);
        ir->arg.str = NULL;
        break;
    }
}

/* program = { ( label | instruction ) EOL } */
IRec *read_program(char *_file_path, IDescr *_opcode_table, int *_instr_counter)
{
    FILE *fp;
    FixUp *fx, *next;
    char linebuf[LINEBUFSIZ];

    file_path = _file_path;
    opcode_table = _opcode_table;

    if ((fp=fopen(file_path, "rb")) == NULL) {
        fprintf(stderr, "%s: cannot read code file `%s'\n", prog_name, file_path);
        return NULL;
    }
    if (!setjmp(env)) {
        while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
            if (linebuf[0] != '\n') {
                if (isblank(linebuf[0]))
                    parse_instruction(linebuf);
                else
                    parse_label(linebuf);
            }
            ++line_counter;
        }
    } else {
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    for (fx = fixup_list; fx != NULL; fx = next) {
        IRec *ir;
        int loc;

        ir = &instructions[fx->loc];
        if ((loc=label_value(ir->arg.str)) == -1) {
            fprintf(stderr, "%s: label `%s' referenced but never defined\n",
            prog_name, ir->arg.str);
            return NULL;
        }
        free(ir->arg.str);
        ir->arg.loc = loc;
        next = fx->next;
        free(fx);
    }
    *_instr_counter = instr_counter;
    return instructions;
}

void print_instr(IRec *ir)
{
    int i;

    for (i = 0; opcode_table[i].mne != NULL; i++)
        if (opcode_table[i].opc == ir->opcode)
            break;
    assert(opcode_table[i].mne != NULL);
    switch (opcode_table[i].arg_kind) {
    case ARG_NONE:
        printf("%s(%d)\n", opcode_table[i].mne, ir->opcode);
        break;
    case ARG_ID:
        printf("%s(%d) %d\n", opcode_table[i].mne, ir->opcode, ir->arg.loc);
        break;
    case ARG_STR:
        printf("%s(%d) '%s'\n", opcode_table[i].mne, ir->opcode, ir->arg.str);
        break;
    case ARG_NUM:
    case ARG_NBLK:
        printf("%s(%d) %d\n", opcode_table[i].mne, ir->opcode, ir->arg.val);
        break;
    }
}
