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

#include "bhv_penalty_kick.h"
#include "bhv_basic_offensive_kick.h"
#include "bhv_goalie_basic_move.h"
#include "bhv_set_play.h"

#include <rcsc/action/body_intercept.h>
#include <rcsc/action/body_smart_kick.h>

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_go_to_point_look_ball.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/body_stop_dash.h>
#include <rcsc/action/body_stop_ball.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/penalty_kick_state.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/ray_2d.h>
#include <rcsc/soccer_math.h>
#include <rcsc/math_util.h>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

    switch ( wm.gameMode().type() ) {
    case GameMode::PenaltySetup_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKickerSetup( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyReady_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKickerReady( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyTaken_:
        if ( state->currentTakerSide() == agent->world().ourSide() )
        {
            if ( state->isKickTaker( wm.ourSide(), wm.self().unum() ) )
            {
                return doKicker( agent );
            }
        }
        // their kick phase
        else
        {
            if ( wm.self().goalie() )
            {
                return doGoalie( agent );
            }
        }
        break;
    case GameMode::PenaltyScore_:
    case GameMode::PenaltyMiss_:
        if ( state->currentTakerSide() == wm.ourSide() )
        {
            if ( wm.self().goalie() )
            {
                return doGoalieSetup( agent );
            }
        }
        break;
    case GameMode::PenaltyOnfield_:
    case GameMode::PenaltyFoul_:
        break;
    default:
        // nothing to do.
        std::cerr << "Current playmode is NOT a Penalty Shootout???" << std::endl;
        return false;
    }


    if ( wm.self().goalie() )
    {
        return doGoalieWait( agent );
    }
    else
    {
        return doKickerWait( agent );
    }

    // never reach here
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKickerWait( PlayerAgent * agent )
{
    double dist_step = ( 9.0 + 9.0 ) / 12;
    Vector2D wait_pos( -2.0, -9.8 + dist_step * agent->world().self().unum() );

    // already there
    if ( agent->world().self().pos().dist( wait_pos ) < 0.7 )
    {
        Bhv_NeckBodyToBall().execute( agent );
    }
    else
    {
        // no dodge
        Body_GoToPoint( wait_pos,
                        0.3,
                        ServerParam::i().maxDashPower()
                        ).execute( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */

bool
Bhv_PenaltyKick::doKickerSetup( PlayerAgent * agent )
{
    const Vector2D goal_c = ServerParam::i().theirTeamGoalPos();

    // ball is close enoughly.

    if ( ! Bhv_SetPlay::GoToStaticBall(agent, 0.0) )
    {
        Body_TurnToPoint( goal_c ).execute( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKickerReady( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();
    const PenaltyKickState * state = wm.penaltyKickState();

    // stamina recovering...
    if ( wm.self().stamina() < ServerParam::i().staminaMax() - 10.0
         && ( wm.time().cycle() - state->time().cycle() > ServerParam::i().penReadyWait() - 3 ) )
    {
        return doKickerSetup( agent );
    }

    if ( ! wm.self().isKickable() )
    {
        return doKickerSetup( agent );
    }

    return doKicker( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doKicker( PlayerAgent * agent )
{
    //
    // server allows multiple kicks
    //

    const WorldModel & wm = agent->world();

    // get ball
    if ( ! wm.self().isKickable() )
    {
        if ( ! Body_Intercept().execute( agent ) )
        {
            Body_GoToPoint( wm.ball().pos(),
                            0.4,
                            ServerParam::i().maxDashPower()
                            ).execute( agent );
        }

        return true;
    }

    // kick decision
    if ( doShoot( agent ) )
    {
        return true;
    }

    return doDribble( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doShoot( PlayerAgent * agent )
{
    if(Bhv_BasicOffensiveKick().shoot(agent))
        return true;

    return false;
}

/*-------------------------------------------------------------------*/
/*!
  dribble to the shootable point
*/
bool
Bhv_PenaltyKick::doDribble( PlayerAgent * agent )
{
    if(Bhv_BasicOffensiveKick().dribble(agent))
        return true;
    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieWait( PlayerAgent* agent )
{
    Body_TurnToBall().execute( agent );
    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieSetup( PlayerAgent * agent )
{
    Vector2D move_point( ServerParam::i().ourTeamGoalLineX() + ServerParam::i().penMaxGoalieDistX() - 0.1,
                         0.0 );

    if ( Body_GoToPoint( move_point,
                         0.5,
                         ServerParam::i().maxDashPower()
                         ).execute( agent ) )
    {
        return true;
    }

    // already there
    if ( std::fabs( agent->world().self().body().abs() ) > 2.0 )
    {
        Vector2D face_point( 0.0, 0.0 );
        Body_TurnToPoint( face_point ).execute( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalie( PlayerAgent* agent )
{
    const ServerParam & SP = ServerParam::i();
    const WorldModel & wm = agent->world();

    ///////////////////////////////////////////////
    // check if catchabale
    Rect2D our_penalty( Vector2D( -SP.pitchHalfLength(),
                                  -SP.penaltyAreaHalfWidth() + 1.0 ),
                        Size2D( SP.penaltyAreaLength() - 1.0,
                                SP.penaltyAreaWidth() - 2.0 ) );

    if ( wm.ball().distFromSelf() < SP.catchableArea() - 0.05
         && our_penalty.contains( wm.ball().pos() ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": goalie try to catch" );
        return agent->doCatch();
    }

    if ( wm.self().isKickable() )
    {
        Bhv_BasicOffensiveKick().clearball(agent);
        return true;
    }


    return doGoalieBasicMove( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_PenaltyKick::doGoalieBasicMove( PlayerAgent * agent )
{
    Bhv_GoalieBasicMove().execute(agent);
    return true;
}
