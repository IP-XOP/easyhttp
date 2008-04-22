// Runtime param structure for THReasyHTTP operation.
#include "easyHTTP.h"
#include <sys/file.h>

int
ExecuteTHReasyHTTP(THReasyHTTPRuntimeParamsPtr p)
{
	int err = 0;
	CURL *curl = NULL;
	CURLcode res;
   	char url[MAX_URL_LENGTH+1];
	char pathName[MAX_PATH_LEN+1];
	char pathNameToWrite[MAX_PATH_LEN+1];
	char userpassword[MAX_PASSLEN+1];
	char postfields[4097];
	
	Handle hand = NULL;
	
	long dimensionSizes[MAX_DIMENSIONS+1]; // Array of new dimension sizes 
	long indices[MAX_DIMENSIONS]; // Identifies the point of interest 
	double value[2];
	
    struct curl_slist *headerlist=NULL;
	
	XOP_FILE_REF outputFile = NULL;
	char curlerror[CURL_ERROR_SIZE+1];
	
	struct MemoryStruct chunk;
	chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
    chunk.size = 0;    /* no data at this point */
	
	if( igorVersion < 602 )
		return REQUIRES_IGOR_500;
	if( igorVersion >= 700)
		return XOP_OBSOLETE;

	curl_global_init(CURL_GLOBAL_ALL);
  
	/* init the curl session */
	curl= curl_easy_init();
	if( !curl)
		goto done;
	
	curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,curlerror);
	
	if(p->main1Encountered){
		if(p->main1WaveH == NULL){
			err = NOWAV;
			goto done;
		}
		memset(dimensionSizes,0,sizeof(long)*(MAX_DIMENSIONS+1));
		
		if(WaveType(p->main1WaveH) == TEXT_WAVE_TYPE){
			err = EXPECTED_NUMERIC_WAVE;
			goto done;
		}
		if(WavePoints(p->main1WaveH)==0){
			err = WAVE_TOO_SHORT;
			goto done;
		}
	}
		
	/* The URL of interest */
	if (p->main0Encountered) {
		// Parameter: p->urlStrH (test for NULL handle before using)
		if (p->main0StrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->main0StrH,url,MAX_URL_LENGTH))
			goto done;
 		//Specify the URL to get
		curl_easy_setopt(curl, CURLOPT_URL, url);
	}
	
	// Flag parameters.
	if (p->AUTHFlagEncountered) {
		// Parameter: p->AUTHFlagStrH (test for NULL handle before using)
		if (p->AUTHFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
	}
	
	/* for proxies*/
	if(p->PROXFlagEncountered){
		if (p->PROXFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->PROXFlagStrH,url,MAX_URL_LENGTH))
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
	
	if(p->POSTFlagEncountered){
		if (p->POSTFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->POSTFlagStrH,postfields,4096))
			goto done;
		 
		 curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
		 curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(postfields));
	}	
		
	if(p->FTPFlagEncountered){
		if (p->FTPFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->FTPFlagStrH,pathName,MAX_PATH_LEN))
			goto done;	
		if(err = GetNativePath(pathName,pathNameToWrite))
			goto done;
		if(err = XOPOpenFile(pathNameToWrite,0,&outputFile))
			goto done;
		
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1) ;
		curl_easy_setopt(curl, CURLOPT_READDATA, outputFile);
	}	

	/* Do you want to save the file to disc? */
	if (p->main2Encountered && !p->FTPFlagEncountered) {
		// Parameter: p->FILEFlagStrH (test for NULL handle before using)
		if (p->main2StrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->main2StrH,pathName,MAX_PATH_LEN))
			goto done;
		if(err = GetNativePath(pathName,pathNameToWrite))
			goto done;
		if(err = XOPOpenFile(pathNameToWrite,1,&outputFile))
			goto done;
		
		#ifdef _MACINTOSH_
		//lock the file for this thread
		if(err = ftrylockfile(outputFile)){
			err = COULDNT_LOCK_FILE;
			goto done;
		}
		#endif
		
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
	
	//if you haven't had errors resize the wave to 1 row
	//if there is no file flag then put the memory in it.
	//it there is a file flag then leave it empty
	if(!(res || err)){
		memset(indices,0,sizeof(indices));
		value[0]=0;
		value[1]=0;
		if(err = MDSetNumericWavePointValue(p->main1WaveH,indices,value))
			goto done;
	}
	
		
done:
	if(err || res){
		memset(indices,0,sizeof(indices));
		value[0]=1;
		value[1]=1;
		MDSetNumericWavePointValue(p->main1WaveH,indices,value);
	}
	
	if(chunk.memory)
		free(chunk.memory);
		
	if(hand)
		DisposeHandle(hand);

	if(curl){
		//always cleanup
		curl_easy_cleanup(curl);
	}

	if(headerlist)
      curl_slist_free_all (headerlist);
 
 
	if (outputFile != NULL){
		err = XOPCloseFile(outputFile);
		#ifdef _MACINTOSH_
		funlockfile(outputFile);
		#endif
	}
	/* cleanup libcurl*/
	curl_global_cleanup();
	
	return err;
}

int
RegisterTHReasyHTTP(void)
{
	char* cmdTemplate;
	char* runtimeNumVarList;
	char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the THReasyHTTPRuntimeParams structure as well.
	cmdTemplate = "THReasyHTTP/auth=string/pass=string/proxy=string/post=string/ftp=string string, wave,string";
	runtimeNumVarList = "";
	runtimeStrVarList = "";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(THReasyHTTPRuntimeParams), (void*)ExecuteTHReasyHTTP, kOperationIsThreadSafe);
}