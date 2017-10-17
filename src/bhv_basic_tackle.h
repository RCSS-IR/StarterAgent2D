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

#ifndef BHV_BASIC_TACKLE_H
#define BHV_BASIC_TACKLE_H



#include <rcsc/geom/vector_2d.h>
#include <rcsc/game_time.h>
#include <rcsc/geom/voronoi_diagram.h>
#include <rcsc/player/player_object.h>
#include <cmath>
#include <vector>

namespace rcsc {
class AbstractPlayerObject;
class WorldModel;
}

/*!
  \class TackleGenerator
  \brief tackle/foul generator
 */
class TackleGenerator {
public:

    struct TackleResult {
        rcsc::AngleDeg tackle_angle_; //!< global angle
        rcsc::Vector2D ball_vel_; //!< result ball velocity
        double ball_speed_;
        rcsc::AngleDeg ball_move_angle_;
        double score_;

        TackleResult();
        TackleResult( const rcsc::AngleDeg & angle,
                      const rcsc::Vector2D & vel );
        void clear();
    };

    typedef std::vector< TackleResult > Container;


private:

    //! candidate container
    Container M_candidates;

    //! best tackle result
    TackleResult M_best_result;

    // private for singleton
    TackleGenerator();

    // not used
    TackleGenerator( const TackleGenerator & );
    const TackleGenerator & operator=( const TackleGenerator & );
public:

    static
    TackleGenerator & instance();

    void generate( const rcsc::WorldModel & wm );


    const Container & candidates( const rcsc::WorldModel & wm )
      {
          generate( wm );
          return M_candidates;
      }

    const TackleResult & bestResult( const rcsc::WorldModel & wm )
      {
          generate( wm );
          return M_best_result;
      }

private:

    void clear();

    void calculate( const rcsc::WorldModel & wm );
    double evaluate( const rcsc::WorldModel & wm,
                     const TackleResult & result );

    int predictOpponentsReachStep( const rcsc::WorldModel & wm,
                                   const rcsc::Vector2D & first_ball_pos,
                                   const rcsc::Vector2D & first_ball_vel,
                                   const rcsc::AngleDeg & ball_move_angle );
    int predictOpponentReachStep( const rcsc::AbstractPlayerObject * opponent,
                                  const rcsc::Vector2D & first_ball_pos,
                                  const rcsc::Vector2D & first_ball_vel,
                                  const rcsc::AngleDeg & ball_move_angle,
                                  const int max_cycle );
    static
    int predict_player_turn_cycle( const rcsc::PlayerType * player_type,
                                   const rcsc::AngleDeg & player_body,
                                   const double & player_speed,
                                   const double & target_dist,
                                   const rcsc::AngleDeg & target_angle,
                                   const double & dist_thr,
                                   const bool use_back_dash );

};
#include <rcsc/player/soccer_action.h>

class Bhv_BasicTackle
    : public rcsc::SoccerBehavior {
private:
    const double M_min_probability;
    const double M_body_thr;
public:
    Bhv_BasicTackle( const double & min_prob,
                     const double & body_thr )
        : M_min_probability( min_prob )
        , M_body_thr( body_thr )
      { }

    bool execute( rcsc::PlayerAgent * agent );

private:

    bool executeV14( rcsc::PlayerAgent * agent,
                     const bool use_foul );
};

#endif
