#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include "types.hpp"

Display *display;
Window window;
XEvent event;
GC gc;
int screen;

void drawImage(MCU *image_data, unsigned int _width, unsigned int _height, unsigned int mcuWidth, unsigned int mcuHeight, int mcuWidthReal, int width_y)
{
    for (int y = _height - 1; y >= 0; --y)
    {
        const int mcuRow = y / 8;
        const int pixelRow = y % 8;
        for (int x = width_y; x < _width; ++x)
        {
            const int mcuColumn = x / 8;
            const int pixelColumn = x % 8;
            const int mcuIndex = mcuRow * mcuWidthReal + mcuColumn;
            const int pixelIndex = pixelRow * 8 + pixelColumn;

            int red = image_data[mcuIndex].r[pixelIndex];
            int green = image_data[mcuIndex].g[pixelIndex];
            int blue = image_data[mcuIndex].b[pixelIndex];

            XColor color;
            color.red = red * 256;
            color.green = green * 256;
            color.blue = blue * 256;
            XAllocColor(display, DefaultColormap(display, screen), &color);

            XSetForeground(display, gc, color.pixel);
            XDrawPoint(display, window, gc, x, y);
            XFlush(display);
        }
    }
}

bool displayImage(MCU *image_data, unsigned int width, unsigned int height, unsigned int mcuWidth, unsigned int mcuHeight, int mcuWidthReal)
{
    if (!XInitThreads())
    {
        // Initialization failed, handle error
        return false;
    }
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "DISPLAY :: Cannot open display\n");
        return false;
    }

    screen = DefaultScreen(display);

    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 0, BlackPixel(display, screen), WhitePixel(display, screen));

    XSelectInput(display, window, ExposureMask | KeyPressMask);

    XMapWindow(display, window);
    gc = XCreateGC(display, window, 0, NULL);
    Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteMessage, 1);
    std::vector<std::thread> threads;
    int t_count = 1; // increase this to increase the number of threads
    for (int i = 0; i < t_count; i++)
    {
        unsigned int _width = (width / t_count) * (i + 1) + (width % t_count);
        unsigned int _height = height;
        unsigned int _width_offset = (width / t_count * i) + (i != 0 ? (width % t_count) : 0);
        threads.emplace_back([image_data, _width, _height, mcuWidth, mcuHeight, mcuWidthReal, _width_offset]()
                             { drawImage(image_data, _width, _height, mcuWidth, mcuHeight, mcuWidthReal, _width_offset); });
    }
    //  for (auto& thread : threads) {
    //     thread.join();
    // }
    while (1)
    {
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            // drawImage(image_data, width, height, mcuWidth, mcuHeight, mcuWidthReal, 0);
            cout << "DISPLAY :: Image Drawn!" << endl;
        }

        if (event.type == KeyPress)
        {
            break;
        }

        if (event.type == ClientMessage)
        {
            if (event.xclient.data.l[0] == static_cast<long>(wmDeleteMessage))
            {
                std::cout << "DISPLAY :: Closing window!" << std::endl;
                break;
            }
        }
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    free(image_data);
    return true;
}
