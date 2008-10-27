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
 
int
ExecuteEasyHTTP(easyHttpRuntimeParamsPtr p)
{
	int err = 0;
	CURL *curl = NULL;
	CURLcode res;
   	char url[MAX_URL_LENGTH+1];
	char pathName[MAX_PATH_LEN+1];
	char pathNameToWrite[MAX_PATH_LEN+1];
	char pathNameToRead[MAX_PATH_LEN+1];
	char userpassword[MAX_PASSLEN+1];

	XOP_FILE_REF inputFile = NULL;
	XOP_FILE_REF outputFile = NULL;
	char curlerror[CURL_ERROR_SIZE+1];
	
	MemoryStruct chunk;
	
	if( igorVersion < 503 )
		return REQUIRES_IGOR_500;

	curl_global_init(CURL_GLOBAL_ALL);
  
	/* init the curl session */
	curl= curl_easy_init();
	if( !curl)
		goto done;
	
	curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,curlerror);
	
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
		if(err = GetCStringFromHandle(p->POSTFlagStrH,pathName,MAX_PATH_LEN))
			goto done;	
		if(err = GetNativePath(pathName,pathNameToRead))
			goto done;
		if(err = XOPOpenFile(pathNameToRead,0,&inputFile))
			goto done;
		 
		curl_easy_setopt(curl, CURLOPT_READDATA,inputFile);
		curl_easy_setopt(curl, CURLOPT_POST,1L);
	}	
		
	if(p->FTPFlagEncountered){
		if (p->FTPFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->FTPFlagStrH,pathName,MAX_PATH_LEN))
			goto done;	
		if(err = GetNativePath(pathName,pathNameToRead))
			goto done;
		if(err = XOPOpenFile(pathNameToRead,0,&inputFile))
			goto done;
		unsigned long numBytes = 0;
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
		size_t (*writeBack)(void*, size_t, size_t,void*) = (MemoryStruct::WriteMemoryCallback);
		
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBack);
		/* we pass our 'chunk' struct to the callback function */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	}

	/* some servers don't like requests that are made without a user-agent
	field, so we provide one */
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	
	//follow redirects
	curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);
	
	/*get it*/
	try{
		if(res = curl_easy_perform(curl)){
			XOPNotice("easyHTTP error: ");
			XOPNotice(curlerror);
			XOPNotice("\r");
			goto done;
		}
	} catch (bad_alloc&){
		err = NOMEM;
		goto done;
	}
	
	//if not in a file put into a string handle
	if (!p->FILEFlagEncountered && chunk.getData()){
		if(p->main1ParamsSet[0])
			if(err = StoreStringDataUsingVarName(p->main1VarName,(const char*)chunk.getData(),chunk.getMemSize()))
				goto done;
		char nul[1];
		nul[0] = 0x00;
		try{
			chunk.WriteMemoryCallback(&nul, sizeof(char), 1);
		} catch (bad_alloc&){
			err = NOMEM;
		}
		if(!err && (err = SetOperationStrVar("S_getHttp",(const char*)chunk.getData())))
			goto done;
	}
		
done:
	if((err || res)){
		 SetOperationNumVar("V_flag",1);
	} else {
		err = SetOperationNumVar("V_flag",0);
	}

	if(curl){
		//always cleanup
		curl_easy_cleanup(curl);
	}
	
	if (outputFile != NULL) 
		err = XOPCloseFile(outputFile);

	if (inputFile != NULL) 
		err = XOPCloseFile(inputFile);
			
	/* cleanup libcurl*/
	curl_global_cleanup();
	
	return err;
}

int
RegisterEasyHTTP(void)
{
	char* cmdTemplate;
	char* runtimeNumVarList;
	char* runtimeStrVarList;

	// NOTE: If you change this template, you must change the easyHttpRuntimeParams structure as well.
	cmdTemplate = "easyHTTP/auth=string/pass=string/file=string/prox=string/post=string/ftp=string string[,varname]";
	runtimeNumVarList = "V_Flag";
	runtimeStrVarList = "S_getHttp";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(easyHttpRuntimeParams), (void*)ExecuteEasyHTTP, 0);
}
