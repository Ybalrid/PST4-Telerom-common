#pragma once

//stream interface
#include <iostream>
//RakNet messages enum
#include <MessageIdentifiers.h>
//C functions for moving strings and bytes around
#include <cstring>
//C++ string
#include <string>
//Hand coded "safe" functions for GNU/Linux
#include <linux_safe.h>

//This can be defined by the includer. Add Annwvyn types translation
#ifdef I_AM_CLIENT
#include <Annwvyn.h>
#endif

namespace PST4
{
	///Do a secure string copy, needs the max size dest can hold. Return false upon any modes of failure
	static bool secure_strcpy(char* dest, rsize_t destMax, const char* src)
	{
		auto result = strcpy_s(dest, destMax, src);
		if (result != 0) return false;
		return true;
	}

	/*Structure of any RakNet packet :

	------------------------------------
	| ID (1 Byte) | Payload (variable) |
	------------------------------------

	The ID must be one of the values defined either inside RakNet, or one of the enumerated below.

	The Payload is whatever we want.

	Packets are sent as "just a bunch of bytes". We're defining packets as 1 byte aligned (packed) structs.
	C and C++ structs have the same "memory" representation, but in semantics, C++ treat a struct as a "class with members public by default"

	This has 2 effect on our code :

	- When defining "struct Something". In C you're defining the type "struct Something". In C++ you're actually defining the type "Something"
	- You can add constructors, a destructor and other methods in them, and this will not change the "bunch of contiguous bytes" that the
	  struct will represent in memory (unlike adding a pointer to a function inside a C struct)

	Considering the following code : (assuming byte alignment)

	struct SomePacket
	{
		//default constructor
		SomePacket() : type{ MyEnumeratedType } { //Code for constructor here }
		//constructor that will populate members
		SomePacket(char* str, int integer) : type{ MyEnumeratedType }, someInt{ integer } { strcpy(someString, str); }

		unsigned char type; //1 byte
		char someString[4]; //4 bytes
		int someInt;		//4 bytes
	};

	SomePacket packet;

	Will put this in memory :

	-------------------------------------------------------------------------
	|	0	|	1	|	2	|	3	|	4	|	5	|	6	|	7	|	8	|
	| type	|			someString			|			someInt				|
	-------------------------------------------------------------------------

	Theses structs are enclosed by #pragma pack(push/pop) directives, to force the compiler to "pack" them with a byte alignment. If we don't do that,
	compilers tend to add padding inside structures to make them more "runtime access friendly", aligning the memory in chunks or 4 bytes for example.
	This would wast 3 bytes between the "type" and "someString" members, making our packet grow. If this was just in ram, this will not be a problem,
	but we're sending this data over the network, so if we prevent to waste bandwidth...

	*/

	enum ID_PST4_MESSAGE_TYPE
	{
		//TODO define RakNet messages here
		ID_PST4_MESSAGE_ECHO = ID_USER_PACKET_ENUM + 1,				//Send a buffer of 255 chars
		ID_PST4_MESSAGE_HEAD_POSE = ID_USER_PACKET_ENUM + 2,		//Send the pose of the head
		ID_PST4_MESSAGE_HAND_POSE = ID_USER_PACKET_ENUM + 3,
		ID_PST4_MESSAGE_SPEEX_BUFFER = ID_USER_PACKET_ENUM + 4,
		ID_PST4_MESSAGE_SESSION_ID = ID_USER_PACKET_ENUM + 5,		//Sends the session ID to the assignee client. If the client send this packet, server will send the session id again.

		ID_PST4_MESSAGE_HEARTBEAT = ID_USER_PACKET_ENUM + 10		//1byte empty packet. Signal that you are alive.
	};

#pragma pack(push, 1) //align on 1 byte, instead of 4 by default. Thankfully this also works on GCC -> https://gcc.gnu.org/onlinedocs/gcc-4.4.4/gcc/Structure_002dPacking-Pragmas.html

	//Example of packet : a simple echo
	///This packet transport a simple string of text (255 chars) as a payload;
	struct echoPacket
	{
		echoPacket() : type{ ID_PST4_MESSAGE_ECHO } {}	//Default constructor, will always put the type at the right value
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

	///Heartbeat. Basically what it says
	struct heartbeatPacket
	{
		heartbeatPacket() : type{ ID_PST4_MESSAGE_HEARTBEAT } {}

		unsigned char type;
	};

	///Transmit the session ID
	struct serverToClientIdPacket
	{
		serverToClientIdPacket(size_t id) : type{ ID_PST4_MESSAGE_SESSION_ID }, clientId{ id } {}
		unsigned char type;
		size_t clientId;
	};

	///Internal Vector3 type, to represent an absolute position in the world, takes 6 bytes
	struct Vect3f
	{
		Vect3f() = default;
		Vect3f(float _x, float _y, float _z) : x{ _x }, y{ _y }, z{ _z } {}
#ifdef I_AM_CLIENT
		Annwvyn::AnnVect3 getAnnVect3() const { return{ x, y, z }; }
#endif
		float x;
		float y;
		float z;
	};

	///Internal Quaternion type, to represent an absolute orientation, takes 8 bytes
	struct Quatf
	{
		Quatf() = default;
		Quatf(float _w, float _x, float _y, float _z) : w{ _w }, x{ _x }, y{ _y }, z{ _z } {}
#ifdef I_AM_CLIENT
		Annwvyn::AnnQuaternion getAnnQuaternion() const { return{ w, x, y, z }; }
#endif
		float x;
		float y;
		float z;
		float w;
	};

	///Operator overload to write Vect3f to an output stream
	static std::ostream& operator<<(std::ostream& stream, const Vect3f& vector)
	{
		stream << "Vect3f(" << vector.x << ", " << vector.y << ", " << vector.z << ")";
		return stream;
	}

	///Operator overload to write a Quatf to an output stream
	static std::ostream& operator<<(std::ostream& stream, const Quatf& quat)
	{
		stream << "Quatf(" << quat.w << ", " << quat.w << ", " << quat.y << ", " << quat.x << ")";
		return stream;
	}

	///Pose of the head of an user
	struct headPosePacket
	{
		headPosePacket() : type{ ID_PST4_MESSAGE_HEAD_POSE } {};
		headPosePacket(size_t sessionId, Vect3f position, Quatf orientation) : type{ ID_PST4_MESSAGE_HEAD_POSE },
			sessionId{ sessionId }, absPos{ position }, absOrient{ orientation } {}

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

#pragma pack(pop)///undo the configuration change we did earlier. ;-)
}