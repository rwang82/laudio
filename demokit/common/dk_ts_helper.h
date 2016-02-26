#ifndef __DK_TS_HELPER_H__
#define __DK_TS_HELPER_H__

class dk_ts_helper {
public:
    dk_ts_helper();
    virtual ~dk_ts_helper();

public:
    bool safeEnterFunc() const;
    void safeExitFunc() const;
    void cancelAllAccess() const;
    HANDLE getExitEvent() const { return m_hTSEventExit; }

private:
    HANDLE m_hTSEventAccessSafe;
    HANDLE m_hTSEventExit;
    mutable DWORD m_dwTSFlag;
};

#endif //__DK_TS_HELPER_H__





















