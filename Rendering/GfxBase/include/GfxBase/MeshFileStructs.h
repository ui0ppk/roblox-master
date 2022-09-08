#pragma once


namespace RBX {

#pragma pack( push, 1)
	// nb: keep backward/forward compatibility by only appending to these structs. 
	// stride information will keep this working.
	struct FileMeshHeader
	{
		unsigned short cbSize;
		unsigned char cbVerticesStride;
		unsigned char cbFaceStride;
		// ---dword boundary-----
		unsigned int num_vertices;
		unsigned int num_faces;
	};

	struct FileMeshVertexNormalTexture3d
	{
		float vx,vy,vz;
		float nx,ny,nz;
		float tu,tv,tw;
	};

	struct FileMeshFace
	{
		unsigned int a;
		unsigned int b;
		unsigned int c;
	};

#pragma pack( pop )

}

