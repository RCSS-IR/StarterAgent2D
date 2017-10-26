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

#include "bhv_goalie_basic_move.h"

#include "bhv_basic_tackle.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_turn_to_point.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/bhv_go_to_point_look_ball.h>
#include <rcsc/action/body_intercept.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/line_2d.h>
#include <rcsc/soccer_math.h>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_GoalieBasicMove::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    const Vector2D move_point = getTargetPoint( agent );

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_GoalieBasicMove. move_point(%.2f %.2f)",
                  move_point.x, move_point.y );

    //////////////////////////////////////////////////////////////////
    // tackle
    if ( Bhv_BasicTackle( 0.8, 90.0 ).execute( agent ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": tackle" );
        return true;
    }

    if (self_min < opp_min && self_min < mate_min){
        Body_Intercept2009().execute(agent);
        return true;
    }
    if (!Body_GoToPoint2010(move_point,1.0,100).execute(agent))
        Body_TurnToPoint(move_point).execute(agent);

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
Bhv_GoalieBasicMove::getTargetPoint( PlayerAgent * agent )
{
    const double base_move_x = -49.8;
    const double danger_move_x = -51.5;
    const WorldModel & wm = agent->world();

    int ball_reach_step = 0;
    if ( ! wm.existKickableTeammate()
         && ! wm.existKickableOpponent() )
    {
        ball_reach_step
            = std::min( wm.interceptTable()->teammateReachCycle(),
                        wm.interceptTable()->opponentReachCycle() );
    }
    const Vector2D base_pos = wm.ball().inertiaPoint( ball_reach_step );


    //---------------------------------------------------------//
    // angle is very dangerous
    if ( base_pos.y > ServerParam::i().goalHalfWidth() + 3.0 )
    {
        Vector2D right_pole( - ServerParam::i().pitchHalfLength(),
                             ServerParam::i().goalHalfWidth() );
        AngleDeg angle_to_pole = ( right_pole - base_pos ).th();

        if ( -140.0 < angle_to_pole.degree()
             && angle_to_pole.degree() < -90.0 )
        {
            agent->debugClient().addMessage( "RPole" );
            return Vector2D( danger_move_x, ServerParam::i().goalHalfWidth() + 0.001 );
        }
    }
    else if ( base_pos.y < -ServerParam::i().goalHalfWidth() - 3.0 )
    {
        Vector2D left_pole( - ServerParam::i().pitchHalfLength(),
                            - ServerParam::i().goalHalfWidth() );
        AngleDeg angle_to_pole = ( left_pole - base_pos ).th();

        if ( 90.0 < angle_to_pole.degree()
             && angle_to_pole.degree() < 140.0 )
        {
            agent->debugClient().addMessage( "LPole" );
            return Vector2D( danger_move_x, - ServerParam::i().goalHalfWidth() - 0.001 );
        }
    }

    //---------------------------------------------------------//
    // ball is close to goal line
    if ( base_pos.x < -ServerParam::i().pitchHalfLength() + 8.0
         && base_pos.absY() > ServerParam::i().goalHalfWidth() + 2.0 )
    {
        Vector2D target_point( base_move_x, ServerParam::i().goalHalfWidth() - 0.1 );
        if ( base_pos.y < 0.0 )
        {
            target_point.y *= -1.0;
        }

        dlog.addText( Logger::TEAM,
                      __FILE__": getTarget. target is goal pole" );
        agent->debugClient().addMessage( "Pos(1)" );

        return target_point;
    }

//---------------------------------------------------------//
    {
        const double x_back = 7.0; // tune this!!
        int ball_pred_cycle = 5; // tune this!!
        const double y_buf = 0.5; // tune this!!
        const Vector2D base_point( - ServerParam::i().pitchHalfLength() - x_back,
                                   0.0 );
        Vector2D ball_point;
        if ( wm.existKickableOpponent() )
        {
            ball_point = base_pos;
            agent->debugClient().addMessage( "Pos(2)" );
        }
        else
        {
            int opp_min = wm.interceptTable()->opponentReachCycle();
            if ( opp_min < ball_pred_cycle )
            {
                ball_pred_cycle = opp_min;
                dlog.addText( Logger::TEAM,
                              __FILE__": opp may reach near future. cycle = %d",
                              opp_min );
            }

            ball_point
                = inertia_n_step_point( base_pos,
                                        wm.ball().vel(),
                                        ball_pred_cycle,
                                        ServerParam::i().ballDecay() );
            agent->debugClient().addMessage( "Pos(3)" );
        }

        if ( ball_point.x < base_point.x + 0.1 )
        {
            ball_point.x = base_point.x + 0.1;
        }

        Line2D ball_line( ball_point, base_point );
        double move_y = ball_line.getY( base_move_x );

        if ( move_y > ServerParam::i().goalHalfWidth() - y_buf )
        {
            move_y = ServerParam::i().goalHalfWidth() - y_buf;
        }
        if ( move_y < - ServerParam::i().goalHalfWidth() + y_buf )
        {
            move_y = - ServerParam::i().goalHalfWidth() + y_buf;
        }

        return Vector2D( base_move_x, move_y );
    }
}

