#include "winiStatusCode.h"
#include "winiGlobals.h"
#include "IRodsLib3.h"
#include "IRodsGlobals.h"

//#include "srb_error.h"

extern "C"
{
}

namespace WINI
{
#if 0
StatusCode::StatusCode(const int& source)
{
	m_code = source;
}
StatusCode::StatusCode(const StatusCode& source)
{
	m_code = source.m_code;
}
StatusCode::StatusCode()
{
	m_code = WINI_OK;
}
#endif


StatusCode& StatusCode::operator=(const int& source)
{
	m_code = source;

	return *this;
}


bool StatusCode::isOk()
{
	if(m_code < 0)	//negative	//nonzero
		if(-3005 != m_code)
			return false;

	return true;
}


const char* StatusCode::GetError()
{
	return m_myMessage;
#if 0
	int i;

	for(i=0;i<Srb_numents;i++)
		if(m_code == srb_errent[i].srb_err)
			break;

	return srb_errent[i].srb_err_short;
#endif
}

}

