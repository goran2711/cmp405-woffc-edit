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
    explicit TransformDialog(CWnd* parent = nullptr);

    // Dialog Triangle
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIALOG_TRANSFORM };
#endif

    void SetObjects(SceneObject* object);

    using UpdateObjectCallback = std::function<void(SceneObject*)>;
    // Set a callback that will be used to update the display object
    // (TransformDialog only changes SceneObject directly)
    void SetUpdateCallback(UpdateObjectCallback callback);

protected:
    // Allows us to get access to the dialog's resources
    virtual void DoDataExchange(CDataExchange* pDX) override;

private:
    afx_msg void End();

    SceneObject* m_object;
    UpdateObjectCallback m_update;

    DECLARE_MESSAGE_MAP()
};

