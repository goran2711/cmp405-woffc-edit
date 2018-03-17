#include "ToolMain.h"
#include "resource.h"
#include <vector>
#include <sstream>

#include <DirectXMath.h>

//ToolMain Class
ToolMain::ToolMain()
{
    m_currentChunk = 0;		//default value
    m_sceneGraph.clear();	//clear the vector for the scenegraph
    m_databaseConnection = NULL;

    //zero input commands
    ZeroMemory(&m_toolInputCommands, sizeof(InputCommands));
}


ToolMain::~ToolMain()
{
    sqlite3_close(m_databaseConnection);		//close the database connection
}


const std::vector<int> ToolMain::getCurrentSelectionIDs() const
{
    return m_selectedObjects;
}

void ToolMain::onActionInitialise(HWND handle, int width, int height)
{
    //FIX: This is a work-around to stop TransformDialog being left with an invalid pointer
    m_selectedObjects.reserve(256);

    //window size, handle etc for directX
    m_width = width;
    m_height = height;
    m_toolHandle = handle;
    m_d3dRenderer.Initialize(handle, m_width, m_height);

    // Initialise handle measurements
    GetWindowRect(m_toolHandle, &m_windowRect);
    m_windowRect.left += BORDER_OFFSET;
    m_windowRect.top += BORDER_OFFSET;
    m_windowRect.right -= BORDER_OFFSET;
    m_windowRect.bottom -= BORDER_OFFSET;

    GetClientRect(m_toolHandle, &m_dxClientRect);
    UpdateClientCenter();

    //database connection establish
    int rc;
    rc = sqlite3_open("database/test.db", &m_databaseConnection);

    if (rc)
    {
        TRACE("Can't open database");
        //if the database cant open. Perhaps a more catastrophic error would be better here
    }
    else
    {
        TRACE("Opened database successfully");
    }

    onActionLoad();
}

void ToolMain::onActionLoad()
{
    //load current chunk and objects into lists
    if (!m_sceneGraph.empty())		//is the vector empty
    {
        m_sceneGraph.clear();		//if not, empty it
    }

    //SQL
    int rc;
    char *sqlCommand;
    char *ErrMSG = 0;
    sqlite3_stmt *pResults;								//results of the query
    sqlite3_stmt *pResultsChunk;

    //OBJECTS IN THE WORLD
    //prepare SQL Text
    sqlCommand = "SELECT * from Objects";				//sql command which will return all records from the objects table.
    //Send Command and fill result object
    rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResults, 0);

    //loop for each row in results until there are no more rows.  ie for every row in the results. We create and object
    while (sqlite3_step(pResults) == SQLITE_ROW)
    {
        SceneObject newSceneObject;
        newSceneObject.ID = sqlite3_column_int(pResults, 0);
        newSceneObject.chunk_ID = sqlite3_column_int(pResults, 1);
        newSceneObject.model_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 2));
        newSceneObject.tex_diffuse_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 3));
        newSceneObject.posX = static_cast<float>(sqlite3_column_double(pResults, 4));
        newSceneObject.posY = static_cast<float>(sqlite3_column_double(pResults, 5));
        newSceneObject.posZ = static_cast<float>(sqlite3_column_double(pResults, 6));
        newSceneObject.rotX = static_cast<float>(sqlite3_column_double(pResults, 7));
        newSceneObject.rotY = static_cast<float>(sqlite3_column_double(pResults, 8));
        newSceneObject.rotZ = static_cast<float>(sqlite3_column_double(pResults, 9));
        newSceneObject.scaX = static_cast<float>(sqlite3_column_double(pResults, 10));
        newSceneObject.scaY = static_cast<float>(sqlite3_column_double(pResults, 11));
        newSceneObject.scaZ = static_cast<float>(sqlite3_column_double(pResults, 12));
        newSceneObject.render = (sqlite3_column_int(pResults, 13) != 0);
        newSceneObject.collision = (sqlite3_column_int(pResults, 14) != 0);
        newSceneObject.collision_mesh = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 15));
        newSceneObject.collectable = (sqlite3_column_int(pResults, 16) != 0);
        newSceneObject.destructable = (sqlite3_column_int(pResults, 17) != 0);
        newSceneObject.health_amount = sqlite3_column_int(pResults, 18);
        newSceneObject.editor_render = (sqlite3_column_int(pResults, 19) != 0);
        newSceneObject.editor_texture_vis = (sqlite3_column_int(pResults, 20) != 0);
        newSceneObject.editor_normals_vis = (sqlite3_column_int(pResults, 21) != 0);
        newSceneObject.editor_collision_vis = (sqlite3_column_int(pResults, 22) != 0);
        newSceneObject.editor_pivot_vis = (sqlite3_column_int(pResults, 23) != 0);
        newSceneObject.pivotX = static_cast<float>(sqlite3_column_double(pResults, 24));
        newSceneObject.pivotY = static_cast<float>(sqlite3_column_double(pResults, 25));
        newSceneObject.pivotZ = static_cast<float>(sqlite3_column_double(pResults, 26));
        newSceneObject.snapToGround = (sqlite3_column_int(pResults, 27) != 0);
        newSceneObject.AINode = (sqlite3_column_int(pResults, 28) != 0);
        newSceneObject.audio_path = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 29));
        newSceneObject.volume = static_cast<float>(sqlite3_column_double(pResults, 30));
        newSceneObject.pitch = static_cast<float>(sqlite3_column_double(pResults, 31));
        newSceneObject.pan = static_cast<float>(sqlite3_column_int(pResults, 32));
        newSceneObject.one_shot = (sqlite3_column_int(pResults, 33) != 0);
        newSceneObject.play_on_init = (sqlite3_column_int(pResults, 34) != 0);
        newSceneObject.play_in_editor = (sqlite3_column_int(pResults, 35) != 0);
        newSceneObject.min_dist = static_cast<int>(sqlite3_column_double(pResults, 36));
        newSceneObject.max_dist = static_cast<int>(sqlite3_column_double(pResults, 37));
        newSceneObject.camera = (sqlite3_column_int(pResults, 38) != 0);
        newSceneObject.path_node = (sqlite3_column_int(pResults, 39) != 0);
        newSceneObject.path_node_start = (sqlite3_column_int(pResults, 40) != 0);
        newSceneObject.path_node_end = (sqlite3_column_int(pResults, 41) != 0);
        newSceneObject.parent_id = sqlite3_column_int(pResults, 42);
        newSceneObject.editor_wireframe = (sqlite3_column_int(pResults, 43) != 0);
        newSceneObject.name = reinterpret_cast<const char*>(sqlite3_column_text(pResults, 44));

        //send completed object to scenegraph
        m_sceneGraph.push_back(newSceneObject);
    }

    //THE WORLD CHUNK
    //prepare SQL Text
    sqlCommand = "SELECT * from Chunks";				//sql command which will return all records from  chunks table. There is only one tho.
                                                        //Send Command and fill result object
    rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResultsChunk, 0);


    sqlite3_step(pResultsChunk);
    m_chunk.ID = sqlite3_column_int(pResultsChunk, 0);
    m_chunk.name = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 1));
    m_chunk.chunk_x_size_metres = sqlite3_column_int(pResultsChunk, 2);
    m_chunk.chunk_y_size_metres = sqlite3_column_int(pResultsChunk, 3);
    m_chunk.chunk_base_resolution = sqlite3_column_int(pResultsChunk, 4);
    m_chunk.heightmap_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 5));
    m_chunk.tex_diffuse_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 6));
    m_chunk.tex_splat_alpha_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 7));
    m_chunk.tex_splat_1_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 8));
    m_chunk.tex_splat_2_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 9));
    m_chunk.tex_splat_3_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 10));
    m_chunk.tex_splat_4_path = reinterpret_cast<const char*>(sqlite3_column_text(pResultsChunk, 11));
    m_chunk.render_wireframe = (sqlite3_column_int(pResultsChunk, 12) != 0);
    m_chunk.render_normals = (sqlite3_column_int(pResultsChunk, 13) != 0);
    m_chunk.tex_diffuse_tiling = sqlite3_column_int(pResultsChunk, 14);
    m_chunk.tex_splat_1_tiling = sqlite3_column_int(pResultsChunk, 15);
    m_chunk.tex_splat_2_tiling = sqlite3_column_int(pResultsChunk, 16);
    m_chunk.tex_splat_3_tiling = sqlite3_column_int(pResultsChunk, 17);
    m_chunk.tex_splat_4_tiling = sqlite3_column_int(pResultsChunk, 18);


    //Process REsults into renderable
    m_d3dRenderer.BuildDisplayList(&m_sceneGraph);
    //build the renderable chunk 
    m_d3dRenderer.BuildDisplayChunk(&m_chunk);

}

void ToolMain::onActionSave()
{
    //SQL
    int rc;
    char *sqlCommand;
    char *ErrMSG = 0;
    sqlite3_stmt *pResults;								//results of the query


    //OBJECTS IN THE WORLD Delete them all
    //prepare SQL Text
    sqlCommand = "DELETE FROM Objects";	 //will delete the whole object table.   Slightly risky but hey.
    rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand, -1, &pResults, 0);
    sqlite3_step(pResults);

    //Populate with our new objects
    std::wstring sqlCommand2;
    int numObjects = m_sceneGraph.size();	//Loop thru the scengraph.

    for (int i = 0; i < numObjects; i++)
    {
        std::stringstream command;
        command << "INSERT INTO Objects "
            << "VALUES(" << m_sceneGraph.at(i).ID << ","
            << m_sceneGraph.at(i).chunk_ID << ","
            << "'" << m_sceneGraph.at(i).model_path << "'" << ","
            << "'" << m_sceneGraph.at(i).tex_diffuse_path << "'" << ","
            << m_sceneGraph.at(i).posX << ","
            << m_sceneGraph.at(i).posY << ","
            << m_sceneGraph.at(i).posZ << ","
            << m_sceneGraph.at(i).rotX << ","
            << m_sceneGraph.at(i).rotY << ","
            << m_sceneGraph.at(i).rotZ << ","
            << m_sceneGraph.at(i).scaX << ","
            << m_sceneGraph.at(i).scaY << ","
            << m_sceneGraph.at(i).scaZ << ","
            << m_sceneGraph.at(i).render << ","
            << m_sceneGraph.at(i).collision << ","
            << "'" << m_sceneGraph.at(i).collision_mesh << "'" << ","
            << m_sceneGraph.at(i).collectable << ","
            << m_sceneGraph.at(i).destructable << ","
            << m_sceneGraph.at(i).health_amount << ","
            << m_sceneGraph.at(i).editor_render << ","
            << m_sceneGraph.at(i).editor_texture_vis << ","
            << m_sceneGraph.at(i).editor_normals_vis << ","
            << m_sceneGraph.at(i).editor_collision_vis << ","
            << m_sceneGraph.at(i).editor_pivot_vis << ","
            << m_sceneGraph.at(i).pivotX << ","
            << m_sceneGraph.at(i).pivotY << ","
            << m_sceneGraph.at(i).pivotZ << ","
            << m_sceneGraph.at(i).snapToGround << ","
            << m_sceneGraph.at(i).AINode << ","
            << "'" << m_sceneGraph.at(i).audio_path << "'" << ","
            << m_sceneGraph.at(i).volume << ","
            << m_sceneGraph.at(i).pitch << ","
            << m_sceneGraph.at(i).pan << ","
            << m_sceneGraph.at(i).one_shot << ","
            << m_sceneGraph.at(i).play_on_init << ","
            << m_sceneGraph.at(i).play_in_editor << ","
            << m_sceneGraph.at(i).min_dist << ","
            << m_sceneGraph.at(i).max_dist << ","
            << m_sceneGraph.at(i).camera << ","
            << m_sceneGraph.at(i).path_node << ","
            << m_sceneGraph.at(i).path_node_start << ","
            << m_sceneGraph.at(i).path_node_end << ","
            << m_sceneGraph.at(i).parent_id << ","
            << m_sceneGraph.at(i).editor_wireframe << ","
            << "'" << m_sceneGraph.at(i).name << "'"
            << ")";
        std::string sqlCommand2 = command.str();
        rc = sqlite3_prepare_v2(m_databaseConnection, sqlCommand2.c_str(), -1, &pResults, 0);
        sqlite3_step(pResults);
    }
    MessageBox(NULL, L"Objects Saved", L"Notification", MB_OK);
}

void ToolMain::onActionSaveTerrain()
{
    m_d3dRenderer.SaveDisplayChunk(&m_chunk);
}

void ToolMain::Tick(MSG *msg)
{
    //do we have a selection
    //do we have a mode
    //are we clicking / dragging /releasing
    //has something changed
        //update Scenegraph
        //add to scenegraph
        //resend scenegraph to Direct X renderer

    if (m_captureCursorThisFrame)
    {
        captureMouse(!m_cursorCaptured, (!m_cursorCaptured && m_captureCursorForCameraThisFrame));
        m_captureCursorThisFrame = false;
        m_captureCursorForCameraThisFrame = false;
    }

    if (m_cursorCaptured)
    {
        long mouseDX = m_clientCenter.x - m_cursorPos.x;
        long mouseDY = m_clientCenter.y - m_cursorPos.y;

        if (m_cursorControlsCamera)
        {
            m_toolInputCommands.mouseDX = mouseDX;
            m_toolInputCommands.mouseDY = mouseDY;
        }
        // if the cursor captured, but not controlling camera, it is moving objects along an axis
        else if (!m_selectedObjects.empty())
        {
            constexpr static float sensitivity = 0.25f;

            if (m_moveAxis != MoveAxis::AXIS_NONE)
            {
                // Move each selected object by some amount proportional to the cursor movement
                for (int id : m_selectedObjects)
                {
                    SceneObject* object = GetObjectFromID(id);

                    switch (m_moveAxis)
                    {
                        case MoveAxis::AXIS_X:
                            object->posX += (float) mouseDX * sensitivity;
                            break;
                        case MoveAxis::AXIS_Y:
                            object->posY += (float) mouseDY * sensitivity;
                            break;
                        case MoveAxis::AXIS_Z:
                            object->posZ += (float) mouseDX * sensitivity;
                            break;
                    }

                    UpdateDisplayObject(object);
                }
            }
        }

        // Move cursor back to the center of the screen
        POINT clientCenterScreen = m_clientCenter;
        ClientToScreen(m_toolHandle, &clientCenterScreen);

        SetCursorPos(clientCenterScreen.x, clientCenterScreen.y);
    }

    // TODO: if dragging mouse
    if (m_dragging)
    {
        m_toolInputCommands.selectionRectangleBegin = { m_beginDragPos.x, m_beginDragPos.y };
        m_toolInputCommands.selectionRectangleEnd = { m_currentDragPos.x, m_currentDragPos.y };
    }
    else
    {
        m_toolInputCommands.selectionRectangleBegin = m_toolInputCommands.selectionRectangleEnd = { -1, -1 };
    }

    // TODO: Come up with a better approach
    m_d3dRenderer.SetSelectionIDs(m_selectedObjects);

    //Renderer Update Call
    m_d3dRenderer.Tick(&m_toolInputCommands);

    // "Reset" input commands
    m_toolInputCommands.mouseDX = m_toolInputCommands.mouseDY = 0;
}

SceneObject * ToolMain::GetObjectFromID(int id)
{
    // FIX: This pointer could be invalidated if the scene graph grows
    auto object = std::find_if(m_sceneGraph.begin(), m_sceneGraph.end(),
                               [&](const auto& o) { return o.ID == id; });

    if (object == m_sceneGraph.cend())
        return nullptr;

    return &(*object);
}

void ToolMain::UpdateDisplayObject(SceneObject * sceneObject)
{
    m_d3dRenderer.UpdateDisplayListItem(*sceneObject);
}

bool ToolMain::UpdateInput(MSG * msg)
{
    switch (msg->message)
    {
        //Global inputs,  mouse position and keys etc
        case WM_KEYDOWN:
            m_keyArray[msg->wParam] = true;
            break;

        case WM_KEYUP:
            m_keyArray[msg->wParam] = false;
            break;

        case WM_LBUTTONDOWN:
            m_cursorPos.x = GET_X_LPARAM(msg->lParam);
            m_cursorPos.y = GET_Y_LPARAM(msg->lParam);

            m_beginDragPos = m_currentDragPos = { m_cursorPos.x, m_cursorPos.y };
            break;

        case WM_MOUSEMOVE:
            m_cursorPos.x = GET_X_LPARAM(msg->lParam);
            m_cursorPos.y = GET_Y_LPARAM(msg->lParam);

            // if the left mouse button is down (dragging) and the mouse is free
            if ((msg->wParam & MK_LBUTTON) != 0 && !m_cursorCaptured)
            {
                // Add a threshold for registering drag
                float distX = m_cursorPos.x - m_beginDragPos.x;
                float distY = m_cursorPos.y - m_beginDragPos.y;

                float dist = distX * distX + distY * distY;

                if (dist > DRAG_THRESHOLD)
                {
                    m_dragging = true;
                    m_currentDragPos = { m_cursorPos.x, m_cursorPos.y };
                    ClipCursor(&m_windowRect);
                }
            }
            // Right mouse button is down and the cursor is not controlling the camera
            else if ((msg->wParam & MK_RBUTTON) && !m_cursorControlsCamera)
            {
                bool isCtrlDown = (msg->wParam & MK_CONTROL);
                // msg->wParam does not indicate whether alt is pressed for some reason
                bool isAltDown = GetAsyncKeyState(VK_MENU);
                bool isShiftDown = (msg->wParam & MK_SHIFT);

                if (isCtrlDown)
                    m_moveAxis = MoveAxis::AXIS_X;
                else if (isAltDown)
                    m_moveAxis = MoveAxis::AXIS_Y;
                else if (isShiftDown)
                    m_moveAxis = MoveAxis::AXIS_Z;
                else
                    m_moveAxis = MoveAxis::AXIS_NONE;

            }
            // NOTE: Might need to make it so that this is run whenever L_BUTTON is not down
            else
                m_dragging = false;

            break;

        case WM_LBUTTONUP:
        {
            // Do box selection if the user has performed a drag action
            if (m_dragging)
            {
                long selRectBeginX = m_beginDragPos.x;
                long selRectBeginY = m_beginDragPos.y;
                long selRectEndX = m_currentDragPos.x;
                long selRectEndY = m_currentDragPos.y;

                RECT selectionRect =
                {
                    min(selRectBeginX, selRectEndX),	// left
                    min(selRectBeginY, selRectEndY),	// top
                    max(selRectBeginX, selRectEndX),	// right
                    max(selRectBeginY, selRectEndY)		// bottom
                };

                // determine picking mode
                Game::PickingMode mode = Game::PICK_NORMAL;

                // invert selection if ctrl is down
                if ((msg->wParam & MK_CONTROL) != 0)
                    mode = Game::PICK_INVERT;
                // exclusive (de-)selection if shift is down
                else if ((msg->wParam & MK_SHIFT) != 0)
                    mode = Game::PICK_EXCLUSIVE;

                // Call on the renderer to select within that rect for us
                m_d3dRenderer.PickWithinScreenRectangle(selectionRect, m_selectedObjects, mode);

                m_dragging = false;
                ClipCursor(nullptr);
            }
            // Do picking if this is a normal click
            else
            {
                // Get the ID of the object the user is clicking on (if any)
                int id = -1;
                if (m_d3dRenderer.Pick(m_cursorPos, m_dxClientRect, id))
                {
                    // Search for the picked object in our selected objects container
                    auto itID = std::find(m_selectedObjects.cbegin(), m_selectedObjects.cend(), id);

                    // If the object is already selected
                    if (itID != m_selectedObjects.cend())
                    {
                        // If control is down, unselect the object ...
                        if ((msg->wParam & MK_CONTROL) != 0)
                            m_selectedObjects.erase(itID);
                        // ... otherwise, make this the only selection
                        else
                        {
                            m_selectedObjects.clear();
                            m_selectedObjects.push_back(id);
                        }
                    }
                    // If the object is not yet selected
                    else
                    {
                        // Clear the selected objects if CTRL is not down
                        if ((msg->wParam & MK_CONTROL) == 0)
                            m_selectedObjects.clear();

                        m_selectedObjects.push_back(id);
                    }
                }
                //// FIX: Currently disabled because it causes selection to be immediately
                ////      cleared when the IDD_DIALOG1 (select) dialogue is up (WM_LBUTTONUP received)
                // Clear selections if no "selectable" object was clicked
                //else
                //	m_selectedObjects.clear();
            }
        }
        break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            // Capture/release the cursor when right mouse button is clicked/released
            if (!m_cursorControlsCamera)
                m_captureCursorThisFrame = true;
            break;
    }
    //here we update all the actual app functionality that we want.  This information will either be used int toolmain, or sent down to the renderer (Camera movement etc

    // Quit if escape is pressed
    if (m_keyArray[VK_ESCAPE])
        return false;

    // NOTE: Do I want to move this somewhere else?
    // Lock mouse for camera control if space is pressed
    bool cursorCapturedByRightClick = (m_cursorCaptured && !m_cursorControlsCamera);
    if (m_keyArray[VK_SPACE] && !cursorCapturedByRightClick)
    {
        // UpdateInput is called several times per frame; by delaying the call to
        // captureCursor, there is no chance that the cursor moves between us capturing it
        // and calculating the delta movement (it would cause snapping)
        m_captureCursorThisFrame = true;
        m_captureCursorForCameraThisFrame = true;

        // TODO: INVESTIGATE
        // Because for some reason WM_KEYUP doesn't seem to do the job?
        //  Simulates KEY PRESS instead of KEY DOWN
        m_keyArray[' '] = false;
    }

    // Delete selected object(s)
    if (m_keyArray[VK_DELETE])
    {
        OnDelete();
        
        m_keyArray[VK_DELETE] = false;
    }

    // Undo
    if (GetAsyncKeyState(VK_CONTROL) && m_keyArray['Z'])
    {
        OnCtrlZ();

        m_keyArray['Z'] = false;
    }

    // Redo
    if (GetAsyncKeyState(VK_CONTROL) && m_keyArray['Y'])
    {
        OnCtrlY();

        m_keyArray['Y'] = false;
    }

    //WASD movement
    m_toolInputCommands.forward = m_keyArray['W'];
    m_toolInputCommands.back = m_keyArray['S'];
    m_toolInputCommands.left = m_keyArray['A'];
    m_toolInputCommands.right = m_keyArray['D'];
    m_toolInputCommands.rotRight = m_keyArray['E'];
    m_toolInputCommands.rotLeft = m_keyArray['Q'];

    return true;
}

void ToolMain::OnResize(int width, int height)
{
    m_dxClientRect.right = width;
    m_dxClientRect.bottom = height;

    UpdateClientCenter();

    m_d3dRenderer.OnWindowSizeChanged(width, height);
}

void ToolMain::OnPosChanged(WINDOWPOS newPos)
{
    m_windowRect.left = newPos.x + BORDER_OFFSET;
    m_windowRect.top = newPos.y + BORDER_OFFSET;
    m_windowRect.right = newPos.x + newPos.cx - BORDER_OFFSET;
    m_windowRect.bottom = newPos.y + newPos.cy - BORDER_OFFSET;
}

void ToolMain::captureMouse(bool val, bool forFPSCamera)
{
    m_cursorCaptured = val;
    m_cursorControlsCamera = (m_cursorCaptured && forFPSCamera);

    // Show/hide cursor
    ShowCursor(!m_cursorCaptured);

    if (m_cursorCaptured)
    {
        // Lock the cursor to the dx render area
        ClipCursor(&m_windowRect);

        // Store the cursor position (in screen coordinates) so that we can move it back when they unlock the cursor
        m_lastCursorPos = m_cursorPos;
        ClientToScreen(m_toolHandle, &m_lastCursorPos);

        // Move the cursor to the center of the screen
        POINT clientCenterScreen = m_clientCenter;
        ClientToScreen(m_toolHandle, &clientCenterScreen);

        SetCursorPos(clientCenterScreen.x, clientCenterScreen.y);

        // Overwrite the mouse coordinates found in input handling to avoid snapping
        m_cursorPos = m_clientCenter;
    }
    else
    {
        // Move the cursor back where we found it
        SetCursorPos(m_lastCursorPos.x, m_lastCursorPos.y);

        // "Unlock" the cursor
        ClipCursor(nullptr);
    }
}

void ToolMain::UpdateClientCenter()
{
    m_clientCenter = CalculateCenter(m_dxClientRect);
}

POINT ToolMain::CalculateCenter(const RECT & rect) const
{
    return{ rect.right / 2, rect.bottom / 2 };
}

bool ToolMain::DeleteSceneObjects(const std::vector<int>& objectIDs)
{
    std::vector<SceneObject> deletedObjects;

    bool deletedAnything = false;
    for (int id : objectIDs)
    {
        for (auto it = m_sceneGraph.cbegin(); it != m_sceneGraph.cend();)
        {
            // Erase this item only if the id matches
            if (it->ID != id)
                ++it;
            else
            {
                deletedAnything = true;

                // Save for posterity
                deletedObjects.push_back(*it);

                // Remove item
                m_d3dRenderer.RemoveDisplayListItem(id);
                it = m_sceneGraph.erase(it);
            }
        }
    }

    m_deleteHistory.push_back(deletedObjects);
    return deletedAnything;
}

void ToolMain::OnDelete()
{
    if (!m_selectedObjects.empty())
    {
        DeleteSceneObjects(m_selectedObjects);
        m_selectedObjects.clear();

        // The user has performed a new action, so let's forget our redo history
        m_redoHistory.clear();
    }
}

void ToolMain::OnCtrlZ()
{
    if (!m_deleteHistory.empty())
    {
        // Add the previous item(s) back into the scene graph
        std::vector<SceneObject> objectsToRestore = m_deleteHistory.back();
        m_deleteHistory.pop_back();

        std::vector<int> restoredObjectIDs;
        restoredObjectIDs.reserve(objectsToRestore.size());
        for (auto object : objectsToRestore)
        {
            m_d3dRenderer.AddDisplayListItem(object);
            m_sceneGraph.push_back(object);

            restoredObjectIDs.push_back(object.ID);
        }

        m_redoHistory.push_back(restoredObjectIDs);
    }
}

void ToolMain::OnCtrlY()
{
    if (!m_redoHistory.empty())
    {
        std::vector<int> objectsToDelete = m_redoHistory.back();
        m_redoHistory.pop_back();

        DeleteSceneObjects(objectsToDelete);
    }
}
