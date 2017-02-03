#pragma once
#include <MessageIdentifiers.h>
#include <cstring>
#include <string>
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
		ID_PST4_MESSAGE_SESSION_ID = ID_USER_PACKET_ENUM + 5,

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

	struct serverToClientIdPacket
	{
		serverToClientIdPacket(size_t id) : type{ ID_PST4_MESSAGE_SESSION_ID }, clientId{ id } {}
		unsigned char type;
		size_t clientId;
	};

	struct Vect3f
	{
		Vect3f() = default;
		Vect3f(float _x, float _y, float _z) : x{ _x }, y{ _y }, z{ _z } {}
		float x;
		float y;
		float z;
	};

	struct Quatf
	{
		Quatf() = default;
		Quatf(float _w, float _x, float _y, float _z) : w{ _w }, x{ _x }, y{ _y }, z{ _z } {}
		float x;
		float y;
		float z;
		float w;
	};

	struct headPosePacket
	{
		headPosePacket() : type{ ID_PST4_MESSAGE_HEAD_POSE } {};

#ifdef I_AM_CLIENT
		headPosePacket(size_t sessionId, Annwvyn::AnnVect3 position, Annwvyn::AnnQuaternion orientation) : type{ ID_PST4_MESSAGE_HEAD_POSE }, sessionId{ sessionId }
		{
			absPos.x = position.x;
			absPos.y = position.y;
			absPos.z = position.z;

			absOrient.x = orientation.x;
			absOrient.y = orientation.y;
			absOrient.z = orientation.z;
			absOrient.w = orientation.w;
		}
#endif

		unsigned char type;
		size_t sessionId;
		Vect3f absPos;
		Quatf absOrient;
	};

#pragma pack(pop)
}