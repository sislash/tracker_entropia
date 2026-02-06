/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   window_linux.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: you <you@student.42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04                                 #+#    #+#           */
/*   Updated: 2026/02/04                                 #+#    #+#           */
/*                                                                            */
/* ************************************************************************** */

#ifndef _WIN32

# include "window.h"
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/keysym.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>

typedef struct s_x11_backend
{
    Display	*d;
    int		screen;
    Window	win;
    GC		gc;
    Atom	wm_delete;
    XImage	*img;
    /* X11 double-buffer (Pixmap) to prevent flicker */
    Pixmap	back;
}	t_x11_backend;

static unsigned long	ft_x11_color(Display *d, int screen, int rgb)
{
    XColor		c;
    Colormap	cm;
    
    cm = DefaultColormap(d, screen);
    c.red = (unsigned short)(((rgb >> 16) & 0xFF) * 257);
    c.green = (unsigned short)(((rgb >> 8) & 0xFF) * 257);
    c.blue = (unsigned short)((rgb & 0xFF) * 257);
    c.flags = DoRed | DoGreen | DoBlue;
    if (XAllocColor(d, cm, &c) == 0)
        return (BlackPixel(d, screen));
    return (c.pixel);
}

static void	ft_clear_buf(t_window *w, unsigned int color)
{
    int	i;
    int	n;
    
    if (!w || !w->pixels)
        return ;
    n = w->pitch * w->height;
    i = 0;
    while (i < n)
    {
        w->pixels[i] = color;
        i++;
    }
}

int	window_init(t_window *w, const char *title, int width, int height)
{
    t_x11_backend	*b;
    Visual			*vis;
    int				depth;
    
    if (!w || width <= 0 || height <= 0)
        return (1);
    b = (t_x11_backend *)malloc(sizeof(*b));
    if (!b)
        return (1);
    memset(b, 0, sizeof(*b));
    b->d = XOpenDisplay(NULL);
    if (!b->d)
        return (free(b), 1);
    b->screen = DefaultScreen(b->d);
    vis = DefaultVisual(b->d, b->screen);
    depth = DefaultDepth(b->d, b->screen);
    b->win = XCreateSimpleWindow(b->d, RootWindow(b->d, b->screen), 0, 0,
                                 (unsigned int)width, (unsigned int)height, 1,
                                 BlackPixel(b->d, b->screen), WhitePixel(b->d, b->screen));
    XStoreName(b->d, b->win, title ? title : "window");
    XSelectInput(b->d, b->win, ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask);
    b->wm_delete = XInternAtom(b->d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(b->d, b->win, &b->wm_delete, 1);
    XMapWindow(b->d, b->win);
    b->gc = XCreateGC(b->d, b->win, 0, NULL);
    
    
    /* Create a Pixmap backbuffer to eliminate flicker when redrawing */
    b->back = XCreatePixmap(b->d, b->win, (unsigned int)width, (unsigned int)height,
                            (unsigned int)depth);
    
    w->pixels = (unsigned int *)malloc((size_t)width * (size_t)height * 4);
    if (!w->pixels)
        return (XCloseDisplay(b->d), free(b), 1);
    b->img = XCreateImage(b->d, vis, (unsigned int)depth, ZPixmap, 0,
                          (char *)w->pixels, width, height, 32, 0);
    if (!b->img)
        return (free(w->pixels), XCloseDisplay(b->d), free(b), 1);
    w->pitch = b->img->bytes_per_line / 4;
    
    w->running = 1;
    w->title = title;
    w->width = width;
    w->height = height;
    w->key_up = 0;
    w->key_down = 0;
    w->key_enter = 0;
    w->key_escape = 0;
    w->key_z = 0;
    w->key_q = 0;
    w->key_s = 0;
    w->key_d = 0;
    w->use_buffer = 1;
    
    w->backend_1 = (void *)b;
    w->backend_2 = NULL;
    w->backend_3 = NULL;
    
    ft_clear_buf(w, 0xFFFFFFFFu);
    return (0);
}

void	window_poll_events(t_window *w)
{
    t_x11_backend	*b;
    XEvent			ev;
    KeySym			ks;
    
    if (!w)
        return ;
    w->key_up = 0;
    w->key_down = 0;
    w->key_enter = 0;
    w->key_escape = 0;
    b = (t_x11_backend *)w->backend_1;
    if (!b || !b->d)
        return ;
    while (XPending(b->d) > 0)
    {
        XNextEvent(b->d, &ev);
        if (ev.type == DestroyNotify)
            w->running = 0;
        else if (ev.type == ClientMessage)
        {
            if ((Atom)ev.xclient.data.l[0] == b->wm_delete)
                w->running = 0;
        }
        else if (ev.type == KeyPress)
        {
            ks = XLookupKeysym(&ev.xkey, 0);
            if (ks == XK_Up)
                w->key_up = 1;
            else if (ks == XK_Down)
                w->key_down = 1;
            else if (ks == XK_Return || ks == XK_KP_Enter)
                w->key_enter = 1;
            else if (ks == XK_Escape)
                w->key_escape = 1;
            else if (ks == XK_z || ks == XK_Z)
                w->key_z = 1;
            else if (ks == XK_q || ks == XK_Q)
                w->key_q = 1;
            else if (ks == XK_s || ks == XK_S)
                w->key_s = 1;
            else if (ks == XK_d || ks == XK_D)
                w->key_d = 1;
        }
        else if (ev.type == KeyRelease)
        {
            ks = XLookupKeysym(&ev.xkey, 0);
            if (ks == XK_z || ks == XK_Z)
                w->key_z = 0;
            else if (ks == XK_q || ks == XK_Q)
                w->key_q = 0;
            else if (ks == XK_s || ks == XK_S)
                w->key_s = 0;
            else if (ks == XK_d || ks == XK_D)
                w->key_d = 0;
        }
    }
}

void	window_clear(t_window *w, int color)
{
    t_x11_backend	*b;
    unsigned long	pix;
    Drawable		dst;
    
    if (!w)
        return ;
    b = (t_x11_backend *)w->backend_1;
    if (!b || !b->d || !b->gc)
        return ;
    dst = (w->use_buffer && b->back) ? b->back : b->win;
    pix = ft_x11_color(b->d, b->screen, color);
    XSetForeground(b->d, b->gc, pix);
    XFillRectangle(b->d, dst, b->gc, 0, 0,
                   (unsigned int)w->width, (unsigned int)w->height);
}

void	window_fill_rect(t_window *w, int x, int y, int width, int height,
                         int color)
{
    t_x11_backend	*b;
    unsigned long	pix;
    Drawable		dst;
    
    if (!w || width <= 0 || height <= 0)
        return ;
    b = (t_x11_backend *)w->backend_1;
    if (!b || !b->d || !b->gc)
        return ;
    dst = (w->use_buffer && b->back) ? b->back : b->win;
    pix = ft_x11_color(b->d, b->screen, color);
    XSetForeground(b->d, b->gc, pix);
    XFillRectangle(b->d, dst, b->gc, x, y,
                   (unsigned int)width, (unsigned int)height);
}

void	window_draw_text(t_window *w, int x, int y, const char *text, int color)
{
    t_x11_backend	*b;
    unsigned long	pix;
    Drawable		dst;
    
    if (!w || !text)
        return ;
    b = (t_x11_backend *)w->backend_1;
    if (!b || !b->d || !b->gc)
        return ;
    dst = (w->use_buffer && b->back) ? b->back : b->win;
    pix = ft_x11_color(b->d, b->screen, color);
    XSetForeground(b->d, b->gc, pix);
    XDrawString(b->d, dst, b->gc, x, y + 16, text, (int)strlen(text));
}

void	window_present(t_window *w)
{
    t_x11_backend	*b;
    
    if (!w)
        return ;
    b = (t_x11_backend *)w->backend_1;
    if (!b || !b->d)
        return ;
    if (w->use_buffer && b->back)
    {
        XCopyArea(b->d, b->back, b->win, b->gc, 0, 0,
                  (unsigned int)w->width, (unsigned int)w->height, 0, 0);
    }
    else if (w->use_buffer && b->img)
    {
        XPutImage(b->d, b->win, b->gc, b->img, 0, 0, 0, 0,
                  (unsigned int)w->width, (unsigned int)w->height);
    }
    XFlush(b->d);
}

void	window_destroy(t_window *w)
{
    t_x11_backend	*b;
    
    if (!w)
        return ;
    b = (t_x11_backend *)w->backend_1;
    if (b)
    {
        if (b->back)
            XFreePixmap(b->d, b->back);
        if (b->img)
        {
            b->img->data = (char *)w->pixels;
            XDestroyImage(b->img);
        }
        if (b->gc)
            XFreeGC(b->d, b->gc);
        if (b->win)
            XDestroyWindow(b->d, b->win);
        if (b->d)
            XCloseDisplay(b->d);
        free(b);
    }
    w->backend_1 = NULL;
    w->backend_2 = NULL;
    w->backend_3 = NULL;
    w->pixels = NULL;
    w->pitch = 0;
}

#endif
