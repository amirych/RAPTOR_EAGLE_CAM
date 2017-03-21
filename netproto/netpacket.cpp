#include "netpacket.h"


#include <iomanip>

                    /*  Network packet base class implementation  */

NetPacket::NetPacket(const NetPacketID id, const std::string &content):
    _id(id), _content(content)
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

template<typename T>
NetPacketFeature<T>::NetPacketFeature(const std::string &name, const T &value):
    NetPacket(NetPacket::NETPACKET_ID_FEATURE), _name(name), _value(value)
{
    setContent(_name,_value);
}


template<typename T>
T NetPacketFeature<T>::value() const
{
    return _value;
}


template<typename T>
std::string NetPacketFeature<T>::name() const
{
    return _name;
}
