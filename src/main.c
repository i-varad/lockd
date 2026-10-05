#include<stdio.h>
#include<X11/Xlib.h>

int main(){
    
    /*
    Display *display;
    is an opaque Xlib data structure representing connection to X server.
    */
    Display *display; 

    display = XOpenDisplay(NULL);

    
    
    // confirming if connection to X server is established or not.
    if(display == NULL){
        fprintf(stderr,"lockd: failed to open X display\n");
        return 1;
    } 
    
    printf("lockd: connected to X server\n");


    /* Name of the X11 display we are connected to. */
    printf("  display: %s\n", DisplayString(display));

    /* Vendor/implementation of the X11 server. */
    printf("  vendor: %s\n", ServerVendor(display));
    
    /* Major and revision number of the X11 protocol. */
    printf("  protocol: %d.%d\n", 
            ProtocolVersion(display),
            ProtocolRevision(display));



    
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    printf("  screen: %d\n", screen);
    printf("  root window: 0x%lx\n", root);
    


    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    printf("  width: %d\n", width);
    printf("  height: %d\n", height);


    /* Creating first Window */

    Window window = XCreateSimpleWindow(
        display,
        root,
        0,
        0,
        width,
        height,
        0,
        0,
        0
    );

    /* Calling X server to map the window */
    XMapWindow(display, window);

    /* flushing the request */
    XFlush(display);

    /* Testing window we just created */
    while (1){
    }
    

    // XCloseDisplay(display);

    return 0;
}