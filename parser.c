#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "parser.h"
#include "compiler.h"

// you can declare prototypes of parser functions below

extern ClassManager *manager;
extern int pass;

ParserInfo classDeclar();
ParserInfo memberDeclar();
ParserInfo classVarDeclar();
ParserInfo type(char *type);
ParserInfo subroutineDeclar();
ParserInfo paramList();
ParserInfo subroutineBody();
ParserInfo statement();
ParserInfo varDeclarStatement();
ParserInfo letStatement();
ParserInfo ifStatement();
ParserInfo whileStatement();
ParserInfo doStatement();
ParserInfo subroutineCall();
ParserInfo expressionList();
ParserInfo returnStatement();
ParserInfo expression();
ParserInfo relationalExpression();
ParserInfo ArithmeticExpression();
ParserInfo term();
ParserInfo factor();
ParserInfo operand();

// Program structure
// classDeclar → class identifier { {memberDeclar} }
ParserInfo classDeclar() {

  ParserInfo pi = {0};

  // class
  Token t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp != RESWORD || strcmp(t.lx, "class")) {
    pi.er = classExpected;
    pi.tk = t;
    return pi;
  }

  // identifier
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp != ID) {
    pi.er = idExpected;
    pi.tk = t;
    return pi;
  }

  if (pass == 0) {
    if (createClass(manager, t.lx) == -1) {
      pi.er = redecIdentifier;
      pi.tk = t;
      return pi;
    }
  } else {
    setCurrentClass(manager, t.lx);
  }

  // opening curly bracket
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp != SYMBOL || strcmp(t.lx, "{")) {
    pi.er = openBraceExpected;
    pi.tk = t;
    return pi;
  }

  // zero or more memberDeclar
  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp != SYMBOL && strcmp(t.lx, "}")) {
    pi = memberDeclar();
    if (pi.er != none) {
      return pi;
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
  }

  // closing curly bracket
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp != SYMBOL || strcmp(t.lx, "}")) {
    pi.er = closeBraceExpected;
    pi.tk = t;
    return pi;
  }
  // return the parser info struct

  clearCurrentClass(manager);
  pi.tk = t;
  return pi;
}

// memberDeclar → classVarDeclar | subroutineDeclar
ParserInfo memberDeclar() {

  ParserInfo pi = {0};
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  // ClassVardDeclar check if toke is one of static, field
  if (t.tp == RESWORD && (!strcmp(t.lx, "static") || !strcmp(t.lx, "field"))) {
    pi = classVarDeclar();
    if (pi.er != none) {
      return pi;
    }
  }
  // subroutineDeclar check if token is one of constructor,function, method
  else if (t.tp == RESWORD &&
           (!strcmp(t.lx, "constructor") || !strcmp(t.lx, "function") ||
            !strcmp(t.lx, "method"))) {
    pi = subroutineDeclar();
    if (pi.er != none) {
      return pi;
    }
  } else {
    pi.er = memberDeclarErr;
    pi.tk = t;
    return pi;
  }
  pi.tk = t;
  return pi;
}

// classVarDeclar → (static | field) type identifier {, identifier} ;
ParserInfo classVarDeclar() {
  ParserInfo pi = {0};
  Token t = GetNextToken();
  char type_var[128] = {0};
  char kind_var[128] = {0};
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  // Check if first token is static or field
  if (t.tp == RESWORD && !strcmp(t.lx, "static")) {
    strcpy(kind_var, "static");
  } else if (t.tp == RESWORD && !strcmp(t.lx, "field")) {
    strcpy(kind_var, "this");
  } else {
    pi.er = classVarErr;
    pi.tk = t;
    return pi;
  }

  // peek the next token for the symbol table to check type has been defined
  t = PeekNextToken();
  // Check for type
  pi = type(type_var);
  if (pi.er != none) {
    return pi;
  }
  // check if token is identifier
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == ID) {
    if (pass == 1) {
      int res = insertClassSymbol(manager, t.lx, type_var, kind_var);
      if (res < 0) {
        fprintf(stderr, "Error adding symbol to class\n");
        if (res == -3 || res == -4) {
          pi.er = redecIdentifier;
          pi.tk = t;
          return pi;
        }
      }
    }
  } else {
    pi.er = idExpected;
    pi.tk = t;
    return pi;
  }
  // Check for 0 or more commas followed by identifiers
  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp == SYMBOL && !strcmp(t.lx, ",")) {
    // if next token is a comma then there will be a command then identifier
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    // Eat comma
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp == ID) {
      if (pass == 1) {
        int res = insertClassSymbol(manager, t.lx, type_var, kind_var);
        if (res != 0) {
          fprintf(stderr, "Error adding symbol to class\n");
          if (res == -3 || res == -4) {
            pi.er = redecIdentifier;
            pi.tk = t;
            return pi;
          }
        }
      }
    } else {
      pi.er = idExpected;
      pi.tk = t;
      return pi;
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
  }
  // Check for semicolon
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, ";")) {
    ;
  } else {
    pi.er = semicolonExpected;
    pi.tk = t;
    return pi;
  }
  pi.tk = t;
  return pi;
}

// type → int | char | boolean | identifier
ParserInfo type(char *type_var) {
  ParserInfo pi = {0};
  Token t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == ID) {
    if (pass >= 1) {
      if (classExists(manager, t.lx) == 1) {
        strcpy(type_var, t.lx);
      } else {
        pi.er = undecIdentifier;
        pi.tk = t;
        return pi;
      }
    }

  } else if (t.tp == RESWORD &&
             (!strcmp(t.lx, "int") || !strcmp(t.lx, "char") ||
              !strcmp(t.lx, "boolean"))) {
    strcpy(type_var, t.lx);
  } else {
    pi.er = illegalType;
    pi.tk = t;
    return pi;
  }
  pi.tk = t;
  return pi;
}

// subroutineDeclar → (constructor | function | method) (type|void) identifier
// (paramList) subroutineBody
ParserInfo subroutineDeclar() {
  ParserInfo pi = {0};
  char type_var[128] = {0};
  char kind_var[128] = {0};

  // (constructor | function | method))
  Token t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == RESWORD &&
      (!strcmp(t.lx, "constructor") || !strcmp(t.lx, "function") ||
       !strcmp(t.lx, "method"))) {
    strcpy(kind_var, t.lx);
  } else {

    pi.er = subroutineDeclarErr;
    pi.tk = t;
    return pi;
  }
  // (type|void)

  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == RESWORD && !strcmp(t.lx, "void")) {
    // eat void
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    strcpy(type_var, t.lx);
  }

  else {
    pi = type(type_var);
    if (pi.er != none) {
      return pi;
    }
  }
  // identifier
  char subroutineName[128] = {0};
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == ID) {
    if (pass == 1) {
      int res = insertSubroutine(manager, kind_var, type_var, t.lx);
      if (res < 0) {
        fprintf(stderr, "Error adding symbol to subroutine err code:%d\n", res);
        if (res == -3 || res == -4) {
          pi.er = redecIdentifier;
          pi.tk = t;
          return pi;
        }
      }
    } else if (pass > 1) {
      setCurrentSubroutine(manager, t.lx);
      strcpy(subroutineName, t.lx);
    }
  } else {
    pi.er = idExpected;
    pi.tk = t;
    return pi;
  }

  // (paramList) with brackets at characterse
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == SYMBOL && !strcmp(t.lx, "(")) {
    ;
  } else {
    pi.er = openParenExpected;
    pi.tk = t;
    return pi;
  }
  pi = paramList();
  if (pi.er != none) {
    return pi;
  }
  // closing bracket

  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, ")")) {
    ;
  } else {

    pi.er = closeParenExpected;
    pi.tk = t;
    return pi;
  }

  if (pass == 3) {
    writeFunction(manager->currentClass->name, subroutineName,
                  manager->currentSubroutine->localCount);
    if (strcmp(kind_var, "constructor") == 0) {
      int fieldCount = manager->currentClass->fieldCount;
      writePush("constant", fieldCount);
      writeCall("Memory", "alloc", 1);
      writePop("pointer", 0);
    }
    if (strcmp(kind_var, "method") == 0) {
      writePush("argument", 0);
      writePop("pointer", 0);
    }
  }

  // subroutineBody
  pi = subroutineBody();
  if (pi.er != none) {
    return pi;
  }
  clearCurrentSubroutine(manager);
  pi.tk = t;
  return pi;
}

// paramList → type identifier {, type identifier} | ε
ParserInfo paramList() {
  ParserInfo pi = {0};
  char type_var[128] = {0};
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  // check if next token is going to be type or if it is empty
  if (t.tp == ID ||
      (t.tp == RESWORD && (!strcmp(t.lx, "int") || !strcmp(t.lx, "char") ||
                           !strcmp(t.lx, "boolean")))) {
    // check for type
    t = PeekNextToken();
    pi = type(type_var);
    if (pi.er != none) {
      return pi;
    }
    // check for indentifier
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp == ID) {
      if (pass == 1) {
        int res = insertSubroutineSymbol(manager, t.lx, type_var, "argument",
                                         manager->currentSubroutine->name);
        if (res < 0) {
          fprintf(stderr, "Error adding symbol to subroutine err code:%d\n",
                  res);
          if (res == -3 || res == -4) {
            pi.er = redecIdentifier;
            pi.tk = t;
            return pi;
          }
        }
      }
    } else {
      pi.tk = t;
      pi.er = idExpected;
      return pi;
    }

    // check for 0 or more , type identifier
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    while (t.tp == SYMBOL && !strcmp(t.lx, ",")) {
      t = GetNextToken();
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      // eat the comma
      t = PeekNextToken();
      pi = type(type_var);
      if (pi.er != none) {
        return pi;
      }
      t = GetNextToken();
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      if (t.tp == ID) {
        if (pass == 1) {
          int res = insertSubroutineSymbol(manager, t.lx, type_var, "argument",
                                           manager->currentSubroutine->name);
          if (res < 0) {
            fprintf(stderr, "Error adding symbol to subroutine err code:%d\n",
                    res);
            if (res == -3 || res == -4) {
              pi.er = redecIdentifier;
              pi.tk = t;
              return pi;
            }
          }
        }
      } else {
        pi.tk = t;
        pi.er = idExpected;
        return pi;
      }
      t = PeekNextToken();
      if (t.ec)
        return (ParserInfo){lexerErr, t};
    }
  }

  pi.tk = t;
  return pi;
}

// subroutineBody → { {statement} }
ParserInfo subroutineBody() {

  ParserInfo pi = {0};
  Token t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, "{")) {
    ;
  } else {
    pi.er = openBraceExpected;
    pi.tk = t;
    return pi;
  }

  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp != SYMBOL && strcmp(t.lx, "}")) {
    pi = statement();
    if (pi.er != none) {
      return pi;
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
  }

  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, "}")) {
    ;
  } else {
    pi.er = closeBraceExpected;
    pi.tk = t;
    return pi;
  }
  pi.tk = t;
  return pi;
}

// Statement grammar

// statement → varDeclarStatement | letStatemnt | ifStatement | whileStatement |
// doStatement | returnStatemnt
ParserInfo statement() {
  ParserInfo pi = {0};
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  // all are type RESWORD

  // varDeclar
  if (t.tp == RESWORD && !strcmp(t.lx, "var")) {
    pi = varDeclarStatement();
    if (pi.er != none) {
      return pi;
    }
  }

  // let
  else if (t.tp == RESWORD && !strcmp(t.lx, "let")) {
    pi = letStatement();
    if (pi.er != none) {
      return pi;
    }
  }
  // if
  else if (t.tp == RESWORD && !strcmp(t.lx, "if")) {
    pi = ifStatement();
    if (pi.er != none) {
      return pi;
    }
  }
  // while
  else if (t.tp == RESWORD && !strcmp(t.lx, "while")) {
    pi = whileStatement();
    if (pi.er != none) {
      return pi;
    }
  }
  // do
  else if (t.tp == RESWORD && !strcmp(t.lx, "do")) {
    pi = doStatement();
    if (pi.er != none) {
      return pi;
    }
  }
  // return
  else if (t.tp == RESWORD && !strcmp(t.lx, "return")) {
    pi = returnStatement();
    if (pi.er != none) {
      return pi;
    }
  } else {
    pi.tk = t;
    // probably not correct error
    pi.er = syntaxError;
    return pi;
  }
  pi.tk = t;
  return pi;
}

// varDeclarStatement → var type identifier { , identifier } ;
ParserInfo varDeclarStatement() {

  ParserInfo pi = {0};
  char type_var[128] = {0};
  Token t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == RESWORD && !strcmp(t.lx, "var")) {
    ;
  } else {
    pi.tk = t;
    pi.er = syntaxError;
    return pi;
  }
  t = PeekNextToken();
  pi = type(type_var);
  if (pi.er != none) {
    return pi;
  }
  // Find what the type is
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == ID) {
    if (pass == 1) {
      int res = insertSubroutineSymbol(manager, t.lx, type_var, "local",
                                       manager->currentSubroutine->name);
      if (res < 0) {
        fprintf(stderr, "Error adding symbol to subroutine err code:%d\n", res);
        if (res == -3 || res == -4) {
          pi.er = redecIdentifier;
          pi.tk = t;
          return pi;
        }
      }
    }
  }
  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp == SYMBOL && !strcmp(t.lx, ",")) {
    // if next token is a comma then there will be a command then identifier

    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};

    // Eat comma
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp == ID) {
      if (pass == 1) {
        int res = insertSubroutineSymbol(manager, t.lx, type_var, "local",
                                         manager->currentSubroutine->name);
        if (res < 0) {
          fprintf(stderr, "Error adding symbol to subroutine err code:%d\n",
                  res);
          if (res == -3 || res == -4) {
            pi.er = redecIdentifier;
            pi.tk = t;
            return pi;
          }
        }
      }
    } else {
      pi.er = idExpected;
      pi.tk = t;
      return pi;
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
  }

  // check for semicolon
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == SYMBOL && !strcmp(t.lx, ";")) {
    ;
  } else {
    pi.er = semicolonExpected;
    pi.tk = t;
    return pi;
  }
  pi.tk = t;
  return pi;
}

// letStatemnt → let identifier [ [ expression ] ] = expression ;
ParserInfo letStatement() {
  ParserInfo pi = {0};
  Token t = GetNextToken();
  int isArrayAsignment = 0;
  char varName[128] = {0};
  Symbol *symbol = NULL;
  char outKind[128] = {0};

  if (t.ec)
    return (ParserInfo){lexerErr, t};

  // Check for 'let' keyword
  if (!(t.tp == RESWORD && !strcmp(t.lx, "let"))) {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }

  // Get identifier
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp != ID) {
    pi.er = idExpected;
    pi.tk = t;
    return pi;
  }

  strcpy(varName, t.lx);

  if (pass >= 2) {
    symbol = lookUpSymbol(manager, varName, manager->currentSubroutine->name,
                          outKind);
    if (strcmp(outKind, "invalid") == 0 || strcmp(outKind, "not_found") == 0) {
      pi.er = undecIdentifier;
      pi.tk = t;
      return pi;
    }
  }
  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, "[")) {
    isArrayAsignment = 1;
    t = GetNextToken(); // Consume '['
    if (t.ec)
      return (ParserInfo){lexerErr, t};



    pi = expression();
        if (pass == 3) {
      // printf("outkind: %s\n", outKind);
      // printf("symbol kind: %s addr: %d\n", symbol->kind, symbol->address);
      writePush(symbol->kind, symbol->address);
    }
    if (pi.er != none)
      return pi;

    if (pass == 3) {
      writeArithmetic("add");
    }

    t = GetNextToken(); // Expect ']'
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (!(t.tp == SYMBOL && !strcmp(t.lx, "]"))) {
      pi.er = closeBracketExpected;
      pi.tk = t;
      return pi;
    }
  }

  // Check for '='
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && !strcmp(t.lx, "="))) {
    pi.er = equalExpected;
    pi.tk = t;
    return pi;
  }

  // Handle right-hand side expression
  pi = expression();
  if (pi.er != none)
    return pi;

  if (pass == 3) {
    if (isArrayAsignment) {
      writePop("temp", 0);
      writePop("pointer", 1);
      writePush("temp", 0);
      writePop("that", 0);
    } else {

      writePop(symbol->kind, symbol->address);
    }
  }

  // Check for ';'
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && !strcmp(t.lx, ";"))) {
    pi.er = semicolonExpected;
    pi.tk = t;
    return pi;
  }

  pi.tk = t;
  return pi;
}

// ifStatement → if ( expression ) { {statement} } [else { {statement} }]
ParserInfo ifStatement() {
  ParserInfo pi = {0};
  Token t = GetNextToken();
  int ifLabelCounter = 0;

  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (!(t.tp == RESWORD && !strcmp(t.lx, "if"))) {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }

  // check for (
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && !strcmp(t.lx, "("))) {
    pi.er = openParenExpected;
    pi.tk = t;
    return pi;
  }

  // Evaluate condition
  pi = expression();
  if (pi.er != none)
    return pi;

  // check for )
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && !strcmp(t.lx, ")"))) {
    pi.er = closeParenExpected;
    pi.tk = t;
    return pi;
  }

  // VM: generate unique labels
  char trueLabel[64], falseLabel[64], endLabel[64];
  sprintf(trueLabel, "IF_TRUE%d", ifLabelCounter);
  sprintf(falseLabel, "IF_FALSE%d", ifLabelCounter);
  sprintf(endLabel, "IF_END%d", ifLabelCounter);
  ifLabelCounter++;

  if (pass == 3) {
    writeIfGoto(trueLabel); // if-goto IF_TRUE
    writeGoto(falseLabel);  // goto IF_FALSE
    writeLabel(trueLabel);  // label IF_TRUE
  }

  // check for {
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && !strcmp(t.lx, "{"))) {
    pi.er = openBraceExpected;
    pi.tk = t;
    return pi;
  }

  // then-block statements
  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (!(t.tp == SYMBOL && !strcmp(t.lx, "}"))) {
    pi = statement();
    if (pi.er != none)
      return pi;
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
  }

  // consume '}'
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && !strcmp(t.lx, "}"))) {
    pi.er = closeBraceExpected;
    pi.tk = t;
    return pi;
  }

  // check for optional else block
  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  int hasElse = 0;
  if (t.tp == RESWORD && !strcmp(t.lx, "else")) {
    hasElse = 1;

    // consume 'else'
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};

    if (pass == 3) {
      writeGoto(endLabel);    // goto IF_END
      writeLabel(falseLabel); // label IF_FALSE
    }

    // expect '{'
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (!(t.tp == SYMBOL && !strcmp(t.lx, "{"))) {
      pi.er = openBraceExpected;
      pi.tk = t;
      return pi;
    }

    // else-block statements
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    while (!(t.tp == SYMBOL && !strcmp(t.lx, "}"))) {
      pi = statement();
      if (pi.er != none)
        return pi;
      t = PeekNextToken();
      if (t.ec)
        return (ParserInfo){lexerErr, t};
    }

    // consume '}'
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (!(t.tp == SYMBOL && !strcmp(t.lx, "}"))) {
      pi.er = closeBraceExpected;
      pi.tk = t;
      return pi;
    }
  }

  if (pass == 3) {
    if (!hasElse)
      writeLabel(falseLabel); // label IF_FALSE if no else
    else
      writeLabel(endLabel); // label IF_END
  }

  pi.tk = t;
  return pi;
}

// whileStatement → while ( expression ) { {statement} }
ParserInfo whileStatement() {
  int whileLabelCount = 0; // static counter for uniqueness
  int labelIndex = whileLabelCount++;
  char whileExpLabel[64], whileEndLabel[64];
  snprintf(whileExpLabel, sizeof(whileExpLabel), "WHILE_EXP%d", labelIndex);
  snprintf(whileEndLabel, sizeof(whileEndLabel), "WHILE_END%d", labelIndex);

  ParserInfo pi = {0};
  Token t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (!(t.tp == RESWORD && strcmp(t.lx, "while") == 0)) {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }

  // Write WHILE_EXP label
  if (pass == 3)
    writeLabel(whileExpLabel);

  // Expect '('
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && strcmp(t.lx, "(") == 0)) {
    pi.er = openParenExpected;
    pi.tk = t;
    return pi;
  }

  // Parse condition
  pi = expression();
  if (pi.er != none)
    return pi;

  // Expect ')'
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && strcmp(t.lx, ")") == 0)) {
    pi.er = closeParenExpected;
    pi.tk = t;
    return pi;
  }

  // Negate the condition and jump to WHILE_END if false
  if (pass == 3) {
    writeArithmetic("not");     // negate condition
    writeIfGoto(whileEndLabel); // jump out if false
  }

  // Expect '{'
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && strcmp(t.lx, "{") == 0)) {
    pi.er = openBraceExpected;
    pi.tk = t;
    return pi;
  }

  // Parse loop body
  t = PeekNextToken();
  while (!(t.tp == SYMBOL && strcmp(t.lx, "}") == 0)) {
    pi = statement();
    if (pi.er != none)
      return pi;
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
  }

  // Consume '}'
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && strcmp(t.lx, "}") == 0)) {
    pi.er = closeBraceExpected;
    pi.tk = t;
    return pi;
  }

  // Jump back to condition and mark end
  if (pass == 3) {
    writeGoto(whileExpLabel);  // loop back
    writeLabel(whileEndLabel); // end of while
  }

  pi.tk = t;
  return pi;
}

// doStatement → do subroutineCall ;
ParserInfo doStatement() {
  ParserInfo pi = {0};
  Token t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == RESWORD && !strcmp(t.lx, "do")) {
    ;
  } else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
  // check for subroutineCall
  pi = subroutineCall();
  if (pi.er != none) {
    return pi;
  }
  // check for ;
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, ";")) {
    ;
  } else {
    pi.er = semicolonExpected;
    pi.tk = t;
    return pi;
  }
  pi.tk = t;
  return pi;
}

// subroutineCall → identifier [ . identifier ] ( expressionList )
ParserInfo subroutineCall() {
  ParserInfo pi = {0};
  Token t = GetNextToken();
  Symbol *firstSymbol = NULL;
  char className[128] = {0};
  char subroutineName[128] = {0};
  char outKind[128] = {0};

  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp != ID) {
    return (ParserInfo){idExpected, t};
  }

  if (pass >= 2) {
    firstSymbol =
        lookUpSymbol(manager, t.lx, manager->currentSubroutine->name, outKind);
  }

  // Peek ahead to check for "."
  Token dotCheck = PeekNextToken();
  if (dotCheck.tp == SYMBOL && strcmp(dotCheck.lx, ".") == 0) {
    // We have a qualified call: identifier.identifier(...)
    strcpy(className, (firstSymbol && strcmp(outKind, "class") != 0)
                          ? firstSymbol->type
                          : t.lx);
    if (pass == 3 && firstSymbol && strcmp(outKind, "class") != 0) {
      // It's an object method call — push object
      writePush(firstSymbol->kind, firstSymbol->address);
    }

    GetNextToken();     // consume '.'
    t = GetNextToken(); // subroutine name
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp != ID)
      return (ParserInfo){idExpected, t};
    strcpy(subroutineName, t.lx);
  } else {
    // No dot — this is a method call in current class (this.method())
    strcpy(subroutineName, t.lx);
    if (pass == 3) {
      writePush("pointer", 0); // push this
    }
    strcpy(className, manager->currentClass->name);
  }

  // Expect "("
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && !strcmp(t.lx, "("))) {
    return (ParserInfo){openParenExpected, t};
  }

  // Parse expressionList
  pi = expressionList();
  if (pi.er != none)
    return pi;

  // Expect ")"
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (!(t.tp == SYMBOL && !strcmp(t.lx, ")"))) {
    return (ParserInfo){closeParenExpected, t};
  }

  // Code generation: call the subroutine
  if (pass == 3) {
    Subroutine *subroutine = getSubroutine(manager, className, subroutineName);
    int totalArgs = subroutine->argumentCount;
    writeCall(className, subroutineName, totalArgs);

    if (strcmp(subroutine->returnType, "void") == 0) {
      writePop("temp", 0);
    }
  }

  pi.tk = t;
  return pi;
}

// expressionList → expression { , expression } | ε
ParserInfo expressionList() {
  ParserInfo pi = {0};
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, ")")) {
    // expressionList is empty
    return pi;
  }
  // check for expression
  pi = expression();
  if (pi.er != none) {
    return pi;
  }
  // check for 0 or more , expression
  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp == SYMBOL && !strcmp(t.lx, ",")) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t}; // eat the ,
    pi = expression();
    if (pi.er != none) {
      return pi;
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
  }
  pi.tk = t;
  return pi;
}

// returnStatemnt → return [ expression ] ;
ParserInfo returnStatement() {
  ParserInfo pi = {0};
  Token t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == RESWORD && !strcmp(t.lx, "return")) {
    ;
  } else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }
  // check zero or more expressions
  t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  int hasExpr = 0;
  while (t.tp != SYMBOL && strcmp(t.lx, ";")) {
    pi = expression();
    if (pi.er != none) {
      return pi;
    }
    hasExpr = 1;
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
  }
  // check for ;
  t = GetNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, ";")) {
    ;
  } else {
    pi.er = semicolonExpected;
    pi.tk = t;
    return pi;
  }
  if (pass == 3) {
    if (!hasExpr) {
      // void subroutine — push dummy value
      writePush("constant", 0);
    }
    writeReturn();
  }

  pi.tk = t;
  return pi;
}

// Expression grammar

// expression → relationalExpression { ( & | | ) relationalExpression }
ParserInfo expression() {
  char operator[100] = { 0 };
  ParserInfo pi = {0};
  pi = relationalExpression();
  if (pi.er != none) {
    return pi;
  }
  // check for zero or more "and-ed" or "or-ed" relation Expressions
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp == SYMBOL && (!strcmp(t.lx, "&") || !strcmp(t.lx, "|"))) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp == SYMBOL && !strcmp(t.lx, "&")) {
      strcpy(operator, "and");
    } else if (t.tp == SYMBOL && !strcmp(t.lx, "|")) {
      strcpy(operator, "or");
    } else {
      pi.tk = t;
      pi.er = syntaxError;
      return pi;
    }
    // Arithmetic Expression
    pi = ArithmeticExpression();
    if (pi.er != none) {
      return pi;
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (pass == 3 && operator[0] != 0) {
      writeArithmetic(operator);
    }
  }
  pi.tk = t;
  return pi;
}

// relationalExpression → ArithmeticExpression { ( = | > | < )
// ArithmeticExpression }
ParserInfo relationalExpression() {
  char operator[100] = { 0 };
  ParserInfo pi = {0};
  pi = ArithmeticExpression();
  if (pi.er != none) {
    return pi;
  }
  // zero or more symbol then expression
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp == SYMBOL &&
         (!strcmp(t.lx, "=") || !strcmp(t.lx, "<") || !strcmp(t.lx, ">"))) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp == SYMBOL && !strcmp(t.lx, "=")) {
      strcpy(operator, "eq");
    } else if (t.tp == SYMBOL && !strcmp(t.lx, "<")) {
      strcpy(operator, "lt");
    } else if (t.tp == SYMBOL && !strcmp(t.lx, ">")) {
      strcpy(operator, "gt");
    } else {
      pi.tk = t;
      pi.er = syntaxError;
      return pi;
    }
    // Arithmetic Expression
    pi = ArithmeticExpression();
    if (pi.er != none) {
      return pi;
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (pass == 3 && operator[0] != 0) {
      writeArithmetic(operator);
    }
  }
  pi.tk = t;
  return pi;
}

// ArithmeticExpression → term { ( + | - ) term }
ParserInfo ArithmeticExpression() {
  char operator[100] = { 0 };
  ParserInfo pi = {0};
  pi = term();
  if (pi.er != none) {
    return pi;
  }
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp == SYMBOL && (!strcmp(t.lx, "+") || !strcmp(t.lx, "-"))) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp == SYMBOL && !strcmp(t.lx, "+")) {
      strcpy(operator, "add");
    } else if (t.tp == SYMBOL && !strcmp(t.lx, "-")) {
      strcpy(operator, "sub");
    }
    pi = term();
    if (pi.er != none) {
      return pi;
    }
    if (pass == 3 && operator[0] != 0) {
      writeArithmetic(operator);
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};

  }
  pi.tk = t;
  return pi;
}

// term → factor { ( * | / ) factor }
ParserInfo term() {

  ParserInfo pi = {0};
  pi = factor();
  char operation[100];
  if (pi.er != none) {
    return pi;
  }
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  while (t.tp == SYMBOL && (!strcmp(t.lx, "*") || !strcmp(t.lx, "/"))) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp == SYMBOL && !strcmp(t.lx, "*")) {
      strcpy(operation, "multiply");
    } else if (t.tp == SYMBOL && !strcmp(t.lx, "/")) {
      strcpy(operation, "divide");
    }
    pi = factor();
    if (pi.er != none) {
      return pi;
    }
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (pass == 3 && operation[0] != 0) {
      writeCall("Math", operation, 2);
    }
  }
  pi.tk = t;
  return pi;
}

// factor → ( - | ~ | ε ) operand
ParserInfo factor() {
  char operator[100] = { 0 };
  ParserInfo pi = {0};
  Token t = PeekNextToken();
  if (t.ec)
    return (ParserInfo){lexerErr, t};
  if (t.tp == SYMBOL && !strcmp(t.lx, "-")) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    strcpy(operator, "neg"); // eat the -
  } else if (t.tp == SYMBOL && !strcmp(t.lx, "~")) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    // eat the ~
    strcpy(operator, "not");
  }
  pi = operand();
  if (pi.er != none) {
    return pi;
  }
  if (pass == 3 && operator[0] != 0) {
    writeArithmetic(operator);
  }
  pi.tk = t;
  return pi;
}

// operand → integerConstant | identifier [.identifier ] [ [ expression ] |
// (expressionList) ] | stringLiteral | true | false | null | this
ParserInfo operand() {
  ParserInfo pi = {0};
  Token t = PeekNextToken();
  Symbol *firstSymbol = NULL;
  char className[128] = {0};
  char identifier[128] = {0};
  char methodName[128] = {0};

  if (t.ec)
    return (ParserInfo){lexerErr, t};

  if (t.tp == INT) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (pass == 3) {
      writePush("constant", atoi(t.lx));
    }

  } else if (t.tp == ID) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};

    strcpy(identifier, t.lx);

    if (pass >= 2) {
      char outKind[128] = {0};
      firstSymbol = lookUpSymbol(manager, identifier,
                                 manager->currentSubroutine->name, outKind);
      if (strcmp(outKind, "invalid") == 0 ||
          strcmp(outKind, "not_found") == 0) {
        pi.er = undecIdentifier;
        pi.tk = t;
        return pi;
      }
      if (strcmp(outKind, "class") == 0) {
        strcpy(className, t.lx);
      }
    }

    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};

    // Check for method or static call
    if (t.tp == SYMBOL && !strcmp(t.lx, ".")) {
      t = GetNextToken(); // consume '.'
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      t = GetNextToken(); // get method name
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      if (t.tp == ID) {
        strcpy(methodName, t.lx);
        char outKind[128] = {0};

        if (pass >= 2) {
          if (strlen(className) > 0) {
            // static call: className.methodName
            if (!classExists(manager, className)) {
              pi.er = undecIdentifier;
              pi.tk = t;
              return pi;
            }
          } else {
            // method call: object.methodName
            Class *curr = manager->currentClass;
            setCurrentClass(manager, firstSymbol->type);
            lookUpSymbol(manager, methodName, NULL, outKind);
            if (strcmp(outKind, "invalid") == 0 ||
                strcmp(outKind, "not_found") == 0) {
              pi.er = undecIdentifier;
              pi.tk = t;
              return pi;
            }
            setCurrentClass(manager, curr->name);
          }
        }

        // Peek for '(' to parse argument list
        t = PeekNextToken();
        if (t.ec)
          return (ParserInfo){lexerErr, t};
        if (t.tp == SYMBOL && !strcmp(t.lx, "(")) {
          t = GetNextToken(); // consume '('
          if (t.ec)
            return (ParserInfo){lexerErr, t};
          if (pass == 3 && strlen(className) == 0 && firstSymbol != NULL) {
            writePush(firstSymbol->kind, firstSymbol->address);
          }

          ParserInfo pi = expressionList();
          if (pi.er != none)
            return pi;

          t = GetNextToken(); // consume ')'
          if (t.ec)
            return (ParserInfo){lexerErr, t};
          if (!(t.tp == SYMBOL && !strcmp(t.lx, ")"))) {
            pi.er = closeParenExpected;
            pi.tk = t;
            return pi;
          }

          if (pass == 3) {

            if (strlen(methodName) > 0) {
              if (strlen(className) > 0) {
                Subroutine *subroutine =
                    getSubroutine(manager, className, methodName);
                int nArgs = subroutine->argumentCount;
                writeCall(className, methodName, nArgs);
                if (strcmp(subroutine->returnType, "void") == 0) {
                  writePop("temp", 0);
                }
              } else if (firstSymbol != NULL) {
                Subroutine *subroutine =
                    getSubroutine(manager, firstSymbol->type, methodName);
                int nArgs = subroutine->argumentCount;
                writeCall(firstSymbol->type, methodName, nArgs);
                if (strcmp(subroutine->returnType, "void") == 0) {
                  writePop("temp", 0);
                }
              } else {
                Subroutine *subroutine = getSubroutine(
                    manager, manager->currentClass->name, methodName);
                int nArgs = subroutine->argumentCount;
                writeCall(manager->currentClass->name, methodName, nArgs);
                if (strcmp(subroutine->returnType, "void") == 0) {
                  writePop("temp", 0);
                }
              }
            }
          }

          return pi;
        }
      } else {
        pi.er = idExpected;
        pi.tk = t;
        return pi;
      }
    }

    // Check for array access
    t = PeekNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (t.tp == SYMBOL && !strcmp(t.lx, "[")) {
      t = GetNextToken(); // consume '['
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      pi = expression();
      if (pi.er != none)
        return pi;

      t = GetNextToken(); // consume ']'
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      if (!(t.tp == SYMBOL && !strcmp(t.lx, "]"))) {
        pi.er = closeBracketExpected;
        pi.tk = t;
        return pi;
      }

      if (pass == 3) {

        writePush(firstSymbol->kind, firstSymbol->address);

        writeArithmetic("add");
        writePop("pointer", 1);
        writePush("that", 0);
      }
      return pi;
    }

    // Check for standalone variable (not array or method call)
    if (pass == 3 && firstSymbol) {
      writePush(firstSymbol->kind, firstSymbol->address);
    }

  } else if (t.tp == SYMBOL && !strcmp(t.lx, "(")) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    pi = expression();
    if (pi.er != none)
      return pi;
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    if (!(t.tp == SYMBOL && !strcmp(t.lx, ")"))) {
      pi.er = closeParenExpected;
      pi.tk = t;
      return pi;
    }

  } else if (t.tp == STRING) {
    t = GetNextToken();
    if (t.ec)
      return (ParserInfo){lexerErr, t};
    int strLength = strlen(t.lx);
    writePush("constant", strLength);
    writeCall("String", "new", 1);
    for (int i = 0; i < strLength; i++) {
      writePush("constant", t.lx[i]);
      writeCall("String", "appendChar", 2);
    }

  } else if (t.tp == RESWORD) {
    if (!strcmp(t.lx, "true")) {
      t = GetNextToken();
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      writePush("constant", 0);
      writeArithmetic("not");
    } else if (!strcmp(t.lx, "false") || !strcmp(t.lx, "null")) {
      t = GetNextToken();
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      writePush("constant", 0);
    } else if (!strcmp(t.lx, "this")) {
      t = GetNextToken();
      if (t.ec)
        return (ParserInfo){lexerErr, t};
      writePush("pointer", 0);
    }
  } else {
    pi.er = syntaxError;
    pi.tk = t;
    return pi;
  }

  pi.tk = t;
  return pi;
}

int InitParser(char *file_name) { return InitLexer(file_name); }

ParserInfo Parse() {
  ParserInfo pi = {0};
  pi = classDeclar();

  return pi;
}

int StopParser() { return StopLexer(); }

// #ifndef TEST_PARSER
// int main()
// {

// 	return 1;
// }
// #endif
