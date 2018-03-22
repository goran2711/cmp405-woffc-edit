#pragma once

struct InputCommands
{
	bool forward;
	bool back;
	bool right;
	bool left;
	bool rotRight;
	bool rotLeft;

	long mouseDX;
	long mouseDY;

    long mouseX;
    long mouseY;

    bool leftMouseDown;
    bool rightMouseDown;

	struct
	{
		long x, y;
	} selectionRectangleBegin, selectionRectangleEnd;
};
