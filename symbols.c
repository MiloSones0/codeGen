
#include "symbols.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>





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
  while (entry) {
    if (strcmp(entry->name, className) == 0) {
      fprintf(stderr, "Error: Class '%s' already defined\n", className);
      return -1;
    }
    entry = entry->next;
  }

  // Create new class entryP
  ClassEntry *newEntry = (ClassEntry *)malloc(sizeof(ClassEntry));
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
  newEntry->next = manager->classList;

  // Add new class to classList and set current class to newly made class
  manager->classList = newEntry;
  manager->currentClass = class;
  return 0;
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
  while (entry) {
    // printf("%s:%s\n", entry->name, className);
    if (strcmp(entry->name, className) == 0) {
      manager->currentClass = entry->class;
      return;
    }
    entry = entry->next;
  }
  fprintf(stderr, "Error: Cannot switch to class '%s' as not defined\n",
          className);
}

void clearCurrentClass(ClassManager *manager) {
  manager->currentClass = NULL;
  return;
}

int setCurrentSubroutine(ClassManager *manager, const char *subroutineName) {
  // Validate inputs
  if (!manager || !manager->currentClass || !subroutineName) {
    fprintf(stderr, "Error: Invalid parameters to setCurrentSubroutine\n");
    return -1;
  }

  // Search for subroutine
  for (int i = 0; i < manager->currentClass->subroutineCount; i++) {
    if (strcmp(manager->currentClass->subroutines[i].name, subroutineName) ==
        0) {
      manager->currentSubroutine = &manager->currentClass->subroutines[i];
      return 0; // Success
    }
  }

  // Not found
  fprintf(stderr, "Error: Subroutine '%s' not found in class '%s'\n",
          subroutineName, manager->currentClass->name);
  return -1;
}

void clearCurrentSubroutine(ClassManager *manager) {
  manager->currentSubroutine = NULL;
  return;
}

int insertSubroutine(ClassManager *manager, const char *subroutineType,
                     const char *returnType, const char *name) {
  int subroutineCount = manager->currentClass->subroutineCount;
  // check if a subroutine with this name already exists
  for (int i = 0; i < subroutineCount; i++) {
    if (strcmp(manager->currentClass->subroutines[i].name, name) == 0) {
      // Subroutine with this name already exists
      printf("Error: Subroutine '%s' is already declared\n", name);
      return -1; // Return error code
    }
  }

  // Create subroutine and zero values in it
  Subroutine *subroutine = &manager->currentClass->subroutines[subroutineCount];
  strcpy(subroutine->subroutineType, subroutineType);
  strcpy(subroutine->returnType, returnType);
  strcpy(subroutine->name, name);

  subroutine->argumentCount = 0;
  subroutine->localCount = 0;

  memset(subroutine->arguments, 0, sizeof(subroutine->arguments));
  memset(subroutine->locals, 0, sizeof(subroutine->locals));

  // Check if subroutine is a method or a constructor as they will
  // have an implicit this argument that needs to be added
  if (strcmp(subroutineType, "method") == 0) {
    strcpy(subroutine->arguments[0].lexeme, "this");
    strcpy(subroutine->arguments[0].type, manager->currentClass->name);
    strcpy(subroutine->arguments[0].kind, "argument");
    subroutine->arguments[0].address = subroutine->argumentCount++;
  }

  manager->currentSubroutine = subroutine;
  // Return the index of the newly added subroutine

  return manager->currentClass->subroutineCount++;
}

int insertSubroutineSymbol(ClassManager *manager, const char *lexeme,
                           const char *type, const char *kind,
                           const char *subroutineName) {
  if (!manager || !manager->currentClass || !lexeme || !type || !kind ||
      !subroutineName) {
    fprintf(stderr,
            "Error: insertSubroutineSymbol received invalid parameters\n");
    return -1;
  }

  // Find the target subroutine
  Subroutine *subroutine = NULL;
  for (int i = 0; i < manager->currentClass->subroutineCount; i++) {
    if (strcmp(manager->currentClass->subroutines[i].name, subroutineName) ==
        0) {
      subroutine = &manager->currentClass->subroutines[i];
      break;
    }
  }

  if (!subroutine) {
    fprintf(stderr, "Error: Subroutine '%s' not found in class '%s'\n",
            subroutineName, manager->currentClass->name);
    return -2;
  }

  // Check for duplicate symbol
  for (int i = 0; i < subroutine->argumentCount; i++) {
    if (strcmp(subroutine->arguments[i].lexeme, lexeme) == 0) {
      fprintf(stderr,
              "Error: Duplicate argument symbol '%s' in subroutine '%s'\n",
              lexeme, subroutineName);
      return -3;
    }
  }
  for (int i = 0; i < subroutine->localCount; i++) {
    if (strcmp(subroutine->locals[i].lexeme, lexeme) == 0) {
      fprintf(stderr, "Error: Duplicate local symbol '%s' in subroutine '%s'\n",
              lexeme, subroutineName);
      return -4;
    }
  }

  Symbol *targetArray;
  int *count;
  int address;

  if (strcmp(kind, "argument") == 0) {
    targetArray = subroutine->arguments;
    count = &subroutine->argumentCount;
    address = *count;
  } else if (strcmp(kind, "local") == 0) {
    targetArray = subroutine->locals;
    count = &subroutine->localCount;
    address = *count;
  } else {
    fprintf(stderr, "Error: Unknown symbol kind '%s' for '%s'\n", kind, lexeme);
    return -5;
  }

  if (*count >= 128) {
    fprintf(stderr,
            "Error: Symbol table overflow in subroutine '%s' for kind '%s'\n",
            subroutineName, kind);
    return -6;
  }

  Symbol *symbol = &targetArray[*count];
  strcpy(symbol->lexeme, lexeme);
  strcpy(symbol->type, type);
  strcpy(symbol->kind, kind);
  symbol->address = address;
  (*count)++;

  return 0;
}

int insertClassSymbol(ClassManager *manager, const char *lexeme,
                      const char *type, const char *kind) {
  // Validate inputs
  if (!manager || !manager->currentClass || !lexeme || !type || !kind) {
    return -1; // Invalid parameters
  }

  // Determine which symbol array to use based on kind
  Symbol *targetArray;
  int *count;
  int address;

  if (strcmp(kind, "this") == 0) {
    targetArray = manager->currentClass->fields;
    count = &manager->currentClass->fieldCount;
    address = *count; // Fields get addresses starting from 0
  } else if (strcmp(kind, "static") == 0) {
    targetArray = manager->currentClass->statics;
    count = &manager->currentClass->staticCount;
    address = *count; // Statics get addresses starting from 0
  } else {
    return -2; // Invalid kind (must be "field" or "static")
  }

  // Check for duplicate symbol in class scope
  for (int i = 0; i < manager->currentClass->fieldCount; i++) {
    if (strcmp(manager->currentClass->fields[i].lexeme, lexeme) == 0) {
      return -3; // Duplicate in fields
    }
  }
  for (int i = 0; i < manager->currentClass->staticCount; i++) {
    if (strcmp(manager->currentClass->statics[i].lexeme, lexeme) == 0) {
      return -4; // Duplicate in statics
    }
  }

  // Check array bounds
  if (*count >= 128) {
    return -5; // Array full
  }

  // Insert the symbol
  Symbol *symbol = &targetArray[*count];
  strcpy(symbol->lexeme, lexeme);
  strcpy(symbol->type, type);
  strcpy(symbol->kind, kind);
  symbol->address = address++;
  (*count)++;

  return 0; // Success
}

Symbol *lookUpSymbol(ClassManager *manager, const char *name,
                     const char *subroutineName, char *outKind) {
  if (!manager) {
    fprintf(stderr, "Error: lookUpSymbol called with NULL manager\n");
    if (outKind)
      strcpy(outKind, "invalid");
    return NULL;
  }

  if (!manager->currentClass) {
    fprintf(stderr, "Error: lookUpSymbol called with NULL currentClass\n");
    if (outKind)
      strcpy(outKind, "invalid");
    return NULL;
  }

  if (!name || name[0] == '\0') {
    fprintf(stderr, "Error: lookUpSymbol called with invalid name\n");
    if (outKind)
      strcpy(outKind, "invalid");
    return NULL;
  }

  // fprintf(stderr, "Looking up symbol '%s' in class '%s'\n", name,
  //         manager->currentClass->name);

  // Check subroutine scope (args, locals)
  if (subroutineName && manager->currentSubroutine) {
    // fprintf(stderr, "Searching in subroutine '%s' (args, locals)\n",
    // subroutineName);

    for (int j = 0; j < manager->currentSubroutine->argumentCount; j++) {
      if (strcmp(manager->currentSubroutine->arguments[j].lexeme, name) == 0) {
        // fprintf(stderr, "Found symbol '%s' as argument\n", name);
        if (outKind)
          strcpy(outKind, "argument");
        return &manager->currentSubroutine->arguments[j];
      }
    }

    for (int j = 0; j < manager->currentSubroutine->localCount; j++) {
      if (strcmp(manager->currentSubroutine->locals[j].lexeme, name) == 0) {
        // fprintf(stderr, "Found symbol '%s' as local\n", name);
        if (outKind)
          strcpy(outKind, "local");
        return &manager->currentSubroutine->locals[j];
      }
    }
  } else if (subroutineName && !manager->currentSubroutine) {
    fprintf(
        stderr,
        "Warning: Subroutine name provided but currentSubroutine is NULL\n");
  }

  // Check class-level fields
  for (int i = 0; i < manager->currentClass->fieldCount; i++) {
    if (strcmp(manager->currentClass->fields[i].lexeme, name) == 0) {
      // fprintf(stderr, "Found symbol '%s' as class field\n", name);
      if (outKind)
        strcpy(outKind, "this");
      return &manager->currentClass->fields[i];
    }
  }

  // Check class-level statics
  for (int i = 0; i < manager->currentClass->staticCount; i++) {
    if (strcmp(manager->currentClass->statics[i].lexeme, name) == 0) {
      // fprintf(stderr, "Found symbol '%s' as class static\n", name);
      if (outKind)
        strcpy(outKind, "static");
      return &manager->currentClass->statics[i];
    }
  }

  // Check subroutine name match (not returning a pointer)
  for (int i = 0; i < manager->currentClass->subroutineCount; i++) {
    if (strcmp(manager->currentClass->subroutines[i].name, name) == 0) {
      // fprintf(stderr, "Symbol '%s' matches a subroutine name\n", name);
      if (outKind)
        strcpy(outKind, "subroutine");
      return NULL;
    }
  }

  // Check if name matches a class
  if (classExists(manager, name)) {
    // fprintf(stderr, "Symbol '%s' is a class name\n", name);
    if (outKind)
      strcpy(outKind, "class");
    return NULL;
  }

  fprintf(stderr, "Symbol '%s' not found in any scope\n", name);
  if (outKind)
    strcpy(outKind, "not_found");
  return NULL;
}

int classExists(ClassManager *manager, const char *className) {
  if (!manager) {
    fprintf(stderr, "Error: NULL manager pointer\n");
    return -1;
  }
  if (!className || className[0] == '\0') {
    fprintf(stderr, "Error: Invalid class name\n");
    return -1;
  }

  ClassEntry *current = manager->classList;
  while (current != NULL) {
    if (current->class && strcmp(current->name, className) == 0) {
      return 1;
    }
    current = current->next;
  }

  return 0;
}

int getSubroutineArgCount(ClassManager *manager, const char *className,
                          const char *subroutineName) {
  // Look for the class
  ClassEntry *entry = manager->classList;
  while (entry) {
    if (strcmp(entry->name, className) == 0) {
      Class *cls = entry->class;
      for (int i = 0; i < cls->subroutineCount; i++) {
        if (strcmp(cls->subroutines[i].name, subroutineName) == 0) {
          return cls->subroutines[i].argumentCount;
        }
      }
    }
    entry = entry->next;
  }
  return -1; // Not found
}

Subroutine *getSubroutine(ClassManager *manager, const char *className,
                          const char *subroutineName) {
  ClassEntry *entry = manager->classList;
  while (entry) {
    if (strcmp(entry->name, className) == 0) {
      Class *cls = entry->class;
      for (int i = 0; i < cls->subroutineCount; i++) {
        if (strcmp(cls->subroutines[i].name, subroutineName) == 0) {
          return &cls->subroutines[i];
        }
      }
    }
    entry = entry->next;
  }
  return NULL; // Not found
}