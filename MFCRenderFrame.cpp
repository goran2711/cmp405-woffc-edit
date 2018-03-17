
#include "MFCRenderFrame.h"
#include "ToolMain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildRender::CChildRender()
{
}

CChildRender::~CChildRender()
{
}


BEGIN_MESSAGE_MAP(CChildRender, CWnd)
	ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildRender::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), NULL);

	return TRUE;
}

void CChildRender::OnPaint()
{
	CPaintDC dc(this); // device context for painting
}

void CChildRender::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    if (toolSystem)
        toolSystem->OnResize(cx, cy);
}

void CChildRender::OnWindowPosChanged(WINDOWPOS*)
{
    if (toolSystem)
    {
        // The WM_WINDOWPOSCHANGED message is actually received by CMyFrame, but it is forwarded
        // to CChildRender because it is the frame that DirectX is rendering to, and the measurements
        // in ToolMain which we are interested in updating are related to the rendering area (CChildRender).

        // All of this could technically be done in CMyFrame::OnWindowPosChanged, but since it is the
        // window rect of the CChildRender's handle we are interested in, it makes more sense to have it here from a design perspective.
        // (The function parameter is being ignored because the message is forwarded from CMyFrame, and the measurements are therefore incorrect)

        RECT windowRect;
        GetWindowRect(&windowRect);

        WINDOWPOS newPos;
        newPos.x = windowRect.left;
        newPos.y = windowRect.top;
        newPos.cx = windowRect.right - windowRect.left;
        newPos.cy = windowRect.bottom - windowRect.top;

        toolSystem->OnPosChanged(newPos);
    }
}