#include<stdio.h>
#include<X11/Xlib.h>

int main(){
    
    Display *display;

    display = XOpenDisplay(NULL);

    if(display == NULL){
        fprintf(stderr,"lockd: failed to open X display\n");
        return 1;
    } 

    printf("lockd: connected to X server\n");

    XCloseDisplay(display);

    return 0;
}