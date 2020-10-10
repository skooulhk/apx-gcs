﻿import QtQuick 2.6
import QtQuick.Controls 2.1
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import QtQml 2.12

import APX.Facts 1.0
import Apx.Common 1.0

ColumnLayout {
    id: menuPage

    //Fact bindings
    property var fact

    property bool pageInfo: false //load fact info instead of list
    property var pageInfoAction //load fact action info instead fact info

    property string pageTitle: fact.title
    property string pageDescr: fact.descr
    property string pageStatus: fact.text

    property bool valid: fact?true:false

    Component.onCompleted: {
        pageLoader.source=pageSource()
    }

    StackView.onRemoved: {
        if(menuPage)menuPage.destroy()
    }

    onValidChanged: if(!valid) back()

    Component {
        id: factC
        Fact {}
    }
    Connections {
        enabled: valid
        target: fact
        function onMenuBack()
        {
            //console.log("menuBack")
            valid=false
        }
        function onRemoved()
        {
            //console.log("removed",fact)
            valid=false
            fact=factC.createObject(this)
        }
    }


    property int padding: 4
    clip: true

    Loader {
        active: valid
        asynchronous: true
        source: "FactMenuPageTitle.qml"
        Layout.fillWidth: true
        Layout.leftMargin: padding
        Layout.rightMargin: padding
        Layout.topMargin: padding
        Layout.alignment: Qt.AlignTop|Qt.AlignHCenter
        clip: true
    }
    Loader {
        id: pageLoader
        active: valid
        asynchronous: true
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: padding
        Layout.rightMargin: padding
        Layout.topMargin: padding
        clip: true
    }

    function pageSource()
    {
        if(fact.opts.page){
            var s=fact.opts.page
            if(s.indexOf(":")>=0){
                return s
            }
            return "../"+s
        }
        return "FactMenuPageList.qml"
    }
}
