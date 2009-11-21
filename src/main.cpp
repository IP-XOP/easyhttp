// Runtime param structure for GetHTTP operation.
#include "easyHttp.h"

easyHttpPreferencesHandle thePreferences;

static void initialisePreferences(){
	//reference to references handle is initially NULL
	thePreferences = NULL;
	//get the preferences
	GetXOPPrefsHandle((Handle*) &thePreferences);
	//you should test the preferences handle for non-NULL behaviour before using it.
}

static void saveAndCleanupPreferences(){
	//only if there are preferences will you save them.
	if(thePreferences){
		SaveXOPPrefsHandle((Handle) thePreferences);
		DisposeHandle((Handle) thePreferences);
	}
}

static int
RegisterOperations(void)		// Register any operations with Igor.
{
	int result;
	
	// Register XOP1 operation.
	if (result = RegisterEasyHTTP())
		return result;
	
	//register threadsafe version as well, only if your IGOR version is good enough
	if ((igorVersion > 602) && (igorVersion <= 700)){ 
		if(result = RegisterTHReasyHTTP())
			return result;
	} 
	
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
	long msg = GetXOPMessage();
	switch (msg) {
		case INIT:
			//setup any globals that Curl needs.
			result = curl_global_init(CURL_GLOBAL_ALL);
			
			//load the easyHttp preferences (mainly proxy settings)
			initialisePreferences();
			break;
		case CLEANUP:
			curl_global_cleanup();
			#ifdef _WINDOWS_
				pthread_win32_process_detach_np();
			#endif
			//save the easyHttp preferences (mainly proxy settings)
			saveAndCleanupPreferences();
			break;
		default:
		break;
	}
	SetXOPResult(result);
}

/*	main(ioRecHandle)

This is the initial entry point at which the host application calls XOP.
The message sent by the host must be INIT.

main does any necessary initialization and then sets the XOPEntry field of the
ioRecHandle to the address to be called for future messages.
*/
#ifdef _MACINTOSH_
HOST_IMPORT int main(IORecHandle ioRecHandle){
#endif
#ifdef _WINDOWS_
HOST_IMPORT void
main(IORecHandle ioRecHandle){
#endif

	int result;
	
	XOPInit(ioRecHandle);							// Do standard XOP initialization.
	
	SetXOPEntry(XOPEntry);							// Set entry point for future calls.
	
	//get the proxy preferences
	initialisePreferences();
	
	#ifdef _WINDOWS_
	//start up the pthread library
	pthread_win32_process_attach_np();
	#endif

	if (result = RegisterOperations()) {
		SetXOPResult(result);
#ifdef _MACINTOSH_
		return 0;
#endif
	}
	
	SetXOPResult(0);
#ifdef _MACINTOSH_
		return 0;
#endif
}
