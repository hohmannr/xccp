#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>

const char* helpstr = "usage:\n"
	"\t%s [ -h | -x | -X ]\n"
	"\n"
	"description:\n"
	"\tA simple color picker for X11 - Changes mouse cursor to color picker. "
	"Click anywhere on your screen to obtain the color code to stdout.\n"
	"\n"
	"flags:\n"
	"\t-h\tprints this help and exits\n"
	"\t-x\tchange color output to be in hexadecimal format\n"
	"\t-X\tsame as -x but with capital hexadecimal digits\n";

enum ARGS {
	ARG_HELP   = 1,
	ARG_HEX	   = 1 << 1,
	ARG_HEXCAP = 1 << 2,
};

int parse_argv(unsigned *args_mask, int argc, char **argv);
void mouse_wait_click(Display *dpy);
int mouse_pos(Display *dpy, int *x, int *y);
void pixel_color(Display *dpy, int x, int y, XColor *color);

int main(int argc, char **argv) {
	int ok, mouse_x, mouse_y;
	char *printstr;
	XColor color;
	unsigned args;

	ok = parse_argv(&args, argc, argv);
	if(!ok) {
		fprintf(stderr, "error: Argument parsing failed.\n");
		return 1;
	}

	if(args & ARG_HELP) {
		printf(helpstr, argv[0]);
		return 0;
	}

	/* init x-server connection */
	Display *dpy = XOpenDisplay(NULL);
	assert(dpy);

	/* wait for a mouse click, then get cursor location */
	mouse_wait_click(dpy);
	ok = mouse_pos(dpy, &mouse_x, &mouse_y);
	if(!ok) {
		fprintf(stderr, "error: Could not grab mouse pointer.\n");
		XCloseDisplay(dpy);
		return 1;
	}

	/* get color */
	pixel_color(dpy, mouse_x, mouse_y, &color);

	/* decide based on given args how to print*/
	if(args & ARG_HEX)
		printstr = "#%02x%02x%02x\n";
	else if(args & ARG_HEXCAP)
		printstr = "#%02X%02X%02X\n";
	else
		printstr = "(%d, %d, %d)\n";

	/* print colors in determined format (hex, HEX or rgb) */
	printf(printstr, color.red/255, color.green/255, color.blue/255);
	
	/* free */
	XCloseDisplay(dpy);

	return 0;
}

/* Changed cursor font to crosshair, waits for a button release (click) event of the mouse. */
void mouse_wait_click(Display *dpy)
{
	Window root;
	XEvent event;
	Cursor crosshair_cursor;
	int clicked;

	root = XDefaultRootWindow(dpy);

	crosshair_cursor = XCreateFontCursor(dpy, XC_crosshair);
	XGrabPointer(dpy, root, False, ButtonReleaseMask, GrabModeAsync,
        GrabModeAsync, root, crosshair_cursor, CurrentTime);
	XSelectInput(dpy, root, ButtonReleaseMask);
	clicked = 0;
	while(!clicked) {
		XNextEvent(dpy, &event);
		switch(event.type) {
			case ButtonRelease:
				clicked = 1;
				break;
			default:
				break;
		}
	}

	/* free */
	XUngrabPointer(dpy, CurrentTime);
}

/* Gets current mouse position. Returns if pointer query was succesful. */
int mouse_pos(Display *dpy, int *x, int *y)
{
	int win_x, win_y;
	Bool ok;
	unsigned mask;
	Window def_win, root_win, child_win;
	
	def_win = XDefaultRootWindow(dpy);
	ok = XQueryPointer(dpy, def_win, &root_win, &child_win, x, y, &win_y, &win_y, &mask);
	if(!ok)
		return 0;
	return 1;
}

/* Gets XColor from the pixel at x, y in root window. */
void pixel_color(Display *dpy, int x, int y, XColor *color)
{
	Window root;
	XImage *px_img;
	Colormap colormap;
	int screen;

	root = XDefaultRootWindow(dpy);
	screen = XDefaultScreen(dpy);

	/* get 1x1 image around pixel x, y */
	px_img = XGetImage(dpy, root, x, y, 1, 1, AllPlanes, XYPixmap);

	/* get pixel into color object */
	color->pixel = XGetPixel(px_img, 0, 0);

	colormap = XDefaultColormap(dpy, screen);
	XQueryColor(dpy, colormap, color);
}

/* Parses given argument list and sets bit flags accordingly. Returns if parsing was succesful. */
int parse_argv(unsigned *args_mask, int argc, char **argv)
{
	char *arg;

	/* clear flags */
	*args_mask = 0;

	/* skip binary name */
	--argc, ++argv;

	/* handle arguments */
	while(argc > 0) {
		arg = *argv;
		if(!strcmp(arg, "-h") || !strcmp(arg, "--help"))
			*args_mask |= ARG_HELP;
		else if(!strcmp(arg, "-x"))
			if(!(*args_mask & ARG_HEXCAP))
				*args_mask |= ARG_HEX;
			else
				return 0;
		else if(!strcmp(arg, "-X"))
			if(!(*args_mask & ARG_HEX))
				*args_mask |= ARG_HEXCAP;
			else
				return 0;
		else
			return 0;

		--argc; ++argv;
	}

	return 1;
}
