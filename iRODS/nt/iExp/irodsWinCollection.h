#ifndef __irods_collection__
#define __irods_collection__

#include "irodsWinObj.h"
#include "irodsWinDataobj.h"
#include <vector>


class irodsWinCollection: public irodsWinObj
{
public:
	CString name;
	CString fullPath;
	CString type;
	CString owner;
	CString info1;
	CString info2;
	bool queried;

	irodsWinCollection()
	{
		name = "";
		fullPath = "";
		type = "";
		owner = "";
		info1 = "";
		info2 = "";
		queried = false;
		SetWinObjType(IRODS_COLLECTION_TYPE);
	}

	~irodsWinCollection()
	{
		childDataObjs.clear();
		childCollections.clear();
	}

	void CopyTo(irodsWinCollection & CollCopyTo)
	{
		CollCopyTo.name = this->name;
		CollCopyTo.fullPath = this->fullPath;
		CollCopyTo.type = this->type;
		CollCopyTo.owner = this->owner;
		CollCopyTo.info1 = this->info1;
		CollCopyTo.info2 = this->info2;
		CollCopyTo.queried = this->queried;
		CollCopyTo.my_win_obj_type = this->my_win_obj_type;

		for(int i=0;i< this->childCollections.size();i++)
		{
			CollCopyTo.childCollections.push_back(this->childCollections[i]);
		}
		for(int i=0;i<this->childDataObjs.size();i++)
		{
			CollCopyTo.childDataObjs.push_back(this->childDataObjs[i]);
		}
	}

	std::vector<irodsWinCollection> childCollections;
	std::vector<irodsWinDataobj> childDataObjs;
};

#endif