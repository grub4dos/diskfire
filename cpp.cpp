#include <windows.h>
#include <winnls.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>

extern "C" HRESULT CppCreateShortcut(WCHAR* pTarget, WCHAR* pLnkPath, WCHAR* pParam, WCHAR* pIcon, INT id, INT sw)
{
	// Create shortcut
	IShellLink* pLink;
	HRESULT hResult;
	hResult = CoInitialize(NULL);
	hResult = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (PVOID*)&pLink);
	if (SUCCEEDED(hResult))
	{
		IPersistFile* pFile;
		hResult = pLink->SetShowCmd(sw);
		hResult = pLink->SetPath(pTarget);
		if (pParam)
			hResult = pLink->SetArguments(pParam);
		if (pIcon)
			hResult = pLink->SetIconLocation(pIcon, id);
		hResult = pLink->QueryInterface(IID_IPersistFile, (PVOID*)&pFile);
		if (SUCCEEDED(hResult))
		{
			hResult = pFile->Save(pLnkPath, FALSE);
			pFile->Release();
		}
		pLink->Release();
	}
	CoUninitialize();
	return hResult;
}
