/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   window.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: you <you@student.42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04                                 #+#    #+#           */
/*   Updated: 2026/02/04                                 #+#    #+#           */
/*                                                                            */
/* ************************************************************************** */

#ifndef WINDOW_H
# define WINDOW_H

typedef struct s_window
{
    int				running;
    const char		*title;
    int				width;
    int				height;
    
    int				key_up;
    int				key_down;
    int				key_enter;
    int				key_escape;
    
    int			key_z;
	int			key_q;
	int			key_s;
	int			key_d;

    int				use_buffer;
    
    void			*backend_1;
    void			*backend_2;
    void			*backend_3;
    
    unsigned int	*pixels;
    int				pitch;
}	t_window;

int		window_init(t_window *w, const char *title, int width, int height);
void	window_poll_events(t_window *w);

void	window_clear(t_window *w, int color);
void	window_fill_rect(t_window *w, int x, int y, int width, int height,
                         int color);
void	window_draw_text(t_window *w, int x, int y, const char *text, int color);

void	window_present(t_window *w);
void	window_destroy(t_window *w);

#endif
