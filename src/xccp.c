#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>

#define LEFT_MOUSE_BTN 1

const char* helpstr = "usage:\n"
	"\t%s [ -h | -x | -X ]\n"
	"\n"
	"description:\n"
	"\tA simple color picker for X11 - Changes mouse cursor to color picker. "
	"Left click anywhere on your screen to obtain the color code to stdout. "
	"Right click (and other mouse buttons) cancel the action.\n"
	"\n"
	"flags:\n"
	"\t-h\tprints this help and exits\n"
	"\t-x\tchange color output to be in hexadecimal format\n"
	"\t-X\tsame as -x but with capital hexadecimal digits\n"
	"\t-c\tuse tool in continuous mode";

enum ARGS {
	ARG_HELP     = 1,
	ARG_HEX      = 1 << 1,
	ARG_HEXCAP   = 1 << 2,
	ARG_CONTINUE = 1 << 3,
};

int parse_argv(unsigned *args_mask, int argc, char **argv);
int wait_input(Display *dpy, XEvent *event);
int mouse_pos(Display *dpy, int *x, int *y);
void pixel_color(Display *dpy, int x, int y, XColor *color);

int main(int argc, char **argv) {
	int ok, cancel;
	char *printstr;
	XColor color;
	unsigned args;
	XEvent event;

	ok = parse_argv(&args, argc, argv);
	if(!ok) {
		fprintf(stderr, "error: Argument parsing failed.\n");
		return 1;
	}

	if(args & ARG_HELP) {
		printf(helpstr, argv[0]);
		return 0;
	}

	/* init X server connection */
	Display *dpy = XOpenDisplay(NULL);
	assert(dpy);

	do {
		/* wait for a mouse click, then get cursor location via gotten XEvent */
		cancel = wait_input(dpy, &event);
		if(cancel)
			goto cleanup; /* I know a simple break statement would also do the job :D, wanted to try out linux kernel practices */

		/* get color */
		pixel_color(dpy, event.xbutton.x_root, event.xbutton.y_root, &color);

		/* decide based on given args how to print*/
		if(args & ARG_HEX)
			printstr = "#%02x%02x%02x\n";
		else if(args & ARG_HEXCAP)
			printstr = "#%02X%02X%02X\n";
		else
			printstr = "(%d, %d, %d)\n";

		/* print colors in determined format (hex, HEX or rgb) */
		printf(printstr, color.red/256, color.green/256, color.blue/256);
	} while(args & ARG_CONTINUE); /* if continuous mode is given via args, repeat */
	
cleanup:
	/* free */
	XCloseDisplay(dpy);

	return 0;
}

/* Changes cursor font to crosshair, waits for a main mouse button release (click) event of the mouse. On other input returns cancel statement. */
int wait_input(Display *dpy, XEvent *event)
{
	Window root;
	Cursor crosshair_cursor;
	int clicked, cancel;

	root = XDefaultRootWindow(dpy);

	crosshair_cursor = XCreateFontCursor(dpy, XC_crosshair);
	XGrabPointer(dpy, root, False, ButtonReleaseMask, GrabModeAsync,
        GrabModeAsync, root, crosshair_cursor, CurrentTime);

	clicked = cancel = 0;
	while(!clicked) {
		XNextEvent(dpy, event);
		if(event->type == ButtonRelease) {
			clicked = 1;
			/* check if button was  */
			if(event->xbutton.button != LEFT_MOUSE_BTN)
				cancel = 1;
		}
	}

	/* free */
	XUngrabPointer(dpy, CurrentTime);

	return cancel;
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
		else if(!strcmp(arg, "-c"))
			*args_mask |= ARG_CONTINUE;
		else
			return 0;

		--argc; ++argv;
	}

	return 1;
}
