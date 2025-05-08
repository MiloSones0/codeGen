

#ifndef COMPILER_H
#define COMPILER_H

#define TEST_COMPILER // uncomment to run the compiler autograder

#include "symbols.h"
#include "parser.h"

typedef enum
{
    CONST,
    ARG,
    LOCAL,
    STATIC,
    THIS,
    THAT,
    POINTER,
    TEMP
} Segment;

typedef enum
{
    ADD,
    SUB,
    NEG,
    EQ,
    GT,
    LT,
    AND,
    OR,
    NOT
} Command;

int InitCompiler();
ParserInfo compile(char *dir_name);
int StopCompiler();

void openfile(char *filename);
void writePush(char *Segment, int index);
void writePop(Segment segment, int index);
void writeArithmetic(Command command);
void writeLabel(char *label);
void writeIf(char *label);
void writeCall(char *class, char *name, int nArgs);
void writeFunction(char *class, char *name, int nLocals);
void writeReturn(void);
void close(void);

#endif
