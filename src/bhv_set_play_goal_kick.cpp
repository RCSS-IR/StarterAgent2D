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

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_set_play_goal_kick.h"
#include "bhv_basic_offensive_kick.h"
#include "bhv_basic_move.h"

#include "bhv_set_play.h"
#include "bhv_prepare_set_play_kick.h"
#include "bhv_go_to_static_ball.h"

#include "intention_wait_after_set_play_kick.h"

#include <rcsc/action/body_clear_ball.h>
#include <rcsc/action/body_stop_ball.h>
#include <rcsc/action/body_intercept.h>

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_kick_one_step.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include <rcsc/geom/rect_2d.h>

#include <rcsc/math_util.h>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!
  execute action
*/
bool
Bhv_SetPlayGoalKick::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_SetPlayGoalKick" );

    if ( Bhv_SetPlay::is_kicker( agent ) )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SetPlayGoalKick::doKick( PlayerAgent * agent )
{
    // go to ball
    if ( Bhv_GoToStaticBall( 0.0 ).execute( agent ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKick) go to ball" );
        // tring to reach the ball yet
        return;
    }

    // already ball point

    if ( doKickWait( agent ) )
    {
        return;
    }

    if ( doPass( agent ) )
    {
        return;
    }

    Body_ClearBall().execute( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlayGoalKick::doKickWait( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const int real_set_play_count
        = static_cast< int >( wm.time().cycle() - wm.lastSetPlayStartTime().cycle() );

    if ( real_set_play_count >= ServerParam::i().dropBallTime() - 10 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) real set play count = %d > drop_time-10, no wait",
                      real_set_play_count );
        return false;
    }

    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        agent->debugClient().addMessage( "GoalKick:Delaying" );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) delaying" );

        Body_TurnToBall().execute( agent );
        return true;
    }

    // face to the ball
    if ( ( wm.ball().angleFromSelf() - wm.self().body() ).abs() > 3.0 )
    {
        agent->debugClient().addMessage( "GoalKick:TurnToBall" );
        Body_TurnToBall().execute( agent );
        return true;
    }

    if ( wm.setplayCount() <= 6 )
    {
        agent->debugClient().addMessage( "GoalKick:Wait%d", wm.setplayCount() );
        Body_TurnToBall().execute( agent );
        return true;
    }

    if ( wm.setplayCount() <= 30
         && wm.teammatesFromSelf().empty() )
    {
        agent->debugClient().addMessage( "GoalKick:NoTeammate%d", wm.setplayCount() );
        Body_TurnToBall().execute( agent );
        return true;
    }

    if ( wm.setplayCount() >= 15
         && wm.seeTime() == wm.time()
         && wm.self().stamina() > ServerParam::i().staminaMax() * 0.6 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) set play count = %d, force kick mode",
                      wm.setplayCount() );
        return false;
    }

    if ( wm.setplayCount() <= 3
         || wm.seeTime() != wm.time()
         || wm.self().stamina() < ServerParam::i().staminaMax() * 0.9 )
    {
        Body_TurnToBall().execute( agent );

        agent->debugClient().addMessage( "GoalKick:Wait%d", wm.setplayCount() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) no see or recover" );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlayGoalKick::doPass( PlayerAgent * agent )
{
	if(Bhv_BasicOffensiveKick().pass(agent)){
	    	return true;
	    }
	return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlayGoalKick::doIntercept( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( wm.ball().pos().x < -ServerParam::i().pitchHalfLength() + ServerParam::i().goalAreaLength() + 1.0
         && wm.ball().pos().absY() < ServerParam::i().goalAreaWidth() * 0.5 + 1.0 )
    {
        return false;
    }

    if ( wm.self().isKickable() )
    {
        return false;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    if ( self_min > mate_min )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doIntercept) other ball kicker" );
        return false;
    }

    Vector2D trap_pos = wm.ball().inertiaPoint( self_min );
    if ( ( trap_pos.x > ServerParam::i().ourPenaltyAreaLineX() - 8.0
           && trap_pos.absY() > ServerParam::i().penaltyAreaHalfWidth() - 5.0 )
         || wm.ball().vel().r2() < std::pow( 0.5, 2 ) )
    {
        agent->debugClient().addMessage( "GoalKick:Intercept" );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doIntercept) intercept" );

        Body_Intercept().execute( agent );
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Bhv_SetPlayGoalKick::doMove( PlayerAgent * agent )
{
    if ( doIntercept( agent ) )
    {
        return;
    }

    const WorldModel & wm = agent->world();

    double dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    Vector2D target_point = Bhv_BasicMove().getPosition( wm, wm.self().unum() );
    target_point.y += wm.ball().pos().y * 0.5;

    agent->debugClient().addMessage( "GoalKickMove" );
    agent->debugClient().setTarget( target_point );

    if ( ! Body_GoToPoint( target_point,
                           dist_thr,
                           dash_power
                           ).execute( agent ) )
    {
        // already there
        Body_TurnToBall().execute( agent );
    }
}
