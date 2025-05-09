
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "symbols.h"

int insertIntoArray(Symbol *array, int *count, const char *name,
                    SymbolType type, SymbolKind kind, const char *className,
                    const char *subroutineName);

// Initialize symbol table manager
SymbolTableManager *initSymbolTableManager()
{
    SymbolTableManager *manager = (SymbolTableManager *)malloc(sizeof(SymbolTableManager));
    manager->classList = NULL;
    manager->currentClass = NULL;
    manager->currentSubroutine = NULL;
    return manager;
}

// Destroy symbol table manager and all its contents
void destroySymbolTableManager(SymbolTableManager *manager)
{
    ClassEntry *currentClass = manager->classList;
    while (currentClass)
    {
        ClassEntry *nextClass = currentClass->next;

        // Free all subroutine scopes for this class
        SubroutineScope *currentSub = currentClass->classScope->subroutineScopes;
        while (currentSub)
        {
            SubroutineScope *nextSub = currentSub->next;
            free(currentSub->scope);
            free(currentSub);
            currentSub = nextSub;
        }

        free(currentClass->classScope);
        free(currentClass);
        currentClass = nextClass;
    }
    free(manager);
}

// Start a new class scope
void startClass(SymbolTableManager *manager, const char *className)
{
    // Check if class already exists
    ClassEntry *entry = manager->classList;
    while (entry)
    {
        if (strcmp(entry->name, className) == 0)
        {
            fprintf(stderr, "Error: Class '%s' already defined\n", className);
            return;
        }
        entry = entry->next;
    }

    // Create new class entry
    ClassEntry *newEntry = (ClassEntry *)malloc(sizeof(ClassEntry));
    strncpy(newEntry->name, className, sizeof(newEntry->name) - 1);

    // Create class scope
    Scope *classScope = (Scope *)malloc(sizeof(Scope));
    strncpy(classScope->name, className, sizeof(classScope->name) - 1);
    classScope->scopeType = SCOPE_CLASS;
    classScope->parent = NULL;
    classScope->fieldCount = 0;
    classScope->staticCount = 0;
    classScope->subroutineCount = 0;

    // Initialize symbol arrays
    memset(classScope->fields, 0, sizeof(classScope->fields));
    memset(classScope->statics, 0, sizeof(classScope->statics));
    memset(classScope->subroutines, 0, sizeof(classScope->subroutines));

    newEntry->classScope = classScope;
    newEntry->next = manager->classList;
    manager->classList = newEntry;

    manager->currentClass = classScope;
    manager->currentSubroutine = NULL;
}

void switchClass(SymbolTableManager *manager, const char *className)
{
    // Check if class already exists
    ClassEntry *entry = manager->classList;
    while (entry)
    {
        if (strcmp(entry->name, className) == 0)
        {
            manager->currentClass = entry->classScope;
            manager->currentSubroutine = NULL;
            return;
        }
        entry = entry->next;
    }
    fprintf(stderr, "Error: Cannot switch to class '%s' as not defined\n", className);
}

Symbol *findSubroutineSymbol(SymbolTableManager *manager, const char *subroutineName)
{
    if (!manager || !manager->currentClass || !subroutineName)
    {
        return NULL;
    }

    Scope *currentClass = manager->currentClass;

    // Search through all subroutines in the current class
    for (int i = 0; i < currentClass->subroutineCount; i++)
    {
        if (strcmp(currentClass->subroutines[i].lexeme, subroutineName) == 0)
        {
            return &currentClass->subroutines[i];
        }
    }

    return NULL; // Subroutine not found
}

// End current class scope
void endClass(SymbolTableManager *manager)
{
    if (manager->currentSubroutine)
    {
        endSubroutine(manager);
    }

    // Free all subroutine scopes for this class
    // (We need to track them somehow - could add them to a list in the class scope)
    // For now, we'll assume they're managed elsewhere
    manager->currentClass = NULL;
}

// Start a new subroutine scope
void startSubroutine(SymbolTableManager *manager, const char *subroutineName, SymbolType symbolType, SymbolKind symbolKind)
{
    if (!manager->currentClass)
    {
        fprintf(stderr, "Error: No class scope defined\n");
        return;
    }

    // Check if subroutine already exists

    for (int i = 0; i < manager->currentClass->subroutineCount; i++)
    {
        if (strcmp(manager->currentClass->subroutines[i].lexeme, subroutineName) == 0)
        {
            fprintf(stderr, "Error: Subroutine '%s' already defined in class '%s'\n",
                    subroutineName, manager->currentClass->name);
            return;
        }
    }

    // Create new subroutine scope
    Scope *newScope = (Scope *)malloc(sizeof(Scope));
    strncpy(newScope->name, subroutineName, sizeof(newScope->name) - 1);
    newScope->scopeType = SCOPE_SUBROUTINE;
    newScope->parent = manager->currentClass;

    // Initialize symbol arrays
    memset(newScope->arguments, 0, sizeof(newScope->arguments));
    memset(newScope->locals, 0, sizeof(newScope->locals));
    newScope->argumentCount = 0;
    newScope->localCount = 0;

    // Add to the class's subroutine scopes list
    SubroutineScope *newSub = (SubroutineScope *)malloc(sizeof(SubroutineScope));
    newSub->scope = newScope;
    newSub->next = manager->currentClass->subroutineScopes;
    manager->currentClass->subroutineScopes = newSub;

    // Set as current subroutine
    manager->currentSubroutine = newScope;

    if (symbolKind == KIND_METHOD)
    {
        insertIntoArray(newScope->arguments, &newScope->argumentCount,
                        "this", TYPE_IDENTIFIER, KIND_ARGUMENT,
                        manager->currentClass->name, subroutineName);
    }

    // Also add to the class's subroutine declarations
    int i = manager->currentClass->subroutineCount;
    strncpy(manager->currentClass->subroutines[i].lexeme, subroutineName,
            sizeof(manager->currentClass->subroutines[i].lexeme) - 1);

    manager->currentClass->subroutines[i].type = symbolType; // Can be set properly later
    manager->currentClass->subroutines[i].kind = symbolKind;
    strncpy(manager->currentClass->subroutines[i].className,
            manager->currentClass->name,
            sizeof(manager->currentClass->subroutines[i].className) - 1);
    manager->currentClass->subroutineCount += 1;
}

void switchSubroutine(SymbolTableManager *manager, const char *subroutineName)
{
    if (!manager->currentClass)
    {
        fprintf(stderr, "Error: No class scope defined\n");
        return;
    }

    SubroutineScope *entry = manager->currentClass->subroutineScopes;
    while (entry)
    {
        if (strcmp(entry->scope->name, subroutineName) == 0)
        {
            manager->currentSubroutine = entry->scope;
            return;
        }
        entry = entry->next;
    }
    fprintf(stderr, "Error: Cannot switch to subroutine '%s' as not defined in %s\n", subroutineName, manager->currentClass->name);
}

// End current subroutine scope
void endSubroutine(SymbolTableManager *manager)
{
    // No need to free here - we'll free when the class ends
    manager->currentSubroutine = NULL;
}

// Helper function to insert into a symbol array
int insertIntoArray(Symbol *array, int *count, const char *name,
                    SymbolType type, SymbolKind kind, const char *className,
                    const char *subroutineName)
{
    if (*count >= 128)
    { // Check if array is full
        return -1;
    }
    strncpy(array[*count].lexeme, name, sizeof(array[*count].lexeme) - 1);
    array[*count].address = *count;

    array[*count].type = type;
    array[*count].kind = kind;

    if (className)
    {
        strncpy(array[*count].className, className, sizeof(array[*count].className) - 1);
    }

    if (subroutineName)
    {
        strncpy(array[*count].subroutineName, subroutineName, sizeof(array[*count].subroutineName) - 1);
    }

    (*count)++;        // Increment the count
    return *count - 1; // Return the index where the symbol was inserted
}
// Insert a symbol into the appropriate table

int insertSymbol(SymbolTableManager *manager, const char *name, SymbolType type, SymbolKind kind, const char *className, const char *subroutineName)
{
    if (!manager->currentClass)
        return -1;

    if (manager->currentSubroutine)
    {
        // We're in a subroutine scope
        switch (kind)
        {
        case KIND_ARGUMENT:
            Symbol *symbol = findSubroutineSymbol(manager, manager->currentSubroutine->name);
            if (symbol->kind == KIND_METHOD && manager->currentSubroutine->argumentCount == 0)
            {
                return insertIntoArray(manager->currentSubroutine->arguments,
                                       &manager->currentSubroutine->argumentCount,
                                       name, type, kind, className, subroutineName);
            }
            else
            {
                return insertIntoArray(manager->currentSubroutine->arguments,
                                       &manager->currentSubroutine->argumentCount,
                                       name, type, kind, className, subroutineName);
            }

        case KIND_LOCAL:
            return insertIntoArray(manager->currentSubroutine->locals,
                                   &manager->currentSubroutine->localCount,
                                   name, type, kind, className, subroutineName);
        default:
            break;
        }
    }

    // Class-level symbols
    switch (kind)
    {
    case KIND_STATIC:
        return insertIntoArray(manager->currentClass->statics,
                               &manager->currentClass->staticCount,
                               name, type, kind, className, NULL);
    case KIND_FIELD:
        return insertIntoArray(manager->currentClass->fields,
                               &manager->currentClass->fieldCount,
                               name, type, kind, className, NULL);

    // not needed as subroutine are added to array when created

    // case KIND_SUBROUTINE:
    //     return insertIntoArray(manager->currentClass->subroutines,
    //                 &manager->currentClass->subroutineCount,
    //                 name, address, type, kind, className, NULL);
    default:
        break;
    }

    return -1; // Invalid kind for current scope
}

Scope *lookupClass(SymbolTableManager *manager, const char *className)
{
    ClassEntry *entry = manager->classList;
    while (entry)
    {
        if (strcmp(entry->name, className) == 0)
        {
            return entry->classScope;
        }
        entry = entry->next;
    }
    return NULL;
}

// Lookup a symbol in a specific class
Symbol *lookupSymbolInClass(SymbolTableManager *manager, const char *className, const char *name)
{
    // Find the class
    ClassEntry *entry = manager->classList;
    while (entry)
    {
        if (strcmp(entry->name, className) == 0)
        {
            // Check fields and statics
            for (int i = 0; i < entry->classScope->fieldCount; i++)
            {
                if (strcmp(entry->classScope->fields[i].lexeme, name) == 0)
                {
                    return &entry->classScope->fields[i];
                }
            }

            for (int i = 0; i < entry->classScope->staticCount; i++)
            {
                if (strcmp(entry->classScope->statics[i].lexeme, name) == 0)
                {
                    return &entry->classScope->statics[i];
                }
            }

            for (int i = 0; i < entry->classScope->subroutineCount; i++)
            {
                // printf("subroutines in %s: %s == %s\n", className, entry->classScope->subroutines[i].lexeme, name);
                if (strcmp(entry->classScope->subroutines[i].lexeme, name) == 0)
                {
                    // printf("returning\n");
                    return &entry->classScope->subroutines[i];
                }
            }

            return NULL; // Not found in this class
        }
        entry = entry->next;
    }

    return NULL; // Class not found
}

Symbol *lookupSymbolInSubroutine(Scope *subroutineScope, const char *name)
{
    // check arguments
    for (int i = 0; i < subroutineScope->argumentCount; i++)
    {
        if (strcmp(subroutineScope->arguments[i].lexeme, name) == 0)
        {
            return &subroutineScope->arguments[i];
        }
    }
    // Check locals
    for (int i = 0; i < subroutineScope->localCount; i++)
    {
        if (strcmp(subroutineScope->locals[i].lexeme, name) == 0)
        {
            return &subroutineScope->locals[i];
        }
    }
    return NULL;
}

// Print the entire symbol table
void printSymbolTableTree(SymbolTableManager *manager)
{
    ClassEntry *entry = manager->classList;
    if (!entry)
    {
        printf("Symbol table is empty\n");
        return;
    }

    while (entry)
    {
        Scope *classScope = entry->classScope;
        printf("Class: %s\n", classScope->name);

        // Print fields
        printf("  Fields:\n");
        for (int i = 0; i < classScope->fieldCount; i++)
        {
            if (*classScope->fields[i].lexeme != '\0')
            {
                printf("    %s (address: %d, type: %d, kind: %d)\n",
                       classScope->fields[i].lexeme,
                       classScope->fields[i].address,
                       classScope->fields[i].type,
                       classScope->fields[i].kind);
            }
        }

        // Print statics
        printf("  Statics:\n");
        for (int i = 0; i < classScope->staticCount; i++)
        {
            if (*classScope->statics[i].lexeme != '\0')
            {
                printf("    %s (address: %d, type: %d, kind: %d)\n",
                       classScope->statics[i].lexeme,
                       classScope->statics[i].address,
                       classScope->statics[i].type,
                       classScope->statics[i].kind);
            }
        }

        // Print subroutines and their symbols
        SubroutineScope *currentSub = classScope->subroutineScopes;
        int j = 0;
        while (currentSub)
        {

            Scope *subScope = currentSub->scope;
            printf("  Subroutine: %s (kind: %d type: %d)\n", subScope->name, classScope->subroutines[j].kind, classScope->subroutines[j].type);

            printf("    Arguments:\n");
            for (int i = 0; i < subScope->argumentCount; i++)
            {

                if (*subScope->arguments[i].lexeme != '\0')
                {
                    printf("      %s (address: %d, type: %d, kind: %d)\n",
                           subScope->arguments[i].lexeme,
                           subScope->arguments[i].address,
                           subScope->arguments[i].type,
                           subScope->arguments[i].kind);
                }
            }

            printf("    Locals:\n");
            for (int i = 0; i < subScope->localCount; i++)
            {
                if (*subScope->locals[i].lexeme != '\0')
                {
                    printf("      %s (address: %d, type: %d, kind: %d)\n",
                           subScope->locals[i].lexeme,
                           subScope->locals[i].address,
                           subScope->locals[i].type,
                           subScope->locals[i].kind);
                }
            }

            currentSub = currentSub->next;
            j++;
        }

        entry = entry->next;
    }
}
Scope *getSubroutineScope(SymbolTableManager *manager, const char *className, const char *subroutineName)
{
    // First find the class
    ClassEntry *classEntry = manager->classList;
    while (classEntry != NULL)
    {
        if (strcmp(classEntry->name, className) == 0)
        {
            // Found the class, now search its subroutines
            Scope *classScope = classEntry->classScope;

            // Check if the subroutine exists in the class declarations
            for (int i = 0; i < classScope->subroutineCount; i++)
            {
                if (strcmp(classScope->subroutines[i].lexeme, subroutineName) == 0)
                {
                    // Now look for the implementation scope in subroutineScopes
                    SubroutineScope *subScope = classScope->subroutineScopes;
                    while (subScope != NULL)
                    {
                        if (strcmp(subScope->scope->name, subroutineName) == 0)
                        {
                            return subScope->scope;
                        }
                        subScope = subScope->next;
                    }
                    return NULL; // Subroutine declared but no implementation scope found
                }
            }
            return NULL; // Subroutine not found in class
        }
        classEntry = classEntry->next;
    }
    return NULL; // Class not found
}