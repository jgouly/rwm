#include <ruby.h>
#include <stdio.h>

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#include <xcb/xcb.h>

#define TEXTW(text, len) XTextWidth(XLoadQueryFont(rwm->dpy, "fixed"), text, len)

static VALUE rb_mRWM;
static VALUE rb_cDisplay;
static VALUE rb_cWindow;

// TODO REMOVE THIS
typedef struct windowptr {
  Window win;
} WindowPtr;

typedef struct rwm {
  Display *dpy;
  int screen;
  int s_width, s_height;
  Window root;
  Window bar;
  Drawable drwbl;
  int d_x, d_y, d_w, d_h;
  GC gc;
} Rwm;

typedef struct config {
  int border_width;
  int normal_border_colour;
  int sel_border_colour;
  int normal_bg;
  int normal_fg;
} Config;

Rwm *rwm;
Config *config;
int running = 0;

// TODO free()
static VALUE
get_text_prop(Window w, Atom atom)
{
  XTextProperty name;
  XGetTextProperty(rwm->dpy, w, &name, atom);
  if(!name.nitems) return Qnil;
  if(name.encoding == XA_STRING)
    return rb_str_new2((char *) name.value);
  else
  {
    char **list = NULL; int n;
    if(XmbTextPropertyToTextList(rwm->dpy, &name, &list,  &n) >= Success && n > 0 && *list)
    {
      return rb_str_new2(*list);
    }
  }
  return rb_str_new2("");
}

static VALUE
get_config(VALUE self)
{
  config = ALLOC(Config);
  #define get_conf(X) config->X = NUM2INT(rb_iv_get(self, "@"#X));
  get_conf(border_width);
  get_conf(normal_border_colour);
  get_conf(sel_border_colour);
  get_conf(normal_bg);
  get_conf(normal_fg);
  #undef get_conf
  return Qnil;
}

static VALUE
get_title(VALUE self)
{
  WindowPtr *w_ptr;
  Data_Get_Struct(self, WindowPtr, w_ptr);
  return get_text_prop(w_ptr->win, XA_WM_NAME);
}

static VALUE
get_colour(VALUE self, VALUE colstr)
{
  Colormap cmap = DefaultColormap(rwm->dpy, rwm->screen);
  XColor color;
 if(!XAllocNamedColor(rwm->dpy, cmap, RSTRING_PTR(colstr), &color, &color))
		return INT2NUM(0);
	return LONG2NUM(color.pixel);
}

static VALUE
write_text(VALUE self, VALUE str, VALUE bg, VALUE fg)
{
  XRectangle r = {rwm->d_x, rwm->d_y, rwm->d_w, rwm->d_h};
  
  XSetForeground(rwm->dpy, rwm->gc, NIL_P(bg)?config->normal_bg:NUM2INT(bg));
  XFillRectangles(rwm->dpy, rwm->drwbl, rwm->gc, &r, 1);
  
  XSetForeground(rwm->dpy, rwm->gc, NIL_P(fg)?config->normal_fg:NUM2INT(fg));
  XDrawString(rwm->dpy, rwm->drwbl, rwm->gc, rwm->d_x, 13, RSTRING_PTR(str), RSTRING_LEN(str));
  rwm->d_x += TEXTW(RSTRING_PTR(str), RSTRING_LEN(str));
  return Qnil;
}

static VALUE
open_display(VALUE self)
{
  rwm = ALLOC(Rwm);
  rwm->dpy = XOpenDisplay(NULL);
  if(!rwm->dpy)
  {
    printf("ERROR"); return Qnil;
  }

  rwm->screen = DefaultScreen(rwm->dpy);
  rwm->root = RootWindow(rwm->dpy,rwm-> screen);
  rwm->s_width = DisplayWidth(rwm->dpy, rwm->screen);
  rwm->s_height = DisplayHeight(rwm->dpy, rwm->screen);
  rb_iv_set(self, "@width", INT2NUM(rwm->s_width));
  rb_iv_set(self, "@height", INT2NUM(rwm->s_height));
  rwm->drwbl = XCreatePixmap(rwm->dpy, rwm->root, rwm->s_width, 20, DefaultDepth(rwm->dpy, rwm->screen));
  rwm->gc = XCreateGC(rwm->dpy, rwm->root, 0, NULL);
  return Qnil;
}

static VALUE
create_bar(VALUE self)
{
  XSetWindowAttributes wa;
  wa.override_redirect = True;
  rwm->bar = XCreateWindow(rwm->dpy, rwm->root, 0, 0, rwm->s_width, 20, 0, DefaultDepth(rwm->dpy, rwm->screen), CopyFromParent, DefaultVisual(rwm->dpy, rwm->screen),CWOverrideRedirect,&wa);
  XMapRaised(rwm->dpy, rwm->bar);
  return Qnil;
}

static VALUE
draw_bar(VALUE self)
{
  rwm->d_x=rwm->d_y=0;
  rwm->d_w=rwm->s_width; rwm->d_h =20;
  rb_funcall(self, rb_intern("draw_tags"), 0);
  rb_funcall(self, rb_intern("draw_title"), 0);
  VALUE status = rb_funcall(self, rb_intern("update_status"), 0);
  rwm->d_x = rwm->d_w - TEXTW(RSTRING_PTR(status), RSTRING_LEN(status));
  write_text(self, status, Qnil, Qnil);
  XCopyArea(rwm->dpy, rwm->drwbl, rwm->bar, rwm->gc, 0, 0, rwm->s_width, 20, 0,0); 
  return Qnil;
}

static VALUE
get_key(VALUE self, KeySym ks){
  char *y = XKeysymToString(ks);
  VALUE action = rb_funcall(self, rb_intern("find_action"), 1, rb_str_new2(y));
  return action;
} 

void
handle_key(VALUE self, XEvent *e)
{
  XKeyEvent *ev = &e->xkey;
  KeySym ks = XKeycodeToKeysym(rwm->dpy, (KeyCode)ev->keycode,0);
  VALUE action = get_key(self, ks);
  if(rb_respond_to(action, rb_intern("call")))
  {
    rb_funcall(action, rb_intern("call"), 0);
  }
  else
  { 
    VALUE layout = rb_iv_get(self, "@layout");
    ID meth = rb_intern(RSTRING_PTR(get_key(self, ks)));
    rb_funcall(rb_respond_to(layout, meth)?layout:self,meth, 0);
  } 
}

static VALUE
focus(VALUE self, VALUE window)
{
 // fprintf(stderr, "focus\n");
  WindowPtr *w;
  Data_Get_Struct(window, WindowPtr, w);
  /*if(!NIL_P(rb_iv_get(self, "@focused_window")))
  {
    WindowPtr *old;
    Data_Get_Struct(rb_iv_get(self, "@focused_window"), WindowPtr, old);
    XSetWindowBorder(rwm->dpy, old->win, config->normal_border_colour); // CRASHESH HERE
  }
  rb_iv_set(self, "@focused_window", window);*/
  XSetInputFocus(rwm->dpy, w->win, 
                 RevertToPointerRoot, CurrentTime);
  XSetWindowBorder(rwm->dpy, w->win, config->sel_border_colour);
  return Qnil;
}
int
find_window(VALUE ary, Window win)
{
  int size = RARRAY_LEN(ary), i;
  WindowPtr *w_ptr;
  for(i = 0; i < size; i++)
    {
      Data_Get_Struct(RARRAY_PTR(ary)[i], WindowPtr, w_ptr);
      if(win == w_ptr->win)
	      return i;
    }
  return -1;
}

static VALUE
resize_window(VALUE self, VALUE x, VALUE y, VALUE w, VALUE h)
{
  //fprintf(stderr, "resize\n");
  WindowPtr *wp; XWindowChanges wc;
  Data_Get_Struct(self, WindowPtr, wp);
  wc.x = NUM2INT(x);
  wc.y = NUM2INT(y);
  wc.width = NUM2INT(w) - 2*config->border_width;//borderwidth
  wc.height = NUM2INT(h) - 2*config->border_width;
  XConfigureWindow(rwm->dpy, wp->win, CWX|CWY|CWWidth|CWHeight, &wc);   
  return Qnil;
}

void manage(VALUE self, Window win)
{
 // fprintf(stderr, "manage\n");
  WindowPtr *w_ptr = ALLOC(WindowPtr);
  w_ptr->win = win;
  VALUE windows = rb_iv_get(self, "@windows");
  if(find_window(windows, win) == -1){
    VALUE x = Data_Wrap_Struct(rb_cWindow, 0, free, w_ptr);
    rb_funcall(x, rb_intern("set_view"), 1, rb_iv_get(self, "@s_view"));
    rb_ary_push(windows, x);
  }
  XWindowChanges wc;
  wc.border_width = config->border_width;
  XConfigureWindow(rwm->dpy, w_ptr->win, CWBorderWidth, &wc);
  XSetWindowBorder(rwm->dpy, w_ptr->win, config->normal_border_colour);
  XMapWindow(rwm->dpy, w_ptr->win);
  rb_funcall(self, rb_intern("arrange"), 0);
  VALUE win2 = Data_Wrap_Struct(rb_cWindow, 0, free, w_ptr);
  focus(self, win2);
  XSelectInput(rwm->dpy, w_ptr->win, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
  XSync(rwm->dpy, False);
}
static VALUE
hide(VALUE self)
{
  WindowPtr *w_ptr;
  Data_Get_Struct(self, WindowPtr, w_ptr);
  XMoveWindow(rwm->dpy, w_ptr->win, 0, rwm->s_width*2);
  return Qnil;
}
void configure_request(VALUE self, XEvent *e)
{
//  fprintf(stderr, "config request\n");
  XWindowChanges wc;
  WindowPtr *w_ptr = ALLOC(WindowPtr);
  XConfigureRequestEvent *ev = &e->xconfigurerequest;
  VALUE windows = rb_iv_get(self, "@windows");
  w_ptr->win = ev->window;
  int index = find_window(windows, w_ptr->win);
  wc.x = ev->x;//0;// ev->x;
  wc.y = ev->y;//20;//ev->y;
  wc.width = ev->width;
  wc.height = ev->height;//rwm->s_height-20;//ev->height;
  wc.border_width = 20;
  wc.sibling = ev->above;
  wc.stack_mode = ev->detail;
  XConfigureWindow(rwm->dpy,w_ptr->win, ev->value_mask,&wc);
  XSetWindowBorder(rwm->dpy, w_ptr->win, 30000);
  XMapWindow(rwm->dpy, w_ptr->win);
  if(index ==-1){ 
    VALUE x = Data_Wrap_Struct(rb_cWindow, 0, free, w_ptr);
    rb_funcall(x, rb_intern("set_view"), 1, rb_iv_get(self, "@s_view"));
    rb_ary_push(windows, x);
  }
  rb_funcall(self, rb_intern("arrange"), 0);
  VALUE win2 = Data_Wrap_Struct(rb_cWindow, 0, free, w_ptr);
  focus(self, win2);
  XSync(rwm->dpy, False);
}

static VALUE
sync_display(VALUE self) // named to avoid compile errors
{
  XSync(rwm->dpy, False);
  return Qnil;
}

void destroy_notify(VALUE self, XEvent *e)
{
  //fprintf(stderr, "destroy notify\n");
  XDestroyWindowEvent *ev = &e->xdestroywindow;
  VALUE windows = rb_iv_get(self, "@windows");
  int index = find_window(windows, ev->window);
  if(index != -1)
  {
    VALUE win = rb_ary_entry(windows, index);
    if(win == rb_iv_get(self, "@focused_window"))
    {
      if(NUM2INT(rb_funcall(rb_funcall(self, rb_intern("windows"), 0), rb_intern("size"), 0)) > 1)
      {
        rb_iv_set(self, "@focused_window", rb_ary_entry(windows, 0));
      }
      else
      {
        rb_iv_set(self, "@focused_window", Qnil);
      }
    }
    rb_ary_delete_at(windows, index);
    rb_funcall(self, rb_intern("arrange"), 0);
  }
}

int grabkey(VALUE key, VALUE val, VALUE self)
{
  KeyCode kc = XKeysymToKeycode(rwm->dpy, XStringToKeysym(RSTRING_PTR(rb_ary_entry(key,1))));
 //fprintf(stderr, "grabbing %s\n", RSTRING_PTR(rb_ary_entry(key,1)));
  XGrabKey(rwm->dpy, kc, NUM2INT(rb_ary_entry(key,0)),
           rwm->root, True, GrabModeAsync, GrabModeAsync);
  return 0;
}

static VALUE
register_keys(VALUE self, VALUE ungrab)
{
  if(NUM2INT(ungrab) == 1)
    XUngrabKey(rwm->dpy, AnyKey, AnyModifier, rwm->root);
  VALUE keys = rb_iv_get(self, "@keys");
  if(!NIL_P(keys))
    rb_hash_foreach(keys, grabkey, self);
  return Qnil;
}

void
map_request(VALUE self, XEvent *e)
{
// fprintf(stderr, "map req\n");
  XMapRequestEvent *ev = &e->xmaprequest;
  int index = find_window(rb_iv_get(self, "@windows"), ev->window);
  if(index == -1)
    manage(self, ev->window);
}
void
enter_notify(VALUE self, XEvent *e)
{
  //fprintf(stderr, "enter not\n");
  XCrossingEvent *ev = &e->xcrossing;
  if(ev->window == rwm->root) return;
  int index = find_window(rb_iv_get(self, "@windows"), ev->window);
  if(index != -1)
  {
    VALUE win = rb_ary_entry(rb_iv_get(self, "@windows"), index);
    focus(self, win);
  }
}
static VALUE
quit(VALUE self)
{
  running = 0;
  XCloseDisplay(rwm->dpy);
  free(config);
  return Qnil;
}
static VALUE
run(VALUE self)
{ 
  XSetWindowAttributes wa;
  wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
  XChangeWindowAttributes(rwm->dpy, rwm->root, CWEventMask, &wa);
  XSelectInput(rwm->dpy, rwm->root, wa.event_mask);
 
  XEvent ev;
  running = 1;
  while(running && !XNextEvent(rwm->dpy, &ev))
    {
      switch(ev.type) {
      case KeyPress: handle_key(self, &ev); break;
      case MapRequest: map_request(self, &ev);break;
      case ConfigureRequest: configure_request(self, &ev); break;
      case DestroyNotify: destroy_notify(self, &ev); break;
      case PropertyNotify: if((&ev.xproperty)->window == rwm->root) draw_bar(self); break;
      case EnterNotify: enter_notify(self, &ev); break;
      }
    }
  return Qnil;
}

void
Init_rwm()
{
  rb_mRWM = rb_define_module("RWM");
  rb_cDisplay = rb_define_class_under(rb_mRWM, "Display", rb_cObject);
  rb_cWindow = rb_define_class_under(rb_mRWM, "Window", rb_cObject);

  rb_define_const(rb_mRWM, "Shift", INT2NUM(ShiftMask));
  rb_define_const(rb_mRWM, "Control", INT2NUM(ControlMask));

  rb_define_method(rb_cDisplay, "open_display", open_display, 0);
  rb_define_method(rb_cDisplay, "quit", quit, 0);
  rb_define_method(rb_cDisplay, "create_bar", create_bar, 0);
  rb_define_method(rb_cDisplay, "run", run, 0);
  rb_define_method(rb_cDisplay, "sync", sync_display, 0);
  rb_define_method(rb_cDisplay, "register_keys", register_keys, 1);
  rb_define_method(rb_cDisplay, "write_text", write_text, 3);
  rb_define_method(rb_cDisplay, "draw_bar", draw_bar, 0);
  rb_define_method(rb_cDisplay, "get_colour", get_colour, 1);
  rb_define_method(rb_cDisplay, "get_config", get_config, 0);
  rb_define_method(rb_cDisplay, "focus", focus, 1);

  rb_define_method(rb_cWindow, "resize", resize_window, 4);
  rb_define_method(rb_cWindow, "title", get_title, 0);
  rb_define_method(rb_cWindow, "hide", hide, 0);
 
  rb_define_method(rb_define_class_under(rb_mRWM, "Layout", rb_cObject), "register_keys", register_keys, 1);
}
