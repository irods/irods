#ifndef __WINI_STATUS_CODE_H__
#define __WINI_STATUS_CODE_H__

#include "winiGlobals.h"
static char* m_myMessage = "STATUS CODES NOT IMPLEMENTED";
namespace WINI
{
class StatusCode
{
public:
	StatusCode(const int& source){m_code = source;};
	StatusCode(const StatusCode& source){m_code = source.m_code;};
	StatusCode(){m_code = WINI_OK;};
	bool isOk(); 
	
	StatusCode& operator=(const int& source);
	friend bool operator==(const int& left, const StatusCode& right) { if(left == right.m_code){ return true;} return false;};
	friend bool operator!=(const int& left, const StatusCode& right) { if(left != right.m_code){ return true;} return false;};

	const char* GetError();
	int GetCode() { return m_code;};

#if 0
	StatusCode& operator=(const StatusCode& source);

	friend bool operator==(const StatusCode& left, const StatusCode& right);
	friend bool operator==(const StatusCode& left, const int& right);
	friend bool operator==(const int& left, const StatusCode& right);
	friend bool operator!=(const StatusCode& left, const StatusCode& right);
	friend bool operator!=(const StatusCode& left, const int& right);
	friend bool operator(bool);
#endif

private:
	int m_code;
	
};
}//end namespace
#endif
