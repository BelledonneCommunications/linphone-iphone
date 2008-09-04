/*
 * affine.c -- Affine Transforms for 2d objects
 * Copyright (C) 2002 Charles Yates <charles.yates@pandora.be>
 * Portions Copyright (C) 2003 Dan Dennedy <dan@dennedy.org>
 * 		ported from C++ to C
 *		wrote affine_scale()
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

#include "affine.h"

static inline void Multiply( affine_transform_t *this, affine_transform_t *that )
{
	double output[2][2];
	register int i, j;

	for ( i = 0; i < 2; i ++ )
		for ( j = 0; j < 2; j ++ )
			output[ i ][ j ] = this->matrix[ i ][ 0 ] * that->matrix[ j ][ 0 ] +
							   this->matrix[ i ][ 1 ] * that->matrix[ j ][ 1 ];

	this->matrix[ 0 ][ 0 ] = output[ 0 ][ 0 ];
	this->matrix[ 0 ][ 1 ] = output[ 0 ][ 1 ];
	this->matrix[ 1 ][ 0 ] = output[ 1 ][ 0 ];
	this->matrix[ 1 ][ 1 ] = output[ 1 ][ 1 ];
}

void affine_transform_init( affine_transform_t *this )
{
	this->matrix[ 0 ][ 0 ] = 1;
	this->matrix[ 0 ][ 1 ] = 0;
	this->matrix[ 1 ][ 0 ] = 0;
	this->matrix[ 1 ][ 1 ] = 1;
}

// Rotate by a given angle
void affine_transform_rotate( affine_transform_t *this, double angle )
{
	affine_transform_t affine;
	affine.matrix[ 0 ][ 0 ] = cos( angle * M_PI / 180 );
	affine.matrix[ 0 ][ 1 ] = 0 - sin( angle * M_PI / 180 );
	affine.matrix[ 1 ][ 0 ] = sin( angle * M_PI / 180 );
	affine.matrix[ 1 ][ 1 ] = cos( angle * M_PI / 180 );
	Multiply( this, &affine );
}

// Shear by a given value
void affine_transform_shear( affine_transform_t *this, double shear )
{
	affine_transform_t affine;
	affine.matrix[ 0 ][ 0 ] = 1;
	affine.matrix[ 0 ][ 1 ] = shear;
	affine.matrix[ 1 ][ 0 ] = 0;
	affine.matrix[ 1 ][ 1 ] = 1;
	Multiply( this, &affine );
}

void affine_transform_scale( affine_transform_t *this, double sx, double sy )
{
	affine_transform_t affine;
	affine.matrix[ 0 ][ 0 ] = sx;
	affine.matrix[ 0 ][ 1 ] = 0;
	affine.matrix[ 1 ][ 0 ] = 0;
	affine.matrix[ 1 ][ 1 ] = sy;
	Multiply( this, &affine );
}

// Obtain the mapped x coordinate of the input
double affine_transform_mapx( affine_transform_t *this, int x, int y )
{
	return this->matrix[0][0] * x + this->matrix[0][1] * y;
}

// Obtain the mapped y coordinate of the input
double affine_transform_mapy( affine_transform_t *this, int x, int y )
{
	return this->matrix[1][0] * x + this->matrix[1][1] * y;
}

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

void affine_scale( const unsigned char *src, unsigned char *dest, int src_width, int src_height, int dest_width, int dest_height, int bpp )
{	
	affine_transform_t affine;
	double scale_x = (double) dest_width / (double) src_width;
	double scale_y = (double) dest_height / (double) src_height;
	register unsigned char *d = dest;
    register const unsigned char  *s = src;
	register int i, j, k, x, y;
	
	affine_transform_init( &affine );
	
	if ( scale_x <= 1.0 && scale_y <= 1.0 )
	{
		affine_transform_scale( &affine, scale_x, scale_y );

		for( j = 0; j < src_height; j++ )
			for( i = 0; i < src_width; i++ )
			{
				x = (int) ( affine_transform_mapx( &affine, i - src_width/2, j - src_height/2 ) );
				y = (int) ( affine_transform_mapy( &affine, i - src_width/2, j - src_height/2 ) );
				x += dest_width/2;
				x = CLAMP( x, 0, dest_width);
				y += dest_height/2;
				y = CLAMP( y, 0, dest_height);
				s = src + (j*src_width*bpp) + i*bpp; // + (bpp-1);
				d = dest + y*dest_width*bpp + x*bpp;
				for ( k = 0; k < bpp; k++ )
					*d++ = *s++;
			}
	}
	else if ( scale_x > 1.0 && scale_y > 1.0 )
	{
		affine_transform_scale( &affine, 1.0/scale_x, 1.0/scale_y );
	
		for( y = 0; y < dest_height; y++ )
			for( x = 0; x < dest_width; x++ )
			{
				i = (int) ( affine_transform_mapx( &affine, x - dest_width/2, y - dest_height/2 ) );
				j = (int) ( affine_transform_mapy( &affine, x - dest_width/2, y - dest_height/2 ) );
				i += src_width/2;
				i = CLAMP( i, 0, dest_width);
				j += src_height/2;
				j = CLAMP( j, 0, dest_height);
				s = src + (j*src_width*bpp) + i*bpp; // + (bpp-1);
				d = dest + y*dest_width*bpp + x*bpp;
				for ( k = 0; k < bpp; k++ )
					*d++ = *s++;
			}
	}
}
