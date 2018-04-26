#pragma once
#include "afxdialogex.h"
#include "afxwin.h"
#include "resource.h"

// IDEAS: Make it rebuild display list when OK is pressed? Or whenever some 'Apply' button is pressed

class SceneObject;

class TransformDialog : public CDialogEx
{
    // Adds the ability to access run-time information about an object's class when deriving a class from CObject
    DECLARE_DYNAMIC(TransformDialog)

public:
    explicit TransformDialog(CWnd* parent = nullptr);

    // Dialog Triangle
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIALOG_TRANSFORM };
#endif

    void SetSceneObject(SceneObject* object);

    void NotifyDisplayObjectUpdated() { m_controlsChanged = false; }
    bool ControlsChanged() const { return m_controlsChanged; }

    const SceneObject* GetSceneObject() const { return m_object; }

protected:
    // Allows us to get access to the dialog's resources
    virtual void DoDataExchange(CDataExchange* pDX) override;

private:
    afx_msg void OnControlChanged();
    afx_msg void End();

    SceneObject* m_object;
    bool m_controlsChanged = false;

    DECLARE_MESSAGE_MAP()
};

