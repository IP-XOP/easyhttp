// Runtime param structure for GetHTTP operation.
#include "easyHttp.h"

static int
RegisterOperations(void)		// Register any operations with Igor.
{
	int result;

	//setup any globals that Curl needs.
	result = curl_global_init(CURL_GLOBAL_ALL);
	
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
	
	switch (GetXOPMessage()) {
			break;
		case CLEANUP:
			curl_global_cleanup();
			break;
		// We don't need to handle any messages for this XOP.
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
