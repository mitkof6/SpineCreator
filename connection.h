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

#ifndef CONNECTION_H
#define CONNECTION_H

#include "globalHeader.h"

#include "rootdata.h"
#include "nineML_classes.h"
#include "population.h"

#define NO_DELAY -1 // used to determine if Python Scripts have delay data

struct change {
    int row;
    int col;
    float value;
};

class connection: public QObject
{
    Q_OBJECT

public:
    connection();
    virtual ~connection();

    QString name;
    connectionType type;

    virtual void write_node_xml(QXmlStreamWriter &){}
    virtual void import_parameters_from_xml(QDomNode &){}
    virtual void write_metadata_xml(QDomDocument &, QDomNode &) {}
    virtual void read_metadata_xml(QDomNode &) {}
    virtual void writeDelay(QXmlStreamWriter &xmlOut);
    virtual QLayout * drawLayout(rootData * , viewVZLayoutEditHandler * , rootLayout * ) {return new QHBoxLayout();}

    virtual connection * newFromExisting() {return new connection;}
    virtual int getIndex();

    virtual QString getTypeStr(void);

    ParameterData * delay;

    /*!
     * The source population for this connection.
     */
    QSharedPointer <population> src;

    /*!
     * The destination population for this connection.
     */
    QSharedPointer <population> dst;

    /*!
     * Setter for srcName.
     */
    void setSrcName (QString& s);

    /*!
     * Setter for dstName.
     */
    void setDstName (QString& d);

    /*!
     * Obtain the index number of the synapse in which this connection
     * exists.
     */
    int getSynapseIndex();

    /*!
     * Set the index number of the synapse in which this connection
     * exists.
     */
    void setSynapseIndex(int synidx);

private:
    QString filename;

protected:
    /*!
     * The name of the source population for this connection. This may
     * be used where it is inconvenient to set the
     * QSharedPointer<population> src.
     */
    QString srcName;

    /*!
     * The name of the dest population for this connection. This may
     * be used where it is inconvenient to set the
     * QSharedPointer<population> dst.
     */
    QString dstName;

    /*!
     * The synapse index for this connection (a projection between two
     * populations can have many synapses, which count from
     * 0). Initialised in constructors to a negative number, which
     * must be changed later to a valid number 0 or above.
     */
    int synapseIndex;
};

class alltoAll_connection : public connection
{
        Q_OBJECT
public:
    alltoAll_connection();
    ~alltoAll_connection();

    void write_node_xml(QXmlStreamWriter &xmlOut);
    void import_parameters_from_xml(QDomNode &);
    QLayout * drawLayout(rootData * data, viewVZLayoutEditHandler * viewVZhandler, rootLayout * rootLay);
    connection * newFromExisting() {alltoAll_connection * c = new alltoAll_connection; c->delay = new ParameterData(this->delay); return c;}

private:
};

class onetoOne_connection : public connection
{
        Q_OBJECT
public:
    onetoOne_connection();
    ~onetoOne_connection();

    void write_node_xml(QXmlStreamWriter &xmlOut);
    void import_parameters_from_xml(QDomNode &);
    QLayout * drawLayout(rootData * data, viewVZLayoutEditHandler * viewVZhandler, rootLayout * rootLay);
    connection * newFromExisting() {onetoOne_connection * c = new onetoOne_connection; c->delay = new ParameterData(this->delay); return c;}

private:
};

class fixedProb_connection : public connection
{
        Q_OBJECT
public:
    fixedProb_connection();
    ~fixedProb_connection();

    QStringList values;
    void write_node_xml(QXmlStreamWriter &xmlOut);
    void import_parameters_from_xml(QDomNode &);
    QLayout * drawLayout(rootData * data, viewVZLayoutEditHandler * viewVZhandler, rootLayout * rootLay);
    connection * newFromExisting() {
        fixedProb_connection * c = new fixedProb_connection;
        c->p = this->p;
        c->seed = this->seed;
        c->delay = new ParameterData(this->delay);
        return c;
    }


    // the probability of a connection
    float p;
    int seed;

private:
};

/*!
 * \brief The csv_connection class
 * This class is a subclass of connection. It allows the use of explicit connection lists
 * in the form of source-destination pairs, with an optional individual or global delay.
 */
class csv_connection : public connection
{
        Q_OBJECT
public:

    csv_connection(QString fileName);
    csv_connection();
    ~csv_connection();

    QStringList values;
    /*!
     * \brief import_csv
     * \param filename
     * Import data into the connection from a file written in Comma Seperated Variable format.
     * This format consists of ASCII text data written as S,D,L/n where S is the source index,
     * D is the destination index and L (optional) is the delay.
     */
    void import_csv(QString filename);
    /*!
     * \brief import_packed_binary
     * \param fileIn The source file from which to import
     * \param fileOut The connection's storage file
     * Import data into the connection from a file written in the packed binary format. This
     * format consists of close packed binary data with the structure (int S)(int D)(opt float L)
     * where S is the source index, D is the dest index, and optionally L is the delay.
     */
    void import_packed_binary(QFile &fileIn, QFile& fileOut);
    QVector <float> fetchData(int index);
    void getAllData(QVector < conn > &conns);
    float getData(int, int);
    float getData(QModelIndex &index);
    QString getHeader(int section);
    int getNumRows();
    void setNumRows(int);
    int getNumCols();
    void setNumCols(int);
    void setData(const QModelIndex & index, float value);
    void setData(int, int, float);
    void clearData();
    void flushChangesToDisk();
    void abortChanges();
    void write_node_xml(QXmlStreamWriter &xmlOut);
    void write_metadata_xml(QDomDocument &, QDomNode &);
    void import_parameters_from_xml(QDomNode &);
    void read_metadata_xml(QDomNode &);
    void setFileName(QString name);
    QString getFileName();
    void fetch_headings();
    connection * generator;
    QLayout * drawLayout(rootData *, viewVZLayoutEditHandler * viewVZhandler, rootLayout * rootLay);
    int getIndex();
    connection * newFromExisting();

private:
    QString filename;
    QXmlStreamWriter xmlOut;
    QXmlStreamReader xmlIn;
    int numRows;
    QVector < change > changes;

    /*!
     * Generate a filename based on the source and destination
     * population names, throwing an exception if either of these is
     * not set.
     */
    void generateFilename(void);


    /*!
     * Get the directory used for storage of the connection binary
     * files - i.e. the model directory.
     */
    QDir getLibDir (void);

    /*!
     * Replace chars in str which are not in the string allowed with
     * replaceChar.
     */
    void sanitizeReplace (QString& str,
                          const QString& allowed,
                          const char replaceChar);
};

class pythonscript_connection : public connection
{
        Q_OBJECT
public:
    pythonscript_connection(QSharedPointer <population> src, QSharedPointer <population> dst, csv_connection *conn_targ);
    pythonscript_connection() {
        type = Python;
        this->isAList = false;
        selfConnections = false;
        rotation = 0;
        hasChanged = true;
        this->scriptValidates = false;
        this->hasWeight = false;
        this->hasDelay = false;
    }

    ~pythonscript_connection();

    void write_node_xml(QXmlStreamWriter &xmlOut);
    void import_parameters_from_xml(QDomNode &);
    void write_metadata_xml(QDomDocument &, QDomNode &);
    void read_metadata_xml(QDomNode &);
    int getIndex();
    QString getTypeStr(void);

    float rotation;
    QString errorLog;

    QSharedPointer <population> src;
    QSharedPointer <population> dst;
    QVector < conn > *conns;
    QMutex * mutex;
    bool isList();
    bool selfConnections;
    bool changed();

    QVector <conn> connections;

    QString scriptText;
    QString lastGeneratedScriptText;
    QString scriptName;
    QStringList parNames;
    QVector <double> parValues;
    QVector <double> lastGeneratedParValues;
    QVector < QPoint > parPos;

    QString weightProp;
    QString lastGeneratedWeightProp;
    QVector <double> weights;

    QString pythonErrors;

    bool scriptValidates;
    bool hasWeight;
    bool hasDelay;

    ParameterData *getPropPointer();
    QStringList getPropList();
    QLayout * drawLayout(rootData * data, viewVZLayoutEditHandler * viewVZhandler, rootLayout * rootLay);

    // the explicit connection list to copy the generated weights to
    csv_connection * connection_target;

    connection * newFromExisting();

private:

    csv_connection * explicitList;
    bool isAList;
    bool hasChanged;
    int srcSize;
    int dstSize;


public slots:
    void generate_connections();
    //void convertToList(bool);
    /*!
     * \brief configureFromScript
     * Get a Python script as a string and parse it to set up the connection
     * parameters. Is a slot so it can be triggered by events.
     */
    void configureFromScript(QString);

    void regenerateConnections();

    void setUnchanged(bool);

    /*!
     * \brief enableGen
     * A simple slot to allow the QDoubleSpinBoxes with the pars to re-enable the Generator button
     */
    void enableGen(double);

    /*!
     * \brief enableGen
     * A simple slot to allow the QComboBox with the Propoerty for the weight to re-enable the Generator button
     */
    void enableGen(int);

signals:
    void progress(int);
    void connectionsDone();
    void setGenEnabled(bool);

};

#endif // CONNECTION_H
