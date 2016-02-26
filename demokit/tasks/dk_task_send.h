#ifndef __DKTASKSEND_H__
#define __DKTASKSEND_H__
#include "DKTaskEngine.h"
//
class dk_root;
//
class dk_task_send : public DKTaskBase {
public:
	dk_task_send(dk_root* pDKRoot);
	virtual ~dk_task_send();

public:
	virtual void Run();

private:
	dk_root* m_pDKRoot;
	HANDLE m_hFilePCM;
};



#endif //__DKTASKSEND_H__