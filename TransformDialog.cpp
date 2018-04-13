#include "TransformDialog.h"
#include "stdafx.h"
#include "SceneObject.h"
#include <vector>
#include <cassert>

// Generates the C++ code necessary for a dynamic CObject-derived
// class with run-time access to the class name and position within the hierarchy
IMPLEMENT_DYNAMIC(TransformDialog, CDialogEx)

BEGIN_MESSAGE_MAP(TransformDialog, CDialogEx)
    // NOTE: There must be a better way of doing this
    ON_EN_CHANGE(IDC_EDIT_POSITION_X, OnControlChanged)
    ON_EN_CHANGE(IDC_EDIT_POSITION_Y, OnControlChanged)
    ON_EN_CHANGE(IDC_EDIT_POSITION_Z, OnControlChanged)
    ON_EN_CHANGE(IDC_EDIT_ROTATION_ROLL, OnControlChanged)
    ON_EN_CHANGE(IDC_EDIT_ROTATION_YAW, OnControlChanged)
    ON_EN_CHANGE(IDC_EDIT_ROTATION_PITCH, OnControlChanged)
    ON_EN_CHANGE(IDC_EDIT_SCALE_X, OnControlChanged)
    ON_EN_CHANGE(IDC_EDIT_SCALE_Y, OnControlChanged)
    ON_EN_CHANGE(IDC_EDIT_SCALE_Z, OnControlChanged)

    ON_COMMAND(IDOK, &TransformDialog::End)
END_MESSAGE_MAP()

TransformDialog::TransformDialog(CWnd * parent)
    :   CDialogEx(IDD_DIALOG_TRANSFORM, parent)
{
}

void TransformDialog::SetSceneObject(SceneObject * object)
{
    assert(object != nullptr);

    m_object = object;

    // Write values to dialog controls
    UpdateData(FALSE);
}

void TransformDialog::DoDataExchange(CDataExchange * pDX)
{
    CDialogEx::DoDataExchange(pDX);

    if (m_object)
    {
        DDX_Text(pDX, IDC_EDIT_POSITION_X, m_object->posX);
        DDX_Text(pDX, IDC_EDIT_POSITION_Y, m_object->posY);
        DDX_Text(pDX, IDC_EDIT_POSITION_Z, m_object->posZ);

        DDX_Text(pDX, IDC_EDIT_ROTATION_ROLL, m_object->rotX);
        DDX_Text(pDX, IDC_EDIT_ROTATION_YAW, m_object->rotY);
        DDX_Text(pDX, IDC_EDIT_ROTATION_PITCH, m_object->rotZ);

        DDX_Text(pDX, IDC_EDIT_SCALE_X, m_object->scaX);
        DDX_Text(pDX, IDC_EDIT_SCALE_Y, m_object->scaY);
        DDX_Text(pDX, IDC_EDIT_SCALE_Z, m_object->scaZ);
    }
}

void TransformDialog::OnControlChanged()
{
    // Read values from dialog controls
    if (m_object)
    {
        UpdateData(TRUE);
        m_controlsChanged = true;
    }
}

void TransformDialog::End()
{
    DestroyWindow();
}
