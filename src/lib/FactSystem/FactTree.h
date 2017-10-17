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
#ifndef FactTree_H
#define FactTree_H
//=============================================================================
#include <QtCore>
//=============================================================================
class FactTree: public QAbstractListModel
{
  Q_OBJECT
  Q_ENUMS(ItemType)

  Q_PROPERTY(ItemType treeItemType READ treeItemType CONSTANT)
  Q_PROPERTY(int level READ level CONSTANT)
  Q_PROPERTY(int size READ size NOTIFY sizeChanged)


public:

  enum ItemType {
    RootItem =0,
    GroupItem,
    FactItem,
    ConstItem,
  };

  explicit FactTree(FactTree *parent, ItemType treeItemType);

  //tree structure manipulation
  virtual void addItem(FactTree *child);
  virtual void removeItem(FactTree *child);

  //internal tree
  int num() const;
  FactTree * child(int n) const;
  FactTree * parentItem() const;
  QList<FactTree*> childItems() const;

public slots:
  virtual void clear(void);
signals:
  void structChanged(FactTree *item);

protected:
  //ListModel override
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;

  QList<FactTree*> m_items;
  FactTree *m_parentItem;

  //-----------------------------------------
  //PROPERTIES
public:
  virtual ItemType treeItemType() const;
  virtual int level(void) const;
  virtual int size() const;
protected:
  ItemType m_treeItemType;
  int m_level;
signals:
  void sizeChanged();
};
//=============================================================================
#endif
