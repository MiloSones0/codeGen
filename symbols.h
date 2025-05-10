#ifndef SYMBOLS_H
#define SYMBOLS_H

// Forward declarations
typedef struct ClassManager ClassManager;
typedef struct ClassEntry ClassEntry;

typedef struct Symbol {
  char lexeme[128];
  int address;
  char type[128];
  char kind[128];
} Symbol;

typedef struct Subroutine {
  char name[128];
  char subroutineType[128];
  char returnType[128];

  Symbol arguments[128];
  int argumentCount;
  Symbol locals[128];
  int localCount;
} Subroutine;

typedef struct Class {
  char name[128];

  Symbol fields[128];
  int fieldCount;
  Symbol statics[128];
  int staticCount;

  Subroutine subroutines[128];
  int subroutineCount;
} Class;

struct ClassEntry {
  char name[128];
  Class *class;
  struct ClassEntry *next;
};

struct ClassManager {
  Class *currentClass;
  Subroutine *currentSubroutine;
  ClassEntry *classList;
};

ClassManager *initClassManager();
int createClass(ClassManager *manager, const char *className);
void setCurrentClass(ClassManager *manager, const char *className);
void clearCurrentClass(ClassManager *manager);

int insertSubroutine(ClassManager *manager, const char *subroutineType,
                     const char *returnType, const char *name);

int setCurrentSubroutine(ClassManager *manager, const char *subroutineName);
void clearCurrentSubroutine(ClassManager *manager);

int insertSubroutineSymbol(ClassManager *manager, const char *lexeme,
                           const char *type, const char *kind,
                           const char *subroutineName);

int insertClassSymbol(ClassManager *manager, const char *lexeme,
                      const char *type, const char *kind);
Symbol *lookUpSymbol(ClassManager *manager, const char *name,
                     const char *subroutineName, char *outKind);

int classExists(ClassManager *manager, const char *className);
int getSubroutineArgCount(ClassManager *manager, const char *className,
                          const char *subroutineName);

Subroutine *getSubroutine(ClassManager *manager, const char *className,
                          const char *subroutineName);

#endif // SYMBOLS_H