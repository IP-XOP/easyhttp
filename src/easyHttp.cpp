// Runtime param structure for GetHTTP operation.
#include "easyHttp.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include "TextWaveAccess.h"

#include <curl/curl.h>


using namespace std;


#ifdef MACIGOR
#include "proxy.h"
#endif
#ifdef WINIGOR
#include "Winhttp.h"
#endif

int getProxy(string url, string & proxy);
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
    
    chunk->append((const char *) contents, size * nmemb);
    
    return size * nmemb;
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
    struct curl_httppost* formpost = NULL;
    struct curl_httppost* lastpost = NULL;
    vector<string> tokens;

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
		url.assign(*(p->main0StrH), WMGetHandleSize(p->main0StrH));
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
            proxyurl.assign(*(p->PROXFlagStrH), WMGetHandleSize(p->PROXFlagStrH));
            curl_easy_setopt(curl, CURLOPT_PROXY, proxyurl.c_str());
        } else {
            /* you are going to query the system configured proxy settings. */
            if(err = getProxy(url, proxyurl))
                goto done;
            curl_easy_setopt(curl, CURLOPT_PROXY, proxyurl.c_str());
        }
        
        /*
         you want to save the proxy in the IGOR preferences file
        you can only do this if running in the main thread.
         */
        if(p->SFlagEncountered && RunningInMainThread()){
            //if the preferences handle doesn't exist we have to create it.
            if(!thePreferences){
                thePreferences = (easyHttpPreferencesHandle) WMNewHandle(sizeof(easyHttpPreferences));
                if(!thePreferences){
                    err = NOMEM;
                    goto done;
                }
                memset(*thePreferences, 0, WMGetHandleSize((Handle) thePreferences));
            }
            //just check if the preferences handle is changed in size.
            if(WMGetHandleSize((Handle) thePreferences) != sizeof(easyHttpPreferences)){
                err = WMSetHandleSize((Handle) thePreferences, sizeof(easyHttpPreferences));
                if(err)
                    goto done;
                memset(*thePreferences, 0, WMGetHandleSize((Handle) thePreferences));
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
        proxyUserPassword.assign(*(p->PPASFlagStrH), (int) WMGetHandleSize(p->PPASFlagStrH));
		curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxyUserPassword.c_str());
		
		/* you want to save the proxy in the IGOR preferences file. IN PLAINTEXT */
		if(p->SFlagEncountered && RunningInMainThread()){
			/*if the preferences handle doesn't exist we have to create it. */
			if(!thePreferences){
				thePreferences = (easyHttpPreferencesHandle) WMNewHandle(sizeof(easyHttpPreferences));
				if(thePreferences == NULL){
					err = NOMEM;
					goto done;
				}
				memset(*thePreferences, 0, WMGetHandleSize((Handle) thePreferences));
			}
			/* just check if the preferences handle is changed in size. */
			if(WMGetHandleSize((Handle) thePreferences) != sizeof(easyHttpPreferences)){
				err = WMSetHandleSize((Handle) thePreferences, sizeof(easyHttpPreferences));
				if(err)
					goto done;
				memset(*thePreferences, 0, WMGetHandleSize((Handle) thePreferences));
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
        userpassword.assign(*(p->PASSFlagStrH), (int) WMGetHandleSize(p->PASSFlagStrH));
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
		postString.assign(*(p->POSTFlagStrH), (int) WMGetHandleSize(p->POSTFlagStrH));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t) postString.size());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (void *) postString.data());
	}
    
    /* For a form post request */
    if(p->FORMFlagEncountered){
        //get the form tokens into a string vector
        size_t num_names = 0;
        
        if(err = textWaveToTokens(p->FORMFlagWaveH, tokens))
            goto done;
        //assume that wave is 2D, with first column being name, second column being contents.
        num_names = tokens.size() / 2;
        for(size_t ii = 0 ; ii < num_names ; ii++){
            curl_formadd(&formpost,
                          &lastpost,
                            CURLFORM_PTRNAME,
                             tokens.at(ii).data(),
                              CURLFORM_NAMELENGTH,
                               tokens.at(ii).size(),
                                CURLFORM_PTRCONTENTS,
                                 tokens.at(ii + num_names).data(),
                                  CURLFORM_CONTENTSLENGTH,
                                   tokens.at(ii + num_names).size(),
                                    CURLFORM_END);
        }
    
        /* Set the form info */
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
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
                int varDataType = 1;

                if (err = VarNameToDataType(p->main1VarName, &varDataType))
                    goto done;

                if (varDataType != 0){
                    err = EXPECTED_STRINGVARNAME;
                    goto done;
                }
                if(err = StoreStringDataUsingVarName(p->main1VarName, (const char*)chunk.data(), chunk.size()))
                    goto done;
            } else if (err = SetOperationStrVar2("S_getHttp", (const char*)chunk.data(), chunk.size()))
                goto done;
        }
	}

    /* always cleanup */
    if(formpost)
        curl_formfree(formpost);
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
	char const * cmdTemplate = "easyhttp/S/VERB/TIME=number/pass=string/file=string/prox[=string]/ppas=string/post=string/ftp=string/form=wave string[,varname]";
    
    char const * runtimeNumVarList = "V_Flag";
	char const * runtimeStrVarList = "S_getHttp;S_error";

	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(easyHttpRuntimeParams), (void*)ExecuteEasyHTTP, kOperationIsThreadSafe);
}

#ifdef WINIGOR
std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}
#endif

int getProxy(string url, string & proxy){
    /*
     Uses libproxy to find which system configured proxy to use for a given URL.  It examines and parses any
     possible proxy.pac file, as well as individually configured proxies.
     
     url:       a string containing the URL of interest
     proxy:     a string that is filled out with the proxy to use.  The string is empty if no proxy is required.
     
     returns:   0 for success, non-zero for failure.
     
     */
	int err = 0;
    proxy.clear();

#ifdef MACIGOR
    pxProxyFactory *pf = px_proxy_factory_new();
    if (!pf)
        return 1;
    // Get which proxies to use in order to fetch the URL
    char **proxies = px_proxy_factory_get_proxies(pf, url.c_str());
        
    //use the first proxy
    if(proxies[0])
        proxy.assign(proxies[0]);
    
    // Free the proxy list
    for (int i=0 ; proxies[i] ; i++)
        free(proxies[i]);
    free(proxies);
    
    // Free the proxy factory
    px_proxy_factory_free(pf);
#endif

#ifdef WINIGOR
	HINTERNET  hSession = NULL;
	WINHTTP_AUTOPROXY_OPTIONS pAutoProxyOptions;
	WINHTTP_PROXY_INFO pProxyInfo;
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG pProxyConfig;
	LPWSTR LPWproxy = NULL;
	LPCWSTR result;

	pProxyInfo.dwAccessType = 0L;
	pProxyInfo.lpszProxy = NULL;
	pProxyInfo.lpszProxyBypass = NULL;
	
	pAutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
	pAutoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
	pAutoProxyOptions.lpszAutoConfigUrl = NULL;
	pAutoProxyOptions.lpvReserved = NULL;
	pAutoProxyOptions.dwReserved = 0;
	pAutoProxyOptions.fAutoLogonIfChallenged = 1;

	pProxyConfig.fAutoDetect = 0;
	pProxyConfig.lpszAutoConfigUrl = NULL;
	pProxyConfig.lpszProxy = NULL;
	pProxyConfig.lpszProxyBypass = NULL;

	// returns True on success
	WinHttpGetDefaultProxyConfiguration(&pProxyInfo);

	if(pProxyInfo.lpszProxy) {
		LPWproxy = pProxyInfo.lpszProxy;
	} else {
		WinHttpGetIEProxyConfigForCurrentUser(&pProxyConfig);
		if(pProxyConfig.lpszProxy)
			LPWproxy = pProxyConfig.lpszProxy;
		if(pProxyConfig.lpszAutoConfigUrl)
			pAutoProxyOptions.lpszAutoConfigUrl = pProxyConfig.lpszAutoConfigUrl;
	}
	
	if(!pProxyConfig.lpszProxy && !pProxyInfo.lpszProxy){
		hSession = WinHttpOpen(L"libcurl/1.0", 
                                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                WINHTTP_NO_PROXY_NAME, 
                                WINHTTP_NO_PROXY_BYPASS, 0);

		if(hSession){
			std::wstring stemp = s2ws(url);
			result = stemp.c_str();
			err = WinHttpGetProxyForUrl(hSession, result, &pAutoProxyOptions, &pProxyInfo);
		}
		if(pProxyInfo.lpszProxy)
			LPWproxy = pProxyInfo.lpszProxy;
	}

	if(LPWproxy){
		int sz = lstrlenW(LPWproxy);
		int szL = 0;
		szL = WideCharToMultiByte(CP_ACP, 0, LPWproxy, sz, NULL, 0, NULL, NULL);
		proxy.resize(szL);
		WideCharToMultiByte(CP_ACP, 0, LPWproxy, sz, (LPSTR) proxy.data(), (int) proxy.size(), NULL, NULL);
	}
		
	if(hSession)
		WinHttpCloseHandle(hSession);
	if(pProxyInfo.lpszProxy)
		GlobalFree(pProxyInfo.lpszProxy);
	if(pProxyInfo.lpszProxyBypass)
		GlobalFree(pProxyInfo.lpszProxyBypass);
	if(pProxyConfig.lpszAutoConfigUrl)
		GlobalFree(pProxyConfig.lpszAutoConfigUrl);
	if(pProxyConfig.lpszProxy)
		GlobalFree(pProxyConfig.lpszProxy);
	if(pProxyConfig.lpszProxyBypass)
		GlobalFree(pProxyConfig.lpszProxyBypass);
#endif
    
    if(proxy.find(string("DIRECT")) != string::npos)
        proxy.clear();
 
    if(proxy.find(string("direct")) != string::npos)
        proxy.clear();
    
    return 0;
}


void licence(string &data){
    /* a function to detail the licences of the contributing libraries
     data:      a string that will contain the licence information
    */
    data.assign("easyHttp uses: libcurl, libssh2, libproxy, libz, openssl.");
    
#ifdef MACIGOR
    CFStringRef licencePath;
    
    /* Get a reference to the main bundle */
    CFBundleRef easyHttpBundle = CFBundleGetBundleWithIdentifier(CFSTR("easyhttp64"));

    /* Get a reference to the licence URL */
    CFURLRef licenceURL = CFBundleCopyResourceURL(easyHttpBundle, CFSTR("COPYING.txt"), NULL, NULL);
        
    /* Convert the URL reference into a string reference */
    if(licenceURL){
        licencePath = CFURLCopyFileSystemPath(licenceURL, kCFURLPOSIXPathStyle);
    } else {
        return;
    }
    // Get the system encoding method
    //CFStringEncoding encodingMethod = CFStringGetSystemEncoding();
    
    /* Convert the string reference into a C string */
    char path[MAX_PATH_LEN + 1];
    CFStringGetCString(licencePath, path, MAX_PASSLEN + 1, kCFStringEncodingMacRoman);
    
    /* open, read and fillout data. */
    ifstream infile;
    infile.open (path, std::ios::in | std::ios::binary);
    if (infile)  {
        infile.seekg(0, std::ios::end);
        data.resize(infile.tellg());
        infile.seekg(0, std::ios::beg);
        infile.read(&data[0], data.size());
        infile.close();
    }
#endif
}
