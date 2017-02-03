#include <MessageIdentifiers.h>
#include <linux_safe.h>

namespace PST4
{
	static bool secure_strcpy(char* dest, rsize_t len, const char* src)
	{
		auto result = strcpy_s(dest, len, src);
		if (result != 0) return false;
		return true;
	}

	/*Structure of any RakNet packet :

	----------------------------------
	|ID (1 Byte) | Payload (variable)|
	----------------------------------

	The ID must be one of the values defined either inside RakNet, or one of the enumerated below
	*/

	enum ID_PST4_MESSAGE_TYPE
	{
		//TODO define RakNet messages here
		ID_PST4_MESSAGE_ECHO = ID_USER_PACKET_ENUM + 1,
		ID_PST4_MESSAGE_HEAD_POSE = ID_USER_PACKET_ENUM + 2,
		ID_PST4_MESSAGE_HAND_POSE = ID_USER_PACKET_ENUM + 3,
		ID_PST4_MESSAGE_SPEEX_BUFFER = ID_USER_PACKET_ENUM + 4,

		ID_PST4_MESSAGE_HEARTBEAT = ID_USER_PACKET_ENUM + 10
	};

	//Example of packet : a simple echo
#pragma pack(push, 1)
	///This packet transport a simple string of text as a payload;
	struct echoPacket
	{
		echoPacket() : type{ ID_PST4_MESSAGE_ECHO } {}	//Default constructor, will allways put the type at the right value
		echoPacket(const std::string& str) :			//Commodity constructor, assign the good bits where they should go
			type{ ID_PST4_MESSAGE_ECHO }
		{
			if (str.length() < 255)
				if (!secure_strcpy(message, sizeof message, str.c_str()))
					throw std::runtime_error("Error while copying string into echo packet.");
		}

		//The order here is EXTRA IMPORTANT!
		unsigned char type; //One byte that will contain the type of the packet, defined in the enumeration above

		//Payload here
		char message[256];
	};

	struct heartbeatPacket
	{
		heartbeatPacket() : type{ ID_PST4_MESSAGE_HEARTBEAT } {}

		unsigned char type;
	};
#pragma pack(pop)
}