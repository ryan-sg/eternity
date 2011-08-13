// Emacs style mode select -*- C -*- vi:sw=3 ts=3:
//----------------------------------------------------------------------------
//
// Copyright (C) 2006-2010 by The Odamex Team.
// Copyright(C) 2011 Charles Gunyon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//   C/S CTF routines.
//
//----------------------------------------------------------------------------

// [CG] Notably, CTF was largely written by Toke for Odamex, and although that
//      implementation varies significantly from this one, it's still the same
//      fundamental design.  Thanks :) .

#include "c_io.h"
#include "doomtype.h"
#include "doomstat.h"
#include "d_player.h"
#include "e_things.h"
#include "e_sound.h"
#include "g_game.h"
#include "m_fixed.h"
#include "p_maputl.h"
#include "p_mobj.h"
#include "s_sound.h"

#include "cs_team.h"
#include "cs_ctf.h"
#include "cs_main.h"
#include "cs_netid.h"
#include "cl_main.h"
#include "sv_main.h"

flag_stand_t cs_flag_stands[team_color_max];
flag_t cs_flags[team_color_max];

static void start_flag_sound(flag_t *flag, const char *sound_name)
{
   mobj_t *actor = CS_GetActorFromNetID(flag->net_id);

   if(actor)
   {
      S_StartSfxInfo(
         actor, E_SoundForName(sound_name), 127, ATTN_NONE, false, CHAN_AUTO
      );
   }
}

static void respawn_flag(flag_t *flag, fixed_t x, fixed_t y, fixed_t z,
                         const char *type_name)
{
   mobj_t *actor;

   CS_RemoveFlagActor(flag);
   actor = P_SpawnMobj(x, y, z, E_SafeThingName(type_name));
   flag->net_id = actor->net_id;
}

void CS_RemoveFlagActor(flag_t *flag)
{
   mobj_t *actor;

   if(flag->net_id)
   {
      actor = CS_GetActorFromNetID(flag->net_id);
      if(actor != NULL)
      {
         P_RemoveMobj(actor);
         actor = NULL;
      }
      flag->net_id = 0;
   }
}

void CS_ReturnFlag(flag_t *flag)
{
   teamcolor_t color = flag - cs_flags;
   flag_stand_t *flag_stand = &cs_flag_stands[color];

   if(color == team_color_red)
   {
      respawn_flag(
         flag, flag_stand->x, flag_stand->y, flag_stand->z, "RedFlag"
      );
   }
   else if(color == team_color_blue)
   {
      respawn_flag(
         flag, flag_stand->x, flag_stand->y, flag_stand->z, "BlueFlag"
      );
   }

   flag->carrier = 0;
   flag->pickup_time = 0;
   flag->timeout = 0;
   flag->state = flag_home;
}

void CS_GiveFlag(int playernum, flag_t *flag)
{
   teamcolor_t color = flag - cs_flags;
   player_t *player = &players[playernum];
   position_t position;

   CS_SaveActorPosition(&position, player->mo, 0);

   if(color == team_color_red)
   {
      respawn_flag(
         flag, position.x, position.y, position.z, "CarriedRedFlag"
      );
   }
   else if(color == team_color_blue)
   {
      respawn_flag(
         flag, position.x, position.y, position.z, "CarriedBlueFlag"
      );
   }

   flag->carrier = playernum;
   if(CS_CLIENT)
   {
      flag->pickup_time = cl_current_world_index;
   }
   else if(CS_SERVER)
   {
      flag->pickup_time = sv_world_index;
   }
   flag->timeout = 0;
   flag->state = flag_carried;
}

void CS_HandleFlagTouch(player_t *player, teamcolor_t color)
{
   int playernum = player - players;
   client_t *client = &clients[playernum];
   flag_t *flag = &cs_flags[color];
   teamcolor_t other_color;

   if(color == team_color_red)
   {
      other_color = team_color_blue;
   }
   else
   {
      other_color = team_color_red;
   }

   if(flag->state == flag_dropped)
   {
      if(client->team == color)
      {
         CS_ReturnFlag(flag);
         doom_printf(
            "%s returned the %s flag", player->name, team_color_names[color]
         );
         if(s_use_announcer)
         {
            if(color == team_color_red)
               start_flag_sound(flag, "RedFlagReturned");
            else if(color == team_color_blue)
               start_flag_sound(flag, "BlueFlagReturned");
            else
               start_flag_sound(flag, "FlagReturned");
         }
         else
            start_flag_sound(flag, "FlagReturned");
      }
      else
      {
         CS_GiveFlag(playernum, flag);
         doom_printf(
            "%s picked up the %s flag", player->name, team_color_names[color]
         );
         if(s_use_announcer)
         {
            if(color == team_color_red)
               start_flag_sound(flag, "RedFlagTaken");
            else if(color == team_color_blue)
               start_flag_sound(flag, "BlueFlagTaken");
            else
               start_flag_sound(flag, "FlagTaken");
         }
         else
            start_flag_sound(flag, "FlagTaken");
      }
   }
   else
   {
      if(client->team == color)
      {
         if(cs_flags[other_color].carrier == playernum)
         {
            CS_ReturnFlag(&cs_flags[other_color]);
            team_scores[color]++;
            doom_printf(
               "%s captured the %s flag",
               player->name,
               team_color_names[other_color]
            );
            if(s_use_announcer)
            {
               if(client->team == team_color_red)
                  start_flag_sound(flag, "RedTeamScores");
               else if(client->team == team_color_blue)
                  start_flag_sound(flag, "BlueTeamScores");
               else
                  start_flag_sound(flag, "FlagCaptured");
            }
            else
               start_flag_sound(flag, "FlagCaptured");
         }
      }
      else
      {
         CS_GiveFlag(playernum, flag);
         doom_printf(
            "%s has taken the %s flag", player->name, team_color_names[color]
         );
         if(s_use_announcer)
         {
            if(color == team_color_red)
               start_flag_sound(flag, "RedFlagTaken");
            else if(color == team_color_blue)
               start_flag_sound(flag, "BlueFlagTaken");
            else
               start_flag_sound(flag, "FlagTaken");
         }
         else
            start_flag_sound(flag, "FlagTaken");
      }
   }
}

flag_t* CS_GetFlagCarriedByPlayer(int playernum)
{
   flag_t *flag;
   teamcolor_t other_color;
   client_t *client = &clients[playernum];

   if(client->team == team_color_red)
   {
      other_color = team_color_blue;
   }
   else
   {
      other_color = team_color_red;
   }

   flag = &cs_flags[other_color];

   if(flag->carrier == playernum)
   {
      return flag;
   }
   return NULL;
}

void CS_DropFlag(int playernum)
{
   flag_t *flag = CS_GetFlagCarriedByPlayer(playernum);
   teamcolor_t color = flag - cs_flags;
   mobj_t *corpse = players[playernum].mo;

   if(!flag || flag->carrier != playernum)
   {
      return;
   }

   doom_printf(
      "%s dropped the %s flag",
      players[playernum].name,
      team_color_names[color]
   );
   if(s_use_announcer)
   {
      if(color == team_color_red)
         start_flag_sound(flag, "RedFlagDropped");
      else if(color == team_color_blue)
         start_flag_sound(flag, "BlueFlagDropped");
      else
         start_flag_sound(flag, "FlagDropped");
   }
   else
      start_flag_sound(flag, "FlagDropped");

   if(color == team_color_red)
   {
      respawn_flag(flag, corpse->x, corpse->y, corpse->z, "RedFlag");
   }
   else if(color == team_color_blue)
   {
      respawn_flag(flag, corpse->x, corpse->y, corpse->z, "BlueFlag");
   }

   flag->carrier = 0;
   flag->pickup_time = 0;
   flag->timeout = 0;
   flag->state = flag_dropped;
}

void CS_CTFTicker(void)
{
   teamcolor_t color;
   flag_t *flag;
   position_t position;
   mobj_t *actor;

   for(color = team_color_none; color < team_color_max; color++)
   {
      flag = &cs_flags[color];

      if(flag->state == flag_dropped)
      {
         if(++flag->timeout > (10 * TICRATE))
         {
            CS_ReturnFlag(flag);
            doom_printf("%s flag returned", team_color_names[color]);
            if(s_use_announcer)
            {
               if(color == team_color_red)
                  start_flag_sound(flag, "RedFlagReturned");
               else if(color == team_color_blue)
                  start_flag_sound(flag, "BlueFlagReturned");
               else
                  start_flag_sound(flag, "FlagReturned");
            }
            else
               start_flag_sound(flag, "FlagReturned");
         }
      }
      else if(flag->state == flag_carried)
      {
         actor = CS_GetActorFromNetID(flag->net_id);
         if(actor == NULL)
         {
            I_Error(
               "CS_CTFTicker: No actor for %s flag, exiting.\n",
               team_color_names[color]
            );
         }
         CS_SaveActorPosition(&position, players[flag->carrier].mo, 0);
         P_UnsetThingPosition(actor);
         actor->x = position.x;
         actor->y = position.y;
         actor->z = position.z;
         P_SetThingPosition(actor);
      }
   }
}

