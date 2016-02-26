#ifndef __TASKENGINE_H__
#define __TASKENGINE_H__
#include <deque>
//////////////////////////////////////////////////////////////////////////
#define DKTASKID_INVALID (0xFFFFFFFF)
#define DKTASKID_BEGIN (0x00000000)
//
class DKTaskEngine;
//
class DKTaskBase
{
	friend class DKTaskEngine;
public:
	DKTaskBase();
	virtual ~DKTaskBase();

public:
	virtual void Run() = 0;

public:
	unsigned int getTaskId() const;

protected:
    bool _isNeedExitTask(unsigned int uTimeMS = 0);

private:
	void setEvent( HANDLE hEventTaskEngineExit, HANDLE hEventCurTaskExit );
	void setTaskId( unsigned int uTaskId );

protected:
	HANDLE m_hEventTaskEngineExit;
	HANDLE m_hEventCurTaskExit;
private:
	unsigned int m_uTaskId;
};

class DKTaskEngine {
public:
	typedef unsigned int task_id_type;
	typedef std::deque< DKTaskBase* > task_container_type;
public:
	DKTaskEngine();
	virtual ~DKTaskEngine();

public:
	bool pushbackTask( DKTaskBase* pTask, task_id_type& taskId );
	bool cancelTask( task_id_type idTask );
	void cancelAllTask();
	bool getCurTaskId( task_id_type& idTask );
	void exit();

private:
	DKTaskBase* _popupTask();
	bool _safeEnterFunc();
	void _safeExitFunc();
	static DWORD WINAPI TaskEngineThreadProc( LPVOID lpParameter );
	task_id_type _allocalTaskId();
	void _cancelAllTask();
	bool _isRunning();
	void _clearAllTask();
	
private:
	task_id_type m_curTaskId; // 
	unsigned int m_uFlag;
	HANDLE m_hThreadTaskEngine;
	HANDLE m_hEventAccessSafe;
	HANDLE m_hEventExitTaskEngine;
	HANDLE m_hEventExitCurTask;
	task_container_type m_containerTask;
};



#endif //__TASKENGINE_H__