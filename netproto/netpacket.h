#ifndef NETPACKET_H
#define NETPACKET_H

#include <string>
#include <sstream>



                        /***********************************
                        *                                  *
                        *   NETWORK PROTOCOL DEFINITION    *
                        *                                  *
                        ***********************************/


#define NET_PROTO_PACKET_ID_FIELD_LEN 2    // length of identification field of network packet (bytes)
#define NET_PROTO_PACKET_STOP_SYMBOL '\0'  // stop symbol of packet
#define NET_PROTO_PACKET_SEP_SYMBOL ' '    // packet fields separation symbol



                    /*  Network packet base class definition  */

class NetPacket
{
public:
    enum NetPacketID {NETPACKET_ID_UNKNOWN = -1, NETPACKET_ID_HELLO, NETPACKET_ID_COMMAND, NETPACKET_ID_FEATURE};
    enum NetPacketSender {NETPACKET_SENDER_UNKNOWN = -1, NETPACKET_SENDER_SERVER, NETPACKET_SENDER_CLIENT};

    NetPacket(const NetPacketID id);
    NetPacket(const NetPacketID id, const std::string &content);
    NetPacket(const NetPacketID id, const char *content);

    virtual ~NetPacket();

    void setID(const NetPacket::NetPacketID id);

    template<typename... T>
    void setContent(T... args);

    NetPacket::NetPacketID id() const;
    std::string content() const;

protected:
    NetPacketID _id;
    std::string _content;

    std::string _packet;

    virtual void makePacket();

    std::stringstream ss;

    template<typename T1, typename... T2>
    void argHelper(T1 first, T2... last);

    template<typename T>
    void argHelper(T arg);

    void argHelper();
};


                /*  Class definition for HELLO-type network packet  */


class NetPacketHello: public NetPacket
{
public:
    NetPacketHello(const NetPacket::NetPacketSender sender_id);
    NetPacketHello(const NetPacket::NetPacketSender sender_id, const std::string &description);
    NetPacketHello(const NetPacket::NetPacketSender sender_id, const char *description);

    NetPacket::NetPacketSender senderID() const;
    std::string description() const;

protected:
    NetPacketSender _senderID;
    std::string _description;
};




                /*  Class definition for COMMAND-type network packet  */


class NetPacketCommand: public NetPacket
{
public:
    template<typename... T>
    NetPacketCommand(const std::string &cmd_name,  T... args);

    template<typename... T>
    NetPacketCommand(const char *cmd_name,  T... args);

    std::string cmdName() const;
    std::string params() const;


protected:
    std::string _cmdName;
    std::string _params;

    virtual void makePacket();

};




            /*  Class definition for FEATURE-type network packet  */

enum NetPacketFeatureType {NETPACKET_FEATURE_UNKNOWN = -1, NETPACKET_FEATURE_INTTYPE,
                           NETPACKET_FEATURE_FLOATYPE, NETPACKET_FEATURE_STRINGTYPE};

class NetPacketAbstractFeature: public NetPacket
{
public:
    NetPacketFeatureType type() const;
    std::string name() const;

protected:
    NetPacketAbstractFeature(const NetPacketFeatureType type, const std::string &name);
    NetPacketAbstractFeature(const NetPacketFeatureType type, const char *name);

    virtual ~NetPacketAbstractFeature();

    NetPacketFeatureType _type;
    std::string _name;
};


template<typename T, NetPacketFeatureType type>
class NetPacketFeature: public NetPacketAbstractFeature
{
public:
    NetPacketFeature(const std::string &name, const T value);
    NetPacketFeature(const char *name, const T value);

    T value() const;

protected:
    T _value;
};


#endif // NETPACKET_H
