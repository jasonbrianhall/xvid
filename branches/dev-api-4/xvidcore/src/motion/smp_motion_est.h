/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	- Multithreaded motion estimation -
 *
 *  Copyright(C) 2002-2003 Christoph Lampert <gruel@web.de>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: smp_motion_est.h,v 1.4.2.2 2003-06-09 13:54:59 edgomez Exp $
 *
 *************************************************************************/

#ifndef _SMP_MOTION_EST_H
#define _SMP_MOTION_EST_H

#ifdef _SMP

#error SMP support has been removed until B-frame API is stable.

#endif
