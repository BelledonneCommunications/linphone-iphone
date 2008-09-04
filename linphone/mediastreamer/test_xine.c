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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "ms.h"
#include "msfilter.h"
#include "msxine.h"
#include <unistd.h>



int main()
{
	MSFilter *xinefilter1,*xinefilter2;
	
	xinefilter1=ms_xine_new();
	//xinefilter2=ms_xine_new();
	//sleep(10);
	ms_xine_start(MS_XINE(xinefilter1));
	sleep(15);
	ms_xine_stop(MS_XINE(xinefilter1));
	ms_filter_destroy(xinefilter1);
	//ms_filter_destroy(xinefilter2);
	g_message("End of test program.");
	return 0;
}


