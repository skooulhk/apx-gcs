﻿import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Material 2.2
import GCS.FactSystem 1.0

Item {
    id: field
    width: parent?parent.width:0
    height: visible?(sep?4:fieldItemSize):0

    visible: fact?fact.visible:true
    focus: visible && enabled

    property int fieldItemSize: (showDescr||bAction)?itemSize*1.3:itemSize

    //configurable properties
    property Fact fact
    property string title: fact?fact.title:""
    property string descr: fact?fact.descr:""
    property bool separator: false
    property var busy: false
    property bool showBusy: !delegate
    property bool showDescr: descr && app.settings.showdescr.value

    property bool checkable: fact && fact.dataType==Fact.BoolData
    property bool checked: fact && fact.dataType==Fact.BoolData && fact.value

    property bool enabled: fact?fact.enabled:true

    property Component delegate

    property string page        //page file to be loaded in body
    property string pageMenu    //page file to be loaded in stackView directly (nested menus)
    property GCSMenuModel fields //sub menu fields

    property var itemData

    signal clicked()
    signal toggled()

    onClicked: {
        if(fact && fact.dataType==Fact.BoolData && (!showNext)){
            fact.value = !fact.value
        }
    }

    onToggled: {
        if(fact && fact.dataType==Fact.BoolData){
            fact.value = !fact.value
        }
    }


    //internal
    property bool sep: visible && separator && (!sephdg)
    property bool sephdg: visible && separator && title
    property bool showNext: visible && (fields || pageMenu || page || (fact && fact.size)) && (!delegate) && (!bEditList) && (!bEditListText)

    //editor types
    property bool bEditText:     field.fact && field.fact.dataType==Fact.TextData && field.fact.size === 0
    property bool bEditList:     field.fact && field.fact.dataType==Fact.EnumData && field.fact.size>0
    property bool bEditListText: field.fact && field.fact.dataType==Fact.TextData && field.fact.size>0
    property bool bAction:       field.fact && field.fact.dataType==Fact.ActionData

    Button {
        id: fieldButton
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: bAction?undefined:parent.left
        anchors.right: parent.right
        //anchors.fill: bAction?undefined:parent
        anchors.topMargin: 1 //space between fields in list view
        visible: field.visible

        enabled: field.visible && field.enabled && (!field.separator) //&& (!field.delegate)
        Material.background: sephdg?"transparent":bAction?(field.fact.value===Fact.RemoveAction?colorActionRemove:undefined):colorBgField
        flat: !bAction
        hoverEnabled: enabled
        highlighted: enabled

        background.height: bAction?(field.height-background.y*2):(field.height-1)
        background.y: bAction?background.y:0

        text: bAction?field.title:""

        onClicked: {
            //console.log("click menu");
            field.focus=true;
            if(fact)fact.trigger();
            if(fact && (fact.treeItemType!=Fact.FactItem)){
                //console.log("open: "+field);
                openFact(fact)
            }else if(fields){
                //console.log("open: "+field);
                openMenuField(field)
            }else if(field.checkable){
                //field.checked=!field.checked
                //menuItemSwitch.toggle()
            }else if(field.pageMenu){
                pushUrl(field.pageMenu)
            }else if(field.page){
                openPage({"page": Qt.resolvedUrl("../"+field.page),"title": field.title})
                //console.log("open: "+field.page);
            }else if(bEditText){
                //editTextDialog.open()
            }else if(closeable)close(); //click and close
            field.clicked()
            if(bAction) back();
        }

        RowLayout {
            id: fieldBody
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            anchors.topMargin: 0
            anchors.bottomMargin: 0
            spacing: 2
            visible: !field.bAction
            //clip: true
            Item { //field title text
                id: fieldTitle
                Layout.fillHeight: true
                Layout.fillWidth: !fieldDelegate.visible
                Layout.preferredWidth: titleText.contentWidth+height/4
                visible: field.visible?field.title:false
                ColumnLayout {
                    id: fieldTitleD
                    anchors.fill: parent
                    anchors.bottomMargin: 1
                    spacing: 0
                    Label { //title
                        id: titleText
                        Layout.fillWidth: true
                        Layout.fillHeight: !showDescr
                        height: font.pixelSize
                        //anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: sephdg?Text.AlignHCenter:Text.AlignLeft
                        font.pixelSize: itemSize*0.6
                        color: sephdg?colorTitleSep:"#fff"
                        font.family: sephdg?font_narrow:font_condenced
                        text: field.visible?field.title:""
                        clip: true
                    }
                    Label { //descr
                        id: descrText
                        visible: showDescr
                        Layout.fillWidth: true
                        //anchors.fill: parent
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        font.pixelSize: itemSize*0.4
                        color: "#888"
                        font.family: font_condenced
                        text: field.visible?field.descr:""
                        clip: true
                    }
                }
                FastBlur {
                    anchors.fill: fieldTitleD
                    transparentBorder: true
                    source: fieldTitleD
                    radius: fieldTitle.height/2
                    visible: effects && fieldButton.hovered
                }
            }

            Item {
                id: fieldDelegate
                Layout.fillHeight: true
                Layout.fillWidth: true
                visible: field.delegate
            }

            BusyIndicator {
                id: fieldBusy
                running: field.busy||fieldBusyTimer.running
                visible: running && field.showBusy
                Layout.fillHeight: true
                implicitWidth: height
                Connections {
                    target: fieldButton
                    onClicked: fieldBusy.start()
                }
                Timer {
                    id: fieldBusyTimer
                    interval: 500; running: false; repeat: false
                }
                function start()
                {
                    if(field.showBusy)fieldBusyTimer.start();
                }
            }


            Component.onCompleted: { //if(field.visible){
                //optional field editors
                if(field.delegate){
                    field.delegate.createObject(fieldDelegate,{"anchors.fill": fieldDelegate, "modelData": field});
                }
                if(bEditText){
                    fieldTextC.createObject(fieldBody);
                }
                if(bEditList || bEditListText){
                    fieldListC.createObject(fieldBody);
                }
                if(field.checkable){
                    fieldSwitchC.createObject(fieldBody);
                }
                if(showNext){
                    fieldNextC.createObject(fieldBody);
                }
            }


            // -------------------------------------------
            //          EDITORS
            // -------------------------------------------

            Component {
                id: fieldNextC
                Image {
                    //Layout.fillHeight: true
                    height: itemSize*0.8
                    verticalAlignment: Image.AlignVCenter
                    sourceSize.height: itemSize*0.8
                    source: iconNext
                }
            }

            Component {
                id: fieldSwitchC
                Switch {
                    Layout.fillHeight: true
                    //Layout.preferredWidth: height
                    anchors.verticalCenter: parent.verticalCenter
                    checked: field.checked
                    enabled: field.enabled
                    onClicked: {
                        fieldBusy.start();
                        field.toggled();
                    }
                }
            }
            Component {
                id: fieldTextC
                TextInput {
                    Layout.fillHeight: true
                    Layout.minimumWidth: itemSize*2
                    //Layout.fillWidth: true
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: itemSize*0.6
                    color: activeFocus?colorValueTextEdit:colorValueText
                    font.family: font_condenced
                    text: field.fact.text
                    //clip: true
                    selectByMouse: true
                    onEditingFinished: {
                        field.fact.value=text;
                        parent.forceActiveFocus();
                    }
                    onActiveFocusChanged: if(activeFocus)selectAll();
                    Rectangle {
                        visible: field.enabled
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: -itemSize*0.05
                        anchors.rightMargin: anchors.leftMargin
                        anchors.verticalCenter: parent.verticalCenter
                        height: itemSize*0.8
                        radius: 3
                        color: "transparent"
                        border.width: 1
                        border.color: parent.color
                        opacity: 0.3
                    }
                    Connections {
                        target: field
                        onClicked: dialog.open()
                    }
                    Dialog {
                        id: dialog
                        modal: true
                        title: field.title + " ("+field.fact.descr+")"
                        standardButtons: Dialog.Ok | Dialog.Cancel
                        parent: menuPage
                        x: (parent.width - width) / 2
                        y: (parent.height - height) / 2
                        implicitWidth: itemSize*10
                        onAboutToShow: {
                            editor.text=field.fact.value;
                            editor.selectAll();
                            editor.forceActiveFocus();
                        }
                        onAccepted: field.fact.value=editor.text
                        TextField {
                            id: editor
                            anchors.left: parent.left
                            anchors.right: parent.right
                            selectByMouse: true
                            placeholderText: field.fact.descr
                            onAccepted: dialog.accept()
                        }
                    }
                }
            }
            Component {
                id: fieldListC
                ComboBox {
                    id: editor
                    Layout.fillHeight: true
                    Layout.fillWidth: editable
                    editable: field.bEditListText
                    model: field.fact
                    textRole: "name"
                    //currentIndex: find("by")
                    contentItem: editable?textInputC.createObject(editor):editor.contentItem
                    background: editable?textInputBgC.createObject(editor):editor.background
                    Component.onCompleted: currentIndex=find(field.fact.text)
                    /*onAccepted: {
                        field.fact.value=editText
                        parent.forceActiveFocus();
                    }*/
                    onActivated: {
                        field.fact.value=textAt(index)
                        parent.forceActiveFocus();
                    }
                    Component {
                        id: textInputC
                        TextInput {
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            font.pixelSize: itemSize*0.6
                            color: activeFocus?colorValueTextEdit:colorValueText
                            font.family: font_condenced
                            text: field.fact.text
                            selectByMouse: true
                            onEditingFinished: {
                                field.fact.value=text;
                                editor.parent.forceActiveFocus();
                            }
                            onActiveFocusChanged: if(activeFocus)selectAll();
                        }
                    }
                    Component {
                        id: textInputBgC
                        Item {}
                    }
                }
            }
        }
    }
}
