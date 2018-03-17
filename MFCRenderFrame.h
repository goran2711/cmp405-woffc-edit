#pragma once
#include <afxwin.h> 
#include <afxext.h>

class ToolMain;

// CChildView window

class CChildRender : public CWnd
{
	// Construction
public:
	CChildRender();

	// Attributes
public:

	ToolMain* toolSystem = nullptr;

	// Operations
public:

	// Overrides
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// Implementation
public:
	virtual ~CChildRender();

	// Generated message map functions
protected:
	afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnWindowPosChanged(WINDOWPOS*);
	
	DECLARE_MESSAGE_MAP()
};

