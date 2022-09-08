

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Mon Jun 09 14:17:19 2014
 */
/* Compiler settings for .\RobloxProxy.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __RobloxProxy_i_h__
#define __RobloxProxy_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ILauncher_FWD_DEFINED__
#define __ILauncher_FWD_DEFINED__
typedef interface ILauncher ILauncher;
#endif 	/* __ILauncher_FWD_DEFINED__ */


#ifndef ___ILauncherEvents_FWD_DEFINED__
#define ___ILauncherEvents_FWD_DEFINED__
typedef interface _ILauncherEvents _ILauncherEvents;
#endif 	/* ___ILauncherEvents_FWD_DEFINED__ */


#ifndef __Launcher_FWD_DEFINED__
#define __Launcher_FWD_DEFINED__

#ifdef __cplusplus
typedef class Launcher Launcher;
#else
typedef struct Launcher Launcher;
#endif /* __cplusplus */

#endif 	/* __Launcher_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ILauncher_INTERFACE_DEFINED__
#define __ILauncher_INTERFACE_DEFINED__

/* interface ILauncher */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ILauncher;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("699F0898-B7BC-4DE5-AFEE-0EC38AD42240")
    ILauncher : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartGame( 
            /* [in] */ BSTR authenticationUrl,
            /* [in] */ BSTR script) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HelloWorld( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Hello( 
            /* [retval][out] */ BSTR *result) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InstallHost( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Update( void) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AuthenticationTicket( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsUpToDate( 
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PreStartGame( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsGameStarted( 
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetSilentModeEnabled( 
            /* [in] */ VARIANT_BOOL silentModeEnbled) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetKeyValue( 
            /* [in] */ BSTR key,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetKeyValue( 
            /* [in] */ BSTR key,
            /* [in] */ BSTR val) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DeleteKeyValue( 
            /* [in] */ BSTR key) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetStartInHiddenMode( 
            /* [in] */ VARIANT_BOOL hiddenModeEnabled) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnhideApp( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BringAppToFront( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ResetLaunchState( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetEditMode( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILauncherVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILauncher * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILauncher * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILauncher * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILauncher * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILauncher * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILauncher * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILauncher * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StartGame )( 
            ILauncher * This,
            /* [in] */ BSTR authenticationUrl,
            /* [in] */ BSTR script);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *HelloWorld )( 
            ILauncher * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Hello )( 
            ILauncher * This,
            /* [retval][out] */ BSTR *result);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InstallHost )( 
            ILauncher * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Update )( 
            ILauncher * This);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AuthenticationTicket )( 
            ILauncher * This,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsUpToDate )( 
            ILauncher * This,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *PreStartGame )( 
            ILauncher * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsGameStarted )( 
            ILauncher * This,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetSilentModeEnabled )( 
            ILauncher * This,
            /* [in] */ VARIANT_BOOL silentModeEnbled);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetKeyValue )( 
            ILauncher * This,
            /* [in] */ BSTR key,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetKeyValue )( 
            ILauncher * This,
            /* [in] */ BSTR key,
            /* [in] */ BSTR val);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteKeyValue )( 
            ILauncher * This,
            /* [in] */ BSTR key);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetStartInHiddenMode )( 
            ILauncher * This,
            /* [in] */ VARIANT_BOOL hiddenModeEnabled);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UnhideApp )( 
            ILauncher * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BringAppToFront )( 
            ILauncher * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ResetLaunchState )( 
            ILauncher * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetEditMode )( 
            ILauncher * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            ILauncher * This,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } ILauncherVtbl;

    interface ILauncher
    {
        CONST_VTBL struct ILauncherVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILauncher_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ILauncher_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ILauncher_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ILauncher_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ILauncher_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ILauncher_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ILauncher_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define ILauncher_StartGame(This,authenticationUrl,script)	\
    ( (This)->lpVtbl -> StartGame(This,authenticationUrl,script) ) 

#define ILauncher_HelloWorld(This)	\
    ( (This)->lpVtbl -> HelloWorld(This) ) 

#define ILauncher_Hello(This,result)	\
    ( (This)->lpVtbl -> Hello(This,result) ) 

#define ILauncher_get_InstallHost(This,pVal)	\
    ( (This)->lpVtbl -> get_InstallHost(This,pVal) ) 

#define ILauncher_Update(This)	\
    ( (This)->lpVtbl -> Update(This) ) 

#define ILauncher_put_AuthenticationTicket(This,newVal)	\
    ( (This)->lpVtbl -> put_AuthenticationTicket(This,newVal) ) 

#define ILauncher_get_IsUpToDate(This,result)	\
    ( (This)->lpVtbl -> get_IsUpToDate(This,result) ) 

#define ILauncher_PreStartGame(This)	\
    ( (This)->lpVtbl -> PreStartGame(This) ) 

#define ILauncher_get_IsGameStarted(This,result)	\
    ( (This)->lpVtbl -> get_IsGameStarted(This,result) ) 

#define ILauncher_SetSilentModeEnabled(This,silentModeEnbled)	\
    ( (This)->lpVtbl -> SetSilentModeEnabled(This,silentModeEnbled) ) 

#define ILauncher_GetKeyValue(This,key,pVal)	\
    ( (This)->lpVtbl -> GetKeyValue(This,key,pVal) ) 

#define ILauncher_SetKeyValue(This,key,val)	\
    ( (This)->lpVtbl -> SetKeyValue(This,key,val) ) 

#define ILauncher_DeleteKeyValue(This,key)	\
    ( (This)->lpVtbl -> DeleteKeyValue(This,key) ) 

#define ILauncher_SetStartInHiddenMode(This,hiddenModeEnabled)	\
    ( (This)->lpVtbl -> SetStartInHiddenMode(This,hiddenModeEnabled) ) 

#define ILauncher_UnhideApp(This)	\
    ( (This)->lpVtbl -> UnhideApp(This) ) 

#define ILauncher_BringAppToFront(This)	\
    ( (This)->lpVtbl -> BringAppToFront(This) ) 

#define ILauncher_ResetLaunchState(This)	\
    ( (This)->lpVtbl -> ResetLaunchState(This) ) 

#define ILauncher_SetEditMode(This)	\
    ( (This)->lpVtbl -> SetEditMode(This) ) 

#define ILauncher_get_Version(This,pVal)	\
    ( (This)->lpVtbl -> get_Version(This,pVal) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ILauncher_INTERFACE_DEFINED__ */



#ifndef __RobloxProxyLib_LIBRARY_DEFINED__
#define __RobloxProxyLib_LIBRARY_DEFINED__

/* library RobloxProxyLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_RobloxProxyLib;

#ifndef ___ILauncherEvents_DISPINTERFACE_DEFINED__
#define ___ILauncherEvents_DISPINTERFACE_DEFINED__

/* dispinterface _ILauncherEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__ILauncherEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("6E9600BE-5654-47F0-9A68-D6DC25FADC55")
    _ILauncherEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _ILauncherEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ILauncherEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ILauncherEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ILauncherEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _ILauncherEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _ILauncherEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _ILauncherEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _ILauncherEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _ILauncherEventsVtbl;

    interface _ILauncherEvents
    {
        CONST_VTBL struct _ILauncherEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _ILauncherEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _ILauncherEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _ILauncherEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _ILauncherEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _ILauncherEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _ILauncherEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _ILauncherEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___ILauncherEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Launcher;

#ifdef __cplusplus

class DECLSPEC_UUID("76D50904-6780-4c8b-8986-1A7EE0B1716D")
Launcher;
#endif
#endif /* __RobloxProxyLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


