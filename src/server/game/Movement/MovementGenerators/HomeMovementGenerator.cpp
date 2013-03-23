/*
 * Copyright (C) 2010-2011 Izb00shka <http://izbooshka.net/>
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "HomeMovementGenerator.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "Traveller.h"
#include "DestinationHolderImp.h"
#include "WorldPacket.h"
#include "World.h"

void
HomeMovementGenerator<Creature>::Initialize(Creature & owner)
{
    float x, y, z;
    owner.GetHomePosition(x, y, z, ori);
    owner.RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
    owner.AddUnitState(UNIT_STAT_EVADE);
    _setTargetLocation(owner);
}

void
HomeMovementGenerator<Creature>::Finalize(Creature & owner)
{
    owner.ClearUnitState(UNIT_STAT_EVADE);
}

void
HomeMovementGenerator<Creature>::Reset(Creature &)
{
}

void
HomeMovementGenerator<Creature>::_setTargetLocation(Creature & owner)
{
    if (!&owner)
        return;

    if (owner.HasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED | UNIT_STAT_DISTRACTED))
        return;

    float x, y, z;
    owner.GetHomePosition(x, y, z, ori);

    PathInfo path(&owner, x, y, z);
    i_path = path.getFullPath();

    CreatureTraveller traveller(owner);
    MoveToNextNode(traveller);

    float speed = traveller.Speed() * 0.001f; // in ms
    uint32 transitTime = uint32(i_path.GetTotalLength() / speed);

    owner.SendMonsterMoveByPath(i_path, 1, i_path.size(), transitTime);
    owner.ClearUnitState(UNIT_STAT_ALL_STATE & ~UNIT_STAT_EVADE);
}

void HomeMovementGenerator<Creature>::MoveToNextNode(Creature &owner)
{
    CreatureTraveller traveller(owner);
    PathNode &node = i_path[i_currentNode];
    i_destinationHolder.SetDestination(traveller, node.x, node.y, node.z, false);
}

bool
HomeMovementGenerator<Creature>::Update(Creature &owner, const uint32& time_diff)
{
    CreatureTraveller traveller(owner);

    i_destinationHolder.UpdateTraveller(traveller, time_diff, false);

    if (i_path.empty())
        return false;

    if (i_destinationHolder.HasArrived())
    {
        ++i_currentNode;

        // if we are at the last node, stop charge
        if (i_currentNode >= i_path.size())
        {
            owner.AddUnitMovementFlag(MOVEMENTFLAG_WALKING);

            // restore orientation of not moving creature at returning to home
            if (owner.GetDefaultMovementType() == IDLE_MOTION_TYPE)
            {
                owner.SetOrientation(ori);
                WorldPacket packet;
                owner.BuildHeartBeatMsg(&packet);
                owner.SendMessageToSet(&packet, false);
            }

            owner.ClearUnitState(UNIT_STAT_EVADE);
            owner.LoadCreaturesAddon(true);
            owner.AI()->JustReachedHome();

            return false;
        }

        MoveToNextNode(traveller);
    }

    return true;
}
