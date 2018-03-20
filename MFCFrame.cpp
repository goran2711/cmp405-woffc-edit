#include "MFCFrame.h"
#include "resource.h"



BEGIN_MESSAGE_MAP(CMyFrame, CFrameWnd)
	ON_WM_CREATE()
    ON_WM_WINDOWPOSCHANGED()
	ON_WM_SIZE()
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TOOL, &CMyFrame::OnUpdatePage)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_TOOL
};
//FRAME CLASS


//frame initialiser
CMyFrame::CMyFrame()
{
	m_selectionID = 999; //an obviously wrong selection ID,  to verify its working
}

void CMyFrame::SetCurrentSelectionID(int ID)
{
	m_selectionID = ID;
}

void CMyFrame::OnUpdatePage(CCmdUI * pCmdUI)
{
	pCmdUI->Enable();
	CString strPage;
	strPage.Format(_T("%d"), m_selectionID);
	pCmdUI->SetText(strPage);
}

//oncretae, called after init but before window is shown. 
int CMyFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1) return -1;

	// create a view to occupy the client area of the frame //This is where DirectX is rendered
	if (!m_DirXView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	m_menu1.LoadMenuW(IDR_MENU1);
	SetMenu(&m_menu1);

	if (!m_toolBar.CreateEx(this, TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_toolBar.LoadToolBar(IDR_TOOLBAR1))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CRect rect;
	GetClientRect(&rect);
	if (!m_wndStatusBar.Create(this))
	{
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_TOOL, SBPS_NORMAL, rect.Width() - 500);//set width of status bar panel

	return 0;
}

void CMyFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	// Forward this message to the CChildRender
	if (m_DirXView.toolSystem)
	{
		WPARAM wParam = nType;
		LPARAM lParam = LOWORD(cx) | HIWORD(cy);
		SendMessageA(m_DirXView.m_hWnd, WM_SIZE, wParam, lParam);
	}
}

void CMyFrame::OnWindowPosChanged(WINDOWPOS* newPos)
{
    CFrameWnd::OnWindowPosChanged(newPos);

    // Forward this message to the CChildRender
    // (In-depth reasoning in CChildRender::OnWindowPosChanged)
    if (m_DirXView.toolSystem)
    {
        LPARAM lParam = (LPARAM)newPos;
        SendMessageA(m_DirXView.m_hWnd, WM_WINDOWPOSCHANGED, NULL, lParam);
    }
}