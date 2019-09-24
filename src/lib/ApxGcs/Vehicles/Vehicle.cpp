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
#include "VehicleMandala.h"
#include "VehicleWarnings.h"
#include "Vehicles.h"

#include <ApxApp.h>
#include <ApxLog.h>
#include <Mission/VehicleMission.h>
#include <Nodes/Nodes.h>
#include <Telemetry/Telemetry.h>

namespace mandala {
#include <Mandala/MandalaConstants.h>
#include <Mandala/MandalaIndexes.h>
}; // namespace mandala
//=============================================================================
Vehicle::Vehicle(Vehicles *vehicles,
                 QString callsign,
                 quint16 squawk,
                 QString uid,
                 VehicleClass vclass,
                 ProtocolVehicle *protocol)
    : Fact(vclass >= LOCAL ? vehicles : vehicles->f_list, callsign.toLower(), callsign, "", Group)
    , uid(uid)
    , dbKey(0)
    , protocol(protocol)
    , m_streamType(OFFLINE)
    , m_squawk(squawk)
    , m_callsign(callsign)
    , m_vehicleClass(vclass)
    , m_follow(false)
    , m_flightState(FS_UNKNOWN)
{
    setSection(vehicles->title());
    setIcon(vclass == LOCAL ? "chip" : vclass == REPLAY ? "play-circle" : "drone");

    connect(this, &Vehicle::callsignChanged, this, &Vehicle::updateTitle);
    connect(this, &Vehicle::streamTypeChanged, this, &Vehicle::updateStatus);

    f_select
        = new Fact(this, "select", tr("Select"), tr("Make this vehicle active"), Action, "select");
    connect(f_select, &Fact::triggered, this, [=]() { vehicles->selectVehicle(this); });
    connect(vehicles, &Vehicles::vehicleSelected, this, [=](Vehicle *v) {
        f_select->setEnabled(v != this);
    });

    f_mandala = new VehicleMandala(this);
    f_nodes = new Nodes(this);
    f_mission = new VehicleMission(this);
    f_warnings = new VehicleWarnings(this);
    f_telemetry = new Telemetry(this);

    //f_recorder=new Recorder(this);

    //Mandala facts binfing
    f_gps_lat = f_mandala->factByName("gps_lat");
    f_gps_lon = f_mandala->factByName("gps_lon");
    f_gps_hmsl = f_mandala->factByName("gps_hmsl");
    f_home_lat = f_mandala->factByName("home_lat");
    f_home_lon = f_mandala->factByName("home_lon");
    f_home_hmsl = f_mandala->factByName("home_hmsl");
    f_gps_Vdown = f_mandala->factByName("gps_Vdown");
    f_mode = f_mandala->factByName("mode");
    f_stage = f_mandala->factByName("stage");

    updateInfoTimer.setInterval(300);
    updateInfoTimer.setSingleShot(true);
    connect(&updateInfoTimer, &QTimer::timeout, this, &Vehicle::updateInfo);

    connect(f_gps_lat, &VehicleMandalaFact::valueChanged, this, &Vehicle::updateCoordinate);
    connect(f_gps_lon, &VehicleMandalaFact::valueChanged, this, &Vehicle::updateCoordinate);
    connect(f_gps_hmsl, &VehicleMandalaFact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_gps_Vdown, &VehicleMandalaFact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_mode, &VehicleMandalaFact::valueChanged, this, &Vehicle::updateInfoReq);
    connect(f_stage, &VehicleMandalaFact::valueChanged, this, &Vehicle::updateInfoReq);

    connect(f_mode, &VehicleMandalaFact::valueChanged, this, &Vehicle::updateFlightState);
    connect(f_stage, &VehicleMandalaFact::valueChanged, this, &Vehicle::updateFlightState);

    if (!isTemporary()) {
        connect(vehicles, &Vehicles::vehicleSelected, this, [=](Vehicle *v) {
            setActive(v == this);
            if (active() && (!(isLocal() || isReplay())))
                dlinkReqTimer.start();
            else
                dlinkReqTimer.stop();
        });

        connect(this, &Fact::activeChanged, this, [=]() { setFollow(false); });

        onlineTimer.setSingleShot(true);
        onlineTimer.setInterval(7000);
        connect(&onlineTimer, &QTimer::timeout, this, [=]() { setStreamType(OFFLINE); });

        //downlink request timer
        if (!(isLocal() || isReplay())) {
            dlinkReqTimer.setInterval(1000);
            dlinkReqTimer.start();
            connect(&dlinkReqTimer, &QTimer::timeout, this, [=]() {
                if (active())
                    protocol->sendUplink(QByteArray()); //request telemetry
            });
        }
    }

    if (protocol && !isTemporary() && !isReplay()) {
        connect(this,
                &Vehicle::coordinateChanged,
                this,
                &Vehicle::updateGeoPath,
                Qt::QueuedConnection);
    }
    if (!isTemporary()) {
        //connect(f_telemetry, &Fact::activeChanged, this, &Vehicle::resetGeoPath);
        Fact *f = new Fact(this,
                           "rpath",
                           tr("Reset Path"),
                           tr("Clear travelled path"),
                           Action,
                           "history");
        connect(f, &Fact::triggered, this, &Vehicle::resetGeoPath);
        connect(this, &Vehicle::geoPathChanged, f, [this, f]() {
            f->setEnabled(!geoPath().isEmpty());
        });
        f->setEnabled(false);
    }

    updateStatus();
    updateInfo();

    //connect protocols
    if (protocol) {
        connect(protocol, &QObject::destroyed, this, [this]() { this->protocol = nullptr; });
        //status and stream type
        connect(protocol, &ProtocolVehicle::xpdrData, this, &Vehicle::setStreamXpdr);
        connect(protocol->telemetry,
                &ProtocolTelemetry::downstreamDataReceived,
                this,
                &Vehicle::setStreamTelemetry);
        connect(protocol->telemetry,
                &ProtocolTelemetry::valueDataReceived,
                this,
                &Vehicle::setStreamData);
        connect(protocol->telemetry,
                &ProtocolTelemetry::serialDataReceived,
                this,
                &Vehicle::setStreamData);

        connect(protocol->telemetry,
                &ProtocolTelemetry::mandalaValueReceived,
                this,
                &Vehicle::updateDatalinkVars);

        //recorder
        connect(protocol, &ProtocolVehicle::xpdrData, this, &Vehicle::recordDownlink);
        connect(protocol->telemetry,
                &ProtocolTelemetry::downstreamDataReceived,
                this,
                &Vehicle::recordDownlink);
        connect(protocol->telemetry,
                &ProtocolTelemetry::valueDataReceived,
                this,
                &Vehicle::recordDownlink);
        connect(protocol->telemetry,
                &ProtocolTelemetry::serialDataReceived,
                this,
                [this](uint portNo, QByteArray data) {
                    emit recordSerialData(portNo, data, false);
                });
        //connect(this,&Vehicle::recordNodes,this,&Vehicle::dbSaveConfigDataBackup);

        //ident update
        connect(protocol, &ProtocolVehicle::identUpdated, this, [this, protocol]() {
            setSquawk(protocol->squawk);
            setCallsign(protocol->ident.callsign);
            setVehicleClass(static_cast<Vehicle::VehicleClass>(protocol->ident.vclass));
            apxMsg() << tr("Vehicle IDENT updated").append(":") << vehicleClassText()
                     << "'" + this->callsign() + "'"
                     << "(" + squawkText() + ")";
        });

        //forward signals
        connect(protocol->telemetry,
                &ProtocolTelemetry::downstreamDataReceived,
                this,
                &Vehicle::downstreamDataReceived);
        connect(protocol->telemetry,
                &ProtocolTelemetry::valueDataReceived,
                this,
                &Vehicle::valueDataReceived);
        connect(protocol->telemetry,
                &ProtocolTelemetry::serialDataReceived,
                this,
                &Vehicle::serialDataReceived);
    }

    //register JS new vehicles instantly
    connect(this, &Vehicle::nameChanged, this, [=]() { ApxApp::jsync(this); });
    ApxApp::jsync(this);
}
Vehicle::~Vehicle()
{
    qDebug() << "vehicle removed";
}
//=============================================================================
//=============================================================================
void Vehicle::dbSaveVehicleInfo()
{
    if (isReplay())
        return;
    QVariantMap info;
    info.insert("time", QDateTime::currentDateTime().toMSecsSinceEpoch());
    info.insert("uid", uid);
    info.insert("callsign", callsign());
    info.insert("class", vehicleClassText());
    info.insert("squawk", squawkText());
    DBReqSaveVehicleInfo *req = new DBReqSaveVehicleInfo(info);
    connect(req,
            &DBReqSaveVehicleInfo::foundID,
            this,
            &Vehicle::dbSetVehicleKey,
            Qt::QueuedConnection);
    req->exec();
}
void Vehicle::dbSetVehicleKey(quint64 key)
{
    dbKey = key;
}
//=============================================================================
//=============================================================================
void Vehicle::updateTitle()
{
    setName(callsign());
    setTitle(callsign());
}
void Vehicle::updateStatus()
{
    setStatus(streamTypeText());
    f_mandala->setStatus(status());
}
void Vehicle::updateInfo()
{
    QStringList st;
    //st<<callsign();
    if (vehicleClass() != GCU) {
        QString s;
        int alt = f_gps_hmsl->value().toInt();
        if (std::abs(alt) >= 50)
            alt = (alt / 10) * 10;
        else if (alt < 1)
            alt = 0;
        s = QString("MSL%1").arg(alt);

        int vs = -f_gps_Vdown->value().toInt();
        if (vs > 1)
            s.append(QString("+%1").arg(vs));
        else if (vs < -1)
            s.append(QString("%1").arg(vs));
        st << s;
        s = f_mode->text();
        uint stage = f_stage->value().toUInt();
        if (stage > 1)
            s.append(QString("/%1").arg(stage));
        st << s;
    }

    QString v = st.join("\n");
    if (m_info == v)
        return;
    m_info = v;
    emit infoChanged();
}
void Vehicle::updateInfoReq()
{
    if (updateInfoTimer.isActive())
        return;
    updateInfoTimer.start();
}
void Vehicle::updateCoordinate()
{
    setCoordinate(QGeoCoordinate(f_gps_lat->value().toDouble(),
                                 f_gps_lon->value().toDouble(),
                                 f_gps_hmsl->value().toDouble()));
}
void Vehicle::updateFlightState()
{
    if ((f_mode->value().toUInt() == mandala::mode_LANDING) && (f_stage->value().toUInt() >= 250)) {
        setFlightState(FS_LANDED);
    } else if ((f_mode->value().toUInt() == mandala::mode_TAKEOFF)
               && (f_stage->value().toUInt() >= 2) && (f_stage->value().toUInt() < 100)) {
        setFlightState(FS_TAKEOFF);
    } else
        setFlightState(FS_UNKNOWN);
}
void Vehicle::updateGeoPath()
{
    QGeoCoordinate c(coordinate());
    if (!c.isValid())
        return;
    if (c.latitude() == 0.0)
        return;
    if (c.longitude() == 0.0)
        return;
    if (!m_geoPath.isEmpty()) {
        QGeoCoordinate c0(m_geoPath.path().last());
        /*if (c0.latitude() == c.latitude())
            return;
        if (c0.longitude() == c.longitude())
            return;*/
        if (c0.distanceTo(c) < 10.0)
            return;
    }

    m_geoPath.addCoordinate(c);
    emit geoPathChanged();
    //emit geoPathAppend(c);
    if (m_geoPath.size() >= 3) {
        emit geoPathAppend(m_geoPath.path().at(m_geoPath.size() - 3));
    }
}
//=============================================================================
bool Vehicle::isLocal() const
{
    return vehicleClass() == LOCAL;
}
bool Vehicle::isReplay() const
{
    return !protocol; // vehicleClass()==REPLAY;
}
bool Vehicle::isTemporary() const
{
    return vehicleClass() == TEMPORARY;
}
void Vehicle::setReplay(bool v)
{
    if (v) {
        setStreamType(TELEMETRY);
        onlineTimer.start();
    } else if (isReplay()) {
        onlineTimer.stop();
        setStreamType(OFFLINE);
    }
}
QGeoRectangle Vehicle::geoPathRect() const
{
    return geoPath().boundingGeoRectangle();
}
//=============================================================================
QString Vehicle::streamTypeText() const
{
    return QMetaEnum::fromType<StreamType>().valueToKey(streamType());
}
QString Vehicle::vehicleClassText() const
{
    return QMetaEnum::fromType<VehicleClass>().valueToKey(vehicleClass());
}
QString Vehicle::squawkText() const
{
    return squawkText(squawk());
}
QString Vehicle::squawkText(quint16 v) const
{
    return QString::number(v, 16).toUpper();
}
//=============================================================================
/*void Vehicle::dlinkData(const QByteArray &packet)
{
  if(isReplay()) return;
  if(f_nodes->unpackService(packet)){
    emit nmtReceived(packet);
    if(telemetryTime.elapsed()>2000 && xpdrTime.elapsed()>3000)
      setStreamType(SERVICE);
  }else if(f_mandala->unpackTelemetry(packet)){
    emit recordDownlink(packet);
    setStreamType(TELEMETRY);
    telemetryTime.start();
  }else if(f_mandala->unpackData(packet)){
    if(telemetryTime.elapsed()>2000 && xpdrTime.elapsed()>3000)
      setStreamType(DATA);
  }else if(f_mission->unpackMission(packet)){
    if(telemetryTime.elapsed()>2000 && xpdrTime.elapsed()>3000)
      setStreamType(DATA);
  }else return;
  onlineTimer.start();
}*/
//=============================================================================
//=============================================================================
void Vehicle::setStreamXpdr()
{
    //emit recordDownlink(data);
    setStreamType(XPDR);
    xpdrTime.start();
    onlineTimer.start();
}
void Vehicle::setStreamTelemetry()
{
    //emit recordDownlink(packet);
    setStreamType(TELEMETRY);
    telemetryTime.start();
    onlineTimer.start();
}
void Vehicle::setStreamData()
{
    if (telemetryTime.elapsed() > 2000 && xpdrTime.elapsed() > 3000) {
        setStreamType(DATA);
    }
    onlineTimer.start();
}
void Vehicle::setStreamService()
{
    //emit nmtReceived(packet);
    if (telemetryTime.elapsed() > 2000 && xpdrTime.elapsed() > 3000)
        setStreamType(SERVICE);
    onlineTimer.start();
}
//=============================================================================
//=============================================================================
//=============================================================================
void Vehicle::vmexec(QString func)
{
    if (protocol)
        protocol->vmexec(func);
}
void Vehicle::sendSerial(quint8 portID, QByteArray data)
{
    if (protocol)
        protocol->sendSerial(portID, data);
}
//=============================================================================
void Vehicle::flyHere(const QGeoCoordinate &c)
{
    if (!protocol)
        return;
    if (!c.isValid())
        return;
    const QGeoCoordinate h(f_home_lat->value().toDouble(), f_home_lon->value().toDouble());
    qreal azimuth_r = qDegreesToRadians(h.azimuthTo(c));
    qreal distance = h.distanceTo(c);
    qreal n = std::cos(azimuth_r) * distance;
    qreal e = std::sin(azimuth_r) * distance;
    protocol->telemetry->sendPointValue(f_mandala->factByName("cmd_north")->id(), n, e);
}
void Vehicle::lookHere(const QGeoCoordinate &c)
{
    if (!protocol)
        return;
    if (!c.isValid())
        return;
    double hmsl = f_home_hmsl->value().toDouble();
    protocol->telemetry->sendVectorValue(f_mandala->factByName("cam_lat")->id(),
                                         c.latitude(),
                                         c.longitude(),
                                         hmsl);
}
void Vehicle::setHomePoint(const QGeoCoordinate &c)
{
    if (!protocol)
        return;
    if (!c.isValid())
        return;
    double hmsl = f_home_hmsl->value().toDouble();
    protocol->telemetry->sendVectorValue(f_home_lat->id(), c.latitude(), c.longitude(), hmsl);
}
void Vehicle::sendPositionFix(const QGeoCoordinate &c)
{
    if (!protocol)
        return;
    if (!c.isValid())
        return;
    double hmsl = f_gps_hmsl->value().toDouble();
    protocol->telemetry->sendVectorValue(f_gps_lat->id(), c.latitude(), c.longitude(), hmsl);
}
//=============================================================================
void Vehicle::resetGeoPath()
{
    setGeoPath(QGeoPath());
}
//=============================================================================
//=============================================================================
QString Vehicle::fileTitle() const
{
    QString s = confTitle();
    if (s.isEmpty())
        return title();
    return s;
}
QString Vehicle::confTitle() const
{
    if (!(f_nodes->nodesCount() > 0 && f_nodes->dictValid() && f_nodes->dataValid())) {
        return QString();
    }
    QMap<QString, QString> byName;
    QString shiva;
    QString longest;
    QString anyName;
    foreach (NodeItem *node, f_nodes->nodes()) {
        QString name = node->title();
        QString s = node->status().trimmed();
        if (anyName.isEmpty())
            anyName = name;
        if (s.isEmpty())
            continue;
        int sz = s.size();
        if (byName.value(name).size() < sz)
            byName[name] = s;
        if (name.endsWith(".shiva")) {
            if (shiva.size() < sz)
                shiva = s;
        }
        if (longest.size() < sz)
            longest = s;
    }
    if (!shiva.isEmpty())
        return shiva;
    QString s;
    s = byName.value("nav");
    if (!s.isEmpty())
        return s;
    s = byName.value("mhx");
    if (!s.isEmpty())
        return s;
    s = byName.value("ifc");
    if (!s.isEmpty())
        return s;
    s = longest;
    if (!s.isEmpty())
        return s;
    return anyName;
}
//=============================================================================
//=============================================================================
QString Vehicle::mandalaToString(quint16 mid) const
{
    VehicleMandalaFact *mf = f_mandala->factById(mid);
    return mf ? mf->title() : QString();
}
quint16 Vehicle::stringToMandala(const QString &s) const
{
    if ((!s.isEmpty()) && s != "0") {
        VehicleMandalaFact *mf;
        //try int
        bool ok = false;
        uint i = s.toUInt(&ok);
        if (ok && i < 0xFFFF) {
            mf = f_mandala->factById(static_cast<quint16>(i));
            if (mf)
                return mf->id();
        }
        //try text
        mf = f_mandala->factByName(s);
        if (mf)
            return mf->id();
    }
    return 0;
}
const QStringList *Vehicle::mandalaNames() const
{
    return &f_mandala->names;
}
//=============================================================================
void Vehicle::updateDatalinkVars(quint16 id, double)
{
    switch (id) {
    default:
        return;
    case mandala::idx_gcu_RSS:
    case mandala::idx_gcu_Ve:
    case mandala::idx_gcu_MT:
        break;
    }
    Fact *fdest = Vehicles::instance()->f_local->f_mandala->factById(id);
    if (!fdest)
        return;
    Fact *fsrc = f_mandala->factById(id);
    if (!fsrc)
        return;
    fdest->setValue(fsrc->value());
}
//=============================================================================
//=============================================================================
Vehicle::StreamType Vehicle::streamType(void) const
{
    return m_streamType;
}
void Vehicle::setStreamType(const StreamType v)
{
    if (m_streamType == v)
        return;
    m_streamType = v;
    emit streamTypeChanged();
}
quint16 Vehicle::squawk(void) const
{
    return m_squawk;
}
void Vehicle::setSquawk(const quint16 v)
{
    if (m_squawk == v)
        return;
    m_squawk = v;
    emit squawkChanged();
}
QString Vehicle::callsign(void) const
{
    return m_callsign;
}
void Vehicle::setCallsign(const QString &v)
{
    if (m_callsign == v)
        return;
    m_callsign = v;
    emit callsignChanged();
}
Vehicle::VehicleClass Vehicle::vehicleClass(void) const
{
    return m_vehicleClass;
}
void Vehicle::setVehicleClass(const VehicleClass v)
{
    if (m_vehicleClass == v)
        return;
    m_vehicleClass = v;
    emit vehicleClassChanged();
}
void Vehicle::setVehicleClass(const QString &v)
{
    bool ok = false;
    int i = QMetaEnum::fromType<VehicleClass>().keyToValue(v.toUtf8(), &ok);
    if (!ok)
        return;
    setVehicleClass(static_cast<VehicleClass>(i));
}
QString Vehicle::info(void) const
{
    return m_info;
}
bool Vehicle::follow(void) const
{
    return m_follow;
}
void Vehicle::setFollow(const bool &v)
{
    if (m_follow == v)
        return;
    m_follow = v;
    emit followChanged();
}
QGeoCoordinate Vehicle::coordinate(void) const
{
    return m_coordinate;
}
void Vehicle::setCoordinate(const QGeoCoordinate &v)
{
    if (m_coordinate == v)
        return;
    m_coordinate = v;
    emit coordinateChanged();
}
Vehicle::FlightState Vehicle::flightState(void) const
{
    return m_flightState;
}
void Vehicle::setFlightState(const FlightState &v)
{
    if (m_flightState == v)
        return;
    m_flightState = v;
    emit flightStateChanged();
}
QGeoPath Vehicle::geoPath(void) const
{
    return m_geoPath;
}
void Vehicle::setGeoPath(const QGeoPath &v)
{
    if (m_geoPath == v)
        return;
    m_geoPath = v;
    emit geoPathChanged();
}
//=============================================================================
