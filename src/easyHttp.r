#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x00, final, 0x00, 0,				/* version bytes and country integer */
	"1.00",
	"1.00, © 1993 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x05, 0x04, release, 0x00, 0,			/* version bytes and country integer */
	"5.04",
	"(for Igor Pro 2.00 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"easyhttp requires Igor Pro 5.0 or later.",

	}
};

/* no menu item */

resource 'XOPI' (1100) {
	XOP_VERSION,							// XOP protocol version.
	DEV_SYS_CODE,							// Development system information.
	0,										// Obsolete - set to zero.
	0,										// Obsolete - set to zero.
	XOP_TOOLKIT_VERSION,					// XOP Toolkit version.
};

resource 'STR#' (1101) {					// Misc strings for XOP.
	{
		"-1",								// This item is no longer supported by the Carbon XOP Toolkit.
		"No Menu Item",						// This item is no longer supported by the Carbon XOP Toolkit.
		"easyHttp Help",					// Name of XOP's help file.
	}
};

//resource 'XOPF' (1100) {
//	{
		/* str1 = iPeekGetPeekData(host, port, instrument) */	/* This uses the direct call method */
//		"iPeekGetPeekData",							/* function name */
//		F_IO | F_EXTERNAL,					/* function category (string) */
//		HSTRING_TYPE,						/* return value type str1 (string handle) */			
//		{
		//	HSTRING_TYPE,					/* host (string handle) */
		//	NT_FP64,                         /* port (string handle) */
        //    HSTRING_TYPE,                   /* instrument (string handle) */
//		},
//	}
//};

resource 'XOPC' (1100) {
	{
		"easyHttp",								// Name of operation.
		XOPOp+UtilOP+compilableOp,			// Operation's category.
	}
};
