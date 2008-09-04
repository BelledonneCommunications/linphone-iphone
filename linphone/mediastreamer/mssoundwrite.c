/*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org
  										
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation
  
  */
  
#include "mssoundwrite.h"


void ms_sound_write_init(MSSoundWrite *w)
{
	ms_filter_init(MS_FILTER(w));
	
}

void ms_sound_write_class_init(MSSoundWriteClass *klass)
{
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	MS_FILTER_CLASS(klass)->max_finputs=1;  /* one fifo output only */
	
	ms_filter_class_set_attr( MS_FILTER_CLASS(klass),FILTER_IS_SINK);
}

