## Process this file with automake to produce Makefile.in

PLAYERSOURCES = \
	bhv_basic_move.cpp \
	bhv_basic_offensive_kick.cpp \
	bhv_basic_tackle.cpp \
	bhv_custom_before_kick_off.cpp \
	bhv_goalie_basic_move.cpp \
	bhv_goalie_free_kick.cpp \
	bhv_set_play.cpp \
	bhv_penalty_kick.cpp \
	intention_receive.cpp \
	sample_communication.cpp \
	sample_player.cpp \
	main_player.cpp

PLAYERHEADERS = \
	bhv_basic_move.h \
	bhv_basic_offensive_kick.h \
	bhv_basic_tackle.h \
	bhv_custom_before_kick_off.h \
	bhv_goalie_basic_move.h \
	bhv_goalie_free_kick.h \
	bhv_set_play.h \
	bhv_penalty_kick.h \
	intention_receive.h \
	communication.h \
	sample_communication.h \
	sample_player.h


COACHSOURCES = \
	sample_coach.cpp \
	main_coach.cpp

COACHHEADERS = \
	sample_coach.h

TRAINERSOURCES = \
	sample_trainer.cpp \
	main_trainer.cpp

TRAINERHEADERS = \
	sample_trainer.h


noinst_PROGRAMS = sample_player sample_coach sample_trainer

noinst_DATA = \
	start.sh.in \
	keepaway.sh.in \
	train.sh.in \
	coach.conf \
	player.conf \
	team_logo.xpm 

sample_player_SOURCES = \
	$(PLAYERSOURCES)
sample_player_CXXFLAGS = -W -Wall
sample_player_LDFLAGS =
sample_player_LDADD =

sample_coach_SOURCES = \
	$(COACHSOURCES)
sample_coach_CXXFLAGS = -W -Wall
sample_coach_LDFLAGS =
sample_coach_LDADD =

sample_trainer_SOURCES = \
	$(TRAINERSOURCES)
sample_trainer_CXXFLAGS = -W -Wall
sample_trainer_LDFLAGS =
sample_trainer_LDADD =

noinst_HEADERS = \
	$(PLAYERHEADERS) \
	$(COACHHEADERS) \
	$(TRAINERHEADERS)

AM_CPPFLAGS = -I$(top_srcdir)/src 
AM_CFLAGS = -W -Wall
AM_CXXFLAGS = -W -Wall
AM_LDLAGS =

CLEANFILES = core *~

DISTCLEANFILES = start.sh train.sh

EXTRA_DIST = $(noinst_DATA) \
	start-debug.sh \
	start-offline.sh
