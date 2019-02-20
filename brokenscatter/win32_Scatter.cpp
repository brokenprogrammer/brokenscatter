#include <Windows.h>
#include <ShObjIdl.h>
#include <wchar.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include "stretchy_buffer.h"

/*
	Notes:
	Currently the application is functional enough to run the data sets provided in the assignment but there are some problems.
	With this software that I'll have to take my time and fix on a later date.

	The most important ones are that currently the values for the x and y axis are really janky when they are normalized. It is basically
	made so that it will work for the two data sets provided in the assignment which is really trash and needs to be fixed asap.

	The code is using a lot of "magic values" this is because everything had to be set in pixels and due to my time limit I couldn't invenstigate a better solution
	in time.

	There can currently only be 3 data types within the loaded CSV data. This is because I messed up when I was for no reason trying to convert normal char pointers to 
	wide char pointers. I don't know how I ended up even attempting that instead of just using normal char pointers. Might have been intellisense that recommended me a wide function
	and then I just thought it expected wide strings as paramenter. Its stupid non the less. 

	There is currently no validation that an actual CSV file is being opened which also has to be done.

	Rest of things to fix is marked as TODOs within the code.

	- Oskar Mendel
*/

#define BufferSize 1024
#define NumberBufferSize 128

#define IDM_FILE_NEW 1
#define IDM_FILE_OPEN 2
#define IDM_FILE_QUIT 3

#define XAXISLINEPADDING 3
#define LINESPACE 10

struct coordinate
{
	float X;
	float Y;
};

struct data
{
	float X;
	float Y;
	char Type[128];
};

struct CSVData
{
	data *Data; //NOTE: This is using stb stretchy buffer.
	char *Types[3]; //TODO: This is currently locked to 3 different values. Fix later.
	float MaxX;
	float MaxY;
	float MinX;
	float MinY;
};

//NOTE: Globals
static CSVData GlobalCSVData;
static COLORREF Colors[] = { RGB(255, 0, 0), RGB(0, 0, 255), RGB(0, 255, 0), RGB(255, 255, 0) };

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

void
DrawAxesAndLegend(HWND Window, HDC DeviceContext, RECT PlotRectangle, CSVData Data)
{
	int ScreenWidth = PlotRectangle.right - PlotRectangle.left;
	int ScreenHeight = PlotRectangle.bottom - PlotRectangle.top;

	HFONT Font = CreateFontW(15, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, 0, 0, L"Georgia");
	SelectObject(DeviceContext, Font);

	wchar_t NumberBuffer[NumberBufferSize];
	
	// Y axis line.
	MoveToEx(DeviceContext, PlotRectangle.left, PlotRectangle.bottom, NULL);
	LineTo(DeviceContext, PlotRectangle.left, PlotRectangle.top);

	// X axis line.
	MoveToEx(DeviceContext, PlotRectangle.left, PlotRectangle.bottom, NULL);
	LineTo(DeviceContext, PlotRectangle.right, PlotRectangle.bottom);

	// Y axis numbers.
	for (int x = PlotRectangle.bottom; x > PlotRectangle.top; x-= (PlotRectangle.bottom - PlotRectangle.top) / 10)
	{
		MoveToEx(DeviceContext, PlotRectangle.left - LINESPACE, x, NULL);
		LineTo(DeviceContext, PlotRectangle.left + LINESPACE, x);

		int ScreenY = (int)(GlobalCSVData.MinY + (GlobalCSVData.MaxY - GlobalCSVData.MinY) * (ScreenHeight - x)/ScreenHeight);
		swprintf_s(NumberBuffer, L"%i", ScreenY);
		TextOutW(DeviceContext, PlotRectangle.left - 30, x - 7, NumberBuffer, lstrlenW(NumberBuffer));
	}

	// X axis numbers.
	for (int x = 0; x < PlotRectangle.right; x += (PlotRectangle.right - PlotRectangle.left) / 10)
	{
		MoveToEx(DeviceContext, x + PlotRectangle.left, PlotRectangle.bottom - LINESPACE, NULL);
		LineTo(DeviceContext, x + PlotRectangle.left, PlotRectangle.bottom + LINESPACE);

		// Line number
		int ScreenX = (int)(GlobalCSVData.MinX + (GlobalCSVData.MaxX - GlobalCSVData.MinX) * x / ScreenWidth);
		
		swprintf_s(NumberBuffer, L"%i", ScreenX);
		TextOutW(DeviceContext, x-10 + PlotRectangle.left, PlotRectangle.bottom + 20, NumberBuffer, lstrlenW(NumberBuffer));
	}

	// Draw Legend
	RECT WindowRect;
	GetClientRect(Window, &WindowRect);

	for (int DataType = 0; DataType < 3; ++DataType)
	{
		char *String = GlobalCSVData.Types[DataType];
		TextOutA(DeviceContext, WindowRect.left+(40 * DataType), WindowRect.bottom-15, GlobalCSVData.Types[DataType], lstrlenA(GlobalCSVData.Types[DataType]));

		// Select color based on type
		HBRUSH ColoredBrush = CreateSolidBrush(Colors[DataType]);

		RECT Rectangle = {};
		Rectangle.top = (WindowRect.bottom - 10) - 5;
		Rectangle.bottom = (WindowRect.bottom - 10) + 5;
		Rectangle.left = (WindowRect.left + (40 * DataType) + 27) - 5;
		Rectangle.right = (WindowRect.left + (40 * DataType)+ 27) + 5;

		FillRect(DeviceContext, &Rectangle, ColoredBrush);
	}
}

CSVData
Win32LoadCSV(wchar_t *FilePath)
{
	//TODO: Validate that its a .csv file thats being loaded.

	//TODO: Read CSV file into some structure
	CSVData Result = {};

	Result.MaxX = FLT_MIN;
	Result.MaxY = FLT_MIN;
	Result.MinX = FLT_MAX;
	Result.MinY = FLT_MAX;
	FILE *File;
	int Types = 0;

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

				if (Data.X > Result.MaxX)
				{
					Result.MaxX = Data.X;
				}
				if (Data.X < Result.MinX)
				{
					Result.MinX = Data.X;
				}
				if (Data.Y > Result.MaxY)
				{
					Result.MaxY = Data.Y;
				}
				if (Data.Y < Result.MinY)
				{
					Result.MinY = Data.Y;
				}

				bool TypeFound = false;
				for (int TypeIndex = 0; TypeIndex < 3; ++TypeIndex)
				{
					if (Result.Types[TypeIndex])
					{
						if (strcmp(Result.Types[TypeIndex], Data.Type) == 0)
						{
							TypeFound = true;
							break;
						}
					}
				}

				if (!TypeFound)
				{
					Result.Types[Types] = (char *)malloc(strlen(Data.Type) + 1);
					strcpy_s(Result.Types[Types], sizeof(Result.Types[Types]), Data.Type);
					Types++;
				}

				stb_sb_push(Arr, Data);
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

									if (SUCCEEDED(Result))
									{
										//TODO: Free old stb allocated structures if they already exist.
										GlobalCSVData = Win32LoadCSV(FilePath); //TODO: Now make this be displayed somehow in the applicaiton.
										CoTaskMemFree(FilePath);
									}

									Item->Release();
								}
							}

							OpenDialog->Release();
						}

						CoUninitialize();

						// Redraw the window.
						RedrawWindow(Window, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
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
			RECT WindowRect;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			GetClientRect(Window, &WindowRect);

			//TODO: This is temp
			FillRect(DeviceContext, &Paint.rcPaint, (HBRUSH)(COLOR_WINDOW));
			
			RECT PlotRect = {};
			PlotRect.top = 50;
			PlotRect.left = 50;
			PlotRect.right = Paint.rcPaint.right - 50;
			PlotRect.bottom = Paint.rcPaint.bottom - 50;
			
			int ScreenWidth = PlotRect.right - PlotRect.left;
			int ScreenHeight = PlotRect.bottom - PlotRect.top;

			
			//Rectangle(DeviceContext, PlotRect.left, PlotRect.top, PlotRect.right, PlotRect.bottom);
			if (GlobalCSVData.Data)
			{

				GlobalCSVData.MaxX = ceilf(GlobalCSVData.MaxX);
				GlobalCSVData.MaxY = ceilf(GlobalCSVData.MaxY);

				GlobalCSVData.MinX = floorf(GlobalCSVData.MinX);
				GlobalCSVData.MinY = floorf(GlobalCSVData.MinY);

				//TODO: Fix a better value perhaps based on the difference between Min and Max values.
				if (GlobalCSVData.MaxX < 60.0f)
				{
					GlobalCSVData.MaxX = 100.0f;
				}
				if (GlobalCSVData.MaxY < 60.0f)
				{
					GlobalCSVData.MaxY = 100.0f;
				}
				if (GlobalCSVData.MinX < -50.0f)
				{
					GlobalCSVData.MinX = -100.0f;
				}
				if (GlobalCSVData.MinY < -50.0f)
				{
					GlobalCSVData.MinY = -100.0f;
				}

				// Draw the axes
				DrawAxesAndLegend(Window, DeviceContext, PlotRect, GlobalCSVData);

				for (int i = 0; i < stb_sb_count(GlobalCSVData.Data); ++i)
				{
					data Data = GlobalCSVData.Data[i];

					float ScreenX = (Data.X - GlobalCSVData.MinX) / (GlobalCSVData.MaxX - GlobalCSVData.MinX)*ScreenWidth;
					float ScreenY = ScreenHeight - (Data.Y - GlobalCSVData.MinY) / (GlobalCSVData.MaxY - GlobalCSVData.MinY)*ScreenHeight;

					ScreenX += 50;
					ScreenY += 50;

					RECT R = {};
					R.top = ScreenY - 5;
					R.bottom = ScreenY + 5;
					R.left = ScreenX - 5;
					R.right = ScreenX + 5;

					HBRUSH ColoredBrush = 0;

					for (int TypeIndex = 0; TypeIndex < 3; ++TypeIndex)
					{
						if (strcmp(GlobalCSVData.Types[TypeIndex], Data.Type) == 0)
						{
							ColoredBrush = CreateSolidBrush(Colors[TypeIndex]);
							break;
						}
					}

					FillRect(DeviceContext, &R, ColoredBrush);
				}
			}

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
		HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "BrokenScatter", (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME) | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL,
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