#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include "types.hpp"

Display *display;
Window window;
GC gc;
int screen;
bool shouldExit = false;
std::vector<std::thread> threads;

void drawImage(MCU *image_data, unsigned int _width, unsigned int _height, unsigned int mcuWidth, int mcuWidthReal, int width_y)
{
    for (int y = _height - 1; y >= 0; --y)
    {
        const int mcuRow = y / 8;
        const int pixelRow = y % 8;
        for (int x = width_y; x < _width; ++x)
        {
            if (shouldExit)
            {
                return;
            }
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

    int t_count = 1; // increase this to increase the number of threads
    for (int i = 0; i < t_count; i++)
    {
        unsigned int _width = (width / t_count) * (i + 1) + (width % t_count);
        unsigned int _height = height;
        unsigned int _width_offset = (width / t_count * i) + (i != 0 ? (width % t_count) : 0);
        threads.emplace_back([image_data, _width, _height, mcuWidth, mcuWidthReal, _width_offset]()
                             { drawImage(image_data, _width, _height, mcuWidth, mcuWidthReal, _width_offset); });
    }

    while (1)
    {
        XEvent event;
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            // DrawImage() is already called by threads
            printf("DISPLAY :: Image Drawn!\n");
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
                shouldExit = true;
                for (auto &thread : threads)
                {
                    thread.join();
                }
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
