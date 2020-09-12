#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/Xinerama.h>
#include <sys/utsname.h>

Display* _display;

void error_exit(int ret)
{
	if (NULL != _display)
		XCloseDisplay(_display);
	exit(ret);
}

Window get_active_window(Window winroot)
{
	Atom atype;
	int format, status;
	unsigned char* props = NULL;
	unsigned long nitems, bytes_after;
	Atom afilter = XInternAtom(_display, "_NET_ACTIVE_WINDOW", True);
	status = XGetWindowProperty(_display, winroot, afilter, 0, LONG_MAX,
		False, AnyPropertyType, &atype, &format,
		&nitems, &bytes_after, &props);
	if (status != Success)
		error_exit(status);
	unsigned long longprop = *((long*)props);
	XFree(props);
	return longprop;
}

int is_window_maximized(Window window)
{
	Atom atype, aprop;
	int format, status;
	unsigned char* props = NULL;
	unsigned long nitems, bytes_after;
	Atom afilter = XInternAtom(_display, "_NET_WM_STATE", True);
	Atom amaxvert = XInternAtom(_display, "_NET_WM_STATE_MAXIMIZED_VERT", True);
	Atom amaxhorz = XInternAtom(_display, "_NET_WM_STATE_MAXIMIZED_HORZ", True);
	status = XGetWindowProperty(_display, window, afilter, 0, LONG_MAX,
		False, AnyPropertyType, &atype, &format,
		&nitems, &bytes_after, &props);
	if (status != Success)
		error_exit(status);
	int founds = 0;
	for (size_t i = 0; i < nitems; i++)
	{
		if (((Atom*)props)[i] == amaxvert)
		{
			founds++;
		}
		if (((Atom*)props)[i] == amaxhorz)
		{
			founds++;
		}
	}
	XFree(props);
	return (2 == founds) ? 1 : 0;
}

// wmstate 0 - remove
// wmstate 1 - add
// wmstate 2 - toggle
void maximize_window(Window window, long wmstate)
{
	Atom astate = XInternAtom(_display, "_NET_WM_STATE", True);
	Atom amaxvert = XInternAtom(_display, "_NET_WM_STATE_MAXIMIZED_VERT", True);
	Atom amaxhorz = XInternAtom(_display, "_NET_WM_STATE_MAXIMIZED_HORZ", True);
	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = window;
	xev.xclient.message_type = astate;
	xev.xclient.format = 32;
	// _NET_WM_STATE_REMOVE 0
	// _NET_WM_STATE_ADD 1
	// _NET_WM_STATE_TOGGLE 2
	xev.xclient.data.l[0] = wmstate;
	xev.xclient.data.l[1] = amaxhorz;
	xev.xclient.data.l[2] = amaxvert;
	XSendEvent(_display, DefaultRootWindow(_display), False, SubstructureNotifyMask, &xev);
}

int main(int argc, char** argv)
{
	int targetmon = 1;
	int actualmon = -1;
	int moncount = 0;
	unsigned long nitems, bytesafter;
	unsigned char* prop;
	int curx, cury;
	XWindowAttributes xwa;
	Window wchild;
	if (argc > 1)
	{
		int tm = atoi(argv[1]);
		if ((tm > 0) && (tm < 17))
			targetmon = tm;
	}
	_display = XOpenDisplay(NULL);
	if (NULL == _display)
		return EXIT_FAILURE;
	int screen = XDefaultScreen(_display);
	Window window = RootWindow(_display, screen);
	window = get_active_window(window);
	int ismax = is_window_maximized(window);
	XGetWindowAttributes(_display, window, &xwa);
	XTranslateCoordinates(_display, window, RootWindow(_display, screen),
		0, 0, &curx, &cury, &wchild);
	XineramaScreenInfo* xsi = XineramaQueryScreens(_display, &moncount);
	if (targetmon > moncount)
		error_exit(1);
	for (size_t i = 0; i < (size_t)moncount; i++)
	{
		if ((curx >= xsi[i].x_org) && (curx < (xsi[i].width + xsi[i].x_org)))
			if ((cury >= xsi[i].y_org) && (cury < (xsi[i].height + xsi[i].y_org)))
				actualmon = i+1;
	}
	if (actualmon == targetmon)
	{
		XCloseDisplay(_display);
		return EXIT_SUCCESS;
	}
	if (0 != ismax)
	{
		maximize_window(window, 0);
	}
	XMoveWindow(_display, window, xsi[targetmon-1].x_org, xwa.y);
	if (0 != ismax)
	{
		maximize_window(window, 1);
	}
	XFree(xsi);
	XCloseDisplay(_display);
	return EXIT_SUCCESS;
}