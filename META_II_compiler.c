/*
    META II compiler.
    Take a META II program and produce assembly instructions for the META II machine.

    This compiler attempts to emit exactly what the META II machine would emit
    when invoked with a compiled version of META II (META_II.m2a). Note that
    this is not a fundamental requirement and that we could instead emit any
    code as long as it implements what the syntax equations demand.
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

enum {
    TOK_KW_SYNTAX,
    TOK_KW_END,
    TOK_KW_ID,
    TOK_KW_NUMBER,
    TOK_KW_STRING,
    TOK_KW_EMPTY,
    TOK_KW_OUT,
    TOK_KW_LABEL,
    TOK_ID,
    TOK_STR,
    TOK_STAR,
    TOK_STAR1,
    TOK_STAR2,
    TOK_DOLLAR,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EQ,
    TOK_SEMI,
    TOK_SLASH,
    TOK_EOF,
};

char *prog_name;
char *input_path;
char *curr;
int LA;
int line_counter = 1;
char token_string[512];
int label_counter = 1;

void err(char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "%s: %s:%d: error: ", prog_name, input_path, line_counter);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void skip_white(void)
{
    while (isspace(*curr)) {
        if (*curr == '\n')
            ++line_counter;
        ++curr;
    }
}

int get_token(void)
{
    char *p;

    for (;;) {
        skip_white();
        p = token_string;
        if (*curr == '.') {
            *p++ = *curr++;
            if (*curr == ',') {
                *p++ = *curr++;
                *p = '\0';
                return TOK_SEMI;
            } else if (isalpha(*curr)) {
                int n;
#define TEST_KW(s)                              \
    if (strncmp(curr, #s, n=strlen(#s)) == 0) { \
        strncpy(p, curr, n);                    \
        p[n] = '\0';                            \
        curr += n;                              \
        return TOK_KW_##s;                      \
    }
                TEST_KW(SYNTAX);
                TEST_KW(END);
                TEST_KW(ID);
                TEST_KW(NUMBER);
                TEST_KW(STRING);
                TEST_KW(EMPTY);
                TEST_KW(OUT);
                TEST_KW(LABEL);
#undef TEST_KW
            }
            continue;
        } else if (isalpha(*curr)) {
            do
                *p++ = *curr++;
            while (isalnum(*curr));
            *p = '\0';
            return TOK_ID;
        } else if (*curr == '\'') {
            do
                *p++ = *curr++;
            while (*curr!='\0' && *curr!='\'' && *curr!='\n');
            if (*curr != '\'')
                err("unterminated string literal");
            *p++ = *curr++;
            *p = '\0';
            return TOK_STR;
        } else {
            switch (*p++ = *curr++) {
            case '\0': --curr; return TOK_EOF;
            case '*':
                if (*curr == '1') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_STAR1;
                } else if (*curr == '2') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_STAR2;
                } else {
                    *p = '\0';
                    return TOK_STAR;
                }
            case '$': *p = '\0'; return TOK_DOLLAR;
            case '(': *p = '\0'; return TOK_LPAREN;
            case ')': *p = '\0'; return TOK_RPAREN;
            case '=': *p = '\0'; return TOK_EQ;
            case '/': *p = '\0'; return TOK_SLASH;
            default:
                continue;
            }
        }
    }
}

void match(int expected)
{
    if (LA == expected)
        LA = get_token();
    else
        err("unexpected `%s'", token_string);
}

/*
    OUT1 = '*1'    .OUT('GN1')  /
           '*2'    .OUT('GN2')  /
           '*'     .OUT('CI')   /
           .STRING .OUT('CL '*) .,
*/
void out1(void)
{
    switch (LA) {
    case TOK_STAR1:
        printf("\tGN1\n");
        break;
    case TOK_STAR2:
        printf("\tGN2\n");
        break;
    case TOK_STAR:
        printf("\tCI\n");
        break;
    case TOK_STR:
        printf("\tCL %s\n", token_string);
        break;
    default:
        err("out1(): unexpected `%s'", token_string);
        break;
    }
    match(LA);
}

/*
    OUTPUT = ('.OUT' '('$OUT1')' /
              '.LABEL' .OUT('LB') OUT1)
             .OUT('OUT') .,
*/
void output(void)
{
    if (LA == TOK_KW_OUT) {
        match(TOK_KW_OUT);
        match(TOK_LPAREN);
        while (LA != TOK_RPAREN)
            out1();
        match(TOK_RPAREN);
    } else {
        match(TOK_KW_LABEL);
        printf("\tLB\n");
        out1();
    }
    printf("\tOUT\n");
}

void ex1(void);

/*
    EX3 = .ID       .OUT('CLL ' *) /
          .STRING   .OUT('TST ' *) /
          '.ID'     .OUT('ID')     /
          '.NUMBER' .OUT('NUM')    /
          '.STRING' .OUT('SR')     /
          '(' EX1 ')'              /
          '.EMPTY'  .OUT('SET')    /
          '$' .LABEL *1 EX3 .OUT('BT ' *1) .OUT('SET') .,
*/
void ex3(void)
{
    int lab1;

    switch (LA) {
    case TOK_ID:
        printf("\tCLL %s\n", token_string);
        match(TOK_ID);
        break;
    case TOK_STR:
        printf("\tTST %s\n", token_string);
        match(TOK_STR);
        break;
    case TOK_KW_ID:
        printf("\tID\n");
        match(TOK_KW_ID);
        break;
    case TOK_KW_NUMBER:
        printf("\tNUM\n");
        match(TOK_KW_NUMBER);
        break;
    case TOK_KW_STRING:
        printf("\tSR\n");
        match(TOK_KW_STRING);
        break;
    case TOK_KW_EMPTY:
        printf("\tSET\n");
        match(TOK_KW_EMPTY);
        break;
    case TOK_DOLLAR:
        match(TOK_DOLLAR);
        lab1 = label_counter++;
        printf("L%d\n", lab1);
        ex3();
        printf("\tBT L%d\n", lab1);
        printf("\tSET\n");
        break;
    case TOK_LPAREN:
        match(TOK_LPAREN);
        ex1();
        match(TOK_RPAREN);
        break;
    default:
        err("ex3(): unexpected `%s'", token_string);
        break;
    }
}

/*
    EX2 = (EX3 .OUT('BF ' *1) / OUTPUT)
          $(EX3 .OUT('BE') / OUTPUT)
          .LABEL *1 .,
*/
void ex2(void)
{
    int lab1;

    lab1 = -1;
    if (LA==TOK_KW_OUT || LA==TOK_KW_LABEL) {
        output();
    } else {
        ex3();
        lab1 = label_counter++;
        printf("\tBF L%d\n", lab1);
    }
    while (LA!=TOK_SLASH && LA!=TOK_SEMI && LA!=TOK_RPAREN) {
        if (LA==TOK_KW_OUT || LA==TOK_KW_LABEL) {
            output();
        } else {
            ex3();
            printf("\tBE\n");
        }
    }
    printf("L%d\n", (lab1!=-1)?lab1:label_counter++);
}

/*
    EX1 = EX2 $('/' .OUT('BT ' *1) EX2)
          .LABEL *1 .,
*/
void ex1(void)
{
    int lab1;

    ex2();
    lab1 = label_counter++;
    while (LA == TOK_SLASH) {
        match(TOK_SLASH);
        printf("\tBT L%d\n", lab1);
        ex2();
    }
    printf("L%d\n", lab1);
}

/*
    ST = .ID .LABEL * '=' EX1 '.,' .OUT('R') .,
*/
void st(void)
{
    if (LA == TOK_ID)
        printf("%s\n", token_string);
    match(TOK_ID);
    match(TOK_EQ);
    ex1();
    match(TOK_SEMI);
    printf("\tR\n");
}

/*
    PROGRAM = '.SYNTAX' .ID .OUT('ADR ' *)
              $ ST
              '.END' .OUT('END') .,
*/
void program(void)
{
    match(TOK_KW_SYNTAX);
    if (LA == TOK_ID)
        printf("\tADR %s\n", token_string);
    match(TOK_ID);
    while (LA != TOK_KW_END)
        st();
    match(TOK_KW_END);
    printf("\tEND\n");
    match(TOK_EOF);
}

int main(int argc, char *argv[])
{
    char *buf;
    FILE *fp;
    unsigned len;

    prog_name = argv[0];
    if (argc < 2) {
        fprintf(stderr, "usage: %s <program>\n", prog_name);
        exit(EXIT_SUCCESS);
    }
    input_path = argv[1];
    if ((fp=fopen(input_path, "rb")) == NULL) {
        fprintf(stderr, "%s: cannot read file `%s'\n", prog_name, input_path);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);
    buf = malloc(len+1);
    len = fread(buf, 1, len, fp);
    buf[len] = '\0';
    fclose(fp);
    curr = buf;
    LA = get_token();
    program();
    free(buf);

    return 0;
}
