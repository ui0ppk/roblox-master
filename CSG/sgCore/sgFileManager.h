#ifndef  __sgFileManager__
#define  __sgFileManager__


namespace sgFileManager
{
	sgCore_API bool Save(const sgCScene* scen, const char* file_name, 
							const void* userData, unsigned long userDataSize);

#define   SG_FILE_SIGNATURE      "SGSc"

	typedef struct 
	{
		char            signature[5];    //  SG_FILE_SIGNATURE
		int             major_ver;
		int             minor_ver;
        unsigned long   userBlockSize;
	} SG_FILE_HEADER;
    
	sgCore_API bool GetFileHeader(const char* file_name, SG_FILE_HEADER& file_header);
	sgCore_API bool GetUserData(const char* file_name, void* usetData);

	sgCore_API bool Open(const sgCScene* scen, const char* file_name);


	sgCore_API bool ExportDXF(const sgCScene* scen, const char* file_name);  
	sgCore_API bool ImportDXF(const sgCScene* scen, const char* file_name);  

	sgCore_API bool ExportSTL(const sgCScene* scen, const char* file_name);  
	sgCore_API bool ImportSTL(const sgCScene* scen, const char* file_name, bool  solids_checking=false);

	sgCore_API const void*  ObjectToBitArray(const sgCObject* obj, unsigned long& arrSize);
	sgCore_API sgCObject*   BitArrayToObject(const void* bitArray, unsigned long arrSize);

	sgCore_API sgCObject*   ObjectFromTriangles(const SG_VERT* vertexes, long vert_count,
												const SG_INDEX_TRIANGLE* triangles, long tr_count,
												float smooth_angle_in_radians=30.0*3.14159265/180.0,
												bool  solids_checking=false);
};

#endif