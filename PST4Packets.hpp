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
//#include <ws2def.h>
#include <stdexcept>
#include <exception>

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

	/*
	Structure of any RakNet packet :

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

	/*
	Protocol documentation:

	//This is as simple as it can get, and most of the magic is done by the server program, that is less than

	All packet types can been sent from the client to the server and form the server to the client.

	Packet that needs identification contains the sessionID of the user

	1) Client(s) connect to server
	2) Server send each client it's own "seesionID" in a ID_PST4_MESSAGE_SESSION_ID packet
	3) If client send a ID_PST4_MISSION_SESSION_ID packet to the server, server will resend the client it's session number
	4) When client has it's own session ID it will send continuously :
		- ID_PST4_MESSAGE_HEAD_POSE
		- ID_PST4_MESSAGE_HAND_POSE
		- ID_PST4_MESSAGE_VOICE_BUFFER
	5) When server receive one of the 3 packet above, it will resend it to ALL connected client
	6) Server will send, with an ACK request, an heartbeat packet every 5 seconds. Sever will keep track of this interval
	7) At clean session end, ID_PST4_MESSAGE_NOTIFY_SESSION_END is broadcasted by the server
	8) If server lost connection with a client, it will also broadcast ID_PST4_MESSAGE_NOTIFY_SESSION_END
	9) Client can send arbitrary text to the server in a ID_PST4_MESSAGE_ECHO packet. This is used for debugging.
	 */

	enum ID_PST4_MESSAGE_TYPE
	{
		//TODO define RakNet messages here
		ID_PST4_MESSAGE_ECHO = ID_USER_PACKET_ENUM + 1,					//Send a buffer of 255 chars
		ID_PST4_MESSAGE_HEAD_POSE = ID_USER_PACKET_ENUM + 2,			//Send the pose of the head
		ID_PST4_MESSAGE_HAND_POSE = ID_USER_PACKET_ENUM + 3,			//Send the pose of the hands
		ID_PST4_MESSAGE_VOICE_BUFFER = ID_USER_PACKET_ENUM + 4,			//Send a compressed audio buffer
		ID_PST4_MESSAGE_SESSION_ID = ID_USER_PACKET_ENUM + 5,			//Sends the session ID to the assignee client. Or client request to resend sessionID
		ID_PST4_MESSAGE_NOTIFY_SESSION_END = ID_USER_PACKET_ENUM + 6,	//Tell client that other client session has ended
		ID_PST4_MESSAGE_DYNAMIC_SCENE_OBJECT = ID_USER_PACKET_ENUM + 7, //Tell that an object exist in the scene

		ID_PST4_MESSAGE_HEARTBEAT = ID_USER_PACKET_ENUM + 10			//1byte empty packet. Signal that you are alive.
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
		Vect3f(const Annwvyn::AnnVect3& v) : x(v.x), y(v.y), z(v.z) {}
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
		Quatf(const Annwvyn::AnnQuaternion& q) : x(q.x), y(q.y), z(q.z), w(q.w) {}
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
		stream << "Quatf(" << quat.w << ", " << quat.x << ", " << quat.y << ", " << quat.z << ")";
		return stream;
	}

	///Pose of the head of an user
	struct headPosePacket
	{
		headPosePacket() : type{ ID_PST4_MESSAGE_HEAD_POSE }, sessionId{ 0 } {};
		headPosePacket(size_t sessionId, Vect3f position, Quatf orientation) : type{ ID_PST4_MESSAGE_HEAD_POSE },
			sessionId{ sessionId }, absPos{ position }, absOrient{ orientation } {}

#ifdef I_AM_CLIENT
		headPosePacket(size_t sessionId, Annwvyn::AnnVect3 position, Annwvyn::AnnQuaternion orientation) : type{ ID_PST4_MESSAGE_HEAD_POSE }, sessionId{ sessionId }
		{
			absPos = position;
			absOrient = orientation;
		}
#endif

		unsigned char type;
		size_t sessionId;
		Vect3f absPos;
		Quatf absOrient;
	};

	struct sessionEndedPacket
	{
		sessionEndedPacket(size_t id) : type{ ID_PST4_MESSAGE_NOTIFY_SESSION_END }, sessionId{ id } {}
		unsigned char type;
		size_t sessionId;
	};

	struct handPosePacket
	{
		handPosePacket(size_t session, bool state) : type{ ID_PST4_MESSAGE_HAND_POSE }, hasHands(state), sessionId(session){}
		handPosePacket(size_t session, Vect3f leftV, Quatf leftQ, Vect3f rightP, Quatf rightQ) : type{ ID_PST4_MESSAGE_HAND_POSE },
			hasHands(true), sessionId(session), leftPos(leftV), rightPos(rightP), leftOrient(leftQ), rightOrient(rightQ)
		{}

		unsigned char type;
		bool hasHands; //if false, the user don't have tracked hands
		size_t sessionId;
		Vect3f leftPos, rightPos;
		Quatf leftOrient, rightOrient;
	};

	//4 frames
	//This packet is actually sent as a bitstream and that stuct is not used
	struct voicePacket
	{
		voicePacket(size_t session) : type{ ID_PST4_MESSAGE_VOICE_BUFFER }, sessionId{ session }
		{
			dataLen = 0;
		}
		unsigned char type;
		size_t sessionId;
		unsigned char frameSizes[4]; //I expect {38, 38, 38, 38}
		unsigned char dataLen; //MAX 255
		unsigned char data[38 * 4]; //WILL BE TRUNCATED!!!
	};

	struct dynamicSceneObjectPacket
	{
		dynamicSceneObjectPacket(const std::string& id) :
			type(ID_PST4_MESSAGE_DYNAMIC_SCENE_OBJECT), owner{0}
		{
			setId(id);
		}

		dynamicSceneObjectPacket(const std::string& id, const Vect3f& pos, const Vect3f& s, const Quatf orient) :
			type(ID_PST4_MESSAGE_DYNAMIC_SCENE_OBJECT),
			position{pos}, scale{s}, orientation{orient}, owner{0}
		{
			setId(id);
		}
		
		void setId(const std::string& id)
		{
			if (id.length() < 255)
				secure_strcpy(idstring, sizeof idstring, id.c_str());
			else
				throw std::runtime_error("ID name too long");
		}

		bool isOwned()
		{
			return owner == 0;
		}

		void setOwner(const size_t& id)
		{
			owner = id;
		}

		unsigned char type;
		char idstring[256];
		size_t owner;
		Vect3f position, scale;
		Quatf orientation;
	};

#pragma pack(pop)///undo the configuration change we did earlier. ;-)
}