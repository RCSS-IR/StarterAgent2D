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

#include "bhv_their_goal_kick_move.h"

#include "bhv_basic_move.h"

#include "bhv_set_play.h"

#include <rcsc/action/body_intercept.h>

#include <rcsc/action/basic_actions.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>

#include <rcsc/player/player_agent.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/debug_client.h>

#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>
#include <rcsc/geom/rect_2d.h>
#include <rcsc/geom/ray_2d.h>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_TheirGoalKickMove::execute( PlayerAgent * agent )
{
    static const Rect2D
        expand_their_penalty
        ( Vector2D( ServerParam::i().theirPenaltyAreaLineX() - 0.75,
                    -ServerParam::i().penaltyAreaHalfWidth() - 0.75 ),
          Size2D( ServerParam::i().penaltyAreaLength() + 0.75,
                  ServerParam::i().penaltyAreaWidth() + 1.5 ) );

    const WorldModel & wm = agent->world();
    Vector2D target_point = Bhv_BasicMove().getPosition( wm, wm.self().unum() );
    if(expand_their_penalty.contains(target_point))
        target_point.x = ServerParam::i().theirPenaltyAreaLineX() - 0.75;




    double dash_power
        = wm.self().getSafetyDashPower( ServerParam::i().maxDashPower() * 0.8 );

    double dist_thr = wm.ball().distFromSelf() * 0.07;
    if ( dist_thr < 1.0 ) dist_thr = 1.0;

    if ( ! Body_GoToPoint( target_point,
                           dist_thr,
                           dash_power
                           ).execute( agent ) )
    {
        // already there
        Body_TurnToBall().execute( agent );
    }
    agent->setNeckAction( new Neck_ScanField() );
    return true;
}

