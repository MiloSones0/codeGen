/************************************************************************
University of Leeds
School of Computing
COMP2932- Compiler Design and Construction
Lexer Module

I confirm that the following code has been developed and written by me and it is entirely the result of my own work.
I also confirm that I have not copied any parts of this program from another person or any other source or facilitated someone to copy this program from me.
I confirm that I will not publish the program online or share it with anyone without permission of the module leader.

Student Name: Milo Sones
Student ID: 201730449
Email: sc23m3s@leeds.ac.uk
Date Work Commenced: 1/3/25
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

// YOU CAN ADD YOUR OWN FUNCTIONS, DECLARATIONS AND VARIABLES HERE

#define NumberKeywords 21

#define NumberSymbols 19

FILE *f;
int TokenReady;
Token t;
int LineCount;

const char *keywords[NumberKeywords] = {"class", "constructor", "method", "function", "int", "boolean", "char", "void", "let", "static", "field", "var", "do", "if", "else", "while", "return", "true", "false", "null", "this"};
const char symbols[NumberSymbols] = {'(', ')', '[', ']', '{', '}', ',', ';', '=', '.', '+', '-', '*', '/', '&', '|', '~', '<', '>'};

int isValidIdChar(char c)
{
	return isalnum(c) || c == '_';
}

int isSymbol(char c)
{
	for (int i = 0; i < NumberSymbols; i++)
	{
		if (c == symbols[i])
		{
			return 1;
		}
	}
	return 0;
}

int IsKeyWord(char *str)
{
	for (int i = 0; i < NumberKeywords; i++)
		if (!strcmp(keywords[i], str))
			return 1;
	return 0;
}

// eat ws and c and return first non white space char
int EatWC()
{
	// consume white space and comments
	int c;
	c = getc(f);

	while (c != EOF && isspace(c))
	{
		if ((char)c == '\n')
			LineCount++;
		c = getc(f);
	}
	// we have hit non-white-space

	// remove comments

	if (c == '/')
	{
		char c_old = c;
		c = getc(f);

		if (c == '*')
		{
			// start of style 2 and 3 commenet
			// ends at */
			while (c != EOF)
			{
				c_old = c;
				c = getc(f);
				if ((char)c == '\n')
				{
					LineCount++;
				}
				if (c_old == '*' && c == '/')
				{
					c = getc(f);
					if (isspace(c) || c == '/')
					{
						ungetc(c, f);
						return EatWC();
					}
				}
				if (c == EOF)
				{
					t.ec = EofInCom;
					t.tp = ERR;
					t.ln = LineCount;
					strcpy(t.lx, "Error: unexpected eof in comment");
					return c;
				}
			}
		}
		if (c != '/')
		{
			ungetc(c, f);
			return c_old;
		}

		if (c == '/')
		{
			// start of style 1 comment
			// ends at newline

			while (c != EOF && c != '\n')
			{
				c = getc(f);
				if ((char)c == '\n')
				{
					LineCount++;
				}
			}
			if (c == EOF)
			{
				return c;
			}

			c = getc(f);
			if (isspace(c) || c == '/')
			{
				ungetc(c, f);
				return EatWC();
			}
		}
	}

	return c;
}

Token BuildToken()
{
	// default case is symbol

	t.tp = SYMBOL;
	// Set token type to something that isnt an err to make sure eatws can properly detect errors

	int c = EatWC();
	if (t.tp == ERR)
	{
		return t;
	}

	if (c == EOF)
	{
		t.tp = EOFile;
		t.ln = LineCount;
		return t;
	}
	t.ln = LineCount;
	t.lx[0] = c;
	t.lx[1] = '\0';
	char temp[128];
	int i = 0;

	// handle STRING constant

	if (c == '"')
	{
		while (c != EOF && c != '\n')
		{
			c = getc(f);
			if (c == '"')
			{
				temp[i] = '\0';
				strcpy(t.lx, temp);
				t.tp = STRING;
				t.ln = LineCount;
				return t;
			}
			temp[i++] = c;
		}
		if (c == EOF)
		{
			t.ec = EofInStr;
			t.tp = ERR;
			t.ln = LineCount;
			strcpy(t.lx, "Error: unexpected eof in string constant");
			return t;
		}
		if (c == '\n')
		{
			t.ec = NewLnInStr;
			t.tp = ERR;
			t.ln = LineCount;
			strcpy(t.lx, "Error: new line in string constant");
			return t;
		}
	}

	// handle INT constant
	if (isdigit(c))
	{
		while (c != EOF && isdigit(c))
		{
			temp[i++] = c;
			c = getc(f);
		}
		temp[i] = '\0';
		ungetc(c, f);

		strcpy(t.lx, temp);
		t.tp = INT;
		t.ln = LineCount;
		return t;
	}

	// Handle ID and RESWORD
	if (isValidIdChar(c) && !isdigit(c))
	{
		while (c != EOF && isValidIdChar(c))
		{
			temp[i++] = c;
			c = getc(f);
		}
		temp[i] = '\0';
		ungetc(c, f);

		strcpy(t.lx, temp);

		if (IsKeyWord(temp))
			t.tp = RESWORD;
		else
			t.tp = ID;
		t.ln = LineCount;
		return t;
	}

	if (!isSymbol(c))
	{
		t.ec = IllSym;
		t.tp = ERR;
		t.ln = LineCount;
		strcpy(t.lx, "Error: illegal symbol in source file");
		return t;
	}

	return t;
}

// IMPLEMENT THE FOLLOWING functions
//***********************************

// Initialise the lexer to read from source file
// file_name is the name of the source file
// This requires opening the file and making any necessary initialisations of the lexer
// If an error occurs, the function should return 0
// if everything goes well the function should return 1
int InitLexer(char *file_name)
{
	f = fopen(file_name, "r");
	if (f == 0)
		return 0;
	TokenReady = 0;
	LineCount = 1;
	strcpy(t.fl, file_name);
	return 1;
}

// Get the next token from the source file
Token GetNextToken()
{
	if (TokenReady)
	{
		TokenReady = 0;
		return t;
	}
	t = BuildToken();
	TokenReady = 0;
	return t;
}

// peek (look) at the next token in the source file without removing it from the stream
Token PeekNextToken()
{
	if (TokenReady)
		return t;
	t = BuildToken();
	TokenReady = 1;
	return t;
}

// clean out at end, e.g. close files, free memory, ... etc
int StopLexer()
{
	fclose(f);
	return 0;
}

// do not remove the next line
// #ifndef TEST
// int main()
// {
// 	// implement your main function here
// 	// NOTE: the autograder will not use your main function
// 	InitLexer("Main.jack");
// 	while (1)
// 	{
// 		t = GetNextToken();
// 	}
// 	return 0;
// }
// // do not remove the next line
// #endif
