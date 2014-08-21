/**
 * LaosExtent.cpp
 *
 * Copyright (c) 2011 Joost Nieuwenhuijse
 *
 *   This file is part of the LaOS project (see: http://laoslaser.org)
 *spe
 *   LaOS is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   LaOS is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with LaOS.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 */

#include "LaosExtent.h"
#include "LaosMotion.h"

LaosExtent::LaosExtent()
{
	Reset();
}

void LaosExtent::Reset()
{
	m_MinX=0;
	m_MaxX=0;
	m_MinY=0;
	m_MaxY=0;
	m_HasMinMaxCoordinates=false;
	m_Error=errNone;
	m_Step=0;
	m_Command=0;
}

void LaosExtent::AddToBoundary(int x, int y)
{
	if(!m_HasMinMaxCoordinates)
	{
		// this is the first coordinate where the laser goes on:
		m_MinX=x;
		m_MinY=y;
		m_MaxX=x;
		m_MaxY=y;
	}
	if(x < m_MinX) m_MinX=x;
	if(y < m_MinY) m_MinY=y;
	if(x > m_MaxX) m_MaxX=x;
	if(y > m_MaxY) m_MaxY=y;
	m_HasMinMaxCoordinates=true;
}

LaosExtent::TError LaosExtent::GetBoundary(int &minx, int &miny, int &maxx, int &maxy) const
{
	minx=m_MinX;
	miny=m_MinY;
	maxx=m_MaxX;
	maxy=m_MaxY;
	if( (!m_Error) && (!m_HasMinMaxCoordinates))
	{
		return errEmpty;
	}
	else
	{
		return m_Error;
	}
}
// Feed simplecode
void LaosExtent::Write(int i)
{
//  	printf(">%i (command: %i, step: %i)\n",i,m_Command,m_Step);

  if ( m_Step == 0 )
  {
    m_Command = i;
    m_Step++;
  }
  else
  {
     switch( m_Command )
     {
          case 0: // move x,y (laser off)
          case 1: // line x,y (laser on)
            switch ( m_Step )
            {
              case 1:
                if(m_Command == 1) // ignore moves with the laser off
                {
                	// add previous endpoint to the extent:
                	AddToBoundary(m_TargetX, m_TargetY);
                }
                m_TargetX = i;
                break;
              case 2:
                m_TargetY = i;
                m_Step=0;
                if(m_Command == 1) // ignore moves with the laser off
                {
                	AddToBoundary(m_TargetX, m_TargetY);
                }
                break;
            }
            break;
          case 2: // move z
            // ignored
          	if(m_Step == 1) m_Step=0;
            break;
         case 4: // set x,y,z (absolute)
         	// Not supported
         	if(!m_Error) m_Error = errCoordReferenceChanged;
          	if(m_Step == 3) m_Step=0;
            break;
         case 5: // nop
           m_Step = 0;
           break;
         case 7: // set index,value
         	// ignored
          	if(m_Step == 2) m_Step=0;
          	break;
         case 9: // Store bitmap mark data format: 9 <bpp> <width> <data-0> <data-1> ... <data-n>
            if ( m_Step == 1 )
            {
              m_BitmapBpp = i;
            }
            else if ( m_Step == 2 )
            {
              m_BitmapSize = (m_BitmapBpp * i) / 32;
              if  ( (m_BitmapBpp * i) % 32 )  // padd to next 32-bit
                m_BitmapSize++;

            }
            else if ( m_Step > 2 ) // bitmap data
            {
			  if ( m_Step-2 == m_BitmapSize ) // last dword received
              {
                m_Step = 0;
              }
            }
            break;
         default: // I do not understand:
         	if(!m_Error) m_Error = errFileFormatError;
            m_Step = 0;
            break;
    }
    if ( m_Step )
	  m_Step++;
  }
}

void LaosExtent::ShowBoundaries(LaosMotion *mot) const
{
	if( (!m_Error) && (m_HasMinMaxCoordinates))
	{
		int dummy1, dummy2, z;
    mot->getPlannedPositionRelativeToOrigin(&dummy1, &dummy2, &z);
		for(int step = 0; step <= 4; step++)
		{
			int x,y;
			if( (step == 2)||(step ==3) )
			{
				y=m_MaxY;
			}
			else
			{
				y=m_MinY;
			}
			if( (step == 1)||(step ==2) )
			{
				x=m_MaxX;
			}
			else
			{
				x=m_MinX;
			}
			while(mot->queue()) {} // wait until motion is done
			if(step == 1)
			{
				// wait a bit after going to the origin, so the user knows it's the origin:
			    wait(1.0);
			}
			mot->moveToRelativeToOrigin(x, y, z);
		}
	}
}
