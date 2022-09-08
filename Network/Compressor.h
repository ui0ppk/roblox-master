#pragma once

#include "Util/G3DCore.h"

namespace RakNet {
	class BitStream;
}

namespace RBX { 

namespace Network {

	class Compressor
	{
	public:
		typedef enum {UNCOMPRESSED = 0, RAKNET_COMPRESSED, HEAVILY_COMPRESSED} CompressionType;

	private:
		static bool canHeavilyCompressTranslation(const Vector3& translation);

		static void writeCompressionType(RakNet::BitStream& bitStream, CompressionType compressionType);
		static CompressionType readCompressionType(RakNet::BitStream& bitStream);

	public:
		static void writeTranslation(RakNet::BitStream& bitStream, const Vector3& translation, CompressionType compressionType);
		static void writeRotation(RakNet::BitStream& bitStream, const Matrix3& rotation, CompressionType compressionType);

		static void readTranslation(RakNet::BitStream& bitStream, Vector3& translation);
		static void readRotation(RakNet::BitStream& bitStream, Matrix3& rotation);
	};

}}