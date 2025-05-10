
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "symbols.h"

ClassManager *initClassManager() {
    ClassManager *manager = (ClassManager *)malloc(sizeof(ClassManager));
    manager->currentClass = NULL;
    manager->classList = NULL;
    return manager;
}


/**
 * @brief Creates a class if not in classList already
 * 
 * @param manager Pointer to manager
 * @param className Name of class to be added
 * @return 1 for success, -1 for already defined
 */
int createClass(ClassManager *manager, const char *className) {
    // Check if class is already in classList
    ClassEntry *entry = manager->classList;
    while(entry){
     if (strcmp(entry->name, className) == 0)
        {
            fprintf(stderr, "Error: Class '%s' already defined\n", className);
            return -1;
        }
        entry = entry->next;
    }

    // Create new class entry
    ClassEntry *newEntry = (ClassEntry*)malloc(sizeof(ClassEntry));
    strcpy(newEntry->name, className);

    // Create new class 
    Class *class = (Class *)malloc(sizeof(Class));
    strcpy(class->name, className);

    // Zero all values in class
    class->fieldCount = 0;
    class->staticCount = 0;
    class->subroutineCount = 0;

    memset(class->fields, 0, sizeof(class->fields));
    memset(class->statics, 0, sizeof(class->statics));
    memset(class->subroutines, 0, sizeof(class->subroutines));

    // Add class and next to class
    newEntry->class = class;
    newentry->next = manager->classList;

    // Add new class to classList and set current class to newly made class
    manager->classList = newEntry;
    manager->currentClass = class;

}


/**
 * @brief sets the current class in manger to provided class
 * 
 * @param manager Pointer to manager
 * @param className Name of class to be added
 * @return void
 */
void setCurrentClass(ClassManager *manager, const char *className) {
    ClassEntry *entry = manager->classList;
    while (entry)
    {
        if (strcmp(entry->name, className) == 0)
        {
            manager->currentClass = entry->class;
            return;
        }
        entry = entry->next;
    }
    fprintf(stderr, "Error: Cannot switch to class '%s' as not defined\n", className);
}


void clearCurrentClass(ClassManager *manager) {
    manger->currentClass = NULL;
    return;
}

int insertSubroutine(
    const char *subroutineType,
    const char *returnType,
    const char* name,
) {
    int subroutineCount = manager->currentClass->subroutineCount;
    // check if a subroutine with this name already exists
    for (int i = 0; i < subroutineCount; i++) {
        if (strcmp(manager->currentClass->subroutines[i].name, name) == 0) {
            // Subroutine with this name already exists
            printf("Error: Subroutine '%s' is already declared\n", name);
            return -1;  // Return error code
        }
    }


    // Create subroutine and zero values in it
    Subroutine *subroutine = &manager->currentClass->subroutines[subroutineCount];
    strcpy(subroutine->subroutineType,subroutineType);
    strcpy(subroutine->returnType,returnType);
    strcpy(subroutine->name,name);

    subroutine->argumentCount = 0;
    subroutine->localCount = 0;

    memst(subroutine->arguments, 0, sizeof(subroutine->arguments));
    memst(subroutine->locals, 0, sizeof(subroutine->locals));

    // Check if subroutine is a method or a constructor as they will
    // have an implicit this argument that needs to be added
    if (strcmp(subroutineType, "method") == 0 || strcmp(subroutineType, "constructor") == 0) 
    {
        strcpy(subroutine->arguments[0].lexeme, "this");
        strcpy(subroutine->arguments[0].type, manager->currentClass->name);
        strcpy(subroutine->arguments[0].kind, "argument");
        subroutine->arguments[0].address = subroutine->argumentCount++;
    }

    // Return the index of the newly added subroutine
    return manager->currentClass->subroutineCount++;
}

int insertSubroutineSymbol(
    const char* lexeme,
    const char* type,
    const char* kind,

    const char* subroutineName
) {
    ;
}

int insertClassSymbol(
    const char* lexeme,
    const char* type,
    const char* kind,
) {
    ;
}

Symbol *lookUpSymbol(const char *name, const char* subroutineName) 
{
    ;
}