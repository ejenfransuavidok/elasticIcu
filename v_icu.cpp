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


class V_Icu{
public:
	V_Icu();
	int getLastError();
	const char* getErrorName();
	UErrorCode errorCode;
};

void allocAndCopyStr(char *dest, char *src, int length) {
	asm("pushad");
	ralloc_ch(&dest, length);
	asm("popad");
	strcpy(dest, src);
}

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
		/**
		 * here exit because is error
		 */
		DataSeg[0] = 0;
		DataSeg[1] = -1;
		return;
	}
	/**
	 * 4 - because UTF-8 can consists of four bites
	 */
	int destCapacity = s->length() * 4;
	char *tmp = new char[destCapacity];
	s->extract(tmp, destCapacity, NULL, Class->errorCode);
	if(Class->errorCode != U_ZERO_ERROR) {
	/**
	 * here exit because is error
	 */
		DataSeg[0] = 0;
		DataSeg[1] = -1;
		return;
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
		DataSeg[1] = (DWORD)"";
		DataSeg[0] = -1;
		return;
	}
	/**
	 * 4 - because UTF-8 can consists of four bites
	 */
	int destCapacity = result.length() * 4;
	char *tmp = new char[destCapacity];
	result.extract(tmp, destCapacity, NULL, Class->errorCode);
	if(Class->errorCode != U_ZERO_ERROR) {
	/**
	 * here exit because is error
	 */
		DataSeg[0] = 0;
		DataSeg[1] = -1;
		return;
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
	RegexMatcher m(delimeter, 0, status);
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
		UErrorCode errorCode;
		words[i].extract(0, words[i].length(), tmp, sizeof(tmp));
		printf("%s\n", tmp);
	}
	delete [] words;
	DataSeg[3] = 0;
}

void V_Icu_GetLastError(int **Adr, int NumOfOpr, DWORD *DataSeg) {
	V_Icu *Class  = (V_Icu*)Adr[2][2];
	DataSeg [3] = Class->getLastError();
}

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
	"int GetLastError", &V_Icu_GetLastError,
	"string ptr GetLastErrorName", &V_Icu_GetLastErrorName,
	NULL
};

Binary_Methods_STRUC *MethodStrucTable[] = {
    V_Icu_STRUC_Methods,
    NULL
};

V_Icu::V_Icu() : errorCode(U_ZERO_ERROR)
{
}

int V_Icu::getLastError() {
	return errorCode;
}

const char* V_Icu::getErrorName() {
	return u_errorName(errorCode);
}

V_ICU_API ConstructProc GetConstructor(int i){
    printf("GetConstructor:%d\n",i);
    return V_Icu_Construct;
}

// Вызывается по загрузке .so
V_ICU_API Binary_Methods_STRUC  **Init(const char ***Names, ELAST_FUNC *FuncTable){
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
