// -*-c++-*-

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

//Student Soccer 2D Simulation Base , STDAGENT2D
//Simplified the Agent2D Base for HighSchool Students.
//Technical Committee of Soccer 2D Simulation League, IranOpen
//Nader Zare
//Mostafa Sayahi
//Pooria Kaviani
/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_basic_tackle.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/ray_2d.h>

//#define DEBUG_PRINT

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicTackle::execute( PlayerAgent * agent )
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    bool use_foul = false;
    double tackle_prob = wm.self().tackleProbability();

    if ( wm.self().card() == NO_CARD
         && tackle_prob < wm.self().foulProbability() )
    {
        tackle_prob = wm.self().foulProbability();
        use_foul = true;
    }

    if ( tackle_prob < M_min_probability )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": failed. low tackle_prob=%.2f < %.2f",
                      wm.self().tackleProbability(),
                      M_min_probability );
        return false;
    }

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    const Vector2D self_reach_point = wm.ball().inertiaPoint( self_min );

    //
    // check where the ball shall be gone without tackle
    //

    bool ball_will_be_in_our_goal = false;

    if ( self_reach_point.x < -SP.pitchHalfLength() )
    {
        const Ray2D ball_ray( wm.ball().pos(), wm.ball().vel().th() );
        const Line2D goal_line( Vector2D( -SP.pitchHalfLength(), 10.0 ),
                                Vector2D( -SP.pitchHalfLength(), -10.0 ) );

        const Vector2D intersect = ball_ray.intersection( goal_line );
        if ( intersect.isValid()
             && intersect.absY() < SP.goalHalfWidth() + 1.0 )
        {
            ball_will_be_in_our_goal = true;

            dlog.addText( Logger::TEAM,
                          __FILE__": ball will be in our goal. intersect=(%.2f %.2f)",
                          intersect.x, intersect.y );
        }
    }

    if ( wm.existKickableOpponent()
         || ball_will_be_in_our_goal
         || ( opp_min < self_min - 3
              && opp_min < mate_min - 3 )
         || ( self_min >= 5
              && wm.ball().pos().dist2( SP.theirTeamGoalPos() ) < std::pow( 10.0, 2 )
              && ( ( SP.theirTeamGoalPos() - wm.self().pos() ).th() - wm.self().body() ).abs() < 45.0 )
         )
    {
        // try tackle

        double tackle_dir = 0.0;

        agent->doTackle( tackle_dir, use_foul );

        return true;
    }
    else
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": Bhv_BasicTackle. not necessary" );
        return false;
    }
    return false;

}
