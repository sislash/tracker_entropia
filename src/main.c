/* ************************************************************************** */
/*                                                                            */
/*                                                                            */
/*   main.c                                                                   */
/*                                                                            */
/*   By: you <you@student.42.fr>                                              */
/*                                                                            */
/*   Created: 2026/02/04                                                      */
/*   Updated: 2026/02/04                                                      */
/*                                                                            */
/* ************************************************************************** */

#include "menu_principale.h"   /* menu_init / menu_update / menu_render_screen */
#include "menu_tracker_chasse.h"
#include "menu_globals.h"
#include "utils.h"  /* ft_time_ms / ft_sleep_ms */
#include <stdlib.h> /* (plus utilisé ici, peut être supprimé si tu veux) */

/*
 * * main :
 ** - Initialise le menu + la fenêtre
 ** - Boucle principale :
 **   - calcule dt
 **   - poll events
 **   - affiche l’écran courant (menu ou placeholder)
 **   - limite le framerate
 */
int	main(void)
{
	/* Fenêtre + menu + items */
	t_window		w;
	t_menu			menu;
	const char		*items[4];
	
	/* Gestion d’écrans + retour du menu */
	int				screen;
	int				action;
	
	/* Timing (delta time + framerate) */
	uint64_t		last_ms;
	uint64_t		now_ms;
	uint64_t		frame_start;
	uint64_t		frame_ms;
	float			dt;
	int				sleep_ms;
	
	/* Textes du menu (4 entrées) */
	items[0] = "Menu CHASSE (tout: parser, armes, stats, dashboard, sweat)";
	items[1] = "Menu GLOBALS (parser + dashboard separe + CSV)";
	items[2] = "STOP ALL (arreter tous les parsers)";
	items[3] = "Quitter";
	
	/* Initialise l’état du menu avec 4 items */
	menu_init(&menu, items, 4);
	
	/* Crée la fenêtre (si échec -> exit) */
	if (window_init(&w, "tracker_loot", 1024, 768) != 0)
		return (1);
	
	/* On démarre sur le menu */
	screen = 0;
	last_ms = ft_time_ms();
	
	/* Loop tant que la fenêtre est ouverte */
	while (w.running)
	{
		/* Petit bouton retour réutilisable sur les écrans "Echap" */
		const char	*back_items[1] = { "Retour menu" };
		t_menu		back_menu;
		int			back_action;
		int			back_y;
		menu_init(&back_menu, back_items, 1);
		/* Début de frame + calcul dt (en secondes) */
		frame_start = ft_time_ms();
		now_ms = frame_start;
		dt = (float)(now_ms - last_ms) / 1000.0f;
		last_ms = now_ms;
		
		/* Clamp dt pour éviter des “sauts” si grosse latence */
		if (dt > 0.10f)
			dt = 0.10f;
		
		/* Lit les événements (clavier, fermeture fenêtre, etc.) */
		window_poll_events(&w);
		
		/* Écran 0 : menu principal */
		if (screen == 0)
		{
			/* Met à jour la sélection / valide un item -> renvoie action */
			action = menu_update(&menu, &w);
			
			/* Routage simple en fonction du choix */
			if (action == 0)
				menu_tracker_chasse(&w);
			else if (action == 1)
				menu_globals(&w);
			else if (action == 2)
				screen = 3;
			else if (action == 3)
				w.running = 0;
			
			/* Rend l’écran menu */
			menu_render_screen(&menu, &w, 40, 90);
		}
		/* Écran 1 : placeholder */
		else if (screen == 1)
		{
			if (w.key_escape)
				screen = 0;
			menu_tracker_chasse(&w);
		}
		/* Écran 2 : placeholder */
		else if (screen == 2)
		{
			if (w.key_escape)
				screen = 0;
			menu_globals(&w);
		}
		/* Écran 3 : placeholder options/stop */
		else if (screen == 3)
		{
			/*
			** Mouse hit-testing in menu_update() relies on m->render_x/y.
			** back_menu is re-initialized every frame, so we must pre-set
			** its render position *before* calling menu_update(), otherwise
			** mouse clicks never register (back button looks clickable but isn't).
			*/
			back_y = w.height - 60;
			if (back_y < 0)
				back_y = 0;
			back_menu.render_x = 60;
			back_menu.render_y = back_y;
			back_menu.item_w = 600;
			back_menu.item_h = 36;
			back_action = menu_update(&back_menu, &w);
			if (w.key_escape || back_action == 0)
				screen = 0;

			stop_all_parsers(&w, 60, 70);
			/* Keep the back button visible on any screen size */
			menu_render(&back_menu, &w, 60, back_y);
			window_present(&w);
		}
		else if (screen == 4)
		{
			back_y = w.height - 60;
			if (back_y < 0)
				back_y = 0;
			back_menu.render_x = 40;
			back_menu.render_y = back_y;
			back_menu.item_w = 600;
			back_menu.item_h = 36;
			back_action = menu_update(&back_menu, &w);
			if (w.key_escape || back_action == 0)
				screen = 0;
			
			window_clear(&w, 0xFFFFFF);
			window_draw_text(&w, 40, 80,
							 "Options (placeholder) - Echap pour revenir", 0x000000);
			/* Keep the back button visible on any screen size */
			menu_render(&back_menu, &w, 40, back_y);
			window_present(&w);
		}
		
		/* Limiteur FPS (~60fps) : on dort le temps restant */
		frame_ms = ft_time_ms() - frame_start;
		sleep_ms = 16 - (int)frame_ms;
		if (sleep_ms > 0)
			ft_sleep_ms(sleep_ms);
	}
	
	/* Nettoyage */
	window_destroy(&w);
	return (0);
}
