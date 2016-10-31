
#include "XOPStandardHeaders.h"


#define MAX_URL_LENGTH 2048
#define MAX_PASSLEN 200

#define REQUIRES_IGOR_700 1 + FIRST_XOP_ERR
#define ERR_KEY_VAL 2 + FIRST_XOP_ERR
#define WAVE_TOO_SHORT 3 + FIRST_XOP_ERR
#define COULDNT_LOCK_FILE 4 + FIRST_XOP_ERR

/* Prototypes */
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);

#pragma pack(2)	// All structures passed to Igor are two-byte aligned.
struct easyHttpPreferences {				// this is a structure to save preferences for 
	char proxyURLandPort[MAX_URL_LENGTH + 1];				// Where to get input value, where to store output value.
	char proxyUserNameandPassword[MAX_PASSLEN + 1];
};
typedef struct easyHttpPreferences easyHttpPreferences;
typedef struct easyHttpPreferences *easyHttpPreferencesPtr;
typedef struct easyHttpPreferences **easyHttpPreferencesHandle;
#pragma pack()		// Reset structure alignment to default.


#pragma pack(2)	// All structures passed to Igor are two-byte aligned.
struct easyHttpRuntimeParams {
	// Flag parameters.
	
	// Parameters for /S flag group.
	/* this flag signifies that you want to save the proxy settings (both URL:port and username:password) in the preferences file */
	int SFlagEncountered;
	// There are no fields for this group because it has no parameters.
	
	//should the XOP be quiet as it downloads?
	int VERBFlagEncountered;

	int TIMEFlagEncountered;
	double TIMEFlagNumber;
	int TIMEFlagParamsSet[1];

	// Parameters for /PASS flag group.
	int PASSFlagEncountered;
	Handle PASSFlagStrH;
	int PASSFlagParamsSet[1];

	// Parameters for /FILE flag group.
	int FILEFlagEncountered;
	Handle FILEFlagStrH;
	int FILEFlagParamsSet[1];

	// Parameters for /PROX flag group.
	int PROXFlagEncountered;
	Handle PROXFlagStrH;
	int PROXFlagParamsSet[1];
	
	// Parameters for /PPAS flag group.
	int PPASFlagEncountered;
	Handle PPASFlagStrH;
	int PPASFlagParamsSet[1];

	// Parameters for /POST flag group.
	int POSTFlagEncountered;
	Handle POSTFlagStrH;
	int POSTFlagParamsSet[1];

	// Parameters for /UPLOAD flag group.
	int FTPFlagEncountered;
	Handle FTPFlagStrH;
	int FTPFlagParamsSet[1];
    
    // Parameters for /FORM flag group.
	int FORMFlagEncountered;
	waveHndl FORMFlagWaveH;
	int FORMFlagParamsSet[1];

	// Main parameters.

	// Parameters for URL keyword group.
	int main0Encountered;
	Handle main0StrH;
	int main0ParamsSet[1];

	// Parameters for simple main group #1.
	int main1Encountered;
	char main1VarName[MAX_OBJ_NAME+1];		// Optional parameter.
	int main1ParamsSet[1];	

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
	UserFunctionThreadInfoPtr tp;	
};
typedef struct easyHttpRuntimeParams easyHttpRuntimeParams;
typedef struct easyHttpRuntimeParams* easyHttpRuntimeParamsPtr;
#pragma pack()		// Reset structure alignment to default.

int ExecuteEasyHTTP(easyHttpRuntimeParamsPtr p);
int RegisterEasyHTTP(void);


static size_t write_filedata(void *ptr, size_t size, size_t nmemb, void *stream);
      
static size_t write_filedata(void *ptr, size_t size, size_t nmemb, void *stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}