

#ifndef COMPILER_H
#define COMPILER_H

#define TEST_COMPILER // uncomment to run the compiler autograder


#include "parser.h"
#include "symbols.h"

int InitCompiler();
ParserInfo compile(char *dir_name);
int StopCompiler();

void openfile(char *filename);
void writePush(char *Segment, int index);
void writePop(char *Segment, int index);
void writeArithmetic(char *command);
void writeLabel(char *label);
void writeGoto(char *label);
void writeIfGoto(char *label);
void writeCall(char *class, char *name, int nArgs);
void writeFunction(char *class, char *name, int nLocals);
void writeReturn(void);
void close(void);

#endif
