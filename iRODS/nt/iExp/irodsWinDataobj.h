#ifndef __irods_dataobj_h__
#define __irods_dataobj_h__

#include "irodsWinObj.h"

class irodsWinDataobj: public irodsWinObj
{
public:
	irodsWinDataobj()
	{
		name = "";
		parentCollectionFullPath = "";
		owner = "";
		size = 0;
		modifiedTime = "";
		replNum = 0;
		replStatus = 0;
		rescName = "";
		rescGroup = "";
		checkSum = "";
		version = "";
		this->SetWinObjType(IRODS_DATAOBJ_TYPE);
	}

	void CopyTo(irodsWinDataobj & ObjCopyTo)
	{
		ObjCopyTo.name = this->name;
		ObjCopyTo.parentCollectionFullPath = this->parentCollectionFullPath;
		ObjCopyTo.owner = this->owner;
		ObjCopyTo.size = this->size;
		ObjCopyTo.modifiedTime = this->modifiedTime;
		ObjCopyTo.replNum = this->replNum;
		ObjCopyTo.replStatus = this->replStatus;
		ObjCopyTo.rescName = this->rescName;
		ObjCopyTo.rescGroup = this->rescGroup;
		ObjCopyTo.checkSum = this->checkSum;
		ObjCopyTo.version = this->version;
		ObjCopyTo.my_win_obj_type = this->my_win_obj_type;
	}

	CString name;
	CString parentCollectionFullPath;
	CString owner;
	__int64 size;
	CString modifiedTime;
	int		replNum;
	int		replStatus;
	CString rescName;      // resource name
	CString rescGroup;     // rsource group
	CString checkSum;
	CString version;
};

#endif
