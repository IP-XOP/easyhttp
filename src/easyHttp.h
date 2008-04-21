
#include "XOPStandardHeaders.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>
 
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include "memutils.h"
#include "kvp.h"

#define MAX_URL_LENGTH 2048
#define MAX_PASSLEN 200

#define REQUIRES_IGOR_500 1 + FIRST_XOP_ERR
#define ERR_KEY_VAL 2 + FIRST_XOP_ERR
#define WAVE_TOO_SHORT 3 + FIRST_XOP_ERR
#define COULDNT_LOCK_FILE 4 + FIRST_XOP_ERR

/* Prototypes */
#ifdef _MACINTOSH_
HOST_IMPORT int main(IORecHandle ioRecHandle);
#endif
#ifdef _WINDOWS_
HOST_IMPORT void main(IORecHandle ioRecHandle);
#endif


using namespace std;

#include "XOPStructureAlignmentTwoByte.h"	// All structures passed to Igor are two-byte aligned.
struct easyHttpRuntimeParams {
	// Flag parameters.

	// Parameters for /AUTH flag group.
	int AUTHFlagEncountered;
	Handle AUTHFlagStrH;
	int AUTHFlagParamsSet[1];

	// Parameters for /PASS flag group.
	int PASSFlagEncountered;
	Handle PASSFlagStrH;
	int PASSFlagParamsSet[1];

	// Parameters for /FILE flag group.
	int FILEFlagEncountered;
	Handle FILEFlagStrH;
	int FILEFlagParamsSet[1];

	// Parameters for /PROXY flag group.
	int PROXYFlagEncountered;
	Handle PROXYFlagStrH;
	int PROXYFlagParamsSet[1];

	// Parameters for /POST flag group.
	int POSTFlagEncountered;
	Handle POSTFlagStrH;
	int POSTFlagParamsSet[1];

	// Parameters for /UPLOAD flag group.
	int FTPFlagEncountered;
	Handle FTPFlagStrH;
	int FTPFlagParamsSet[1];

	// Main parameters.

	// Parameters for URL keyword group.
	int main0Encountered;
	Handle main0StrH;
	int main0ParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct easyHttpRuntimeParams easyHttpRuntimeParams;
typedef struct easyHttpRuntimeParams* easyHttpRuntimeParamsPtr;
#include "XOPStructureAlignmentReset.h"		// Reset structure alignment to default.

int ExecuteEasyHTTP(easyHttpRuntimeParamsPtr p);
int RegisterEasyHTTP(void);

#include "XOPStructureAlignmentTwoByte.h"	// All structures passed to Igor are two-byte aligned.
struct THReasyHTTPRuntimeParams {
	// Flag parameters.

	// Parameters for /AUTH flag group.
	int AUTHFlagEncountered;
	Handle AUTHFlagStrH;
	int AUTHFlagParamsSet[1];

	// Parameters for /PASS flag group.
	int PASSFlagEncountered;
	Handle PASSFlagStrH;
	int PASSFlagParamsSet[1];

	// Parameters for /PROXY flag group.
	int PROXYFlagEncountered;
	Handle PROXYFlagStrH;
	int PROXYFlagParamsSet[1];

	// Parameters for /POST flag group.
	int POSTFlagEncountered;
	Handle POSTFlagStrH;
	int POSTFlagParamsSet[1];

	// Parameters for /FTP flag group.
	int FTPFlagEncountered;
	Handle FTPFlagStrH;
	int FTPFlagParamsSet[1];

	// Main parameters.

	// Parameters for simple main group #0.
	int main0Encountered;
	Handle main0StrH;
	int main0ParamsSet[1];

	// Parameters for simple main group #1.
	int main1Encountered;
	waveHndl main1WaveH;
	int main1ParamsSet[1];

	// Parameters for simple main group #2.
	int main2Encountered;
	Handle main2StrH;
	int main2ParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
	struct tag_UserFuncThreadInfo *tp;		// If not null, we are running from a ThreadSafe function.
};
typedef struct THReasyHTTPRuntimeParams THReasyHTTPRuntimeParams;
typedef struct THReasyHTTPRuntimeParams* THReasyHTTPRuntimeParamsPtr;
#include "XOPStructureAlignmentReset.h"		// Reset structure alignment to default.

int ExecuteTHReasyHTTP(THReasyHTTPRuntimeParamsPtr p);
int RegisterTHReasyHTTP(void);

static size_t write_filedata(void *ptr, size_t size, size_t nmemb, void *stream);
      
static size_t write_filedata(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}