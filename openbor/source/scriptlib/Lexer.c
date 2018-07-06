/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#include "Lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//MACROS
/******************************************************************************
*  CONSUMECHARACTER -- This macro inserts code to remove a character from the 
*  input stream and add it to the current token buffer.
******************************************************************************/
#define CONSUMECHARACTER \
   plexer->theTokenSource[strlen(plexer->theTokenSource)] = *(plexer->pcurChar);\
   plexer->theTokenSource[strlen(plexer->theTokenSource)] = '\0';\
   plexer->pcurChar++; \
   plexer->theTextPosition.col++; \
   plexer->offset++; 

/******************************************************************************
*  MAKETOKEN(x) -- This macro inserts code to create a new CToken object of 
*  type x, using the current token position, and source.
******************************************************************************/
#define MAKETOKEN(x) \
   Token_Init(theNextToken, x, plexer->theTokenSource, plexer->theTokenPosition, \
   plexer->tokOffset);

/******************************************************************************
*  SKIPCHARACTER -- 跳过一个字符，不加入到plexer->theTokenSource中。
*  2007-1-22
******************************************************************************/
#define SKIPCHARACTER \
   plexer->pcurChar++; \
   plexer->theTextPosition.col++; \
   plexer->offset++; 

/******************************************************************************
*  CONSUMEESCAPE -- 读取下一个转义字符，并且修改上一个plexer->theTokenSource为该转义
*  字符代表的字符，参考CONSUMECHARACTER宏。
*  目前支持的转义字符有\r\n\t\s\"\'\\
*  2007-1-22
******************************************************************************/
#define CONSUMEESCAPE \
  switch ( *(plexer->pcurChar))\
  {\
  case 's':\
      plexer->theTokenSource[strlen(plexer->theTokenSource) - 1] = ' ';\
      break;\
  case 'r':\
      plexer->theTokenSource[strlen(plexer->theTokenSource) - 1] = '\r';\
      break;\
  case 'n':\
      plexer->theTokenSource[strlen(plexer->theTokenSource) - 1] = '\n';\
      break;\
  case 't':\
      plexer->theTokenSource[strlen(plexer->theTokenSource) - 1] = '\t';\
      break;\
  case '\"':\
      plexer->theTokenSource[strlen(plexer->theTokenSource) - 1] = '\"';\
      break;\
  case '\'':\
      plexer->theTokenSource[strlen(plexer->theTokenSource) - 1] = '\'';\
      break;\
  case '\\':\
      plexer->theTokenSource[strlen(plexer->theTokenSource) - 1] = '\\';\
      break;\
  default:\
      CONSUMECHARACTER;\
      CONSUMECHARACTER;\
      MAKETOKEN( TOKEN_ERROR );\
      return E_FAIL;\
  }\
   plexer->pcurChar++; \
   plexer->theTextPosition.col++; \
   plexer->offset++; 
  

//Constructor
void Token_Init(Token* ptoken, MY_TOKEN_TYPE theType, LPCSTR theSource, TEXTPOS theTextPosition, ULONG charOffset)
{
    ptoken->theType = theType;
    ptoken->theTextPosition = theTextPosition;
    ptoken->charOffset = charOffset;
    strcpy(ptoken->theSource, theSource );
}


void Lexer_Init(Lexer* plexer, LPCSTR theSource, TEXTPOS theStartingPosition)
{
     plexer->ptheSource = theSource;
     plexer->theTextPosition = theStartingPosition;
     plexer->pcurChar = (CHAR*)plexer->ptheSource;
     plexer->offset = 0;
     plexer->tokOffset = 0;
     /*pl = plexer;*/
}

void Lexer_Clear(Lexer* plexer)
{
    memset(plexer, 0, sizeof(Lexer));
}

/******************************************************************************
*  getNextToken -- Thie method searches the input stream and returns the next 
*  token found within that stream, using the principle of maximal munch.  It
*  embodies the start state of the FSA.
*  
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Lexer_GetNextToken(Lexer* plexer, Token* theNextToken)
{
   //start a whitespace-eating loop
   for(;;){
      memset(plexer->theTokenSource, 0, MAX_TOKEN_LENGTH * sizeof(CHAR));
      plexer->theTokenPosition = plexer->theTextPosition;
      plexer->tokOffset = plexer->offset;
      
      //Whenever we get a new token, we need to watch out for the end-of-input.
      //Otherwise, we could walk right off the end of the stream.
      if ( !strncmp( plexer->pcurChar, "\0", 1)){   //A null character marks the end
         //of the stream
         MAKETOKEN( TOKEN_EOF );
         return S_OK;
      }
      
      //getNextToken eats whitespace
      //newline
      else if ( !strncmp( plexer->pcurChar, "\n", 1)){
         //increment the line counter and reset the offset counter
         plexer->theTextPosition.col = 0;
         plexer->theTextPosition.row++;
         plexer->pcurChar++;
         plexer->offset++;
      }
      
      //tab
      else if ( !strncmp( plexer->pcurChar, "\t", 1)){
         //increment the offset counter by TABSIZE
         plexer->theTextPosition.col += TABSIZE;
         plexer->pcurChar++;
         plexer->offset++;
      }
      
      //carriage return
      else if ( !strncmp( plexer->pcurChar, "\r", 1)){
         //reset the offset counter to zero
         plexer->theTextPosition.col = 0;
         //increment the line counter
         //plexer->theTextPosition.row++;
         plexer->pcurChar++;
         plexer->offset++;
      }
      
      //line feed
      else if ( !strncmp( plexer->pcurChar, "\f", 1)){
         //increment the line counter
         plexer->theTextPosition.row++;
         plexer->pcurChar++;
         plexer->offset++;
      }
      
      //space
      else if ( !strncmp(plexer->pcurChar, " ", 1)){
         //increment the offset counter
         plexer->theTextPosition.col++;
         plexer->pcurChar++;
         plexer->offset++;
      }
      
      //an Identifier starts with an alphabetical character or underscore
      else if ( *plexer->pcurChar=='_' || (*plexer->pcurChar>= 'a' && *plexer->pcurChar <= 'z') ||
          (*plexer->pcurChar >= 'A' && *plexer->pcurChar <= 'Z')){ 
         return Lexer_GetTokenIdentifier(plexer, theNextToken );
      }
      
      //a Number starts with a numerical character
      else if ((*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9') ){
         return Lexer_GetTokenNumber(plexer, theNextToken );
      }
      //string
      else if (!strncmp( plexer->pcurChar, "\"", 1)){
         return Lexer_GetTokenStringLiteral(plexer, theNextToken );
      }
      //character
      else if (!strncmp( plexer->pcurChar, "'", 1)){
          //skip first '    2007 - 1 - 22
         SKIPCHARACTER;
         //escape characters    
         if (!strncmp( plexer->pcurChar, "\\", 1)){
            CONSUMECHARACTER;
            CONSUMEESCAPE;
         }
         //must not be an empty character
         else if(strncmp( plexer->pcurChar, "'", 1))
         {
            CONSUMECHARACTER;
         }
         else
         {
            CONSUMECHARACTER;
            CONSUMECHARACTER;
            MAKETOKEN( TOKEN_ERROR );
            return S_OK;
         }
         //skip last '
         if (!strncmp( plexer->pcurChar, "'", 1)){
            SKIPCHARACTER;
            MAKETOKEN( TOKEN_STRING_LITERAL );
            return S_OK;
         }
         else{
            CONSUMECHARACTER;
            CONSUMECHARACTER;
            MAKETOKEN( TOKEN_ERROR );
            return S_OK;
         }
      }
      
      //Before checking for comments
      else if ( !strncmp( plexer->pcurChar, "/", 1)){
         CONSUMECHARACTER;      
         if ( !strncmp( plexer->pcurChar, "/", 1)){
            Lexer_SkipComment(plexer, COMMENT_SLASH);
         }
         else if ( !strncmp( plexer->pcurChar, "*", 1)){
            Lexer_SkipComment(plexer, COMMENT_STAR);
         }
         
         //Now complete the symbol scan for regular symbols.
         else if ( !strncmp( plexer->pcurChar, "=", 1)){
            CONSUMECHARACTER;         
            MAKETOKEN( TOKEN_DIV_ASSIGN );
            return S_OK;
         }
         else{
            MAKETOKEN( TOKEN_DIV);
            return S_OK;
         }
      }
      
      //a Symbol starts with one of these characters
      else if (( !strncmp( plexer->pcurChar, ">", 1)) || ( !strncmp( plexer->pcurChar, "<", 1))
         || ( !strncmp( plexer->pcurChar, "+", 1)) || ( !strncmp( plexer->pcurChar, "-", 1)) 
         || ( !strncmp( plexer->pcurChar, "*", 1)) || ( !strncmp( plexer->pcurChar, "/", 1)) 
         || ( !strncmp( plexer->pcurChar, "%", 1)) || ( !strncmp( plexer->pcurChar, "&", 1)) 
         || ( !strncmp( plexer->pcurChar, "^", 1)) || ( !strncmp( plexer->pcurChar, "|", 1))
         || ( !strncmp( plexer->pcurChar, "=", 1)) || ( !strncmp( plexer->pcurChar, "!", 1)) 
         || ( !strncmp( plexer->pcurChar, ";", 1)) || ( !strncmp( plexer->pcurChar, "{", 1))
         || ( !strncmp( plexer->pcurChar, "}", 1)) || ( !strncmp( plexer->pcurChar, ",", 1))
         || ( !strncmp( plexer->pcurChar, ":", 1)) || ( !strncmp( plexer->pcurChar, "(", 1)) 
         || ( !strncmp( plexer->pcurChar, ")", 1)) || ( !strncmp( plexer->pcurChar, "[", 1)) 
         || ( !strncmp( plexer->pcurChar, "]", 1)) || ( !strncmp( plexer->pcurChar, ".", 1))
         || ( !strncmp( plexer->pcurChar, "~", 1)) || ( !strncmp( plexer->pcurChar, "?", 1)))
      {
         return Lexer_GetTokenSymbol(plexer, theNextToken );
      }
      
      //If we get here, we've hit a character we don't recognize
      else{
         //Consume the character so it can be sent to the error handler
         CONSUMECHARACTER;

         //Create an error token, and send it to the error handler
         MAKETOKEN( TOKEN_ERROR );
         //HandleCompileError( *theNextToken, UNRECOGNIZED_CHARACTER );
      }
   }
}

/******************************************************************************
*  Identifier -- This method extracts an identifier from the stream, once it's
*  recognized as an identifier.  After it is extracted, this method determines
*  if the identifier is a keyword.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Lexer_GetTokenIdentifier(Lexer* plexer, Token* theNextToken)
{
   //copy the source that makes up this token
   //an identifier is a string of letters, digits and/or underscores
   do{
      CONSUMECHARACTER;
   }while ((*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9')  || 
       (*plexer->pcurChar >= 'a' && *plexer->pcurChar <= 'z')  || 
       (*plexer->pcurChar >= 'A' && *plexer->pcurChar <= 'Z') ||
      ( !strncmp( plexer->pcurChar, "_", 1)));
   
   //Check the Identifier against current keywords
   if (!strcmp( plexer->theTokenSource, "auto")){
      MAKETOKEN( TOKEN_AUTO );}
   else if (!strcmp( plexer->theTokenSource, "break")){
      MAKETOKEN( TOKEN_BREAK );}
   else if (!strcmp( plexer->theTokenSource, "case")){
      MAKETOKEN( TOKEN_CASE );}
   else if (!strcmp( plexer->theTokenSource, "char")){
      MAKETOKEN( TOKEN_CHAR );}
   else if (!strcmp( plexer->theTokenSource, "const")){
      MAKETOKEN( TOKEN_CONST );}
   else if (!strcmp( plexer->theTokenSource, "continue")){
      MAKETOKEN( TOKEN_CONTINUE );}
   else if (!strcmp( plexer->theTokenSource, "default")){
      MAKETOKEN( TOKEN_DEFAULT );}
   else if (!strcmp( plexer->theTokenSource, "do")){
      MAKETOKEN( TOKEN_DO );}
   else if (!strcmp( plexer->theTokenSource, "double")){
      MAKETOKEN( TOKEN_DOUBLE );}
   else if (!strcmp( plexer->theTokenSource, "else")){
      MAKETOKEN( TOKEN_ELSE );}
   else if (!strcmp( plexer->theTokenSource, "enum")){
      MAKETOKEN( TOKEN_ENUM );}
   else if (!strcmp( plexer->theTokenSource, "extern")){
      MAKETOKEN( TOKEN_EXTERN );}
   else if (!strcmp( plexer->theTokenSource, "float")){
      MAKETOKEN( TOKEN_FLOAT );}
   else if (!strcmp( plexer->theTokenSource, "for")){
      MAKETOKEN( TOKEN_FOR );}
   else if (!strcmp( plexer->theTokenSource, "goto")){
      MAKETOKEN( TOKEN_GOTO );}
   else if (!strcmp( plexer->theTokenSource, "if")){
      MAKETOKEN( TOKEN_IF );}
   else if (!strcmp( plexer->theTokenSource, "int")){
      MAKETOKEN( TOKEN_INT );}
   else if (!strcmp( plexer->theTokenSource, "long")){
      MAKETOKEN( TOKEN_LONG );}
   else if (!strcmp( plexer->theTokenSource, "register")){
      MAKETOKEN( TOKEN_REGISTER );}
   else if (!strcmp( plexer->theTokenSource, "return")){
      MAKETOKEN( TOKEN_RETURN );}
   else if (!strcmp( plexer->theTokenSource, "short")){
      MAKETOKEN( TOKEN_SHORT );}
   else if (!strcmp( plexer->theTokenSource, "signed")){
      MAKETOKEN( TOKEN_SIGNED );}
   else if (!strcmp( plexer->theTokenSource, "sizeof")){
      MAKETOKEN( TOKEN_SIZEOF );}
   else if (!strcmp( plexer->theTokenSource, "static")){
      MAKETOKEN( TOKEN_STATIC );}
   else if (!strcmp( plexer->theTokenSource, "struct")){
      MAKETOKEN( TOKEN_STRUCT );}
   else if (!strcmp( plexer->theTokenSource, "switch")){
      MAKETOKEN( TOKEN_SWITCH );}
   else if (!strcmp( plexer->theTokenSource, "typedef")){
      MAKETOKEN( TOKEN_TYPEDEF );}
   else if (!strcmp( plexer->theTokenSource, "union")){
      MAKETOKEN( TOKEN_UNION );}
   else if (!strcmp( plexer->theTokenSource, "unsigned")){
      MAKETOKEN( TOKEN_UNSIGNED );}
   else if (!strcmp( plexer->theTokenSource, "void")){
      MAKETOKEN( TOKEN_VOID );}
   else if (!strcmp( plexer->theTokenSource, "volatile")){
      MAKETOKEN( TOKEN_VOLATILE );}
   else if (!strcmp( plexer->theTokenSource, "while")){
      MAKETOKEN( TOKEN_WHILE );}
   else{
      MAKETOKEN( TOKEN_IDENTIFIER );}
   
   return S_OK;
}

/******************************************************************************
*  Number -- This method extracts a numerical constant from the stream.  It
*  only extracts the digits that make up the number.  No conversion from string
*  to numeral is performed here.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Lexer_GetTokenNumber(Lexer* plexer, Token* theNextToken)
{
   //copy the source that makes up this token
   //a constant is one of these:
   
   //0[xX][a-fA-F0-9]+{u|U|l|L}
   //0{D}+{u|U|l|L}
   if (( !strncmp( plexer->pcurChar, "0X", 2)) || ( !strncmp( plexer->pcurChar, "0x", 2))){
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      while ((*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9') ||
          (*plexer->pcurChar >= 'a' && *plexer->pcurChar <= 'f') ||
          (*plexer->pcurChar >= 'A' && *plexer->pcurChar <= 'F'))
      {
         CONSUMECHARACTER;
      }
      
      if (( !strncmp( plexer->pcurChar, "u", 1)) || ( !strncmp( plexer->pcurChar, "U", 1)) || 
         ( !strncmp( plexer->pcurChar, "l", 1)) || ( !strncmp( plexer->pcurChar, "L", 1)))
      {
         CONSUMECHARACTER;
      }
      
      MAKETOKEN( TOKEN_HEXCONSTANT );
   }
   else{
      while (*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9') 
      {
         CONSUMECHARACTER;
      }
      
      if (( !strncmp( plexer->pcurChar, "E", 1)) || ( !strncmp( plexer->pcurChar, "e", 1)))
      {
         CONSUMECHARACTER; 
         while (*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9') 
         {
            CONSUMECHARACTER;
         }
         
         if (( !strncmp( plexer->pcurChar, "f", 1)) || ( !strncmp( plexer->pcurChar, "F", 1)) || 
            ( !strncmp( plexer->pcurChar, "l", 1)) || ( !strncmp( plexer->pcurChar, "L", 1)))
         {
            CONSUMECHARACTER;
         }
         
         MAKETOKEN( TOKEN_FLOATCONSTANT );
      }
      else if ( !strncmp( plexer->pcurChar, ".", 1))
      {
         CONSUMECHARACTER;         
         while (*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9') 
         {
            CONSUMECHARACTER;
         }
         
         if (( !strncmp( plexer->pcurChar, "E", 1)) || ( !strncmp( plexer->pcurChar, "e", 1)))
         {
            CONSUMECHARACTER;
            
            while (*plexer->pcurChar >= '0' && *plexer->pcurChar <= '9')
            {
               CONSUMECHARACTER;
            }
            
            if (( !strncmp( plexer->pcurChar, "f", 1)) ||
               ( !strncmp( plexer->pcurChar, "F", 1)) || 
               ( !strncmp( plexer->pcurChar, "l", 1)) || 
               ( !strncmp( plexer->pcurChar, "L", 1)))
            {
               CONSUMECHARACTER;
            }            
         }
         MAKETOKEN( TOKEN_FLOATCONSTANT );
         
      }
      else{
         MAKETOKEN( TOKEN_INTCONSTANT );
      }
   }
   return S_OK;
}

/******************************************************************************
*  StringLiteral -- This method extracts a string literal from the character
*  stream.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Lexer_GetTokenStringLiteral(Lexer* plexer, Token* theNextToken)
{
   //copy the source that makes up this token
   //an identifier is a string of letters, digits and/or underscores
   //skip that first quote mark
   int esc = 0;
   SKIPCHARACTER;
   while ( strncmp( plexer->pcurChar, "\"", 1))
   {
      if(!strncmp( plexer->pcurChar, "\\", 1))
      {
          esc = 1;
      }
      CONSUMECHARACTER;
      if(esc)
      {
        CONSUMEESCAPE;
        esc = 0;
      }
   }
   
   //skip that last quote mark
   SKIPCHARACTER;
   
   MAKETOKEN( TOKEN_STRING_LITERAL );
   return S_OK;
}
/******************************************************************************
*  Symbol -- This method extracts a symbol from the character stream.  For the 
*  purposes of lexing, comments are considered symbols.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Lexer_GetTokenSymbol(Lexer* plexer, Token* theNextToken)
{
   //">>="
   if ( !strncmp( plexer->pcurChar, ">>=", 3))
   {
      CONSUMECHARACTER;      
      CONSUMECHARACTER;      
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_RIGHT_ASSIGN );
   }
   
   //">>"
   else if ( !strncmp( plexer->pcurChar, ">>", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_RIGHT_OP );
   }

   //">="
   else if ( !strncmp( plexer->pcurChar, ">=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_GE_OP );
   }

   //">"
   else if ( !strncmp( plexer->pcurChar, ">", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_GT );
   }
   
   //"<<="
   else if ( !strncmp( plexer->pcurChar, "<<=", 3))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_LEFT_ASSIGN );
   }
   
   //"<<"
   else if ( !strncmp( plexer->pcurChar, "<<", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_LEFT_OP );
   }
   
   //"<="
   else if ( !strncmp( plexer->pcurChar, "<=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_LE_OP );
   }

   //"<"
   else if ( !strncmp( plexer->pcurChar, "<", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_LT );
   }
   
   //"++"
   else if ( !strncmp( plexer->pcurChar, "++", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_INC_OP );
   }
   
   //"+="
   else if ( !strncmp( plexer->pcurChar, "+=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_ADD_ASSIGN );
   }
   
   //"+"
   else if ( !strncmp( plexer->pcurChar, "+", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_ADD );
   }
   
   //"--"
   else if ( !strncmp( plexer->pcurChar, "--", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_DEC_OP );
   }
   
   //"-="
   else if ( !strncmp( plexer->pcurChar, "-=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_SUB_ASSIGN );
   }
   
   //"-"
   else if ( !strncmp( plexer->pcurChar, "-", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_SUB );
   }
   
   //"*="
   else if ( !strncmp( plexer->pcurChar, "*=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_MUL_ASSIGN );
   }
   
   //"*"
   else if ( !strncmp( plexer->pcurChar, "*", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_MUL );
   }
   
   //"%="
   else if ( !strncmp( plexer->pcurChar, "%=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_MOD_ASSIGN );
   }
   
   //"%"
   else if ( !strncmp( plexer->pcurChar, "%", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_MOD );
   }
   
   //"&&"
   else if ( !strncmp( plexer->pcurChar, "&&", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_AND_OP );
   }
   
   //"&="
   else if ( !strncmp( plexer->pcurChar, "&=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_AND_ASSIGN );
   }
   
   //"&"
   else if ( !strncmp( plexer->pcurChar, "&", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_BITWISE_AND );
   }
   
   //"^="
   else if ( !strncmp( plexer->pcurChar, "^=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_XOR_ASSIGN );
   }
   
   //"^"
   else if ( !strncmp( plexer->pcurChar, "^", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_XOR );
   }
   
   //"||"
   else if ( !strncmp( plexer->pcurChar, "||", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_OR_OP );
   }
   
   //"|="
   else if ( !strncmp( plexer->pcurChar, "|=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_OR_ASSIGN );
   }
   
    //"|"
   else if ( !strncmp( plexer->pcurChar, "|", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_BITWISE_OR );
   }
   
   //"=="
   else if ( !strncmp( plexer->pcurChar, "==", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_EQ_OP );
   }
   
  //"="
   else if ( !strncmp( plexer->pcurChar, "=", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_ASSIGN );
   }
   
   //"!="
   else if ( !strncmp( plexer->pcurChar, "!=", 2))
   {
      CONSUMECHARACTER;
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_NE_OP );
   }
   
   //"!"
   else if ( !strncmp( plexer->pcurChar, "!", 1))
   {
      CONSUMECHARACTER;
      MAKETOKEN( TOKEN_BOOLEAN_NOT );
   }
   
   //";"
   else if ( !strncmp( plexer->pcurChar, ";", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_SEMICOLON);
   }
   
   //"{"
   else if ( !strncmp( plexer->pcurChar, "{", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_LCURLY);
   }
   
   //"}"
   else if ( !strncmp( plexer->pcurChar, "}", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_RCURLY);
   }
   
   //","
   else if ( !strncmp( plexer->pcurChar, ",", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_COMMA);
   }
   
   //":"
   else if ( !strncmp( plexer->pcurChar, ":", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_COLON);
   }
   
   //"("
   else if ( !strncmp( plexer->pcurChar, "(", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_LPAREN);
   }
   
   //")"
   else if ( !strncmp( plexer->pcurChar, ")", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_RPAREN);
   }
   
   //"["
   else if ( !strncmp( plexer->pcurChar, "[", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_LBRACKET);
   }
   
   //"]"
   else if ( !strncmp( plexer->pcurChar, "]", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_RBRACKET);
   }
   
   //"."
   else if ( !strncmp( plexer->pcurChar, ".", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_FIELD);
   }
   
   //"~"
   else if ( !strncmp( plexer->pcurChar, "~", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_BITWISE_NOT);
   }
   
   //"?"
   else if ( !strncmp( plexer->pcurChar, "?", 1))
   {
      CONSUMECHARACTER;      
      MAKETOKEN( TOKEN_CONDITIONAL);
   }
   
   return S_OK;
}

/******************************************************************************
*  Comment -- This method extracts a symbol from the character stream.
*  Parameters: theNextToken -- address of the next CToken found in the stream
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
HRESULT Lexer_SkipComment(Lexer* plexer, COMMENT_TYPE theType)
{
   
   if (theType == COMMENT_SLASH){
      do{
         SKIPCHARACTER;
         //break out if we hit a new line
         if (!strncmp( plexer->pcurChar, "\n", 1)){
            plexer->theTextPosition.col = 0;
            plexer->theTextPosition.row++;
            break;
         }
         else if (!strncmp( plexer->pcurChar, "\r", 1)){
            plexer->theTextPosition.col = 0;
            //plexer->theTextPosition.row++;
            break;
         }
         else if (!strncmp( plexer->pcurChar, "\f", 1)){
            plexer->theTextPosition.row++;
            break;
         }
      }while (strncmp( plexer->pcurChar, "\0", 1));
   }
   else if (theType == COMMENT_STAR){
      //consume the '*' that gets this comment started
      SKIPCHARACTER;
      
      //loop through the characters till we hit '*/'
      while (strncmp( plexer->pcurChar, "\0", 1)){
         if (0==strncmp( plexer->pcurChar, "*/", 2)){
            SKIPCHARACTER;
            SKIPCHARACTER;
            break;
         }
         else if (!strncmp( plexer->pcurChar, "\n", 1)){
            plexer->theTextPosition.col = 0;
            plexer->theTextPosition.row++;
         }
         else if (!strncmp( plexer->pcurChar, "\r", 1)){
            plexer->theTextPosition.col = 0;
            //plexer->theTextPosition.row++;
         }
         else if (!strncmp( plexer->pcurChar, "\f", 1)){
            plexer->theTextPosition.row++;
         }
         SKIPCHARACTER;
      };
   }
   
   return S_OK;
}
