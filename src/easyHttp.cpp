// Runtime param structure for GetHTTP operation.
#include "easyHttp.h"
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

void licence(string &);

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
  {
    /* in real-world cases, this would probably get this data differently
       as this fread() stuff is exactly what the library already would do
       by default internally */
    size_t retcode = fread(ptr, size, nmemb, (XOP_FILE_REF) stream);
    return retcode;
  }

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    string *chunk = (string *) userp;
    
    chunk->assign((const char *) contents, size * nmemb);
    
    return chunk->size();
}


int
ExecuteEasyHTTP(easyHttpRuntimeParamsPtr p)
{
	int err = 0;
	CURL *curl = NULL;
	CURLcode res = CURLE_OK;
	extern easyHttpPreferencesHandle thePreferences;
	int prefsState = 0;
	char pathName[MAX_PATH_LEN + 1];
	char pathNameToWrite[MAX_PATH_LEN + 1];
	char pathNameToRead[MAX_PATH_LEN + 1];
	XOP_FILE_REF inputFile = NULL;
	XOP_FILE_REF outputFile = NULL;
	char curlerror[CURL_ERROR_SIZE + 1];

   	string url;
    string proxyurl;
    string userpassword;
	string proxyUserPassword;
	string postString;
    curl_version_info_data *curl_data;
    std::stringstream oss;
	string chunk;
	
	if( igorVersion < 600 )
		return REQUIRES_IGOR_600;
  
	if( igorVersion < 620 && !RunningInMainThread())
		return NOT_IN_THREADSAFE;

	/* init the curl session */
	curl= curl_easy_init();
	if( !curl)
		goto done;
	
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

    /*
     If the URL is 'curl_version_info', then return what this version of curl can do
     you can jump directly to the end if this is all you need.
    */
    if(url == "curl_version_info"){
        curl_data = curl_version_info(CURLVERSION_NOW);
        oss << "version:" << curl_data->version << ";";
        oss << "host:" << curl_data->host << ";";
        oss << "features:" << curl_data->features << ";";
        oss << "ssl_version:" << curl_data->ssl_version << ";";
        oss << "libz_version:" << curl_data->libz_version << ";";
        oss << "PROTOCOLS:";
        for(int i = 0 ; curl_data->protocols[i] ; i++)
            oss << curl_data->protocols[i] << ",";
        oss << ";";
        oss << "libssh2_version:" << curl_data->libssh_version << ";";
        
        chunk.assign(oss.str());
        goto done;
    }
    if(url == "licence"){
        licence(chunk);
        goto done;
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

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerror);
    
    /*
     if you are in a multithread environment getting signals can really screw things up.
    http://curl.haxx.se/mail/lib-2013-05/0108.html
    */
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    
    
	// Flag parameters.
	
	/* 
     Proxy behaviour
     1) If the /PROX flag is specified and a proxy host is given, then that proxy is used for the operation.
            e.g. /PROXY=proxy.mycompany.com:3128
     2) If the /PROX flag is specified and NO host is given, then the system proxy settings are interrogated
     for the given URL of interest.
     3) If no /PROX flag is specified, then the easyHttp preferences are checked for a stored proxy.
     */
    
	if(RunningInMainThread())
		if(err = GetPrefsState(&prefsState))
			goto done;
	
	if(p->PROXFlagEncountered){
        if(p->PROXFlagParamsSet[0] && p->PROXFlagStrH != NULL) {
            proxyurl.assign(*(p->PROXFlagStrH), GetHandleSize(p->PROXFlagStrH));
            curl_easy_setopt(curl, CURLOPT_PROXY, proxyurl.c_str());
        } else {
            /* you are going to query the system configured proxy settings. */
        
        }
        
        /*
         you want to save the proxy in the IGOR preferences file
        you can only do this if running in the main thread.
         */
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
            strncpy((*thePreferences)->proxyURLandPort, proxyurl.c_str(), sizeof((*thePreferences)->proxyURLandPort));
        }
	} else if(thePreferences && prefsState){
        /* get proxy from saved preferences */
		if(strlen((*thePreferences)->proxyURLandPort))
			curl_easy_setopt(curl, CURLOPT_PROXY, (*thePreferences)->proxyURLandPort);
	}
	
	if(p->PPASFlagEncountered){
		/* proxy authentication string */
        if (p->PPASFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
        proxyUserPassword.assign(*(p->PPASFlagStrH), (int) GetHandleSize(p->PPASFlagStrH));
		curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxyUserPassword.c_str());
		
		/* you want to save the proxy in the IGOR preferences file. IN PLAINTEXT */
		if(p->SFlagEncountered && RunningInMainThread()){
			/*if the preferences handle doesn't exist we have to create it. */
			if(!thePreferences){
				thePreferences = (easyHttpPreferencesHandle) NewHandle(sizeof(easyHttpPreferences));
				if(thePreferences== NULL){
					err = MemError();
					goto done;
				}
				memset(*thePreferences, 0, GetHandleSize((Handle) thePreferences));
			}
			/* just check if the preferences handle is changed in size. */
			if(GetHandleSize((Handle) thePreferences) != sizeof(easyHttpPreferences)){
				SetHandleSize((Handle) thePreferences, sizeof(easyHttpPreferences));
				if(err = MemError())
					goto done;
				memset(*thePreferences, 0, GetHandleSize((Handle) thePreferences));
			}
			
			/* now put the proxy into the preferences handle */
			memset((*thePreferences)->proxyUserNameandPassword, 0, sizeof((*thePreferences)->proxyUserNameandPassword));
			strncpy((*thePreferences)->proxyUserNameandPassword, proxyUserPassword.data(), sizeof((*thePreferences)->proxyUserNameandPassword));
		}
	} else if(thePreferences && prefsState){
        /* if you didn't specify a proxy authentication and password, then try and retrieve from preferences */
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
        userpassword.assign(*(p->PPASFlagStrH), (int) GetHandleSize(p->PASSFlagStrH));
		//set a user name and password for authentication
		curl_easy_setopt(curl, CURLOPT_USERPWD, userpassword.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	}
	
    /* For a POST request */
	if(p->POSTFlagEncountered){
		if (p->POSTFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		postString.assign(*(p->POSTFlagStrH), (int) GetHandleSize(p->POSTFlagStrH));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t) postString.size());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void *) postString.data());
	}	
		
	if(p->FTPFlagEncountered){
		if (p->FTPFlagStrH == NULL) {
			err = OH_EXPECTED_STRING;
			goto done;
		}
		if(err = GetCStringFromHandle(p->FTPFlagStrH, pathName, MAX_PATH_LEN))
			goto done;	
		if(err = GetNativePath(pathName, pathNameToRead))
			goto done;
		if(err = XOPOpenFile(pathNameToRead, 0, &inputFile))
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
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, outputFile);
		 
	} else {
		/*send all data to this function*/
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		/* we pass our 'chunk' struct to the callback function */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	}

	/* some servers don't like requests that are made without a user-agent
	field, so we provide one */
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	
	/* follow redirects */
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	
	/* set a timeout */
	if(p->TIMEFlagEncountered)
		if(p->TIMEFlagNumber > 0)
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)p->TIMEFlagNumber);

    /* do the operation */
	try{
		res = curl_easy_perform(curl);
	} catch (bad_alloc&){
		err = NOMEM;
	}
	
done:
	if((err || res)){
		 SetOperationNumVar("V_flag", 1);
		 SetOperationStrVar("S_value", curlerror);	
	} else {
		err = SetOperationNumVar("V_flag", 0);
		err = SetOperationStrVar("S_Value", "");
        
        //if not in a file put into a string handle
        if (!p->FILEFlagEncountered && chunk.size()){
            if(p->main1ParamsSet[0]){
                if(err = StoreStringDataUsingVarName(p->main1VarName, (const char*)chunk.data(), chunk.size()))
                    goto done;
            } else if (err = SetOperationStrVar2("S_getHttp", (const char*)chunk.data(), chunk.size()))
                goto done;
        }
	}

    /* always cleanup */
	if(curl)
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
	cmdTemplate = "easyHTTP/S/VERB/TIME=number/pass=string/file=string/prox=[string]/ppas=string/post=string/ftp=string string[,varname]";
	runtimeNumVarList = "V_Flag";
	runtimeStrVarList = "S_getHttp;S_error";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(easyHttpRuntimeParams), (void*)ExecuteEasyHTTP, kOperationIsThreadSafe);
}

void licence(string &data){
    data.assign("easyHttp uses: libcurl, libssh2, libproxy, libz, openssl. Please see the COPYING.txt file from: http://www.igorexchange.com/project/easyHttp");
}
