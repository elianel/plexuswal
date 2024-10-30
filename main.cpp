#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#define NUM_PARTICLES 150
#define MAX_DISTANCE 100.0f
#define MIN_DISTANCE 20.0f
#define PARTICLE_SIZE 3

typedef struct {
    float x, y;
    float vx, vy;
    float speed;
    float angle;
} Particle;

Particle particles[NUM_PARTICLES];

float distance(float x1, float y1, float x2, float y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

void init_particles(int width, int height) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = rand() % width;
        particles[i].y = rand() % height;
        particles[i].speed = ((float)rand() / RAND_MAX) * 0.4f + 0.1f;
        particles[i].angle = ((float)rand() / RAND_MAX) * 2 * M_PI;
        particles[i].vx = cos(particles[i].angle) * particles[i].speed;
        particles[i].vy = sin(particles[i].angle) * particles[i].speed;
    }
}

void update_particles(int width, int height) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].angle += ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        particles[i].vx = cos(particles[i].angle) * particles[i].speed;
        particles[i].vy = sin(particles[i].angle) * particles[i].speed;
        if (particles[i].x < 0) particles[i].x = width;
        if (particles[i].x > width) particles[i].x = 0;
        if (particles[i].y < 0) particles[i].y = height;
        if (particles[i].y > height) particles[i].y = 0;
    }
}

Window create_background_window(Display *display, int screen, int width, int height) {
    Window window;
    XSetWindowAttributes attributes;
    
    attributes.background_pixel = BlackPixel(display, screen);
    attributes.override_redirect = True;
    attributes.backing_store = WhenMapped;
    
    window = XCreateWindow(display, RootWindow(display, screen),
                          0, 0, width, height, 0,
                          DefaultDepth(display, screen),
                          InputOutput,
                          DefaultVisual(display, screen),
                          CWBackPixel | CWOverrideRedirect | CWBackingStore,
                          &attributes);

    // Set window type to desktop
    Atom atom_type = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom atom_desktop = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    XChangeProperty(display, window, atom_type, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&atom_desktop, 1);

    // Set window state to below
    Atom atom_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom atom_below = XInternAtom(display, "_NET_WM_STATE_BELOW", False);
    XChangeProperty(display, window, atom_state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&atom_below, 1);

    XMapWindow(display, window);
    XLowerWindow(display, window);
    
    return window;
}

void draw(Display *display, Window window, GC gc, int width, int height) {
    // Create a pixmap for double buffering
    Pixmap buffer = XCreatePixmap(display, window, width, height, 
                                 DefaultDepth(display, DefaultScreen(display)));
    
    // Clear the buffer
    XSetForeground(display, gc, 0x000000);
    XFillRectangle(display, buffer, gc, 0, 0, width, height);

    // Draw connections
    for (int i = 0; i < NUM_PARTICLES; i++) {
        for (int j = i + 1; j < NUM_PARTICLES; j++) {
            float dist = distance(particles[i].x, particles[i].y, 
                                particles[j].x, particles[j].y);
            
            if (dist < MAX_DISTANCE && dist > MIN_DISTANCE) {
                int opacity = (int)(255 * (1.0f - dist / MAX_DISTANCE));
                unsigned long color = (opacity << 16) | (opacity << 8) | opacity;
                XSetForeground(display, gc, color);
                XDrawLine(display, buffer, gc,
                         (int)particles[i].x, (int)particles[i].y,
                         (int)particles[j].x, (int)particles[j].y);
            }
        }
    }

    // Draw particles
    XSetForeground(display, gc, 0x4080FF);
    for (int i = 0; i < NUM_PARTICLES; i++) {
        XFillArc(display, buffer, gc,
                 (int)particles[i].x - PARTICLE_SIZE/2,
                 (int)particles[i].y - PARTICLE_SIZE/2,
                 PARTICLE_SIZE, PARTICLE_SIZE, 0, 360*64);
    }

    // Copy buffer to window and free the pixmap
    XCopyArea(display, buffer, window, gc, 0, 0, width, height, 0, 0);
    XFreePixmap(display, buffer);
    XFlush(display);
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    // Create a dedicated window for the wallpaper
    Window window = create_background_window(display, screen, width, height);
    
    // Create GC for drawing
    GC gc = XCreateGC(display, window, 0, NULL);

    srand(time(NULL));
    init_particles(width, height);

    while (1) {
        update_particles(width, height);
        draw(display, window, gc, width, height);
        usleep(9000);//usleep(16667); // ~60 FPS
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
