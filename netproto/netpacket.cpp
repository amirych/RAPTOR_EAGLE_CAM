#include "netpacket.h"


#include <iomanip>

                    /*  Network packet base class implementation  */

NetPacket::NetPacket(const NetPacketID id, const std::string &content):
    _id(id), _content(content), _packet()
{
    makePacket();
}


NetPacket::NetPacket(const NetPacketID id, const char *content):
    NetPacket(id,std::string(content))
{

}

NetPacket::NetPacket(const NetPacketID id):
    NetPacket(id,std::string(""))
{
}


NetPacket::~NetPacket()
{

}


NetPacket::NetPacketID NetPacket::id() const
{
    return _id;
}


std::string NetPacket::content() const
{
    return _content;
}


void NetPacket::setID(const NetPacket::NetPacketID id)
{
    _id = id;
    _content.clear();
    makePacket();
}


template<typename... T>
void NetPacket::setContent(T... args)
{
    ss.str("");

    argHelper(args ...);
    _content = ss.str();

    makePacket();
}


void NetPacket::makePacket()
{
    ss.str("");

    ss << std::right << std::setw(NET_PROTO_PACKET_ID_FIELD_LEN) << _id <<
          _content << NET_PROTO_PACKET_STOP_SYMBOL;

    _packet = ss.str();
}


template<typename T1, typename... T2>
void NetPacket::argHelper(T1 first, T2... last)
{
    argHelper(first);
    if ( sizeof...(last) ) {
        ss << NET_PROTO_PACKET_SEP_SYMBOL;
        argHelper(last ...);
    }
}


template<typename T>
void NetPacket::argHelper(T arg)
{
    ss << arg;
}


void NetPacket::argHelper()
{
}


                /*  Class implementation for HELLO-type network packet  */


NetPacketHello::NetPacketHello(const NetPacket::NetPacketSender sender_id, const std::string &description):
    NetPacket(NetPacket::NETPACKET_ID_HELLO), _senderID(sender_id), _description(description)
{
    setContent(_senderID,_description);
}


NetPacketHello::NetPacketHello(const NetPacket::NetPacketSender sender_id, const char *description):
    NetPacketHello(sender_id,std::string(description))
{
}


NetPacketHello::NetPacketHello(const NetPacket::NetPacketSender sender_id):
    NetPacketHello(sender_id,"")
{
}


NetPacket::NetPacketSender NetPacketHello::senderID() const
{
    return _senderID;
}


std::string NetPacketHello::description() const
{
    return _description;
}


                /*  Class implementation for COMMAND-type network packet  */

template<typename... T>
NetPacketCommand::NetPacketCommand(const std::string &cmd_name,  T... args):
    NetPacket(NetPacket::NETPACKET_ID_COMMAND), _cmdName(cmd_name), _params("")
{
    setContent(args ...);
    _params = _content;

    makePacket();
}


template<typename... T>
NetPacketCommand::NetPacketCommand(const char *cmd_name,  T... args):
    NetPacketCommand(std::string(cmd_name), args ...)
{
}


void NetPacketCommand::makePacket()
{
    _content = _cmdName + NET_PROTO_PACKET_SEP_SYMBOL + _params;

    NetPacket::makePacket();
}



                /*  Class implementation for FEATURE-type network packet  */

NetPacketAbstractFeature::NetPacketAbstractFeature(const NetPacketFeatureType type, const std::string &name):
    NetPacket(NetPacket::NETPACKET_ID_FEATURE), _type(type), _name(name)
{
}


NetPacketAbstractFeature::NetPacketAbstractFeature(const NetPacketFeatureType type, const char *name):
    NetPacketAbstractFeature(type, std::string(name))
{
}


NetPacketAbstractFeature::~NetPacketAbstractFeature()
{
}


NetPacketFeatureType NetPacketAbstractFeature::type() const
{
    return _type;
}

std::string NetPacketAbstractFeature::name() const
{
    return _name;
}



template<typename T, NetPacketFeatureType type>
NetPacketFeature<T, type>::NetPacketFeature(const std::string &name, const T value):
    NetPacketAbstractFeature(type, name), _value(value)
{
    setContent(_name,_value);
}


template<typename T, NetPacketFeatureType type>
NetPacketFeature<T, type>::NetPacketFeature(const char *name, const T value):
    NetPacketFeature<T,type>(std::string(name), value)
{
}


template<typename T, NetPacketFeatureType type>
T NetPacketFeature<T, type>::value() const
{
    return _value;
}
