// Runtime param structure for GetHTTP operation.
#include "easyHttp.h"
 
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
  {
    /* in real-world cases, this would probably get this data differently
       as this fread() stuff is exactly what the library already would do
       by default internally */
    size_t retcode = fread(ptr, size, nmemb, (XOP_FILE_REF) stream);
    return retcode;
  }

static size_t WriteMemoryCallbackWithHandle(void *ptr, size_t size, size_t nmemb, void *Data)
{
    size_t realsize = size * nmemb;
    Handle *HandPtr = (Handle *)Data;

	if(PtrAndHand(ptr, *HandPtr, realsize))
		return -1;
	else
		return realsize;

}



int
ExecuteEasyHTTP(easyHttpRuntimeParamsPtr p)
{
	int err = 0;
	CURL *curl = NULL;
	CURLcode res;
	extern easyHttpPreferencesHandle thePreferences;
	int prefsState = 0;
	char pathName[MAX_PATH_LEN + 1];
	char pathNameToWrite[MAX_PATH_LEN + 1];
	char pathNameToRead[MAX_PATH_LEN + 1];
	char userpassword[MAX_PASSLEN + 1];
	char proxyUserPassword[MAX_PASSLEN + 1];
	XOP_FILE_REF inputFile = NULL;
	XOP_FILE_REF outputFile = NULL;
	char curlerror[CURL_ERROR_SIZE + 1];
   	string url;
	string postString;
	
	MemoryStruct chunk;
	
	if( igorVersion < 600 )
		return REQUIRES_IGOR_600;
  
	if( igorVersion < 620 && !RunningInMainThread())
		return NOT_IN_THREADSAFE;

	/* init the curl session */
	curl= curl_easy_init();
	if( !curl)
		goto done;
	
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerror);
    
    //if you are in a multithread environment getting signals can really screw things up.
    //http://curl.haxx.se/mail/lib-2013-05/0108.html
    //
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1)
    
	
	/* The URL of interest */
	if (p->main0Encountered) {
		// Parameter: p->urlStrH (test for NULL handle before using)
		if (p->main0StrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		url.assign(*(p->main0StrH), GetHandleSize(p->main0StrH));
 		//Specify the URL to get
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	}

	if (p->main1Encountered) {
		if (p->main1ParamsSet[0]) {
			// Optional parameter: p->main1VarName
			int dataTypePtr;
			if(err = VarNameToDataType(p->main1VarName, &dataTypePtr))
				goto done;
			if(dataTypePtr){
				err = OH_EXPECTED_VARNAME;
				goto done;
			}
		}
	}

	// Flag parameters.
	
	if (p->AUTHFlagEncountered) {
		// Parameter: p->AUTHFlagStrH (test for NULL handle before using)
		if (p->AUTHFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
	}
	
	/* for proxies
	1) see if there is a proxy specified, command line always gets preference.
	2) if there isn't then see if the IGOR preferences have one
	*/
	if(RunningInMainThread())
		if(err = GetPrefsState(&prefsState))
			goto done;
	
	if(p->PROXFlagEncountered){
		if (p->PROXFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		url.assign(*(p->PROXFlagStrH), GetHandleSize(p->PROXFlagStrH));
		curl_easy_setopt(curl, CURLOPT_PROXY, url.c_str());

		//you want to save the proxy in the IGOR preferences file
		//you can only do this if running in the main thread.
		if(p->SFlagEncountered && RunningInMainThread()){
			//if the preferences handle doesn't exist we have to create it.
			if(!thePreferences){
				thePreferences = (easyHttpPreferencesHandle) NewHandle(sizeof(easyHttpPreferences));
				if(!thePreferences){
					err = MemError();
					goto done;
				}
				memset(*thePreferences, 0, GetHandleSize((Handle) thePreferences));
			}
			//just check if the preferences handle is changed in size.
			if(GetHandleSize((Handle) thePreferences) != sizeof(easyHttpPreferences)){
				SetHandleSize((Handle) thePreferences, sizeof(easyHttpPreferences));
				if(err = MemError())
					goto done;
				memset(*thePreferences, 0, GetHandleSize((Handle) thePreferences));
			}
			//now put the proxy into the preferences handle
			memset((*thePreferences)->proxyURLandPort, 0, sizeof((*thePreferences)->proxyURLandPort));
			strncpy((*thePreferences)->proxyURLandPort, url.c_str(), sizeof((*thePreferences)->proxyURLandPort));
		}
	} else if(thePreferences && prefsState){
		if(strlen((*thePreferences)->proxyURLandPort))
			curl_easy_setopt(curl, CURLOPT_PROXY, (*thePreferences)->proxyURLandPort);
	}
	
	if(p->PPASFlagEncountered){
		if (p->PPASFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->PPASFlagStrH, proxyUserPassword, MAX_PASSLEN))
			goto done;
		curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxyUserPassword);
		
		//you want to save the proxy in the IGOR preferences file
		if(p->SFlagEncountered && RunningInMainThread()){
			//if the preferences handle doesn't exist we have to create it.
			if(!thePreferences){
				thePreferences = (easyHttpPreferencesHandle) NewHandle(sizeof(easyHttpPreferences));
				if(thePreferences== NULL){
					err = MemError();
					goto done;
				}
				memset(*thePreferences, 0, GetHandleSize((Handle) thePreferences));
			}
			//just check if the preferences handle is changed in size.
			if(GetHandleSize((Handle) thePreferences) != sizeof(easyHttpPreferences)){
				SetHandleSize((Handle) thePreferences, sizeof(easyHttpPreferences));
				if(err = MemError())
					goto done;
				memset(*thePreferences, 0, GetHandleSize((Handle) thePreferences));
			}
			
			//now put the proxy into the preferences handle
			memset((*thePreferences)->proxyUserNameandPassword, 0, sizeof((*thePreferences)->proxyUserNameandPassword));
			strncpy((*thePreferences)->proxyUserNameandPassword, proxyUserPassword, sizeof((*thePreferences)->proxyUserNameandPassword));
		}
	} else if(thePreferences && prefsState){
		if(strlen((*thePreferences)->proxyUserNameandPassword))
			curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, (*thePreferences)->proxyUserNameandPassword);
	}
	
	
	/* For Authentication */
	if (p->PASSFlagEncountered) {
		// Parameter: p->PASSFlagStrH (test for NULL handle before using)
		if (p->PASSFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->PASSFlagStrH, userpassword, MAX_PASSLEN))
			goto done;
		//set a user name and password for authentication
		curl_easy_setopt(curl, CURLOPT_USERPWD, userpassword);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	}
	
	if(p->POSTFlagEncountered){
		if (p->POSTFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		postString.assign(*(p->POSTFlagStrH), (int) GetHandleSize(p->POSTFlagStrH));
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postString.c_str());
	}	
		
	if(p->FTPFlagEncountered){
		if (p->FTPFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->FTPFlagStrH,pathName, MAX_PATH_LEN))
			goto done;	
		if(err = GetNativePath(pathName,pathNameToRead))
			goto done;
		if(err = XOPOpenFile(pathNameToRead,0,&inputFile))
			goto done;
		UInt32 numBytes = 0;
		if(err = XOPNumberOfBytesInFile(inputFile, &numBytes))
			goto done;

		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1) ;
		curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t) numBytes);
		curl_easy_setopt(curl, CURLOPT_READDATA, inputFile);
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
	}
	
	/* Do you want to save the file to disc? */
	if (p->FILEFlagEncountered && !p->FTPFlagEncountered) {
		// Parameter: p->FILEFlagStrH (test for NULL handle before using)
		if (p->FILEFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->FILEFlagStrH, pathName, MAX_PATH_LEN))
			goto done;
		if(err = GetNativePath(pathName, pathNameToWrite))
			goto done;
		if(err = XOPOpenFile(pathNameToWrite, 1, &outputFile))
			goto done;
		/* send all data to this function  */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_filedata);
		/*send in the file*/
		curl_easy_setopt(curl,CURLOPT_WRITEDATA ,outputFile);
		 
	} else {
		/*send all data to this function*/
		size_t (*writeBack)(void*, size_t, size_t,void*) = (MemoryStruct::WriteMemoryCallback);
		
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBack);
		/* we pass our 'chunk' struct to the callback function */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	}

	/* some servers don't like requests that are made without a user-agent
	field, so we provide one */
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	
	//follow redirects
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	
	
	//set a timeout
	if(p->TIMEFlagEncountered)
		if(p->TIMEFlagNumber > 0)
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)p->TIMEFlagNumber);

	try{
		res = curl_easy_perform(curl);
	} catch (bad_alloc&){
		err = NOMEM;
	}
	
	if(res)
		goto done;

	//if not in a file put into a string handle
	if (!p->FILEFlagEncountered && chunk.getData()){
		if(p->main1ParamsSet[0]){
			if(err = StoreStringDataUsingVarName(p->main1VarName, (const char*)chunk.getData(), chunk.getMemSize()))
				goto done;
		}else if (!err && (err = SetOperationStrVar2("S_getHttp", (const char*)chunk.getData(), chunk.getMemSize())))
			goto done;
	}
	
done:
	if((err || res)){
		 SetOperationNumVar("V_flag", 1);
		 SetOperationStrVar("S_value", curlerror);	
	} else {
		err = SetOperationNumVar("V_flag", 0);
		err = SetOperationStrVar("S_Value", "");
	}

	if(curl)
		//always cleanup
		curl_easy_cleanup(curl);

	if (outputFile != NULL) 
		err = XOPCloseFile(outputFile);

	if (inputFile != NULL) 
		err = XOPCloseFile(inputFile);
	
	return err;
}

int
RegisterEasyHTTP(void)
{
	char* cmdTemplate;
	char* runtimeNumVarList;
	char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the easyHttpRuntimeParams structure as well.
	cmdTemplate = "easyHTTP/S/VERB/TIME=number/auth=string/pass=string/file=string/prox=string/ppas=string/post=string/ftp=string string[,varname]";
	runtimeNumVarList = "V_Flag";
	runtimeStrVarList = "S_getHttp;S_error";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(easyHttpRuntimeParams), (void*)ExecuteEasyHTTP, kOperationIsThreadSafe);
}
