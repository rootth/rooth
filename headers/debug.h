#pragma once

#include<sstream>
#include<Windows.h>

//a macro for printing debug outputs (prints an ASCII String)
#define alert(ss)																									\
		MessageBoxA(0, ((std::stringstream*)&(std::stringstream()<<ss))->str().c_str(),								\
		((std::stringstream*)&(std::stringstream()<<"Debug: "__FILE__"("<<__LINE__<<")          "))->str().c_str(), \
		MB_ICONINFORMATION);

#define debug alert