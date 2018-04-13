#pragma once

#include <afxext.h>
#include "pch.h"
#include "Game.h"
#include "sqlite3.h"
#include "SceneObject.h"
#include "InputCommands.h"
#include <vector>

class ToolMain
{
	// Distance (square) before a drag action is interpreted
	constexpr static float DRAG_THRESHOLD = 256.f;

    // 2px offset so the mouse won't overlap with the edge borders
    // (WM_MOUSEMOVE is not sent when mouse overlaps border--annoying when cursor is clipped)
    constexpr static long BORDER_OFFSET = 2;

public:
    // ctors/dtor
	ToolMain();
	ToolMain(const ToolMain&) = delete;
	ToolMain& operator=(const ToolMain&) = delete;
	~ToolMain();

    // functions
	//onAction - These are the interface to MFC
	const std::vector<int> ToolMain::getCurrentSelectionIDs() const;		//returns the selection number of currently selected object so that It can be displayed.
	void	onActionInitialise(HWND handle, int width, int height);			//Passes through handle and hieght and width and initialises DirectX renderer and SQL LITE
	void	onActionFocusCamera();
	void	onActionLoad();													//load the current chunk
	afx_msg	void	onActionSave();											//save the current chunk
	afx_msg void	onActionSaveTerrain();									//save chunk geometry

	void	Tick(MSG *msg);
	bool	UpdateInput(MSG *msg);

    void    OnResize(int width, int height);
    void    OnPosChanged(WINDOWPOS newPos);

    void    OnResizeOrPositionChanged();

    SceneObject* GetObjectFromID(int id);

    void    UpdateDisplayObject(const SceneObject* sceneObject);

    void    ToggleBrush();

    bool    ObjectMovedThisFrame() const { return m_objectHasBeenMoved; }

    //variables
	std::vector<SceneObject>    m_sceneGraph;	//our scenegraph storing all the objects in the current chunk

    // TODO: Handle more than just deletion--prob need some custom structure to handle that ...
    //       ... callback with pre-defined redo-functions, perhaps? (e.g. a struct that stores the items, and some
    //       operator() that will put things back / restore, etc.
    std::vector<std::vector<SceneObject>>    m_deleteHistory;
    std::vector<std::vector<int>>            m_redoHistory;

	ChunkObject					m_chunk;		//our landscape chunk
	std::vector<int> m_selectedObjects;						//ID of current Selection

private:
    // functions
	void	onContentAdded();
	void	captureCursor(bool val, bool forFPSCamera);

	void	UpdateClientCenter();

	POINT	CalculateCenter(const RECT& rect) const;

    bool    DeleteSceneObjects(const std::vector<int>& objectIDs);

    void    OnDelete();
    void    OnCtrlZ();
    void    OnCtrlY();
    void    OnCtrlC();
    void    OnCtrlV();
			
    // variables
	HWND	m_toolHandle;		//Handle to the  window
	Game	m_d3dRenderer;		//Instance of D3D rendering system for our tool
	InputCommands m_toolInputCommands;		//input commands that we want to use and possibly pass over to the renderer
	bool	m_keyArray[256];
	sqlite3 *m_databaseConnection;	//sqldatabase handle

	int m_width;		//dimensions passed to directX
	int m_height;
	int m_currentChunk;			//the current chunk of thedatabase that we are operating on.  Dictates loading and saving. 
	
	POINT m_clientCenter{ 0, 0 };
	POINT m_lastCursorPos{ 0, 0 }, m_cursorPos{ 0, 0 };
	RECT m_windowRect, m_dxClientRect;

    bool m_captureCursorThisFrame = false;
    bool m_captureCursorForCameraThisFrame = false;
    bool m_cursorCaptured = false;
    bool m_cursorControlsCamera = false;

    bool m_leftMouseBtnDown = false;
    bool m_rightMouseBtnDown = false;

    enum MoveAxis
    {
        AXIS_NONE = 0,
        AXIS_X,
        AXIS_Y,
        AXIS_Z
    } m_moveAxis;

    bool m_objectHasBeenMoved = false;

	// rectangle selection (rts style)
	bool m_dragging = false;
	POINT m_beginDragPos;
	POINT m_currentDragPos;

    bool m_brushActive = false;
    float m_brushSize = 32.f;
    float m_brushForce = 1.25f;

    DirectX::XMFLOAT3 m_terrainManipPosition;
    bool m_updateTerrainManipPosition = false;

    std::vector<int> m_clipboard;
};
