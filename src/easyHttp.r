#include "XOPStandardHeaders.r"

resource 'vers' (1) {						/* XOP version info */
	0x01, 0x00, final, 0x00, 0,				/* version bytes and country integer */
	"1.00",
	"1.00, � 1993 WaveMetrics, Inc., all rights reserved."
};

resource 'vers' (2) {						/* Igor version info */
	0x07, 0x00, release, 0x00, 0,			/* version bytes and country integer */
	"7.00",
	"(for Igor Pro 7.00 or later)"
};

resource 'STR#' (1100) {					/* custom error messages */
	{
		/* [1] */
		"easyhttp requires Igor Pro 7.0 or later.",
		/* [2] */
		"error with POST string key:value; pairs.",
		/* [3] */
		"Wave must have at least 1 point",
		/* [4] */
		"Couldn't get an exclusive file lock on output file",
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


resource 'XOPC' (1100) {
	{
		"easyHttp",								// Name of operation.
		XOPOp+UtilOP+compilableOp+threadSafeOp,			// Operation's category.
	}
};
