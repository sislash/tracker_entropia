/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   csv.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: entropia-tracker <entropia-tracker@student.42.fr> +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/31 00:00:00 by entropia-tracker    #+#    #+#             */
/*   Updated: 2026/01/31 00:00:00 by entropia-tracker    ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CSV_H
# define CSV_H

# include <stdio.h>

int		csv_split_n(char *line, char **out, int n);
void	csv_write_field(FILE *f, const char *s);
void	csv_write_row6(FILE *f, const char *f0, const char *f1,
			const char *f2, const char *f3,
			const char *f4, const char *f5);
void	csv_ensure_header6(FILE *f);

#endif
