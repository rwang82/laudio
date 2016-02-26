#ifndef __DKTASKFETCH_H__
#define __DKTASKFETCH_H__
#include "DKTaskEngine.h"
//
class dk_root;
//
class dk_task_fetch : public DKTaskBase{
public:
	dk_task_fetch( dk_root* pDKRoot );
	virtual ~dk_task_fetch();

public:
	virtual void Run();

private:
	dk_root* m_pDKRoot;
	HANDLE m_hFilePCM;
};

#endif //__DKTASKFETCH_H__