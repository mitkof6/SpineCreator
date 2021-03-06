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

#include "SC_export_network_image.h"
#include "ui_export_network_image.h"
#include "SC_network_layer_rootdata.h"
#include "SC_network_3d_visualiser_panel.h"

saveNetworkImageDialog::saveNetworkImageDialog(nl_rootdata * data, QString fileName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::saveNetworkImageDialog)
{
    ui->setupUi(this);

    height = -1;

    this->setWindowTitle("Image export settings for network");

    // set data in place
    this->data = data;
    this->fileName = fileName;

    // setup combobox
    this->style = saveNetworkImageDrawStyle;
    ui->comboBox->addItem("SpineCreator");
    ui->comboBox->addItem("Circle and Arrow");
    ui->comboBox->setCurrentIndex(0);
    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDrawStyle(int)));

    // setup sizes
    ui->scale_spin->setValue(3.0);
    ui->scale_spin->setMaximum(8.0);
    ui->scale_spin->setMinimum(0.1);
    scale = 3.0;
    connect(ui->scale_spin,SIGNAL(valueChanged(double)), this, SLOT(changeScale(double)));
    ui->border_spin->setValue(0.3f);
    border = 0.3f;
    ui->scale_spin->setMaximum(5.0);
    connect(ui->border_spin,SIGNAL(valueChanged(double)), this, SLOT(changeBorder(double)));

    // setup listView

    // setup preview
    QPixmap pix = drawPixMap();

    // width and height:
    ui->height_label->setText("Height = " + QString::number(pix.height()));
    ui->width_label->setText("Width = " + QString::number(pix.width()));

    pix = pix.scaled(ui->preview->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);

    ui->preview->setPixmap(pix);

    // for now
    ui->objects->setVisible(false);
    ui->label_2->setVisible(false);

    connect(ui->buttonBox,SIGNAL(accepted()), this, SLOT(save()));

}

saveNetworkImageDialog::saveNetworkImageDialog(glConnectionWidget * glConnWidget, QString fileName, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::saveNetworkImageDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Image export settings for visualisation");

    // set data in place
    this->glConnWidget = glConnWidget;
    this->fileName = fileName;

    // setup combobox
    ui->comboBox->hide();

    // setup sizes
    ui->scaleLabel->setText("Width");
    ui->borderLabel->setText("Height");

    ui->height_label->hide();
    ui->width_label->hide();

    ui->scale_spin->setMaximum(20000.0);
    ui->scale_spin->setMinimum(100);
    ui->scale_spin->setValue(1000.0);

    ui->border_spin->setMaximum(20000.0);
    ui->border_spin->setMinimum(100);
    ui->border_spin->setValue(1000.0);

    width = 1000;
    height = 1000;

    connect(ui->scale_spin,SIGNAL(valueChanged(double)), this, SLOT(changeWidth(double)));
    connect(ui->border_spin,SIGNAL(valueChanged(double)), this, SLOT(changeHeight(double)));

    // setup listView
    ui->objects->hide();

    // setup preview
    QPixmap pix = drawPixMapVis();

    pix = pix.scaled(ui->preview->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);

    ui->preview->setPixmap(pix);

    // for now
    ui->objects->setVisible(false);
    ui->label_2->setVisible(false);

    connect(ui->buttonBox,SIGNAL(accepted()), this, SLOT(save()));

}

saveNetworkImageDialog::~saveNetworkImageDialog()
{
    delete ui;
}

void saveNetworkImageDialog::reDrawPreview()
{
    QPixmap pix;
    if (height > 0)
        pix = drawPixMapVis();
    else {
        pix = drawPixMap();
        ui->height_label->setText("Height = " + QString::number(pix.height()));
        ui->width_label->setText("Width = " + QString::number(pix.width()));
    }

    pix = pix.scaled(ui->preview->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);

    ui->preview->setPixmap(pix);
}

bool saveNetworkImageDialog::drawOrderLessThan (const QSharedPointer<systemObject>& o1,
                                                const QSharedPointer<systemObject>& o2)
{
    return (o1->type < o2->type ? true : false);
}

QRectF saveNetworkImageDialog::calculateBoundingBox (QVector <QSharedPointer<systemObject> >& list)
{
    QRectF bounds = QRectF(100000,100000,-200000,-200000);

    // work out bounding box. Set the bounds from each population,
    // gradually widening the bounding box to incorporate each
    // population.
    for (int p = 0; p < list.size(); ++p) {

        if (list[p]->type == populationObject) {
            QSharedPointer <population> pop = qSharedPointerDynamicCast <population> (list[p]);
            if (-pop->bottomBound(pop->targy)> bounds.bottom())
                bounds.setBottom(-pop->bottomBound(pop->targy));
            if (-pop->topBound(pop->targy)< bounds.top())
                bounds.setTop(-pop->topBound(pop->targy));
            if (pop->leftBound(pop->targx)< bounds.left())
                bounds.setLeft(pop->leftBound(pop->targx));
            if (pop->rightBound(pop->targx)> bounds.right())
                bounds.setRight(pop->rightBound(pop->targx));
        }

        // If the population has projections, increase the bounding
        // box to incorporate the projections also. This will not
        // take into account projection labels.
        if (list[p]->type == projectionObject) {

            QSharedPointer <projection> proj = qSharedPointerDynamicCast <projection> (list[p]);
            for (int c = 0; c < proj->curves.size(); ++c) {
                bezierCurve * bz = &proj->curves[c];
                if (-bz->C1.y() > bounds.bottom())
                    bounds.setBottom(-bz->C1.y());
                if (-bz->C1.y() < bounds.top())
                    bounds.setTop(-bz->C1.y());
                if (bz->C1.x() < bounds.left())
                    bounds.setLeft(bz->C1.x());
                if (bz->C1.x() > bounds.right())
                    bounds.setRight(bz->C1.x());

                if (-bz->C2.y() > bounds.bottom())
                    bounds.setBottom(-bz->C2.y());
                if (-bz->C2.y() < bounds.top())
                    bounds.setTop(-bz->C2.y());
                if (bz->C2.x() < bounds.left())
                    bounds.setLeft(bz->C2.x());
                if (bz->C2.x() > bounds.right())
                    bounds.setRight(bz->C2.x());
            }
        }
    }

    bounds.setTopLeft(bounds.topLeft() - QPointF(border,border));
    bounds.setBottomRight(bounds.bottomRight() + QPointF(border,border));

    return bounds;
}

void saveNetworkImageDialog::setupPainter (QPainter* painter)
{
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter->setRenderHint(QPainter::Antialiasing,true);
    painter->setRenderHint(QPainter::TextAntialiasing,true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform,true);

    // setup the painter
    QFont font("Monospace", 5.0f);
    font.setStyleHint(QFont::TypeWriter);
    painter->setFont(font);
}

QPixmap saveNetworkImageDialog::drawPixMap()
{
    QVector <QSharedPointer<systemObject> > list = this->getDrawableList();
    QRectF bounds = this->calculateBoundingBox (list);
    QPixmap outPix(bounds.width()*100*scale, bounds.height()*100*scale);

    if (ui->checkBox->isChecked()) {
        outPix.fill(Qt::transparent);
    } else {
        outPix.fill();// white
    }

    if (list.empty()) {
        // User hasn't made a selection, so open a dialog to hint that
        // a selection is required for an image. (tested here so that
        // we can return a null QPixmap)
        QMessageBox::warning(this, QString("No populations selected"),
                             QString("The image will be blank as no populations have been selected. "
                                     "Please select at least one population for the image."));

        return outPix;
    }

    QPainter *painter = new QPainter(&outPix);
    this->setupPainter (painter);
    this->renderDrawables (painter, list, bounds);
    painter->end();
    delete painter;

    return outPix;
}

QVector <QSharedPointer<systemObject> >
saveNetworkImageDialog::getDrawableList (void)
{
    QVector <QSharedPointer<systemObject> > list = data->selList;
    // Order the list now to ensure the correct drawing output?
    // Populations then projections then genericinputs.
    qSort(list.begin(), list.end(), saveNetworkImageDialog::drawOrderLessThan);
    return list;
}

void saveNetworkImageDialog::renderDrawables (QPainter* painter,
                                              QVector <QSharedPointer<systemObject> >& list,
                                              const QRectF& bounds)
{
    // Just render selection list:
    for (int i = 0; i < list.size(); ++i) {
            list[i]->draw(painter, 200.0*scale,
                          -bounds.center().x(), -bounds.center().y(), bounds.width()*100*scale,
                          bounds.height()*100*scale, data->popImage, this->style);
    }
}

void saveNetworkImageDialog::drawSVG (QSvgGenerator& svg)
{
    QVector <QSharedPointer<systemObject> > list = this->getDrawableList();
    QRectF bounds = this->calculateBoundingBox (list);

    if (list.empty()) {
        // User hasn't made a selection, so open a dialog to hint that
        // a selection is required for an image. (tested here so that
        // we can return a null QPixmap)
        QMessageBox::warning(this, QString("No populations selected"),
                             QString("The image will be blank as no populations have been selected. "
                                     "Please select at least one population for the image."));

        return;
    }

    // The correct resolution is 100 dpi. This related to the size of
    // a population, which appears to be 1 inch high and at scale==1
    // is 100 pixels high.
    svg.setResolution (100);

    QPainter *painter = new QPainter(&svg);
    this->setupPainter (painter);
    this->renderDrawables (painter, list, bounds);
    painter->end();
    delete painter;
}

QPixmap saveNetworkImageDialog::drawPixMapVis()
{
    return glConnWidget->renderImage(width, height);
}

void saveNetworkImageDialog::changeScale(double value)
{
    scale = value;
    reDrawPreview();
}

void saveNetworkImageDialog::changeBorder(double value)
{
    border = value;
    reDrawPreview();
}

void saveNetworkImageDialog::changeWidth(double value)
{
    width = round(value);
    ((QDoubleSpinBox *) sender())->setValue(round(value));
    reDrawPreview();
}

void saveNetworkImageDialog::changeHeight(double value)
{
    height = round(value);
    ((QDoubleSpinBox *) sender())->setValue(round(value));
    reDrawPreview();
}

void saveNetworkImageDialog::changeDrawStyle(int index)
{
    switch (index) {
    case 0:
        // "SpineCreator" in the menu
        this->style = saveNetworkImageDrawStyle;
        break;
    case 1:
        // "Circle and arrow" in the menu
        this->style = microcircuitDrawStyle;
        break;
    }
    reDrawPreview();
}

void saveNetworkImageDialog::save()
{
    this->fileName = QFileDialog::getSaveFileName(this, tr("Export As Image"), qgetenv("HOME"), tr("SVG (*.svg);;PNG (*.png)"));

    if (this->fileName.isEmpty()) {
        return;
    }

    if (fileName.endsWith("png", Qt::CaseInsensitive)) {
        // PNG Save
        QPixmap pix;
        if (height > 0) {
            pix = drawPixMapVis();
        } else {
            pix = drawPixMap();
        }

        QImage outIm = pix.toImage();
        outIm.save(fileName,"png");

    } else {
        // SVG Save by default
        QSvgGenerator svg;
        svg.setFileName (this->fileName);
        svg.setTitle("SpineCreator Image");
        svg.setDescription ("Image exported from SpineCreator");
        this->drawSVG (svg);
    }
}
