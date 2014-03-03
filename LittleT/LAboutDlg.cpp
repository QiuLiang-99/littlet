#include "LAboutDlg.h"
#include "../common/LittleTcmn.h"
#include "crypt/CXUnzip.h"
#include "sys/Regkey.h"
#include "LittleT.h"

QUI_BEGIN_EVENT_MAP(LAboutDlg, QDialog)
    BN_CLICKED_NAME(L"a-link", &LAboutDlg::OnClkLink)
QUI_END_EVENT_MAP()

LAboutDlg::LAboutDlg(void)
    :QDialog(L"qabs:misc/about.htm")
{
}

void LAboutDlg::OnClkLink( HELEMENT he )
{
    QString sHref = ECtrl(he).get_attribute("href");
    if (!sHref.IsEmpty())
    {
        ShellExecute(NULL, L"open", sHref, NULL, NULL, SW_SHOW);
    }
}

void LAboutDlg::ShowModal()
{
    LAboutDlg *pDlg = LAboutDlg::GetInstance();
    if (!pDlg->IsWindow())
    {
        pDlg->DoModal();
    }
}


//////////////////////////////////////////////////////////////////////////
LUpdateInfoDlg::LUpdateInfoDlg( LPCWSTR szUpdateFile )
    :QDialog(L"qabs:misc/update.htm")
{
    m_sZipFile = szUpdateFile;
}

LRESULT LUpdateInfoDlg::OnDocumentComplete()
{
    CXUnzip theZip;
    if (!theZip.Open(m_sZipFile))
        return 0;

    QBuffer bufTxt;
    if (theZip.UnzipToBuffer(LITTLET_UPDATE_HISTORY_FILENEWNAME, bufTxt))
    {
        GetCtrl("#box-content").set_html(
            bufTxt.GetBuffer(), bufTxt.GetBufferLen());
    }
    theZip.Close();

    return 0;
}

//////////////////////////////////////////////////////////////////////////
QUI_BEGIN_EVENT_MAP(LSettingDlg, QDialog)
    BN_CLICKED_ID(L"chk_cmn_autorun",&LSettingDlg::OnCmnChkAutoRun)
QUI_END_EVENT_MAP()

static LPCWSTR SC_PSZ_AUTORUN_REGPATH = 
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
//    "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run";

// static const char* SC_PSZ_AUTORUN_PROGPATH = 
//     "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run";

static LPCWSTR SC_PSZ_AUTORUN_ITEMNAME = L"LittleT";

LSettingDlg::LSettingDlg()
    :QDialog(L"qabs:misc/setting.htm")
{

}

void LSettingDlg::ShowModal()
{
    LSettingDlg *pDlg = LSettingDlg::GetInstance();
    if (!pDlg->IsWindow())
    {
        pDlg->DoModal();
    }
}

LRESULT LSettingDlg::OnDocumentComplete()
{
    //  休息一会儿
    LittleTConfig *pCfg = (LittleTConfig*)QUIGetConfig();
    _BreakSec().SetInt(pCfg->GetBreakSec());
    _BreakSpan().SetInt(pCfg->GetPicSec());
    _BreakAlpha().SetInt(pCfg->GetPicAlpha());
    _BreakFolder().SetFolderPath(pCfg->GetPicFolder());
//    _BreakColor().SetCOLORREF(pCfg->GetPicBkcolor());

    _AutoRun().SetCheck(IsStartupShortcutExist());

    return 0;
}

WString LSettingDlg::_KeyPath()
{
    WString sRet;
    sRet.Format(L"%s\\%s",SC_PSZ_AUTORUN_REGPATH, SC_PSZ_AUTORUN_ITEMNAME);
    return sRet;
}

WString LSettingDlg::_ProgPath()
{
    // 需要加上双引号
    TCHAR path[MAX_PATH] = {0};
    GetModuleFileName(NULL, path, MAX_PATH);
    QString sRet;
    sRet.Format(L"\"%s\" /normal", path);
    return sRet;
}

BOOL LSettingDlg::OnDefaultButton( INT_PTR nBtn )
{
    if (IDOK != nBtn)
        return TRUE;

    //  休息一会儿
    LittleTConfig *pCfg = (LittleTConfig*)QUIGetConfig();
    pCfg->SetBreakFolder(_BreakFolder().GetFolderPath());
    pCfg->SetBreakSpan(_BreakSpan().GetInt());
    pCfg->SetBreakSec(_BreakSec().GetInt());
    pCfg->SetBreakAlpha(_BreakAlpha().GetInt());
//    pCfg->SetBreakBkcolor(_BreakColor().GetCOLORREF());

//     if (_AutoRun().IsChecked())
//     {
//         m_reg.Write(SC_PSZ_AUTORUN_ITEMNAME, _ProgPath());
//     }
//     else
//     {
//         m_reg.Delete(HKEY_CURRENT_USER, _KeyPath());
//     }


    return TRUE;
}

void LSettingDlg::OnCmnChkAutoRun( HELEMENT he)
{
    ECheck chkAutoRun(he);

    BOOL bOk = FALSE;
    QString sStartupDir;
    if ( quibase::GetSpeialPath(CSIDL_STARTUP, sStartupDir) )
    {
        if (chkAutoRun.IsChecked())
        {
            wchar_t szModuleName[1024] = {0};
            GetModuleFileName(NULL,szModuleName, 1024);
            ASSERT(FALSE);
            //bOk = quibase::CreateShortcut(szModuleName, sStartupDir);
        }
        else
        {
            QString sShortcut = sStartupDir + L"\\" + quibase::GetModuleName(FALSE) + L".lnk";
            bOk = DeleteFile(sShortcut);
            if (!bOk)
            {
                bOk = IsStartupShortcutExist();
            }
        }
    }

    if (!bOk)
    {
        chkAutoRun.SetCheck( !chkAutoRun.IsChecked() );

        chkAutoRun.ShowTooltip(L"操作失败");
    }
}

BOOL LSettingDlg::IsStartupShortcutExist()
{
    QString sPath;
    if (quibase::GetSpeialPath(CSIDL_STARTUP, sPath))
    {
        sPath += L"\\" + quibase::GetModuleName(FALSE) + L".lnk";
        return quibase::IsFileExist(sPath);
    }
    return FALSE;
}

