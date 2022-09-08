
#ifdef RBXTESTHOOKS_EXPORTS
#define RBXTESTHOOKS_API __declspec(dllexport)
#else
#define RBXTESTHOOKS_API __declspec(dllimport)
#endif

namespace RBX
{

	class IWorkspaceTestHooks
	{
	public:
		virtual HRESULT ExecScriptImpl(BSTR script, VARIANT arg1, VARIANT arg2, VARIANT arg3, VARIANT arg4, SAFEARRAY** result) = 0;
	};

}

RBXTESTHOOKS_API HRESULT IWorkspaceTestHooks_ExecScript(RBX::IWorkspaceTestHooks *p, BSTR script, VARIANT arg1, VARIANT arg2, VARIANT arg3, VARIANT arg4, SAFEARRAY** result);
