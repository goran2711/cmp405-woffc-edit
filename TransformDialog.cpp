#include "TransformDialog.h"
#include "stdafx.h"
#include "SceneObject.h"
#include <vector>

// Generates the C++ code necessary for a dynamic CObject-derived
// class with run-time access to the class name and position within the hierarchy
IMPLEMENT_DYNAMIC(TransformDialog, CDialogEx)

BEGIN_MESSAGE_MAP(TransformDialog, CDialogEx)
    ON_COMMAND(IDOK, &TransformDialog::End)
END_MESSAGE_MAP()

TransformDialog::TransformDialog(CWnd * parent)
    :   CDialogEx(IDD_DIALOG_TRANSFORM, parent)
{
}

void TransformDialog::SetObjects(SceneObject * object)
{
    assert(object != nullptr);

    m_object = object;

    if (m_object)
    {
        m_positionX.SetWindowTextW(std::to_wstring(m_object->posX).c_str());
        m_positionY.SetWindowTextW(std::to_wstring(m_object->posY).c_str());
        m_positionZ.SetWindowTextW(std::to_wstring(m_object->posZ).c_str());

        m_rotationR.SetWindowTextW(std::to_wstring(m_object->rotX).c_str());
        m_rotationY.SetWindowTextW(std::to_wstring(m_object->rotY).c_str());
        m_rotationP.SetWindowTextW(std::to_wstring(m_object->rotZ).c_str());

        m_scaleX.SetWindowTextW(std::to_wstring(m_object->scaX).c_str());
        m_scaleY.SetWindowTextW(std::to_wstring(m_object->scaY).c_str());
        m_scaleZ.SetWindowTextW(std::to_wstring(m_object->scaZ).c_str());
    }
}

void TransformDialog::SetUpdateCallback(UpdateObjectCallback callback)
{
    m_update = callback;
}

void TransformDialog::DoDataExchange(CDataExchange * pDX)
{
    CDialogEx::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_EDIT_POSITION_X, m_positionX);
    DDX_Control(pDX, IDC_EDIT_POSITION_Y, m_positionY);
    DDX_Control(pDX, IDC_EDIT_POSITION_Z, m_positionZ);

    DDX_Control(pDX, IDC_EDIT_ROTATION_ROLL, m_rotationR);
    DDX_Control(pDX, IDC_EDIT_ROTATION_YAW, m_rotationY);
    DDX_Control(pDX, IDC_EDIT_ROTATION_PITCH, m_rotationP);

    DDX_Control(pDX, IDC_EDIT_SCALE_X, m_scaleX);
    DDX_Control(pDX, IDC_EDIT_SCALE_Y, m_scaleY);
    DDX_Control(pDX, IDC_EDIT_SCALE_Z, m_scaleZ);
}

float TransformDialog::GetCEditTextAsFloat(const CEdit & cedit) const
{
    int length = cedit.GetWindowTextLengthW();

    std::vector<TCHAR> buf(length);
    cedit.GetWindowTextW(buf.data(), length);

    float num = wcstof(buf.data(), nullptr);
    return num;
}

void TransformDialog::End()
{
    if (m_object)
    {
        m_object->posX = GetCEditTextAsFloat(m_positionX);
        m_object->posY = GetCEditTextAsFloat(m_positionY);
        m_object->posZ = GetCEditTextAsFloat(m_positionZ);

        m_object->rotX = GetCEditTextAsFloat(m_rotationR);
        m_object->rotY = GetCEditTextAsFloat(m_rotationY);
        m_object->rotZ = GetCEditTextAsFloat(m_rotationP);

        m_object->scaX = GetCEditTextAsFloat(m_scaleX);
        m_object->scaY = GetCEditTextAsFloat(m_scaleY);
        m_object->scaZ = GetCEditTextAsFloat(m_scaleZ);
    }

    m_update(m_object);
    DestroyWindow();
}
