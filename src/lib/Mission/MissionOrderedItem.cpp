﻿/*
 * Copyright (C) 2011 Aliaksei Stratsilatau <sa@uavos.com>
 *
 * This file is part of the UAV Open System Project
 *  http://www.uavos.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "MissionOrderedItem.h"
#include "MissionItems.h"
//=============================================================================
MissionOrderedItem::MissionOrderedItem(MissionItems *parent, const QString &name, const QString &title, const QString &descr)
  : Fact(parent,name,title,descr,GroupItem,NoData),
    missionItems(parent),
    namePrefix(name)
{
  connect(parent,&Fact::sizeChanged,this,&Fact::nameChanged);
  connect(this,&Fact::nameChanged,this,&MissionOrderedItem::updateTitle);
  updateTitle();
}
//=============================================================================
void MissionOrderedItem::updateTitle()
{
  setTitle(QString::number(num()+1));
}
//=============================================================================
//=============================================================================
