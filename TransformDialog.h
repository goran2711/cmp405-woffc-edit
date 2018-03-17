#pragma once
#include "afxdialogex.h"
#include "afxwin.h"
#include "resource.h"
#include <functional>

// IDEAS: Make it rebuild display list when OK is pressed? Or whenever some 'Apply' button is pressed

class SceneObject;

class TransformDialog : public CDialogEx
{
    // Adds the ability to access run-time information about an object's class when deriving a class from CObject
    DECLARE_DYNAMIC(TransformDialog)

public:
    using UpdateObjectCallback = std::function<void(SceneObject*)>;

    explicit TransformDialog(CWnd* parent = nullptr);

    // Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIALOG_TRANSFORM };
#endif

    void SetObjects(SceneObject* object);
    void SetUpdateCallback(UpdateObjectCallback callback);

protected:
    // Allows us to get access to the dialog's resources
    virtual void DoDataExchange(CDataExchange* pDX) override;

private:
    float GetCEditTextAsFloat(const CEdit& cedit) const;

    afx_msg void End();

    // TODO: Extend this to allow multiple objects to be transformed at once
    SceneObject* m_object;
    UpdateObjectCallback m_update;

    CEdit m_positionX, m_positionY, m_positionZ;
    CEdit m_rotationR, m_rotationY, m_rotationP;
    CEdit m_scaleX, m_scaleY, m_scaleZ;

    DECLARE_MESSAGE_MAP()
};

