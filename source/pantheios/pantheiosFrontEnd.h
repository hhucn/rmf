/* FILE: pantheiosConfiguration.h */
#ifndef _pantheiosConfiguration_
#define _pantheiosConfiguration_

#include "pantheiosHeader.h"

// LIBS for the backends are needed in the Makefile !

// - FRONT END: Custom
// ---------------------------------------------------------
PANTHEIOS_CALL(int) pantheios_fe_init(void *reserved, void **ptoken)
{
    *ptoken = NULL;

    return(0);
}

PANTHEIOS_CALL(void) pantheios_fe_uninit(void *token)
{
	// uninitialization: do nothing
}

PANTHEIOS_CALL(char const*) pantheios_fe_getProcessIdentity(void* token)
{
    return("rmf"); // Rate Measurement (Framework)
}

PANTHEIOS_CALL(int) pantheios_fe_isSeverityLogged(void *token, int severity, int backEndId)
{
    return ( severity <= pantheois_logLevel );
}
// ---------------------------------------------------------

#endif /* _pantheiosConfiguration_ */
