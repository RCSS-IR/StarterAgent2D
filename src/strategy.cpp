// -*-c++-*-

/*!
  \file strategy.cpp
  \brief team strategh Source File
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "strategy.h"

//#ifndef USE_GENERIC_FACTORY


#include <rcsc/formation/formation_static.h>

#include <rcsc/player/intercept_table.h>
#include <rcsc/player/world_model.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/param/cmd_line_parser.h>
#include <rcsc/param/param_map.h>
#include <rcsc/game_mode.h>

#include <set>
#include <fstream>
#include <iostream>
#include <cstdio>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
Strategy::Strategy()
    : M_goalie_unum( Unum_Unknown ),
      M_current_situation( Normal_Situation ),
      M_positions( 11 )
{
#ifndef USE_GENERIC_FACTORY

    //
    // formations
    //

    M_formation_factory[FormationStatic::name()] = &FormationStatic::create;
#endif

}

/*-------------------------------------------------------------------*/
/*!

 */
Strategy &
Strategy::instance()
{
    static Strategy s_instance;
    return s_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
Strategy::init( CmdLineParser & cmd_parser )
{
    ParamMap param_map( "HELIOS_base options" );

    // std::string fconf;
    //param_map.add()
    //    ( "fconf", "", &fconf, "another formation file." );

    //
    //
    //

    if ( cmd_parser.count( "help" ) > 0 )
    {
        param_map.printHelp( std::cout );
        return false;
    }

    //
    //
    //

    cmd_parser.parse( param_map );

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */


/*-------------------------------------------------------------------*/
/*!

 */

/*-------------------------------------------------------------------*/
/*!

 */

/*-------------------------------------------------------------------*/
/*!

 */
void
Strategy::update( const WorldModel & wm )
{
    static GameTime s_update_time( -1, 0 );

    if ( s_update_time == wm.time() )
    {
        return;
    }
    s_update_time = wm.time();

    updateSituation( wm );
    updatePosition( wm );
}


/*-------------------------------------------------------------------*/
/*!

 */
void
Strategy::updateSituation( const WorldModel & wm )
{
    M_current_situation = Normal_Situation;

    if ( wm.gameMode().type() != GameMode::PlayOn )
    {
        if ( wm.gameMode().isPenaltyKickMode() )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": Situation PenaltyKick" );
            M_current_situation = PenaltyKick_Situation;
        }
        else if ( wm.gameMode().isPenaltyKickMode() )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": Situation OurSetPlay" );
            M_current_situation = OurSetPlay_Situation;
        }
        else
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": Situation OppSetPlay" );
            M_current_situation = OppSetPlay_Situation;
        }
        return;
    }

    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();
    int our_min = std::min( self_min, mate_min );

    if ( opp_min <= our_min - 2 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": Situation Defense" );
        M_current_situation = Defense_Situation;
        return;
    }

    if ( our_min <= opp_min - 2 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": Situation Offense" );
        M_current_situation = Offense_Situation;
        return;
    }

    dlog.addText( Logger::TEAM,
                  __FILE__": Situation Normal" );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
Strategy::updatePosition( const WorldModel & wm )
{
    static GameTime s_update_time( 0, 0 );
    if ( s_update_time == wm.time() )
    {
        return;
    }
    s_update_time = wm.time();


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

    M_positions.clear();
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
    	M_positions.push_back(Vector2D(x_pos,y_pos));
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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
            if ( M_positions[unum-1].x > max_x )
            {
                dlog.addText( Logger::TEAM,
                              "____ %d offside. home_pos_x %.2f -> %.2f",
                              unum,
                              M_positions[unum-1].x, max_x );
                M_positions[unum-1].x = max_x;
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/*!

 */
Vector2D
Strategy::getPosition( const int unum ) const
{
    const int number = unum;

    if ( number < 1 || 11 < number )
    {
        std::cerr << __FILE__ << ' ' << __LINE__
                  << ": Illegal number : " << number
                  << std::endl;
        return Vector2D::INVALIDATED;
    }

    try
    {
        return M_positions.at( number - 1 );
    }
    catch ( std::exception & e )
    {
        std::cerr<< __FILE__ << ':' << __LINE__ << ':'
                 << " Exception caught! " << e.what()
                 << std::endl;
        return Vector2D::INVALIDATED;
    }
}


/*-------------------------------------------------------------------*/
/*!

 */
double
Strategy::get_normal_dash_power( const WorldModel & wm )
{
    static bool s_recover_mode = false;

    if ( wm.self().staminaModel().capacityIsEmpty() )
    {
        return std::min( ServerParam::i().maxDashPower(),
                         wm.self().stamina() + wm.self().playerType().extraStamina() );
    }

    const int self_min = wm.interceptTable()->selfReachCycle();
    const int mate_min = wm.interceptTable()->teammateReachCycle();
    const int opp_min = wm.interceptTable()->opponentReachCycle();

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
        dlog.addText( Logger::TEAM,
                      __FILE__": (get_normal_dash_power) correct DF line. keep max power" );
        // keep max power
        dash_power = ServerParam::i().maxDashPower();
    }
    else if ( s_recover_mode )
    {
        dash_power = my_inc - 25.0; // preffered recover value
        if ( dash_power < 0.0 ) dash_power = 0.0;

        dlog.addText( Logger::TEAM,
                      __FILE__": (get_normal_dash_power) recovering" );
    }
    // exist kickable teammate
    else if ( wm.existKickableTeammate()
              && wm.ball().distFromSelf() < 20.0 )
    {
        dash_power = std::min( my_inc * 1.1,
                               ServerParam::i().maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (get_normal_dash_power) exist kickable teammate. dash_power=%.1f",
                      dash_power );
    }
    // in offside area
    else if ( wm.self().pos().x > wm.offsideLineX() )
    {
        dash_power = ServerParam::i().maxDashPower();
        dlog.addText( Logger::TEAM,
                      __FILE__": in offside area. dash_power=%.1f",
                      dash_power );
    }
    else if ( wm.ball().pos().x > 25.0
              && wm.ball().pos().x > wm.self().pos().x + 10.0
              && self_min < opp_min - 6
              && mate_min < opp_min - 6 )
    {
        dash_power = bound( ServerParam::i().maxDashPower() * 0.1,
                            my_inc * 0.5,
                            ServerParam::i().maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (get_normal_dash_power) opponent ball dash_power=%.1f",
                      dash_power );
    }
    // normal
    else
    {
        dash_power = std::min( my_inc * 1.7,
                               ServerParam::i().maxDashPower() );
        dlog.addText( Logger::TEAM,
                      __FILE__": (get_normal_dash_power) normal mode dash_power=%.1f",
                      dash_power );
    }

    return dash_power;
}
