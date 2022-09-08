// dllmain.h : Declaration of module class.

class CRobloxProxyModule : public CAtlDllModuleT< CRobloxProxyModule >
{
public :
	DECLARE_LIBID(LIBID_RobloxProxyLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_ROBLOXPROXY, "{664B192B-D17A-4921-ABF9-C6F6264E5110}")
};

extern class CRobloxProxyModule _AtlModule;
