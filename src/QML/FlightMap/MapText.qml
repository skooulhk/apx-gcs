import QtQuick 2.5;
import "."

Rectangle {
    id: mapText

    property alias text: textItem.text
    property alias font: textItem.font
    property alias horizontalAlignment: textItem.horizontalAlignment
    property alias verticalAlignment: textItem.verticalAlignment
    property alias textColor: textItem.color

    property bool square: false

    property int margins: 2
    property int rightMargin: 0
    property int minWidth: 0
    property int minHeight: 0

    Behavior on implicitWidth { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }
    Behavior on implicitHeight { enabled: app.settings.smooth.value; NumberAnimation {duration: 100; } }

    border.width: 0
    color: "gray"
    smooth: true
    implicitWidth: (square?textItem.width:textItem.contentWidth)+mapText.margins*2+1+rightMargin
    implicitHeight: Math.max(minHeight,textItem.contentHeight+mapText.margins*2+1)
    radius: 3
    clip: true
    Text {
        id: textItem
        x: mapText.margins
        y: mapText.margins
        color: "white"
        font.bold: true
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        width: Math.max(minWidth-mapText.margins*2-1-rightMargin,square?Math.max(contentWidth,contentHeight):implicitWidth)
        height: Math.max(minHeight-mapText.margins*2-1,contentHeight)
    }
}
