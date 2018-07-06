/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD licence, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

/* This file include all script methods used by openbor engine

 Notice: Make sure to null *pretvar when you about to return E_FAIL,
         Or the engine might crash.
         
 Notice: Every new ScriptVariant must be initialized when you first alloc it by 
         ScriptVariant_Init immediately, memset it all to zero should also work by now, 
         unless VT_EMPTY is changed.
         
         If you want to reset a ScriptVariant to empty, you must use ScriptVariant_Clear instead.
         ScriptVariant_Init or memset must be called only ONCE, later you should use ScriptVariant_Clear.
         
         Besure to call ScriptVariant_Clear if you want to use tracefree to delete those variants.
         
         If you want to copy a ScriptVariant from another, use ScriptVariant_Copy instead of assignment,
         not because it is faster, but this method is neccessary for string types.
         
         If you want to change types of an ScriptVariant, use ScriptVariant_ChangeType, don't change vt directly.
         
*/

#include "openborscript.h"
#include "openbor.h"
#include "soundmix.h"
#include "globals.h"

extern int			  PLAYER_MIN_Z;
extern int			  PLAYER_MAX_Z;
extern int			  BGHEIGHT;
extern int            MAX_WALL_HEIGHT;
extern int			  SAMPLE_GO;
extern int			  SAMPLE_BEAT;
extern int			  SAMPLE_BLOCK;
extern int			  SAMPLE_INDIRECT;
extern int			  SAMPLE_GET;
extern int			  SAMPLE_GET2;
extern int			  SAMPLE_FALL;
extern int			  SAMPLE_JUMP;
extern int			  SAMPLE_PUNCH;
extern int			  SAMPLE_1UP;
extern int			  SAMPLE_TIMEOVER;
extern int			  SAMPLE_BEEP;
extern int			  SAMPLE_BEEP2;
extern int			  SAMPLE_BIKE;
extern int            current_palette;
extern s_player       player[4];
extern s_savedata     savedata;
extern s_savelevel    savelevel[MAX_DIFFICULTIES];
extern s_savescore    savescore;
extern s_level        *level;
extern entity         *self;
extern int            *animspecials;
extern int            *animattacks;
extern int            *animfollows;
extern int            *animpains;
extern int            *animfalls;
extern int            *animrises;
extern int            *animriseattacks;
extern int            *animdies;
extern int            *animidles;
extern int            *animwalks;
extern int            *animbackwalks;
extern int            *animups;
extern int            *animdowns;
extern int            noshare;
extern int            credits;
extern char           musicname[128];
extern float          musicfade[2];
extern int            musicloop;
extern unsigned long  musicoffset;

extern unsigned char* blendings[MAX_BLENDINGS];
extern int            current_palette;
s_variantnode** global_var_list = NULL;
Script* pcurrentscript = NULL;//used by local script functions
static List   theFunctionList;
static List   scriptheap;
static s_spawn_entry spawnentry;
extern s_drawmethod plainmethod;
static s_drawmethod drawmethod;

int max_global_var_index = -1;

ScriptVariant* indexed_var_list = NULL;
int            max_global_vars = MAX_GLOBAL_VAR;
int            max_indexed_vars = 0;
int            max_entity_vars = 0;
int            max_script_vars = 0;


//this function should be called before all script methods, for once
void Script_Global_Init()
{
    int i;
    if(max_global_vars>0)
    {
        global_var_list = tracemalloc("Script_Global_Init", sizeof(s_variantnode*)*max_global_vars);
        memset(global_var_list, 0, sizeof(s_variantnode*)*max_global_vars);
    }
    for(i=0; i<max_global_vars; i++)
    {
        global_var_list[i] = tracemalloc("Script_Global_Init#1", sizeof(s_variantnode));
        memset(global_var_list[i], 0, sizeof(s_variantnode));
    }
    max_global_var_index = -1;
    memset(&spawnentry, 0, sizeof(s_spawn_entry));//clear up the spawn entry
    drawmethod = plainmethod;
    if(max_indexed_vars>0) 
    {
        indexed_var_list = (ScriptVariant*)tracemalloc("Script_Global_Init#2", sizeof(ScriptVariant)*max_indexed_vars);
        memset(indexed_var_list, 0, sizeof(ScriptVariant)*max_indexed_vars);
    }
    List_Init(&theFunctionList);
    Script_LoadSystemFunctions();
    List_Init(&scriptheap);
}

//this function should only be called when the engine is shutting down
void Script_Global_Clear()
{
    int i, size;
    List_Clear(&theFunctionList);
    // dump all un-freed variants
    size = List_GetSize(&scriptheap);
    if(size>0) printf("\nWarning: %d script variants are not freed, dumping...\n", size);
    for(i=0, List_Reset(&scriptheap); i<size; List_GotoNext(&scriptheap), i++)
    {
        printf("%s\n", List_GetName(&scriptheap));
        tracefree(List_Retrieve(&scriptheap));
    }
    List_Clear(&scriptheap);
    // clear the global list
    if(global_var_list)
    {
        for(i=0; i<max_global_vars; i++)
        {
            if(global_var_list[i] != NULL) {ScriptVariant_Clear(&(global_var_list[i]->value));tracefree(global_var_list[i]);}
            global_var_list[i] = NULL;
        }
        tracefree(global_var_list);
        global_var_list = NULL;
    }
    if(indexed_var_list)
    {
        for(i=0; i<max_indexed_vars; i++) ScriptVariant_Clear(indexed_var_list+i); 
        tracefree(indexed_var_list);
    }
    indexed_var_list = NULL;
    max_global_var_index = -1;
    memset(&spawnentry, 0, sizeof(s_spawn_entry));//clear up the spawn entry
    StrCache_Clear();
}


ScriptVariant* Script_Get_Global_Variant(char* theName)
{
    int i;

    if(!theName || !theName[0]) return NULL;

    for(i=0; i<=max_global_var_index; i++){
        if(!global_var_list[i]->owner &&
           strcmp(theName, global_var_list[i]->key)==0)
            return &(global_var_list[i]->value);
    }

    return NULL;
}

// local function
int _set_var(char* theName, ScriptVariant* var, Script* owner)
{
    int i;
    s_variantnode* tempnode;
    if(!theName[0] || !theName || (owner && !owner->initialized)) return 0;
    // search the name
    for(i=0; i<=max_global_var_index; i++)
    {
        if(global_var_list[i]->owner == owner &&
           !strcmp(theName, global_var_list[i]->key))
        {
            if(var->vt != VT_EMPTY)
                ScriptVariant_Copy(&(global_var_list[i]->value), var);
            else // set to null, so remove this value
            {
                /// re-adjust bounds, swap with last node
                if(i!=max_global_var_index)
                {
                    tempnode = global_var_list[i];
                    global_var_list[i] = global_var_list[max_global_var_index];
                    global_var_list[max_global_var_index] = tempnode;
                }
                max_global_var_index--;
            }
            return 1;
        }
    }
    if(var->vt == VT_EMPTY) return 1;
    // all slots are taken
    if(max_global_var_index >= max_global_vars-1)
        return 0;
    // so out of bounds, find another slot
    else
    {
        ++max_global_var_index;
        ScriptVariant_Copy(&(global_var_list[max_global_var_index]->value), var);
        global_var_list[max_global_var_index]->owner = owner;
        strcpy(global_var_list[max_global_var_index]->key, theName);
        return 1;
    }
}// end of _set_var

int Script_Set_Global_Variant(char* theName, ScriptVariant* var)
{
    return _set_var(theName, var, NULL);
}

void Script_Local_Clear()
{
    int i;
    s_variantnode* tempnode;
    if(!pcurrentscript) return;
    for(i=0; i<=max_global_var_index; i++)
    {
        if(global_var_list[i]->owner == pcurrentscript)
        {
            if(i!=max_global_var_index)
            {
                tempnode = global_var_list[i];
                global_var_list[i] = global_var_list[max_global_var_index];
                global_var_list[max_global_var_index] = tempnode;
            }
            max_global_var_index--;
        }
    }
    if(pcurrentscript->vars)
        for(i=0; i<max_script_vars; i++) ScriptVariant_Clear(pcurrentscript->vars+i);
}


ScriptVariant* Script_Get_Local_Variant(char* theName)
{
    int i;

    if(!pcurrentscript || !pcurrentscript->initialized || 
       !theName || !theName[0]) return NULL;

    for(i=0; i<=max_global_var_index; i++)
    {
        if(global_var_list[i]->owner == pcurrentscript &&
           strcmp(theName, global_var_list[i]->key)==0)
            return &(global_var_list[i]->value);
    }

    return NULL;
}

int Script_Set_Local_Variant(char* theName, ScriptVariant* var)
{
    if(!pcurrentscript) return 0;
    return _set_var(theName, var, pcurrentscript);
}

Script* alloc_script()
{
    int i;
    Script* pscript = (Script*)tracemalloc("alloc_script", sizeof(Script));
    memset(pscript, 0, sizeof(Script));
    if(max_script_vars>0) 
    {
        pscript->vars = (ScriptVariant*)tracemalloc("alloc_script#pscript->vars", sizeof(ScriptVariant)*max_script_vars);
        for(i=0; i<max_script_vars; i++) ScriptVariant_Init(pscript->vars+i);
    }
    return pscript;
}

void Script_Init(Script* pscript, char* theName, int first)
{
    int i;
    if(first)
    {
        memset(pscript, 0, sizeof(Script));
        if(max_script_vars>0) 
        {
            pscript->vars = (ScriptVariant*)tracemalloc("Script_Init#pscript->vars", sizeof(ScriptVariant)*max_script_vars);
            for(i=0; i<max_script_vars; i++) ScriptVariant_Init(pscript->vars+i);
        }
    }
    if(!theName || !theName[0])  return; // if no name specified, only alloc the variants
    pcurrentscript = pscript; //used by local script functions
    pscript->pinterpreter = (Interpreter*)tracemalloc("Script_Init#2", sizeof(Interpreter));
    Interpreter_Init(pscript->pinterpreter, theName, &theFunctionList);
    pscript->interpreterowner = 1; // this is the owner, important
    pscript->initialized = 1;
}

//safe copy method
void Script_Copy(Script* pdest, Script* psrc, int localclear)
{
    if(!psrc->initialized) return;
    if(pdest->initialized) Script_Clear(pdest, localclear);
    pdest->pinterpreter = psrc->pinterpreter;
    pdest->interpreterowner = 0; // dont own it
    pdest->initialized = psrc->initialized; //just copy, it should be 1
}

void Script_Clear(Script* pscript, int localclear)
{
    Script* temp;
    int i;
    ScriptVariant* pvars;
    if(localclear==2 && pscript->vars) 
    {
        for(i=0; i<max_script_vars; i++)
        {
            ScriptVariant_Clear(pscript->vars+i);
        }
        tracefree(pscript->vars); 
        pscript->vars = NULL;
    }
    if(!pscript->initialized) return;
    temp = pcurrentscript;
    pcurrentscript = pscript; //used by local script functions
    //if it is the owner, free the interpreter
    if(pscript->pinterpreter && pscript->interpreterowner){
        Interpreter_Clear(pscript->pinterpreter);
        tracefree(pscript->pinterpreter);
        pscript->pinterpreter = NULL;
    }
    if(localclear) Script_Local_Clear();
    pvars = pscript->vars; // in game clear(localclear!=2) just keep this value
    memset(pscript, 0, sizeof(Script));
    pscript->vars = pvars; // copy it back
    pcurrentscript = temp;
}

//append part of the script
//Because the script might not be initialized in 1 time.
int Script_AppendText(Script* pscript, char* text, char* path)
{
    pcurrentscript = pscript; //used by local script functions
    //printf(text);
    Interpreter_Reset(pscript->pinterpreter);
    return SUCCEEDED(Interpreter_ParseText(pscript->pinterpreter, text, 1, path));
}

//should be called only once after parsing text
int Script_Compile(Script* pscript)
{
    int result;
    if(!pscript || !pscript->pinterpreter) return 1;
    //Interpreter_OutputPCode(pscript->pinterpreter, "code");
    result = SUCCEEDED(Interpreter_CompileInstructions(pscript->pinterpreter));
    if(!result) {Script_Clear(pscript, 1);shutdown(1, "Can't compile script!\n");}
    return result;
}

int Script_IsInitialized(Script* pscript)
{
    if(pscript && pscript->initialized) pcurrentscript = pscript; //used by local script functions
    return pscript->initialized;
}

//execute the script
int Script_Execute(Script* pscript)
{
    int result=S_OK;
    Script* temp = pcurrentscript;
    pcurrentscript = pscript; //used by local script functions
    Interpreter_Reset(pscript->pinterpreter);
    result = (int)SUCCEEDED(Interpreter_EvaluateImmediate(pscript->pinterpreter));
    pcurrentscript = temp;
    if(!result) shutdown(1, "There's an exception while executing script '%s'.\n", pscript->pinterpreter->theSymbolTable.name);
    return result;
}

#ifndef COMPILED_SCRIPT
//this method is for debug purpose
int Script_Call(Script* pscript, char* method, ScriptVariant* pretvar)
{
    pcurrentscript = pscript; //used by local script functions
    Interpreter_Reset(pscript->pinterpreter);
    return (int)SUCCEEDED(Interpreter_Call(pscript->pinterpreter, method, pretvar));
}
#endif

//used by Script_Global_Init
void Script_LoadSystemFunctions()
{
    //printf("Loading system script functions....");
    //load system functions if we need
    List_Reset(&theFunctionList);

    List_InsertAfter(&theFunctionList,
                      (void*)system_isempty, "isempty");
    List_InsertAfter(&theFunctionList,
                      (void*)system_NULL, "NULL");
    List_InsertAfter(&theFunctionList,
                      (void*)system_rand, "rand");
    List_InsertAfter(&theFunctionList,
                      (void*)system_maxglobalvarindex, "maxglobalvarindex");
    List_InsertAfter(&theFunctionList,
                      (void*)system_getglobalvar, "getglobalvar");
    List_InsertAfter(&theFunctionList,
                      (void*)system_setglobalvar, "setglobalvar");
    List_InsertAfter(&theFunctionList,
                      (void*)system_getlocalvar, "getlocalvar");
    List_InsertAfter(&theFunctionList,
                      (void*)system_setlocalvar, "setlocalvar");
    List_InsertAfter(&theFunctionList,
                      (void*)system_clearglobalvar, "clearglobalvar");
	List_InsertAfter(&theFunctionList,
                      (void*)system_clearindexedvar, "clearindexedvar");
    List_InsertAfter(&theFunctionList,
                      (void*)system_clearlocalvar, "clearlocalvar");
    List_InsertAfter(&theFunctionList,
                      (void*)system_free, "free");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_systemvariant, "openborvariant");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changesystemvariant, "changeopenborvariant");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawstring, "drawstring");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawstringtoscreen, "drawstringtoscreen");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_log, "log");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawbox, "drawbox");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawboxtoscreen, "drawboxtoscreen");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawline, "drawline");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawlinetoscreen, "drawlinetoscreen");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawsprite, "drawsprite");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawspritetoscreen, "drawspritetoscreen");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawdot, "drawdot");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawdottoscreen, "drawdottoscreen");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_drawscreen, "drawscreen");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changeplayerproperty, "changeplayerproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changeentityproperty, "changeentityproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getplayerproperty, "getplayerproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getentityproperty, "getentityproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_tossentity, "tossentity");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_clearspawnentry, "clearspawnentry");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_setspawnentry, "setspawnentry");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_spawn, "spawn");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_projectile, "projectile");
	List_InsertAfter(&theFunctionList,
                      (void*)openbor_transconst, "openborconstant");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_playmusic, "playmusic");
	List_InsertAfter(&theFunctionList,
                      (void*)openbor_fademusic, "fademusic");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_setmusicvolume, "setmusicvolume");
	List_InsertAfter(&theFunctionList,
                      (void*)openbor_setmusictempo, "setmusictempo");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_pausemusic, "pausemusic");
	List_InsertAfter(&theFunctionList,
                      (void*)openbor_playsample, "playsample");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_loadsample, "loadsample");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_unloadsample, "unloadsample");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_fadeout, "fadeout");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_playerkeys, "playerkeys");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changepalette, "changepalette");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_damageentity, "damageentity");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_killentity, "killentity");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_findtarget, "findtarget");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_checkrange, "checkrange");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_gettextobjproperty, "gettextobjproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changetextobjproperty, "changetextobjproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_settextobj, "settextobj");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_cleartextobj, "cleartextobj");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getbglayerproperty, "getbglayerproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changebglayerproperty, "changebglayerproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getfglayerproperty, "getfglayerproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changefglayerproperty, "changefglayerproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getlevelproperty, "getlevelproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changelevelproperty, "changelevelproperty");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_checkhole, "checkhole");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_checkwall, "checkwall");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_checkplatformbelow, "checkplatformbelow");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_openfilestream, "openfilestream");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getfilestreamline, "getfilestreamline");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getfilestreamargument, "getfilestreamargument");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_filestreamnextline, "filestreamnextline");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getfilestreamposition, "getfilestreamposition");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_setfilestreamposition, "setfilestreamposition");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getindexedvar, "getindexedvar");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_setindexedvar, "setindexedvar");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getscriptvar, "getscriptvar");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_setscriptvar, "setscriptvar");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getentityvar, "getentityvar");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_setentityvar, "setentityvar");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_jumptobranch, "jumptobranch");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changelight, "changelight");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_changeshadowcolor, "changeshadowcolor");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_bindentity, "bindentity");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_allocscreen, "allocscreen");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_setdrawmethod, "setdrawmethod");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_updateframe, "updateframe");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_performattack, "performattack");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_setidle, "setidle");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_getentity, "getentity");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_loadmodel, "loadmodel");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_loadsprite, "loadsprite");
    List_InsertAfter(&theFunctionList,
                      (void*)openbor_playgif, "playgif");
    //printf("Done!\n");

}

//////////////////////////////////////////////////////////
////////////   system functions                           
//////////////////////////////////////////////////////////
//isempty(var);
HRESULT system_isempty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    *pretvar = NULL;
    if(paramCount != 1) return E_FAIL;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)((varlist[0])->vt == VT_EMPTY );
    
    return S_OK;
}
//NULL();
HRESULT system_NULL(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant_Clear(*pretvar);

    return S_OK;
}
HRESULT system_rand(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)rand32();
    return S_OK;
}
//getglobalvar(varname);
HRESULT system_getglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant * ptmpvar;
    if(paramCount != 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }
    if(varlist[0]->vt != VT_STR)
    {
        printf("Function getglobalvar must have a string parameter.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    ptmpvar = Script_Get_Global_Variant(StrCache_Get(varlist[0]->strVal));
    if(ptmpvar) ScriptVariant_Copy(*pretvar, ptmpvar);
    else ScriptVariant_ChangeType(*pretvar, VT_EMPTY);
    return S_OK;
}
//maxglobalvarindex();
HRESULT system_maxglobalvarindex(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)max_global_var_index;
    return S_OK;
}
//setglobalvar(varname, value);
HRESULT system_setglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    if(paramCount != 2) {
        *pretvar = NULL;
        return E_FAIL;
    }
    if(varlist[0]->vt != VT_STR)
    {
        printf("Function setglobalvar's first parameter must be a string value.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    (*pretvar)->lVal = (LONG)Script_Set_Global_Variant(StrCache_Get(varlist[0]->strVal), (varlist[1]));

    return S_OK;
}
//getlocalvar(varname);
HRESULT system_getlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant *ptmpvar;

    if(paramCount != 1) {
        *pretvar = NULL;
        return E_FAIL;
    }
    if(varlist[0]->vt != VT_STR)
    {
        printf("Function getlocalvar must have a string parameter.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    ptmpvar = Script_Get_Local_Variant(StrCache_Get(varlist[0]->strVal));
    if(ptmpvar) ScriptVariant_Copy(*pretvar,  ptmpvar);
    else        ScriptVariant_ChangeType(*pretvar, VT_EMPTY);
    return S_OK;
}
//setlocalvar(varname, value);
HRESULT system_setlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    if(paramCount < 2) {
        *pretvar = NULL;
        return E_FAIL;
    }
    if(varlist[0]->vt != VT_STR)
    {
        printf("Function setlocalvar's first parameter must be a string value.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    (*pretvar)->lVal = (LONG)Script_Set_Local_Variant(StrCache_Get(varlist[0]->strVal), varlist[1]);

    return S_OK;;
}
//clearlocalvar();
HRESULT system_clearlocalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    *pretvar = NULL;
    Script_Local_Clear();
    return S_OK;
}
//clearglobalvar();
HRESULT system_clearglobalvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	*pretvar = NULL;
    max_global_var_index = -1;
    return S_OK;
}

//clearindexedvar();
HRESULT system_clearindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    int i;
	*pretvar = NULL;
    for(i=0; i<max_indexed_vars; i++) ScriptVariant_Clear(indexed_var_list+i);
    return S_OK;
}

//free();
HRESULT system_free(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    *pretvar = NULL;
    if(paramCount<1) return E_FAIL;
    if(List_Includes(&scriptheap, varlist[0]->ptrVal))
    {
        tracefree(List_Retrieve(&scriptheap));
        List_Remove(&scriptheap);
        return S_OK;
    }
    return E_FAIL;
}
//////////////////////////////////////////////////////////
////////////   openbor functions                           
//////////////////////////////////////////////////////////
//sample function, used for getting a system variant
//openborvariant(varname);
HRESULT openbor_systemvariant(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    //used for getting the name from varlist
    char* variantname = NULL;
    //the paramCount used for checking.
    //check it first so the engine wont crash if the list is empty
    if(paramCount != 1)  goto systemvariant_error;
    //the variant name should be here
    //you can check the argument type if you like
    if(varlist[0]->vt == VT_STR)
        variantname = StrCache_Get(varlist[0]->strVal);
    else  goto systemvariant_error;
    ///////these should be your get method, ///////
    ScriptVariant_Clear(*pretvar);
    if(getsyspropertybyname(*pretvar, variantname))
    {
        return S_OK;
    }
    //else if
    //////////////////////////////////////////////
systemvariant_error:
    *pretvar = NULL;
    // we have finshed, so return
    return E_FAIL;
}

//used for changing a system variant
//changeopenborvariant(varname, value);
HRESULT openbor_changesystemvariant(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    //used for getting the name from varlist
    char* variantname = NULL;
    //reference to the arguments
    ScriptVariant* arg = NULL;
    //the paramCount used for checking.
    //check it first so the engine wont crash if the list is empty
    if(paramCount != 2)   goto changesystemvariant_error;
    //get the 1st argument
    arg = varlist[0];
    //the variant name should be here
    //you can check the argument type if you like
    if(arg->vt == VT_STR)
        variantname = StrCache_Get(arg->strVal);
    else goto changesystemvariant_error;

    if(changesyspropertybyname(variantname, varlist[1]))
    {
        return S_OK;
    }
changesystemvariant_error:
    *pretvar = NULL;
    // we have finshed, so return
    return E_FAIL;

}

// use font_printf to draw string
//drawstring(x, y, font, string, z);
HRESULT openbor_drawstring(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    char buf[256];
    LONG value[4];
    *pretvar = NULL;

    if(paramCount < 4) goto drawstring_error;

	for(i=0; i<3; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
            goto drawstring_error;
	}
	if(paramCount>4)
	{
		if(FAILED(ScriptVariant_IntegerValue(varlist[4], value+3)))
			goto drawstring_error;
	}
	else value[3] = 0;
    ScriptVariant_ToString(varlist[3], buf);
    font_printf((int)value[0], (int)value[1], (int)value[2], (int)value[3], "%s", buf);
    return S_OK;

drawstring_error:
	printf("First 3 values must be integer values and 4th value a string: drawstring(int x, int y, int font, value)\n");
	return E_FAIL;
}

//use screen_printf
//drawstringtoscreen(screen, x, y, font, string);
HRESULT openbor_drawstringtoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    int i;
    s_screen* scr;
    char buf[256];
    LONG value[3];
    *pretvar = NULL;

    if(paramCount != 5) goto drawstring_error;

	if(varlist[0]->vt!=VT_PTR) goto drawstring_error;
	scr = (s_screen*)varlist[0]->ptrVal;
	if(!scr)  goto drawstring_error;

	for(i=0; i<3; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i+1], value+i)))
            goto drawstring_error;
	}

    ScriptVariant_ToString(varlist[4], buf);
    screen_printf(scr, (int)value[0], (int)value[1], (int)value[2], "%s", buf);
    return S_OK;

drawstring_error:
	printf("Function needs a valid screen handle, 3 integers and a string value: drawstringtoscreen(screen, int font, value)\n");
	return E_FAIL;
}

// debug purpose
//log(string);
HRESULT openbor_log(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    char buf[256];
    *pretvar = NULL;

    if(paramCount != 1) goto drawstring_error;

    ScriptVariant_ToString(varlist[0], buf);
    printf("%s", buf);
    return S_OK;

drawstring_error:
	printf("Function needs 1 parameter: log(value)\n");
	return E_FAIL;
}

//drawbox(x, y, width, height, z, color, lut);
HRESULT openbor_drawbox(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    LONG value[6], l;
    *pretvar = NULL;
 
    if(paramCount < 6) goto drawbox_error;

	for(i=0; i<6; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
            goto drawbox_error;
	}

    if(paramCount > 6)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
			goto drawbox_error;
    }
    else l = -1;

    if(l >= 0)
    {
        l %= MAX_BLENDINGS+1;
    }
    spriteq_add_box((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], (int)value[5], l);

    return S_OK;

drawbox_error:
	printf("Function requires 6 integer values: drawbox(int x, int y, int width, int height, int z, int color, int lut)\n");
	return E_FAIL;
}

//drawboxtoscreen(screen, x, y, width, height, color, lut);
HRESULT openbor_drawboxtoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	s_screen* s;
    LONG value[5], l;
    *pretvar = NULL;
 
    if(paramCount < 6) goto drawbox_error;
    
    s = (s_screen*)varlist[0]->ptrVal;
    
    if(!s) goto drawbox_error;

	for(i=1; i<6; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
            goto drawbox_error;
	}

    if(paramCount > 6)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
			goto drawbox_error;
    }
    else l = -1;

    if(l >= 0)
    {
        l %= MAX_BLENDINGS+1;
    }
    switch(s->pixelformat)
    {
    case PIXEL_8:
        drawbox((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
        break;
    case PIXEL_16:
        drawbox16((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
        break;
    case PIXEL_32:
        drawbox32((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
        break;
    }

    return S_OK;

drawbox_error:
	printf("Function requires a screen handle and 5 integer values, 7th integer value is optional: drawboxtoscreen(screen, int x, int y, int width, int height, int color, int lut)\n");
	return E_FAIL;
}

//drawline(x1, y1, x2, y2, z, color, lut);
HRESULT openbor_drawline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    LONG value[6], l;
    *pretvar = NULL;

    if(paramCount < 6) goto drawline_error;

	for(i=0; i<6; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto drawline_error;
	}

    if(paramCount > 6)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
			goto drawline_error;
    }
    else l = -1;
    
    if(l >=0 )
    {
        l %= MAX_BLENDINGS+1;
    }
    spriteq_add_line((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], (int)value[5], l);
    
    return S_OK;

drawline_error:
	printf("Function requires 6 integer values, 7th integer value is optional: drawline(int x1, int y1, int x2, int y2, int z, int color, int lut)\n");
	return E_FAIL;
}

//drawlinetoscreen(screen, x1, y1, x2, y2, color, lut);
HRESULT openbor_drawlinetoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    LONG value[5], l;
    s_screen *s;
    *pretvar = NULL;

    if(paramCount < 6) goto drawline_error;
    
    s = (s_screen*)varlist[0]->ptrVal;
    
    if(!s) goto drawline_error;

	for(i=1; i<6; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawline_error;
	}

    if(paramCount > 6)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[6], &l)))
			goto drawline_error;
    }
    else l = -1;
    
    if(l >=0 )
    {
        l %= MAX_BLENDINGS+1;
    }
    switch(s->pixelformat)
    {
    case PIXEL_8:
        line((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
        break;
    case PIXEL_16:
        line16((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
        break;
    case PIXEL_32:
        line32((int)value[0], (int)value[1], (int)value[2], (int)value[3], (int)value[4], s, l);
        break;
    }

    return S_OK;
drawline_error:
	printf("Function requires a screen handle and 5 integer values, 7th integer value is optional: drawlinetoscreen(screen, int x1, int y1, int x2, int y2, int color, int lut)\n");
	return E_FAIL;
}

//drawsprite(sprite, x, y, z, sortid);
HRESULT openbor_drawsprite(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    LONG value[4];
    s_sprite* spr;
    *pretvar = NULL;

    if(paramCount < 4) goto drawsprite_error;    
    if(varlist[0]->vt!=VT_PTR) goto drawsprite_error;

    spr = varlist[0]->ptrVal;
    if(!spr) goto drawsprite_error;

    value[3] = (LONG)0;
	for(i=1; i<paramCount && i<5; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawsprite_error;
	}

    spriteq_add_frame((int)value[0], (int)value[1], (int)value[2], spr, &drawmethod, (int)value[3]);

    return S_OK;

drawsprite_error:
	printf("Function requires a valid sprite handle 3 integer values, 5th integer value is optional: drawsprite(sprite, int x, int y, int z, int sortid)\n");
	return E_FAIL;
}

//drawspritetoscreen(sprite, screen, x, y);
HRESULT openbor_drawspritetoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    LONG value[2];
    s_sprite* spr;
    s_screen* scr;
    *pretvar = NULL;

    if(paramCount < 4) goto drawsprite_error;    
    if(varlist[0]->vt!=VT_PTR) goto drawsprite_error;
    spr = varlist[0]->ptrVal;
    if(!spr) goto drawsprite_error;

    if(varlist[1]->vt!=VT_PTR) goto drawsprite_error;
    scr = varlist[1]->ptrVal;
    if(!scr) goto drawsprite_error;

	for(i=2; i<paramCount && i<4; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-2)))
			goto drawsprite_error;
	}

    putsprite((int)value[0], (int)value[1], spr, scr, &drawmethod);

    return S_OK;

drawsprite_error:
	printf("Function requires a valid sprite handle, a valid screen handle and 2 integer values: drawspritetoscreen(sprite, screen, int x, int y)\n");
	return E_FAIL;
}

//drawdot(x, y, z, color, lut);
HRESULT openbor_drawdot(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    LONG value[4], l;
    *pretvar = NULL;

    if(paramCount < 4) goto drawdot_error;

	for(i=0; i<4; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto drawdot_error;
	}

    if(paramCount > 4)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
			goto drawdot_error;
    }
    else l = -1;

    if(l >=0 )
    {
        l %= MAX_BLENDINGS+1;
    }
    spriteq_add_dot((int)value[0], (int)value[1], (int)value[2], (int)value[3], l);

    return S_OK;

drawdot_error:
	printf("Function requires 4 integer values, 5th integer value is optional: drawdot(int x, int y, int z, int color, int lut)\n");
	return E_FAIL;
}

//drawdottoscreen(screen, x, y, color, lut);
HRESULT openbor_drawdottoscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    LONG value[3], l;
    s_screen* s;
	*pretvar = NULL;

    if(paramCount < 4) goto drawdot_error;

    s = (s_screen*)varlist[0]->ptrVal;

    if(!s) goto drawdot_error;

	for(i=1; i<4; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawdot_error;
	}

    if(paramCount > 4)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
			goto drawdot_error;
    }
    else l = -1;

    if(l >=0 )
    {
        l %= MAX_BLENDINGS+1;
    }
    switch(s->pixelformat)
    {
    case PIXEL_8:
        putpixel((int)value[0], (int)value[1], (int)value[2], s, l);
        break;
    case PIXEL_16:
        putpixel16((int)value[0], (int)value[1], (int)value[2], s, l);
        break;
    case PIXEL_32:
        putpixel32((int)value[0], (int)value[1], (int)value[2], s, l);
        break;
    }

    return S_OK;

drawdot_error:
	printf("Function requires a screen handle and 3 integer values, 5th integer value is optional: dottoscreen(screen, int x, int y, int color, int lut)\n");
	return E_FAIL;
}


//drawscreen(screen, x, y, z, lut);
HRESULT openbor_drawscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
    LONG value[3], l;
    s_screen* s;
	extern s_drawmethod plainmethod;
	s_drawmethod screenmethod;
	*pretvar = NULL;

    if(paramCount < 4) goto drawscreen_error;

    s = (s_screen*)varlist[0]->ptrVal;
    
    if(!s) goto drawscreen_error;

	for(i=1; i<4; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1)))
			goto drawscreen_error;
	}

    if(paramCount > 4)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[4], &l)))
			goto drawscreen_error;
    }
    else l = -1;
    
    if(l >=0 )
    {
        l %= MAX_BLENDINGS+1;
    }
    if(paramCount<=4) screenmethod = drawmethod;
    else
    {
        screenmethod = plainmethod;
        screenmethod.alpha = l;
        screenmethod.transbg = 1;
    }
    spriteq_add_screen((int)value[0], (int)value[1], (int)value[2], s, &screenmethod, 0);

    return S_OK;

drawscreen_error:
	printf("Function requires a screen handle and 3 integer values, 5th integer value is optional: drawscreen(screen, int x, int y, int z, int lut)\n");
	return E_FAIL;
}

//getindexedvar(int index);
HRESULT openbor_getindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;

    if(paramCount < 1 || max_indexed_vars<=0)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_Clear(*pretvar);

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function requires 1 numberic value: getindexedvar(int index)\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    if(ind<0 || ind>=max_indexed_vars) return S_OK;

    ScriptVariant_Copy(*pretvar, indexed_var_list+ind);

    return S_OK;
}

//setindexedvar(int index, var);
HRESULT openbor_setindexedvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    
    if(paramCount < 2 || max_indexed_vars<=0)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numberic value: setindexedvar(int index, var)\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind>=max_indexed_vars)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }

    ScriptVariant_Copy(indexed_var_list+ind, varlist[1]);
    (*pretvar)->lVal = 1;

    return S_OK;
}

//getscriptvar(int index);
HRESULT openbor_getscriptvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    
    if(paramCount < 1 || max_script_vars<=0 || !pcurrentscript || !pcurrentscript->vars)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function requires 1 numberic value: getscriptvar(int index)\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_Clear(*pretvar);

    if(ind<0 || ind>=max_script_vars) return S_OK;
    
    ScriptVariant_Copy(*pretvar, pcurrentscript->vars+ind);
    
    return S_OK;
}

//setscriptvar(int index, var);
HRESULT openbor_setscriptvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    
    if(paramCount < 2 || max_script_vars<=0 || !pcurrentscript || !pcurrentscript->vars)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numberic value: setscriptvar(int index, var)\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind>=max_script_vars)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }

    ScriptVariant_Copy(pcurrentscript->vars+ind, varlist[1]);
    (*pretvar)->lVal = 1;
    
    return S_OK;
}

//getentityvar(entity, int index);
HRESULT openbor_getentityvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    LONG ind;
    entity* ent;
    
    if(paramCount < 2 || max_entity_vars<=0 )
    {
        *pretvar = NULL;
        return E_FAIL; 
    }


    arg = varlist[0];

    ScriptVariant_Clear(*pretvar);
    if(arg->vt == VT_EMPTY) ent= NULL;
    else if(arg->vt == VT_PTR) ent = (entity*)arg->ptrVal;
    else
    {
        printf("Function's 1st argument must be a valid entity handle value or empty value: getentityvar(entity, int index)\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    if(!ent || !ent->entvars) return S_OK;

    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &ind)))
    {
        printf("Function's 2nd argument must be a numberic value: getentityvar(entity, int index)\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    if(ind<0 || ind>=max_entity_vars) return S_OK;
    
    ScriptVariant_Copy(*pretvar, ent->entvars+ind);
    
    return S_OK;
}

//setentityvar(int index, var);
HRESULT openbor_setentityvar(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    LONG ind;
    entity* ent;

    if(paramCount < 3 || max_entity_vars<=0)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    arg = varlist[0];

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = 0;

    if(arg->vt == VT_EMPTY) ent = NULL;
    else if(arg->vt == VT_PTR)
        ent = (entity*)arg->ptrVal;
    else
    {
        printf("Function's 1st argument must be a valid entity handle value or empty value: setentityvar(entity, int index, var)\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    if(!ent || !ent->entvars) return S_OK;

    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &ind)))
    {
        printf("Function's 2nd argument must be a numberic value: setentityvar(entity, int index, var)\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    if(ind<0 || ind>=max_entity_vars) return S_OK;

    ScriptVariant_Copy(ent->entvars+ind, varlist[2]);
    (*pretvar)->lVal = 1;
    
    return S_OK;
}

//getentityproperty(pentity, propname);
HRESULT openbor_getentityproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent = NULL;
    char* propname = NULL;
    char* tempstr = NULL;
    ScriptVariant* arg = NULL;
    s_sprite* spr;
    LONG ltemp;
    int i = 0;
    int propind ;
    //int tempint = 0;
    
	// arranged list, for searching
    static const char* proplist[] = {
        "a",
        "aggression",
        "aiattack",
        "aiflag",
        "aimove",
        "alpha",
        "animal",
        "animating",
        "animation",
        "animationid",
        "animheight",
		"animhits",
        "animnum",
        "animpos",
        "animvalid",
        "antigravity",
        "attacking",
        "autokill",
        "base",
        "blink",
        "blockpain",
        "bounce",
		"chargerate",
        "colourmap",
        "damage_on_landing",
        "defaultmodel",
        "defaultname",
        "defense",        
        "detect",        
        "direction",
        "dot",
		"dropframe",
        "edelay",
        "energycost",
        "exists",
        "falldie",
        "gfxshadow",
        "grabbing",
		"guardpoints",
        "health",
        "height",
        "hmapl",
        "hmapu",
        "invincible",
        "invinctime",
		"jugglepoints",
        "knockdowncount",
        "komap_apply",
        "komap_map",
		"landframe",
        "lifespancountdown",
        "link",
        "map",
        "mapcount",
		"maxguardpoints",
        "maxhealth",
		"maxjugglepoints",
        "maxmp",
        "model",
        "mp",
		"mpdroprate",
		"mprate",
		"mpstable",
		"mpstableval",
        "name",
		"nextanim",
		"nextthink",
        "no_adjust_base",
        "noaicontrol",
        "nodieblink",
        "nodrop",
        "nograb",
        "nopain",
        "offense",
        "opponent",
        "owner",
        "parent",
        "playerindex",
        "projectile",
        "running",
        "rush_count",
		"rush_tally",
		"rush_time",
        "scroll",
		"seal",
		"sealtime",
        "setlayer",        
        "speed",
        "sprite",
        "stalltime",
        "staydown",
        "staydownatk",
        "stealth",
        "subentity",
        "subject_to_gravity",
        "subject_to_hole",
        "subject_to_maxz",
        "subject_to_minz",
        "subject_to_obstacle",
        "subject_to_platform",
        "subject_to_screen",
        "subject_to_wall",
        "subtype",
		"tosstime",
        "tossv",
        "type",
        "x",
        "xdir",
        "z",
        "zdir",
                
    };
    // index enum 
    typedef enum {
        _ep_a,
        _ep_aggression,
        _ep_aiattack,
        _ep_aiflag,
        _ep_aimove,
        _ep_alpha,
        _ep_animal,
        _ep_animating,
        _ep_animation,
        _ep_animationid,
        _ep_animheight,
		_ep_animhits,
        _ep_animnum,
        _ep_animpos,
        _ep_animvalid,
        _ep_antigravity,
        _ep_attacking,        
        _ep_autokill,
        _ep_base,
        _ep_blink,
        _ep_blockpain,
        _ep_bounce,
		_ep_chargerate,
        _ep_colourmap,
        _ep_damage_on_landing,
        _ep_defaultmodel,
        _ep_defaultname,
        _ep_defense,        
        _ep_detect,
        _ep_direction,
        _ep_dot,
		_ep_dropframe,
        _ep_edelay,
        _ep_energycost,
        _ep_exists,
        _ep_falldie,
        _ep_gfxshadow,
        _ep_grabbing,
		_ep_guardpoints,
        _ep_health,
        _ep_height,
        _ep_hmapl,
        _ep_hmapu,
        _ep_invincible,
        _ep_invinctime,
		_ep_jugglepoints,
        _ep_knockdowncount,
        _ep_komap_apply,
        _ep_komap_map,
		_ep_landframe,
        _ep_lifespancountdown,
        _ep_link,
        _ep_map,
        _ep_mapcount,
		_ep_maxguardpoints,
        _ep_maxhealth,
		_ep_maxjugglepoints,
        _ep_maxmp,
        _ep_model,
        _ep_mp,
		_ep_mpdroprate,
		_ep_mprate,
		_ep_mpstable,
		_ep_mpstableval,
        _ep_name,
		_ep_nextanim,
		_ep_nextthink,
        _ep_no_adjust_base,
        _ep_noaicontrol,
        _ep_nodieblink,
        _ep_nodrop,
        _ep_nograb,
        _ep_nopain,
        _ep_offense,
        _ep_opponent,
        _ep_owner,
        _ep_parent,
        _ep_playerindex,
        _ep_projectile,        
        _ep_running,
        _ep_rush_count,
		_ep_rush_tally,
		_ep_rush_time,
        _ep_scroll,
		_ep_seal,
		_ep_sealtime,
		_ep_setlayer,        
        _ep_speed,
        _ep_sprite,
        _ep_stalltime,
        _ep_staydown,
        _ep_staydownatk,
        _ep_stealth,
        _ep_subentity,
        _ep_subject_to_gravity,
        _ep_subject_to_hole,
        _ep_subject_to_maxz,
        _ep_subject_to_minz,
        _ep_subject_to_obstacle,
        _ep_subject_to_platform,
        _ep_subject_to_screen,
        _ep_subject_to_wall,
        _ep_subtype,
		_ep_tosstime,
        _ep_tossv,
        _ep_type,
        _ep_x,
        _ep_xdir,
        _ep_z,
        _ep_zdir,
        _ep_the_end,        
        
    } prop_enum;

    if(paramCount < 2)  {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_Clear(*pretvar);

    arg = varlist[0];
    if(arg->vt != VT_PTR && arg->vt != VT_EMPTY)
    {
        printf("Function getentityproperty must have a valid entity handle.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    ent = (entity*)arg->ptrVal; //retrieve the entity
    if(!ent) return S_OK;

    arg = varlist[1];
    if(arg->vt!=VT_STR)
    {
        printf("Function getentityproperty must have a string property name.\n");
    }
    propname = (char*)StrCache_Get(arg->strVal);//see what property it is
    
    propind = searchList(proplist, propname, _ep_the_end);

    switch(propind)
    {
    case _ep_model:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        strcpy(StrCache_Get((*pretvar)->strVal), ent->model->name);
        break;
    }
    case _ep_animal:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.animal;
        break;
    }
    case _ep_maxhealth:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.health;
        break;
    }
    case _ep_health:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->health;
        break;
    }
	case _ep_animhits:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animation->animhits;
        break;
    }
	case _ep_nextanim:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->nextanim;
        break;
    }
	case _ep_nextthink:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->nextthink;
        break;
    }
	case _ep_tosstime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->toss_time;
        break;
    }
    case _ep_mp:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->mp;
        break;
    }  
    case _ep_maxmp:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mp;
        break;
    }
	case _ep_mpstableval:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mpstableval;
        break;
    }
	case _ep_mpstable:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mpstable;
        break;
    }case _ep_mprate:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mprate;
        break;
    }
	case _ep_mpdroprate:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.mpdroprate;
        break;
    }
	case _ep_chargerate:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.chargerate;
        break;
    }
	case _ep_guardpoints:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.guardpoints[0];
        break;
    }
	case _ep_maxguardpoints:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.guardpoints[1];
        break;
    }
	case _ep_jugglepoints:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.jugglepoints[0];
        break;
    }
	case _ep_maxjugglepoints:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.jugglepoints[1];
        break;
    }
    case _ep_alpha:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.alpha;
        break;
    }  
    case _ep_height:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.height;
        break;
    }
    case _ep_setlayer:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.setlayer;
        break;
    }
    case _ep_speed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.speed;
        break;
    }
    case _ep_sprite:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        i = ent->animation->sprite[ent->animpos];
        spr = sprite_map[i].sprite;
        spr->centerx = sprite_map[i].centerx;
        spr->centery = sprite_map[i].centery;
        (*pretvar)->ptrVal = (VOID*)(spr);
        break;
    }
    case _ep_playerindex:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->playerindex;
        //printf("%d %s %d\n", ent->sortid, ent->name, ent->playerindex);
        break;
    }
    case _ep_colourmap:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID*)(ent->colourmap);
        break;
    }
    case _ep_map:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)0;
        for(i=0;i<ent->modeldata.maps_loaded;i++)
        {
            if(ent->colourmap == ent->modeldata.colourmap[i])
            {
                (*pretvar)->lVal = (LONG)(i+1);
                break;
            }
        }
        break;
    }
    case _ep_mapcount:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);       
        (*pretvar)->lVal = (LONG)(ent->modeldata.maps_loaded+1);
         break;
    }
    case _ep_hmapl:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);       
        (*pretvar)->lVal = (LONG)ent->modeldata.hmap1;
         break;
    }
    case _ep_hmapu:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);       
        (*pretvar)->lVal = (LONG)ent->modeldata.hmap2;
         break;
    }
    case _ep_name:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        strcpy(StrCache_Get((*pretvar)->strVal), ent->name);
        break;
    }
    case _ep_defaultname:
    case _ep_defaultmodel:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        strcpy(StrCache_Get((*pretvar)->strVal), ent->defaultmodel->name);
        break;
    }
    case _ep_x:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->x;
        break;
    }
    case _ep_z:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->z;
        break;
    }
    case _ep_a:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->a;
        break;
    }
    case _ep_xdir:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->xdir;
        break;
    }
    case _ep_zdir:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->zdir;
        break;
    }
    case _ep_tossv:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->tossv;
        break;
    }
    case _ep_base:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->base;
        break;
    }
    case _ep_direction:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->direction;
        break;
    }
    case _ep_exists:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->exists;
        break;
    }
    case _ep_edelay:
    {              
        arg = varlist[2];
        if(arg->vt != VT_STR)
        {
            printf("You must give a string name for edelay property.\n");
            return E_FAIL;
        }
        tempstr = (char*)StrCache_Get(arg->strVal);
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        
        if(stricmp(tempstr, "mode")==0)
        {            
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.mode;
        }
        else if(stricmp(tempstr, "factor")==0)
        {
            (*pretvar)->dblVal = (float)ent->modeldata.edelay.factor;
        }
        else if(stricmp(tempstr, "cap_min")==0)
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.cap_min;
        }
        else if(stricmp(tempstr, "cap_max")==0)
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.cap_max;
        }
        else if(stricmp(tempstr, "range_min")==0)
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.range_min;
        }
        else if(stricmp(tempstr, "range_max")==0)
        {
            (*pretvar)->lVal = (LONG)ent->modeldata.edelay.range_max;
        }
        break;
    }
    case _ep_energycost:
    {
        ltemp = 0;
        if(paramCount == 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
            ltemp = (LONG)0;
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.animation[ltemp]->energycost;
        break;
    }
	case _ep_dropframe:
    {
        ltemp = 0;
        if(paramCount == 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
            ltemp = (LONG)0;
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.animation[ltemp]->dropframe;
        break;
    }
	case _ep_landframe:
    {
        ltemp = 0;
        if(paramCount == 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
            ltemp = (LONG)0;
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)ent->modeldata.animation[ltemp]->landframe[0];
        break;
    }
    case _ep_type:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.type;
        break;
    }
    case _ep_subtype:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subtype;
        break;
    }
    case _ep_aiflag:
    {
        if(paramCount<3) break;
        arg = varlist[2];
        if(arg->vt != VT_STR)
        {
            printf("You must give a string name for aiflag.\n");
            return E_FAIL;
        }
        tempstr = (char*)StrCache_Get(arg->strVal);
        if(stricmp(tempstr, "dead")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->dead;
        }
        else if(stricmp(tempstr, "jumping")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->jumping;
        }
        else if(stricmp(tempstr, "idling")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->idling;
        }
        else if(stricmp(tempstr, "drop")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->drop;
        }
        else if(stricmp(tempstr, "attacking")==0)
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->attacking;
        }
        else if(stricmp(tempstr, "getting")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->getting;
        }
        else if(stricmp(tempstr, "turning")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->turning;
        }
        else if(stricmp(tempstr, "charging")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->charging;
        }
        else if(stricmp(tempstr, "blocking")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->blocking;
        }
        else if(stricmp(tempstr, "falling")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->falling;
        }
        else if(stricmp(tempstr, "running")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->running;
        }
        else if(stricmp(tempstr, "inpain")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->inpain;
        }
        else if(stricmp(tempstr, "projectile")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->projectile;
        }
        else if(stricmp(tempstr, "frozen")==0)
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->frozen;
        }
        else if(stricmp(tempstr, "freezetime")==0)
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->freezetime;
        }
        else if(stricmp(tempstr, "toexplode")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->toexplode;
        }
        else if(stricmp(tempstr, "animating")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->animating;
        }
        else if(stricmp(tempstr, "blink")==0)
        {
            ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->blink;
        }
        else if(stricmp(tempstr, "invincible")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->invincible;
        }
        else if(stricmp(tempstr, "autokill")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->autokill;
        }
        break;
    }
    case _ep_animation:
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID*)ent->animation;
        break;
    }
    case _ep_animnum:
    case _ep_animationid:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animnum;
        break;
    }
    case _ep_animpos:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animpos;
        break;
    }
    case _ep_animvalid:
    {
        ltemp = 0;
        if(paramCount == 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
            ltemp = (LONG)0;
        }
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)validanim(ent, ltemp);
        break;        
    }
    case _ep_animheight:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animation->height;
        break;
    }
    case _ep_animating:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->animating;
        break;
    }
    case _ep_invincible:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->invincible;
        break;
    }
    case _ep_invinctime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->invinctime;
        break;
    }
	case _ep_rush_count:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->rush[0];
        break;
    }
	case _ep_rush_tally:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->rush[1];
        break;
    }
	case _ep_rush_time:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->rushtime;
        break;
    }
    case _ep_knockdowncount:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->knockdowncount;
        break;
    }
    case _ep_komap_map:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.komap[0];
        break;
    }
    case _ep_komap_apply:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.komap[1];
        break;
    }
    case _ep_lifespancountdown:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->lifespancountdown;
        break;
    }
    case _ep_blink:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->blink;   
        break;
    }
    case _ep_subject_to_screen:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_screen;   
        break;
    }
    case _ep_subject_to_minz:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_minz;   
        break;
    }
    case _ep_subject_to_maxz:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_maxz;   
        break;
    }
    case _ep_subject_to_wall:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_wall;   
        break;
    }
    case _ep_subject_to_hole:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_hole;   
        break;
    }
    case _ep_subject_to_gravity:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_gravity;   
        break;
    }
    case _ep_subject_to_obstacle:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_obstacle;   
        break;
    }
    case _ep_subject_to_platform:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.subject_to_platform;   
        break;
    }
    case _ep_defense:
    {
        ltemp = 0;
        if(paramCount == 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
                ltemp = (LONG)0;
        }                     

        arg = varlist[3];
        if(arg->vt != VT_STR)
        {
            printf("You must give a string name for defense property.\n");
            return E_FAIL;
        }
        tempstr = (char*)StrCache_Get(arg->strVal);
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        
        if(stricmp(tempstr, "factor")==0)
        {            
            (*pretvar)->dblVal = (float)ent->modeldata.defense_factors[(int)ltemp];
        }
        else if(stricmp(tempstr, "blockpower")==0)
        {             
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_blockpower[(int)ltemp];
        }
        else if(stricmp(tempstr, "blockratio")==0)
        {             
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_blockratio[(int)ltemp];
        }
        else if(stricmp(tempstr, "blockthreshold")==0)
        {
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_blockthreshold[(int)ltemp];
        }
        else if(stricmp(tempstr, "blocktype")==0)
        {
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_blocktype[(int)ltemp];
        }
        else if(stricmp(tempstr, "knockdown")==0)
        {             
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_knockdown[(int)ltemp];
        }
        else if(stricmp(tempstr, "pain")==0)
        {
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.defense_pain[(int)ltemp];
        }        
        break;
    }    
    case _ep_no_adjust_base:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.no_adjust_base;   
        break;
    }
    case _ep_noaicontrol:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->noaicontrol;   
        break;
    }
    case _ep_nodieblink:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.nodieblink;   
        break;
    }
    case _ep_bounce:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.bounce;   
        break;
    }
    case _ep_falldie:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.falldie;   
        break;
    }
    case _ep_nodrop:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.nodrop;   
        break;
    }
    case _ep_nograb:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->nograb;   
        break;
    }
    case _ep_nopain:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.nopain;   
        break;
    }
    case _ep_offense:
    {
        ltemp = 0;
        if(paramCount >= 3)
        {
            arg = varlist[2];
            if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
                ltemp = (LONG)0;
        }
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.offense_factors[(int)ltemp];
        break;
    }
    case _ep_antigravity:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->antigravity;
        break;
    }
    case _ep_owner:
    {
        if(ent->owner) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID*)ent->owner;
        }
        break;
    }
    case _ep_parent:
    {
        if(ent->parent) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID*)ent->parent;
        }
        break;
    }
    case _ep_subentity:
    {
        if(ent->subentity) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID*)ent->subentity;
        }   
        break;
    }
    case _ep_opponent:
    {
        if(ent->opponent) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID*)ent->opponent;
        }   
        break;
    }
    case _ep_grabbing:
    {
        if(ent->grabbing) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID*)ent->grabbing;
        }
        break;
    }
    case _ep_link:
    {
        if(ent->link) // always return an empty var if it is NULL
        {
            ScriptVariant_ChangeType(*pretvar, VT_PTR);
            (*pretvar)->ptrVal = (VOID*)ent->link;
        }
        break;
    }
    case _ep_aimove:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.aimove;
        break;  
    }
    case _ep_aggression:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.aggression;
        break;
    }
    case _ep_aiattack:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.aiattack;
        break;
    }
    case _ep_attacking:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->attacking;
        break;
    }
    case _ep_autokill:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->autokill;
        break;
    }
    case _ep_scroll:
    {        
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)ent->modeldata.scroll;
        break;    
    }
    case _ep_damage_on_landing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->damage_on_landing;
        break;
    }
    case _ep_detect:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.detect;
        break;
    }
    case _ep_stealth:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.stealth;
        break;
    }
    case _ep_stalltime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->stalltime;
        break;
    }
    case _ep_staydown:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->staydown;
        break;
    }
    case _ep_staydownatk:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->staydownatk;
        break;
    }
    case _ep_gfxshadow:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->modeldata.gfxshadow;
        break;
    }

    case _ep_projectile:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->projectile;
        break;
    }
    case _ep_running:
    {
        if(paramCount<3) break;
        arg = varlist[2];
        if(arg->vt != VT_STR)
        {
            printf("You must give a string name for running property.\n");
            return E_FAIL;
        }
        tempstr = (char*)StrCache_Get(arg->strVal);
        if(stricmp(tempstr, "speed")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.runspeed;
        }
        else if(stricmp(tempstr, "jumpY")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.runjumpheight;
        }
        else if(stricmp(tempstr, "jumpX")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
            (*pretvar)->dblVal = (DOUBLE)ent->modeldata.runjumpdist;
        }
        else if(stricmp(tempstr, "land")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
            (*pretvar)->lVal = (LONG)ent->modeldata.runhold;
        }
        else if(stricmp(tempstr, "moveZ")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->modeldata.runupdown;
        }
        break;
    }    
    case _ep_seal:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->seal;
        break;
    }
    case _ep_sealtime:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)ent->sealtime;
        break;
    }
    case _ep_dot:
    {
        if(paramCount<4) break;
      
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            i = (int)ltemp;
        
        arg = varlist[3];
        if(arg->vt != VT_STR)
        {
            printf("You must give a string name for dot property.\n");
            return E_FAIL;
        }
        tempstr = (char*)StrCache_Get(arg->strVal);
        if(stricmp(tempstr, "time")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->dot_time[i];
        }        
        else if(stricmp(tempstr, "mode")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->dot[i];
        }
        else if(stricmp(tempstr, "force")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->dot_force[i];
        }
        else if(stricmp(tempstr, "rate")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->dot_rate[i];
        }
        else if(stricmp(tempstr, "type")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
             (*pretvar)->lVal = (LONG)ent->dot_atk[i];
        }
        else if(stricmp(tempstr, "owner")==0)
        {
             ScriptVariant_ChangeType(*pretvar, VT_PTR);
             (*pretvar)->ptrVal = (VOID*)ent->dot_owner[i];
        }
        break;        
    }
    case _ep_blockpain:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.blockpain = (int)ltemp;
        break;        
    }
    default:
        printf("Property name '%s' is not supported by function getentityproperty.\n", propname);
        *pretvar = NULL;
        return E_FAIL;
        break;
    }

    return S_OK;
}
//changeentityproperty(pentity, propname, value1[ ,value2, value3, ...]);
HRESULT openbor_changeentityproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent = NULL;
    s_model* tempmodel ;
    char* propname = NULL;
    char* tempstr = NULL;
    LONG ltemp, ltemp2;
    DOUBLE dbltemp;
    int propind;
	int i = 0;

    static const char* proplist[] = {
        "aggression",
        "aiattack",
        "aiflag",
        "aimove",
        "alpha",
        "animation",
		"animhits",
        "animpos",
		"antigrab",
        "antigravity",
        "attacking",        
        "autokill",        
        "base",
        "blink",
        "blockpain",
        "bounce",
		"candamage",
        "combostep",
        "colourmap",
        "damage_on_landing",
        "defaultname",
        "defense",
        "detect",
        "direction",
        "dot",
        "edelay",
        "falldie",
        "freezetime",
        "frozen",
        "gfxshadow",
		"grabforce",
		"guardpoints",
        "health",
        "hmapl",
        "hmapu",
		"hostile",
        "iconposition",
        "invincible",
        "invinctime",
		"jugglepoints",
        "komap",
        "lifeposition",
        "lifespancountdown",
        "map",
        "maptime",
		"maxguardpoints",
        "maxhealth",
		"maxjugglepoints",
        "maxmp",
        "model",
        "mp",
		"mpset",
        "name",
        "nameposition",
		"nextanim",
		"nextthink",
        "no_adjust_base",
        "noaicontrol",
        "nodieblink",
        "nodrop",
        "nograb",
        "nopain",
        "offense",
        "opponent",
        "owner",
        "parent",
        "position",
        "projectile",
        "running",
		"rush_count",
		"rush_tally",
		"rush_time",
        "scroll",
		"seal",
		"sealtime",
		"setlayer",        
		"speed",
        "stalltime",
        "staydown",
        "staydownatk",
		"stealth",
        "subentity",
        "subject_to_gravity",
        "subject_to_hole",
        "subject_to_maxz",
        "subject_to_minz",
        "subject_to_obstacle",
        "subject_to_platform",
        "subject_to_screen",
        "subject_to_wall",
        "takeaction",
		"tosstime",
        "trymove",
        "velocity",
        "weapon",
        
    };
    
    typedef enum
    {
        _ep_aggression,
        _ep_aiattack,
        _ep_aiflag,
        _ep_aimove,
        _ep_alpha,
        _ep_animation,
		_ep_animhits,
        _ep_animpos,
		_ep_antigrab,
        _ep_antigravity,
        _ep_attacking,        
        _ep_autokill,        
        _ep_base,
        _ep_blink,
        _ep_blockpain,
        _ep_bounce,
		_ep_candamage,
        _ep_combostep,
        _ep_colourmap,
        _ep_damage_on_landing,
        _ep_defaultname,
        _ep_defense,
        _ep_detect,
        _ep_direction,
        _ep_dot,
        _ep_edelay,
        _ep_falldie,
        _ep_freezetime,
        _ep_frozen,
        _ep_gfxshadow,
		_ep_grabforce,
		_ep_guardpoints,
        _ep_health,
        _ep_hmapl,
        _ep_hmapu,
		_ep_hostile,
        _ep_iconposition,
        _ep_invincible,
        _ep_invinctime,
		_ep_jugglepoints,
        _ep_komap,
        _ep_lifeposition,
        _ep_lifespancountdown,
        _ep_map,
        _ep_maptime,
		_ep_maxguardpoints,
        _ep_maxhealth,
		_ep_maxjugglepoints,
        _ep_maxmp,
        _ep_model,
        _ep_mp,
		_ep_mpset,
        _ep_name,
        _ep_nameposition,
		_ep_nextanim,
		_ep_nextthink,
        _ep_no_adjust_base,
        _ep_noaicontrol,
        _ep_nodieblink,
        _ep_nodrop,
        _ep_nograb,
        _ep_nopain,
        _ep_offense,
        _ep_opponent,
        _ep_owner,
        _ep_parent,
        _ep_position,
        _ep_projectile,
        _ep_running,
		_ep_rush_count,
		_ep_rush_tally,
		_ep_rush_time,
        _ep_scroll,
		_ep_seal,
		_ep_sealtime,
		_ep_setlayer,
        _ep_speed,
        _ep_stalltime,
        _ep_staydown,
        _ep_staydownatk,
		_ep_stealth,
        _ep_subentity,
        _ep_subject_to_gravity,
        _ep_subject_to_hole,
        _ep_subject_to_maxz,
        _ep_subject_to_minz,
        _ep_subject_to_obstacle,
        _ep_subject_to_platform,
        _ep_subject_to_screen,
        _ep_subject_to_wall,
        _ep_takeaction,
		_ep_tosstime,
        _ep_trymove,
        _ep_velocity,
        _ep_weapon,
        _ep_the_end,        
        
    } prop_enum;

    if(paramCount < 3) goto changeentityproperty_error;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)0;

    if(varlist[0]->vt != VT_PTR && varlist[0]->vt != VT_EMPTY)
    {
        printf("Function changeentityproperty must have a valid entity handle.");
        goto changeentityproperty_error;
    }
    ent = (entity*)varlist[0]->ptrVal; //retrieve the entity
    if(!ent)
    {
        (*pretvar)->lVal = (LONG)0;
        return S_OK;
    }

    if(varlist[1]->vt != VT_STR)
    {
        printf("Function changeentityproperty must have a string property name.\n");
        goto changeentityproperty_error;
    }
    propname = (char*)StrCache_Get(varlist[1]->strVal);//see what property it is

    propind = searchList(proplist, propname, _ep_the_end);

    switch(propind)
    {
    case _ep_takeaction:
    {
        if(varlist[2]->vt != VT_STR)
        {
            printf("You must give a name for aiflag.\n");
            goto changeentityproperty_error;
        }
        tempstr = (char*)StrCache_Get(varlist[2]->strVal);
        if(stricmp(tempstr, "common_spawn")==0)
            ent->takeaction = common_spawn;
        else if(stricmp(tempstr, "common_attack_proc")==0)
            ent->takeaction = common_attack_proc;
        else if(stricmp(tempstr, "common_jump")==0)
            ent->takeaction = common_jump;
        else if(stricmp(tempstr, "common_drop")==0)
            ent->takeaction = common_drop;
        else if(stricmp(tempstr, "common_fall")==0)
            ent->takeaction = common_fall;
        else if(stricmp(tempstr, "common_block")==0)
            ent->takeaction = common_block;
        else if(stricmp(tempstr, "common_pain")==0)
            ent->takeaction = common_pain;
        else if(stricmp(tempstr, "common_turn")==0)
            ent->takeaction = common_turn;
        else if(stricmp(tempstr, "common_land")==0)
            ent->takeaction = common_land;
        else if(stricmp(tempstr, "common_lie")==0)
            ent->takeaction = common_lie;        
        else if(stricmp(tempstr, "player_blink")==0)
            ent->takeaction = player_blink;
        else if(stricmp(tempstr, "suicide")==0)
            ent->takeaction = suicide;
        else if(stricmp(tempstr, "common_rise")==0)
            ent->takeaction = common_rise;
        else if(stricmp(tempstr, "common_grabbed")==0)
            ent->takeaction = common_grabbed;
        else if(stricmp(tempstr, "common_grabattack")==0)
            ent->takeaction = common_grabattack;
        else if(stricmp(tempstr, "common_grab")==0)
            ent->takeaction = common_grab;
        else if(stricmp(tempstr, "common_vault")==0)
            ent->takeaction = common_vault;
        else if(stricmp(tempstr, "normal_prepare")==0)
            ent->takeaction = normal_prepare;
        else if(stricmp(tempstr, "bomb_explode")==0)
            ent->takeaction = bomb_explode;
        else if(stricmp(tempstr, "npc_warp")==0)
            ent->takeaction = npc_warp;
        else if(stricmp(tempstr, "common_prejump")==0)
            ent->takeaction = common_prejump;
        else if(stricmp(tempstr, "common_get")==0)
            ent->takeaction = common_get;        
        else
            ent->takeaction = NULL;
        break;
    }
	case _ep_candamage:
		{
                ent->modeldata.candamage = 0;
                
                for(i=2; i<paramCount; i++)
                {
					tempstr = (char*)StrCache_Get(varlist[i]->strVal);
                    if(stricmp(tempstr, "TYPE_ENEMY")==0)
                        ent->modeldata.candamage |= TYPE_ENEMY;
                     if(stricmp(tempstr, "TYPE_PLAYER")==0)
                        ent->modeldata.candamage |= TYPE_PLAYER;
                     if(stricmp(tempstr, "TYPE_OBSTACLE")==0)
                        ent->modeldata.candamage |= TYPE_OBSTACLE;
                     if(stricmp(tempstr, "TYPE_SHOT")==0)
                        ent->modeldata.candamage |= TYPE_SHOT;
                     if(stricmp(tempstr, "TYPE_NPC")==0)
                        ent->modeldata.candamage |= TYPE_NPC;
                     if(stricmp(tempstr, "ground")==0) // not really needed, though
                        ent->modeldata.ground = 1;
					 }
                    
					break;
            }
		case _ep_hostile:
		{
                ent->modeldata.hostile = 0;
                
                for(i=2; i<paramCount; i++)
                {
					tempstr = (char*)StrCache_Get(varlist[i]->strVal);
                    if(stricmp(tempstr, "TYPE_ENEMY")==0)
                        ent->modeldata.hostile |= TYPE_ENEMY;
                     if(stricmp(tempstr, "TYPE_PLAYER")==0)
                        ent->modeldata.hostile |= TYPE_PLAYER;
                     if(stricmp(tempstr, "TYPE_OBSTACLE")==0)
                        ent->modeldata.hostile |= TYPE_OBSTACLE;
                     if(stricmp(tempstr, "TYPE_SHOT")==0)
                        ent->modeldata.hostile |= TYPE_SHOT;
                     if(stricmp(tempstr, "TYPE_NPC")==0)
                        ent->modeldata.hostile |= TYPE_NPC;
			         }
                    
					break;
            }
    case _ep_model:
    {
        if(varlist[2]->vt != VT_STR)
        {
            printf("You must give a string value for model name.\n");
            goto changeentityproperty_error;
        }
        tempstr = (char*)StrCache_Get(varlist[2]->strVal);
        if(paramCount > 3)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                (*pretvar)->lVal = (LONG)1;
        }
        else
        {
            ltemp = (LONG)0;
            (*pretvar)->lVal = (LONG)1;
        }
        if((*pretvar)->lVal == (LONG)1) set_model_ex(ent, tempstr, -1, NULL, (int)ltemp);
        if(!ent->weapent) ent->weapent = ent;
        break;
    }
    case _ep_weapon:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            if(paramCount > 3)
            {
                if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
                    (*pretvar)->lVal = (LONG)1;
            }
            else
            {
                ltemp2 = (LONG)0;
                (*pretvar)->lVal = (LONG)1;
            }
            set_weapon(ent, (int)ltemp, (int)ltemp2);
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_maxhealth:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.health = (int)ltemp;
            if(ent->modeldata.health < 0) ent->modeldata.health = 0; //OK, no need to have ot below 0
        }
        break;
    }
    case _ep_health:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->health = (int)ltemp;                      
            if(ent->health > ent->modeldata.health) ent->health = ent->modeldata.health;
            else if(ent->health < 0) ent->health = 0;           
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_maxmp:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.mp = (int)ltemp;
            if(ent->modeldata.mp < 0) ent->modeldata.mp = 0; //OK, no need to have ot below 0
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_mp:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->mp = (int)ltemp;
            if(ent->mp > ent->modeldata.mp) ent->mp = ent->modeldata.mp;
            else if(ent->mp < 0) ent->mp = 0;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_setlayer:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.setlayer = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_speed:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.speed = (float)dbltemp;
        }
        break;
    }
	case _ep_animhits:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->animation->animhits = (int)ltemp;
		}
		break;
    }
    case _ep_alpha:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.alpha = (int)ltemp;
        }
        break;
    }
	case _ep_guardpoints:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.guardpoints[0] = (int)ltemp;
        }
        break;
    }
	case _ep_maxguardpoints:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.guardpoints[1] = (int)ltemp;
        }
        break;
    }
	case _ep_jugglepoints:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.jugglepoints[0] = (int)ltemp;
        }
        break;
    }
    case _ep_komap:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.komap[0] = (int)ltemp;
        }
        if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                ent->modeldata.komap[1] = (int)ltemp;
            else (*pretvar)->lVal = (LONG)0;
        }        
        break;
    }
	case _ep_maxjugglepoints:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.jugglepoints[1] = (int)ltemp;
        }
        break;
    }
    case _ep_antigravity:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.antigravity = (float)dbltemp;
        }
        break;
    }   
    case _ep_map:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent_set_colourmap(ent, (int)ltemp);
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_maptime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->maptime = (int)ltemp;
        }
        break;
    }
    case _ep_colourmap:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            self->colourmap = self->modeldata.colourmap[ltemp-1];
        }
        break;
    }
    case _ep_hmapl:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            (*pretvar)->lVal = (LONG)1;
            self->modeldata.hmap1 = ltemp;
        break;
    }
    case _ep_hmapu:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            (*pretvar)->lVal = (LONG)1;
            self->modeldata.hmap2 = ltemp;
        break;
    }
    case _ep_name:
    {
        if(varlist[2]->vt != VT_STR)
        {
            //printf("You must give a string value for entity name.\n");
            goto changeentityproperty_error;
        }
        strcpy(ent->name, (char*)StrCache_Get(varlist[2]->strVal));
        (*pretvar)->lVal = (LONG)1;
        break;
    }
    case _ep_defaultname:
    {
        if(varlist[2]->vt != VT_STR)
        {
            //printf("You must give a string value for entity name.\n");
            goto changeentityproperty_error;
        }
        tempmodel = find_model((char*)StrCache_Get(varlist[2]->strVal));
        if(!tempmodel)
        {
            printf("Use must give an existing model's name for entity's default model name.\n");
            goto changeentityproperty_error;
        }
        ent->defaultmodel = tempmodel;
        (*pretvar)->lVal = (LONG)1;
        break;
    }
    case _ep_position:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->x = (float)dbltemp;
        }
        if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
                ent->z = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        } 
        if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
                ent->a = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        break;
    }
    case _ep_base:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->base = (float)dbltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_direction:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            ent->direction = (int)dbltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_trymove:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            if(ltemp == 1)
                 ent->trymove = common_trymove;
            else if(ltemp == 2)
                 ent->trymove = player_trymove;
            else
                 ent->trymove = NULL;
        }
        break;
    }
	case _ep_nextanim:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->nextanim = (int)ltemp;
		}
		break;
    }
	case _ep_nextthink:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->nextthink = (int)ltemp;
		}
		break;
    }
	case _ep_tosstime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
		{
			(*pretvar)->lVal = (LONG)1;
			ent->toss_time = (int)ltemp;
		}
		break;
    }
    case _ep_velocity:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->xdir = (float)dbltemp;
        }
        if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1) 
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
                ent->zdir = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        } 
        if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1) 
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
                ent->tossv = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        break;
    }
    case _ep_defense:
    {
        if(paramCount >= 4 &&
           SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) &&
           ltemp < (LONG)MAX_ATKS && ltemp >= (LONG)0)
        {
            (*pretvar)->lVal = (LONG)1;
        }
        if((*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
                ent->modeldata.defense_factors[(int)ltemp] = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
                ent->modeldata.defense_pain[(int)ltemp] = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
                ent->modeldata.defense_knockdown[(int)ltemp] = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
                ent->modeldata.defense_blockpower[(int)ltemp] = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 8 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp)))
                ent->modeldata.defense_blockthreshold[(int)ltemp] = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 9 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[8], &dbltemp)))
                ent->modeldata.defense_blockratio[(int)ltemp] = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 10 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[9], &dbltemp)))
                ent->modeldata.defense_blocktype[(int)ltemp] = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        break;
    }
    case _ep_offense:
    {
        if(paramCount >= 4 &&
           SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)) &&
           ltemp < (LONG)MAX_ATKS && ltemp >= (LONG)0)
        {
            (*pretvar)->lVal = (LONG)1;
        }
        if((*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
                ent->modeldata.offense_factors[(int)ltemp] = (float)dbltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        break;
    }
    case _ep_grabforce:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->model->grabforce = (int)ltemp;
        break;
    }
	case _ep_antigrab:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->model->antigrab = (int)ltemp;
        break;
    }
	case _ep_nodrop:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.nodrop = (int)ltemp;
        break;
    }
    case _ep_nopain:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.nopain = (int)ltemp;
        break;
    }
    case _ep_nograb:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->nograb = (int)ltemp;
        break;
    }
    case _ep_no_adjust_base:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.no_adjust_base = (int)ltemp;
        break;
    }
    case _ep_noaicontrol:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->noaicontrol = (int)ltemp;
        break;
    }
    case _ep_nodieblink:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.nodieblink = (int)ltemp;
        break;
    }
    case _ep_falldie:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.falldie = (int)ltemp;
        break;
    }
    case _ep_freezetime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->freezetime = (int)ltemp;
        break;
    }
    case _ep_frozen:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->frozen = (int)ltemp;
        break;
    }
    case _ep_bounce:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.bounce = (int)ltemp;
        break;
    }
    case _ep_aiflag:
    {
        if(varlist[2]->vt != VT_STR) 
        {
            //printf("");
            goto changeentityproperty_error;
        }
        tempstr = (char*)StrCache_Get(varlist[2]->strVal);
        if(paramCount<4) break;
        if(stricmp(tempstr, "dead")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->dead = (int)ltemp;
        }
        else if(stricmp(tempstr, "jumping")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->jumping = (int)ltemp;
        }
        else if(stricmp(tempstr, "idling")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->idling = (int)ltemp;
        }
        else if(stricmp(tempstr, "drop")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->drop = (int)ltemp;
        }
        else if(stricmp(tempstr, "attacking")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->attacking = (int)ltemp;
        }
        else if(stricmp(tempstr, "getting")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->getting = (int)ltemp;
        }
        else if(stricmp(tempstr, "turning")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->turning = (int)ltemp;
        }
        else if(stricmp(tempstr, "charging")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->charging = (int)ltemp;
        }
        else if(stricmp(tempstr, "blocking")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->blocking = (int)ltemp;
        }
        else if(stricmp(tempstr, "falling")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->falling = (int)ltemp;
        }
        else if(stricmp(tempstr, "running")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->running = (int)ltemp;
        }
        else if(stricmp(tempstr, "inpain")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->inpain = (int)ltemp;
        }
        else if(stricmp(tempstr, "projectile")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->projectile = (int)ltemp;
        }
        else if(stricmp(tempstr, "frozen")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->frozen = (int)ltemp;
        }
        else if(stricmp(tempstr, "toexplode")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->toexplode = (int)ltemp;
        }
        else if(stricmp(tempstr, "animating")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->animating = (int)ltemp;
        }
        else if(stricmp(tempstr, "blink")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->blink = (int)ltemp;
        }
        else if(stricmp(tempstr, "invincible")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->invincible = (int)ltemp;
        }
        else if(stricmp(tempstr, "autokill")==0)
        {
             if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                  ent->autokill = (int)ltemp;
        }
        break;
    }
    case _ep_animation:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            (*pretvar)->lVal = (LONG)1;
        if(paramCount >= 4)
        {
            if(FAILED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
                (*pretvar)->lVal = (LONG)0;
        }
        else ltemp2 = (LONG)1;
        if((*pretvar)->lVal == (LONG)1)
        {
            ent_set_anim(ent, (int)ltemp, (int)ltemp2);
        }
        break;
    }
    case _ep_animpos:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->animpos = (int)ltemp;
        break;
    }
    case _ep_invincible:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->invincible = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_invinctime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->invinctime = (int)ltemp;
        break;
    }
	case _ep_rush_count:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->rush[0] = (int)ltemp;
        }
        break;
    }   
	case _ep_rush_tally:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->rush[1] = (int)ltemp;
        }
        break;
    }
	case _ep_rush_time:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->rushtime = (int)ltemp;
        break;
    }
	case _ep_iconposition:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.iconx = (int)ltemp;
        if(paramCount>3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
            ent->modeldata.icony = (int)ltemp;
        break;
    }
	case _ep_lifeposition:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.hpx = (int)ltemp;
        if(paramCount>3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
            ent->modeldata.hpy = (int)ltemp;
        break;
    }
	case _ep_nameposition:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.namex = (int)ltemp;
        if(paramCount>3 && SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
            ent->modeldata.namey = (int)ltemp;
        break;
    }
    case _ep_lifespancountdown:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
            ent->lifespancountdown = (float)dbltemp;
        break;
    }
    case _ep_blink:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->blink = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_subject_to_screen:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_screen = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_subject_to_maxz:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_maxz = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_subject_to_minz:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_minz = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_subject_to_wall:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_wall = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_subject_to_obstacle:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_obstacle = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_subject_to_platform:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_platform = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_subject_to_gravity:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_gravity = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_subject_to_hole:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->modeldata.subject_to_hole = (int)ltemp;
            (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_owner:
    {
        ent->owner = (entity*)varlist[2]->ptrVal;
        break;
    }
    case _ep_parent:
    {
        ent->parent = (entity*)varlist[2]->ptrVal;
        break;
    }
    case _ep_opponent:
    {
        ent->opponent = (entity*)varlist[2]->ptrVal;
        break;
    }
    case _ep_subentity:
    {
        if(ent->subentity) ent->subentity->parent = NULL;
        ent->subentity = (entity*)varlist[2]->ptrVal;
        if(ent->subentity) ent->subentity->parent = ent;
        break;
    }
    case _ep_aimove:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.aimove = (int)ltemp;
        break;
    }
    case _ep_aggression:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.aggression = (int)ltemp;            
        break;
    }
    case _ep_aiattack:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.aiattack = (int)ltemp;
        break;
    }
    case _ep_combostep:
    {
        if(paramCount >= 4 &&
           SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
        }
        if((*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp2)))
                ent->combostep[(int)ltemp]=(int)ltemp2;
            else (*pretvar)->lVal = (LONG)0;
        }
        break;
    }
    case _ep_attacking:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
           ent->attacking = (int)ltemp;
           (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_scroll:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.scroll = (float)dbltemp;
        }
        break;
    }
    case _ep_autokill:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            ent->autokill = (int)ltemp;
           (*pretvar)->lVal = (LONG)1;
        }
        break;
    }
    case _ep_damage_on_landing:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->damage_on_landing = (int)ltemp;
        break;        
    }
    case _ep_detect:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.detect = (int)ltemp;
        break;
    }
    case _ep_stalltime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->stalltime = (int)ltemp;
        break;        
    }
    case _ep_staydown:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->staydown = (int)ltemp;
        }
        if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                ent->staydownatk = (int)ltemp;
        }         
    }  
    case _ep_stealth:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.stealth = (int)ltemp;
        break;        
    }
    case _ep_gfxshadow:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.gfxshadow = (int)ltemp;
        break;        
    }
    case _ep_projectile:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->projectile = (int)ltemp;
        break;        
    }
    case _ep_running:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.runspeed = (float)dbltemp;
        }
        if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
                ent->modeldata.runjumpheight = (float)dbltemp;
        } 
        if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
                ent->modeldata.runjumpdist = (float)dbltemp;
        }
        if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
                ent->modeldata.runhold = (int)dbltemp;
        }
		if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
                ent->modeldata.runupdown = (int)dbltemp;
        }

        break;        
    }
    case _ep_seal:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->seal = (int)ltemp;
        break;        
    }
    case _ep_sealtime:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->sealtime = (int)ltemp;
        break;        
    }
    case _ep_dot:
    {   
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {        
            i = (int)ltemp;
        }
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->dot_time[i] = (long)dbltemp;
        }
        if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1) 
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
                ent->dot[i] = (int)dbltemp;
            else (*pretvar)->lVal = (LONG)0;            
        } 
        if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1) 
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
                ent->dot_force[i] = (int)dbltemp;
            else (*pretvar)->lVal = (LONG)0;            
        }
        if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1) 
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
                ent->dot_rate[i] = (int)dbltemp;
            else (*pretvar)->lVal = (LONG)0;            
        }
        if(paramCount >= 8 && (*pretvar)->lVal == (LONG)1) 
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp)))
                ent->dot_atk[i] = (int)dbltemp;
            else (*pretvar)->lVal = (LONG)0;            
        }
        if(paramCount >= 9) 
        {
            ent->dot_owner[i] = (entity*)varlist[8]->ptrVal;            
        }        
        break;
    }
    case _ep_edelay:
    {        
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.edelay.mode = (int)ltemp;
        }
        if(paramCount >= 3 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
                ent->modeldata.edelay.factor = (float)dbltemp;            
        } 
        if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
                ent->modeldata.edelay.cap_min = (int)ltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[5], &ltemp)))
                ent->modeldata.edelay.cap_max = (int)ltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[6], &ltemp)))
                ent->modeldata.edelay.range_min = (int)ltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[7], &ltemp)))
                ent->modeldata.edelay.range_max = (int)ltemp;
            else (*pretvar)->lVal = (LONG)0;
        }
        break;        
    }
	case _ep_mpset:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            ent->modeldata.mp = (int)dbltemp;
        }
        if(paramCount >= 4 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp)))
                ent->modeldata.mpstable = (int)dbltemp;
        } 
        if(paramCount >= 5 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[4], &dbltemp)))
                ent->modeldata.mpstableval = (int)dbltemp;
        }
        if(paramCount >= 6 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[5], &dbltemp)))
                ent->modeldata.mprate = (int)dbltemp;
        }
		if(paramCount >= 7 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[6], &dbltemp)))
                ent->modeldata.mpdroprate = (int)dbltemp;
        }
		if(paramCount >= 8 && (*pretvar)->lVal == (LONG)1)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[7], &dbltemp)))
                ent->modeldata.chargerate = (int)dbltemp;
        }
        break;
    }
    case _ep_blockpain:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
            ent->modeldata.blockpain = (int)ltemp;
        break;
    }
    default:
        printf("Property name '%s' is not supported by function changeentityproperty.\n", propname);
        goto changeentityproperty_error;
        break;
    }

    return S_OK;
changeentityproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}
//tossentity(entity, height, speedx, speedz)
HRESULT openbor_tossentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent = NULL;
    DOUBLE height=0, speedx=0, speedz=0;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)1;

    ent = (entity*)varlist[0]->ptrVal; //retrieve the entity
    if(!ent)
    {
        (*pretvar)->lVal = (LONG)0;
        return S_OK;
    }

    if(paramCount >= 2)
    {
        if(FAILED(ScriptVariant_DecimalValue(varlist[1], &height)))
        {
            (*pretvar)->lVal = (LONG)0;
            return S_OK;
        }
    }
    if(paramCount >= 3)
    {
        if(FAILED(ScriptVariant_DecimalValue(varlist[2], &speedx)))
        {
            (*pretvar)->lVal = (LONG)0;
            return S_OK;
        }
    }
    if(paramCount >= 4)
    {
        if(FAILED(ScriptVariant_DecimalValue(varlist[3], &speedz)))
        {
            (*pretvar)->lVal = (LONG)0;
            return S_OK;
        }
    }
    ent->xdir = (float)speedx;
    ent->zdir = (float)speedz;
    toss(ent, (float)height);
    return S_OK;
}

//getplayerproperty(index, propname);
HRESULT openbor_getplayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ltemp;
    int index;
    entity* ent = NULL;
    char* propname = NULL;
    ScriptVariant* arg = NULL;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_Clear(*pretvar);

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
    {
        index = 0;
    } else index = (int)ltemp;
    if(!(ent = player[index].ent))  //this player is not selected, so just return
    {
        return S_OK; //return S_OK, to tell the engine it is not a FATAL error
    }

    arg = varlist[1];
    if(arg->vt!=VT_STR) 
    {
        printf("Function call getplayerproperty has invalid propery name parameter, it must be a string value.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    propname = (char*)StrCache_Get(arg->strVal);

    //change the model
    if(strcmp(propname, "ent")==0 || strcmp(propname, "entity")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID*)ent;
    }
    else if(stricmp(propname, "name")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        strncpy(StrCache_Get((*pretvar)->strVal), (char*)player[index].name, MAX_STR_VAR_LEN);
    }
    else if(strcmp(propname, "score")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].score;
    }
    else if(strcmp(propname, "lives")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].lives;
    }
    else if(strcmp(propname, "playkeys")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)player[index].playkeys;
    }
	else if(strcmp(propname, "keys")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		(*pretvar)->lVal = (LONG)player[index].keys;
    }
    else if(strcmp(propname, "credits")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
		if(noshare) (*pretvar)->lVal = (LONG)player[index].credits;
        else        (*pretvar)->lVal = credits;
    }
    //this property is not known
    //else{
    //  .....
    //}
    return S_OK;
}

//changeplayerproperty(index, propname, value[, value2, value3,...]);
HRESULT openbor_changeplayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ltemp, ltemp2;
    int index;
    entity* ent = NULL;
    char* propname = NULL;    
    char* tempstr = NULL;
    ScriptVariant* arg = NULL;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)1;
    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &ltemp)))
    {
        index = 0;
    } else index = (int)ltemp;

    if(!(ent = player[index].ent))  //this player is not selected, so just return 
    {
        return S_OK; //return S_OK, to tell the engine it is not a FATAL error
    }
    
    if(varlist[1]->vt != VT_STR)
    {
        printf("You must give a string value for player property name.\n");
        return E_FAIL;
    }
    propname = (char*)StrCache_Get(varlist[1]->strVal);

    arg = varlist[2];
    //change the model
    if(strcmp(propname, "weapon")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp))){
            if(paramCount > 3)
            {
                arg = varlist[3];
                if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp2)))
                    (*pretvar)->lVal = (LONG)1;
            }
            else
            {
                ltemp2 = (LONG)0;
                (*pretvar)->lVal = (LONG)1;
            }
            set_weapon(player[index].ent, (int)ltemp, (int)ltemp2);
            (*pretvar)->lVal = (LONG)1;
        }
        else (*pretvar)->lVal = (LONG)0;
    }
    else if(stricmp(propname, "name")==0)
    {
        if(arg->vt != VT_STR)
        {
            //printf();
            return E_FAIL;
        }
        tempstr = (char*)StrCache_Get(arg->strVal);
        strcpy(player[index].name, tempstr);
        (*pretvar)->lVal = (LONG)1;
    }
    else if(strcmp(propname, "score")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
        {
            if(ltemp < 0) ltemp = 0;
            else if(ltemp > 999999999) ltemp = 999999999;
            player[index].score = (unsigned long)ltemp;
        }
        else (*pretvar)->lVal = (LONG)0;
    }
    else if(strcmp(propname, "lives")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
            player[index].lives = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
    }
    else if(strcmp(propname, "playkeys")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
            player[index].playkeys = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
    }
    else if(strcmp(propname, "credits")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg,&ltemp)))
        {
           	if(noshare) player[index].credits = (int)ltemp;
            else        credits = (int)ltemp;
		}
        else (*pretvar)->lVal = (LONG)0;
    }
    //this property is not known, so return 0
    //else{
    //    (*pretvar)->lVal = (LONG)0;
    //}
    return S_OK;
}

//checkhole(x,z), return 1 if there's hole here
HRESULT openbor_checkhole(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    DOUBLE x, z;
    
    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)0;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
        return S_OK;

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
        return S_OK;

    (*pretvar)->lVal = (LONG)(checkhole((float)x, (float)z) && checkwall((float)x, (float)z)<0);
    return S_OK;
}

//checkwall(x,z), return wall height, or 0
HRESULT openbor_checkwall(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    DOUBLE x, z;
    int wall;
    
    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    (*pretvar)->dblVal = (DOUBLE)0;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
        return S_OK;

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
        return S_OK;

    if((wall=checkwall_below((float)x, (float)z, 100000))>=0)
    {
        (*pretvar)->dblVal = (DOUBLE)level->walls[wall][7];
    }
    return S_OK;
}

//checkplatformbelow(x,z,a), return the highest platfrom entity below
HRESULT openbor_checkplatformbelow(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    DOUBLE x, z, a;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
    (*pretvar)->dblVal = (DOUBLE)0;

    arg = varlist[0];
    if(FAILED(ScriptVariant_DecimalValue(arg, &x)))
        return S_OK;

    arg = varlist[1];
    if(FAILED(ScriptVariant_DecimalValue(arg, &z)))
        return S_OK;
        
    arg = varlist[2];
    if(FAILED(ScriptVariant_DecimalValue(arg, &a)))
        return S_OK;
        
    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID*)check_platform_below((float)x,(float)z,(float)a);
    return S_OK;
}

HRESULT openbor_openfilestream(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    char* filename = NULL;
    ScriptVariant* arg = NULL;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_Clear(*pretvar);

    arg = varlist[0];
    if(arg->vt!=VT_STR)
    {
        printf("Filename for openfilestream must be a string.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    
    filename = (char*)StrCache_Get(arg->strVal);

    if(!level->numfilestreams) level->numfilestreams = 0;
    else if(level->numfilestreams == LEVEL_MAX_FILESTREAMS)
    {
        printf("Maximum file streams exceeded.\n");
        *pretvar = NULL;
        return E_FAIL;
    }

    if(buffer_pakfile(filename, &level->filestreams[level->numfilestreams].buf, &level->filestreams[level->numfilestreams].size)!=1)
    {
          printf("Invalid filename used in openfilestream.\n");
          *pretvar = NULL;
          return E_FAIL;
    }
    
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)level->numfilestreams;

    level->filestreams[level->numfilestreams].pos = 0;
    level->numfilestreams++;
    return S_OK;
}

HRESULT openbor_getfilestreamline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	char line[MAX_STR_VAR_LEN];
	int length;
    ScriptVariant* arg = NULL;
    LONG filestreamindex;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
        return S_OK;

    ScriptVariant_Clear(*pretvar);
    ScriptVariant_ChangeType(*pretvar, VT_STR);

	length = 0;
	strncpy(line, "it's initialized now", MAX_STR_VAR_LEN);

    while(level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos+length] && level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos+length]!='\n' && level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos+length]!='\r') ++length;
    if(length >= MAX_STR_VAR_LEN)
        strncpy(StrCache_Get((*pretvar)->strVal), (char*)(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos), MAX_STR_VAR_LEN);
    else
    {
        strncpy(line, (char*)(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos), length);
        line[length] = '\0';
        strncpy(StrCache_Get((*pretvar)->strVal), line, MAX_STR_VAR_LEN);
    }
    return S_OK;
}

HRESULT openbor_getfilestreamargument(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    LONG filestreamindex, argument;
    char* argtype = NULL;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
        return S_OK;

    arg = varlist[1];
    if(FAILED(ScriptVariant_IntegerValue(arg, &argument)))
        return S_OK;
    ScriptVariant_Clear(*pretvar);
    
    if(varlist[2]->vt != VT_STR)
    {
        printf("You must give a string value specifying what kind of value you want the argument converted to.\n");
        return E_FAIL;
    }
    argtype = (char*)StrCache_Get(varlist[2]->strVal);

    if(stricmp(argtype, "string")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        strncpy(StrCache_Get((*pretvar)->strVal), (char*)findarg(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos, argument), MAX_STR_VAR_LEN);
    }
    else if(stricmp(argtype, "int")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)atoi(findarg(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos, argument));
    }
    else if(stricmp(argtype, "float")==0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)atof(findarg(level->filestreams[filestreamindex].buf+level->filestreams[filestreamindex].pos, argument));
    }
    else
    {
        printf("Invalid type for argument converted to (getfilestreamargument).\n");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT openbor_filestreamnextline(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    LONG filestreamindex;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
        return S_OK;
    while(level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos] && level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos]!='\n' && level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos]!='\r') ++level->filestreams[filestreamindex].pos;
    while(level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos]=='\n' || level->filestreams[filestreamindex].buf[level->filestreams[filestreamindex].pos]=='\r') ++level->filestreams[filestreamindex].pos;

    return S_OK;
}

HRESULT openbor_getfilestreamposition(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    LONG filestreamindex;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
        return S_OK;
        
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)level->filestreams[filestreamindex].pos;
    return S_OK;
}

HRESULT openbor_setfilestreamposition(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    ScriptVariant* arg = NULL;
    LONG filestreamindex, position;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    arg = varlist[0];
    if(FAILED(ScriptVariant_IntegerValue(arg, &filestreamindex)))
        return S_OK;
        
    arg = varlist[1];
    if(FAILED(ScriptVariant_IntegerValue(arg, &position)))
        return S_OK;

    level->filestreams[filestreamindex].pos = position;
    return S_OK;
}

//damageentity(entity, other, force, drop, type)
HRESULT openbor_damageentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent = NULL;
    entity* other = NULL;
    entity* temp = NULL;
    LONG force, drop, type;
    s_attack attack;
    extern s_attack emptyattack;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)0;

    ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
    if(!ent)  return S_OK;

    other = ent;
    force = (LONG)1;
    drop = (LONG)0;
    type = (LONG)ATK_NORMAL;

    if(paramCount >= 2)
    {
        other = (entity*)(varlist[1])->ptrVal;
        if(!other) 
            return S_OK;
    }
    if(paramCount >= 3)
    {
        if(FAILED(ScriptVariant_IntegerValue((varlist[2]), &force)))
            return S_OK;
    }

    if(!ent->takedamage)
    {
        ent->health -= force;
        if(ent->health <= 0) kill(ent);
        (*pretvar)->lVal = (LONG)1;
        return S_OK;
    }

    if(paramCount >= 4)
    {
        if(FAILED(ScriptVariant_IntegerValue((varlist[3]), &drop)))
            return S_OK;
    }
    if(paramCount >= 5)
    {
        if(FAILED(ScriptVariant_IntegerValue((varlist[4]), &type)))
            return S_OK;
    }

    temp = self; self = ent;
    attack = emptyattack;
    attack.attack_force = force;
    attack.attack_drop = drop;
    if(drop) {attack.dropv[0] = (float)3; attack.dropv[1] = (float)1.2; attack.dropv[2] = (float)0;}
    attack.attack_type = type;
    self->takedamage(other, &attack);
    self = temp;
    return S_OK;
}

//killentity(entity)
HRESULT openbor_killentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent = NULL;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
    if(!ent)
    {
        (*pretvar)->lVal = (LONG)0;
        return S_OK;
    }
    kill(ent);
    (*pretvar)->lVal = (LONG)1;
    return S_OK;
}

//findtarget(entity, int animation);
HRESULT openbor_findtarget(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent = NULL;
    entity* tempself, *target;
    LONG anim = -1;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_PTR);

    ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
    if(!ent)
    {
        ScriptVariant_Clear(*pretvar);
        return S_OK;
    }
    if(paramCount>1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim))) return E_FAIL;
    tempself = self;
    self = ent;
    target = normal_find_target((int)anim);
    if(!target) ScriptVariant_Clear(*pretvar);
    else (*pretvar)->ptrVal = (VOID*)target;
    self = tempself;
    return S_OK;
}

//checkrange(entity, target, int ani);
HRESULT openbor_checkrange(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent = NULL, *target = NULL;
    LONG ani = 0;
    extern int max_animations;

    if(paramCount < 2) goto checkrange_error;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(varlist[0]->vt !=VT_PTR || varlist[1]->vt!=VT_PTR) goto checkrange_error;

    ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
    target = (entity*)(varlist[1])->ptrVal; //retrieve the target
    
    if(!ent || !target) goto checkrange_error;
    
    if(paramCount >2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &ani))) goto checkrange_error;
    else if(paramCount<=2) ani = ent->animnum;

    if(ani<0 || ani>=max_animations)
    {
        printf("Animation id out of range: %d / %d.\n", (int)ani, max_animations);
        goto checkrange_error;
    }

    (*pretvar)->lVal = check_range(ent, target, ani);

    return S_OK;
checkrange_error:
    printf("Function needs at least 2 valid entity handles, the third parameter is optional: checkrange(entity, target, int animnum)\n");
    *pretvar = NULL;
    return E_FAIL;
}

//clearspawnentry();
HRESULT openbor_clearspawnentry(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    *pretvar = NULL;
    memset(&spawnentry, 0, sizeof(s_spawn_entry));
    spawnentry.index = spawnentry.itemindex = spawnentry.weaponindex = -1;
    return S_OK;
}

//setspawnentry(propname, value1[, value2, value3, ...]);
HRESULT openbor_setspawnentry(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    char* propname = NULL;
    LONG ltemp;
    s_model* tempmodel;
    DOUBLE dbltemp;
    int temp, prop;
    ScriptVariant* arg = NULL;
    
    static const char* proplist[] = 
    {
        "2phealth",
        "2pitem",
        "3phealth",
        "3pitem",
        "4phealth",
        "4pitem",
        "aggression",
        "alias",
        "alpha",
        "boss",
        "coords",
        "credit",
        "dying",
        "flip",
        "health",
        "item",
        "itemalias",
        "itemhealth",
        "itemmap",
        "map",
        "mp",
        "multiple",
        "name",
        "nolife",
        "weapon",
    };
    
    enum propenum
    {
        _ep_2phealth,
        _ep_2pitem,
        _ep_3phealth,
        _ep_3pitem,
        _ep_4phealth,
        _ep_4pitem,
        _ep_aggression,
        _ep_alias,
        _ep_alpha,
        _ep_boss,
        _ep_coords,
        _ep_credit,
        _ep_dying,
        _ep_flip,
        _ep_health,
        _ep_item,
        _ep_itemalias,
        _ep_itemhealth,
        _ep_itemmap,
        _ep_map,
        _ep_mp,
        _ep_multiple,
        _ep_name,
        _ep_nolife,
        _ep_weapon,
        _ep_the_end,
    };

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)1;

    if(varlist[0]->vt != VT_STR)
    {
        printf("You must give a string value for spawn entry property name.\n");
        *pretvar = NULL;
        return E_FAIL;
    }
    propname = (char*)StrCache_Get(varlist[0]->strVal);
    
    prop = searchList(proplist, propname, _ep_the_end);

    arg = varlist[1];
    
    switch(prop)
    {
    case _ep_name:
        if(arg->vt != VT_STR) 
        {
            printf("You must use a string value for spawn entry's name property: function setspawnentry.\n");
            goto setspawnentry_error;
        }
        spawnentry.model = find_model((char*)StrCache_Get(arg->strVal));
        break;
    case _ep_alias:
        if(arg->vt != VT_STR) goto setspawnentry_error;
        strcpy(spawnentry.alias, (char*)StrCache_Get(arg->strVal));
        break;
    case _ep_item:
        if(arg->vt != VT_STR) goto setspawnentry_error;
        spawnentry.itemmodel = find_model((char*)StrCache_Get(arg->strVal));
        spawnentry.item = spawnentry.itemmodel->name;
        spawnentry.itemindex = get_cached_model_index(spawnentry.item);
        spawnentry.itemplayer_count = 0;
        break;
    case _ep_2pitem:
        if(arg->vt != VT_STR) goto setspawnentry_error;
        tempmodel = find_model((char*)StrCache_Get(arg->strVal));
        if(!tempmodel) spawnentry.item = NULL;
        else spawnentry.item = tempmodel->name;
        spawnentry.itemplayer_count = 1;
        break;
    case _ep_3pitem:
        if(arg->vt != VT_STR) goto setspawnentry_error;
        spawnentry.itemmodel = find_model((char*)StrCache_Get(arg->strVal));
        spawnentry.itemplayer_count = 2;
        break;
    case _ep_4pitem:
        if(arg->vt != VT_STR) goto setspawnentry_error;
        spawnentry.itemmodel = find_model((char*)StrCache_Get(arg->strVal));
        spawnentry.itemplayer_count = 3;
        break;
    case _ep_health:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.health[0] = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_itemhealth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.itemhealth = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_itemalias:
        if(arg->vt != VT_STR) return E_FAIL;
        strcpy(spawnentry.itemalias, (char*)StrCache_Get(arg->strVal));
        break;
    case _ep_2phealth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.health[1] = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_3phealth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.health[2] = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_4phealth:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.health[3] = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_coords:
        temp = 1;
        if(SUCCEEDED(ScriptVariant_DecimalValue(arg, &dbltemp))) spawnentry.x = (float)dbltemp;
        else temp = 0;
        if(paramCount >= 3 && temp)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp))) spawnentry.z = (float)dbltemp;
            else temp = 0;
        }
        if(paramCount >= 4 && temp)
        {
            if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[3], &dbltemp))) spawnentry.a = (float)dbltemp;
            else temp = 0;
        }
        (*pretvar)->lVal = (LONG)temp;
        break;
    case _ep_mp:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.mp = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_map:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.colourmap = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_itemmap:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.itemmap = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_alpha:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.alpha = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_multiple:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.multiple = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_dying:
        temp = 1;
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.dying = (int)ltemp;
        else temp = 0;
        if(paramCount >= 3 && temp)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
                spawnentry.per1 = (int)ltemp;
            else temp = 0;
        }
        if(paramCount >= 4 && temp)
        {
            if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
                spawnentry.per2 = (int)ltemp;
            else temp = 0;
        }
        (*pretvar)->lVal = (LONG)temp;
        break;
    case _ep_nolife:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.nolife = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_boss:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.boss = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_flip:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.flip = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_credit:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.credit = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_aggression:
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            spawnentry.aggression = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
        break;
    case _ep_weapon:
        if(arg->vt != VT_STR) goto setspawnentry_error;
        spawnentry.weaponmodel = find_model((char*)StrCache_Get(arg->strVal));
        break;
    default:
        printf("Property name '%s' is not supported by setspawnentry.\n", propname);
        goto setspawnentry_error;
    }

    return S_OK;
setspawnentry_error:
    *pretvar = NULL;
    return E_FAIL;
}

//spawn();
HRESULT openbor_spawn(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent;

    ent = smartspawn(&spawnentry);

    if(ent)
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID*) ent;
    }
    else     ScriptVariant_Clear(*pretvar);

    return S_OK;
}

//entity * projectile(char *name, float x, float z, float a, int direction, int type, int ptype, int map);
HRESULT openbor_projectile(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[7];
	entity* ent;
	char name[64] = {""};
	
	if(paramCount != 8) goto projectile_error;
    if(varlist[0]->vt != VT_STR) goto projectile_error;
 
    ScriptVariant_Clear(*pretvar);

	strncpy(name, StrCache_Get(varlist[0]->strVal), MAX_STR_VAR_LEN);

	for(i=1; i<=7; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], &value[i-1])))
		    goto projectile_error;
	}

	switch((int)value[5])
	{
		default:
		case 0:
			ent = knife_spawn(name, -1, (float)value[0], (float)value[1], (float)value[2], (int)value[3], (int)value[4], (int)value[6]);
			break;
		case 1:
			ent = bomb_spawn(name, -1, (float)value[0], (float)value[1], (float)value[2], (int)value[3], (int)value[6]);
			break;
	}

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    (*pretvar)->ptrVal = (VOID*) ent;

    return S_OK;

projectile_error:
	printf("Function requires 8 values: entity * projectile(char *name, float x, float z, float a, int direction, int type, int ptype, int map)\n");
	return E_FAIL;
}

#define ICMPCONST(x) \
if(stricmp(#x, constname)==0) {\
    (*pretvar)->lVal = (LONG)x;\
    return S_OK;\
}


//openborconstant(constname);
//translate a constant by string, used to retrieve a constant or macro of openbor
HRESULT openbor_transconst(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    char* constname = NULL;
    int temp;

    if(paramCount < 1)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }
    
    if(varlist[0]->vt != VT_STR)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }
    constname = StrCache_Get(varlist[0]->strVal);

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    ICMPCONST(COMPATIBLEVERSION)
    ICMPCONST(PIXEL_8)
    ICMPCONST(PIXEL_x8)
    ICMPCONST(PIXEL_16)
    ICMPCONST(PIXEL_32)
    ICMPCONST(CV_SAVED_GAME)
    ICMPCONST(CV_HIGH_SCORE)
    ICMPCONST(THINK_SPEED)
    ICMPCONST(COUNTER_SPEED)
    ICMPCONST(MAX_ENTS)
    ICMPCONST(MAX_PANELS)
    ICMPCONST(MAX_WEAPONS)
    ICMPCONST(MAX_COLOUR_MAPS)
    ICMPCONST(MAX_NAME_LEN)
    ICMPCONST(LEVEL_MAX_SPAWNS)
    ICMPCONST(LEVEL_MAX_PANELS)
    ICMPCONST(LEVEL_MAX_HOLES)
    ICMPCONST(LEVEL_MAX_WALLS)
    ICMPCONST(MAX_LEVELS)
    ICMPCONST(MAX_DIFFICULTIES)
    ICMPCONST(MAX_SPECIALS)
    ICMPCONST(MAX_ATCHAIN)
    ICMPCONST(MAX_ATTACKS)
    ICMPCONST(MAX_FOLLOWS)
    ICMPCONST(MAX_PLAYERS)
    ICMPCONST(MAX_ARG_LEN)
    ICMPCONST(FLAG_ESC)
    ICMPCONST(FLAG_START)
    ICMPCONST(FLAG_MOVELEFT)
    ICMPCONST(FLAG_MOVERIGHT)
    ICMPCONST(FLAG_MOVEUP)
    ICMPCONST(FLAG_MOVEDOWN)
    ICMPCONST(FLAG_ATTACK)
	ICMPCONST(FLAG_ATTACK2)
	ICMPCONST(FLAG_ATTACK3)
	ICMPCONST(FLAG_ATTACK4)
    ICMPCONST(FLAG_JUMP)
    ICMPCONST(FLAG_SPECIAL)
    ICMPCONST(FLAG_SCREENSHOT)
    ICMPCONST(FLAG_ANYBUTTON)
    ICMPCONST(FLAG_FORWARD)
    ICMPCONST(FLAG_BACKWARD)
    ICMPCONST(SDID_MOVEUP)
    ICMPCONST(SDID_MOVEDOWN)
    ICMPCONST(SDID_MOVELEFT)
    ICMPCONST(SDID_MOVERIGHT)
    ICMPCONST(SDID_SPECIAL)
    ICMPCONST(SDID_ATTACK)
	ICMPCONST(SDID_ATTACK2)
	ICMPCONST(SDID_ATTACK3)
	ICMPCONST(SDID_ATTACK4)
    ICMPCONST(SDID_JUMP)
    ICMPCONST(SDID_START)
    ICMPCONST(SDID_SCREENSHOT)
    ICMPCONST(TYPE_NONE)
    ICMPCONST(TYPE_PLAYER)
    ICMPCONST(TYPE_ENEMY)
    ICMPCONST(TYPE_ITEM)
    ICMPCONST(TYPE_OBSTACLE)
    ICMPCONST(TYPE_STEAMER)
    ICMPCONST(TYPE_SHOT)
    ICMPCONST(TYPE_TRAP)
    ICMPCONST(TYPE_TEXTBOX)
    ICMPCONST(TYPE_ENDLEVEL)
    ICMPCONST(TYPE_NPC)
    ICMPCONST(TYPE_PANEL)
    ICMPCONST(SUBTYPE_NONE)
    ICMPCONST(SUBTYPE_BIKER)
    ICMPCONST(SUBTYPE_NOTGRAB)
    ICMPCONST(SUBTYPE_ARROW)
    ICMPCONST(SUBTYPE_TOUCH)
    ICMPCONST(SUBTYPE_WEAPON)
    ICMPCONST(SUBTYPE_NOSKIP)
    ICMPCONST(SUBTYPE_FLYDIE)
    ICMPCONST(SUBTYPE_BOTH)
    ICMPCONST(SUBTYPE_PROJECTILE)
    ICMPCONST(SUBTYPE_FOLLOW)
    ICMPCONST(SUBTYPE_CHASE)
    ICMPCONST(AIMOVE1_NORMAL)
    ICMPCONST(AIMOVE1_CHASE)
    ICMPCONST(AIMOVE1_CHASEZ)
    ICMPCONST(AIMOVE1_CHASEX)
    ICMPCONST(AIMOVE1_AVOID)
    ICMPCONST(AIMOVE1_AVOIDZ)
    ICMPCONST(AIMOVE1_AVOIDX)
    ICMPCONST(AIMOVE1_WANDER)
    ICMPCONST(AIMOVE1_NOMOVE)
    ICMPCONST(AIMOVE1_BIKER)
    ICMPCONST(AIMOVE1_STAR)
    ICMPCONST(AIMOVE1_ARROW)
    ICMPCONST(AIMOVE1_BOMB)
    ICMPCONST(AIMOVE2_NORMAL)
    ICMPCONST(AIMOVE2_IGNOREHOLES)
    ICMPCONST(AIATTACK1_NORMAL)
    ICMPCONST(AIATTACK1_LONG)
    ICMPCONST(AIATTACK1_MELEE)
    ICMPCONST(AIATTACK1_NOATTACK)
    ICMPCONST(AIATTACK2_NORMAL)
    ICMPCONST(AIATTACK2_DODGE)
    ICMPCONST(AIATTACK2_DODGEMOVE)
    ICMPCONST(FRONTPANEL_Z)
    ICMPCONST(HOLE_Z)
    ICMPCONST(NEONPANEL_Z)
    ICMPCONST(SHADOW_Z)
    ICMPCONST(SCREENPANEL_Z)
    ICMPCONST(PANEL_Z)
    ICMPCONST(MIRROR_Z)
    ICMPCONST(PIT_DEPTH)
    ICMPCONST(P2_STATS_DIST)
    ICMPCONST(CONTACT_DIST_H)
    ICMPCONST(CONTACT_DIST_V)
    ICMPCONST(GRAB_DIST)
    ICMPCONST(GRAB_STALL)
    ICMPCONST(ATK_NORMAL)
    ICMPCONST(ATK_NORMAL2)
    ICMPCONST(ATK_NORMAL3)
    ICMPCONST(ATK_NORMAL4)
    ICMPCONST(ATK_BLAST)
    ICMPCONST(ATK_BURN)
    ICMPCONST(ATK_FREEZE)
    ICMPCONST(ATK_SHOCK)
    ICMPCONST(ATK_STEAL)
    ICMPCONST(ATK_NORMAL5)
    ICMPCONST(ATK_NORMAL6)
    ICMPCONST(ATK_NORMAL7)
    ICMPCONST(ATK_NORMAL8)
    ICMPCONST(ATK_NORMAL9)
    ICMPCONST(ATK_NORMAL10)
    ICMPCONST(ATK_ITEM)
    ICMPCONST(SCROLL_RIGHT)
    ICMPCONST(SCROLL_DOWN)
    ICMPCONST(SCROLL_LEFT)
    ICMPCONST(SCROLL_UP)
    ICMPCONST(SCROLL_BOTH)
    ICMPCONST(SCROLL_LEFTRIGHT)
    ICMPCONST(SCROLL_RIGHTLEFT)
    ICMPCONST(SCROLL_INWARD)
    ICMPCONST(SCROLL_OUTWARD)
    ICMPCONST(SCROLL_INOUT)
    ICMPCONST(SCROLL_OUTIN)
    ICMPCONST(SCROLL_UPWARD)
    ICMPCONST(SCROLL_DOWNWARD)
    ICMPCONST(ANI_IDLE)   
    ICMPCONST(ANI_WALK)   
    ICMPCONST(ANI_JUMP)
    ICMPCONST(ANI_LAND)
    ICMPCONST(ANI_PAIN)
    ICMPCONST(ANI_FALL)
    ICMPCONST(ANI_RISE)
    //ICMPCONST(ANI_ATTACK1)// move these below because we have some dynamic animation ids
    //ICMPCONST(ANI_ATTACK2)
    //ICMPCONST(ANI_ATTACK3)
    //ICMPCONST(ANI_ATTACK4)
    ICMPCONST(ANI_UPPER)
    ICMPCONST(ANI_BLOCK)
    ICMPCONST(ANI_JUMPATTACK)
    ICMPCONST(ANI_JUMPATTACK2)
    ICMPCONST(ANI_GET)
    ICMPCONST(ANI_GRAB)
    ICMPCONST(ANI_GRABATTACK)
    ICMPCONST(ANI_GRABATTACK2)
    ICMPCONST(ANI_THROW)
    ICMPCONST(ANI_SPECIAL)
    //ICMPCONST(ANI_FREESPECIAL)// move these below because we have some dynamic animation ids
    ICMPCONST(ANI_SPAWN)
    ICMPCONST(ANI_DIE)
    ICMPCONST(ANI_PICK)
    //ICMPCONST(ANI_FREESPECIAL2)
    ICMPCONST(ANI_JUMPATTACK3)
    //ICMPCONST(ANI_FREESPECIAL3)
    ICMPCONST(ANI_UP)           
    ICMPCONST(ANI_DOWN)         
    ICMPCONST(ANI_SHOCK)
    ICMPCONST(ANI_BURN)
    ICMPCONST(ANI_SHOCKPAIN)
    ICMPCONST(ANI_BURNPAIN)
    ICMPCONST(ANI_GRABBED)
    ICMPCONST(ANI_SPECIAL2)
    ICMPCONST(ANI_RUN)
    ICMPCONST(ANI_RUNATTACK)
    ICMPCONST(ANI_RUNJUMPATTACK)
    ICMPCONST(ANI_ATTACKUP)
    ICMPCONST(ANI_ATTACKDOWN)
    ICMPCONST(ANI_ATTACKFORWARD)
    ICMPCONST(ANI_ATTACKBACKWARD)
    //ICMPCONST(ANI_FREESPECIAL4)
    //ICMPCONST(ANI_FREESPECIAL5)
    //ICMPCONST(ANI_FREESPECIAL6)
    //ICMPCONST(ANI_FREESPECIAL7)
    //ICMPCONST(ANI_FREESPECIAL8)
    ICMPCONST(ANI_RISEATTACK)
    ICMPCONST(ANI_DODGE)
    ICMPCONST(ANI_ATTACKBOTH)
    ICMPCONST(ANI_GRABFORWARD)
    ICMPCONST(ANI_GRABFORWARD2)
    ICMPCONST(ANI_JUMPFORWARD)
    ICMPCONST(ANI_GRABDOWN)
    ICMPCONST(ANI_GRABDOWN2)
    ICMPCONST(ANI_GRABUP)
    ICMPCONST(ANI_GRABUP2)
    ICMPCONST(ANI_SELECT)
    ICMPCONST(ANI_DUCK)
    ICMPCONST(ANI_FAINT)
    ICMPCONST(ANI_CANT)
    ICMPCONST(ANI_THROWATTACK)
    ICMPCONST(ANI_CHARGEATTACK)
    ICMPCONST(ANI_VAULT)
    ICMPCONST(ANI_JUMPCANT)
    ICMPCONST(ANI_JUMPSPECIAL)
    ICMPCONST(ANI_BURNDIE)
    ICMPCONST(ANI_SHOCKDIE)
    ICMPCONST(ANI_PAIN2)
    ICMPCONST(ANI_PAIN3)
    ICMPCONST(ANI_PAIN4)
    ICMPCONST(ANI_FALL2)
    ICMPCONST(ANI_FALL3)
    ICMPCONST(ANI_FALL4)
    ICMPCONST(ANI_DIE2)
    ICMPCONST(ANI_DIE3)
    ICMPCONST(ANI_DIE4)
    ICMPCONST(ANI_CHARGE)
    ICMPCONST(ANI_BACKWALK) 
    ICMPCONST(ANI_SLEEP)
    //ICMPCONST(ANI_FOLLOW1) // move these below because we have some dynamic animation ids
    //ICMPCONST(ANI_FOLLOW2)
    //ICMPCONST(ANI_FOLLOW3)
    //ICMPCONST(ANI_FOLLOW4)
    ICMPCONST(ANI_PAIN5)
    ICMPCONST(ANI_PAIN6)
    ICMPCONST(ANI_PAIN7)
    ICMPCONST(ANI_PAIN8)
    ICMPCONST(ANI_PAIN9)
    ICMPCONST(ANI_PAIN10)
    ICMPCONST(ANI_FALL5)
    ICMPCONST(ANI_FALL6)
    ICMPCONST(ANI_FALL7)
    ICMPCONST(ANI_FALL8)
    ICMPCONST(ANI_FALL9)
    ICMPCONST(ANI_FALL10)
    ICMPCONST(ANI_DIE5)
    ICMPCONST(ANI_DIE6)
    ICMPCONST(ANI_DIE7)
    ICMPCONST(ANI_DIE8)
    ICMPCONST(ANI_DIE9)
    ICMPCONST(ANI_DIE10)
    ICMPCONST(ANI_TURN)
    ICMPCONST(ANI_RESPAWN)
    ICMPCONST(ANI_FORWARDJUMP)
    ICMPCONST(ANI_RUNJUMP)
    ICMPCONST(ANI_JUMPLAND)
    ICMPCONST(ANI_JUMPDELAY)
    ICMPCONST(ANI_HITWALL)
    ICMPCONST(ANI_GRABBACKWARD)
    ICMPCONST(ANI_GRABBACKWARD2)
    ICMPCONST(ANI_GRABWALK)
    ICMPCONST(ANI_GRABBEDWALK)
    ICMPCONST(ANI_GRABWALKUP)
    ICMPCONST(ANI_GRABBEDWALKUP)
    ICMPCONST(ANI_GRABWALKDOWN)
    ICMPCONST(ANI_GRABBEDWALKDOWN)
    ICMPCONST(ANI_GRABTURN)
    ICMPCONST(ANI_GRABBEDTURN)
    ICMPCONST(ANI_GRABBACKWALK)
    ICMPCONST(ANI_GRABBEDBACKWALK)
    ICMPCONST(ANI_SLIDE)
    ICMPCONST(ANI_RUNSLIDE)
    ICMPCONST(ANI_BLOCKPAIN)
    ICMPCONST(ANI_DUCKATTACK)
    ICMPCONST(MAX_ANIS)
    ICMPCONST(PLAYER_MIN_Z)
    ICMPCONST(PLAYER_MAX_Z)
    ICMPCONST(BGHEIGHT)
    ICMPCONST(MAX_WALL_HEIGHT)
	ICMPCONST(SAMPLE_GO);
	ICMPCONST(SAMPLE_BEAT);
	ICMPCONST(SAMPLE_BLOCK);
	ICMPCONST(SAMPLE_INDIRECT);
	ICMPCONST(SAMPLE_GET);
	ICMPCONST(SAMPLE_GET2);
	ICMPCONST(SAMPLE_FALL);
	ICMPCONST(SAMPLE_JUMP);
	ICMPCONST(SAMPLE_PUNCH);
	ICMPCONST(SAMPLE_1UP);
	ICMPCONST(SAMPLE_TIMEOVER);
	ICMPCONST(SAMPLE_BEEP);
	ICMPCONST(SAMPLE_BEEP2);
	ICMPCONST(SAMPLE_BIKE);
	ICMPCONST(ANI_RISE2);			
    ICMPCONST(ANI_RISE3);			
    ICMPCONST(ANI_RISE4);			
    ICMPCONST(ANI_RISE5);			
    ICMPCONST(ANI_RISE6);			
    ICMPCONST(ANI_RISE7);			
    ICMPCONST(ANI_RISE8);			
    ICMPCONST(ANI_RISE9);			
    ICMPCONST(ANI_RISE10);			
    ICMPCONST(ANI_RISEB);			
    ICMPCONST(ANI_RISES);			
    ICMPCONST(ANI_BLOCKPAIN2);		
    ICMPCONST(ANI_BLOCKPAIN3);		
    ICMPCONST(ANI_BLOCKPAIN4);		
    ICMPCONST(ANI_BLOCKPAIN5);		
    ICMPCONST(ANI_BLOCKPAIN6);		
    ICMPCONST(ANI_BLOCKPAIN7);		
    ICMPCONST(ANI_BLOCKPAIN8);		
    ICMPCONST(ANI_BLOCKPAIN9);		
    ICMPCONST(ANI_BLOCKPAIN10);		
    ICMPCONST(ANI_BLOCKPAINB);		
    ICMPCONST(ANI_BLOCKPAINS);		
    ICMPCONST(ANI_CHIPDEATH);       
    ICMPCONST(ANI_GUARDBREAK);      
    ICMPCONST(ANI_RISEATTACK2);	    
    ICMPCONST(ANI_RISEATTACK3);		
    ICMPCONST(ANI_RISEATTACK4);		
    ICMPCONST(ANI_RISEATTACK5);		
    ICMPCONST(ANI_RISEATTACK6);		
    ICMPCONST(ANI_RISEATTACK7);		
    ICMPCONST(ANI_RISEATTACK8);		
    ICMPCONST(ANI_RISEATTACK9);		
    ICMPCONST(ANI_RISEATTACK10);	
    ICMPCONST(ANI_RISEATTACKB);		
    ICMPCONST(ANI_RISEATTACKS);
    ICMPCONST(ANI_SLIDE);   
    ICMPCONST(ANI_RUNSLIDE);
    ICMPCONST(ANI_DUCKATTACK);

    // for the extra animation ids
    // for the extra animation ids
    if(strnicmp(constname, "ANI_DOWN", 8)==0 && constname[8] >= '1') // new down walk?
    {
        temp = atoi(constname+8);
        (*pretvar)->lVal = (LONG)(animdowns[temp-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_UP", 8)==0 && constname[8] >= '1') // new up walk?
    {
        temp = atoi(constname+8);
        (*pretvar)->lVal = (LONG)(animups[temp-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_BACKWALK", 8)==0 && constname[8] >= '1') // new backwalk?
    {
        temp = atoi(constname+8);
        (*pretvar)->lVal = (LONG)(animbackwalks[temp-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_WALK", 8)==0 && constname[8] >= '1') // new Walk?
    {
        temp = atoi(constname+8);
        (*pretvar)->lVal = (LONG)(animwalks[temp-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_IDLE", 8)==0 && constname[8] >= '1') // new idle?
    {
        temp = atoi(constname+8);
        (*pretvar)->lVal = (LONG)(animidles[temp-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_FALL", 8)==0 && constname[8] >= '1' && constname[8]<='9') // new fall?
    {
        temp = atoi(constname+8); // so must be greater than 10
        if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
        (*pretvar)->lVal = (LONG)(animfalls[temp+STA_ATKS-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_RISE", 8)==0 && constname[8] >= '1' && constname[8]<='9') // new fall?
    {
        temp = atoi(constname+8);
        if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
        (*pretvar)->lVal = (LONG)(animrises[temp+STA_ATKS-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_RISEATTACK", 14)==0 && constname[14] >= '1' && constname[14]<='9') // new fall?
    {
        temp = atoi(constname+14);
        if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
        (*pretvar)->lVal = (LONG)(animriseattacks[temp+STA_ATKS-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_PAIN", 8)==0 && constname[8] >= '1' && constname[8]<='9') // new fall?
    {
        temp = atoi(constname+8); // so must be greater than 10
        if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
        (*pretvar)->lVal = (LONG)(animpains[temp+STA_ATKS-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_DIE", 7)==0 && constname[7] >= '1' && constname[7]<='9') // new fall?
    {
        temp = atoi(constname+7); // so must be greater than 10
        if(temp<MAX_ATKS-STA_ATKS+1) temp = MAX_ATKS-STA_ATKS+1; // just in case
        (*pretvar)->lVal = (LONG)(animdies[temp+STA_ATKS-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_ATTACK", 10)==0 && constname[10] >= '1' && constname[10]<='9')
    {
        temp = atoi(constname+10);
        (*pretvar)->lVal = (LONG)(animattacks[temp-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_FOLLOW", 10)==0 && constname[10] >= '1' && constname[10]<='9')
    {
        temp = atoi(constname+10);
        (*pretvar)->lVal = (LONG)(animfollows[temp-1]);
        return S_OK;
    }
    if(strnicmp(constname, "ANI_FREESPECIAL", 15)==0 && (!constname[15] ||(constname[15] >= '1' && constname[15]<='9')))
    {
        temp = atoi(constname+15);
        if(temp<1) temp = 1;
        (*pretvar)->lVal = (LONG)(animspecials[temp-1]);
        return S_OK;
    }
    ScriptVariant_Clear(*pretvar);
    return S_OK;
}

//playerkeys(playerindex, newkey?, key1, key2, ...);
HRESULT openbor_playerkeys(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ltemp;
    int index, newkey;
    int i;
    unsigned long keys;
    ScriptVariant* arg = NULL;

    if(paramCount < 3)
    {
        *pretvar = NULL;
        return E_FAIL;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)1;

    if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &ltemp)))
    {
        index = 0;
    } else index = (int)ltemp;

    if(SUCCEEDED(ScriptVariant_IntegerValue((varlist[1]), &ltemp)))
        newkey = (int)ltemp;
    else newkey = 0;
    
    if(newkey == 1) keys = player[index].newkeys;
    else if(newkey == 2) keys = player[index].releasekeys;
    else keys = player[index].keys;

    for(i=2; i<paramCount; i++)
    {
        arg = varlist[i];
        if(arg->vt == VT_STR)
        {
            if(stricmp(StrCache_Get(arg->strVal), "jump")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_JUMP);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "attack")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_ATTACK);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "attack2")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_ATTACK2);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "attack3")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_ATTACK3);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "attack4")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_ATTACK4);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "special")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_SPECIAL);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "esc")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_ESC);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "start")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_START);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "moveleft")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_MOVELEFT);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "moveright")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_MOVERIGHT);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "moveup")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_MOVEUP);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "movedown")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_MOVEDOWN);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "SCREENSHOT")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_SCREENSHOT);
            }
            else if(stricmp(StrCache_Get(arg->strVal), "ANYBUTTON")==0)
            {
                (*pretvar)->lVal = (LONG)(keys & FLAG_ANYBUTTON);
            }
            else (*pretvar)->lVal = (LONG)0;
        }
        else (*pretvar)->lVal = (LONG)0;
        if(!((*pretvar)->lVal)) break;
    }

    return S_OK;
}

//playmusic(name, loop)
HRESULT openbor_playmusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    int loop = 0;
	LONG offset = 0;
    char* thename = NULL;

    *pretvar = NULL;
    if(paramCount < 1)
    {
        sound_close_music();
        return S_OK;
    }
    if(varlist[0]->vt != VT_STR)
    {
        //printf("");
        return E_FAIL;
    }
    thename = StrCache_Get(varlist[0]->strVal);

    if(paramCount > 1)
    {
        loop = (int)ScriptVariant_IsTrue(varlist[1]);
    }

	if(paramCount > 2)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[2], &offset)))
			return E_FAIL;
    }


    music(thename, loop, offset);
    return S_OK;
}

//fademusic(fade, name, loop, offset)
HRESULT openbor_fademusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	DOUBLE value = 0;
    LONG values[2] = {0,0};
	*pretvar = NULL;
	if(paramCount < 1) goto fademusic_error;
	if(FAILED(ScriptVariant_DecimalValue(varlist[0], &value))) goto fademusic_error;
	musicfade[0] = value;
	musicfade[1] = (float)savedata.musicvol;

	if(paramCount == 4)
	{
		strncpy(musicname, StrCache_Get(varlist[1]->strVal), 128);
		if(FAILED(ScriptVariant_IntegerValue(varlist[2], &values[0]))) goto fademusic_error;
		if(FAILED(ScriptVariant_IntegerValue(varlist[3], &values[1]))) goto fademusic_error;
        musicloop = values[0];
        musicoffset = values[1];
	}
	return S_OK;

fademusic_error:
	printf("Function requires 1 value, with an optional 3 for music triggering: fademusic_error(float fade, char name, int loop, unsigned long offset)\n");
	return E_FAIL;
}

//setmusicvolume(left, right)
HRESULT openbor_setmusicvolume(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG channels[2];

    if(paramCount < 1)
    {
        return S_OK;
    }
    
    if(FAILED(ScriptVariant_IntegerValue(varlist[0], channels)))
            goto setmusicvolume_error;

    if(paramCount > 1)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[1], channels+1)))
            goto setmusicvolume_error;
    }
    else
        channels[1] = channels[0];

    sound_volume_music((int)channels[0], (int)channels[1]);
    return S_OK;
    
setmusicvolume_error:
    printf("values must be integers: setmusicvolume(int left, (optional)int right)\n");
    return E_FAIL;
}

//setmusicvolume(left, right)
HRESULT openbor_setmusictempo(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG new_tempo;

    if(paramCount < 1)
    {
        return S_OK;
    }

	if(FAILED(ScriptVariant_IntegerValue(varlist[0], &new_tempo)))
		return E_FAIL;

    sound_music_tempo(new_tempo);
    return S_OK;
}

//pausemusic(value)
HRESULT openbor_pausemusic(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    int pause = 0;
    if(paramCount < 1)
    {
        return S_OK;
    }

    pause = (int)ScriptVariant_IsTrue(varlist[0]);

    sound_pause_music(pause);
    return S_OK;
}

//playsample(id, priority, lvolume, rvolume, speed, loop)
HRESULT openbor_playsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	int i;
	LONG value[6];

	if(paramCount != 6) goto playsample_error;

    *pretvar = NULL;
	for(i=0; i<6; i++)
	{
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i)))
			goto playsample_error;
	}
	if((int)value[0] < 0)
	{
		printf("Invalid Id for playsample(id=%d, priority=%d, lvolume=%d, rvolume=%d, speed=%d, loop=%d)\n", (int)value[0], (unsigned int)value[1], (int)value[2], (int)value[3], (unsigned int)value[4], (int)value[5]);
		return E_FAIL;
	}
	if((int)value[5]) sound_loop_sample((int)value[0], (unsigned int)value[1], (int)value[2], (int)value[3], (unsigned int)value[4]);
	else sound_play_sample((int)value[0], (unsigned int)value[1], (int)value[2], (int)value[3], (unsigned int)value[4]);
    return S_OK;

playsample_error:
	printf("Function requires 6 integer values: playsample(int id, unsigned int priority, int lvolume, int rvolume, unsigned int speed, int loop)\n");
	return E_FAIL;
}

// int loadsample(filename)
HRESULT openbor_loadsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	if(paramCount != 1)
	{
		goto loadsample_error;
	}
    if(varlist[0]->vt != VT_STR) goto loadsample_error;
 
    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)sound_load_sample(StrCache_Get(varlist[0]->strVal), packfile);
	return S_OK;

loadsample_error:
	printf("Function requires 1 string value: loadsample(string filename)\n");
    *pretvar = NULL;
	return E_FAIL;
}

// void unloadsample(id)
HRESULT openbor_unloadsample(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG id;
    *pretvar = NULL;
	if(paramCount != 1 ) goto unloadsample_error;

    if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &id)))
		goto unloadsample_error;

	sound_unload_sample((int)id);
    return S_OK;

unloadsample_error:
	printf("Function requires 1 integer value: unloadsample(int id)\n");
	return E_FAIL;
}

//fadeout(type);
HRESULT openbor_fadeout(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG type;
	LONG speed;
    *pretvar = NULL;
	if(paramCount < 1 ) goto fade_out_error;

    if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &type)))
		goto fade_out_error;
	if(FAILED(ScriptVariant_IntegerValue((varlist[1]), &speed)))

	fade_out((int)type, (int)speed);
    return S_OK;

fade_out_error:
	printf("Function requires 1 integer value: fade_out(int type)\n");
	return E_FAIL;
}

//changepalette(index);
HRESULT openbor_changepalette(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG index;

    *pretvar = NULL;

    if(paramCount < 1) goto changepalette_error;

    if(FAILED(ScriptVariant_IntegerValue((varlist[0]), &index))) 
        goto changepalette_error;

    change_system_palette((int)index);

    return S_OK;

changepalette_error:
	printf("Function requires 1 integer value: changepalette(int index)\n");
	return E_FAIL;
}

//changelight(x, z);
HRESULT openbor_changelight(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG x, z;
    extern int            light[2];
    ScriptVariant* arg = NULL;

    *pretvar = NULL;
    if(paramCount < 2) goto changelight_error;

    arg = varlist[0];
    if(arg->vt!=VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &x))) 
            goto changelight_error;
        light[0] = (int)x;            
    }
    
    arg = varlist[1];
    if(arg->vt!=VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &z))) 
            goto changelight_error;
        light[1] = (int)z;            
    }

    return S_OK;
changelight_error:
	printf("Function requires 2 integer values: changepalette(int x, int z)\n");
	return E_FAIL;
}

//changeshadowcolor(color, alpha);
// color = 0 means no gfxshadow, -1 means don't fill the shadow with colour
// alpha default to 2, <=0 means no alpha effect
HRESULT openbor_changeshadowcolor(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG c, a;
    extern int            shadowcolor;
    extern int            shadowalpha;

    *pretvar = NULL;
    if(paramCount < 1) goto changeshadowcolor_error;

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &c)))
        goto changeshadowcolor_error;

    shadowcolor = (int)c;

    if(paramCount>1)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[1], &a)))
            goto changeshadowcolor_error;
        shadowalpha = (int)a;
    }

    return S_OK;
changeshadowcolor_error:
	printf("Function requires at least 1 integer value, the 2nd integer parameter is optional: changepalette(int colorindex, int alpha)\n");
	return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_gettextobjproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    char* propname = NULL;
    int propind;

    static const char* proplist[] = {
        "font",
        "text",
        "x",
        "y",
        "z",
    };
    
    typedef enum
    {
        _top_font,
        _top_text,
        _top_x,
        _top_y,
        _top_z,
        _top_the_end,
    } prop_enum;

    if(paramCount < 1)
        goto gettextobjproperty_error;


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: gettextproperty(int index, \"property\")\n");
        goto gettextobjproperty_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind >= LEVEL_MAX_TEXTOBJS)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }
    
    if(varlist[1]->vt != VT_STR)
    {
        printf("Function gettextobjproperty must have a string property name.\n");
        goto gettextobjproperty_error;
    }
    propname = (char*)StrCache_Get(varlist[1]->strVal);//see what property it is

    propind = searchList(proplist, propname, _top_the_end);
    
    switch(propind)
    {
    case _top_font:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].font;
        break;
    }
    case _top_text:
    {
        ScriptVariant_ChangeType(*pretvar, VT_STR);
        strcpy(StrCache_Get((*pretvar)->strVal), level->textobjs[ind].text);
        break;
    }
    case _top_x:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].x;
        break;
    }
    case _top_y:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].y;
        break;
    }
    case _top_z:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->textobjs[ind].z;
        break;
    }
    default:
        printf("Property name '%s' is not supported by function gettextobjproperty.\n", propname);
        goto gettextobjproperty_error;
        break;
    }

    return S_OK;

gettextobjproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_changetextobjproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    char* propname = NULL;
    int propind;
    LONG ltemp;

    static const char* proplist[] = {
        "font",
        "text",
        "x",
        "y",
        "z",
    };
    
    typedef enum
    {
        _top_font,
        _top_text,
        _top_x,
        _top_y,
        _top_z,
        _top_the_end,
    } prop_enum;

    if(paramCount < 2)
        goto changetextobjproperty_error;


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: changetextobjproperty(int index, \"property\", value)\n");
        goto changetextobjproperty_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind >= LEVEL_MAX_TEXTOBJS)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }
    
    if(varlist[1]->vt != VT_STR)
    {
        printf("Function changetextobjproperty must have a string property name.\n");
        goto changetextobjproperty_error;
    }
    propname = (char*)StrCache_Get(varlist[1]->strVal);//see what property it is

    propind = searchList(proplist, propname, _top_the_end);
    
    switch(propind)
    {
    case _top_font:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->textobjs[ind].font = (int)ltemp;
        }
        break;
    }
    case _top_text:
    {
        if(varlist[2]->vt != VT_STR)
        {
            //printf("You must give a string value for textobj text.\n");
            goto changetextobjproperty_error;
        }
        if(!level->textobjs[ind].text)
        {
            level->textobjs[ind].text = (char*)tracemalloc("alloctextobj",MAX_STR_VAR_LEN);
        }
        strncpy(level->textobjs[ind].text, (char*)StrCache_Get(varlist[2]->strVal), MAX_STR_VAR_LEN);
        //level->textobjs[ind].text = (char*)StrCache_Get(varlist[2]->strVal);
        (*pretvar)->lVal = (LONG)1;
        break;
    }
    case _top_x:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->textobjs[ind].x = (int)ltemp;
        }
        break;
    }
    case _top_y:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->textobjs[ind].y = (int)ltemp;
        }
        break;
    }
    case _top_z:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->textobjs[ind].z = (int)ltemp;
        }
        break;
    }
    default:
        printf("Property name '%s' is not supported by function changetextobjproperty.\n", propname);
        goto changetextobjproperty_error;
        break;
    }

    return S_OK;

changetextobjproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_settextobj(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    LONG ltemp;

    if(paramCount < 5)
        goto settextobj_error;


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: settextobj(int index, int x, int y, int font, int z, int time, text)\n");
        goto settextobj_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind >= LEVEL_MAX_TEXTOBJS)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }

    if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
    {
        (*pretvar)->lVal = (LONG)1;
        level->textobjs[ind].x = (int)ltemp;
    }

    if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
    {
        (*pretvar)->lVal = (LONG)1;
        level->textobjs[ind].y = (int)ltemp;
    }
    
    if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[3], &ltemp)))
    {
        (*pretvar)->lVal = (LONG)1;
        level->textobjs[ind].font = (int)ltemp;
    }
    if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[4], &ltemp)))
    {
        (*pretvar)->lVal = (LONG)1;
        level->textobjs[ind].z = (int)ltemp;
    }
    if(varlist[5]->vt != VT_STR)
    {
        printf("You must give a string value for textobj text.\n");
        goto settextobj_error;
    }
    if(!level->textobjs[ind].text)
    {
        level->textobjs[ind].text = (char*)tracemalloc("alloctextobj",MAX_STR_VAR_LEN);
    }
    strncpy(level->textobjs[ind].text, (char*)StrCache_Get(varlist[5]->strVal), MAX_STR_VAR_LEN);
    (*pretvar)->lVal = (LONG)1;


    return S_OK;

settextobj_error:
    *pretvar = NULL;
    return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_cleartextobj(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;

    if(paramCount < 1)
        goto cleartextobj_error;


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: cleartextobj(int index)\n");
        goto cleartextobj_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind >= LEVEL_MAX_TEXTOBJS)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }

    level->textobjs[ind].x = 0;
    level->textobjs[ind].y = 0;
    level->textobjs[ind].font = 0;
    level->textobjs[ind].z = 0;
    if(level->textobjs[ind].text)
         tracefree(level->textobjs[ind].text);
    level->textobjs[ind].text = NULL;
    return S_OK;

cleartextobj_error:
    *pretvar = NULL;
    return E_FAIL;
}



//changelevelproperty(name, value)
HRESULT openbor_getbglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    char* propname = NULL;
    int propind;

    static const char* proplist[] = {
        "alpha",
        "amplitude",
        "bgspeedratio",
        "enabled",
        "transparency",
        "watermode",
        "wavelength",
        "wavespeed",
        "xoffset",
        "xratio",
        "xrepeat",
        "xspacing",
        "zoffset",
        "zratio",
        "zrepeat",
        "zspacing",
    };
    
    typedef enum
    {
        _bglp_alpha,
        _bglp_amplitude,
        _bglp_bgspeedratio,
        _bglp_enabled,
        _bglp_transparency,
        _bglp_watermode,
        _bglp_wavelength,
        _bglp_wavespeed,
        _bglp_xoffset,
        _bglp_xratio,
        _bglp_xrepeat,
        _bglp_xspacing,
        _bglp_zoffset,
        _bglp_zratio,
        _bglp_zrepeat,
        _bglp_zspacing,
        _bglp_the_end,
    } prop_enum;

    if(paramCount < 1)
        goto getbglayerproperty_error;


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: getbglayerproperty(int index, \"property\", value)\n");
        goto getbglayerproperty_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind >= level->numbglayers)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }
    
    if(varlist[1]->vt != VT_STR)
    {
        printf("Function getbglayerproperty must have a string property name.\n");
        goto getbglayerproperty_error;
    }
    propname = (char*)StrCache_Get(varlist[1]->strVal);//see what property it is

    propind = searchList(proplist, propname, _bglp_the_end);
    
    switch(propind)
    {
    case _bglp_alpha:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].alpha;
        break;
    }
    case _bglp_amplitude:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].amplitude;
        break;
    }
    case _bglp_bgspeedratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->bglayers[ind].bgspeedratio;
        break;
    }
    case _bglp_enabled:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].enabled;
        break;
    }
    case _bglp_transparency:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].transparency;
        break;
    }
    case _bglp_watermode:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].watermode;
        break;
    }

    case _bglp_wavelength:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].wavelength;
        break;
    }
    case _bglp_wavespeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->bglayers[ind].wavespeed;
        break;
    }
    case _bglp_xoffset:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].xoffset;
        break;
    }
    case _bglp_xratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->bglayers[ind].xratio;
        break;
    }
    case _bglp_xrepeat:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].xrepeat;
        break;
    }
    case _bglp_xspacing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].xspacing;
        break;
    }
    case _bglp_zoffset:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].zoffset;
        break;
    }
    case _bglp_zratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->bglayers[ind].zratio;
        break;
    }
    case _bglp_zrepeat:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].zrepeat;
        break;
    }
    case _bglp_zspacing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bglayers[ind].zspacing;
        break;
    }
    default:
        printf("Property name '%s' is not supported by function getbglayerproperty.\n", propname);
        goto getbglayerproperty_error;
        break;
    }

    return S_OK;

getbglayerproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_changebglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    char* propname = NULL;
    int propind;
    LONG ltemp;
    DOUBLE dbltemp;
    
    static const char* proplist[] = {
        "alpha",
        "amplitude",
        "bgspeedratio",
        "enabled",
        "transparency",
        "watermode",
        "wavelength",
        "wavespeed",
        "xoffset",
        "xratio",
        "xrepeat",
        "xspacing",
        "zoffset",
        "zratio",
        "zrepeat",
        "zspacing",
    };
    
    typedef enum
    {
        _bglp_alpha,
        _bglp_amplitude,
        _bglp_bgspeedratio,
        _bglp_enabled,
        _bglp_transparency,
        _bglp_watermode,
        _bglp_wavelength,
        _bglp_wavespeed,
        _bglp_xoffset,
        _bglp_xratio,
        _bglp_xrepeat,
        _bglp_xspacing,
        _bglp_zoffset,
        _bglp_zratio,
        _bglp_zrepeat,
        _bglp_zspacing,
        _bglp_the_end,
    } prop_enum;

    if(paramCount < 2)
        goto changebglayerproperty_error;


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: changebglayerproperty(int index, \"property\", value)\n");
        goto changebglayerproperty_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind >= level->numbglayers)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }
    
    if(varlist[1]->vt != VT_STR)
    {
        printf("Function changebglayerproperty must have a string property name.\n");
        goto changebglayerproperty_error;
    }
    propname = (char*)StrCache_Get(varlist[1]->strVal);//see what property it is

    propind = searchList(proplist, propname, _bglp_the_end);
    
    switch(propind)
    {
    case _bglp_alpha:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].alpha = (int)ltemp;
        }
        break;
    }
    case _bglp_amplitude:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].amplitude = (int)ltemp;
        }
        break;
    }
    case _bglp_bgspeedratio:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].bgspeedratio = (float)dbltemp;
        }
        break;
    }
    case _bglp_enabled:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].enabled = (int)ltemp;
        }
        break;
    }
    case _bglp_transparency:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].transparency = (int)ltemp;
        }
        break;
    }
    case _bglp_watermode:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].watermode = (int)ltemp;
        }
        break;
    }

    case _bglp_wavelength:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].wavelength = (int)ltemp;
        }
        break;
    }
    case _bglp_wavespeed:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].wavespeed = (float)dbltemp;
        }
        break;
    }
    case _bglp_xoffset:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].xoffset = (int)ltemp;
        }
        break;
    }
    case _bglp_xratio:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].xratio = (float)dbltemp;
        }
        break;
    }
    case _bglp_xrepeat:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].xrepeat = (int)ltemp;
        }
        break;
    }
    case _bglp_xspacing:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].xspacing = (int)ltemp;
        }
        break;
    }
    case _bglp_zoffset:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].zoffset = (int)ltemp;
        }
        break;
    }
    case _bglp_zratio:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].zratio = (float)dbltemp;
        }
        break;
    }
    case _bglp_zrepeat:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].zrepeat = (int)ltemp;
        }
        break;
    }
    case _bglp_zspacing:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->bglayers[ind].zspacing = (int)ltemp;
        }
        break;
    }
    default:
        printf("Property name '%s' is not supported by function changebglayerproperty.\n", propname);
        goto changebglayerproperty_error;
        break;
    }

    return S_OK;

changebglayerproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_getfglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    char* propname = NULL;
    int propind;

    static const char* proplist[] = {
        "alpha",
        "amplitude",
        "bgspeedratio",
        "enabled",
        "transparency",
        "watermode",
        "wavelength",
        "wavespeed",
        "xoffset",
        "xratio",
        "xrepeat",
        "xspacing",
        "zoffset",
        "zratio",
        "zrepeat",
        "zspacing",
    };
    
    typedef enum
    {
        _fglp_alpha,
        _fglp_amplitude,
        _fglp_bgspeedratio,
        _fglp_enabled,
        _fglp_transparency,
        _fglp_watermode,
        _fglp_wavelength,
        _fglp_wavespeed,
        _fglp_xoffset,
        _fglp_xratio,
        _fglp_xrepeat,
        _fglp_xspacing,
        _fglp_zoffset,
        _fglp_zratio,
        _fglp_zrepeat,
        _fglp_zspacing,
        _fglp_the_end,
    } prop_enum;

    if(paramCount < 1)
        goto getfglayerproperty_error;


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: getfglayerproperty(int index, \"property\", value)\n");
        goto getfglayerproperty_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind >= level->numfglayers)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }
    
    if(varlist[1]->vt != VT_STR)
    {
        printf("Function getfglayerproperty must have a string property name.\n");
        goto getfglayerproperty_error;
    }
    propname = (char*)StrCache_Get(varlist[1]->strVal);//see what property it is

    propind = searchList(proplist, propname, _fglp_the_end);
    
    switch(propind)
    {
    case _fglp_alpha:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].alpha;
        break;
    }
    case _fglp_amplitude:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].amplitude;
        break;
    }
    case _fglp_bgspeedratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->fglayers[ind].bgspeedratio;
        break;
    }
    case _fglp_enabled:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].enabled;
        break;
    }
    case _fglp_transparency:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].transparency;
        break;
    }
    case _fglp_watermode:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].watermode;
        break;
    }

    case _fglp_wavelength:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].wavelength;
        break;
    }
    case _fglp_wavespeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->fglayers[ind].wavespeed;
        break;
    }
    case _fglp_xoffset:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].xoffset;
        break;
    }
    case _fglp_xratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->fglayers[ind].xratio;
        break;
    }
    case _fglp_xrepeat:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].xrepeat;
        break;
    }
    case _fglp_xspacing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].xspacing;
        break;
    }
    case _fglp_zoffset:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].zoffset;
        break;
    }
    case _fglp_zratio:
    {
        ScriptVariant_ChangeType(*pretvar, VT_DECIMAL);
        (*pretvar)->dblVal = (DOUBLE)level->fglayers[ind].zratio;
        break;
    }
    case _fglp_zrepeat:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].zrepeat;
        break;
    }
    case _fglp_zspacing:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->fglayers[ind].zspacing;
        break;
    }
    default:
        printf("Property name '%s' is not supported by function getfglayerproperty.\n", propname);
        goto getfglayerproperty_error;
        break;
    }

    return S_OK;

getfglayerproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_changefglayerproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    char* propname = NULL;
    int propind;
    LONG ltemp;
    DOUBLE dbltemp;
    
    static const char* proplist[] = {
        "alpha",
        "amplitude",
        "bgspeedratio",
        "enabled",
        "transparency",
        "watermode",
        "wavelength",
        "wavespeed",
        "xoffset",
        "xratio",
        "xrepeat",
        "xspacing",
        "zoffset",
        "zratio",
        "zrepeat",
        "zspacing",
    };
    
    typedef enum
    {
        _fglp_alpha,
        _fglp_amplitude,
        _fglp_bgspeedratio,
        _fglp_enabled,
        _fglp_transparency,
        _fglp_watermode,
        _fglp_wavelength,
        _fglp_wavespeed,
        _fglp_xoffset,
        _fglp_xratio,
        _fglp_xrepeat,
        _fglp_xspacing,
        _fglp_zoffset,
        _fglp_zratio,
        _fglp_zrepeat,
        _fglp_zspacing,
        _fglp_the_end,
    } prop_enum;

    if(paramCount < 2)
        goto changefglayerproperty_error;


    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind)))
    {
        printf("Function's 1st argument must be a numeric value: changefglayerproperty(int index, \"property\", value)\n");
        goto changefglayerproperty_error;
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);

    if(ind<0 || ind >= level->numfglayers)
    {
        (*pretvar)->lVal = 0;
        return S_OK;
    }
    
    if(varlist[1]->vt != VT_STR)
    {
        printf("Function changefglayerproperty must have a string property name.\n");
        goto changefglayerproperty_error;
    }
    propname = (char*)StrCache_Get(varlist[1]->strVal);//see what property it is

    propind = searchList(proplist, propname, _fglp_the_end);
    
    switch(propind)
    {
    case _fglp_alpha:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].alpha = (int)ltemp;
        }
        break;
    }
    case _fglp_amplitude:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].amplitude = (int)ltemp;
        }
        break;
    }
    case _fglp_bgspeedratio:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].bgspeedratio = (float)dbltemp;
        }
        break;
    }
    case _fglp_enabled:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].enabled = (int)ltemp;
        }
        break;
    }
    case _fglp_transparency:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].transparency = (int)ltemp;
        }
        break;
    }
    case _fglp_watermode:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].watermode = (int)ltemp;
        }
        break;
    }

    case _fglp_wavelength:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].wavelength = (int)ltemp;
        }
        break;
    }
    case _fglp_wavespeed:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].wavespeed = (float)dbltemp;
        }
        break;
    }
    case _fglp_xoffset:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].xoffset = (int)ltemp;
        }
        break;
    }
    case _fglp_xratio:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].xratio = (float)dbltemp;
        }
        break;
    }
    case _fglp_xrepeat:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].xrepeat = (int)ltemp;
        }
        break;
    }
    case _fglp_xspacing:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].xspacing = (int)ltemp;
        }
        break;
    }
    case _fglp_zoffset:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].zoffset = (int)ltemp;
        }
        break;
    }
    case _fglp_zratio:
    {
        if(SUCCEEDED(ScriptVariant_DecimalValue(varlist[2], &dbltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].zratio = (float)dbltemp;
        }
        break;
    }
    case _fglp_zrepeat:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].zrepeat = (int)ltemp;
        }
        break;
    }
    case _fglp_zspacing:
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[2], &ltemp)))
        {
            (*pretvar)->lVal = (LONG)1;
            level->fglayers[ind].zspacing = (int)ltemp;
        }
        break;
    }
    default:
        printf("Property name '%s' is not supported by function changefglayerproperty.\n", propname);
        goto changefglayerproperty_error;
        break;
    }

    return S_OK;

changefglayerproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}

HRESULT openbor_getlevelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    char* propname = NULL;
    int propind;

    static const char* proplist[] = {
        "bgspeed",
		"cameraxoffset",
		"camerazoffset",
    };
    
    typedef enum
    {
        _lp_bgspeed,
		_lp_cameraxoffset,
		_lp_camerazoffset,
        _lp_the_end, // lol
    } prop_enum;



    if(varlist[0]->vt != VT_STR)
    {
        printf("Function getlevelproperty must have a string property name.\n");
        goto getlevelproperty_error;
    }


    propname = (char*)StrCache_Get(varlist[0]->strVal);//see what property it is

    propind = searchList(proplist, propname, _lp_the_end);
    
    switch(propind)
    {
    case _lp_bgspeed:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->bgspeed;
        break;
    }
	case _lp_cameraxoffset:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->cameraxoffset;
        break;
    }
	case _lp_camerazoffset:
    {
        ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
        (*pretvar)->lVal = (LONG)level->camerazoffset;
        break;
    }
    default:
        printf("Property name '%s' is not supported by function getlevelproperty.\n", propname);
        goto getlevelproperty_error;
        break;
    }

    return S_OK;

getlevelproperty_error:
    *pretvar = NULL;
    return E_FAIL;
}

//changelevelproperty(name, value)
HRESULT openbor_changelevelproperty(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    char* propname = NULL;
    LONG ltemp;
    ScriptVariant* arg = NULL;

    if(paramCount < 2)
    {
        *pretvar = NULL;
        return E_FAIL;
    }
    
    if(varlist[0]->vt != VT_STR)
    {
        *pretvar = NULL;
        return E_FAIL; 
    }

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    (*pretvar)->lVal = (LONG)1;

    propname = (char*)StrCache_Get(varlist[0]->strVal);

    arg = varlist[1];

    if(strcmp(propname, "rock")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            level->rocking = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
    }
    else if(strcmp(propname, "bgspeed")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            level->bgspeed = (float)ltemp;
        else (*pretvar)->lVal = (LONG)0;
    }
	else if(strcmp(propname, "cameraxoffset")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            level->cameraxoffset = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
    }
	else if(strcmp(propname, "camerazoffset")==0)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(arg, &ltemp)))
            level->camerazoffset = (int)ltemp;
        else (*pretvar)->lVal = (LONG)0;
    }

    return S_OK;
}

//jumptobranch(name, immediate)
HRESULT openbor_jumptobranch(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ltemp;   
    extern char branch_name[MAX_NAME_LEN+1];
    extern int  endgame;

    *pretvar = NULL;
    if(paramCount < 1) goto jumptobranch_error;
    if(varlist[0]->vt != VT_STR) goto jumptobranch_error;

    strncpy(branch_name, StrCache_Get(varlist[0]->strVal), MIN(MAX_NAME_LEN, MAX_STR_VAR_LEN)); // copy the string value to branch name

    if(paramCount >= 2)
    {
        if(SUCCEEDED(ScriptVariant_IntegerValue(varlist[1], &ltemp)))
        {
            endgame = (int)ltemp;
            // 1 means goto that level immediately, or, wait until the level is complete
        }
        else goto jumptobranch_error;
    }

    return S_OK;
jumptobranch_error:
    printf("Function requires 1 string value, the second argument is optional(int): jumptobranch(name, immediate)\n");
    return E_FAIL;
}

//bindentity(entity, target, x, z, a, direction, bindanim);
//bindentity(entity, NULL()); // unbind
HRESULT openbor_bindentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    entity* ent = NULL;
    entity* other = NULL;
    ScriptVariant* arg = NULL;
    void adjust_bind(entity* e);
    LONG x=0, z=0, a=0, dir=0, anim=0;

   *pretvar = NULL;
    if(paramCount < 2)
    {
        return E_FAIL; 
    }

    ent = (entity*)(varlist[0])->ptrVal; //retrieve the entity
    if(!ent)  return S_OK;

    other = (entity*)(varlist[1])->ptrVal;
    if(!other)  {ent->bound = NULL; return S_OK;}

    ent->bound = other;

    if(paramCount < 3) goto BIND;
    // x
    arg = varlist[2];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &x)))  return E_FAIL;
        ent->bindoffset[0] = (int)x;
    }
    if(paramCount < 4) goto BIND;
    // z
    arg = varlist[3];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &z)))  return E_FAIL;
        ent->bindoffset[1] = (int)z;
    }
    if(paramCount < 5) goto BIND;
    // a
    arg = varlist[4];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &a)))  return E_FAIL;
        ent->bindoffset[2] = (int)a;
    }
    if(paramCount < 6) goto BIND;
    // direction
    arg = varlist[5];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &dir)))  return E_FAIL;
        ent->bindoffset[3] = (int)dir;
    } 
    if(paramCount < 7) goto BIND;
    // animation
    arg = varlist[6];
    if(arg->vt != VT_EMPTY)
    {
        if(FAILED(ScriptVariant_IntegerValue(arg, &anim)))  return E_FAIL;
        ent->bindanim = (int)anim;
    }
    
BIND:
    adjust_bind(ent);

    return S_OK;
}

//allocscreen(int w, int h);
HRESULT openbor_allocscreen(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG w, h;
    s_screen* screen;

    if(paramCount < 2) goto allocscreen_error;

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &w)))
        goto allocscreen_error;
    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &h)))
        goto allocscreen_error;
        
    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    screen = allocscreen((int)w, (int)h, screenformat);
    if(screen) clearscreen(screen);
    (*pretvar)->ptrVal = (VOID*)screen;

    if((*pretvar)->ptrVal==NULL)
    {
        printf("Not enough memory: allocscreen(%d, %d)\n", (int)w, (int)h);
        (*pretvar) = NULL;
        return E_FAIL;
    }
    List_InsertAfter(&scriptheap, (void*)((*pretvar)->ptrVal), "openbor_allocscreen");
    return S_OK;

allocscreen_error:
    printf("Function requires 2 int values: allocscreen(int width, int height)\n");
    (*pretvar) = NULL;
    return E_FAIL;
}

//setdrawmethod(entity, int flag, int scalex, int scaley, int flipx, int flipy, int shiftx, int alpha, int remap, int fillcolor, int rotate, int fliprotate, int transparencybg, void* colourmap);
HRESULT openbor_setdrawmethod(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG value[12];
    entity* e;
    s_drawmethod *pmethod;
    int i;

    *pretvar = NULL;
    if(paramCount<2) goto setdrawmethod_error;
    
    if(varlist[0]->vt==VT_EMPTY) e = NULL;
    else if(varlist[0]->vt==VT_PTR) e = (entity*)varlist[0]->ptrVal;
    else goto setdrawmethod_error;

    if(e) pmethod = &(e->drawmethod);
    else  pmethod = &(drawmethod);
    
    memset(value, 0, sizeof(LONG)*12);
    for(i=1; i<paramCount && i<13; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i], value+i-1))) goto setdrawmethod_error;
    }

    if(paramCount>=14 && varlist[13]->vt!=VT_PTR && varlist[13]->vt!=VT_EMPTY) goto setdrawmethod_error;

    pmethod->flag = (int)value[0];
    pmethod->scalex = (int)value[1];
    pmethod->scaley = (int)value[2];
    pmethod->flipx = (int)value[3];
    pmethod->flipy = (int)value[4];
    pmethod->shiftx = (int)value[5];
    pmethod->alpha = (int)value[6];
    pmethod->remap = (int)value[7];
    pmethod->fillcolor = (int)value[8];
    pmethod->rotate = ((int)value[9])%360;
    pmethod->fliprotate = (int)value[10];
    pmethod->transbg = (int)value[11];
    if(paramCount>=14) pmethod->table=(unsigned char*)varlist[13]->ptrVal;

    if(pmethod->rotate)  
    {
        if(pmethod->rotate<0) pmethod->rotate += 360;
    }
    return S_OK;

setdrawmethod_error:
    printf("Function need a valid entity handle and at least 1 interger parameter, all other parameters should be integers: setdrawmethod(entity, int flag, int scalex, int scaley, int flipx, int flipy, int shiftx, int alpha, int remap, int fillcolor, int rotate, int fliprotate, int transparencybg, void* colourmap)\n");
    return E_FAIL;
}

//updateframe(entity, int frame);
HRESULT openbor_updateframe(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG f;
    entity* e;
    void update_frame(entity* ent, int f);

    *pretvar = NULL;
    if(paramCount<2) goto updateframe_error;
    
    if(varlist[0]->vt==VT_EMPTY) e = NULL;
    else if(varlist[0]->vt==VT_PTR) e = (entity*)varlist[0]->ptrVal;
    else goto updateframe_error;

    if(!e) goto updateframe_error;

    if(FAILED(ScriptVariant_IntegerValue(varlist[1], &f))) goto updateframe_error;

    update_frame(e, (int)f);
    
    return S_OK;

updateframe_error:
    printf("Function need a valid entity handle and at an interger parameter: updateframe(entity, int frame)\n");
    return E_FAIL;
}

//performattack(entity, int anim, int resetable);
HRESULT openbor_performattack(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG anim, resetable=0;
    entity* e;

    *pretvar = NULL;
    if(paramCount<1) goto performattack_error;
    
    if(varlist[0]->vt==VT_EMPTY) e = NULL;
    else if(varlist[0]->vt==VT_PTR) e = (entity*)varlist[0]->ptrVal;
    else goto performattack_error;

    if(!e) goto performattack_error;
    
    e->takeaction = common_attack_proc;
    e->attacking = 1;
    e->idling = 0;
    e->drop = 0;
    e->falling = 0;
    e->inpain = 0;
    e->blocking = 0;
    
    if(paramCount==1) return S_OK;

    if(paramCount>1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim))) goto performattack_error;
    if(paramCount>2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &resetable)))  goto performattack_error;
    ent_set_anim(e, (int)anim, (int)resetable);

    return S_OK;

performattack_error:
    printf("Function need a valid entity handle, the other 2 integer parameters are optional: performattack(entity, int anim, int resetable)\n");
    return E_FAIL;
}

//setidle(entity, int anim, int resetable, int stalladd);
HRESULT openbor_setidle(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG anim, resetable=0, stalladd;
    entity* e;
    extern unsigned long time;

    *pretvar = NULL;
    if(paramCount<1) goto setidle_error;
    
    if(varlist[0]->vt==VT_EMPTY) e = NULL;
    else if(varlist[0]->vt==VT_PTR) e = (entity*)varlist[0]->ptrVal;
    else goto setidle_error;

    if(!e) goto setidle_error;
    
    e->takeaction = NULL;
    e->attacking = 0;
    e->idling = 1;
    e->drop = 0;
    e->falling = 0;
    e->inpain = 0;
    e->blocking = 0;
	e->nograb = 0;
    
    if(paramCount==1) return S_OK;

    if(paramCount>1 && FAILED(ScriptVariant_IntegerValue(varlist[1], &anim))) goto setidle_error;
    if(paramCount>2 && FAILED(ScriptVariant_IntegerValue(varlist[2], &resetable)))  goto setidle_error;
    if(paramCount>3 && FAILED(ScriptVariant_IntegerValue(varlist[3], &stalladd)))  goto setidle_error;
    ent_set_anim(e, (int)anim, (int)resetable);
    
    if(stalladd>0) e->stalltime = time+stalladd;

    return S_OK;

setidle_error:
    printf("Function need a valid entity handle, the other 2 integer parameters are optional: setidle(entity, int anim, int resetable, int stalladd)\n");
    return E_FAIL;
}

//getentity(int index_from_list)
HRESULT openbor_getentity(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG ind;
    extern entity* ent_list[MAX_ENTS];

    if(paramCount!=1) goto getentity_error;

    if(FAILED(ScriptVariant_IntegerValue(varlist[0], &ind))) goto getentity_error;

    ScriptVariant_Clear(*pretvar);

    if((int)ind<MAX_ENTS && (int)ind>=0)
    {
        ScriptVariant_ChangeType(*pretvar, VT_PTR);
        (*pretvar)->ptrVal = (VOID*)ent_list[(int)ind];
    }
    //else, it should return an empty value
    return S_OK;

getentity_error:
    printf("Function need an integer parameter: getentity(int index_in_list)\n");
    *pretvar = NULL;
    return E_FAIL;
}


//loadmodel(name)
HRESULT openbor_loadmodel(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
	LONG unload = 0;
    if(paramCount<1) goto loadmodel_error;
    if(varlist[0]->vt!=VT_STR) goto loadmodel_error;

     ScriptVariant_ChangeType(*pretvar, VT_PTR);
	 if(paramCount == 2)
		 if(FAILED(ScriptVariant_IntegerValue(varlist[1], &unload))) goto loadmodel_error;

    (*pretvar)->ptrVal = (VOID*)load_cached_model(StrCache_Get(varlist[0]->strVal), "openbor_loadmodel", (char)unload);
    //else, it should return an empty value
    return S_OK;

loadmodel_error:
    printf("Function needs a string and boolean parameters: loadmodel(name, unload)\n");
    ScriptVariant_Clear(*pretvar);
    *pretvar = NULL;
    return E_FAIL;
}

// load a sprite which doesn't belong to the sprite_cache
// loadsprite(path)
HRESULT openbor_loadsprite(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    extern s_sprite * loadsprite2(char *filename, int* width, int* height);
    if(paramCount!=1) goto loadsprite_error;

    if(varlist[0]->vt!=VT_STR) goto loadsprite_error;

    ScriptVariant_ChangeType(*pretvar, VT_PTR);
    if(((*pretvar)->ptrVal = (VOID*)loadsprite2(StrCache_Get(varlist[0]->strVal), NULL, NULL)))
    {
        List_InsertAfter(&scriptheap, (void*)((*pretvar)->ptrVal), "openbor_loadsprite");
    }
    //else, it should return an empty value
    return S_OK;

loadsprite_error:
    printf("Function need a string parameter: loadsprite(path)\n");
    ScriptVariant_Clear(*pretvar);
    *pretvar = NULL;
    return E_FAIL;
}

//playgif(path, int x, int y, int noskip)
HRESULT openbor_playgif(ScriptVariant** varlist , ScriptVariant** pretvar, int paramCount)
{
    LONG temp[3] = {0,0,0}; //x,y,noskip
    int i;
    extern unsigned char pal[1024];
    extern int playgif(char *filename, int x, int y, int noskip);
    if(paramCount<1) goto playgif_error;

    if(varlist[0]->vt!=VT_STR) goto playgif_error;

    ScriptVariant_ChangeType(*pretvar, VT_INTEGER);
    for(i=0; i<3 && i<paramCount-1; i++)
    {
        if(FAILED(ScriptVariant_IntegerValue(varlist[i+1], temp+i))) goto playgif_error;
    }
    (*pretvar)->lVal = (LONG)playgif(StrCache_Get(varlist[0]->strVal), (int)(temp[0]), (int)(temp[1]), (int)(temp[2]));
    palette_set_corrected(pal, savedata.gamma,savedata.gamma,savedata.gamma, savedata.brightness,savedata.brightness,savedata.brightness);
    return S_OK;

playgif_error:
    printf("Function need a string parameter, other parameters are optional: playgif(path, int x, int y, int noskip)\n");
    *pretvar = NULL;
    return E_FAIL;
}
