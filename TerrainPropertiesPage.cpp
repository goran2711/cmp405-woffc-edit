// TerrainPropertiesPage.cpp : implementation file
//

#include "stdafx.h"
#include "TerrainPropertiesPage.h"
#include "afxdialogex.h"
#include "resource.h"


// TerrainPropertiesPage dialog

IMPLEMENT_DYNAMIC(TerrainPropertiesPage, CDialogEx)

TerrainPropertiesPage::TerrainPropertiesPage(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_PROPPAGE_BRUSH, pParent)
{

}

BOOL TerrainPropertiesPage::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_sliderBrushSize.SetRangeMin(1);
    m_sliderBrushSize.SetRangeMax(127);

    m_sliderBrushForce.SetRangeMin(1);
    m_sliderBrushForce.SetRangeMax(10);
}

void TerrainPropertiesPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_SLIDER_BRUSHSIZE, m_sliderBrushSize);
    DDX_Control(pDX, IDC_SLIDER_BRUSHFORCE, m_sliderBrushForce);
}


BEGIN_MESSAGE_MAP(TerrainPropertiesPage, CDialogEx)
END_MESSAGE_MAP()


// TerrainPropertiesPage message handlers
