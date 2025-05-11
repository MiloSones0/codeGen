/************************************************************************
University of Leeds
School of Computing
COMP2932- Compiler Design and Construction
The Compiler Module

I confirm that the following code has been developed and written by me and it is
entirely the result of my own work. I also confirm that I have not copied any
parts of this program from another person or any other source or facilitated
someone to copy this program from me. I confirm that I will not publish the
program online or share it with anyone without permission of the module leader.

Student Name: Milo Sones
Student ID: 201730449
Email: sc23m3s@leeds.ac.uk
Date Work Commenced: 28/4/2025
*************************************************************************/
#include "compiler.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>




ClassManager *manager;
int pass;
FILE *fptr;

int InitCompiler() {
  manager = initClassManager();
  ParserInfo p;
  // parse all std libs
  char *std_lib_files[] = {"Array.jack",  "Keyboard.jack", "Math.jack",
                           "Memory.jack", "Output.jack",   "Screen.jack",
                           "String.jack", "Sys.jack"};

  for (pass = 0; pass < 3; pass++) {
    for (int fileNum = 0; fileNum < 8; fileNum++) {
      InitParser(std_lib_files[fileNum]);
      p = Parse();
      if (p.er != none) {
        printf("Error in std library\n");
        exit(0);
      }
    }
  }
  return 1;
}

ParserInfo compile(char *dir_name) {
  ParserInfo p;
  struct dirent *entry;
  DIR *dp = opendir(dir_name);

  // Symbol table generation
  for (pass = 0; pass < 4; pass++) {
    while ((entry = readdir(dp))) {
      // Check if the file ends with ".jack"
      char *dot = strrchr(entry->d_name, '.');
      if (dot && strcmp(dot, ".jack") == 0) {
        // Build full file path
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);

        if (pass == 3) {
          openfile(path);
        }
        InitParser(path);
        p = Parse();
        if (p.er != none) {
          return p;
        }
        StopParser();
        if (pass == 3) {
          close();
        }
      }
    }
    rewinddir(dp);
  }

  closedir(dp);
  p.er = none;
  return p;
}
int StopCompiler() { 
  return 1; 
}

void openfile(char *filename) {
  // strip the .jack and repalce with .vm
  char newFilename[256];
  strcpy(newFilename, filename);
  char *dot = strrchr(newFilename, '.');
  strcpy(dot, ".vm\0");
  printf("making file %s\n", newFilename);
  fptr = fopen(newFilename, "w");
}

void writePush(char *Segment, int index) {
  // printf("write push %s %d\n", Segment, index);
  // printf("File pointer address: %p\n", (void *)fptr);
  // printf("Is file pointer valid? %s\n", fptr ? "Yes" : "No");
  // if (fptr == NULL)
  // {
  // 	printf("null fptr\n");
  // }
  fprintf(fptr, "push %s %d\n", Segment, index);
}

void writePop(char *Segment, int index) {
  fprintf(fptr, "pop %s %d\n", Segment, index);
}

void writeArithmetic(char *command) { fprintf(fptr, "%s\n", command); }
void writeLabel(char *label) { fprintf(fptr, "label %s\n", label); }
void writeGoto(char *label) { fprintf(fptr, "goto %s\n", label); }
void writeIfGoto(char *label) { fprintf(fptr, "if-goto %s\n", label); }
void writeCall(char *class, char *name, int nArgs) {
  // printf("call %s.%s %d\n", class, name, nArgs);
  fprintf(fptr, "call %s.%s %d\n", class, name, nArgs);
}
void writeFunction(char *class, char *name, int nLocals) {
  // printf("function %s.%s %d\n", class, name, nLocals);
  fprintf(fptr, "function %s.%s %d\n", class, name, nLocals);
}
void writeReturn() { fprintf(fptr, "return\n"); }
void close() {
  if (fptr == NULL) {
    printf("NULL PTR\n");
    exit(0);
  }
  fclose(fptr);
}
#ifndef TEST_COMPILER
int main() {
  InitCompiler();
  ParserInfo p = compile("Pong");
  StopCompiler();
  return 1;
}
#endif
