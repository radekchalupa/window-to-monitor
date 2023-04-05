#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/extensions/Xinerama.h>
#include <sys/utsname.h>

static Display* _display;
static XineramaScreenInfo* _xsi = NULL;

static void cleanup(void)
{
	if (NULL != _xsi)
	{
		XFree(_xsi);
		_xsi = NULL;
	}
	if (NULL != _display)
	{
		XCloseDisplay(_display);
		_display = NULL;
	}
}

static void error_exit(int ret)
{
	cleanup();
	exit(ret);
}

static Window get_active_window(Window winroot)
{
	Atom atype;
	int format, status;
	unsigned char* props = NULL;
	unsigned long nitems, bytes_after;
	unsigned long longprop;
	Atom afilter;
	afilter = XInternAtom(_display, "_NET_ACTIVE_WINDOW", True);
	status = XGetWindowProperty(_display, winroot, afilter, 0, LONG_MAX,
		False, AnyPropertyType, &atype, &format,
		&nitems, &bytes_after, &props);
	if (status != Success)
		error_exit(status);
	longprop = *((long*)props);
	XFree(props);
	return longprop;
}

static int is_window_maximized(Window window)
{
	Atom atype;
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

static void maximize_window(Window window, long wmstate)
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
	xev.xclient.data.l[0] = wmstate;
	xev.xclient.data.l[1] = amaxhorz;
	xev.xclient.data.l[2] = amaxvert;
	XSendEvent(_display, DefaultRootWindow(_display), False,
		SubstructureNotifyMask, &xev);
}

static void get_window_position(Window window, int screen, int* x, int* y,
	int* w, int* h)
{
	XWindowAttributes xwa;
	XGetWindowAttributes(_display, window, &xwa);
	*x = xwa.x;
	*y = xwa.y;
	*w = xwa.width;
	*h = xwa.height;
	Window wchild;
	XTranslateCoordinates(_display, window, RootWindow(_display, screen),
		0, 0, x, y, &wchild);
	Atom atype;
	int format;
	unsigned long nitems, bytesafter;
	unsigned char* props;
	Atom atom = XInternAtom(_display, "_NET_FRAME_EXTENTS", True);
	Status status = XGetWindowProperty(_display, window, atom, 0, LONG_MAX,
		False, AnyPropertyType, &atype, &format,
		&nitems, &bytesafter, &props);
	if (Success != status)
		return;
	*x -= *((unsigned long*)props);
	*y -= *((unsigned long*)props + 2);
	*w += *((unsigned long*)props) + *((unsigned long*)props + 1);
	*h += *((unsigned long*)props + 2) + *((unsigned long*)props + 3);
	XFree(props);
}

int main(int argc, char** argv)
{
	int screen;
	Window window;
	int targetmon = 1;
	int actualmon = -1;
	int moncount = 0;
	int curx, cury, curw, curh;
	int ismax;
	if (argc > 1)
	{
		int tm = atoi(argv[1]);
		if ((tm > 0) && (tm < 17))
			targetmon = tm;
	}
	_display = XOpenDisplay(NULL);
	if (NULL == _display)
		return EXIT_FAILURE;
	screen = XDefaultScreen(_display);
	window = RootWindow(_display, screen);
	window = get_active_window(window);
	if (0 == window)
		error_exit(EXIT_FAILURE);
	ismax = is_window_maximized(window);
	get_window_position(window,  screen, &curx, &cury, &curw, &curh);
	_xsi = XineramaQueryScreens(_display, &moncount);
	if (targetmon > moncount)
		error_exit(1);
	for (size_t i = 0; i < (size_t)moncount; i++)
	{
		if ((curx >= _xsi[i].x_org) && (curx < (_xsi[i].width + _xsi[i].x_org)))
			if ((cury >= _xsi[i].y_org) && (cury < (_xsi[i].height + _xsi[i].y_org)))
				actualmon = i+1;
	}
	if (actualmon == targetmon)
	{
		cleanup();
		return EXIT_SUCCESS;
	}
	if (0 != ismax)
		maximize_window(window, 0);
	int targetx = _xsi[targetmon-1].x_org + curx - _xsi[actualmon-1].x_org;
	int targety = cury;
	if (targetx + curw > _xsi[targetmon-1].x_org + _xsi[targetmon-1].width)
	{
		targetx = _xsi[targetmon-1].x_org + (_xsi[targetmon-1].width - curw);
		if (targetx < _xsi[targetmon-1].x_org)
			targetx = _xsi[targetmon-1].x_org;
	}
	if (targety + curh > _xsi[targetmon-1].y_org + _xsi[targetmon-1].height)
	{
		targety = _xsi[targetmon-1].y_org +
			(_xsi[targetmon-1].height - curh);
		if (targety < _xsi[targetmon-1].y_org)
			targety = _xsi[targetmon-1].y_org;
	}
	XMoveWindow(_display, window, targetx, targety);
	if (0 != ismax)
		maximize_window(window, 1);
	cleanup();
	return EXIT_SUCCESS;
}
