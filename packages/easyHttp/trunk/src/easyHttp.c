// Runtime param structure for GetHTTP operation.
#include "XOPStandardHeaders.h"

#include "easyHttp.h"

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

	// Parameters for /FILE flag group.
	int PROXYFlagEncountered;
	Handle PROXYFlagStrH;
	int PROXYFlagParamsSet[1];
	
	// Main parameters.

	// Parameters for url keyword group.
	int urlEncountered;
	Handle urlStrH;
	int urlParamsSet[1];

	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct easyHttpRuntimeParams easyHttpRuntimeParams;
typedef struct easyHttpRuntimeParams* easyHttpRuntimeParamsPtr;
#include "XOPStructureAlignmentReset.h"		// Reset structure alignment to default.
 
static int
ExecuteEasyHTTP(easyHttpRuntimeParamsPtr p)
{
	int err = 0;
	CURL *curl = NULL;
	CURLcode res;
   	char url[MAX_URL_LENGTH+1];
	char pathName[MAX_PATH_LEN+1];
	char pathNameToWrite[MAX_PATH_LEN+1];
	char userpassword[MAX_PASSLEN+1];
	XOP_FILE_REF outputFile = NULL;
	char curlerror[CURL_ERROR_SIZE+1];
	
	struct MemoryStruct chunk;
	chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
    chunk.size = 0;    /* no data at this point */
	
	if( igorVersion < 503 )
		return REQUIRES_IGOR_500;

	curl_global_init(CURL_GLOBAL_ALL);
  
	/* init the curl session */
	curl= curl_easy_init();
	if( !curl)
		goto done;
	
	curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,curlerror);
	
	// Flag parameters.
	
	if (p->AUTHFlagEncountered) {
		// Parameter: p->AUTHFlagStrH (test for NULL handle before using)
		if (p->AUTHFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
	}
	
	/* for proxies*/
	if(p->PROXYFlagEncountered){
		if (p->PROXYFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->PROXYFlagStrH,url,MAX_URL_LENGTH))
			goto done;
		curl_easy_setopt(curl,CURLOPT_PROXY,url);
	}
	
	/* For Authentication */
	if (p->PASSFlagEncountered) {
		// Parameter: p->PASSFlagStrH (test for NULL handle before using)
		if (p->PASSFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->PASSFlagStrH,userpassword,MAX_PASSLEN))
			goto done;
		//set a user name and password for authentication
		curl_easy_setopt(curl,CURLOPT_USERPWD,userpassword);
		curl_easy_setopt(curl,CURLOPT_HTTPAUTH,CURLAUTH_ANY);
	}
	
	/* The URL of interest */
	if (p->urlEncountered) {
		// Parameter: p->urlStrH (test for NULL handle before using)
		if (p->urlStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->urlStrH,url,MAX_URL_LENGTH))
			goto done;
 		//Specify the URL to get
		curl_easy_setopt(curl, CURLOPT_URL, url);
	}
	
	/* Do you want to save the file to disc? */
	if (p->FILEFlagEncountered) {
		// Parameter: p->FILEFlagStrH (test for NULL handle before using)
		if (p->FILEFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->FILEFlagStrH,pathName,MAX_PATH_LEN))
			goto done;
		if(err = GetNativePath(pathName,pathNameToWrite))
			goto done;
		if(err = XOPOpenFile(pathNameToWrite,1,&outputFile))
			goto done;
		/* send all data to this function  */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_filedata);
		/*send in the file*/
		curl_easy_setopt(curl,CURLOPT_WRITEDATA ,outputFile);
		 
	} else {
		/*send all data to this function*/
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		/* we pass our 'chunk' struct to the callback function */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	}

	/* some servers don't like requests that are made without a user-agent
	field, so we provide one */
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	
	//follow redirects
	curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
	
	/*get it*/
	if(res = curl_easy_perform(curl)){
		XOPNotice("easyHTTP error: ");
		XOPNotice(curlerror);
		XOPNotice("\r");
		goto done;
	}

	//if not in a file put into a string handle
	if (!p->FILEFlagEncountered){
		//data may not be null terminated
//		myrealloc(chunk.memory,chunk.size+sizeof(char));
//		chunk.size += 1;
//		*(chunk.memory + chunk.size-1) = (char)"\0";
		if(err = SetOperationStrVar("S_getHttp",chunk.memory))
			goto done;
	}
		
done:
	if((err | res)){
		 SetOperationNumVar("V_flag",1);
	} else {
		err = SetOperationNumVar("V_flag",0);
	}

	if(chunk.memory)
		free(chunk.memory);

	if(curl){
		//always cleanup
		curl_easy_cleanup(curl);
	}
	if (outputFile != NULL) 
		err = XOPCloseFile(outputFile);
	
	/* cleanup libcurl*/
	curl_global_cleanup();
	
	return err;
}

static int
RegisterEasyHTTP(void)
{
	char* cmdTemplate;
	char* runtimeNumVarList;
	char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the easyHttpRuntimeParams structure as well.
	cmdTemplate = "easyHTTP/auth=string/pass=string/file=string/proxy=string string";
	runtimeNumVarList = "V_Flag";
	runtimeStrVarList = "S_getHttp";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(easyHttpRuntimeParams), (void*)ExecuteEasyHTTP, 0);
}

static int
RegisterOperations(void)		// Register any operations with Igor.
{
	int result;
	
	// Register XOP1 operation.
	if (result = RegisterEasyHTTP())
		return result;
	
	// There are no more operations added by this XOP.
	
	return 0;
}

/*	XOPEntry()

This is the entry point from the host application to the XOP for all
messages after the INIT message.
*/
static void
XOPEntry(void)
{	
	long result = 0;
	
	switch (GetXOPMessage()) {
		// We don't need to handle any messages for this XOP.
	}
	SetXOPResult(result);
}

/*	main(ioRecHandle)

This is the initial entry point at which the host application calls XOP.
The message sent by the host must be INIT.

main does any necessary initialization and then sets the XOPEntry field of the
ioRecHandle to the address to be called for future messages.
*/
HOST_IMPORT void
main(IORecHandle ioRecHandle){
	int result;
	
	XOPInit(ioRecHandle);							// Do standard XOP initialization.
	
	SetXOPEntry(XOPEntry);							// Set entry point for future calls.
	
	if (result = RegisterOperations()) {
		SetXOPResult(result);
		return;
	}
	
	SetXOPResult(0);
}
