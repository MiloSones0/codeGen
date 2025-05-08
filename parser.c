#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "parser.h"
#include "compiler.h"

// you can declare prototypes of parser functions below

extern SymbolTableManager *manager;
extern int pass;
extern FILE *fptr;

char tempType[100];

ParserInfo classDeclar();
ParserInfo memberDeclar();
ParserInfo classVarDeclar();
ParserInfo type(SymbolType *type);
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
ParserInfo classDeclar()
{

	ParserInfo pi = {0};

	// class
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp != RESWORD || strcmp(t.lx, "class"))
	{
		pi.er = classExpected;
		pi.tk = t;
		return pi;
	}

	// identifier
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp != ID)
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}
	if (pass == 0)
	{
		startClass(manager, t.lx);
	}
	else
	{
		// set manager to to current class table
		switchClass(manager, t.lx);
	}

	// opening curly bracket
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp != SYMBOL || strcmp(t.lx, "{"))
	{
		pi.er = openBraceExpected;
		pi.tk = t;
		return pi;
	}

	// zero or more memberDeclar
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp != SYMBOL && strcmp(t.lx, "}"))
	{
		pi = memberDeclar();
		if (pi.er != none)
		{
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
	if (t.tp != SYMBOL || strcmp(t.lx, "}"))
	{
		pi.er = closeBraceExpected;
		pi.tk = t;
		return pi;
	}
	// return the parser info struct
	pi.tk = t;
	if (pass == 0)
	{
		endClass(manager);
	}
	return pi;
}

// memberDeclar → classVarDeclar | subroutineDeclar
ParserInfo memberDeclar()
{

	ParserInfo pi = {0};
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	// ClassVardDeclar check if toke is one of static, field
	if (t.tp == RESWORD && (!strcmp(t.lx, "static") || !strcmp(t.lx, "field")))
	{
		pi = classVarDeclar();
		if (pi.er != none)
		{
			return pi;
		}
	}
	// subroutineDeclar check if token is one of constructor,function, method
	else if (t.tp == RESWORD && (!strcmp(t.lx, "constructor") || !strcmp(t.lx, "function") || !strcmp(t.lx, "method")))
	{
		pi = subroutineDeclar();
		if (pi.er != none)
		{
			return pi;
		}
	}
	else
	{
		pi.er = memberDeclarErr;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// classVarDeclar → (static | field) type identifier {, identifier} ;
ParserInfo classVarDeclar()
{
	SymbolKind kind_var = KIND_TEST;
	SymbolType type_var = KIND_TEST;
	char className[128];
	char *subroutineName;
	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	// Check if first token is static or field
	if (t.tp == RESWORD && !strcmp(t.lx, "static"))
	{
		kind_var = KIND_STATIC;
	}
	else if (t.tp == RESWORD && !strcmp(t.lx, "field"))
	{
		kind_var = KIND_FIELD;
	}
	else
	{
		pi.er = classVarErr;
		pi.tk = t;
		return pi;
	}

	// peek the next token for the symbol table to check type has been defined
	t = PeekNextToken();
	// Check for type
	pi = type(&type_var);
	if (pi.er != none)
	{
		return pi;
	}
	if (pass == 1)
	{
		if (type_var == TYPE_IDENTIFIER)
		{
			// check if type has been declared.

			Scope *classScope = lookupClass(manager, t.lx);
			if (classScope == NULL)
			{
				pi.er = undecIdentifier;
				pi.tk = t;
				return pi;
			}
			strncpy(className, t.lx, sizeof(t.lx));
		}
	}
	// check if token is identifier
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == ID)
	{
		if (pass == 1)
		{
			if (lookupSymbolInClass(manager, manager->currentClass->name, t.lx) != NULL)
			{
				pi.er = redecIdentifier;
				pi.tk = t;
				return pi;
			}
			if (type_var == TYPE_IDENTIFIER)
			{
				// className = t.lx;
				subroutineName = NULL;
			}
			else
			{
				strncpy(className, manager->currentClass->name, sizeof(manager->currentClass->name));
				subroutineName = NULL;
			}
			insertSymbol(manager, t.lx, type_var, kind_var, className, subroutineName);
		}
	}
	else
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}
	// Check for 0 or more commas followed by identifiers
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp == SYMBOL && !strcmp(t.lx, ","))
	{
		// if next token is a comma then there will be a command then identifier
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		// Eat comma
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == ID)
		{
			if (pass == 1)
			{
				if (lookupSymbolInClass(manager, manager->currentClass->name, t.lx) != NULL)
				{
					pi.er = redecIdentifier;
					pi.tk = t;
					return pi;
				}
				insertSymbol(manager, t.lx, type_var, kind_var, className, subroutineName);
			}
		}
		else
		{
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
	if (t.tp == SYMBOL && !strcmp(t.lx, ";"))
	{
		;
	}
	else
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// type → int | char | boolean | identifier
ParserInfo type(SymbolType *type)
{
	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == ID)
	{
		*type = TYPE_IDENTIFIER;
	}
	else if (t.tp == RESWORD && !strcmp(t.lx, "int"))
	{

		*type = TYPE_INT;
	}
	else if (t.tp == RESWORD && !strcmp(t.lx, "char"))
	{
		*type = TYPE_CHAR;
	}
	else if (t.tp == RESWORD && !strcmp(t.lx, "boolean"))
	{
		*type = TYPE_BOOL;
	}
	else
	{
		pi.er = illegalType;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// subroutineDeclar → (constructor | function | method) (type|void) identifier (paramList) subroutineBody
ParserInfo subroutineDeclar()
{
	ParserInfo pi = {0};
	SymbolKind kind_var;
	SymbolType type_var;

	// (constructor | function | method))
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == RESWORD && !strcmp(t.lx, "constructor"))
	{
		kind_var = KIND_CONSTRUCTOR;
	}
	else if (t.tp == RESWORD && !strcmp(t.lx, "function"))
	{
		kind_var = KIND_FUNCTION;
	}
	else if (t.tp == RESWORD && !strcmp(t.lx, "method"))
	{
		kind_var = KIND_METHOD;
	}
	else
	{

		pi.er = subroutineDeclarErr;
		pi.tk = t;
		return pi;
	}
	// (type|void)

	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == RESWORD && !strcmp(t.lx, "void"))
	{
		// eat void
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		type_var = TYPE_VOID;
	}

	else
	{

		pi = type(&type_var);
		if (pi.er != none)
		{
			return pi;
		}
	}
	// identifier
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == ID)
	{
		if (pass == 1)
		{
			startSubroutine(manager, t.lx);
			int i = manager->currentClass->subroutineCount - 1;
			manager->currentClass->subroutines[i].kind = kind_var;
			manager->currentClass->subroutines[i].type = type_var;
		}
		else if (pass > 1)
		{
			switchSubroutine(manager, t.lx);
		}
		if (pass == 3)
		{
			writeFunction(manager->currentClass->name, t.lx, manager->currentSubroutine->localCount);
			if (kind_var == KIND_CONSTRUCTOR)
			{
				writePush("constant", manager->currentClass->fieldCount);
				writeCall("Memory", "alloc", 1);
			}
		}
	}
	else
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}

	// (paramList) with brackets at characterse
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == SYMBOL && !strcmp(t.lx, "("))
	{
		;
	}
	else
	{
		pi.er = openParenExpected;
		pi.tk = t;
		return pi;
	}
	pi = paramList();
	if (pi.er != none)
	{
		return pi;
	}
	// closing bracket

	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, ")"))
	{
		if (pass == 1)
		{
			endSubroutine(manager);
		}
	}
	else
	{

		pi.er = closeParenExpected;
		pi.tk = t;
		return pi;
	}

	// subroutineBody
	pi = subroutineBody();
	if (pi.er != none)
	{
		return pi;
	}
	pi.tk = t;
	return pi;
}

// paramList → type identifier {, type identifier} | ε
ParserInfo paramList()
{
	ParserInfo pi = {0};
	SymbolType type_var;
	char className[128];
	char *subroutineName;
	// char className[128];
	// check if paramList is empty
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	// check if next token is going to be type or if it is empty
	if (t.tp == ID || (t.tp == RESWORD && (!strcmp(t.lx, "int") || !strcmp(t.lx, "char") || !strcmp(t.lx, "boolean"))))
	{
		// check for type
		t = PeekNextToken();
		pi = type(&type_var);
		if (pi.er != none)
		{
			return pi;
		}
		strncpy(className, t.lx, sizeof(t.lx));
		// check for indentifier
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == ID)
		{
			if (pass == 1)
			{
				if (type_var == TYPE_IDENTIFIER)
				{

					subroutineName = NULL;
				}
				else
				{
					strncpy(className, manager->currentClass->name, sizeof(manager->currentClass->name));
					subroutineName = NULL;
				}
				insertSymbol(manager, t.lx, type_var, KIND_ARGUMENT, className, subroutineName);
			}
		}
		else
		{
			pi.tk = t;
			pi.er = idExpected;
			return pi;
		}

		// check for 0 or more , type identifier
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		while (t.tp == SYMBOL && !strcmp(t.lx, ","))
		{
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t};
			// eat the comma
			t = PeekNextToken();
			pi = type(&type_var);
			if (pi.er != none)
			{
				return pi;
			}
			strncpy(className, t.lx, sizeof(t.lx));
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t};
			if (t.tp == ID)
			{
				if (pass == 1)
				{
					if (type_var == TYPE_IDENTIFIER)
					{

						subroutineName = NULL;
					}
					else
					{
						strncpy(className, manager->currentClass->name, sizeof(manager->currentClass->name));
						subroutineName = NULL;
					}
					insertSymbol(manager, t.lx, type_var, KIND_ARGUMENT, className, subroutineName);
				}
			}
			else
			{
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
ParserInfo subroutineBody()
{

	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "{"))
	{
		;
	}
	else
	{
		pi.er = openBraceExpected;
		pi.tk = t;
		return pi;
	}

	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp != SYMBOL && strcmp(t.lx, "}"))
	{
		pi = statement();
		if (pi.er != none)
		{
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}

	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "}"))
	{
		;
	}
	else
	{
		pi.er = closeBraceExpected;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// Statement grammar

// statement → varDeclarStatement | letStatemnt | ifStatement | whileStatement | doStatement | returnStatemnt
ParserInfo statement()
{
	ParserInfo pi = {0};
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	// all are type RESWORD

	// varDeclar
	if (t.tp == RESWORD && !strcmp(t.lx, "var"))
	{
		pi = varDeclarStatement();
		if (pi.er != none)
		{
			return pi;
		}
	}

	// let
	else if (t.tp == RESWORD && !strcmp(t.lx, "let"))
	{
		pi = letStatement();
		if (pi.er != none)
		{
			return pi;
		}
	}
	// if
	else if (t.tp == RESWORD && !strcmp(t.lx, "if"))
	{
		pi = ifStatement();
		if (pi.er != none)
		{
			return pi;
		}
	}
	// while
	else if (t.tp == RESWORD && !strcmp(t.lx, "while"))
	{
		pi = whileStatement();
		if (pi.er != none)
		{
			return pi;
		}
	}
	// do
	else if (t.tp == RESWORD && !strcmp(t.lx, "do"))
	{
		pi = doStatement();
		if (pi.er != none)
		{
			return pi;
		}
	}
	// return
	else if (t.tp == RESWORD && !strcmp(t.lx, "return"))
	{
		pi = returnStatement();
		if (pi.er != none)
		{
			return pi;
		}
	}
	else
	{
		pi.tk = t;
		// probably not correct error
		pi.er = syntaxError;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// varDeclarStatement → var type identifier { , identifier } ;
ParserInfo varDeclarStatement()
{

	ParserInfo pi = {0};
	SymbolKind kind_var = KIND_LOCAL;
	SymbolType type_var;
	char className[128];
	char *subroutineName;
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == RESWORD && !strcmp(t.lx, "var"))
	{
		;
	}
	else
	{
		pi.tk = t;
		pi.er = syntaxError;
		return pi;
	}
	t = PeekNextToken();
	pi = type(&type_var);
	if (pi.er != none)
	{
		return pi;
	}
	// Find what the type is
	strncpy(className, t.lx, sizeof(t.lx));
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == ID)
	{
		if (pass == 2)
		{
			if (lookupSymbolInSubroutine(manager->currentSubroutine, t.lx) != NULL)
			{
				pi.er = redecIdentifier;
				pi.tk = t;
				return pi;
			}
			if (type_var == TYPE_IDENTIFIER)
			{
				// className = t.lx;
				subroutineName = NULL;
			}
			else
			{
				strncpy(className, manager->currentClass->name, sizeof(manager->currentClass->name));
				subroutineName = NULL;
			}

			insertSymbol(manager, t.lx, type_var, KIND_LOCAL, className, subroutineName);
		}
	}
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp == SYMBOL && !strcmp(t.lx, ","))
	{
		// if next token is a comma then there will be a command then identifier

		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};

		// Eat comma
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == ID)
		{
			if (pass == 2)
			{
				if (lookupSymbolInSubroutine(manager->currentSubroutine, t.lx) != NULL)
				{
					pi.er = redecIdentifier;
					pi.tk = t;
					return pi;
				}
				insertSymbol(manager, t.lx, type_var, KIND_LOCAL, className, subroutineName);
			}
		}
		else
		{
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

	if (t.tp == SYMBOL && !strcmp(t.lx, ";"))
	{
		;
	}
	else
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// letStatemnt → let identifier [ [ expression ] ] = expression ;
ParserInfo letStatement()
{
	int scopeLevel = 0;
	char className[128];
	Symbol *symbol;
	Scope *scope;
	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == RESWORD && !strcmp(t.lx, "let"))
	{
		;
	}
	else
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}
	// identifier
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == ID)
	{
		if (pass == 2)
		{
			// Identifier needs to be in symbol table either in class or subroutine
			if (manager->currentSubroutine != NULL)
			{
				if (symbol = lookupSymbolInSubroutine(manager->currentSubroutine, t.lx))
				{
					// It will be defined by a varDeclarStatement
					// printf("Symbol %s in subroutine %s\n", t.lx, manager->currentSubroutine->name);
					// printf("2 symbol %s has type %s\n", t.lx, symbol->className);
					scopeLevel = 2;
				}
				else if (symbol = lookupSymbolInClass(manager, manager->currentClass->name, t.lx))
				{
					// printf("Symbol %s in class %s\n", t.lx, manager->currentClass->name);
					// need to find type of symbol
					// printf("1 symbol %s has type %s\n", t.lx, symbol->className);
					scopeLevel = 1;
				}
				else if (scope = lookupClass(manager, t.lx))
				{
					// printf("Symbol %s is class %s\n", t.lx, manager->currentClass->name);
					scopeLevel = 0;
				}
				else
				{
					// printf("identifier %s not found\n", t.lx);
					pi.er = undecIdentifier;
					pi.tk = t;
					return pi;
				}
			}
		}
	}
	else
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}
	// check for [ expression ]
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp == SYMBOL && !strcmp(t.lx, "["))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // eat the [
		pi = expression();
		if (pi.er != none)
		{
			return pi;
		}
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // get the ]
		if (t.tp == SYMBOL && !strcmp(t.lx, "]"))
		{
			;
		}
		else
		{
			pi.er = closeBracketExpected;
			pi.tk = t;
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}
	// check for =
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == SYMBOL && !strcmp(t.lx, "="))
	{
		;
	}
	else
	{
		pi.er = equalExpected;
		pi.tk = t;
		return pi;
	}
	// check for expression
	pi = expression();
	if (pi.er != none)
	{
		return pi;
	}
	// check for semicolon
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == SYMBOL && !strcmp(t.lx, ";"))
	{
		;
	}
	else
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// ifStatement → if ( expression ) { {statement} } [else { {statement} }]
ParserInfo ifStatement()
{
	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == RESWORD && !strcmp(t.lx, "if"))
	{
		;
	}
	else
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}
	// check for (
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "("))
	{
		;
	}
	else
	{
		pi.er = openParenExpected;
		pi.tk = t;
		return pi;
	}
	// check for expression
	pi = expression();
	if (pi.er != none)
	{
		return pi;
	}
	// check for )
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, ")"))
	{
		;
	}
	else
	{
		pi.er = closeParenExpected;
		pi.tk = t;
		return pi;
	}
	// check for {
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "{"))
	{
		;
	}
	else
	{
		pi.er = openBraceExpected;
		pi.tk = t;
		return pi;
	}
	// check for 0 or more statements
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp != SYMBOL && strcmp(t.lx, "}"))
	{
		pi = statement();
		if (pi.er != none)
		{
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}
	// check for }
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "}"))
	{
		;
	}
	else
	{
		pi.er = closeBraceExpected;
		pi.tk = t;
		return pi;
	}
	// check for else
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == RESWORD && !strcmp(t.lx, "else"))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		// eat else
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == SYMBOL && !strcmp(t.lx, "{"))
		{
			;
		}
		else
		{
			pi.er = openBraceExpected;
			pi.tk = t;
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		while (t.tp != SYMBOL && strcmp(t.lx, "}"))
		{
			pi = statement();
			if (pi.er != none)
			{
				return pi;
			}
			t = PeekNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t};
		}
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == SYMBOL && !strcmp(t.lx, "}"))
		{
			;
		}
		else
		{
			pi.tk = t;
			pi.er = closeBraceExpected;
			return pi;
		}
	}
	pi.tk = t;
	return pi;
}

// whileStatement → while ( expression ) { {statement} }
ParserInfo whileStatement()
{
	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == RESWORD && !strcmp(t.lx, "while"))
	{
		;
	}
	else
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "("))
	{
		;
	}
	else
	{
		pi.er = openParenExpected;
		pi.tk = t;
		return pi;
	}
	// check for expression
	pi = expression();
	if (pi.er != none)
	{
		return pi;
	}
	// check for )
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, ")"))
	{
		;
	}
	else
	{
		pi.er = closeParenExpected;
		pi.tk = t;
		return pi;
	}
	// check for {
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "{"))
	{
		;
	}
	else
	{
		pi.er = openBraceExpected;
		pi.tk = t;
		return pi;
	}
	// check for 0 or more statements
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp != SYMBOL && strcmp(t.lx, "}"))
	{
		pi = statement();
		if (pi.er != none)
		{
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}
	// check for }
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "}"))
	{
		;
	}
	else
	{
		pi.er = closeBraceExpected;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// doStatement → do subroutineCall ;
ParserInfo doStatement()
{
	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == RESWORD && !strcmp(t.lx, "do"))
	{
		;
	}
	else
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}
	// check for subroutineCall
	pi = subroutineCall();
	if (pi.er != none)
	{
		return pi;
	}
	// check for ;
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, ";"))
	{
		;
	}
	else
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// subroutineCall → identifier [ . identifier ] ( expressionList )
ParserInfo subroutineCall()
{
	int scopeLevel = 0;
	char className[128];
	Symbol *symbol;
	Scope *scope;
	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == ID)
	{
		if (pass == 2)
		{
			// Identifier can be in 3 places
			// 1 A class name
			// In current class field or static var
			// In current subroutine
			if (manager->currentSubroutine != NULL)
			{
				if (symbol = lookupSymbolInSubroutine(manager->currentSubroutine, t.lx))
				{
					// It will be defined by a varDeclarStatement
					// printf("Symbol %s in subroutine %s\n", t.lx, manager->currentSubroutine->name);
					// printf("2 symbol %s has type %s\n", t.lx, symbol->className);
					scopeLevel = 2;
				}
				else if (symbol = lookupSymbolInClass(manager, manager->currentClass->name, t.lx))
				{
					// printf("Symbol %s in class %s\n", t.lx, manager->currentClass->name);
					// need to find type of symbol
					// printf("1 symbol %s has type %s\n", t.lx, symbol->className);
					scopeLevel = 1;
				}
				else if (scope = lookupClass(manager, t.lx))
				{
					// printf("Symbol %s is class %s\n", t.lx, manager->currentClass->name);
					scopeLevel = 0;
				}
				else
				{
					// printf("identifier %s not found\n", t.lx);
					// printf("symbol Class name: %s\n", symbol->className);
					pi.er = undecIdentifier;
					pi.tk = t;
					return pi;
				}
			}
		}
	}
	else
	{
		pi.er = idExpected;
		pi.tk = t;
		return pi;
	}

	// check for optional . identifier
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "."))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // eat the .
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // get next token
		if (t.tp != ID)
		{
			pi.er = idExpected;
			pi.tk = t;
			return pi;
		}
		if (pass == 2)
		{
			switch (scopeLevel)
			{
			case 0:
				if (lookupSymbolInClass(manager, scope->name, t.lx) == NULL)
				{
					// printf("symbol Class name: %s\n", symbol->className);
					pi.er = undecIdentifier;
					pi.tk = t;
					return pi;
				}
				break;
			case 1:
			case 2:
				// printf("symbol Class name: %s\n", symbol->className);
				if (lookupSymbolInClass(manager, symbol->className, t.lx) == NULL)
				{
					pi.er = undecIdentifier;
					pi.tk = t;
					return pi;
				}
				break;
			default:
				printf("Error\n");
				break;
			}
		}
	}
	// check for (
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "("))
	{
		;
	}
	else
	{
		pi.er = openParenExpected;
		pi.tk = t;
		return pi;
	}
	// check for expressionList
	pi = expressionList();
	if (pi.er != none)
	{
		return pi;
	}
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, ")"))
	{
		;
	}
	else
	{
		pi.er = closeParenExpected;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// expressionList → expression { , expression } | ε
ParserInfo expressionList()
{
	ParserInfo pi = {0};
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, ")"))
	{
		// expressionList is empty
		return pi;
	}
	// check for expression
	pi = expression();
	if (pi.er != none)
	{
		return pi;
	}
	// check for 0 or more , expression
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp == SYMBOL && !strcmp(t.lx, ","))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // eat the ,
		pi = expression();
		if (pi.er != none)
		{
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
ParserInfo returnStatement()
{
	ParserInfo pi = {0};
	Token t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == RESWORD && !strcmp(t.lx, "return"))
	{
		;
	}
	else
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}
	// check zero or more expressions
	t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp != SYMBOL && strcmp(t.lx, ";"))
	{
		pi = expression();
		if (pi.er != none)
		{
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}
	// check for ;
	t = GetNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, ";"))
	{
		;
	}
	else
	{
		pi.er = semicolonExpected;
		pi.tk = t;
		return pi;
	}
	pi.tk = t;
	return pi;
}

// Expression grammar

// expression → relationalExpression { ( & | | ) relationalExpression }
ParserInfo expression()
{
	ParserInfo pi = {0};
	pi = relationalExpression();
	if (pi.er != none)
	{
		return pi;
	}
	// check for zero or more "and-ed" or "or-ed" relation Expressions
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp == SYMBOL && (!strcmp(t.lx, "&") || !strcmp(t.lx, "|")))
	{
		if (t.tp == SYMBOL && !strcmp(t.lx, "&"))
		{
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t};
			// eat the &
			pi = relationalExpression();
			if (pi.er != none)
			{
				return pi;
			}
		}
		else if (t.tp == SYMBOL && !strcmp(t.lx, "|"))
		{
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t};
			// eat the |
			pi = relationalExpression();
			if (pi.er != none)
			{
				return pi;
			}
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}
	pi.tk = t;
	return pi;
}

// relationalExpression → ArithmeticExpression { ( = | > | < ) ArithmeticExpression }
ParserInfo relationalExpression()
{

	ParserInfo pi = {0};
	pi = ArithmeticExpression();
	if (pi.er != none)
	{
		return pi;
	}
	// zero or more symbol then expression
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp == SYMBOL && (!strcmp(t.lx, "=") || !strcmp(t.lx, "<") || !strcmp(t.lx, ">")))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == SYMBOL && !strcmp(t.lx, "="))
		{
			;
		}
		else if (t.tp == SYMBOL && !strcmp(t.lx, "<"))
		{
			;
		}
		else if (t.tp == SYMBOL && !strcmp(t.lx, ">"))
		{
			;
		}
		else
		{
			pi.tk = t;
			pi.er = syntaxError;
			return pi;
		}
		// Arithmetic Expression
		pi = ArithmeticExpression();
		if (pi.er != none)
		{
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}
	pi.tk = t;
	return pi;
}

// ArithmeticExpression → term { ( + | - ) term }
ParserInfo ArithmeticExpression()
{

	ParserInfo pi = {0};
	pi = term();
	if (pi.er != none)
	{
		return pi;
	}
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp == SYMBOL && (!strcmp(t.lx, "+") || !strcmp(t.lx, "-")))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == SYMBOL && !strcmp(t.lx, "+"))
		{
			;
		}
		else if (t.tp == SYMBOL && !strcmp(t.lx, "-"))
		{
			;
		}
		pi = term();
		if (pi.er != none)
		{
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}
	pi.tk = t;
	return pi;
}

// term → factor { ( * | / ) factor }
ParserInfo term()
{

	ParserInfo pi = {0};
	pi = factor();
	if (pi.er != none)
	{
		return pi;
	}
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	while (t.tp == SYMBOL && (!strcmp(t.lx, "*") || !strcmp(t.lx, "/")))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == SYMBOL && !strcmp(t.lx, "*"))
		{
			;
		}
		else if (t.tp == SYMBOL && !strcmp(t.lx, "/"))
		{
			;
		}
		pi = factor();
		if (pi.er != none)
		{
			return pi;
		}
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
	}
	pi.tk = t;
	return pi;
}

// factor → ( - | ~ | ε ) operand
ParserInfo factor()
{

	ParserInfo pi = {0};
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};
	if (t.tp == SYMBOL && !strcmp(t.lx, "-"))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		// eat the -
	}
	else if (t.tp == SYMBOL && !strcmp(t.lx, "~"))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		// eat the ~
	}
	pi = operand();
	if (pi.er != none)
	{
		return pi;
	}
	pi.tk = t;
	return pi;
}

// operand → integerConstant | identifier [.identifier ] [ [ expression ] ] | (expressionList) | stringLiteral | true | false | null | this
ParserInfo operand()
{
	ParserInfo pi = {0};
	int scopeLevel = 0;
	char className[128];
	Symbol *symbol;
	Scope *scope;
	Token t = PeekNextToken();
	if (t.ec)
		return (ParserInfo){lexerErr, t};

	if (t.tp == INT)
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // consume integer
	}
	else if (t.tp == ID)
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // consume identifier
		if (pass == 2)
		{
			// Check if is a class or var identifier

			if (manager->currentSubroutine != NULL)
			{
				if (symbol = lookupSymbolInSubroutine(manager->currentSubroutine, t.lx))
				{
					// It will be defined by a varDeclarStatement
					// printf("Symbol %s in subroutine %s\n", t.lx, manager->currentSubroutine->name);
					// printf("2 symbol %s has type %s\n", t.lx, symbol->className);
					scopeLevel = 2;
				}
				else if (symbol = lookupSymbolInClass(manager, manager->currentClass->name, t.lx))
				{
					// printf("Symbol %s in class %s\n", t.lx, manager->currentClass->name);
					// need to find type of symbol
					// printf("1 symbol %s has type %s\n", t.lx, symbol->className);
					scopeLevel = 1;
				}
				else if (scope = lookupClass(manager, t.lx))
				{
					// printf("Symbol %s is class %s\n", t.lx, manager->currentClass->name);
					scopeLevel = 0;
				}
				else
				{
					printf("identifier %s not found\n", t.lx);
					printf("symbol Class name: %s\n", symbol->className);
					pi.er = undecIdentifier;
					pi.tk = t;
					return pi;
				}
			}
		}

		// Check for optional .identifier
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == SYMBOL && !strcmp(t.lx, "."))
		{
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t}; // consume .
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t}; // get next token
			if (t.tp != ID)
			{
				pi.er = idExpected;
				pi.tk = t;
				return pi;
			}
			if (pass == 2)
			{
				switch (scopeLevel)
				{
				case 0:
					if (lookupSymbolInClass(manager, scope->name, t.lx) == NULL)
					{
						printf("identifier %s not found\n", t.lx);
						printf("symbol Class name: %s\n", symbol->className);
						pi.er = undecIdentifier;
						pi.tk = t;
						return pi;
					}
					break;
				case 1:
				case 2:
					if (lookupSymbolInClass(manager, symbol->className, t.lx) == NULL)
					{
						printf("identifier %s not found\n", t.lx);
						printf("symbol Class name: %s\n", symbol->className);
						pi.er = undecIdentifier;
						pi.tk = t;
						return pi;
					}
					break;
				default:
					printf("Error\n");
					break;
				}
			}
		}

		// Check for optional [ expression ]
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == SYMBOL && !strcmp(t.lx, "["))
		{
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t}; // consume [
			pi = expression();
			if (pi.er != none)
			{
				return pi;
			}
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t};
			if (!(t.tp == SYMBOL && !strcmp(t.lx, "]")))
			{
				pi.er = closeBracketExpected;
				pi.tk = t;
				return pi;
			}
		}

		// Check for optional expressionList (subroutine call)
		t = PeekNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (t.tp == SYMBOL && !strcmp(t.lx, "("))
		{
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t}; // consume (
			pi = expressionList();
			if (pi.er != none)
			{
				return pi;
			}
			t = GetNextToken();
			if (t.ec)
				return (ParserInfo){lexerErr, t};
			if (!(t.tp == SYMBOL && !strcmp(t.lx, ")")))
			{
				pi.er = closeParenExpected;
				pi.tk = t;
				return pi;
			}
		}
	}
	else if (t.tp == SYMBOL && !strcmp(t.lx, "("))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // consume (
		pi = expressionList();
		if (pi.er != none)
		{
			return pi;
		}
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t};
		if (!(t.tp == SYMBOL && !strcmp(t.lx, ")")))
		{
			pi.er = closeParenExpected;
			pi.tk = t;
			return pi;
		}
	}
	else if (t.tp == STRING)
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // consume string
	}
	else if (t.tp == RESWORD && (!strcmp(t.lx, "true") ||
								 !strcmp(t.lx, "false") ||
								 !strcmp(t.lx, "null") ||
								 !strcmp(t.lx, "this")))
	{
		t = GetNextToken();
		if (t.ec)
			return (ParserInfo){lexerErr, t}; // consume keyword
	}
	else
	{
		pi.er = syntaxError;
		pi.tk = t;
		return pi;
	}

	pi.tk = t;
	return pi;
}

int InitParser(char *file_name)
{
	return InitLexer(file_name);
}

ParserInfo Parse()
{
	ParserInfo pi = {0};
	pi = classDeclar();

	return pi;
}

int StopParser()
{
	return StopLexer();
}

// #ifndef TEST_PARSER
// int main()
// {

// 	return 1;
// }
// #endif
