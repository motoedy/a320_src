/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#include "Interpreter.h"
#include "tracemalloc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void Interpreter_Init(Interpreter* pinterpreter, LPCSTR name, List* pflist)
{
    memset(pinterpreter, 0, sizeof(Interpreter));
    StackedSymbolTable_Init(&(pinterpreter->theSymbolTable), name);
    Parser_Init(&(pinterpreter->theParser));
    pinterpreter->ptheFunctionList = pflist;
}

void Interpreter_Clear(Interpreter* pinterpreter)
{
    int i, size;
    Instruction* pinstruction = NULL;
    ScriptVariant* pvariant = NULL;
    
    StackedSymbolTable_Clear(&(pinterpreter->theSymbolTable));
    Parser_Clear(&(pinterpreter->theParser));
    if(pinterpreter->theInstructionList.solidlist)
    {
        size = pinterpreter->theInstructionList.size;
        for(i=0;i<size;i++)
        {
            Instruction_Clear(pinterpreter->theInstructionList.solidlist[i]);
            tracefree((void*)pinterpreter->theInstructionList.solidlist[i]);
            pinterpreter->theInstructionList.solidlist[i] = NULL;
        }
    }
    else
    {
        FOREACH(
            pinterpreter->theInstructionList,
            pinstruction = (Instruction*)List_Retrieve(&(pinterpreter->theInstructionList));
            Instruction_Clear(pinstruction);
            tracefree((void*)pinstruction);
            pinstruction = NULL;
        );
    }
    while(!Stack_IsEmpty(&(pinterpreter->theDataStack))){
        pvariant = (ScriptVariant*)Stack_Top(&(pinterpreter->theDataStack));
        ScriptVariant_Clear(pvariant);
        tracefree((void*)pvariant);
        Stack_Pop(&(pinterpreter->theDataStack));
    }
    List_Clear(&(pinterpreter->theLabelStack));
    List_Clear(&(pinterpreter->theInstructionList));
    List_Clear(&(pinterpreter->paramList));
    memset(pinterpreter, 0, sizeof(Interpreter));
}


/******************************************************************************
*  ParseText -- This method parses the text in scriptText into a string of
*               byte-codes for the interpreter to execute.
*  Parameters: scriptText -- an LPCSTR containing the script to be parsed.
*              startingLineNumber -- The line number the script starts on
*                                    (For use in HTML-based scripts)
*              dwSourceContext -- DWORD which contains a host provided context
*                                 for the script being parsed.
*  Returns: E_FAIL if parser errors found else S_OK
******************************************************************************/
HRESULT Interpreter_ParseText(Interpreter* pinterpreter, LPCSTR scriptText, 
                           ULONG startingLineNumber, LPCSTR path)
{

    //Parse the script
    Parser_ParseText(&(pinterpreter->theParser), &(pinterpreter->theInstructionList), 
                    scriptText, startingLineNumber, path );
                    
    if(pinterpreter->theParser.errorFound) return E_FAIL;
    else return S_OK;
}

/******************************************************************************
*  PutValue -- This method copies the VARIANT in pValue into the symbol
*  designated by variable.
*  Parameters: variable -- a LPCSTR which denotes which symbol to copy this
*                          value into.
*              pValue -- a pointer to a ScriptVariant which contains the value
*                        to be copied into the symbol.
*  Returns: S_OK
*           E_INVALIDARG
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_PutValue(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant* pValue , int refFlag)
{
    HRESULT hr = E_FAIL;
    Instruction* pref = NULL;
    Symbol* pSymbol = NULL;
    //Check arguments
    if ((pValue == NULL) || (variable == NULL )){
        hr = E_FAIL;
    }
    else{
        //Get the CSymbol that contains the VARIANT we need
        pref = (Instruction*)List_Retrieve(&(pinterpreter->theInstructionList));
        if(refFlag && pref->theRef) {
            *(pref->theRef) = *pValue;
            hr = S_OK;
        }else if (StackedSymbolTable_FindSymbol(&(pinterpreter->theSymbolTable), variable, &pSymbol )){   
            //Copy the valye from the argument to the CSymbol
            ScriptVariant_Copy(&(pSymbol->var), pValue);
            if(refFlag && pSymbol->theRef) 
                pref->theRef = pSymbol->theRef->theVal;
            hr = S_OK;
       }
    }
   
    return hr;
}

/******************************************************************************
*  GetValue -- This method copies the VARIANT in the symbol designated by 
*              variable into the ScriptVariant.
*  Parameters: variable -- a LPCSTR which denotes which symbol to copy this
*                          value from.
*              pValue -- a pointer to a ScriptVariant into which to copy the
*                        value.
*  Returns: S_OK
*           E_INVALIDARG
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_GetValue(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant* pValue)
{
    HRESULT hr = E_FAIL;
    
    //Get the CSymbol that contains the VARIANT we need
    Symbol* pSymbol = NULL;
    
    if (StackedSymbolTable_FindSymbol(&(pinterpreter->theSymbolTable), variable, &pSymbol )){   
        //Copy the value from the symbol to the result VARIANT
        ScriptVariant_Copy(pValue, &(pSymbol->var));
        hr = S_OK;
    }
   
    return hr;
}

HRESULT Interpreter_GetValueByRef(Interpreter* pinterpreter, LPCSTR variable, ScriptVariant** ppValue )
{
    HRESULT hr = E_FAIL;
    
    //Get the CSymbol that contains the VARIANT we need
    Symbol* pSymbol = NULL;
    
    if (StackedSymbolTable_FindSymbol(&(pinterpreter->theSymbolTable), variable, &pSymbol )){   
        //Copy the value from the symbol to the result VARIANT
        *ppValue = pSymbol->theRef->theVal;
        hr = S_OK;
    }
   
    return hr;
}

/******************************************************************************
*  Call -- This method calls the method designated by variable, assuming it
*          exists in the script somewhere.  If there is a return value, it is
*          placed into the pRetValue ScriptVariant.
*  Parameters: method -- a LPCSTR which denotes the method to call
*              pRetValue -- a pointer to a ScriptVariant which accepts the 
*                           return value, if any.
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_Call(Interpreter* pinterpreter)
{
    HRESULT hr = E_FAIL;
    int temp = pinterpreter->currentCallIndex;
    int currentCallIndex = pinterpreter->theInstructionList.index;
    Instruction* currentCall;
    ScriptVariant *pretvar;

    if(currentCallIndex<0){
        pinterpreter->bCallCompleted = FALSE;
        return E_FAIL;
    }
    pinterpreter->currentCallIndex = currentCallIndex;
    currentCall = (Instruction*)(pinterpreter->theInstructionList.solidlist[currentCallIndex]);
    //Search for the specified entry point.
    if (currentCall->theJumpTargetIndex>=0){
        pinterpreter->theInstructionList.index = currentCall->theJumpTargetIndex;
        hr = Interpreter_EvaluateCall(pinterpreter);
    }
    else if( currentCall->functionRef)
    {
        pretvar = currentCall->theVal;
        hr = currentCall->functionRef((ScriptVariant**)currentCall->theRefList->solidlist, &(pretvar), (int)currentCall->theRef->lVal);
        if(FAILED(hr))
        {
            List_Includes(pinterpreter->ptheFunctionList, currentCall->functionRef);
            printf("Script function '%s' returned an exception, check the manual for details.\n", List_GetName(pinterpreter->ptheFunctionList));
        }
    }
    else
    {
        pinterpreter->bCallCompleted = FALSE;
        return E_FAIL;
    }
    //jump back
    if (SUCCEEDED(hr)){
        pinterpreter->theInstructionList.index = currentCallIndex;
    }
    pinterpreter->currentCallIndex = temp;

    //Reset the m_bCallCompleted flag back to false
    pinterpreter->bCallCompleted = FALSE;
   
    return hr;
}


/******************************************************************************
*  EvaluateImmediate -- This method scans the instruction list and evaluates
*  any immediate code that may be present.  In the instruction list, immediate
*  code is preceeded by an IMMEDIATE instruction, and followed by a DEFERRED
*  instruction.
*  Parameters: none
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_EvaluateImmediate(Interpreter* pinterpreter)
{
    BOOL bImmediate = FALSE;
    HRESULT hr = S_OK;
    Instruction* pInstruction = NULL;
    int size;

    if(pinterpreter->bHasImmediateCode)
    {
        //Run through all the instructions in the list, only executing those wrapped
        //by IMMEDIATE and DEFERRED.
        size = pinterpreter->theInstructionList.size;
        for(pinterpreter->theInstructionList.index = 0; pinterpreter->theInstructionList.index<size; pinterpreter->theInstructionList.index++)
        {
            pInstruction = (Instruction*)(pinterpreter->theInstructionList.solidlist[pinterpreter->theInstructionList.index]);
            //Check the current mode
            if (pInstruction->OpCode == IMMEDIATE)
                bImmediate = TRUE;
            else if (pInstruction->OpCode == DEFERRED)
                bImmediate = FALSE;
              
            //If the current mode is Immediate, then evaluate the instruction
            if (bImmediate){
                hr = Interpreter_EvalInstruction(pinterpreter);
                pinterpreter->theInstructionList.index--; //step back, because later we'll do the ++ operation
            }
            //If we failed, then we need to break out of this loop
            if (FAILED(hr))
                break;
        }
    }

    //The "main" function isn't technically immediate code, but it is the well
    //known entry point into C code, so we call it as part of immediate execution.
    //Don't call "main" if it has already been called once.
    if (!pinterpreter->bMainCompleted){
        if(pinterpreter->mainEntryIndex>=0) {
            pinterpreter->theInstructionList.index = pinterpreter->mainEntryIndex;
            hr = Interpreter_EvaluateCall(pinterpreter);
        } else hr = E_FAIL;
        if (SUCCEEDED(hr))
            //Set the m_bMainCompleted flag
            pinterpreter->bMainCompleted = TRUE;
        //else printf("error: %s\n", pinterpreter->theSymbolTable.name);
        pinterpreter->bCallCompleted = FALSE;
    }                             

    return hr;
}

/******************************************************************************
*  EvaluateCall -- This method evaluates the byte-code of a single call into
*  this interpreter.
*  Parameters: none
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_EvaluateCall(Interpreter* pinterpreter)
{
    HRESULT hr = S_OK;
    //Evaluate instructions until an error occurs or until the m_bCallCompleted
    //flag is set to true.
    while(( SUCCEEDED(hr) ) && ( !pinterpreter->bCallCompleted )){
        hr = Interpreter_EvalInstruction(pinterpreter);
    }
    return hr;
}


/******************************************************************************
*  UNARYOP -- The unary ops( +, -, !, etc. ) all work the same, so this macro
*  helps clean up the code.  It pops one value off the stack, applies the 
*  specified operator to it, and pushes the result.
******************************************************************************/
//only 'compile', dont realy use the operator.
#define COMPILEUNARYOP \
    if (pinterpreter->theDataStack.size >= 1){ \
        pSVar1 = Stack_Top(&(pinterpreter->theDataStack)); \
        pInstruction->theRef = pSVar1;\
    } else hr = E_FAIL;
   
   //else 
   //HandleRuntimeError( pInstruction, STACK_FAILURE, this );
   
#define UNARYOP(x) \
    x(pInstruction->theRef);


/******************************************************************************
*  BINARYOP -- The binary ops( +, -, <, ==, etc. ) all work the same, so this 
*  macro helps clean up the code.  It pops two values off the stack, applies
*  the specified operator to them, and pushes the result.
******************************************************************************/
//only 'compile', dont realy use the operator
#define COMPILEBINARYOP \
    if (pinterpreter->theDataStack.size >= 2){ \
        pSVar2 = Stack_Top(&(pinterpreter->theDataStack)); \
        Stack_Pop(&(pinterpreter->theDataStack)); \
        pSVar1 = Stack_Top(&(pinterpreter->theDataStack)); \
        Stack_Pop(&(pinterpreter->theDataStack)); \
        pRetVal = (ScriptVariant*)tracemalloc("COMPILEBINARYOP", sizeof(ScriptVariant));\
        ScriptVariant_Init(pRetVal);\
        Stack_Push(&(pinterpreter->theDataStack),(void*)pRetVal); \
        pInstruction->theVal = pRetVal;\
        pInstruction->theRef = pSVar1;\
        pInstruction->theRef2 = pSVar2;\
    } else hr = E_FAIL;
   
   //else 
   //HandleRuntimeError( pInstruction, STACK_FAILURE, this );
 
#define BINARYOP(x)  \
    ScriptVariant_Copy(pInstruction->theVal, x(pInstruction->theRef, pInstruction->theRef2));

HRESULT Interpreter_CompileInstructions(Interpreter* pinterpreter)
{
    int i, j, size;
    Instruction* pInstruction = NULL;
    Token* pToken ;
    Symbol* pSymbol = NULL;
    LPCSTR pLabel = NULL;
    ScriptVariant* pSVar1 = NULL;
    ScriptVariant* pSVar2 = NULL;
    ScriptVariant* pRetVal = NULL;
	HRESULT hr = S_OK;
    
    if(List_FindByName(&(pinterpreter->theInstructionList), "main"))
        pinterpreter->mainEntryIndex = List_GetIndex(&(pinterpreter->theInstructionList));
    else pinterpreter->mainEntryIndex = -1;
    List_Reset(&(pinterpreter->theInstructionList));
    size = List_GetSize(&(pinterpreter->theInstructionList));
    for(i=0; i<size; i++)
    {
        pInstruction = (Instruction*)List_Retrieve(&(pinterpreter->theInstructionList));
        //The OpCode will tell us what operation to perform.  
        switch( pInstruction->OpCode ){
            //Push a constant string
        case CONSTSTR:
            //Push a constant double
        case CONSTDBL:
            //Push a constant integer
        case CONSTINT:
            //convert to constant first
            Instruction_ConvertConstant(pInstruction);
            Instruction_NewData2(pInstruction);
            ScriptVariant_Copy(pInstruction->theVal2, pInstruction->theVal);
			Stack_Push(&(pinterpreter->theDataStack), (void*)pInstruction->theVal2);
            break;
            
           //Load a value into the data stack
        case LOAD:
            hr = Interpreter_GetValueByRef(pinterpreter, pInstruction->theToken->theSource, &(pInstruction->theRef));
            // cache value
            Instruction_NewData(pInstruction);
            //push the value, not ref
			Stack_Push(&(pinterpreter->theDataStack), (void*)(pInstruction->theVal));
            break;
            
         //Save a value from the data stack
        case SAVE:
            pSVar2 = (ScriptVariant*)Stack_Top(&(pinterpreter->theDataStack));
            Stack_Pop(&(pinterpreter->theDataStack));
            hr = Interpreter_GetValueByRef(pinterpreter, pInstruction->theToken->theSource, &(pInstruction->theRef));
            pInstruction->theRef2 = pSVar2;
            break;
         
            //use the UNARYOP macro to do an AutoIncrement
        case INC:
            //use the UNARYOP macro to do an AutoDecrement
        case DEC:
           //use the UNARYOP macro to do a unary plus
        case POS:
         //use the UNARYOP macro to do a unary minus
        case NEG:
         //use the UNARYOP macro to do a logical not
        case NOT:
            COMPILEUNARYOP;
            break; 
            
         //Use the BINARYOP macro to do a multipy
        case MUL:
         //Use the BINARYOP macro to do a divide
        case DIV: 
         //Use the BINARYOP macro to do a mod
        case MOD: 
         //Use the BINARYOP macro to do an add
        case ADD:
         //Use the BINARYOP macro to do a subtract
        case SUB: 
         //Use the BINARYOP macro to do a greater than- equal
        case GE:
         //Use the BINARYOP macro to do a less than- equal
        case LE:
         //Use the BINARYOP macro to do a less than
        case LT:
         //Use the BINARYOP macro to do a greater than
        case GT:
         //Use the BINARYOP macro to do an equality
        case EQ: 
         //Use the BINARYOP macro to do a not-equal
        case NE: 
         //Use the BINARYOP macro to do a logical OR
        case OR:
         //Use the BINARYOP macro to do a logical AND
        case AND:
           COMPILEBINARYOP;
           break;
         
         //Create a new CSymbol and add it to the symbol table
        case DATA:
            
         //Create a new CSymbol and add it to the symbol table
        case PARAM: 
            Instruction_NewData(pInstruction); //cache the the new variant
            pToken = pInstruction->theToken;
            pSymbol = (Symbol*)tracemalloc("Interpreter_CompileInstructions #1", sizeof(Symbol));
            Symbol_Init(pSymbol, pToken->theSource, 0, NULL, pInstruction);
            StackedSymbolTable_AddSymbol(&(pinterpreter->theSymbolTable), pSymbol );
            break;

         //Call the specified method, and pass in a ScriptVariant* to receive the 
         //return value.  If it's not NULL, then push it onto the data stack.
        case CALL:
            //We need to be able to jump back to this instruction when the call is
            //over, so copy this instruction's label onto the label stack
            pLabel = List_GetName(&(pinterpreter->theInstructionList));
            pToken = pInstruction->theToken;
            
            pInstruction->theJumpTargetIndex = -1;
            pInstruction->functionRef = NULL;
            //cache the jump target
            if(List_FindByName(&(pinterpreter->theInstructionList), pToken->theSource)){
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                List_FindByName(&(pinterpreter->theInstructionList), pLabel); //hop back
            } else if(List_FindByName( pinterpreter->ptheFunctionList, pToken->theSource)){
                pInstruction->functionRef = (SCRIPTFUNCTION)List_Retrieve(pinterpreter->ptheFunctionList);
            }
            else // can't find the jump target
            {
                hr = E_FAIL;
            }
            //cache the paramCount
            pSVar1 = (ScriptVariant*)Stack_Top(&(pinterpreter->theDataStack));
            Stack_Pop(&(pinterpreter->theDataStack));
            pInstruction->theRef = pSVar1;
            //printf("#%u\n", pInstruction->theRef);
            // alloc the param ref list;
            pInstruction->theRefList = (List*)tracemalloc("Interpreter_CompileInstructions #2", sizeof(List));
            List_Init(pInstruction->theRefList);
            //cache parameter list
            for(j=0; j<pSVar1->lVal; j++){
                pSVar2 = (ScriptVariant*)Stack_Top(&(pinterpreter->theDataStack));
                Stack_Pop(&(pinterpreter->theDataStack));
                List_InsertAfter(pInstruction->theRefList, (void*)pSVar2, NULL);
            }
            List_Solidify(pInstruction->theRefList);
            //cache the return value
            Instruction_NewData(pInstruction);
            List_GotoNext(&(pinterpreter->theInstructionList));
            if(((Instruction*)List_Retrieve(&(pinterpreter->theInstructionList)))->OpCode != CLEAN){
                Stack_Push(&(pinterpreter->theDataStack), pInstruction->theVal);
            }
            List_GotoPrevious(&(pinterpreter->theInstructionList));
            break;
         
         //Jump to the specified label
        case JUMP:
            pLabel = pInstruction->Label;
            //cache the jump target
            if(List_FindByName(&(pinterpreter->theInstructionList), pLabel)){
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                List_Includes(&(pinterpreter->theInstructionList), pInstruction); // hop back
            } else hr = E_FAIL;
            break;
            
         //Jump to the end of function, infact it is return
        case JUMPR:
            pSVar1 = (ScriptVariant*)Stack_Top(&(pinterpreter->theDataStack));
            Stack_Pop(&(pinterpreter->theDataStack));
            // cache the return value
            pInstruction->theRef = pSVar1;
            pLabel = pInstruction->Label;
            //cache the jump target
            if(List_FindByName(&(pinterpreter->theInstructionList), pLabel)){
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                List_Includes(&(pinterpreter->theInstructionList), pInstruction); // hop back
            } else hr = E_FAIL;
            break;
         
         //Jump if the top ScriptVariant resolves to false
        case Branch_FALSE:
         //Jump if the top ScriptVariant resolves to true
        case Branch_TRUE: 
            //cache the reference target
            pLabel = pInstruction->Label;
            pSVar1 = Stack_Top(&(pinterpreter->theDataStack));
            pInstruction->theRef = pSVar1;
            Stack_Pop(&(pinterpreter->theDataStack));
            //cache the jump target
            if(List_FindByName(&(pinterpreter->theInstructionList), pLabel)){
                pInstruction->theJumpTargetIndex = List_GetIndex(&(pinterpreter->theInstructionList));
                List_Includes(&(pinterpreter->theInstructionList), pInstruction); // hop back
            } else hr = E_FAIL;
            break;
         
         //Set the m_bCallCompleted flag to true so we know to stop evalutating
         //instructions
        case RET:
            break;
         
         //Make sure the argument count on the top of the stack matches the
         //number of arguments we have
        case CHECKARG:
            //cache the argument count
            Instruction_ConvertConstant(pInstruction);
            break;
         
         //This instructs the interpreter to clean one value off the stack.
        case CLEAN:
            Stack_Pop(&(pinterpreter->theDataStack));
            break;
         
         //This OpCode denotes an error in the instruction list
        case ERR:
            hr = E_FAIL;
            break;
         
         //This is a placeholder.  Don't do anything.
        case NOOP:
            break; 
         
         //This instructs the interpreter to push a symbol scope.
        case PUSH:
            StackedSymbolTable_PushScope(&(pinterpreter->theSymbolTable), NULL);
            break; 
         
         //This instructs the interpreter to pop a symbol scope
        case POP:
            StackedSymbolTable_PopScope(&(pinterpreter->theSymbolTable));
            break;
         
         //Ignore IMMEDIATE and DEFFERRED instructions, since they are only used
         //to mark immediate code.
        case IMMEDIATE: 
            pinterpreter->bHasImmediateCode = TRUE;
            break;
        case DEFERRED:
            break;
         
         //If we hit the default, then we got an unrecognized instruction
        default:
            hr = E_FAIL;
         
            //Report an error
            //HandleRuntimeError( pInstruction, INVALID_INSTRUCTION, this );
            break;
        }
        // if we fail, go back to normal, clear up the compile productions
        if(FAILED(hr)){
            printf("\nScript compile error in '%s': %s line %d, column %d\n", 
            pinterpreter->theSymbolTable.name, (pInstruction->theToken)?pInstruction->theToken->theSource:"",  
            (pInstruction->theToken)?pInstruction->theToken->theTextPosition.row:-1, 
            (pInstruction->theToken)?pInstruction->theToken->theTextPosition.col:-1);
            List_Reset(&(pinterpreter->theInstructionList));
            for(i=0; i<size; i++){
                pInstruction = (Instruction*)List_Retrieve(&(pinterpreter->theInstructionList));
                if(pInstruction->theVal) tracefree(pInstruction->theVal);
                if(pInstruction->theRefList){
                    List_Clear(pInstruction->theRefList);
                    tracefree(pInstruction->theRefList);
                }
                pInstruction->theRef = pInstruction->theRef2 = NULL;
                List_GotoNext(&(pinterpreter->theInstructionList));
            }
            List_Clear(&(pinterpreter->theInstructionList));
            pinterpreter->mainEntryIndex = -1;
            break;           
        }
        List_GotoNext(&(pinterpreter->theInstructionList));
    }
    // clear some unused properties
    List_Reset(&(pinterpreter->theInstructionList));
    size = List_GetSize(&(pinterpreter->theInstructionList));
    for(i=0; i<size; i++)
    {
        pInstruction = (Instruction*)List_Retrieve(&(pinterpreter->theInstructionList));
        // we might not need these 2, free them to cut some memory usage
        if(pInstruction->theToken) {tracefree(pInstruction->theToken); pInstruction->theToken=NULL;}
        if(pInstruction->Label) {tracefree(pInstruction->Label); pInstruction->Label=NULL;}
        List_GotoNext(&(pinterpreter->theInstructionList));
    }
    // make a solid list that can be referenced by index
    List_Solidify(&(pinterpreter->theInstructionList));
    StackedSymbolTable_Clear(&(pinterpreter->theSymbolTable));
    List_Clear(&(pinterpreter->theDataStack));
    List_Clear(&(pinterpreter->theLabelStack));
    
    return hr;
}


/******************************************************************************
*  EvalInstruction -- This method evaluates a single byte-code instruction.
*  Parameters: none
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Interpreter_EvalInstruction(Interpreter* pinterpreter)
{
    HRESULT hr = S_OK;
    Instruction* pInstruction = NULL;
    Instruction* currentCall;
    Instruction* returnEntry;

    //Retrieve the current instruction from the list
    pInstruction = (Instruction*)(pinterpreter->theInstructionList.solidlist[pinterpreter->theInstructionList.index]);
   
    if (pInstruction){
      
        //The OpCode will tell us what operation to perform.  
        switch( pInstruction->OpCode ){
            //Push a constant string
        case CONSTSTR:
            //Push a constant double
        case CONSTDBL:
            //Push a constant integer
        case CONSTINT:
            ScriptVariant_Copy(pInstruction->theVal2, pInstruction->theVal);
            break;
            
           //Load a value into the data stack
        case LOAD:
            ScriptVariant_Copy(pInstruction->theVal, pInstruction->theRef);
            break;
         
         //Save a value from the data stack
        case SAVE:
            ScriptVariant_Copy(pInstruction->theRef, pInstruction->theRef2);
            break;
         
            //use the UNARYOP macro to do an AutoIncrement
        case INC:
            UNARYOP(ScriptVariant_Inc_Op);
            break;
         
            //use the UNARYOP macro to do an AutoDecrement
        case DEC:
            UNARYOP(ScriptVariant_Dec_Op);
            break;
         
           //use the UNARYOP macro to do a unary plus
        case POS:
            UNARYOP(ScriptVariant_Pos);
            break;
         
         //use the UNARYOP macro to do a unary minus
        case NEG:
            UNARYOP(ScriptVariant_Neg);
            break;
         
         //use the UNARYOP macro to do a logical not
        case NOT:
            UNARYOP(ScriptVariant_Boolean_Not);
            break; 
         
         //Use the BINARYOP macro to do a multipy
        case MUL:
            BINARYOP(ScriptVariant_Mul);
            break;
         
         //Use the BINARYOP macro to do a divide
        case DIV: 
            BINARYOP(ScriptVariant_Div);
            break;

         //Use the BINARYOP macro to do a mod
        case MOD: 
            BINARYOP(ScriptVariant_Mod);
            break;
         
         //Use the BINARYOP macro to do an add
        case ADD:
            BINARYOP(ScriptVariant_Add);
            break;
         
         //Use the BINARYOP macro to do a subtract
        case SUB: 
            BINARYOP(ScriptVariant_Sub);
            break;
         
         //Use the BINARYOP macro to do a greater than- equal
        case GE:
            BINARYOP(ScriptVariant_Ge);
            break; 
         
         //Use the BINARYOP macro to do a less than- equal
        case LE:
            BINARYOP(ScriptVariant_Le);
            break;
         
         //Use the BINARYOP macro to do a less than
        case LT:
            BINARYOP(ScriptVariant_Lt);
            break;
         
         //Use the BINARYOP macro to do a greater than
        case GT:
            BINARYOP(ScriptVariant_Gt);
            break;
         
         //Use the BINARYOP macro to do an equality
        case EQ: 
            BINARYOP(ScriptVariant_Eq);
            break;
         
         //Use the BINARYOP macro to do a not-equal
        case NE: 
            BINARYOP(ScriptVariant_Ne);
            break;
         
         //Use the BINARYOP macro to do a logical OR
        case OR:
            BINARYOP(ScriptVariant_Or);
            break; 
         
         //Use the BINARYOP macro to do a logical AND
        case AND:
           BINARYOP(ScriptVariant_And);
           break;
         
         //Create a new CSymbol and add it to the symbol table
        case DATA:
            //again, we dont have to define anything if compiled
            break;
         
         //Create a new CSymbol from a value on the stack and add it to the 
         //symbol table.
        case PARAM: 
            if(pinterpreter->currentCallIndex>=0){
                //copy value from the cached parameter
                currentCall = (Instruction*)(pinterpreter->theInstructionList.solidlist[pinterpreter->currentCallIndex]);
                ScriptVariant_Copy(pInstruction->theVal, (ScriptVariant*)(currentCall->theRefList->solidlist[currentCall->theRefList->index]));
                currentCall->theRefList->index++;
            }
            else hr = E_FAIL;
            //else
            //HandleRuntimeError( pInstruction, STACK_FAILURE, this );

            break; 

         //Call the specified method, and pass in a ScriptVariant* to receive the 
         //return value.  If it's not NULL, then push it onto the data stack.
        case CALL:
            pInstruction->theRefList->index = 0;
            hr = Interpreter_Call(pinterpreter);
            //Reset the m_bCallCompleted flag back to false
            pinterpreter->bCallCompleted = FALSE;
            break;

       // return 
       case JUMPR:
            pinterpreter->returnEntryIndex = pinterpreter->theInstructionList.index;
            if(pInstruction->theJumpTargetIndex<0)
                hr = E_FAIL;
            else pinterpreter->theInstructionList.index = pInstruction->theJumpTargetIndex;
            break;
         
         //Jump to the specified label
        case JUMP:
            if(pInstruction->theJumpTargetIndex<0)
                hr = E_FAIL;
            else pinterpreter->theInstructionList.index = pInstruction->theJumpTargetIndex;
            break;
         
         //Jump if the top ScriptVariant resolves to false
        case Branch_FALSE:
            if(!ScriptVariant_IsTrue(pInstruction->theRef))
            {
                if(pInstruction->theJumpTargetIndex<0)
                    hr = E_FAIL;
                else pinterpreter->theInstructionList.index = pInstruction->theJumpTargetIndex;
            }
            break;

         //Jump if the top ScriptVariant resolves to true
        case Branch_TRUE: 
            if(ScriptVariant_IsTrue(pInstruction->theRef))
            {
                if(pInstruction->theJumpTargetIndex<0)
                    hr = E_FAIL;
                else pinterpreter->theInstructionList.index = pInstruction->theJumpTargetIndex;
            }
            break;
         
         //Set the m_bCallCompleted flag to true so we know to stop evalutating
         //instructions
        case RET:
            if(pinterpreter->returnEntryIndex >= 0){
                returnEntry = (Instruction*)(pinterpreter->theInstructionList.solidlist[pinterpreter->returnEntryIndex]);
                if(returnEntry->theRef && pinterpreter->currentCallIndex>=0) 
                {
                    currentCall = (Instruction*)(pinterpreter->theInstructionList.solidlist[pinterpreter->currentCallIndex]);
                    ScriptVariant_Copy(currentCall->theVal, returnEntry->theRef);
                }
            }
            pinterpreter->returnEntryIndex = -1;
            pinterpreter->bCallCompleted = TRUE;
            break;
         
         //Make sure the argument count on the top of the stack matches the
         //number of arguments we have
        case CHECKARG:
            if(pinterpreter->currentCallIndex>=0 && 
               pInstruction->theVal->lVal != ((Instruction*)(pinterpreter->theInstructionList.solidlist[pinterpreter->currentCallIndex]))->theRef->lVal)
            {
                printf("Runtime error: argument count(%d) doesn't match, check your function call.\n", (int)pInstruction->theVal->lVal);
                hr = E_FAIL;
            }
            break;

         //This instructs the interpreter to clean one value off the stack.
        case CLEAN:
            break;
         
         //This OpCode denotes an error in the instruction list
        case ERR:
            hr = E_FAIL;
            break;
         
         //This is a placeholder.  Don't do anything.
        case NOOP:
            break; 
         
         //This instructs the interpreter to push a symbol scope.
        case PUSH:
            break; 
         
         //This instructs the interpreter to pop a symbol scope
        case POP:
            break;
         
         //Ignore IMMEDIATE and DEFFERRED instructions, since they are only used
         //to mark immediate code.
        case IMMEDIATE: 
        case DEFERRED:
            break;
         
         //If we hit the default, then we got an unrecognized instruction
        default:
            hr = E_FAIL;
         
            //Report an error
            //HandleRuntimeError( pInstruction, INVALID_INSTRUCTION, this );
            break;
        }
        if(hr==E_FAIL)
        {
            printf("\nOpCode: %d\n", pInstruction->OpCode);
        }
      //Increment the instruction list
        pinterpreter->theInstructionList.index++;
    }

    //return the result of this token evaluation
    return hr;
}

#ifndef COMPILED_SCRIPT
/******************************************************************************
*  OutputPCode -- This method creates a new text file called <fileName>.txt 
*                 that contains a pseudo-assembly representation of the PCode 
*                 stored in this CInterpreter object.  The file cannot be read 
*                 back into the interpreter.  It is created for debugging 
*                 purposes only.
*  Parameters: fileName -- an LPCSTR containing the name of the file to 
*                          store the pseudo-assembly code in.  An LPCSTR is
*                          used for consistency with the rest of the engine.
*  Returns: none
******************************************************************************/
void Interpreter_OutputPCode(Interpreter* pinterpreter, LPCSTR fileName )
{
   FILE* instStream = NULL;
   Instruction* pInstruction = NULL;
   LPCSTR pLabel = NULL;
   LPCSTR pStr;
   int i, size;
   //Declare and initialize some string buffers.
   char* buffer = (char*)tracemalloc("Interpreter_OutputPCode #1", 256);
      
   //If the fileName is "", then substitute "Main".
   if (!strcmp( fileName, "" ))
      strcpy( buffer, "Main" );
   else strcpy(buffer, fileName);
   
   //Add ".txt" to the end of the names so the files get created right
   strcat( buffer, ".txt" );
   
   //Open an output stream for the productions enumeration.
   instStream = fopen(buffer, "w");
   if (!instStream)
      return;
   
   //Iterate through the list of instructions
   FOREACH( pinterpreter->theInstructionList, 
      //Get the next CInstruction
   pInstruction = (Instruction*)List_Retrieve(&(pinterpreter->theInstructionList));
   
   //Re-initialize the buffers
   memset(buffer, 0, 256 );
   
   //If this instruction has a label, then get it and write it to the output
   //buffer
   pLabel = List_GetName(&(pinterpreter->theInstructionList));
   if (pLabel != NULL)
      strcpy(buffer, pLabel);
   strcat( buffer, "\t" );
   
   //Get the pseudo-assembly representation of the CInstruction and concat 
   //it onto the output buffer
   pStr = (char*)tracemalloc("Interpreter_OutputPCode #2", 256);
   Instruction_ToString(pInstruction, (char*)pStr);
   strcat( buffer, pStr );
   //donot forget to delete it
   tracefree((void*)pStr);
   
   strcat( buffer, "\n");

   //Write the output buffer to the output stream
   fprintf(instStream, buffer);
   );
   //Close the output stream
   fclose(instStream);
   tracefree(buffer);
}
#endif

/******************************************************************************
*  Reset -- This method resets the interpreter.
*  Parameters: none
*  Returns: none
******************************************************************************/
void Interpreter_Reset(Interpreter* pinterpreter)
{
    pinterpreter->currentCallIndex=-1;
    pinterpreter->returnEntryIndex=-1;
    pinterpreter->theInstructionList.index = 0;
    //Reset the main flag
    pinterpreter->bMainCompleted = FALSE;
}


