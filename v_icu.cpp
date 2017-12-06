/*
 * v_icu.cpp
 *
 *  Created on: Nov 30, 2017
 *      Author: vidok
 */
#ifdef _WIN
  #include <windows.h>
#else
  #include <stdio.h>
  #include <string.h>
  #define V_ICU_API extern "C"
  typedef  unsigned int DWORD;

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wwrite-strings"

#endif
/**
 * icu include
 */
#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <unicode/brkiter.h>
#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <unicode/locid.h>
#include <unicode/ustring.h>
#include <unicode/ucnv.h>
#include <unicode/regex.h>

#include <map>
#include <ctime>
#include <cstdlib>

#include "normalizer.h"

/**
 *  Elastic declare
 */
typedef struct Binary_Methods_STRUC {
    char                  *Name;
    void                  (*Method)(int **Adr, int NumOfOpr, DWORD *DataSeg);
}Binary_Methods_STRUC;

typedef struct ELAST_FUNC {
    char     *Name;
    void    (*Func)();
}ELAST_FUNC;

typedef int (*GetFuncOffFT)(DWORD *Body, char *FuncName);
typedef int (*CallFuncFT)(DWORD *Body, DWORD  FileInd, DWORD Off, int **Parms, int ParmCount);
typedef int (*GetFileIndFT)();
typedef char *(*ralloc_chFT)(char **Str, int Size);
typedef void (*rrealloc_chFT)(char **Str, int Size);
typedef void (*rfree_chFT)(char **Str);

typedef DWORD *(*ConstructProc)();
GetFuncOffFT  GetFuncOff    = NULL;
CallFuncFT    CallFuncOff   = NULL;
GetFileIndFT  GetFileIndOff = NULL;
ralloc_chFT   ralloc_ch     = NULL;
rrealloc_chFT rrealloc_ch   = NULL;
rfree_chFT    rfree_ch      = NULL;
int           GetInitialNumberOff;

typedef enum icuProcedures {
	DO_TOUP,
	DO_TOLOWER
}icuProcedures;

typedef enum normalizeConstants {
	NFC,
	NFD,
	NFKC,
	NFKD,
	NFKCCF
}normalizeConstants;

#define MYERROR_START -100000
#define EMPTY_STRING -1
#define LAST_STRING -2

typedef enum myErrorCode {
	LOCALE_BOGUS = MYERROR_START+1,
	SPLITBUFFER_INDEX_ALREADY_EXISTS = MYERROR_START+2,
	SPLITBUFFER_OUT_OF_MEMORY = MYERROR_START+3,
	SPLITBUFFER_UNDEFINED_ERROR = MYERROR_START+4,
	SPLITBUFFER_UNDEFINED_ID = MYERROR_START+5,
	SPLITBUFFER_STRING_DOES_NOT_EXISTS = MYERROR_START + 6,
	INPUT_STRING_IS_EMPTY = MYERROR_START + 7,
	INPUT_REGEXP_IS_EMPTY = MYERROR_START + 8
}myErrorCode;

std::map<int, UnicodeString**> splitBuffer;

class V_Icu{
public:
	V_Icu();
	int getLastError();
	const char* getErrorName();
	UErrorCode errorCode;
	std::map<int, const char*> errorName;
};

void returnErrorInt(DWORD *DataSeg) {
	DataSeg[0] = -1;
	DataSeg[3] = -1;
	return;
}

void returnErrorStr(DWORD *DataSeg) {
	char *output;
	asm("pushad");
	ralloc_ch(&output, 1);
	asm("popad");
	output[0]  = '\0';
	DataSeg[0] = 0;
	DataSeg[1] = (DWORD)output;
	DataSeg[3] = -1;
	return;
}

UErrorCode prepareSplitBuffer(int id, int capacity) {
	std::map<int, UnicodeString**>::const_iterator got = splitBuffer.find (id);
	if(got != splitBuffer.end()) {
		return (UErrorCode)SPLITBUFFER_INDEX_ALREADY_EXISTS;
	}
	try {
		/**
		 * last element for set last
		 */
		UnicodeString **stringlist = new UnicodeString * [capacity + 1];
		for(int i=0; i<capacity; i++) {
			stringlist[i] = (UnicodeString*)EMPTY_STRING;
		}
		stringlist[capacity] = (UnicodeString*)LAST_STRING;
		splitBuffer[id] = stringlist;
	}
	catch(std::bad_alloc& ba) {
		return (UErrorCode)SPLITBUFFER_OUT_OF_MEMORY;
	}
	catch(...) {
		return (UErrorCode)SPLITBUFFER_UNDEFINED_ERROR;
	}
	return U_ZERO_ERROR;
}

bool isWholeWord(BreakIterator *wordBrk,
                   const UnicodeString& s,
                   int32_t start,
                   int32_t end) {
    wordBrk->setText(s);
    if (!wordBrk->isBoundary(start))
        return false;

    int32_t p = wordBrk->current();
    while (p < end && p != -1)
        p = wordBrk->next();

    return p == end;
}

UErrorCode listWordBoundaries(const UnicodeString& s, int id, int capacity) {
    UErrorCode status = U_ZERO_ERROR;
    Locale locale;
    BreakIterator* bi = BreakIterator::createWordInstance(locale, status);
    if(U_FAILURE(status)) {
    	return status;
    }
    if(locale.isBogus() == TRUE){
    	return (UErrorCode)LOCALE_BOGUS;
    }
    status = prepareSplitBuffer(id, capacity);
    if(status != U_ZERO_ERROR) {
    	return status;
    }
    bi->setText(s);
    int32_t p = bi->first();
    int prev = 0;
    int counter = 0;
    UnicodeString **stringlist = splitBuffer[id];
    while (p != BreakIterator::DONE && counter < capacity) {
        //printf("Boundary at position %d\n", p);
        p = bi->next();
        if(isWholeWord(bi, s, prev, p)) {
        	UnicodeString string = UnicodeString(s, prev, p - prev);
        	int c = (int)string.getBuffer()[0];
        	if(!(u_isspace(c)
        			|| u_charType(c) == U_CONTROL_CHAR
                    || u_charType(c) == U_NON_SPACING_MARK)) {
				stringlist[counter++] = new UnicodeString(string);
				char tmp[100];
				(stringlist[counter - 1])->extract(0, (stringlist[counter - 1])->length(), tmp, sizeof(tmp));
				//printf("- %s (%d, %d)\n", tmp, prev, p);
        	}
        }
        prev = p;
    }
    delete bi;
}

/**
 * return string
 */
void commonToUpToLower(int **Adr, int NumOfOpr, DWORD *DataSeg, icuProcedures proc) {
	DWORD **Src;
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	/**
	 * reset error
	 */
	Class->errorCode = U_ZERO_ERROR;
	Src    = (DWORD**)Adr[4+0];
	char *string = (char *)Src[1];
	UnicodeString *s = new UnicodeString(string);
	switch(proc) {
	case DO_TOUP:
		s->toUpper();
		break;
	case DO_TOLOWER:
		s->toLower();
		break;
	default:
		return returnErrorStr(DataSeg);
	}
	/**
	 * 4 - because UTF-8 can consists of four bites
	 */
	int destCapacity = s->length() * 4;
	char *tmp = new char[destCapacity];
	s->extract(tmp, destCapacity, NULL, Class->errorCode);
	if(Class->errorCode != U_ZERO_ERROR) {
		return returnErrorStr(DataSeg);
	}
	char *output;
	asm("pushad");
	ralloc_ch(&output, destCapacity);
	asm("popad");
	strcpy(output, tmp);
	/**
	 * free tmp
	 */
	delete [] tmp;
	tmp = 0;
	DataSeg[0] = strlen(output);
	DataSeg[1] = (DWORD)(output);
}

void V_Icu_ToUp(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	commonToUpToLower(Adr, NumOfOpr, DataSeg, DO_TOUP);
}

void V_Icu_ToLow(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	commonToUpToLower(Adr, NumOfOpr, DataSeg, DO_TOLOWER);
}

/**
 * return string
 */
void normalizeCommon(int **Adr, int NumOfOpr, DWORD *DataSeg, normalizeConstants nc) {
	DWORD **Src;
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	/**
	 * reset error
	 */
	Class->errorCode = U_ZERO_ERROR;
	Src    = (DWORD**)Adr[4+0];
	char *string = (char *)Src[1];
	UnicodeString *s = new UnicodeString(string);
	NormSample *normSample = new NormSample(Class->errorCode);
	UnicodeString result;

	/**
	 * Use name="nfc" and UNORM2_COMPOSE/UNORM2_DECOMPOSE for Unicode standard NFC/NFD.
	 * Use name="nfkc" and UNORM2_COMPOSE/UNORM2_DECOMPOSE for Unicode standard NFKC/NFKD.
	 * Use name="nfkc_cf" and UNORM2_COMPOSE for Unicode standard NFKC_CF=NFKC_Casefold.
	 */
	switch(nc) {
	case NFC:
		result = normSample->processText(*s, "nfc", UNORM2_COMPOSE, Class->errorCode);
		break;
	case NFD:
		result = normSample->processText(*s, "nfc", UNORM2_DECOMPOSE, Class->errorCode);
		break;
	case NFKC:
		result = normSample->processText(*s, "nfkc", UNORM2_COMPOSE, Class->errorCode);
		break;
	case NFKD:
		result = normSample->processText(*s, "nfkc", UNORM2_DECOMPOSE, Class->errorCode);
		break;
	case NFKCCF:
		result = normSample->processText(*s, "nfkc_cf", UNORM2_COMPOSE, Class->errorCode);
		break;
	default:
		DataSeg[1] = (DWORD)"";
		DataSeg[0] = -1;
		return;
	}

	if(U_FAILURE(Class->errorCode)) {
		return returnErrorStr(DataSeg);
	}
	/**
	 * 4 - because UTF-8 can consists of four bites
	 */
	int destCapacity = result.length() * 4;
	char *tmp = new char[destCapacity];
	result.extract(tmp, destCapacity, NULL, Class->errorCode);
	if(Class->errorCode != U_ZERO_ERROR) {
		return returnErrorStr(DataSeg);
	}
	char *output;
	asm("pushad");
	ralloc_ch(&output, destCapacity);
	asm("popad");
	strcpy(output, tmp);
	/**
	 * free new
	 */
	delete [] tmp;
	tmp = 0;
	delete s;
	s = 0;
	delete normSample;
	normSample = 0;
	/**
	 * return
	 */
	DataSeg[0] = strlen(output);
	DataSeg[1] = (DWORD)(output);
}

void V_Icu_NormalizeNFC(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	normalizeCommon(Adr, NumOfOpr, DataSeg, NFC);
}

void V_Icu_NormalizeNFD(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	normalizeCommon(Adr, NumOfOpr, DataSeg, NFD);
}

void V_Icu_NormalizeNFKC(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	normalizeCommon(Adr, NumOfOpr, DataSeg, NFKC);
}

void V_Icu_NormalizeNFKD(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	normalizeCommon(Adr, NumOfOpr, DataSeg, NFKD);
}

void V_Icu_NormalizeNFKCCF(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	normalizeCommon(Adr, NumOfOpr, DataSeg, NFKCCF);
}

/**
 * split unicode string by whitespace
 * return int
 */
void V_Icu_SplitSimple(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	DWORD **Src, **Delimeter, **Int;
	Src = (DWORD**)Adr[4+0];
	Delimeter = (DWORD**)Adr[4+1];
	Int = (DWORD**)Adr[4+2];
	char *string = (char *)Src[1];
	char *delimeter = (char *)Delimeter[1];
	int maxWords = (int)Int[0];
	UnicodeString *s = new UnicodeString(string);
	UErrorCode status = U_ZERO_ERROR;
	RegexMatcher m(UnicodeString(delimeter), 0, status);
	UnicodeString *words;
	try {
		words = new UnicodeString [maxWords];
	}
	catch(std::bad_alloc& ba) {
		fprintf(stderr, "bad_alloc caught: %s\n", ba.what());
		DataSeg[3] = -1;
	}
	int numWords = m.split(*s, words, maxWords, status);
	for(int i=0; i<numWords; i++) {
		char tmp[100];
		words[i].extract(0, words[i].length(), tmp, sizeof(tmp));
		printf("%s\n", tmp);
	}
	delete [] words;
	DataSeg[3] = 0;
}

/**
 * return int
 */
void V_Icu_SplitUnicodeString(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	DWORD **Str, **Id, **Capacity;
	char *string;
	int id, capacity;
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	Str = (DWORD **)Adr[4+0];
	Id = (DWORD **)Adr[4+1];
	Capacity = (DWORD **)Adr[4+2];
	string = (char *)Str[1];
	id = (int)Id[0];
	capacity = (int)Capacity[0];
	UErrorCode status = listWordBoundaries(UnicodeString(string), id, capacity);
	if(U_FAILURE(status)) {
		Class->errorCode = status;
		return returnErrorInt(DataSeg);
	}
	DataSeg[3] = 0;
}

/**
 * return string
 */
void V_Icu_GetSplitStringByIndex(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	DWORD **Id, **Index;
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	Id = (DWORD **)Adr[4+0];
	Index = (DWORD **)Adr[4+1];
	int id, index;
	id = (int)Id[0];
	index = (int)Index[0];
	std::map<int, UnicodeString**>::const_iterator got = splitBuffer.find (id);
	if(got == splitBuffer.end()) {
		Class->errorCode = (UErrorCode)SPLITBUFFER_UNDEFINED_ID;
		return returnErrorStr(DataSeg);
	}
	UnicodeString **stringlist = splitBuffer[id];
	UnicodeString *s = stringlist[index];
	if(s == (UnicodeString*)-1) {
		Class->errorCode = (UErrorCode)SPLITBUFFER_STRING_DOES_NOT_EXISTS;
		return returnErrorStr(DataSeg);
	}
	int destCapacity = s->length() * 4;
	char *tmp = new char[destCapacity];
	s->extract(tmp, destCapacity, NULL, Class->errorCode);
	if(Class->errorCode != U_ZERO_ERROR) {
		return returnErrorStr(DataSeg);
	}
	char *output;
	asm("pushad");
	ralloc_ch(&output, destCapacity);
	asm("popad");
	strcpy(output, tmp);
	delete [] tmp;
	tmp = 0;
	/**
	 * return
	 */
	DataSeg[0] = strlen(output);
	DataSeg[1] = (DWORD)(output);
}

/**
 * return int
 */
void V_Icu_GetSplitBufferSize(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	DWORD **Id;
	Id = (DWORD **)Adr[4+0];
	int id;
	id = (int)Id[0];
	std::map<int, UnicodeString**>::const_iterator got = splitBuffer.find (id);
	if(got == splitBuffer.end()) {
		Class->errorCode = (UErrorCode)SPLITBUFFER_UNDEFINED_ID;
		return returnErrorInt(DataSeg);
	}
	UnicodeString **stringlist = splitBuffer[id];
	int i=0;
	while((stringlist[i] != (UnicodeString*)EMPTY_STRING)
		&&(stringlist[i] != (UnicodeString*)LAST_STRING)) {
		i++;
	}
	Class->errorCode = U_ZERO_ERROR;
	DataSeg[3] = i;
}

/**
 * return int
 */
void V_Icu_SplitBufferFree(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	DWORD **Id;
	Id = (DWORD **)Adr[4+0];
	int id;
	id = (int)Id[0];
	std::map<int, UnicodeString**>::const_iterator got = splitBuffer.find (id);
	if(got == splitBuffer.end()) {
		Class->errorCode = (UErrorCode)SPLITBUFFER_UNDEFINED_ID;
		return returnErrorInt(DataSeg);
	}
	UnicodeString **stringlist = splitBuffer[id];
	splitBuffer.erase(id);
	int i=0;
	while((stringlist[i] != (UnicodeString*)EMPTY_STRING)
		&&(stringlist[i] != (UnicodeString*)LAST_STRING)) {
		delete stringlist[i];
		stringlist[i] = 0;
		i++;
	}
	delete [] stringlist;
	stringlist = 0;
	DataSeg[3] = 0;
}

/**
 * return int
 */
void V_Icu_GetSplitBufferRandomId(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	int random_integer = rand();
	std::map<int, UnicodeString**>::const_iterator got = splitBuffer.find (random_integer);
	while(got != splitBuffer.end()) {
		random_integer = rand();
		got = splitBuffer.find (random_integer);
	}
	DataSeg[3] = random_integer;
}

/**
 * return int
 */
void V_Icu_RegexUnicodeString(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	DWORD **Str, **RegexpStr, **Id;
	char *string, *regexpString;
	int id;
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	Str = (DWORD **)Adr[4+0];
	RegexpStr = (DWORD **)Adr[4+1];
	Id = (DWORD **)Adr[4+2];
	string = (char *)Str[1];
	regexpString = (char *)RegexpStr[1];
	id = (int)Id[0];
	UErrorCode status = U_ZERO_ERROR;
	UnicodeString ustring = UnicodeString(string);
	UnicodeString uregexp = UnicodeString(regexpString);
	if(ustring.length() == 0) {
		Class->errorCode = (UErrorCode)INPUT_STRING_IS_EMPTY;
		return returnErrorInt(DataSeg);
	}
	if(uregexp.length() == 0) {
		Class->errorCode = (UErrorCode)INPUT_REGEXP_IS_EMPTY;
		return returnErrorInt(DataSeg);
	}
	int capacity = ustring.length();
	status = prepareSplitBuffer(id, capacity);
	if(status != U_ZERO_ERROR) {
		Class->errorCode = status;
		return returnErrorInt(DataSeg);
	}
	UnicodeString **stringlist = splitBuffer[id];
	RegexMatcher *matcher = new RegexMatcher(uregexp, 0, status);
	if (U_FAILURE(status)) {
		Class->errorCode = status;
		return returnErrorInt(DataSeg);
	}
	matcher->reset(ustring);
	int counter = 0;
	while (matcher->find()) {
	   // We found a match.
	   int startOfMatch = matcher->start(status);
	   int endOfMatch = matcher->end(status);
	   if(U_FAILURE(status)) {
		   Class->errorCode = status;
		   return returnErrorInt(DataSeg);
	   }
	   stringlist[counter++] = new UnicodeString(ustring, startOfMatch, endOfMatch - startOfMatch);
	}
	DataSeg[3] = 0;
}

/**
 * return int
 */
void V_Icu_GetLastError(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	DataSeg [3] = Class->getLastError();
}

/**
 * return string
 */
void V_Icu_GetLastErrorName(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	const char* err = Class->getErrorName();
	char *output;
	asm("pushad");
	ralloc_ch(&output, strlen(err));
	asm("popad");
	strcpy(output, err);
	DataSeg [0] = strlen(output);
	DataSeg [1] = (DWORD)(output);
}

// Вызывается из elastic при создании его класса
DWORD* V_Icu_Construct()
{
	V_Icu *p;
	p = new V_Icu();
	return (DWORD*) p;
}

const char *Tbl[] = {"V_Icu"};

/**
 * Так мы определились, сделайте пока разбивку на слова, нормализацию и lowercase
 * Служебное
 * Описываем параметры вызова
 */
Binary_Methods_STRUC V_Icu_STRUC_Methods[] =
{
	"string ptr ToUp  string ptr", &V_Icu_ToUp,
	"string ptr ToLow string ptr", &V_Icu_ToLow,
	/**
	 * Use name="nfc" and UNORM2_COMPOSE/UNORM2_DECOMPOSE for Unicode standard NFC/NFD.
	 * Use name="nfkc" and UNORM2_COMPOSE/UNORM2_DECOMPOSE for Unicode standard NFKC/NFKD.
	 * Use name="nfkc_cf" and UNORM2_COMPOSE for Unicode standard NFKC_CF=NFKC_Casefold.
	 */
	"string ptr NormalizeNFC string ptr", &V_Icu_NormalizeNFC,
	"string ptr NormalizeNFD string ptr", &V_Icu_NormalizeNFD,
	"string ptr NormalizeNFKC string ptr", &V_Icu_NormalizeNFKC,
	"string ptr NormalizeNFKD string ptr", &V_Icu_NormalizeNFKD,
	"string ptr NormalizeNFKCCF string ptr", &V_Icu_NormalizeNFKCCF,
	"int SplitSimple string ptr string ptr int ptr", &V_Icu_SplitSimple,
	"int SplitUnicodeString string ptr int ptr int ptr", &V_Icu_SplitUnicodeString,
	"string ptr GetSplitStringByIndex int ptr int ptr", &V_Icu_GetSplitStringByIndex,
	"int SplitBufferFree int ptr", &V_Icu_SplitBufferFree,
	"int GetSplitBufferSize int ptr", &V_Icu_GetSplitBufferSize,
	"int GetSplitBufferRandomId", &V_Icu_GetSplitBufferRandomId,
	"int RegexUnicodeString string ptr string ptr int ptr", &V_Icu_RegexUnicodeString,
	"int GetLastErrorIcu", &V_Icu_GetLastError,
	"string ptr GetLastErrorNameIcu", &V_Icu_GetLastErrorName,
	NULL
};

Binary_Methods_STRUC *MethodStrucTable[] = {
    V_Icu_STRUC_Methods,
    NULL
};

V_Icu::V_Icu() : errorCode(U_ZERO_ERROR)
{
	errorName[LOCALE_BOGUS] = "Error on locale creating";
	errorName[SPLITBUFFER_INDEX_ALREADY_EXISTS] = "Error index already exists";
	errorName[SPLITBUFFER_OUT_OF_MEMORY] = "Error out of memory during buffer prepare";
	errorName[SPLITBUFFER_UNDEFINED_ERROR] = "Error undefined error during buffer prepare";
	errorName[SPLITBUFFER_UNDEFINED_ID] = "Error buffer undefined id";
	errorName[SPLITBUFFER_STRING_DOES_NOT_EXISTS] = "Error requested string does not exists";
}

int V_Icu::getLastError() {
	return errorCode;
}

const char* V_Icu::getErrorName() {
	std::map<int, const char *>::const_iterator got = errorName.find(errorCode);
	if(got != errorName.end()) {
		return got->second;
	}
	return u_errorName(errorCode);
}

V_ICU_API ConstructProc GetConstructor(int i){
    printf("GetConstructor:%d\n",i);
    return V_Icu_Construct;
}

// Вызывается по загрузке .so
V_ICU_API Binary_Methods_STRUC  **Init(const char ***Names, ELAST_FUNC *FuncTable){
	srand((unsigned)time(0));
	Names[0] = Tbl;
    for(int i=0; ; i++){
        if(FuncTable[i].Name == NULL)break;
        if(strcmp(FuncTable[i].Name, "GetFuncOff") == 0){
           GetFuncOff = (GetFuncOffFT)FuncTable[i].Func;
        }else if(strcmp(FuncTable[i].Name, "CallFunc") == 0){
           CallFuncOff = (CallFuncFT)FuncTable[i].Func;
        }else if(strcmp(FuncTable[i].Name, "GetFileInd") == 0){
           GetFileIndOff = (GetFileIndFT)FuncTable[i].Func;
        }else if(strcmp(FuncTable[i].Name, "alloc_ch") == 0){
           ralloc_ch         = (ralloc_chFT)FuncTable[i].Func;
        }else if(strcmp(FuncTable[i].Name, "free_ch") == 0){
           rfree_ch          = (rfree_chFT)FuncTable[i].Func;
        }else if(strcmp(FuncTable[i].Name, "realloc_ch") == 0){
           rrealloc_ch          = (rrealloc_chFT)FuncTable[i].Func;
        }
    }
    return MethodStrucTable;
}
