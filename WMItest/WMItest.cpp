// WMItest.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"

#define _WIN32_DCOM
#define UNICODE
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "credui.lib")
#pragma comment(lib, "comsuppw.lib")
#include <wincred.h>
#include <strsafe.h>

int __cdecl main(int argc, char **argv)
{
	HRESULT hres;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		cout << "Failed to initialize COM library. Error code = 0x"
			<< hex << hres << endl;
		return 1;                  // Program has failed.
	}

	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------

	hres = CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
		RPC_C_IMP_LEVEL_IDENTIFY,    // Default Impersonation  
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities 
		NULL                         // Reserved
	);


	if (FAILED(hres))
	{
		cout << "Failed to initialize security. Error code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return 1;                    // Program has failed.
	}

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------

	IWbemLocator *pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres))
	{
		cout << "Failed to create IWbemLocator object."
			<< " Err code = 0x"
			<< hex << hres << endl;
		CoUninitialize();
		return 1;                 // Program has failed.
	}

	// Step 4: -----------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method

	IWbemServices *pSvc = NULL;

	// Get the user name and password for the remote computer
	CREDUI_INFO cui;
	bool useToken = false;
	bool useNTLM = true;
	wchar_t pszName[CREDUI_MAX_USERNAME_LENGTH + 1] = L"DOMAINPRIME\\Kartashov";
	wchar_t pszPwd[CREDUI_MAX_PASSWORD_LENGTH + 1] = L"hiii3";
	wchar_t pszDomain[CREDUI_MAX_USERNAME_LENGTH + 1];
	wchar_t pszUserName[CREDUI_MAX_USERNAME_LENGTH + 1];
	wchar_t pszAuthority[CREDUI_MAX_USERNAME_LENGTH + 1];
	BOOL fSave;
	DWORD dwErr;

	memset(&cui, 0, sizeof(CREDUI_INFO));
	cui.cbSize = sizeof(CREDUI_INFO);
	cui.hwndParent = NULL;
	// Ensure that MessageText and CaptionText identify
	// what credentials to use and which application requires them.
	/*cui.pszMessageText = TEXT("Press cancel to use process token");
	cui.pszCaptionText = TEXT("Enter Account Information");
	cui.hbmBanner = NULL;
	fSave = FALSE;

	dwErr = CredUIPromptForCredentials(
		&cui,                             // CREDUI_INFO structure
		TEXT(""),                         // Target for credentials
		NULL,                             // Reserved
		0,                                // Reason
		pszName,                          // User name
		CREDUI_MAX_USERNAME_LENGTH + 1,     // Max number for user name
		pszPwd,                           // Password
		CREDUI_MAX_PASSWORD_LENGTH + 1,     // Max number for password
		&fSave,                           // State of save check box
		CREDUI_FLAGS_GENERIC_CREDENTIALS |// flags
		CREDUI_FLAGS_ALWAYS_SHOW_UI |
		CREDUI_FLAGS_DO_NOT_PERSIST);

	if (dwErr == ERROR_CANCELLED)
	{
		useToken = true;
	}
	else if (dwErr)
	{
		cout << "Did not get credentials " << dwErr << endl;
		pLoc->Release();
		CoUninitialize();
		return 1;
	}*/

	// change the computerName strings below to the full computer name
	// of the remote computer
	if (!useNTLM)
	{
		StringCchPrintf(pszAuthority, CREDUI_MAX_USERNAME_LENGTH + 1, L"kERBEROS:%s", L"domainprime\\oit315n111");
	}

	// Connect to the remote root\cimv2 namespace
	// and obtain pointer pSvc to make IWbemServices calls.
	//---------------------------------------------------------

	hres = pLoc->ConnectServer(
		_bstr_t(L"\\\\oit315n111\\root\\cimv2"),
		_bstr_t(useToken ? NULL : pszName),    // User name
		_bstr_t(useToken ? NULL : pszPwd),     // User password
		NULL,                              // Locale             
		NULL,                              // Security flags
		_bstr_t(useNTLM ? NULL : pszAuthority),// Authority        
		NULL,                              // Context object 
		&pSvc                              // IWbemServices proxy
	);

	if (FAILED(hres))
	{
		cout << "Could not connect. Error code = 0x"
			<< hex << hres << endl;
		pLoc->Release();
		CoUninitialize();
		system("pause");
		return 1;                // Program has failed.
	}

	cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


	// step 5: --------------------------------------------------
	// Create COAUTHIDENTITY that can be used for setting security on proxy

	COAUTHIDENTITY *userAcct = NULL;
	COAUTHIDENTITY authIdent;

	if (!useToken)
	{
		memset(&authIdent, 0, sizeof(COAUTHIDENTITY));
		authIdent.PasswordLength = wcslen(pszPwd);
		authIdent.Password = (USHORT*)pszPwd;

		LPWSTR slash = wcschr(pszName, L'\\');
		if (slash == NULL)
		{
			cout << "Could not create Auth identity. No domain specified\n";
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return 1;               // Program has failed.
		}

		StringCchCopy(pszUserName, CREDUI_MAX_USERNAME_LENGTH + 1, slash + 1);
		authIdent.User = (USHORT*)pszUserName;
		authIdent.UserLength = wcslen(pszUserName);

		StringCchCopyN(pszDomain, CREDUI_MAX_USERNAME_LENGTH + 1, pszName, slash - pszName);
		authIdent.Domain = (USHORT*)pszDomain;
		authIdent.DomainLength = slash - pszName;
		authIdent.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

		userAcct = &authIdent;

	}

	// Step 6: --------------------------------------------------
	// Set security levels on a WMI connection ------------------

	hres = CoSetProxyBlanket(
		pSvc,                           // Indicates the proxy to set
		RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
		COLE_DEFAULT_PRINCIPAL,         // Server principal name 
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
		userAcct,                       // client identity
		EOAC_NONE                       // proxy capabilities 
	);

	if (FAILED(hres))
	{
		cout << "Could not set proxy blanket. Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 7: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

	// For example, get the name of the operating system
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("Select * from Win32_Process"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		cout << "Query for operating system name failed."
			<< " Error code = 0x"
			<< hex << hres << endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 8: -------------------------------------------------
	// Secure the enumerator proxy
	hres = CoSetProxyBlanket(
		pEnumerator,                    // Indicates the proxy to set
		RPC_C_AUTHN_DEFAULT,            // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_DEFAULT,            // RPC_C_AUTHZ_xxx
		COLE_DEFAULT_PRINCIPAL,         // Server principal name 
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE,    // RPC_C_IMP_LEVEL_xxx
		userAcct,                       // client identity
		EOAC_NONE                       // proxy capabilities 
	);

	if (FAILED(hres))
	{
		cout << "Could not set proxy blanket on enumerator. Error code = 0x"
			<< hex << hres << endl;
		pEnumerator->Release();
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// When you have finished using the credentials,
	// erase them from memory.
	SecureZeroMemory(pszName, sizeof(pszName));
	SecureZeroMemory(pszPwd, sizeof(pszPwd));
	SecureZeroMemory(pszUserName, sizeof(pszUserName));
	SecureZeroMemory(pszDomain, sizeof(pszDomain));


	// Step 9: -------------------------------------------------
	// Get the data from the query in step 7 -------------------

	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		// Get the value of the Name property
		hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
		wcout << " Name : " << vtProp.bstrVal;

		// Get the value of the FreePhysicalMemory property
		hr = pclsObj->Get(L"ProcessId",
			0, &vtProp, 0, 0);
		wcout << " ID: "
			<< vtProp.uintVal;

		if (vtProp.uintVal != 4 && vtProp.uintVal != 0) {
			hr = pclsObj->Get(L"ExecutablePath", 0, &vtProp, 0, 0);
			wcout << " Path : " << vtProp.bstrVal;
		}
		wcout << endl;

		VariantClear(&vtProp);

		pclsObj->Release();
		pclsObj = NULL;
	}



	BSTR MethodName = SysAllocString(L"Create");
	BSTR ClassName = SysAllocString(L"Win32_Process");

	IWbemClassObject* pClass = NULL;
	hres = pSvc->GetObject(ClassName, 0, NULL, &pClass, NULL);

	IWbemClassObject* pInParamsDefinition = NULL;
	hres = pClass->GetMethod(MethodName, 0,
		&pInParamsDefinition, NULL);

	IWbemClassObject* pClassInstance = NULL;
	hres = pInParamsDefinition->SpawnInstance(0, &pClassInstance);

	// Create the values for the in parameters
	VARIANT varCommand;
	varCommand.vt = VT_BSTR;
	varCommand.bstrVal = _bstr_t(L"notepad.exe");

	// Store the value for the in parameters
	//hres = pClassInstance->Put(L"CommandLine", 0,
	//	&varCommand, 0);
	wprintf(L"The command is: %s\n", V_BSTR(&varCommand));

	// Execute Method
	IWbemClassObject* pOutParams = NULL;
	hres = pSvc->ExecMethod(ClassName, MethodName, 0,
		NULL, pClassInstance, &pOutParams, NULL);

	if (FAILED(hres))
	{
		cout << "Could not execute method. Error code = 0x"
			<< hex << hres << endl;
		VariantClear(&varCommand);
		SysFreeString(ClassName);
		SysFreeString(MethodName);
		pClass->Release();
		pClassInstance->Release();
		pInParamsDefinition->Release();
		pOutParams->Release();
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// To see what the method returned,
	// use the following code.  The return value will
	// be in &varReturnValue
	VARIANT varReturnValue;
	hres = pOutParams->Get(_bstr_t(L"ReturnValue"), 0,
		&varReturnValue, NULL, 0);

	
	
	MethodName = SysAllocString(L"Terminate");
	ClassName = SysAllocString(L"Win32_Process");

	BSTR ClassNameInstance = SysAllocString(
		L"Win32_Process.Handle=\"2744\"");

	
	pClass = NULL;
	hres = pSvc->GetObject(ClassName, 0, NULL, &pClass, NULL);

	pInParamsDefinition = NULL;
	hres = pClass->GetMethod(MethodName, 0,
		&pInParamsDefinition, NULL);

	pClassInstance = NULL;
	hres = pInParamsDefinition->SpawnInstance(0, &pClassInstance);

	// Create the values for the in parameters

	// Execute Method
	pOutParams = NULL;
	hres = pSvc->ExecMethod(ClassNameInstance, MethodName, 0,
		NULL, pClassInstance, &pOutParams, NULL);


	if (FAILED(hres))
	{
		cout << "Could not execute method. Error code = 0x" << hex << hres << endl;
		cout << _com_error(hres).ErrorMessage() << endl;
		SysFreeString(ClassName);
		SysFreeString(MethodName);
		if (pClass) pClass->Release();
		if (pInParamsDefinition) pInParamsDefinition->Release();
		if (pOutParams) pOutParams->Release();
		if (pSvc) pSvc->Release();
		if (pLoc) pLoc->Release();
		CoUninitialize();
		cout << "press enter to exit" << endl;
		cin.get();
		return 1;               // Program has failed.
	}


	hres = pOutParams->Get(L"ReturnValue", 0, &varReturnValue, NULL, 0);
	if (!FAILED(hres))
		wcout << "ReturnValue " << varReturnValue.intVal << endl;
	VariantClear(&varReturnValue);



	// Cleanup
	// ========

	VariantClear(&varCommand);
	SysFreeString(ClassName);
	SysFreeString(MethodName);
	pClass->Release();
	pClassInstance->Release();
	pInParamsDefinition->Release();
	pOutParams->Release();


	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	if (pclsObj)
	{
		pclsObj->Release();
	}

	CoUninitialize();

	system("pause");

	return 0;   // Program successfully completed.

}

