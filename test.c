#include "symbols.h"
#include <stdio.h>
#include <string.h>

void printSymbol(const Symbol *symbol) {
  printf("Symbol: %s, Type: %s, Kind: %s, Address: %d\n", symbol->lexeme,
         symbol->type, symbol->kind, symbol->address);
}

void printSubroutine(const Subroutine *sub) {
  printf("Subroutine: %s, Type: %s, Return: %s\n", sub->name,
         sub->subroutineType, sub->returnType);
  printf("Arguments (%d):\n", sub->argumentCount);
  for (int i = 0; i < sub->argumentCount; i++) {
    printf("  ");
    printSymbol(&sub->arguments[i]);
  }
  printf("Locals (%d):\n", sub->localCount);
  for (int i = 0; i < sub->localCount; i++) {
    printf("  ");
    printSymbol(&sub->locals[i]);
  }
}

void printClass(const Class *cls) {
  printf("Class: %s\n", cls->name);

  printf("Fields (%d):\n", cls->fieldCount);
  for (int i = 0; i < cls->fieldCount; i++) {
    printf("  ");
    printSymbol(&cls->fields[i]);
  }

  printf("Statics (%d):\n", cls->staticCount);
  for (int i = 0; i < cls->staticCount; i++) {
    printf("  ");
    printSymbol(&cls->statics[i]);
  }

  printf("Subroutines (%d):\n", cls->subroutineCount);
  for (int i = 0; i < cls->subroutineCount; i++) {
    printf("  ");
    printSubroutine(&cls->subroutines[i]);
  }
}

int main() {
  printf("=== Initializing Class Manager ===\n");
  ClassManager *manager = initClassManager();
  if (!manager) {
    printf("Failed to initialize class manager!\n");
    return 1;
  }

  printf("\n=== Testing Class Creation ===\n");
  createClass(manager, "TestClass");
  setCurrentClass(manager, "TestClass");

  printf("\n=== Testing Class Symbol Insertion ===\n");
  insertClassSymbol(manager, "field1", "int", "field");
  insertClassSymbol(manager, "field2", "String", "field");
  insertClassSymbol(manager, "static1", "boolean", "static");

  printf("\n=== Testing Subroutine Creation ===\n");
  insertSubroutine(manager, "method", "int", "compute");
  insertSubroutine(manager, "function", "void", "print");
  insertSubroutine(manager, "constructor", "TestClass", "new");

  // Try to insert duplicate (should fail)
  printf("\nAttempting to insert duplicate subroutine:\n");
  int result = insertSubroutine(manager, "method", "int", "compute");
  if (result == -1) {
    printf("Correctly rejected duplicate subroutine\n");
  }

  printf("\n=== Testing Subroutine Symbol Insertion ===\n");
  insertSubroutineSymbol(manager, "x", "int", "argument", "compute");
  insertSubroutineSymbol(manager, "y", "int", "argument", "compute");
  insertSubroutineSymbol(manager, "sum", "int", "local", "compute");
  insertSubroutineSymbol(manager, "i", "int", "local", "print");

  printf("\n=== Testing Symbol Lookup ===\n");
  Symbol *sym = lookUpSymbol(manager, "field1", NULL);
  if (sym) {
    printf("Found class symbol: ");
    printSymbol(sym);
  }

  sym = lookUpSymbol(manager, "x", "compute"); // Should find in compute method
  if (sym) {
    printf("Found subroutine symbol: ");
    printSymbol(sym);
  }

  printf("\n=== Testing Current Class Switching ===\n");
  createClass(manager, "AnotherClass");
  setCurrentClass(manager, "AnotherClass");
  insertClassSymbol(manager, "newField", "float", "field");
  insertSubroutine(manager, "method", "void", "doSomething");

  printf("\n=== Testing Clear Current Class ===\n");
  clearCurrentClass(manager);
  printf("Current class should now be NULL\n");

  printf("\n=== Final State ===\n");
  // Print all classes and their contents
  ClassEntry *entry = manager->classList;
  while (entry) {
    printClass(entry->class);
    entry = entry->next;
    printf("\n");
  }

  return 0;
}