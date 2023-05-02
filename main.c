#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <X11/Xlib.h>

#define HEIGHT 1000
#define WIDTH 1000

int8_t in_bounds(int32_t x, int32_t y, int64_t w, int64_t h);
void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color);
void gc_draw_line(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
void update(Display *display, GC *gc, Window *window, XImage *image);

uint32_t decodeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) + (g << 8) + b;
}

float map(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) / (in_max - in_min) * (out_max - out_min) + out_min;
}

void render_cannabis_curve(void *memory, int a, uint32_t color)
{   
    int old_x = 0;
    int old_y = 0;
    for(int i = 0; i < WIDTH; ++i) {
        float theta = map(i, 0, WIDTH, -M_PI, M_PI);
        int r = a*(1 + (9.0f/10.0f)*cos(8*theta)) * (1 + (1.0f/10.0f)*cos(24*theta))
                * ((9.0f/10.0f) + (1.0f/10.0f)*cos(200*theta))*(1 + sin(theta));
        int x = 1.9 * r * cos(theta) + WIDTH/2;
        int y = 1.7 * r * -sin(theta) + HEIGHT/1.25;
        if(i == 0) {
            old_x = x;
            old_y = y;
        }
        gc_draw_line(memory, old_x, old_y, x, y, color);
        old_x = x;
        old_y = y;
    }
}

int8_t exitloop = 0;
int8_t auto_update = 0;

int main(void)
{
    Display *display = XOpenDisplay(NULL);

    int screen = DefaultScreen(display);

    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0,
        WIDTH, HEIGHT,
        0, 0,
        0
    );

    char *memory = (char *) malloc(sizeof(uint32_t)*HEIGHT*WIDTH);

    XWindowAttributes winattr = {0};
    XGetWindowAttributes(display, window, &winattr);

    XImage *image = XCreateImage(
        display, winattr.visual, winattr.depth,
        ZPixmap, 0, memory,
        WIDTH, HEIGHT,
        32, WIDTH*4
    );

    GC graphics = XCreateGC(display, window, 0, NULL);

    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &delete_window, 1);

    Mask key = KeyPressMask | KeyReleaseMask;
    XSelectInput(display, window, ExposureMask | key);

    XMapWindow(display, window);
    XSync(display, False);

    XEvent event;

    render_cannabis_curve(memory, 100, 0x0000FF00);
    update(display, &graphics, &window, image);

    while(!exitloop) {
        while(XPending(display) > 0) {
            XNextEvent(display, &event);
            switch(event.type) {
                case Expose: {
                    update(display, &graphics, &window, image);
                    break;
                }
                case ClientMessage: {
                    if((Atom) event.xclient.data.l[0] == delete_window) {
                        exitloop = 1;   
                    }
                    break;
                }
                case KeyPress: {
                    if(event.xkey.keycode == 0x24) {
                        // Draw Once
                        update(display, &graphics, &window, image);
                    }
                    if(event.xkey.keycode == 0x41) {
                        auto_update = !auto_update;
                    }
                    break;
                }
            }
        }

        if(auto_update) {
            // Draw multiple times
            update(display, &graphics, &window, image);
        }
    }


    XCloseDisplay(display);

    free(memory);

    return 0;
}

void update(Display *display, GC *graphics, Window *window, XImage *image)
{
    XPutImage(
        display,
        *window,
        *graphics,
        image,
        0, 0,
        0, 0,
        WIDTH, HEIGHT
    );

    XSync(display, False);
}

int8_t in_bounds(int32_t x, int32_t y, int64_t w, int64_t h)
{
    return (x >= 0 && x < w && y >= 0 && y < h);
}

void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color)
{
    if(in_bounds(x, y, WIDTH, HEIGHT))
        *((uint32_t *) memory + y * WIDTH + x) = color;
}

void gc_draw_line(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    int steps = abs(dy) > abs(dx) ? abs(dy) : abs(dx);

    float X = x0;
    float Y = y0;
    float Xinc = dx / (float) steps;
    float Yinc = dy / (float) steps;

    for(int i = 0; i < steps; ++i) {
        gc_put_pixel(memory, (int) X, (int) Y, color);
        X += Xinc;
        Y += Yinc;
    }
}

