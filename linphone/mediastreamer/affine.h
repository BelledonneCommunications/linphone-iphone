/*
 * affine.h -- Affine Transforms for 2d objects
 * Copyright (C) 2002 Charles Yates <charles.yates@pandora.be>
 * Portions Copyright (C) 2003 Dan Dennedy <dan@dennedy.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _AFFINE_H
#define _AFFINE_H

#include <math.h>

/** Affine transforms for 2d image manipulation. Current provides shearing and 
	rotating support.
*/

typedef struct {
	double matrix[2][2];
} affine_transform_t;

void affine_transform_init( affine_transform_t *this );
void affine_transform_rotate( affine_transform_t *this, double angle );
void affine_transform_shear( affine_transform_t *this, double shear );
void affine_transform_scale( affine_transform_t *this, double sx, double sy );
double affine_transform_mapx( affine_transform_t *this, int x, int y );
double affine_transform_mapy( affine_transform_t *this, int x, int y );
void affine_scale( const unsigned char *src, unsigned char *dest, int src_width, int src_height, int dest_width, int dest_height, int bpp );

#endif

