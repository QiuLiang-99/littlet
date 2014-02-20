#include "QAutoTask.h"
#include <Shlwapi.h>
#include "QTimerMan.h"
#include "file/TextFile.h"
#include "QDBHelper.h"
#include "Worker.h"
#include <iterator>
#include "ui/QUIGlobal.h"

QString QGetTaskDoWhatString(ENUM_AUTOTASK_DOWHAT eDo)
{
	switch (eDo)
	{
	case AUTOTASK_DO_NOTSET : // 0,	// 未设置	
        return L"未设置";
	case AUTOTASK_DO_REMIND : // 2, // 提示信息
        return L"提示信息";
	case AUTOTASK_DO_EXECPROG : // 3,  // 执行程序
        return L"执行程序";
	case AUTOTASK_DO_SYSSHUTDOWN : // 4,  // 关机
        return L"关机";
	case AUTOTASK_DO_SYSREBOOT : // 5,  // 重启
        return L"重启";
	}
	return L"";
}

// 任务定时器
VOID CALLBACK TaskCallback(__in  PVOID lpParameter,__in  BOOLEAN TimerOrWaitFired)
{	// 任务交给工作者线程来完成
	QAutoTask *pTask = QAutoTaskMan::GetInstance()->GetTask((int)lpParameter);
	if (NULL != pTask)
	{
        WORK_PARAM wp;
        wp.eWorkType = WORK_TYPE_AUTOTASKFIRED;
        wp.lParam = (LPARAM)pTask;
        QWorker::GetInstance()->DoWork(&wp);

//        QUIPostCodeToMainWnd(MWND_NOTIFY_AUTOTASKFIRED,(LPARAM)pTask);
//		pTask->TaskFired();
	}
}

BOOL QAutoTask::TaskFired()
{
	// 刷新定时器
    QTimerMan* pTimerMan = QTimerMan::GetInstance();
	pTimerMan->StopTimer(m_pTimer);
	m_eLastStatus = pTimerMan->StartTimer(m_pTimer,m_nID);

    QUIPostCodeToMainWnd(MWND_NOTIFY_AUTOTASKFIRED,(LPARAM)this);

	return TRUE;
}

QAutoTask::QAutoTask(LPCWSTR szTask, int nID,int nTimerID,
	ENUM_AUTOTASK_DOWHAT eDo, int nFlag,QTime tmCreate )
{
	m_sTask = szTask;
	m_nID = nID;
	m_pTimer = QTimerMan::GetInstance()->GetTimer(nTimerID);
	ASSERT(m_pTimer != NULL);
	m_nFlag = nFlag;
	m_tmCreate = tmCreate;
	m_eDoWhat = eDo;

	m_eLastStatus = AUTOTASK_RUNNING_STATUS_OK;
}

BOOL QAutoTask::Run()
{
	if (!IsPaused())
	{
		m_eLastStatus = QTimerMan::GetInstance()->StartTimer(m_pTimer,m_nID);

		return (AUTOTASK_RUNNING_STATUS_OK == m_eLastStatus);
	}
	
	m_eLastStatus = AUTOTASK_RUNNING_STATUS_PAUSED;
	
	return TRUE;
}

BOOL QAutoTask::FlagChanged()
{
	return QDBEvents::GetInstance()->AutoTask_SetFlag(ID(),m_nFlag);
}

BOOL QAutoTask::Startup()
{
	if (IsStartup())
		return TRUE;

	if (NULL == m_pTimer)
	{
		m_eLastStatus = AUTOTASK_RUNNING_STATUS_BADTIMER;
		return FALSE;
	}
	
	_RemoveFlag(m_nFlag,AUTOTASK_FLAG_PAUSED);
	FlagChanged();

	m_eLastStatus = QTimerMan::GetInstance()->StartTimer(m_pTimer,m_nID);

	return (AUTOTASK_RUNNING_STATUS_OK == m_eLastStatus);
}

BOOL QAutoTask::JumpoverThisExec()
{
	if (!IsStartup())
    {
        return FALSE;
    }

	m_eLastStatus = QTimerMan::GetInstance()->JumpoverTimerThisExec(m_pTimer,m_nID);
    return TRUE;
//	return (AUTOTASK_RUNNING_STATUS_OK == m_eLastStatus);
}

BOOL QAutoTask::Pause()
{
	if (!IsStartup())
		return TRUE;
	if (NULL == m_pTimer)
		return FALSE;

	if (QTimerMan::GetInstance()->StopTimer(m_pTimer))
	{
		_AddFlag(m_nFlag,AUTOTASK_FLAG_PAUSED);
		FlagChanged();

		return TRUE;
	}
	return FALSE;
}

BOOL QAutoTask::SetDoWhat( ENUM_AUTOTASK_DOWHAT eDo ,LPCWSTR szTask)
{
	if (QDBEvents::GetInstance()->AutoTask_SetDo(m_nID,szTask,eDo))
	{
		m_eDoWhat = eDo;
		m_sTask = szTask;
		return TRUE;
	}
	return FALSE;
}

BOOL QAutoTask::Edit( LPCWSTR szTask,ENUM_AUTOTASK_DOWHAT eDo,int nFlag )
{
	if (IsStartup())
	{ // 运行的时候不可以编辑
		ASSERT(FALSE);
		return FALSE;
	}
	if (QDBEvents::GetInstance()->AutoTask_Edit(ID(),szTask,eDo,nFlag))
	{
		m_sTask  = szTask;
		m_eDoWhat = eDo;
		m_nFlag = nFlag;

		return TRUE;
	}
	return FALSE;
}

BOOL QAutoTask::SetTimer( const QTime&tmBegin,const QTime&tmEnd,LPCWSTR sTimerExp )
{
	QTimerMan* pTimerMgr = QTimerMan::GetInstance();
	if (NULL == m_pTimer)
	{
		m_pTimer = pTimerMgr->AddTimer(tmBegin,tmEnd,sTimerExp,NULL,NULL);
		if (NULL == m_pTimer)
		{
			ASSERT(FALSE);
			return FALSE;
		}
		
		return QAutoTaskMan::GetInstance()->SetTaskTimer(this,m_pTimer);
	}
	else
	{
		return pTimerMgr->EditTimer(m_pTimer,tmBegin,tmEnd,sTimerExp,
			m_pTimer->GetRemindExp(),m_pTimer->GetXFiled());	
	}
}

BOOL QAutoTask::IsStartup() const
{
	if (m_pTimer != NULL)
	{
		return m_pTimer->IsStarted();
	}
	return FALSE;
}

BOOL QAutoTask::GetNextExecTime(QTime &tmNext)
{
	ASSERT(m_pTimer != NULL);
	if (m_pTimer != NULL)
	{
		if (!IsStartup())
		{
			return FALSE;
		}
		else
		{
			tmNext = m_pTimer->NextExecTime();
			return TRUE;
		}
	}
	return FALSE;
}

QString QAutoTask::GetNextExecTimeString()
{
	ASSERT(m_pTimer != NULL);
	if (m_pTimer != NULL)
	{
		if (!IsStartup())
		{
			return GetLastStartStatusDes();
		}
		else
		{
			return m_pTimer->NextExecTime().Format(L"%Y/%m/%d %H:%M:%S");
		}
	}
	return L"无效的定时器";
}

QString QAutoTask::GetDoWhatString()
{
	QString sRet;
	switch (m_eDoWhat)
	{
	case AUTOTASK_DO_NOTSET : // 0,	// 未设置	
		{
			ASSERT(FALSE);
			return L"Not Set";
		}
	case AUTOTASK_DO_REMIND : // 2, // 提示信息
		{
			sRet.Format(L"提醒:%s",m_sTask);
			break;
		}
	case AUTOTASK_DO_EXECPROG : // 3,  // 执行程序
		{
			sRet.Format(L"执行程序:%s",m_sTask);
			break;			
		}
	case AUTOTASK_DO_SYSSHUTDOWN : // 4,  // 关机
		{
			return L"关机";
		}
	case AUTOTASK_DO_SYSREBOOT : // 5,  // 重启
		{
			return L"重启";
		}
	case AUTOTASK_DO_BREAKAMOMENT : // 6,  // 休息会儿
		{
			return L"休息一会儿";
		}
	}
	return sRet;
}

QString QAutoTask::GetWhenDoString()
{
    QString sRet = L"Bad Timer";
	if (NULL != m_pTimer)
	{
		m_pTimer->GetWhenDoString(sRet);
	}
	return sRet;
}

QString QAutoTask::GetLifeTimeString()
{
	ASSERT(m_pTimer != NULL);
	if (m_pTimer != NULL)
	{
		QString sRet;
		sRet.Format(L"[%s ~ %s]",
			m_pTimer->GetLifeBegin().Format(L"%Y/%m/%d %H:%M"),
			m_pTimer->GetLifeEnd().Format(L"%Y/%m/%d %H:%M"));
		return sRet;
	}
	return L"Bad Timer";
}

QString QAutoTask::GetRemindString()const
{
	ASSERT(m_pTimer != NULL);
    QString sRet;
	if (m_pTimer != NULL)
	{
        m_pTimer->GetRemindString(sRet);
	}
	return sRet;
}

ENUM_AUTOTASK_EXECFLAG QAutoTask::GetExecFlag()
{
	if (m_pTimer != NULL)
	{
		return m_pTimer->GetExecFlag();
	}
	ASSERT(FALSE);
	return AUTOTASK_EXEC_NOTSET;
}

QString QAutoTask::GetLastStartStatusDes()
{
	return GetRunningStatusDescription(m_eLastStatus);
}

BOOL QAutoTask::EnableReminder( BOOL bEnable/*=TRUE*/ )
{
	if (NULL == m_pTimer)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	return QTimerMan::GetInstance()->EnableTimerReminder(m_pTimer,m_nID,bEnable);
}

BOOL QAutoTask::IsReminderEnabled() const
{
	if (NULL != m_pTimer)
	{
		return m_pTimer->IsReminderEnabled();
	}
	return FALSE;
}

BOOL QAutoTask::IsOverdue() const
{
	return AUTOTASK_RUNNING_STATUS_OVERDUE == m_eLastStatus;
}

BOOL QAutoTask::IsPaused() const
{
	if (_HasFlag(Flag(),AUTOTASK_FLAG_PAUSED))
	{
		ASSERT(!m_pTimer->IsStarted());
		return TRUE;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
QAutoTaskMan::QAutoTaskMan()
{
}

TaskListItr QAutoTaskMan::_FindTask( const QAutoTask* pTask )
{
	for (TaskListItr itr = _BeginItr(m_lstTask); 
		itr != _EndItr(m_lstTask); ++itr)
	{
		if (*itr == pTask)
		{
			return itr;
		}
	}
	return _EndItr(m_lstTask);
}

TaskListItr QAutoTaskMan::_FindTask( int nID )
{
	for (TaskListItr itr = _BeginItr(m_lstTask); 
		itr != _EndItr(m_lstTask); ++itr)
	{
		if ((*itr)->m_nID == nID)
		{
			return itr;
		}
	}
	return _EndItr(m_lstTask);
}

void QAutoTaskMan::RemoveAll()
{
	for (TaskListItr itr = _BeginItr(m_lstTask); 
		itr != _EndItr(m_lstTask); ++itr)
	{
		delete *itr;
	}
	m_lstTask.clear();
}

BOOL QAutoTaskMan::DeleteTask( QAutoTask* pTask )
{
	TaskListItr itr = _FindTask(pTask);
	if (_EndItr(m_lstTask) != itr)
	{
        // 先停止掉当前的定时器
        if (pTask->IsStartup())
        {
            pTask->Pause();
        }
        // 删除掉timer
        QTimer *pTimer = pTask->GetTimer();
        if (NULL != pTimer)
        {
            QTimerMan::GetInstance()->DeleteTimer(pTimer);
        }
        // 从数据库删除任务
		if (QDBEvents::GetInstance()->AutoTask_Delete(pTask->ID()))
		{
			delete *itr;
			m_lstTask.erase(itr);
			return TRUE;
		}
	}
	return FALSE;
}

QAutoTask* QAutoTaskMan::GetTask( int nID )
{
	TaskListItr itr = _FindTask(nID);
	if (_EndItr(m_lstTask) != itr)
	{
		return (*itr);
	}
	return NULL;
}

BOOL QAutoTaskMan::Startup()
{
	return QDBEvents::GetInstance()->AutoTask_GetAll(m_lstTask);
}

void QAutoTaskMan::GetTaskList( AutoTaskList &lst )
{
	std::copy(_BeginItr(m_lstTask),_EndItr(m_lstTask),
		std::back_insert_iterator<AutoTaskList>(lst));
}

QAutoTask* QAutoTaskMan::AddTask( LPCWSTR szTask,int nTimerID,ENUM_AUTOTASK_DOWHAT eDo,int nFlag )
{
	int nID = QDBEvents::GetInstance()->AutoTask_Add(szTask,nTimerID,eDo,nFlag);
	if (INVALID_ID != nID)
	{
		QAutoTask *pTask = new QAutoTask(szTask,nID,nTimerID,
			eDo,nFlag,QTime::GetCurrentTime());
		m_lstTask.push_back(pTask);
		return pTask;
	}
	return NULL;
}

BOOL QAutoTaskMan::SetTaskTimer(QAutoTask* pTask,QTimer* pTimer)
{
	if ((NULL==pTimer) || (NULL == pTask))
	{
		ASSERT(FALSE);
		return FALSE;
	}
	return QDBEvents::GetInstance()->AutoTask_SetTimer(pTask->ID(),pTimer->ID());
}

void QAutoTaskMan::OnWindowCreated(HWND hWnd)
{

}

void QAutoTaskMan::OnWindowDestroyed(HWND hWnd)
{

}

BOOL QAutoTaskMan::EnableTaskReminder( int nTaskID,BOOL bEnable/*=TRUE*/ )
{
	TaskListItr itr = _FindTask(nTaskID);
	if (_EndItr(m_lstTask) != itr)
	{
		return (*itr)->EnableReminder(bEnable);
	}
	return FALSE;
}

BOOL QAutoTaskMan::IsTaskReminderEnabled( int nTaskID )
{
	TaskListItr itr = _FindTask(nTaskID);
	if (_EndItr(m_lstTask) != itr)
	{
		return (*itr)->IsReminderEnabled();
	}
	return FALSE;
}

BOOL QAutoTaskMan::JumpoverTaskThisExec( INT nTaskID )
{
	TaskListItr itr = _FindTask(nTaskID);
	if (_EndItr(m_lstTask) != itr)
	{
		return (*itr)->JumpoverThisExec();
	}
	return FALSE;
}

QAutoTask* QAutoTaskMan::GetMostCloseExecute()
{
	QTime tmExec,tmTest;
	tmExec.SetDateTime(9999,12,30,23,59,59);
	QAutoTask *pTask = NULL,*pTest = NULL;
	TaskListItr itrEnd = _EndItr(m_lstTask);
	for (TaskListItr itr = _BeginItr(m_lstTask); itr != itrEnd; ++itr)
	{
		pTest = *itr;
		if (pTest->IsStartup() && pTest->GetNextExecTime(tmTest))
		{
			if (tmTest < tmExec)
			{
				tmExec = tmTest;
				pTask = pTest;
			}
		}
	}
	return pTask;
}

void QAutoTaskMan::TaskOverdue( QAutoTask* pTask )
{
/*	TaskListItr itr = _FindTask(pTask);
	if (_EndItr(m_lstTask) != itr)
	{
		m_lstTask.erase(itr);
	}
*/
}

BOOL QAutoTaskMan::ResetOverdueTask( QAutoTask* pTask )
{
	return TRUE;
// 	TaskListItr itr = _FindTask(pTask);
// 	return (_EndItr(m_lstTask) != itr);
}

int QAutoTaskMan::GetTaskCount() const
{
	return m_lstTask.size();
}

int QAutoTaskMan::GetOverdueTaskCount() 
{
	int nRet = 0;
	TaskListItr itrEnd = _EndItr(m_lstTask);
	for (TaskListItr itr = _BeginItr(m_lstTask); itr != itrEnd; ++itr)
	{
		if ((*itr)->IsOverdue())
		{
			nRet++;
		}
	}
	return nRet;
}

QAutoTask* QAutoTaskMan::NewAutoTask( ENUM_AUTOTASK_DOWHAT nDoWhat, 
    const CStdStringW& sDoWhatParam, const CStdStringW& sWhenDo, 
    const CStdStringW& sRemindexp, QTime tmBegin, QTime tmEnd, 
    __out CStdStringW& sError )
{
    QTimerMan *pTimerMgr = QTimerMan::GetInstance();
    QTimer *pTimer = pTimerMgr->AddTimer(tmBegin, tmEnd,
        sWhenDo, sRemindexp, sDoWhatParam);
    if (NULL == pTimer)
    {
        sError = L"定时器错误";
        return NULL;
    }
    QAutoTask *pTask = AddTask(sDoWhatParam, pTimer->ID(), nDoWhat, 0);
    if (NULL == pTask)
    {
        pTimerMgr->DeleteTimer(pTimer);
        sError = L"添加任务失败！";
        return NULL;
    }

    ENUM_AUTOTASK_RUNNING_STATUS eStatus = pTimer->TestStart();
    if ((AUTOTASK_RUNNING_STATUS_OVERDUE == eStatus)
        || (AUTOTASK_RUNNING_STATUS_NOCHANCETOEXEC == eStatus))
    {
        sError = L"定时时间无法执行到，请重设。";
        pTimerMgr->DeleteTimer(pTimer);
        DeleteTask(pTask);
        return NULL;
    }
    pTask->Startup();
    return pTask;
}
