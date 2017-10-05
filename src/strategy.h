// -*-c++-*-

/*!
  \file strategy.h
  \brief team strategy manager Header File
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef STRATEGY_H
#define STRATEGY_H

#include <rcsc/geom/vector_2d.h>

#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>
#include <string>


namespace rcsc {
class CmdLineParser;
class WorldModel;
}


enum SituationType {
    Normal_Situation,
    Offense_Situation,
    Defense_Situation,
    OurSetPlay_Situation,
    OppSetPlay_Situation,
    PenaltyKick_Situation,
};


class Strategy {
public:

private:
    int M_goalie_unum;


    // situation type
    SituationType M_current_situation;

    // current home positions
    std::vector< rcsc::Vector2D > M_positions;

    // private for singleton
    Strategy();

    // not used
    Strategy( const Strategy & );
    const Strategy & operator=( const Strategy & );
public:

    static
    Strategy & instance();

    static
    const
    Strategy & i()
      {
          return instance();
      }

    //
    // initialization
    //

    bool init( rcsc::CmdLineParser & cmd_parser );

    //
    // update
    //

    void update( const rcsc::WorldModel & wm );


    // accessor to the current information
    //

    int goalieUnum() const { return M_goalie_unum; }

    rcsc::Vector2D getPosition( const int unum ) const;


private:
    void updateSituation( const rcsc::WorldModel & wm );
    // update the current position table
    void updatePosition( const rcsc::WorldModel & wm );

public:

    static
    double get_normal_dash_power( const rcsc::WorldModel & wm );
};

#endif
