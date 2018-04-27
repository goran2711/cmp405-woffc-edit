#include "MFCMain.h"
#include "resource.h"


BEGIN_MESSAGE_MAP(MFCMain, CWinApp)
    ON_COMMAND(ID_FILE_QUIT, &MFCMain::MenuFileQuit)
    ON_COMMAND(ID_FILE_SAVETERRAIN, &MFCMain::MenuFileSaveTerrain)
    ON_COMMAND(ID_EDIT_SELECT, &MFCMain::MenuEditSelect)
    ON_COMMAND(ID_EDIT_TRANSFORM, &MFCMain::MenuEditTransform)
	ON_COMMAND(ID_BUTTON_SAVE, &MFCMain::ToolBarButton1)
    ON_COMMAND(ID_BUTTON_BRUSH, &MFCMain::ToolBarToggleBrush)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TOOL, &CMyFrame::OnUpdatePage)
END_MESSAGE_MAP()

BOOL MFCMain::InitInstance()
{
	//instanciate the mfc frame
	m_frame = new CMyFrame();
	m_pMainWnd = m_frame;

	m_frame->Create(NULL,
		_T("World Of Flim-Flam Craft Editor"),
		WS_OVERLAPPEDWINDOW,
		CRect(100, 100, 1024, 768),
		NULL,
		NULL,
		0,
		NULL
	);
	m_frame->ShowWindow(SW_SHOW);
	m_frame->UpdateWindow();

	//get the rect from the MFC window so we can get its dimensions
//	m_toolHandle = Frame->GetSafeHwnd();						//handle of main window
	m_toolHandle = m_frame->m_DirXView.GetSafeHwnd();				//handle of directX child window
	m_frame->m_DirXView.GetClientRect(&WindowRECT);
	m_width = WindowRECT.Width();
	m_height = WindowRECT.Height();

	m_ToolSystem.onActionInitialise(m_toolHandle, m_width, m_height);

	m_frame->m_DirXView.toolSystem = &m_ToolSystem;

	return TRUE;
}

int MFCMain::Run()
{
	MSG msg;
	BOOL bGotMsg;

	PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);

	while (WM_QUIT != msg.message)
	{
		if (true)
		{
			bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);
		}
		else
		{
			bGotMsg = (GetMessage(&msg, NULL, 0U, 0U) != 0);
		}

		if (bGotMsg)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

            // HACK: Avoids issue where messages meant for the toolbar or other dialogs is sent to the tool system
            if (msg.hwnd == m_frame->GetSafeHwnd() || msg.hwnd == m_toolHandle)
            {
                if (!m_ToolSystem.UpdateInput(&msg))
                    break;
            }
		}
		else
		{
			auto selectionIDs = m_ToolSystem.getCurrentSelectionIDs();

			// TODO: Deal with cases where _A LOT_ of object are selected (so many that we don't want to show all the IDs
			//       in the status bar)
			std::wstring statusString = L"Selected objects: ";
			if (selectionIDs.empty())
				statusString += L"none";
			else
			{
                // Fix language
                if (selectionIDs.size() == 1)
                {
                    int idx = statusString.find_last_of(L's');
                    statusString.erase(statusString.begin() + idx);
                }

				for (int i : selectionIDs)
				{
					statusString += std::to_wstring(i);
					statusString += L", ";
				}
				// chop off the last ", "
				statusString.resize(statusString.size() - 2);
			}

			m_ToolSystem.Tick(&msg);

            // Update the values displayed in the transform dialog
            // NOTE: This works because CWnd allows for implicit conversion to HANDLE, which is just a typedef of void*
            if (m_transformDialogue)
            {
                // If an object has been moved by the user, update the controls
                if (m_ToolSystem.ObjectMovedThisFrame())
                    m_transformDialogue.UpdateData(FALSE);

                // If the controls have changed, update the scene graph and display list
                if (m_transformDialogue.ControlsChanged())
                {
                    m_ToolSystem.UpdateDisplayObject(m_transformDialogue.GetSceneObject());
                    m_transformDialogue.NotifyDisplayObjectUpdated();
                }
            }

			//send current object ID to status bar in The main frame
			m_frame->m_wndStatusBar.SetPaneText(1, statusString.c_str(), 1);
		}
	}

	return (int)msg.wParam;
}

void MFCMain::MenuFileQuit()
{
	//will post message to the message thread that will exit the application normally
	PostQuitMessage(0);
}

void MFCMain::MenuFileSaveTerrain()
{
	m_ToolSystem.onActionSaveTerrain();
}

void MFCMain::MenuEditSelect()
{
	//SelectDialogue m_ToolSelectDialogue(NULL, &m_ToolSystem.m_sceneGraph);		//create our dialoguebox //modal constructor
	//m_ToolSelectDialogue.DoModal();	// start it up modal

	//modeless dialogue must be declared in the class.   If we do local it will go out of scope instantly and destroy itself
	m_ToolSelectDialogue.Create(IDD_DIALOG1);	//Start up modeless
	m_ToolSelectDialogue.ShowWindow(SW_SHOW);	//show modeless
	m_ToolSelectDialogue.SetObjectData(&m_ToolSystem.m_sceneGraph, &m_ToolSystem.m_selectedObjects);
}

void MFCMain::MenuEditTransform()
{
    if (m_ToolSystem.m_selectedObjects.empty())
        MessageBox(m_toolHandle, L"No object selected", L"Error", MB_OK);
	else if (m_ToolSystem.m_selectedObjects.size() > 1)
		MessageBox(m_toolHandle, L"Can only manipulate one object at a time", L"Error", MB_OK);
    else
    {
        m_transformDialogue.Create(IDD_DIALOG_TRANSFORM);
        m_transformDialogue.ShowWindow(SW_SHOW);

        // Transform the first selected object (there should only ever be one selected)
        SceneObject* object = m_ToolSystem.GetObjectFromID(m_ToolSystem.getCurrentSelectionIDs().front());
        m_transformDialogue.SetSceneObject(object);
    }
}

void MFCMain::ToolBarButton1()
{
	m_ToolSystem.onActionSave();
}

void MFCMain::ToolBarToggleBrush()
{
#ifdef DEBUG
    MessageBox(m_toolHandle, L"Terrain manipulation is slow in debug mode", L"Warning", MB_OK);
#endif

    m_ToolSystem.ToggleBrush();
}
