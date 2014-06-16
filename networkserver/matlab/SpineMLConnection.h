/* -*-c++-*- */

#include <iostream>
#include <deque>
#include <vector>

extern "C" {
#include <unistd.h>
#include <errno.h>
}

//extern map<int, SpineMLConnection*>* connections;

using namespace std;

// SpineML tcp/ip comms flags.
#define RESP_DATA_NUMS     31 // a non-printable character
#define RESP_DATA_SPIKES   32 // ' ' (space)
#define RESP_DATA_IMPULSES 33 // '!'
#define RESP_HELLO         41 // ')'
#define RESP_RECVD         42 // '*'
#define RESP_ABORT         43 // '+'
#define RESP_FINISHED      44 // ','
#define AM_SOURCE          45 // '-'
#define AM_TARGET          46 // '.'
#define NOT_SET            99 // 'c'

// SpineML tcp/ip comms data types
enum dataTypes {
    ANALOG,
    EVENT,
    IMPULSE
};

// Handshake stages:
#define CS_HS_GETTINGTARGET     0
#define CS_HS_GETTINGDATATYPE   1
#define CS_HS_GETTINGDATASIZE   2
#define CS_HS_DONE              3

// How many times to fail to read a byte before calling the session a
// failure:
#define NO_DATA_MAX_COUNT   10000

/*
 * A connection class. The SpineML client code connects to this server
 * with a separate connection for each stream of data. For example,
 * population A makes one connection to obtain its input, population B
 * makes a second connection for input. population C makes an output
 * connection. This class holds the file descriptor of the connection,
 * plus information (obtained during the connection handshake) about
 * the data direction, data type and data size.
 *
 * Each one of these connections is expected to run on a separate
 * thread, which means you can use blocking i/o for reading and
 * writing to the network.
 */
class SpineMLConnection
{
public:
    SpineMLConnection()
        : connectingSocket(0)
        , established(false)
        , clientDataDirection (NOT_SET)
        , clientDataType (NOT_SET)
        , clientDataSize (1)
        {
            cout << "SpineMLConnection::SpineMLConnection constructor" << endl;
            pthread_mutex_init (&this->dataMutex, NULL);
        };
    ~SpineMLConnection()
        {
            cout << "SpineMLConnection::SpineMLConnection destructor. close socket..." << endl;
            if (this->connectingSocket > 0) {
                this->closeSocket();
            }
            cout << "SpineMLConnection::SpineMLConnection destructor. destroy mutex..." << endl;
            pthread_mutex_destroy(&this->dataMutex);
        };

    /*!
     * Simple accessors
     */
    //@{
    int getConnectingSocket (void);
    void setConnectingSocket (int i);
    char getClientDataDirection (void);
    char getClientDataType (void);
    unsigned int getClientDataSize (void);
    bool getEstablished (void);
    //@}

    /*!
     * Run through the handshake process. This sets clientDataDirection,Type and Size.
     */
    int doHandshake (void);

    /*!
     * Example this->data. If we have data to write, then write it to
     * the client.
     */
    int doWriteToClient (void) volatile;

    /*!
     * Perform input/output with the client. This will call either
     * doWriteToClient or doReadFromClient.
     */
    int doInputOutput (void);

    /*!
     * Closing the connecting socket.
     */
    void closeSocket (void);

public:
    /*!
     * The thread on which this connection will execute.
     */
    pthread_t thread;

private:
    /*!
     * The file descriptor of the TCP/IP socket on which this connection is running.
     */
    int connectingSocket;
    /*!
     * Set to true once the connection is fully established and the
     * handshake is complete.
     */
    bool established;
    /*!
     * The data direction, as set by the client. Client sends a flag
     * which is either AM_SOURCE (I am a source) or AM_TARGET (I am a
     * target).
     */
    char clientDataDirection; // AM_SOURCE or AM_TARGET
    /*!
     * There are 3 possible data types; nums(analog), spikes(events)
     * or impulses. Only nums implemented.
     */
    char clientDataType;
    /*!
     * The data size. This is the number of data per timestep. For
     * nums, that means it's the number of doubles per timestep.
     */
    unsigned int clientDataSize;
    /*!
     * A mutex for access to the data.
     */
    pthread_mutex_t dataMutex;
    /*!
     * The data which is accessed on the matlab side.
     */
    volatile std::deque<double> data;
};

/*!
 * Accessor implementations
 */
//@{
int
SpineMLConnection::getConnectingSocket (void)
{
    return this->connectingSocket;
}
void
SpineMLConnection::setConnectingSocket (int i)
{
    this->connectingSocket = i;
}
char
SpineMLConnection::getClientDataDirection (void)
{
    return this->clientDataDirection;
}
char
SpineMLConnection::getClientDataType (void)
{
    return this->clientDataType;
}
unsigned int
SpineMLConnection::getClientDataSize (void)
{
    return this->clientDataSize;
}
bool
SpineMLConnection::getEstablished (void)
{
    return this->established;
}
//@}

/*
 * Go through the handshake process, as defined in protocol.txt.
 *
 * The client.h code in SpineML_2_BRAHMS refers to 3 stages in the
 * handshake process: "initial handshake", "set datatype", "set
 * datasize". Here, these 3 stages are referred to in combination as
 * the "handshake".
 */
int
SpineMLConnection::doHandshake (void)
{
    ssize_t b = 0;
    char buf[16];
    // There are three stages in the handshake process:
    int handshakeStage = CS_HS_GETTINGTARGET;
    int noData = 0; // Incremented everytime we get no data. Used so
                    // that we don't spin forever waiting for the
                    // handshake.
    while (handshakeStage != CS_HS_DONE && noData < NO_DATA_MAX_COUNT) {

        if (handshakeStage == CS_HS_GETTINGTARGET) {
            if (!noData) {
                cout << "SpineMLConnection::doHandshake: CS_HS_GETTINGTARGET. this->connectingSocket: " << this->connectingSocket << endl;
            }
            b = read (this->connectingSocket, (void*)buf, 1);
            if (b == 1) {
                // Got byte.
                cout << "SpineMLConnection::doHandshake: Got byte: '"
                     << buf[0] << "'/0x" << hex << (int)buf[0] << dec << endl;
                if (buf[0] == AM_SOURCE || buf[0] == AM_TARGET) {
                    this->clientDataDirection = buf[0];
                    // Write response.
                    buf[0] = RESP_HELLO;
                    if (write (this->connectingSocket, buf, 1) != 1) {
                        cout << "SpineMLConnection::doHandshake: Failed to write RESP_HELLO to client." << endl;
                        return -1;
                    }
                    cout << "SpineMLConnection::doHandshake: Wrote RESP_HELLO to client." << endl;
                    handshakeStage++;
                    noData = 0; // reset the "no data" counter
                } else {
                    // Wrong data direction.
                    this->clientDataDirection = NOT_SET;
                    cout << "SpineMLConnection::doHandshake: Wrong data direction in first handshake byte from client." << endl;
                    return -1;
                }
            } else {
                // No byte read, increment the no_data counter.
                ++noData;
            }

        } else if (handshakeStage == CS_HS_GETTINGDATATYPE) {
            if (!noData) {
                cout << "SpineMLConnection::doHandshake: CS_HS_GETTINGDATATYPE." << endl;
            }
            cout << "SpineMLConnection::doHandshake: call read()" << endl;
            b = read (this->connectingSocket, (void*)buf, 1);
            if (b == 1) {
                // Got byte.
                cout << "SpineMLConnection::doHandshake: Got byte: '"
                     << buf[0] << "'/0x" << hex << (int)buf[0] << dec << endl;
                if (buf[0] == RESP_DATA_NUMS || buf[0] == 'a') { // a is for test/debug
                    this->clientDataType = buf[0];
                    buf[0] = RESP_RECVD;
                    if (write (this->connectingSocket, buf, 1) != 1) {
                        cout << "SpineMLConnection::doHandshake: Failed to write RESP_RECVD to client." << endl;
                        return -1;
                    }
                    cout << "SpineMLConnection::doHandshake: Wrote RESP_RECVD to client." << endl;
                    handshakeStage++;
                    noData = 0; // reset the "no data" counter

                } else if (buf[0] == RESP_DATA_SPIKES || buf[0] == RESP_DATA_IMPULSES) {
                    // These are not yet implemented.
                    cout << "SpineMLConnection::doHandshake: Spikes/Impulses not yet implemented." << endl;
                    return -1;

                } else {
                    // Wrong/unexpected character.
                    cout << "SpineMLConnection::doHandshake: Character '" << buf[0] << "'/0x" << (int)buf[0] << " is unexpected here." << endl;
                    return -1;
                }
            } else {
                if (noData < 10) {
                    cout << "SpineMLConnection::doHandshake: Got " << b << " bytes, not 1" << endl;
                }
                ++noData;
            }

        } else if (handshakeStage == CS_HS_GETTINGDATASIZE) {
            if (!noData) {
                cout << "SpineMLConnection::doHandshake: CS_HS_GETTINGDATASIZE." << endl;
            }
            b = read (this->connectingSocket, (void*)buf, 4);
            if (b == 4) {
                // Got 4 bytes. This is the data size - the number of
                // doubles to transmit during each timestep. E.g.: If
                // a population has 10 neurons, then this is probably
                // 10. Interpret as an unsigned int, with the first
                // byte in the buffer as the least significant byte:
                this->clientDataSize =
                    (unsigned char)buf[0]
                    | (unsigned char)buf[1] << 8
                    | (unsigned char)buf[2] << 16
                    | (unsigned char)buf[3] << 24;

                cout << "SpineMLConnection::doHandshake: client data size: "
                     << this->clientDataSize << " doubles/timestep" << endl;

                buf[0] = RESP_RECVD;
                if (write (this->connectingSocket, buf, 1) != 1) {
                    cout << "SpineMLConnection::doHandshake: Failed to write RESP_RECVD to client." << endl;
                    return -1;
                }
                handshakeStage++;
                noData = 0;

            } else if (b > 0) {
                // Wrong number of bytes.
                cout << "SpineMLConnection::doHandshake: Read " << b << " bytes, expected 4." << endl;
                for (ssize_t i = 0; i<b; ++i) {
                    cout << "buf[" << i << "]: '" << buf[i] << "'  0x" << hex << buf[i] << dec << endl;
                }
                return -1;
            } else {
                ++noData;
            }

        } else if (handshakeStage == CS_HS_DONE) {
            cout << "SpineMLConnection::doHandshake: Handshake finished." << endl;
        } else {
            cout << "SpineMLConnection::doHandshake: Error: Invalid handshake stage." << endl;
            return -1;
        }
    }
    if (noData >= NO_DATA_MAX_COUNT) {
        cout << "SpineMLConnection::doHandshake: Error: Failed to get data from client." << endl;
        return -1;
    }

    // This connection is now established:
    this->established = true;

    return 0;
}

/*
 * Write a datum to the client and then read the acknowledgement byte.
 */
int
SpineMLConnection::doWriteToClient (void) volatile
{
    // This is just me putting some static data in for testing/development.
    vector<double> dbls;
    for (int i = 0; i < this->clientDataSize; ++i) {
        dbls.push_back(5.0);
    }

    cout << "SpineMLConnection::doWriteToClient: write... this->connectingSocket: " << this->connectingSocket << endl;
    ssize_t bytesWritten = write (this->connectingSocket, &(dbls[0]), this->clientDataSize*sizeof(double));
    if (bytesWritten != this->clientDataSize*sizeof(double)) {
        int theError = errno;
        cout << "SpineMLConnection::doWriteToClient: Failed. Wrote " << bytesWritten
             << " bytes. Tried to write " << (this->clientDataSize*sizeof(double)) << ". errno: "
             << theError << endl;
        return -1;
    }
    cout << "SpineMLConnection::doWriteToClient: wrote " << bytesWritten << " bytes." << endl;
    // Expect an acknowledgement now from the client
    char buf[16];
    int b = read (this->connectingSocket, (void*)buf, 1);
    if (b == 1) {
        // Good.
        if (buf[0] != RESP_RECVD) {
            cout << "SpineMLConnection::doWriteToClient: Wrong response from client." << endl;
            return -1;
        }
    } else {
        int theError = errno;
        cout << "SpineMLConnection::doWriteToClient: Failed to read 1 byte from client. errno: " << theError << endl;
        return -1;
    }

    return 0;
}

int
SpineMLConnection::doInputOutput (void)
{
    // Check if this is an established connection.
    if (this->established == false) {
        cout << "SpineMLConnection::doInputOutput: connection is not established, returning 0." << endl;
        return 0;
    }

    // We're going to update some data in memory
    pthread_mutex_lock (&this->dataMutex);

    // Update the buffer by reading/writing from network.
    if (this->clientDataDirection == AM_TARGET) {
        // Client is a target, I need to write data to the client, if I have anything to write.
        cout << "SpineMLConnection::doInputOutput: clientDataDirection: AM_TARGET." << endl;
        if (this->doWriteToClient()) {
            cout << "SpineMLConnection::doInputOutput: Error writing to client." << endl;
            return -1;
        }

    } else if (this->clientDataDirection == AM_SOURCE) {
        // Client is a source, I need to read data from the client.
        // this->doReadFromClient();
        cout << "SpineMLConnection::doInputOutput: clientDataDirection: AM_SOURCE." << endl;
    } else {
        // error.
        cout << "SpineMLConnection::doInputOutput: clientDataDirection has wrong value: "
             << (int)this->clientDataDirection << endl;
    }

    pthread_mutex_unlock (&this->dataMutex);
    return 0;
}

void
SpineMLConnection::closeSocket (void)
{
    if (close (this->connectingSocket)) {
        int theError = errno;
        cout << "SpineMLConnection::closeSocket: Error closing connecting socket: " << theError << endl;
    } else {
        this->connectingSocket = 0;
    }
    this->established = false;
}
