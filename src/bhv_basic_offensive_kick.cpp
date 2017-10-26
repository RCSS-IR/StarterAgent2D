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

#include "bhv_basic_offensive_kick.h"

#include <rcsc/action/body_hold_ball.h>
#include <rcsc/action/body_smart_kick.h>
#include <rcsc/action/body_stop_ball.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

#include <rcsc/geom/sector_2d.h>

#include <vector>
using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_BasicOffensiveKick::execute( PlayerAgent * agent )
{
    dlog.addText( Logger::TEAM,
                  __FILE__": Bhv_BasicOffensiveKick" );

    const WorldModel & wm = agent->world();

    if(shoot(agent)){
    	return true;
    }

    const PlayerPtrCont & opps = wm.opponentsFromSelf();
    const PlayerObject * nearest_opp
        = ( opps.empty()
            ? static_cast< PlayerObject * >( 0 )
            : opps.front() );
    const double nearest_opp_dist = ( nearest_opp
                                      ? nearest_opp->distFromSelf()
                                      : 1000.0 );
//    const Vector2D nearest_opp_pos = ( nearest_opp
//                                       ? nearest_opp->pos()
//                                       : Vector2D( -1000.0, 0.0 ) );

    if(nearest_opp_dist < 10){
    	if(pass(agent))
    		return true;
    }



    if(dribble(agent)){
    	return true;
    }

    if ( nearest_opp_dist > 2.5 )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": hold" );
        agent->debugClient().addMessage( "OffKickHold" );
        Body_HoldBall().execute( agent );
        return true;
    }
    clearball(agent);
    return true;
}

bool Bhv_BasicOffensiveKick::shoot( rcsc::PlayerAgent * agent ){
	const WorldModel & wm = agent->world();
	Vector2D ball_pos = wm.ball().pos();
	Vector2D center_goal = Vector2D(52.5,0);
	if(ball_pos.dist(center_goal) > 25)
            return false;
	Vector2D left_goal = Vector2D(52.5,6);
	Vector2D right_goal = Vector2D(52.5,-6);

	if(left_goal.dist(ball_pos) < right_goal.dist(ball_pos)){
        Body_SmartKick(left_goal,3,0.1,2).execute(agent);
	}else{
        Body_SmartKick(right_goal,3,0.1,2).execute(agent);
	}
	return true;
}

bool Bhv_BasicOffensiveKick::pass(PlayerAgent * agent){
	const WorldModel & wm = agent->world();
	std::vector<Vector2D> targets;
	Vector2D ball_pos = wm.ball().pos();
	for(int u = 1;u<=11;u++){
		const AbstractPlayerObject * tm = wm.ourPlayer(u);
		if(tm==NULL || tm->unum() < 1 || tm->unum() == wm.self().unum() )
			continue;
		Vector2D tm_pos = tm->pos();
		if(tm->pos().dist(ball_pos) > 30)
			continue;
		Sector2D pass = Sector2D(ball_pos,1,tm_pos.dist(ball_pos)+3,(tm_pos - ball_pos).th() - 15,(tm_pos - ball_pos).th() + 15);
		if(!wm.existOpponentIn(pass,5,true)){
			targets.push_back(tm_pos);
		}
	}
	if(targets.size() == 0)
		return false;
	Vector2D best_target = targets[0];
    for(unsigned int i=1;i<targets.size();i++){
		if(targets[i].x > best_target.x)
			best_target = targets[i];
	}
	if(wm.gameMode().type()!= GameMode::PlayOn)
        Body_SmartKick(best_target,3,2.5,1).execute(agent);
	else
        Body_SmartKick(best_target,3,2.5,2).execute(agent);
	return true;
}

bool Bhv_BasicOffensiveKick::dribble(PlayerAgent * agent){
	const WorldModel & wm = agent->world();
	Vector2D ball_pos = wm.ball().pos();
	double dribble_angle = (Vector2D(52.5,0) - ball_pos).th().degree();
	Sector2D dribble_sector = Sector2D(ball_pos,0,3,dribble_angle - 15,dribble_angle+15);
	if(!wm.existOpponentIn(dribble_sector,5,true)){
		Vector2D target = Vector2D::polar2vector(3,dribble_angle) + ball_pos;
		if(Body_SmartKick(target,0.8,0.7,2).execute(agent)){
			return true;
		}
	}
	return false;
}

bool Bhv_BasicOffensiveKick::clearball(PlayerAgent * agent){
    const WorldModel & wm = agent->world();
    if(!wm.self().isKickable())
        return false;
    Vector2D ball_pos = wm.ball().pos();
    Vector2D target = Vector2D(52.5,0);
    if(ball_pos.x < 0){
        if(ball_pos.x > -25){
            if(ball_pos.dist(Vector2D(0,-34)) < ball_pos.dist(Vector2D(0,+34))){
                target = Vector2D(0,-34);
            }else{
                target = Vector2D(0,+34);
            }
        }else{
            if(ball_pos.absY() < 10 && ball_pos.x < -10){
                if(ball_pos.y > 0){
                    target = Vector2D(-52,20);
                }else{
                    target = Vector2D(-52,-20);
                }
            }else{
                if(ball_pos.y > 0){
                    target = Vector2D(ball_pos.x,34);
                }else{
                    target = Vector2D(ball_pos.x,-34);
                }
            }
        }
    }
    if(Body_SmartKick(target,2.7,2.7,2).execute(agent)){
        return true;
    }
    Body_StopBall().execute(agent);
    return true;
}
