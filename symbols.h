#ifndef SYMBOLS_H
#define SYMBOLS_H

typedef struct Symbol Symbol;
typedef struct Scope Scope;
typedef struct ClassEntry ClassEntry;
typedef struct SymbolTableManager SymbolTableManager;
typedef struct SubroutineScope SubroutineScope;

typedef enum
{
    TYPE_INT,        // 0
    TYPE_CHAR,       // 1
    TYPE_BOOL,       // 2
    TYPE_STRING_LIT, // 3
    TYPE_TRUE,       // 4
    TYPE_FALSE,      // 5
    TYPE_NULL,       // 6
    TYPE_IDENTIFIER, // 7
    TYPE_VOID,       // 8
    TYPE_UNKNOWN     // 9
} SymbolType;

typedef enum
{
    KIND_CLASS,       // 0
    KIND_STATIC,      // 1
    KIND_FIELD,       // 2
    KIND_SUBROUTINE,  // 3
    KIND_LOCAL,       // 4
    KIND_ARGUMENT,    // 5
    KIND_CONSTRUCTOR, // 6
    KIND_FUNCTION,    // 7
    KIND_METHOD,      // 8
    KIND_VARIABLE,    // 9
    KIND_TEST         // 10
} SymbolKind;

typedef enum
{
    SCOPE_CLASS,
    SCOPE_SUBROUTINE
} ScopeType;

struct SubroutineScope
{
    Scope *scope;
    struct SubroutineScope *next;
};

struct Symbol
{
    char lexeme[128];
    int address;
    SymbolType type;
    SymbolKind kind;
    char className[128];
    char subroutineName[128];
};

struct Scope
{
    char name[128];
    ScopeType scopeType;

    // Class level symbols
    Symbol fields[128];
    int fieldCount;
    Symbol statics[128];
    int staticCount;
    Symbol subroutines[128]; // Subroutine declarations
    int subroutineCount;

    // Subroutine implementation details (only used when scopeType == SCOPE_SUBROUTINE)
    Symbol arguments[128];
    int argumentCount;
    Symbol locals[128];
    int localCount;
    SubroutineScope *subroutineScopes;

    Scope *parent;
};

struct ClassEntry
{
    char name[128];
    Scope *classScope;
    struct ClassEntry *next;
};

struct SymbolTableManager
{
    ClassEntry *classList;
    Scope *currentClass;
    Scope *currentSubroutine;
};

// Function prototypes
SymbolTableManager *initSymbolTableManager(void);
void destroySymbolTableManager(SymbolTableManager *manager);
void startClass(SymbolTableManager *manager, const char *className);
void switchClass(SymbolTableManager *manager, const char *className);
void endClass(SymbolTableManager *manager);
void startSubroutine(SymbolTableManager *manager, const char *subroutineName, SymbolType symbolType, SymbolKind symbolKind);
void switchSubroutine(SymbolTableManager *manager, const char *subroutineName);
void endSubroutine(SymbolTableManager *manager);
int insertSymbol(SymbolTableManager *manager, const char *name, SymbolType type, SymbolKind kind, const char *className, const char *subroutineName);
Scope *lookupClass(SymbolTableManager *manager, const char *className);
Symbol *lookupSymbolInClass(SymbolTableManager *manager, const char *className, const char *name);
Symbol *lookupSymbolInSubroutine(Scope *subroutineScope, const char *name);
void printSymbolTableTree(SymbolTableManager *manager);
Symbol *findSubroutineSymbol(SymbolTableManager *manager, const char *subroutineName);
Scope *getSubroutineScope(SymbolTableManager *manager, const char *className, const char *subroutineName);

#endif // SYMBOLS_H