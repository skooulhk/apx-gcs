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
#include "Vehicle.h"
#include "Vehicles.h"
#include "VehicleMandala.h"
//=============================================================================
Vehicle::Vehicle(Vehicles *parent, QString callsign, quint16 squawk, QByteArray uid, VehicleClass vclass, bool bLocal)
  : Fact(bLocal?parent:parent->f_list,bLocal?callsign:"vehicle#",callsign,"",GroupItem,NoData)
{
  setSection(parent->title());

  f_streamType=new Fact(this,"stream",tr("Stream"),tr("Current data stream type"),FactItem,ConstData);
  f_streamType->setEnumStrings(QMetaEnum::fromType<StreamType>());

  f_squawk=new Fact(this,"squawk",tr("SQUAWK"),tr("Dynamic vehicle ID"),FactItem,ConstData);
  f_squawk->setValue(QString("%1").arg((ulong)squawk,4,16,QLatin1Char('0')).toUpper());
  f_squawk->setVisible(vclass!=LOCAL);

  f_callsign=new Fact(this,"callsign",tr("Callsign"),tr("Vehicle name"),FactItem,ConstData);
  f_callsign->setValue(callsign);
  f_callsign->setVisible(vclass!=LOCAL);

  f_vclass=new Fact(this,"vclass",tr("Class"),tr("Vehicle class"),FactItem,ConstData);
  f_vclass->setEnumStrings(QMetaEnum::fromType<VehicleClass>());
  f_vclass->setValue(vclass);

  f_uid=new Fact(this,"uid",tr("Unique ID"),"",FactItem,NoData);
  f_uid->setValue(qVariantFromValue(uid));
  f_uid->setStatus(uid.toHex().toUpper());
  f_uid->setVisible(vclass!=LOCAL);

  f_mandala=new VehicleMandala(this);

  if(vclass!=CURRENT){
    //selection action fact
    f_select=new Fact(parent->f_select,name(),title(),descr(),FactItem,NoData);
    connect(this,&Vehicle::destroyed,[=](){ parent->f_select->removeItem(f_select); });
    connect(f_select,&Fact::triggered,[=](){ parent->selectVehicle(this); });

    connect(this,&Vehicle::activeChanged,[=](){ f_select->setActive(active()); });
    connect(parent,&Vehicles::currentChanged,[=](Vehicle *v){ setActive(v==this); });

    connect(this,&Fact::statusChanged,[=](){ f_select->setStatus(status()); });
    f_streamType->setValue(0);

    onlineTimer.setSingleShot(true);
    onlineTimer.setInterval(7000);
    connect(&onlineTimer,&QTimer::timeout,[=](){ f_streamType->setValue(OFFLINE); });

    connect(f_streamType,&Fact::valueChanged,[=](){ setStatus(f_streamType->text()); });
    f_streamType->setValue(0);

    f_selectAction=new Fact(this,"select",tr("Select"),"Make this vehicle active",FactItem,NoData);
    connect(f_selectAction,&Fact::triggered,[=](){ parent->selectVehicle(this); });
    connect(parent,&Vehicles::currentChanged,[=](Vehicle *v){ f_selectAction->setEnabled(v!=this); });
  }

  qmlRegisterUncreatableType<Vehicle>("GCS.Vehicle", 1, 0, "Vehicle", "Reference only");
}
//=============================================================================
//=============================================================================
void Vehicle::downlinkReceived(const QByteArray &ba)
{
  if(f_mandala->unpackService(ba)){
    f_streamType->setValue(SERVICE);
  }else if(f_mandala->unpackTelemetry(ba)){
    f_streamType->setValue(TELEMETRY);
  }else if(f_mandala->unpackData(ba)){
    f_streamType->setValue(DATA);
  }else return;
  onlineTimer.start();
}
//=============================================================================
void Vehicle::xpdrReceived(const QByteArray &ba)
{
  if(f_mandala->unpackXPDR(ba)){
    f_streamType->setValue(XPDR);
  }else return;
  onlineTimer.start();
}
//=============================================================================
