/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "depends.h"
#include "StackedSymbolTable.h"
#include "Instruction.h"
#include "Parser.h"

typedef HRESULT (*SCRIPTFUNCTION)(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount);

typedef struct Interpreter {
   StackedSymbolTable theSymbolTable;
   List* ptheFunctionList;               //external functionlist, to save some memory
   List theInstructionList;
   List paramList;
   Stack theDataStack;
   Stack theLabelStack;
   Parser theParser;

   int currentCallIndex;
   int mainEntryIndex;
   int returnEntryIndex;
   int bHasImmediateCode;

   BOOL bCallCompleted;
   BOOL bMainCompleted;
}Interpreter;

void Interpreter_Init(Interpreter* pinterpreter, LPCSTR name, List* pflist);
void Interpreter_Clear(Interpreter* pinterpreter);
HRESULT Interpreter_ParseText(Interpreter* pinterpreter, LPCSTR scriptText, 
                           ULONG startingLineNumber, LPCSTR path);
HRESULT Interpreter_PutValue(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant* pValue, int refFlag );
HRESULT Interpreter_GetValue(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant* pValue);
HRESULT Interpreter_GetValueByRef(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant** pValue);
HRESULT Interpreter_EvaluateImmediate(Interpreter* pinterpreter);
HRESULT Interpreter_EvaluateCall(Interpreter* pinterpreter);
HRESULT Interpreter_CompileInstructions(Interpreter* pinterpreter);
HRESULT Interpreter_Call(Interpreter* pinterpreter);
HRESULT Interpreter_EvalInstruction(Interpreter* pinterpreter);
void Interpreter_Reset(Interpreter* pinterpreter);


#endif
