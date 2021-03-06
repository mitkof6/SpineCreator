/***************************************************************************
**                                                                        **
**  This file is part of SpineCreator, an easy to use GUI for             **
**  describing spiking neural network models.                             **
**  Copyright (C) 2013-2014 Alex Cope, Paul Richmond, Seb James           **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Alex Cope                                            **
**  Website/Contact: http://bimpa.group.shef.ac.uk/                       **
****************************************************************************/

#ifndef SAVENETWORKIMAGE_DIALOG_H
#define SAVENETWORKIMAGE_DIALOG_H

#include <QDialog>
#include "globalHeader.h"
#include <QSvgGenerator>

namespace Ui {
class saveNetworkImageDialog;
}

class saveNetworkImageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit saveNetworkImageDialog(nl_rootdata *data, QString fileName, QWidget *parent = 0);
    explicit saveNetworkImageDialog(glConnectionWidget *glConnWidget, QString fileName, QWidget *parent = 0);
    ~saveNetworkImageDialog();

private:
    Ui::saveNetworkImageDialog *ui;
    nl_rootdata * data;
    glConnectionWidget * glConnWidget;
    /*!
     * A sorting algorithm for a list of system objects. This orders
     * the members so that projections are drawn upon populations and
     * generic inputs are drawn upon everything. Used in drawPixMap().
     */
    static bool drawOrderLessThan (const QSharedPointer<systemObject>& o1,
                                   const QSharedPointer<systemObject>& o2);
    /*!
     * Set up the painter for drawing the network image. This code is
     * common both to PNG and SVG rendering.
     */
    void setupPainter (QPainter* painter);

    /*!
     * Calculate the bounding box for the image. Common for both SVG
     * and PNG rendering.
     */
    QRectF calculateBoundingBox (QVector <QSharedPointer<systemObject> >& list);

    /*!
     * Get the list of drawable objects, and sort it. Common to both
     * SVG and PNG rendering.
     */
    QVector <QSharedPointer<systemObject> > getDrawableList (void);

    /*!
     * Go through the list of drawables and render each one by calling
     * the draw method for each member of the list.
     */
    void renderDrawables (QPainter* painter,
                          QVector <QSharedPointer<systemObject> >& list,
                          const QRectF& bounds);

    void drawSVG(QSvgGenerator& svg);
    QPixmap drawPixMap();
    QPixmap drawPixMapVis();
    float scale;
    float border;
    int height;
    int width;
    void reDrawPreview();
    QString fileName;
    drawStyle style;

public slots:
    void changeScale(double);
    void changeBorder(double);
    void changeWidth(double);
    void changeHeight(double);
    void changeDrawStyle(int);
    void save();
};

#endif // SAVENETWORKIMAGE_DIALOG_H
