/* ************************************************************************** */
/*                                                                            */
/*                                                                            */
/*   menu.c                                                                   */
/*                                                                            */
/*   By: you <you@student.42.fr>                                              */
/*                                                                            */
/*   Created: 2026/02/04 00:00:00 by you                                      */
/*   Updated: 2026/02/05 00:00:00 by you                                      */
/*                                                                            */
/* ************************************************************************** */

#include "menu_principale.h"
#include "parser_thread.h"
#include "globals_thread.h"
#include "ui_utils.h"

/*
 * ASCII ART du menu (version fenetre)
 *
 * window_draw_text ne gère qu'une seule ligne, donc on dessine l'art
 * ligne par ligne.
 */

static void	draw_lines(t_window *w, int x, int y, const char **lines, int n)
{
    int	i;
    int	line_h;
    
    if (!w || !lines || n <= 0)
        return ;
    line_h = 18;
    i = 0;
    while (i < n)
    {
        if (lines[i])
            window_draw_text(w, x, y + i * line_h, lines[i], 0x000000);
        i++;
    }
}

static void	draw_menu_art_top(t_window *w, int x, int y)
{
    const char	*lines[] = {
        "|---------------------------------------------------------------------------|",
        "|       ####### ###    ## ######## ####### ####### ####### ## #######       |",
        "|       ##      ## #   ##    ##    ##   ## ##   ## ##   ## ## ##   ##       |",
        "|       #####   ##  #  ##    ##    ####### ##   ## ####### ## #######       |",
        "|       ##      ##   # ##    ##    ## ##   ##   ## ##      ## ##   ##       |",
        "|       ####### ##    ###    ##    ##   ## ####### ##      ## ##   ##       |",
        "|                                                                           |",
        "|                 #  # ##   # # #   # #### #### #### ####                   |",
        "|                 #  # # #  # # #   # #    #  # #    #                      |",
        "|=====            #  # # ## # # #   # ###  #### #### ###               =====|",
        "|                 #  # #  # # #  # #  #    # #     # #                      |",
        "|                 #### #   ## #   #   #### #  # #### ####                   |",
        "|---------------------------------------------------------------------------|",
    };
    
    draw_lines(w, x, y, lines, (int)(sizeof(lines) / sizeof(lines[0])));
}

void	stop_all_parsers(t_window *w, int x, int y)
{
    (void)w;
    (void)x;
    (void)y;

    window_clear(w, 0xFFFFFF);
    
    parser_thread_stop();
    globals_thread_stop();
    
    const char	*lines[] = {
        "OK : tous les parsers sont arretes."
    };
    
    draw_lines(w, x, y, lines, (int)(sizeof(lines) / sizeof(lines[0])));
}
    
static int	ft_clamp(int v, int min, int max)
{
    if (v < min)
        return (min);
    if (v > max)
        return (max);
    return (v);
}

void	menu_init(t_menu *m, const char **items, int count)
{
    m->items = items;
    m->count = count;
    m->selected = 0;
	m->render_x = 0;
	m->render_y = 0;
	m->item_w = 600;
	m->item_h = 36;
}

int	menu_update(t_menu *m, t_window *w)
{
	int	idx;

    if (w->key_up)
        m->selected--;
    if (w->key_down)
        m->selected++;
    m->selected = ft_clamp(m->selected, 0, m->count - 1);
	if (w->mouse_left_click)
	{
		if (w->mouse_x >= m->render_x && w->mouse_x < m->render_x + m->item_w
			&& w->mouse_y >= m->render_y
			&& w->mouse_y < m->render_y + m->count * m->item_h)
		{
			idx = (w->mouse_y - m->render_y) / m->item_h;
			idx = ft_clamp(idx, 0, m->count - 1);
			m->selected = idx;
			return (m->selected);
		}
	}
    if (w->key_enter)
        return (m->selected);
    if (w->key_escape)
        return (m->count - 1);
    return (-1);
}

static void	draw_item(t_menu *m, t_window *w, int x, int base_y, int idx)
{
    int			bg;
    int			fg;
	const int	item_h = 36;
    
    bg = 0xDDDDDD;
    fg = 0x000000;
    if (m->selected == idx)
    {
        bg = 0x111111;
        fg = 0xFFFFFF;
    }
	window_fill_rect(w, x, base_y + idx * item_h, m->item_w, item_h - 4, bg);
    window_draw_text(w, x + 12, base_y + idx * item_h + 8, m->items[idx], fg);
}

void	menu_render(t_menu *m, t_window *w, int x, int y)
{
    int	i;

	/* Store last render position for mouse hit-testing */
	m->render_x = x;
	m->render_y = y;
    
	/* Save render position for mouse-based selection */
	m->render_x = x;
	m->render_y = y;
	m->item_w = 600;
	m->item_h = 36;

    i = 0;
    while (i < m->count)
    {
        draw_item(m, w, x, y, i);
        i++;
    }
}

void	menu_render_screen(t_menu *m, t_window *w, int x, int y)
{
    int	art_x;
    int	art_top_y;
    int	menu_y;
    
    (void)y;

    window_clear(w, 0xFFFFFF);
    art_x = 20;
    art_top_y = 10;
    menu_y = 300;
    draw_menu_art_top(w, art_x, art_top_y);
    
    menu_render(m, w, x, menu_y);
    window_present(w);
}
