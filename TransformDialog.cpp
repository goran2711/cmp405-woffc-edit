#include "TransformDialog.h"
#include "stdafx.h"
#include "SceneObject.h"
#include <vector>
#include <cassert>

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
        // Write values to dialog controls
        UpdateData(FALSE);
    }
}

void TransformDialog::SetUpdateCallback(UpdateObjectCallback callback)
{
    m_update = callback;
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

void TransformDialog::End()
{
    if (m_object)
    {
        // Read values from dialog controls
        UpdateData(TRUE);

        // Apply changes to display object as well
        m_update(m_object);
    }

    DestroyWindow();
}
