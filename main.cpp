#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unordered_map>

/////////////////////////////////////////////////////////////////

static bool g_wm_detected = false;

static int on_x_err(Display* dpy, XErrorEvent* ev){
    printf("err: %s\n", "unkown");
    return 1;
}

static int on_wm_detected(Display* dpy, XErrorEvent* ev){
    if(ev->error_code == BadAccess){
        g_wm_detected = true;
        return 1;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////

static Display* g_disp = NULL;
static Window   g_root = 0xff;
static std::unordered_map<Window, Window> g_clients;
bool wm_init(){
    Display* dpy = XOpenDisplay(NULL);
    if(dpy == NULL){
        printf("err: failed to open display %s\n", XDisplayName(NULL));
        return false;
    }
    g_disp = dpy;
    g_root = DefaultRootWindow(g_disp);
    return true;
}

void wm_frame_window(Window w, bool ancient){
    const unsigned int BORDER_WIDTH = 3;
    const unsigned long BORDER_COLOR = 0xff0000;
    const unsigned long BG_COLOR = 0x0000ff;

    XWindowAttributes x_window_attrs;
    XGetWindowAttributes(g_disp, w, &x_window_attrs);

    if (ancient) {
        if (x_window_attrs.override_redirect ||
            x_window_attrs.map_state != IsViewable) {
        return;
        }
    }
    
    const Window frame = XCreateSimpleWindow(
        g_disp,
        g_root,
        x_window_attrs.x,
        x_window_attrs.y,
        x_window_attrs.width,
        x_window_attrs.height,
        BORDER_WIDTH,
        BORDER_COLOR,
        BG_COLOR);
    // 4. Select events on frame.
    XSelectInput(
        g_disp,
        frame,
        SubstructureRedirectMask | SubstructureNotifyMask);
    // 5. Add client to save set, so that it will be restored and kept alive if we
    // crash.
    XAddToSaveSet(g_disp, w);
    // 6. Reparent client window.
    XReparentWindow(
        g_disp,
        w,
        frame,
        0, 0);  // Offset of client window within frame.
    // 7. Map frame.
    XMapWindow(g_disp, frame);
    // 8. Save frame handle.
    g_clients[w] = frame;
    // 9. Grab universal window management actions on client window.
    //   a. Move windows with alt + left button.
    XGrabButton(
        g_disp,
        Button1,
        Mod1Mask,
        w,
        false,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None);
    //   b. Resize windows with alt + right button.
    XGrabButton(
        g_disp,
        Button3,
        Mod1Mask,
        w,
        false,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None);
    //   c. Kill windows with alt + f4.
    XGrabKey(
        g_disp,
        XKeysymToKeycode(g_disp, XK_F4),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync);
    //   d. Switch windows with alt + tab.
    XGrabKey(
        g_disp,
        XKeysymToKeycode(g_disp, XK_Tab),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync);


}

void wm_add_existing_windows(){
    Window returned_root, returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;

    XQueryTree(
      g_disp,
      g_root,
      &returned_root,
      &returned_parent,
      &top_level_windows,
      &num_top_level_windows);

    for (unsigned int i = 0; i < num_top_level_windows; ++i) {
        wm_frame_window(top_level_windows[i], true);
    }

    XFree(top_level_windows);
    XUngrabServer(g_disp);
}

bool wm_load(){
    XSetErrorHandler(on_wm_detected);
    XSelectInput(
        g_disp,
        g_root,
        SubstructureRedirectMask | SubstructureNotifyMask
    );
    XSync(g_disp, false);

    if(g_wm_detected){
        printf("err: another window manager is already in use!\n");
        return false;
    }
    XSetErrorHandler(on_x_err);

    XGrabServer(g_disp);
    XSelectInput(g_disp, g_root, KeyPressMask | KeyReleaseMask);
    wm_add_existing_windows();
    return true;
}

bool wm_destroy(){
    XCloseDisplay(g_disp);
    return true;
}

/////////////////////////////////////////////////////////////////

void wm_on_create(XCreateWindowEvent* event){

}


void wm_on_destroy(XDestroyWindowEvent* event){

}

void wm_on_reparent(XReparentEvent* event){

}

void wm_on_map(XMapEvent* event){

}

void wm_on_unmap(XUnmapEvent* event){

}

void wm_on_configure(XConfigureEvent* event){

}

void wm_on_mapreq(XMapRequestEvent* event){

}

void wm_on_confreq(XConfigureRequestEvent* event){

}

void wm_on_btn_pressed(XButtonPressedEvent* event){

}

void wm_on_btn_released(XButtonReleasedEvent* event){

}

void wm_on_motion(XMotionEvent* event){

}

void wm_on_key_pressed(XKeyEvent* event){

}

void wm_on_key_released(XKeyEvent* event){

}

/////////////////////////////////////////////////////////////////

void wm_listen(XEvent* e){
    switch (e->type) {
        case CreateNotify:
            wm_on_create(&e->xcreatewindow);
            break;
        case DestroyNotify:
            wm_on_destroy(&e->xdestroywindow);
            break;
        case ReparentNotify:
            wm_on_reparent(&e->xreparent);
            break;
        case MapNotify:
            wm_on_map(&e->xmap);
            break;
        case UnmapNotify:
            wm_on_unmap(&e->xunmap);
            break;
        case ConfigureNotify:
            wm_on_configure(&e->xconfigure);
            break;
        case MapRequest:
            wm_on_mapreq(&e->xmaprequest);
            break;
        case ConfigureRequest:
            wm_on_confreq(&e->xconfigurerequest);
            break;
        case ButtonPress:
            wm_on_btn_pressed(&e->xbutton);
            break;
        case ButtonRelease:
            wm_on_btn_released(&e->xbutton);
            break;
        case MotionNotify:
            // Skip any already pending motion events.
            while (XCheckTypedWindowEvent(
                g_disp, e->xmotion.window, MotionNotify, e)) {}
            wm_on_motion(&e->xmotion);
            break;
            /*
        case KeyPress:
            wm_on_key_pressed(e->xkey);
            break;
        case KeyRelease:
            wm_on_key_released(e->xkey);
            break;
            */
        default:
            break;
    }

}

int main(){
    printf("staring window manager...\n");
    if(!wm_init()){
        wm_destroy();
        return 1;
    }
    if(!wm_load()){
        wm_destroy();
        return 1;
    }

    bool running = true;

    while(running){
        XEvent e;
        XNextEvent(g_disp, &e);
        wm_listen(&e);
    }

    printf("shutting down\n");
    wm_destroy();
    return 0;
}