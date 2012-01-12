#ifndef __irods_win_obj__h_
#define __irods_win_obj__h_

/* Author: Bing */

//#define IRODS_DATAOBJ_TYPE 1
//#define IRODS_COLLECTION_TYPE 2

class irodsWinObj
{
public:
	static const int IRODS_DATAOBJ_TYPE = 1;
	static const int IRODS_COLLECTION_TYPE = 2;

	irodsWinObj()
	{
		my_win_obj_type = -1;   // an unknown value
	}
	int my_win_obj_type;

	void SetWinObjType(int newtype)
	{
		my_win_obj_type = newtype;
	}
	int GetWinObjType()
	{
		return my_win_obj_type;
	}
};

#endif