#include <Windows.h>
#include <ShObjIdl.h>
#include <wchar.h>
#include <stdio.h>
#include "stretchy_buffer.h"

#define BufferSize 1024

#define IDM_FILE_NEW 1
#define IDM_FILE_OPEN 2
#define IDM_FILE_QUIT 3

struct data
{
	float X;
	float Y;
	char Type[128];
};

struct CSVData
{
	data *Data; //NOTE: This is using stb stretchy buffer.
};

int
CountCommas(char *String)
{
	int Count = 0;
	while (*String++)
	{
		if (*String == ',')
		{
			++Count;
		}
	}
	return (Count);
}

CSVData
Win32LoadCSV(wchar_t *FilePath)
{
	//TODO: Validate that its a .csv file thats being loaded.

	//TODO: Read CSV file into some structure
	CSVData Result = {};
	
	FILE *File;

	_wfopen_s(&File, FilePath, L"r");
	
	char Buffer[BufferSize];

	if (File != NULL)
	{
		// Check ammount of commas
		fgets(Buffer, sizeof(Buffer), File);
		int Values = CountCommas(Buffer) + 1;

		fseek(File, 0, SEEK_SET);

		// Read data untill end of file.
		if (Values == 3)
		{
			data *Arr = NULL;
			while (1)
			{
				data Data = {};

				fscanf_s(File, "%f,%f,%s\n", &Data.X, &Data.Y, Data.Type, _countof(Data.Type));
				if (ferror(File) || feof(File))
				{
					break;
				}

				sb_push(Arr, Data);
			}

			Result.Data = Arr;
		}
	}

	return (Result);
}

void
Win32CreateMenu(HWND Window)
{
	HMENU MenuBar;
	HMENU Menu;

	MenuBar = CreateMenu();
	Menu = CreateMenu();

	AppendMenuW(Menu, MF_STRING, IDM_FILE_NEW, L"&New");
	AppendMenuW(Menu, MF_STRING, IDM_FILE_OPEN, L"&Open");
	AppendMenuW(Menu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(Menu, MF_STRING, IDM_FILE_QUIT, L"&Quit");

	AppendMenuW(MenuBar, MF_POPUP, (UINT_PTR)Menu, L"&File");
	SetMenu(Window, MenuBar);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_CREATE:
		{
			Win32CreateMenu(Window);
		} break;

		case WM_SIZE:
		{
		} break;

		case WM_CLOSE:
		{
			PostQuitMessage(0);
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
		} break;

		case WM_COMMAND:
		{
			switch (LOWORD(WParam))
			{
				case IDM_FILE_NEW:
				{
					//TODO: This menu should be removed.
				}break;
				case IDM_FILE_OPEN:
				{
					//TODO: Call function that loads csv etc.

					HRESULT Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
					if (SUCCEEDED(Result))
					{
						IFileOpenDialog *OpenDialog;

						Result = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (LPVOID *)&OpenDialog);

						if (SUCCEEDED(Result))
						{

							Result = OpenDialog->Show(NULL);

							// Retrieve filename from the dialog box.
							if (SUCCEEDED(Result))
							{
								IShellItem *Item;
								Result = OpenDialog->GetResult(&Item);
								if (SUCCEEDED(Result))
								{
									LPWSTR FilePath;
									Result = Item->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);

									// TODO: This should be a function call to load the file instead
									if (SUCCEEDED(Result))
									{
										CSVData D = Win32LoadCSV(FilePath); //TODO: Now make this be displayed somehow in the applicaiton.
										CoTaskMemFree(FilePath);
									}

									Item->Release();
								}
							}

							OpenDialog->Release();
						}

						CoUninitialize();
					}

				} break;
				case IDM_FILE_QUIT:
				{
					SendMessage(Window, WM_CLOSE, 0, 0);
				} break;
			}
		} break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);

			//TODO: This is temp
			FillRect(DeviceContext, &Paint.rcPaint, (HBRUSH)(COLOR_WINDOW + 25));

			EndPaint(Window, &Paint);
		} break;

		default:
		{
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return (Result);
}

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmpLine, int nCmdShow)
{
	WNDCLASSA WindowClass = {};

	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; //TODO: Might need to change.
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "BrokenScatter";

	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "BrokenScatter", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
									  NULL, // TODO: MENU
									  hInstance, NULL);

		if (Window)
		{
			//TODO: Code in this scope is temporary.
			ShowWindow(Window, nCmdShow);

			MSG msg = {};
			while (GetMessage(&msg, NULL, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return (0);
}