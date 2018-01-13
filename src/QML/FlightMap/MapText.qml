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

    Behavior on width { enabled: app.settings.smooth.value; NumberAnimation {duration: 200; } }
    Behavior on height { enabled: app.settings.smooth.value; NumberAnimation {duration: 200; } }

    border.width: 0
    color: "gray"
    smooth: true
    implicitWidth: (square?textItem.width:textItem.contentWidth)+mapText.margins*2+1
    implicitHeight: textItem.contentHeight+mapText.margins*2+1
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
        width: square?Math.max(contentWidth,contentHeight):implicitWidth
    }
}
