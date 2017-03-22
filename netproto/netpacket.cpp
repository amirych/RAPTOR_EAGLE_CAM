#include "netpacket.h"


#include <iomanip>
//#include <regex>

//static std::string NETPACKET_ELEMENT_REGEX_STR = std::string(NET_PROTO_PACKET_SEP_SYMBOL) + "*" + "[^" +
//        NET_PROTO_PACKET_SEP_SYMBOL + "]" + NET_PROTO_PACKET_SEP_SYMBOL + "*";

//static std::regex NETPACKET_ELEMENT_REGEX(NETPACKET_ELEMENT_REGEX_STR);



// function deletes leading and trailing whitesaces
static std::string trim_spaces(const std::string& s, const std::string& whitespace = " \t")
{
    std::string str = s;

    std::size_t strBegin = str.find_first_not_of(whitespace);

    if (strBegin == std::string::npos) {
        str.clear();
        return str; // empty string
    }

    std::size_t strEnd = str.find_last_not_of(whitespace);
    std::size_t strRange = strEnd - strBegin + 1;

    str.assign(str, strBegin, strRange);

    return str;
}


static std::vector<std::string> split_str(const std::string &str)
{
    std::vector<std::string> p;
    if ( str.empty() ) return p;

    size_t i1, i2;

    i1 = str.find_first_not_of(NET_PROTO_PACKET_SEP_SYMBOL);
    while ( i1 != std::string::npos ) {
        i2 = str.find_first_of(NET_PROTO_PACKET_SEP_SYMBOL,i1+1);
        if ( i2 == std::string::npos ) {
            p.push_back(str.substr(i1));
            break;
        } else {
            p.push_back(str.substr(i1,i2-i1));
        }
        i1 = str.find_first_not_of(NET_PROTO_PACKET_SEP_SYMBOL);
    }

    return p;
}



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


void NetPacket::parsePacket()
{
    std::vector<std::string> p;

    std::string id_str = trim_spaces( _packet.substr(0,NET_PROTO_PACKET_ID_FIELD_LEN) );

    size_t i;
    long val;

    try {
        val = std::stol(id_str,&i);
        if ( i != id_str.size() ) {
            throw NetPacketException(NetPacket::NETPACKET_ERROR_INVALID_ID_FIELD, "Invalid ID field");
        }

        if ( (val >= NetPacket::NETPACKET_ID_UNKNOWN) || (val < 0) ) {
            throw NetPacketException(NetPacket::NETPACKET_ERROR_ID_FIELD_OUTOFRANGE,
                                     "ID field value is out of valid range");
        }

        _id = static_cast<NetPacket::NetPacketID>(val);
        _content = trim_spaces( _packet.substr(NET_PROTO_PACKET_ID_FIELD_LEN) );

    } catch (std::invalid_argument &ex) {
        throw NetPacketException(NetPacket::NETPACKET_ERROR_INVALID_ID_FIELD, "Invalid ID field");
    }
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


void NetPacketHello::parsePacket()
{
    NetPacket::parsePacket();

    if ( _id != NetPacket::NETPACKET_ID_HELLO ) {
        throw NetPacketException(NetPacket::NETPACKET_ERROR_ID_MISMATCH,"ID value does not correspond to packet type");
    }

    std::vector<std::string> fields = split_str(_content);

    if ( !fields.size() ) {
        throw NetPacketException(NetPacket::NETPACKET_ERROR_INVALID_CONTENT_FIELD,
                                 "Invalid content field");
    }

    size_t i;
    long val;

    try {
        // try to get mandatory 'sender ID' value
        val = std::stol(fields[0],&i);
        if ( i != fields[0].size() ) {
            throw NetPacketException(NetPacket::NETPACKET_ERROR_INVALID_SENDER_ID,
                                     "Invalid sender ID field");
        }

        if ( (val < 0) || (val >= NETPACKET_SENDER_UNKNOWN) ) {
            throw NetPacketException(NetPacket::NETPACKET_ERROR_SENDER_ID_OUTOFRANGE,
                                     "Sender ID value is out of range");
        }

        _senderID = static_cast<NetPacket::NetPacketSender>(val);

        // optional 'description' field. join all fields
        _description.clear();
        for (int i = 1; i < fields.size(); ++i ) {
            _description += NET_PROTO_PACKET_SEP_SYMBOL + fields[i];
        }
        if ( _description.size() ) _description = _description.substr(1); // delete leading sep. symbol

    } catch ( std::invalid_argument &ex ) {
        throw NetPacketException(NetPacket::NETPACKET_ERROR_INVALID_SENDER_ID,
                                 "Invalid sender ID field");
    }
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
    NetPacketFeature(std::string(name), value)
{
}


template<typename T, NetPacketFeatureType type>
T NetPacketFeature<T, type>::value() const
{
    return _value;
}




                /*  NetPacketException class implementation  */

NetPacketException::NetPacketException(const NetPacket::NetPacketError err, const std::string &context):
    exception(), _err(err), _context(context)
{
}


NetPacketException::NetPacketException(const NetPacket::NetPacketError err, const char *context):
    NetPacketException(err, std::string(context))
{
}


NetPacket::NetPacketError NetPacketException::err() const
{
    return _err;
}


const char* NetPacketException::what() const noexcept
{
    return _context.c_str();
}
