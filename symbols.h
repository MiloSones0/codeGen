#ifndef SYMBOLS_H
#define SYMBOLS_H

typedef struct ClassManager ClassManager;
typedef struct ClassEntry ClassEntry;
typedef struct Class Class;
typedef struct Subroutine Subroutine;
typedef struct Symbol Symbol;


struct ClassManager {
    Class *currentClass;
    ClassEntry *classList;
}

struct ClassEntry {
    char name[128];
    Class *class;
    struct ClassEntry *next;
}


struct Class {
    char [name];

    Symbol fields[128];
    int fieldCount;
    Symbol statics[128];
    int staticCount;

    Subroutine subroutines[128];
    int subroutineCount;
}

struct Subroutine {
    char name[128];
    char subroutineType[128];
    char returnType[128];

    Symbol arguments[128];
    int argumentCount;
    Symbol locals[128];
    int localCount;

}

struct Symbol
{
    char lexeme[128];
    int address;
    char type[128];
    char kind[128];

};


ClassManager *initClassManager();

void createClass(ClassManager *manager, const char *className);
void setCurrentClass(ClassManager *manager, const char *className);
void clearCurrentClass(ClassManager *manager);

int insertSubroutine(
    const char *subroutineType,
    const char *returnType,
    const char* name,
);

int insertSubroutineSymbol(
    const char* lexeme,
    const char* type,
    const char* kind,

    const char* subroutineName
)

int insertClassSymbol(
    const char* lexeme,
    const char* type,
    const char* kind,
)

Symbol *lookUpSymbol(const char *name, const char *subroutineName);

#endif // SYMBOLS_H