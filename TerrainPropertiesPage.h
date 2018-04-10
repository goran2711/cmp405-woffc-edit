#pragma once
#include "afxcmn.h"


// TerrainPropertiesPage dialog

class TerrainPropertiesPage : public CDialogEx
{
	DECLARE_DYNAMIC(TerrainPropertiesPage)

public:
	TerrainPropertiesPage(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROPPAGE_BRUSH };
#endif

    BOOL OnInitDialog() override;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    CSliderCtrl m_sliderBrushSize;
    CSliderCtrl m_sliderBrushForce;

	DECLARE_MESSAGE_MAP()
};
