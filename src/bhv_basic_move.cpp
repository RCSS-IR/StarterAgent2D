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

#include "bhv_basic_move.h"

#include "bhv_basic_tackle.h"

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/body_intercept.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include <vector>
#include <cstdio>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicMove::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove" );

    //-----------------------------------------------
    // tackle
    if ( Bhv_BasicTackle( 0.8, 80.0 ).execute( agent ) )
    {
        return true;
    }

    const WorldModel & wm = agent->world();
    /*--------------------------------------------------------*/
    // chase ball
    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( ! wm.existKickableTeammate()
         && ( self_min <= 3
              || ( self_min <= mate_min
                   && self_min < opp_min + 3 )
              )
         )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": intercept" );
        Body_Intercept().execute( agent );

        return true;
    }

    const Vector2D target_point = getPosition( wm, wm.self().unum() );
    const double dash_power = get_normal_dash_power( wm );

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicMove target=(%.1f %.1f) dist_thr=%.2f",
                  target_point.x, target_point.y,
                  dist_thr );

    agent->debugClient().addMessage( "BasicMove%.0f", dash_power );
    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );

    if ( ! Body_GoToPoint( target_point, dist_thr, dash_power
                           ).execute( agent ) )
    {
        Body_TurnToBall().execute( agent );
    }

    return true;
}

rcsc::Vector2D Bhv_BasicMove::getPosition(const rcsc::WorldModel & wm, int self_unum){
    int ball_step = 0;
    if ( wm.gameMode().type() == GameMode::PlayOn
         || wm.gameMode().type() == GameMode::GoalKick_ )
    {
        ball_step = std::min( 1000, wm.interceptTable()->teammateReachCycle() );
        ball_step = std::min( ball_step, wm.interceptTable()->opponentReachCycle() );
        ball_step = std::min( ball_step, wm.interceptTable()->selfReachCycle() );
    }

    Vector2D ball_pos = wm.ball().inertiaPoint( ball_step );

    dlog.addText( Logger::TEAM,
                  __FILE__": HOME POSITION: ball pos=(%.1f %.1f) step=%d",
                  ball_pos.x, ball_pos.y,
                  ball_step );

    std::vector<Vector2D> positions(12);
    double min_x_rectengle[12]={0,-52,-52,-52,-52,-52,-30,-30,-30,0,0,0};
    double max_x_rectengle[12]={0,-48,-10,-10,-10,-10,15,15,15,50,50,50};
    double min_y_rectengle[12]={0,-2,-20,-10,-30,10,-20,-30, 0,-20,-30, 0};
    double max_y_rectengle[12]={0,+2, 10, 20,-10,30, 20, 0,30, 20, 0, 30};

    for(int i=1; i<=11; i++){
          double xx_rectengle = max_x_rectengle[i] - min_x_rectengle[i];
          double yy_rectengle = max_y_rectengle[i] - min_y_rectengle[i];
          double x_ball = ball_pos.x + 52.5;
          x_ball /= 105.5;
          double y_ball = ball_pos.y + 34;
          y_ball /= 68.0;
          double x_pos = xx_rectengle * x_ball + min_x_rectengle[i];
          double y_pos = yy_rectengle * y_ball + min_y_rectengle[i];
          positions[i] = Vector2D(x_pos,y_pos);
    }

    if ( ServerParam::i().useOffside() )
    {
        double max_x = wm.offsideLineX();
        if ( ServerParam::i().kickoffOffside()
             && ( wm.gameMode().type() == GameMode::BeforeKickOff
                  || wm.gameMode().type() == GameMode::AfterGoal_ ) )
        {
            max_x = 0.0;
        }
        else
        {
            int mate_step = wm.interceptTable()->teammateReachCycle();
            if ( mate_step < 50 )
            {
                Vector2D trap_pos = wm.ball().inertiaPoint( mate_step );
                if ( trap_pos.x > max_x ) max_x = trap_pos.x;
            }

            max_x -= 1.0;
        }

        for ( int unum = 1; unum <= 11; ++unum )
        {
            if ( positions[unum].x > max_x )
            {
                dlog.addText( Logger::TEAM,
                              "____ %d offside. home_pos_x %.2f -> %.2f",
                              unum,
                              positions[unum].x, max_x );
                positions[unum].x = max_x;
            }
        }
    }
    return positions.at(self_unum);
}

double Bhv_BasicMove::get_normal_dash_power( const WorldModel & wm )
{
    static bool s_recover_mode = false;

    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        return std::min( ServerParam::i().maxDashPower(),
                         wm.self().stamina() + wm.self().playerType().extraStamina() );
    }

    // check recover
    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        s_recover_mode = false;
    }
    else if ( wm.self().stamina() < ServerParam::i().staminaMax() * 0.5 )
    {
        s_recover_mode = true;
    }
    else if ( wm.self().stamina() > ServerParam::i().staminaMax() * 0.7 )
    {
        s_recover_mode = false;
    }

    /*--------------------------------------------------------*/
    double dash_power = ServerParam::i().maxDashPower();
    const double my_inc
        = wm.self().playerType().staminaIncMax()
        * wm.self().recovery();

    if ( wm.ourDefenseLineX() > wm.self().pos().x
         && wm.ball().pos().x < wm.ourDefenseLineX() + 20.0 )
    {
    }
    else if ( s_recover_mode )
    {
    }
    else if ( wm.existKickableTeammate()
              && wm.ball().distFromSelf() < 20.0 )
    {
    }
    else if ( wm.self().pos().x > wm.offsideLineX() )
    {
    }
    else
    {
        dash_power = std::min( my_inc * 1.7,
                               ServerParam::i().maxDashPower() );
    }

    return dash_power;
}

