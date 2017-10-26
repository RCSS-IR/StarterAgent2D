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

#include "bhv_set_play.h"

#include "bhv_basic_move.h"

#include "bhv_basic_offensive_kick.h"
#include "bhv_goalie_free_kick.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_before_kick_off.h>
#include <rcsc/action/bhv_scan_field.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/circle_2d.h>
#include <rcsc/geom/line_2d.h>
#include <rcsc/geom/segment_2d.h>

#include <vector>
#include <algorithm>
#include <limits>
#include <cstdio>

// #define DEBUG_PRINT

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */


bool
Bhv_SetPlay::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_SetPlay" );

    const WorldModel & wm = agent->world();
    if ( wm.self().goalie() )
    {
        if ( wm.gameMode().type() != GameMode::BackPass_
             && wm.gameMode().type() != GameMode::IndFreeKick_ )
        {
            Bhv_GoalieFreeKick().execute( agent );
            return true;
        }

    }

    if ( wm.gameMode().side() == wm.ourSide() ){
        if (is_kicker(agent)){
            doKick(agent);
        }else{
            doMove(agent);
        }
    }else{
        doMove(agent);
    }
    return true;

}

void
Bhv_SetPlay::doMove( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    Vector2D target_point = Bhv_BasicMove().getPosition( wm, wm.self().unum() );
    if(target_point.x > wm.offsideLineX())
        target_point.x = wm.offsideLineX() - 1.0;

    if(wm.gameMode().side() != wm.ourSide())
        if(!can_go_to(-11,wm,Circle2D(wm.ball().pos(),11.0),target_point))
            target_point = get_avoid_circle_point(wm,target_point);

    double dash_power = get_set_play_dash_power( agent );

    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    agent->debugClient().addMessage( "KickInMove" );
    agent->debugClient().setTarget( target_point );

    if ( ! Body_GoToPoint( target_point,
                           dist_thr,
                           dash_power
                           ).execute( agent ) )
    {
        Body_TurnToBall().execute( agent );
    }

}
void
Bhv_SetPlay::doKick( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    //
    // go to the kick position
    //
    AngleDeg ball_place_angle = ( wm.ball().pos().y > 0.0
                                  ? -90.0
                                  : 90.0 );
    if ( GoToStaticBall( agent, ball_place_angle ) )
    {
        return;
    }

    //
    // wait
    //

    if ( doKickWait( agent ) )
    {
        return;
    }

    //
    // pass
    //

    if(Bhv_BasicOffensiveKick().pass(agent)){
        return;
    }
    //
    // kick to the nearest teammate
    //
    {
        const PlayerObject * receiver = wm.getTeammateNearestToBall( 10 );
        if ( receiver
             && receiver->distFromBall() < 10.0
             && receiver->pos().absX() < ServerParam::i().pitchHalfLength()
             && receiver->pos().absY() < ServerParam::i().pitchHalfWidth() )
        {
            Vector2D target_point = receiver->inertiaFinalPoint();
            target_point.x += 0.5;

            double ball_move_dist = wm.ball().pos().dist( target_point );
            int ball_reach_step
                    = ball_move_dist / 2.5;
            double ball_speed = 0.0;
            if ( ball_reach_step > 3 )
            {
                ball_speed = 2.5;
            }
            else
            {
                ball_speed = 1.5;
            }

            agent->debugClient().addMessage( "KickIn:ForcePass%.3f", ball_speed );
            agent->debugClient().setTarget( target_point );

            dlog.addText( Logger::TEAM,
                          __FILE__":  kick to nearest teammate (%.1f %.1f) speed=%.2f",
                          target_point.x, target_point.y,
                          ball_speed );
            Body_KickOneStep( target_point,
                              ball_speed
                              ).execute( agent );
            return;
        }
    }

    //
    // turn to ball
    //

    if ( ( wm.ball().angleFromSelf() - wm.self().body() ).abs() > 1.5 )
    {
        agent->debugClient().addMessage( "KickIn:Advance:TurnToBall" );
        dlog.addText( Logger::TEAM,
                      __FILE__":  clear. turn to ball" );

        Body_TurnToBall().execute( agent );
        return;
    }


    //
    // kick to the opponent side corner
    //
    {
        agent->debugClient().addMessage( "KickIn:ForceAdvance" );

        Vector2D target_point( ServerParam::i().pitchHalfLength() - 2.0,
                               ( ServerParam::i().pitchHalfWidth() - 5.0 )
                               * ( 1.0 - ( wm.self().pos().x
                                           / ServerParam::i().pitchHalfLength() ) ) );
        if ( wm.self().pos().y < 0.0 )
        {
            target_point.y *= -1.0;
        }
        // enforce one step kick
        dlog.addText( Logger::TEAM,
                      __FILE__": advance(2) to (%.1f, %.1f)",
                      target_point.x, target_point.y );
        Body_KickOneStep( target_point,
                          ServerParam::i().ballSpeedMax()
                          ).execute( agent );
    }
}

bool
Bhv_SetPlay::doKickWait( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    const int real_set_play_count
            = static_cast< int >( wm.time().cycle() - wm.lastSetPlayStartTime().cycle() );

    if ( real_set_play_count >= ServerParam::i().dropBallTime() - 5 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) real set play count = %d > drop_time-10, force kick mode",
                      real_set_play_count );
        return false;
    }

    if ( Bhv_SetPlay::is_delaying_tactics_situation( agent ) )
    {
        agent->debugClient().addMessage( "KickIn:Delaying" );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) delaying" );

        Body_TurnToPoint( Vector2D( 0.0, 0.0 ) ).execute( agent );
        return true;
    }

    if ( wm.teammatesFromBall().empty() )
    {
        agent->debugClient().addMessage( "KickIn:NoTeammate" );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) no teammate" );

        Body_TurnToPoint( Vector2D( 0.0, 0.0 ) ).execute( agent );
        return true;
    }

    if ( wm.setplayCount() <= 3 )
    {
        agent->debugClient().addMessage( "KickIn:Wait%d", wm.setplayCount() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) wait teammates" );

        Body_TurnToBall().execute( agent );
        return true;
    }

    if ( wm.setplayCount() >= 15
         && wm.seeTime() == wm.time()
         && wm.self().stamina() > 8000 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) set play count = %d, force kick mode",
                      wm.setplayCount() );
        return false;
    }

    if ( wm.seeTime() != wm.time()
         || wm.self().stamina() < 8000 )
    {
        Body_TurnToBall().execute( agent );

        agent->debugClient().addMessage( "KickIn:Wait%d", wm.setplayCount() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKickWait) no see or recover" );
        return true;
    }

    return false;
}
/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_SetPlay::get_set_play_dash_power( const PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( ! wm.gameMode().isOurSetPlay( wm.ourSide() ) )
    {
        Vector2D target_point = Bhv_BasicMove().getPosition( wm, wm.self().unum() );
        if ( target_point.x > wm.self().pos().x )
        {
            if ( wm.ball().pos().x < -30.0
                 && target_point.x < wm.ball().pos().x )
            {
                return wm.self().getSafetyDashPower( ServerParam::i().maxDashPower() );
            }

            double rate = 0.0;
            if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.8 )
            {
                rate = 1.5 * wm.self().stamina() / ServerParam::i().staminaMax();
            }
            else
            {
                rate = 0.9
                        * ( wm.self().stamina() - ServerParam::i().recoverDecThrValue() )
                        / ServerParam::i().staminaMax();
                rate = std::max( 0.0, rate );
            }

            return ( wm.self().playerType().staminaIncMax()
                     * wm.self().recovery()
                     * rate );
        }
    }

    return wm.self().getSafetyDashPower( ServerParam::i().maxDashPower() );

}

bool Bhv_SetPlay::can_go_to( const int count,
                             const WorldModel & wm,
                             const Circle2D & ball_circle,
                             const Vector2D & target_point )
{
    Segment2D move_line( wm.self().pos(), target_point );

    int n_intersection = ball_circle.intersection( move_line, NULL, NULL );

    dlog.addText( Logger::TEAM,
                  "%d: (can_go_to) check target=(%.2f %.2f) intersection=%d",
                  count, target_point.x, target_point.y,
                  n_intersection );
    dlog.addLine( Logger::TEAM,
                  wm.self().pos(), target_point,
                  "#0000ff" );
    char num[8];
    snprintf( num, 8, "%d", count );
    dlog.addMessage( Logger::TEAM,
                     target_point, num,
                     "#0000ff" );

    if ( n_intersection == 0 )
    {
        dlog.addText( Logger::TEAM,
                      "%d: (can_go_to) ok(1)", count );
        return true;
    }

    if ( n_intersection == 1 )
    {
        AngleDeg angle = ( target_point - wm.self().pos() ).th();

        dlog.addText( Logger::TEAM,
                      "%d: (can_go_to) intersection=1 angle_diff=%.1f",
                      count,
                      ( angle - wm.ball().angleFromSelf() ).abs() );
        if ( ( angle - wm.ball().angleFromSelf() ).abs() > 80.0 )
        {
            dlog.addText( Logger::TEAM,
                          "%d: (can_go_to) ok(2)", count );
            return true;
        }
    }

    return false;
}


Vector2D Bhv_SetPlay::get_avoid_circle_point( const WorldModel & wm,
                                     const Vector2D & target_point )
{
    const ServerParam & SP = ServerParam::i();

    const double avoid_radius
            = SP.centerCircleR()
            + wm.self().playerType().playerSize();
    const Circle2D ball_circle( wm.ball().pos(), avoid_radius );

#ifdef DEBUG_PRINT
    dlog.addText( Logger::TEAM,
                  __FILE__": (get_avoid_circle_point) first_target=(%.2f %.2f)",
                  target_point.x, target_point.y );
    dlog.addCircle( Logger::TEAM,
                    wm.ball().pos(), avoid_radius,
                    "#ffffff" );
#endif

    if ( can_go_to( -1, wm, ball_circle, target_point ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (get_avoid_circle_point) ok, first point" );
        return target_point;
    }

    AngleDeg target_angle = ( target_point - wm.self().pos() ).th();
    AngleDeg ball_target_angle = ( target_point - wm.ball().pos() ).th();
    bool ball_is_left = wm.ball().angleFromSelf().isLeftOf( target_angle );

    const int ANGLE_DIVS = 6;
    std::vector< Vector2D > subtargets;
    subtargets.reserve( ANGLE_DIVS );

    const int angle_step = ( ball_is_left ? 1 : -1 );

    int count = 0;
    int a = angle_step;

#ifdef DEBUG_PRINT
    dlog.addText( Logger::TEAM,
                  __FILE__": (get_avoid_circle_point) ball_target_angle=%.1f angle_step=%d",
                  ball_target_angle.degree(), angle_step );
#endif

    for ( int i = 1; i < ANGLE_DIVS; ++i, a += angle_step, ++count )
    {
        AngleDeg angle = ball_target_angle + (180.0/ANGLE_DIVS)*a;
        Vector2D new_target = wm.ball().pos()
                + Vector2D::from_polar( avoid_radius + 1.0, angle );

        dlog.addText( Logger::TEAM,
                      "%d: a=%d angle=%.1f (%.2f %.2f)",
                      count, a, angle.degree(),
                      new_target.x, new_target.y );

        if ( new_target.absX() > SP.pitchHalfLength() + SP.pitchMargin() - 1.0
             || new_target.absY() > SP.pitchHalfWidth() + SP.pitchMargin() - 1.0 )
        {
            dlog.addText( Logger::TEAM,
                          "%d: out of field",
                          count );
            break;
        }

        if ( can_go_to( count, wm, ball_circle, new_target ) )
        {
            return new_target;
        }
    }

    a = -angle_step;
    for ( int i = 1; i < ANGLE_DIVS*2; ++i, a -= angle_step, ++count )
    {
        AngleDeg angle = ball_target_angle + (180.0/ANGLE_DIVS)*a;
        Vector2D new_target = wm.ball().pos()
                + Vector2D::from_polar( avoid_radius + 1.0, angle );

#ifdef DEBUG_PRINT
        dlog.addText( Logger::TEAM,
                      "%d: a=%d angle=%.1f (%.2f %.2f)",
                      count, a, angle.degree(),
                      new_target.x, new_target.y );
#endif
        if ( new_target.absX() > SP.pitchHalfLength() + SP.pitchMargin() - 1.0
             || new_target.absY() > SP.pitchHalfWidth() + SP.pitchMargin() - 1.0 )
        {
#ifdef DEBUG_PRINT
            dlog.addText( Logger::TEAM,
                          "%d: out of field",
                          count );
#endif
            break;
        }

        if ( can_go_to( count, wm, ball_circle, new_target ) )
        {
            return new_target;
        }
    }

    return target_point;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlay::is_kicker( const PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( wm.gameMode().type() == GameMode::GoalieCatch_
         && wm.gameMode().side() == wm.ourSide()
         && ! wm.self().goalie() )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (is_kicker) goalie free kick" );
        return false;
    }

    if (wm.gameMode().type() == GameMode::GoalKick_){
        if(wm.self().goalie())
            return true;
        return false;
    }
    int kicker_unum = 0;
    double min_dist2 = std::numeric_limits< double >::max();
    for ( int unum = 1; unum <= 11; ++unum )
    {
        if ( unum == wm.ourGoalieUnum() ) continue;

        Vector2D home_pos = Bhv_BasicMove().getPosition( wm, unum );
        if ( ! home_pos.isValid() ) continue;

        double d2 = home_pos.dist2( wm.ball().pos() );
        if ( d2 < min_dist2 )
        {
            min_dist2 = d2;
            kicker_unum = unum;
        }
    }

    return ( kicker_unum == wm.self().unum() );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_SetPlay::is_delaying_tactics_situation( const PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

#if 1
    const int real_set_play_count = wm.time().cycle() - wm.lastSetPlayStartTime().cycle();
    const int wait_buf = ( wm.gameMode().type() == GameMode::GoalKick_
                           ? 15
                           : 2 );

    if ( real_set_play_count >= ServerParam::i().dropBallTime() - wait_buf )
    {
        return false;
    }
#endif

    int our_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreLeft()
                      : wm.gameMode().scoreRight() );
    int opp_score = ( wm.ourSide() == LEFT
                      ? wm.gameMode().scoreRight()
                      : wm.gameMode().scoreLeft() );

#if 1
    if ( wm.audioMemory().recoveryTime().cycle() >= wm.time().cycle() - 10 )
    {
        if ( our_score > opp_score )
        {
            return true;
        }
    }
#endif

    long cycle_thr = std::max( 0,
                               ServerParam::i().nrNormalHalfs()
                               * ( ServerParam::i().halfTime() * 10 )
                               - 500 );

    if ( wm.time().cycle() < cycle_thr )
    {
        return false;
    }

    if ( our_score > opp_score
         && our_score - opp_score <= 1 )
    {
        return true;
    }

    return false;
}

/*-------------------------------------------------------------------*/

bool
Bhv_SetPlay::GoToStaticBall( PlayerAgent * agent, const AngleDeg & ball_place_angle)
{
    const double dir_margin = 15.0;

    const WorldModel & wm = agent->world();

    AngleDeg angle_diff = wm.ball().angleFromSelf() - ball_place_angle;

    if ( angle_diff.abs() < dir_margin
         && wm.ball().distFromSelf() < ( wm.self().playerType().playerSize()
                                         + ServerParam::i().ballSize()
                                         + 0.08 )
         )
    {
        // already reach
        return false;
    }

    // decide sub-target point
    Vector2D sub_target = wm.ball().pos()
            + Vector2D::polar2vector( 2.0, ball_place_angle + 180.0 );

    double dash_power = 20.0;
    double dash_speed = -1.0;
    if ( wm.ball().distFromSelf() > 2.0 )
    {
        dash_power = Bhv_SetPlay::get_set_play_dash_power( agent );
    }
    else
    {
        dash_speed = wm.self().playerType().playerSize();
        dash_power = wm.self().playerType().getDashPowerToKeepSpeed( dash_speed, wm.self().effort() );
    }

    // it is necessary to go to sub target point
    if ( angle_diff.abs() > dir_margin )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": go to sub-target(%.1f, %.1f)",
                      sub_target.x, sub_target.y );
        Body_GoToPoint( sub_target,
                        0.1,
                        dash_power,
                        dash_speed ).execute( agent );
    }
    // dir diff is small. go to ball
    else
    {
        // body dir is not right
        if ( ( wm.ball().angleFromSelf() - wm.self().body() ).abs() > 1.5 )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": turn to ball" );
            Body_TurnToBall().execute( agent );
        }
        // dash to ball
        else
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": dash to ball" );
            agent->doDash( dash_power );
        }
    }

    return true;
}
