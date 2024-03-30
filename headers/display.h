#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"


void displayImage(MCU *image_data, unsigned int width, unsigned int height, unsigned int mcuWidth, unsigned int mcuHeight, int mcuWidthReal)
{
    Display *display;
    Window window;
    XEvent event;
    GC gc;
    int screen;

    // Open a connection to the X server
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    screen = DefaultScreen(display);

    // Create a window
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 0, BlackPixel(display, screen), WhitePixel(display, screen));

    // Select events to listen for
    XSelectInput(display, window, ExposureMask | KeyPressMask);

    // Map the window to the screen
    XMapWindow(display, window);

    // Create a graphical context
    gc = XCreateGC(display, window, 0, NULL);

    // Main event loop
    while (1)
    {
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            for (int y = height - 1; y >= 0; --y)
            {
                const int mcuRow = y / 8;
                const int pixelRow = y % 8;
                for (int x = 0; x < width; ++x)
                {
                    const int mcuColumn = x / 8;
                    const int pixelColumn = x % 8;
                    const int mcuIndex = mcuRow * mcuWidthReal + mcuColumn;
                    const int pixelIndex = pixelRow * 8 + pixelColumn;

                    // Assuming each MCU contains RGB values
                    int red = image_data[mcuIndex].r[pixelIndex];
                    int green = image_data[mcuIndex].g[pixelIndex];
                    int blue = image_data[mcuIndex].b[pixelIndex];
                    // int green = image_data[y * width + x].g[0];
                    // int blue = image_data[y * width + x].b[0];

                    // Convert RGB values to XColor format (You may need to adjust this part)
                    XColor color;
                    color.red = red * 256;
                    color.green = green * 256;
                    color.blue = blue * 256;
                    XAllocColor(display, DefaultColormap(display, screen), &color);

                    // Set the pixel color and draw a point
                    XSetForeground(display, gc, color.pixel);
                    XDrawPoint(display, window, gc, x, y);
                }
            }
            XFlush(display);
        }

        if (event.type == KeyPress)
        {
            break;
        }
    }

    // Clean up resources
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    free(image_data); // Free image data memory
}
