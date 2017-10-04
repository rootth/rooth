#pragma once

//Version
#define MAJVERSION		1
#define MINVERSION		0
#define SUBVERSION		0
#define TMPVERSION		0

#define STR(s) #s

#if MINVERSION == 0
	#define VERSION(maj, min, sub, tmp) STR(maj) "." STR(min)
#elif TMPVERSION == 0
	#define VERSION(maj, min, sub, tmp) STR(maj) "." STR(min) "." STR(sub) " - alpha"
#else
	#define VERSION(maj, min, sub, tmp) STR(maj) "." STR(min) "." STR(sub) " - beta " STR(tmp)
#endif

#define VERSION_4(maj, min, sub, tmp) STR(maj) "." STR(min) "." STR(sub) "." STR(tmp)
