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
#ifndef FactData_H
#define FactData_H
//=============================================================================
#include "FactTree.h"
//=============================================================================
class FactData: public FactTree
{
  Q_OBJECT
  Q_ENUMS(DataType)
  Q_ENUMS(ActionType)

  Q_PROPERTY(DataType dataType READ dataType WRITE setDataType NOTIFY dataTypeChanged)

  Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)

  Q_PROPERTY(bool modified READ modified WRITE setModified NOTIFY modifiedChanged)

  Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)
  Q_PROPERTY(QVariant min READ min WRITE setMin NOTIFY minChanged)
  Q_PROPERTY(QVariant max READ max WRITE setMax NOTIFY maxChanged)

  Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
  Q_PROPERTY(QString descr READ descr WRITE setDescr NOTIFY descrChanged)

  Q_PROPERTY(QString text READ text NOTIFY textChanged)

  Q_PROPERTY(const QStringList & enumStrings READ enumStrings NOTIFY enumStringsChanged)

  Q_PROPERTY(QString units READ units WRITE setUnits NOTIFY unitsChanged)

  Q_PROPERTY(QVariant defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged)
public:

  enum DataType {
    NoData =0,      // no editor
    ConstData,
    TextData,
    FloatData,
    IntData,
    BoolData,
    EnumData,       // value=value of enumStrings (set by text or index or enumValues)
    MandalaData,    // Mandala ID
    ScriptData,     // script editor
    ActionData,     // button, value=action type
    KeySequenceData,
  };
  Q_ENUM(DataType)

  enum ActionType {
    NormalAction =0,
    ButtonAction,
    RemoveAction,
    UplinkAction,
  };
  Q_ENUM(ActionType)

  explicit FactData(FactTree *parent, const QString &name, const QString &title, const QString &descr, ItemType treeItemType, DataType dataType);


  void copyValuesFrom(const FactData *item);

  int enumValue(const QVariant &v) const;
  QString enumText(int v) const;

  virtual bool isZero() const;

  virtual void bind(FactData *item);

  void defaults();

  //FactTree override
  virtual void insertItem(int i, FactTree *item);
  virtual void removeItem(FactTree *item, bool deleteLater=true);

  //Mandala support - must override in derived classes
  virtual QString mandalaToString(quint16 mid) const;
  virtual quint16 stringToMandala(const QString &s) const;
  virtual const QStringList * mandalaNames() const;

signals:
  void childValueChanged(void);

public slots:
  void backup();
  void restore();



protected:
  FactData *_bindedFact;

  QVariant backup_value;
  bool backup_set;

public:
  //---------------------------------------
  DataType dataType() const;
  void setDataType(const DataType &v);

  virtual QVariant value(void) const;
  Q_INVOKABLE virtual bool setValue(const QVariant &v);

  bool modified() const;
  virtual void setModified(const bool &v, const bool &recursive=false);

  int precision(void) const;
  void setPrecision(const int &v);
  QVariant min(void) const;
  void setMin(const QVariant &v);
  QVariant max(void) const;
  void setMax(const QVariant &v);

  QString title(void) const;
  void setTitle(const QString &v);
  QString descr(void) const;
  void setDescr(const QString &v);

  QString text() const;

  virtual const QStringList & enumStrings() const;
  void setEnumStrings(const QStringList &v, const QList<int> &enumValues=QList<int>());
  void setEnumStrings(const QMetaEnum &v);

  QString units() const;
  void setUnits(const QString &v);

  QVariant defaultValue(void) const;
  void setDefaultValue(const QVariant &v);

protected:
  DataType m_dataType;

  QVariant m_value;

  bool m_modified;

  int  m_precision;
  QVariant m_min;
  QVariant m_max;

  QString  m_title;
  QString  m_descr;

  QStringList  m_enumStrings;
  QList<int>   m_enumValues;

  QString  m_units;

  QVariant m_defaultValue;

signals:
  void dataTypeChanged();

  void valueChanged();

  void modifiedChanged();

  void precisionChanged();
  void minChanged();
  void maxChanged();

  void titleChanged();
  void descrChanged();

  void textChanged();
  void enumStringsChanged();

  void unitsChanged();

  void defaultValueChanged();
};
//=============================================================================
#endif
