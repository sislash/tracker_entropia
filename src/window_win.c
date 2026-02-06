/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   window_win.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: you <you@student.42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04                                 #+#    #+#           */
/*   Updated: 2026/02/04                                 #+#    #+#           */
/*                                                                            */
/* ************************************************************************** */

#ifdef _WIN32

# include "window.h"
# include <windows.h>
# include <stdlib.h>
# include <string.h>

typedef struct s_win_backend
{
    HINSTANCE	hinst;
    HWND		hwnd;
    const char	*class_name;
    BITMAPINFO	bmi;
    /* GDI double-buffer (prevents flicker) */
    HDC		back_dc;
    HBITMAP		back_bmp;
    HBITMAP		back_old_bmp;
    /* Monospace font to keep ASCII art aligned (Windows default is proportional) */
    HFONT		font;
    HFONT		old_font;
}	t_win_backend;

static HFONT	ft_create_mono_font(HDC hdc)
{
    int		px_h;
    HFONT	font;
    
    if (!hdc)
        return (NULL);
    /* 16px-ish at current DPI. Negative = character height in pixels. */
    px_h = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    font = CreateFontA(px_h, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    if (!font)
        font = CreateFontA(px_h, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Lucida Console");
    if (!font)
            font = (HFONT)GetStockObject(OEM_FIXED_FONT);
    return (font);
}

static COLORREF	ft_win_color(int rgb)
{
    int	r;
    int	g;
    int	b;
    
    r = (rgb >> 16) & 0xFF;
    g = (rgb >> 8) & 0xFF;
    b = rgb & 0xFF;
    return (RGB(r, g, b));
}

static void	ft_set_key(t_window *w, WPARAM key)
{
    if (key == VK_UP)
        w->key_up = 1;
    else if (key == VK_DOWN)
        w->key_down = 1;
    else if (key == VK_RETURN)
        w->key_enter = 1;
    else if (key == VK_ESCAPE)
        w->key_escape = 1;
    else if (key == 'Z')
        w->key_z = 1;
    else if (key == 'Q')
        w->key_q = 1;
    else if (key == 'S')
        w->key_s = 1;
    else if (key == 'D')
        w->key_d = 1;
}

static void	ft_unset_key(t_window *w, WPARAM key)
{
    if (key == 'Z')
        w->key_z = 0;
    else if (key == 'Q')
        w->key_q = 0;
    else if (key == 'S')
        w->key_s = 0;
    else if (key == 'D')
        w->key_d = 0;
}

static LRESULT CALLBACK	ft_wndproc(HWND hwnd, UINT msg,
                                   WPARAM wparam, LPARAM lparam)
{
    t_window	*w;
    
    w = (t_window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (msg == WM_CREATE)
    {
        w = (t_window *)((CREATESTRUCT *)lparam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)w);
        return (0);
    }
    if (msg == WM_KEYDOWN && w)
        return (ft_set_key(w, wparam), 0);
    if (msg == WM_KEYUP && w)
        return (ft_unset_key(w, wparam), 0);
    if (msg == WM_ERASEBKGND)
        return (1);
    if (msg == WM_CLOSE)
        return (DestroyWindow(hwnd), 0);
    if (msg == WM_DESTROY)
        return (PostQuitMessage(0), 0);
    return (DefWindowProc(hwnd, msg, wparam, lparam));
}

static int	ft_register_class(t_win_backend *b)
{
    WNDCLASSA	wc;
    DWORD		err;
    
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = ft_wndproc;
    wc.hInstance = b->hinst;
    wc.lpszClassName = b->class_name;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    if (RegisterClassA(&wc))
        return (0);
    err = GetLastError();
    if (err == ERROR_CLASS_ALREADY_EXISTS)
        return (0);
    return (1);
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
    t_win_backend	*b;
    RECT			r;
    
    if (!w || width <= 0 || height <= 0)
        return (1);
    b = (t_win_backend *)malloc(sizeof(*b));
    if (!b)
        return (1);
    memset(b, 0, sizeof(*b));
    b->hinst = GetModuleHandleA(NULL);
    b->class_name = "ft_window_class";
    if (ft_register_class(b) != 0)
        return (free(b), 1);
    r.left = 0;
    r.top = 0;
    r.right = width;
    r.bottom = height;
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    b->hwnd = CreateWindowExA(0, b->class_name, title, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                              NULL, NULL, b->hinst, w);
    if (!b->hwnd)
        return (free(b), 1);
    ShowWindow(b->hwnd, SW_SHOW);
    UpdateWindow(b->hwnd);
    
    
    /* Create a compatible backbuffer to eliminate flicker when redrawing */
    {
        HDC hdc;
        hdc = GetDC(b->hwnd);
        b->back_dc = CreateCompatibleDC(hdc);
        b->back_bmp = CreateCompatibleBitmap(hdc, width, height);
        b->back_old_bmp = (HBITMAP)SelectObject(b->back_dc, b->back_bmp);
        /* Force a monospace font on Windows so ASCII art aligns like on Linux */
        b->font = ft_create_mono_font(hdc);
        if (b->font)
            b->old_font = (HFONT)SelectObject(b->back_dc, b->font);
        ReleaseDC(b->hwnd, hdc);
    }
    
    w->pixels = (unsigned int *)malloc((size_t)width * (size_t)height * 4);
    if (!w->pixels)
        return (DestroyWindow(b->hwnd), free(b), 1);
    w->pitch = width;
    
    memset(&b->bmi, 0, sizeof(b->bmi));
    b->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    b->bmi.bmiHeader.biWidth = width;
    b->bmi.bmiHeader.biHeight = -height;
    b->bmi.bmiHeader.biPlanes = 1;
    b->bmi.bmiHeader.biBitCount = 32;
    b->bmi.bmiHeader.biCompression = BI_RGB;
    
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
    MSG	msg;
    
    if (!w)
        return ;
    w->key_up = 0;
    w->key_down = 0;
    w->key_enter = 0;
    w->key_escape = 0;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            w->running = 0;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void	window_clear(t_window *w, int color)
{
    t_win_backend	*b;
    HDC			hdc;
    RECT		rc;
    HBRUSH		brush;
    
    if (!w)
        return ;
    b = (t_win_backend *)w->backend_1;
    if (!b || !b->hwnd)
        return ;
    GetClientRect(b->hwnd, &rc);
    brush = CreateSolidBrush(ft_win_color(color));
    if (w->use_buffer && b->back_dc)
        FillRect(b->back_dc, &rc, brush);
    else
    {
        hdc = GetDC(b->hwnd);
        FillRect(hdc, &rc, brush);
        ReleaseDC(b->hwnd, hdc);
    }
    DeleteObject(brush);
}

void	window_fill_rect(t_window *w, int x, int y, int width, int height,
                         int color)
{
    t_win_backend	*b;
    HDC			hdc;
    RECT		rc;
    HBRUSH		brush;
    
    if (!w || width <= 0 || height <= 0)
        return ;
    b = (t_win_backend *)w->backend_1;
    if (!b || !b->hwnd)
        return ;
    rc.left = x;
    rc.top = y;
    rc.right = x + width;
    rc.bottom = y + height;
    brush = CreateSolidBrush(ft_win_color(color));
    if (w->use_buffer && b->back_dc)
        FillRect(b->back_dc, &rc, brush);
    else
    {
        hdc = GetDC(b->hwnd);
        FillRect(hdc, &rc, brush);
        ReleaseDC(b->hwnd, hdc);
    }
    DeleteObject(brush);
}

void	window_draw_text(t_window *w, int x, int y, const char *text, int color)
{
    t_win_backend	*b;
    HDC			hdc;
    HFONT			old;
    
    if (!w || !text)
        return ;
    b = (t_win_backend *)w->backend_1;
    if (!b || !b->hwnd)
        return ;
    if (w->use_buffer && b->back_dc)
        hdc = b->back_dc;
    else
        hdc = GetDC(b->hwnd);
    old = NULL;
    /* Ensure monospace font for consistent character widths */
    if (b->font && hdc && hdc != b->back_dc)
        old = (HFONT)SelectObject(hdc, b->font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ft_win_color(color));
    TextOutA(hdc, x, y, text, (int)lstrlenA(text));
    if (old)
        SelectObject(hdc, old);
    if (!(w->use_buffer && b->back_dc))
        ReleaseDC(b->hwnd, hdc);
}

void	window_present(t_window *w)
{
    t_win_backend	*b;
    HDC			hdc;
    
    if (!w)
        return ;
    b = (t_win_backend *)w->backend_1;
    if (!b || !b->hwnd)
        return ;
    hdc = GetDC(b->hwnd);
    if (w->use_buffer && b->back_dc && b->back_bmp)
        BitBlt(hdc, 0, 0, w->width, w->height, b->back_dc, 0, 0, SRCCOPY);
    else if (w->use_buffer && w->pixels)
        StretchDIBits(hdc, 0, 0, w->width, w->height, 0, 0, w->width, w->height,
                      w->pixels, &b->bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(b->hwnd, hdc);
}

void	window_destroy(t_window *w)
{
    t_win_backend	*b;
    
    if (!w)
        return ;
    b = (t_win_backend *)w->backend_1;
    if (w->pixels)
        free(w->pixels);
    if (b)
    {
        if (b->back_dc)
        {
            /* Restore/delete font selected into back_dc */
            if (b->old_font)
                SelectObject(b->back_dc, b->old_font);
            if (b->font && b->font != (HFONT)GetStockObject(OEM_FIXED_FONT))
                DeleteObject(b->font);
            if (b->back_old_bmp)
                SelectObject(b->back_dc, b->back_old_bmp);
            if (b->back_bmp)
                DeleteObject(b->back_bmp);
            DeleteDC(b->back_dc);
        }
        if (b->hwnd)
            DestroyWindow(b->hwnd);
        UnregisterClassA(b->class_name, b->hinst);
        free(b);
    }
    w->backend_1 = NULL;
    w->backend_2 = NULL;
    w->backend_3 = NULL;
    w->pixels = NULL;
    w->pitch = 0;
}

#endif
