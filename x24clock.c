#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <math.h>
#include <time.h>
#include <string.h>

#define TAU 6.2831853
#define HOUR (TAU/24.0)
#define MINUTE (HOUR/60.0)
#define ORIGIN (TAU/4.0)

#define SIDE 140
#define MID (SIDE/2)
#define RADIUS (0.95 * MID)

// compile with: 	gcc -o x24clock x24clock.c -lX11 -lm

int minute = 0, second = 0;
int delay = 60;

Display *dis;
Window win;
int x11_fd;
fd_set in_fds;

int screen;
XColor color, dummy;
XGCValues gr_values;
XFontStruct *fontinfo;
struct timeval tv;
XEvent ev;
GC black, red, blackthin;

XSegment tics[24];

void tic(XSegment *xs, double r1, double r2, double theta)
{
  xs->x1 = MID + r1 * cos(theta);
  xs->y1 = MID + r1 * sin(theta);
  xs->x2 = MID + r2 * cos(theta);
  xs->y2 = MID + r2 * sin(theta);
}

void setup() {
  int i;

  for(i = 0; i < 12; i++) {
    // i 0..11
    tic(&tics[i],    RADIUS*0.85,  RADIUS, 2 * i * HOUR);
    tic(&tics[i+12], RADIUS*0.9, RADIUS, ((2 * i) + 1) * HOUR);
  }
}


void render() {
  time_t now;
  struct tm result;
  double angle;

  now = time(NULL);

  localtime_r(&now, &result);

  angle =
    (TAU / 2.0) +
    (result.tm_hour * HOUR) +
    (result.tm_min  * MINUTE);

  // angle -= ORIGIN;

  XClearWindow(dis, win);
  XDrawLine(dis, win, red, MID, MID,
	    MID+sin(angle)*RADIUS,
	    MID-cos(angle)*RADIUS);
  if(minute) {
    XDrawLine(dis, win, black,
	      SIDE*result.tm_min/60, 0,
	      SIDE*result.tm_min/60, SIDE)-1;
  }
  if(second) {
    XDrawLine(dis, win, black,
	      0, SIDE*result.tm_sec/60,
	      SIDE-1, SIDE*result.tm_sec/60);
  }
  // XDrawString(dis, win, black, MID-6, MID/10, "12", 2);
  // XDrawString(dis, win, black, MID-6, SIDE-MID/12, "00", 2);
  XDrawSegments(dis, win, black, tics, 24);

  XFlush(dis);
}

int main(int argc, char **argv) {

  if(argc == 2) {
    if (0 == strncmp(argv[1], "ms", 2)) {
      minute = 1;
      second = 1;
      delay = 1;
    }
  }

  dis = XOpenDisplay(NULL);
  screen = DefaultScreen(dis);

  win = XCreateSimpleWindow(dis, /* display */
			    RootWindow(dis, 0), /* parent */
			    1, 1,		  /* x, y */
			    SIDE, SIDE,         /* width, height */
			    0,		  /* border_width */
			    WhitePixel(dis, 0), /* border */
			    WhitePixel(dis, 0)); /* background */

  fontinfo = XLoadQueryFont(dis, "fixed");

  XAllocNamedColor(dis, DefaultColormap(dis, screen), "black",
		   &color, &dummy);

  gr_values.font = fontinfo->fid;
  gr_values.foreground = color.pixel;
  gr_values.line_width = 2;
  gr_values.cap_style= CapButt;

  black = XCreateGC(dis, win,
		    GCFont+GCForeground+GCLineWidth+GCCapStyle,
		    &gr_values);

  gr_values.line_width = 1;

  blackthin = XCreateGC(dis, win, GCLineWidth, &gr_values);

  XAllocNamedColor(dis, DefaultColormap(dis, screen), "red",
		   &color, &dummy);

  gr_values.foreground = color.pixel;
  gr_values.line_width = 4;
  gr_values.cap_style= CapRound;

  red = XCreateGC(dis, win,
		  GCForeground+GCLineWidth+GCCapStyle,
		  &gr_values);

  // You don't need all of these. Make the mask as you normally would.
  // XSelectInput(dis, win,
  //     ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
  //     ButtonPressMask | ButtonReleaseMask  | StructureNotifyMask
  //     );
  XSelectInput(dis, win, ExposureMask | StructureNotifyMask);

  XMapWindow(dis, win);

  // This returns the FD of the X11 display (or something like that)
  x11_fd = ConnectionNumber(dis);

  setup();
  render();
  XFlush(dis);

  // Main loop
  while(1) {
    // Create a File Description Set containing x11_fd
    FD_ZERO(&in_fds);
    FD_SET(x11_fd, &in_fds);

    // Set our timer.  One second sounds good.
    tv.tv_usec = 0;
    tv.tv_sec = delay;

    // Wait for X Event or a Timer
    int num_ready_fds = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
    // if (num_ready_fds > 0)
    //     printf("Event Received!\n");
    // else if (num_ready_fds == 0)
    //     // Handle timer here
    //     printf("Timer Fired!\n");
    // else
    //     printf("An error occured!\n");

    // Handle XEvents and flush the input
    while(XPending(dis))
      XNextEvent(dis, &ev);

    // may as well render right here
    render();
  }
  return(0);
}
