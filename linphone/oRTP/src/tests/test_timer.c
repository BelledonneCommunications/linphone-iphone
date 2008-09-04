 /*
  The oRTP LinPhone RTP library intends to provide basics for a RTP stack.
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


#include "../rtptimer.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	RtpTimer *timer=&posix_timer;
	int i;
	struct timeval interval;
	
	interval.tv_sec=0;
	interval.tv_usec=500000;
	
	rtp_timer_set_interval(timer,&interval);
	
	timer->timer_init();
	for (i=0;i<10;i++)
	{
		printf("doing something...\n");
		timer->timer_do();
	}
	timer->timer_uninit();
	return 0;
}
