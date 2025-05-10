
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
 * @param height Name of class to be added
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



void setCurrentClass(ClassManager *manager, const char *className) {




}
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

Symbol *lookUpSymbol(const char *name);