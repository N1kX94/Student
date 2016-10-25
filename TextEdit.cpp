#include <Windows.h>
#include <Shobjidl.h>

#pragma comment(lib,"ComCtl32.Lib")

#define MAX_FILESIZE 1000000 //1Mb
#define ID_ABOUT 102
#define ID_EDIT 103
#define ID_OPEN 104
#define ID_SAVE 105
#define ID_EXIT 110
#define ID_COPY 111
#define ID_PASTE 112

HWND hWnd; //корневое окно
HWND hEdit; //текстовое поле
HWND hStatusWindow;
bool bUpdate;
LPCSTR windowTitle = "Text Edit";

//запись в буфер обмена
void WriteToClipboard() {
	if (OpenClipboard(hWnd))//открываем буфер обмена
	{
		int charNum = GetWindowTextLength(hEdit);
		char *tempBuf = new char[charNum + 1];
		ZeroMemory(tempBuf, charNum + 1);
		DWORD beginPos = 0;
		DWORD endPos = 0;

		HGLOBAL hgBuffer;
		char* chBuffer;
		EmptyClipboard(); //очищаем буфер
		hgBuffer = GlobalAlloc(GMEM_DDESHARE, charNum + 1);//выделяем память
		chBuffer = (char*)GlobalLock(hgBuffer); //блокируем память
		GetWindowText(hEdit, tempBuf, charNum + 1);
		SendMessage(hEdit, EM_GETSEL, (WPARAM)&beginPos, (LPARAM)&endPos);
		//копирование из выделенной области
		if (beginPos != endPos) {
			memcpy(chBuffer, &tempBuf[beginPos], endPos - beginPos);
		}

		chBuffer[endPos - beginPos] = 0; //очищаем буфер

		delete[] tempBuf;
		GlobalUnlock(hgBuffer);//разблокируем память
		SetClipboardData(CF_TEXT, hgBuffer);//помещаем текст в буфер обмена
		CloseClipboard(); //закрываем буфер обмена
	}
}

//чтение из буфера обмена, область должна быть выделена заранее в которую будет помещаться текст из буфера обмена
void ReadFromClipboard() {
	if (OpenClipboard(hWnd))//открываем буфер обмена
	{
		HANDLE hData = GetClipboardData(CF_TEXT);//извлекаем текст из буфера обмена
		char* chBuffer = (char*)GlobalLock(hData);//блокируем память
		SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)chBuffer);
		GlobalUnlock(hData);//разблокируем память
		CloseClipboard();//закрываем буфер обмена
	}
}

//Диалог открытия файла
HANDLE OpenFile(HWND hWnd)
{
	IFileOpenDialog *pFileOpenDialog;

	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpenDialog));
	HANDLE hf;

	if (SUCCEEDED(hr))
	{
		COMDLG_FILTERSPEC rgSpec[] =
		{
			{ L"Text Files (*.txt)", L"*.txt" },
			{ L"All Files (*.*)", L"*.*" }
		};

		hr = pFileOpenDialog->SetFileTypes(2, rgSpec);

		if (SUCCEEDED(hr))
		{
			hr = pFileOpenDialog->Show(hWnd);

			if (SUCCEEDED(hr))
			{
				IShellItem *psiCurrent = 0;
				hr = pFileOpenDialog->GetResult(&psiCurrent);

				if (SUCCEEDED(hr))
				{
					LPWSTR pszPath = 0;
					hr = psiCurrent->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);

					if (SUCCEEDED(hr))
					{
						hf = CreateFileW(pszPath, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						CoTaskMemFree(pszPath);
						return hf;
					}

					psiCurrent->Release();
				}
			}
		}
	}

	return 0;
}
//функция открытия файла
void OpenFile() {
	if (bUpdate)
	{
		//Если были изменения в файле, или пытаететь открыть другой файл
		if (IDYES == MessageBox(hWnd, "Файл был изменен. Сохранить изменения?", windowTitle, MB_YESNO | MB_ICONQUESTION))
		{
			SendMessage(hWnd, BM_CLICK, 0, 0);

			// следующая строка - условие что сохранение файла не удалось и сразу же, если это так, - запрос: все равно продолжить или выйти из обработчика сообщения 
			if (bUpdate	&& IDNO == MessageBox(hWnd, "Старые данные не сохранены. Открыть файл в противном случае (потеряв данные)?", windowTitle, MB_YESNO | MB_ICONQUESTION))
				return;
		}

		bUpdate = FALSE;
	}

	HANDLE hf = OpenFile(hWnd);
	DWORD dwBytesToRead = 0;
	DWORD dwBytesRead = 0;
	DWORD dwCharsToRead = 0;

	if (hf == 0) {
		return;
	}

	if (hf == INVALID_HANDLE_VALUE)
		return;

	LARGE_INTEGER nFileSize;
	GetFileSizeEx(hf, &nFileSize);
	dwBytesToRead = nFileSize.LowPart;

	//Проверка размера файла, если файл больше чем 1МБ, то ошибка
	if (dwBytesToRead > MAX_FILESIZE)
	{
		CloseHandle(hf);
		MessageBox(hWnd, "Файл превышает максимальный размер.", windowTitle, MB_OK);
		return;
	}

	LPSTR lpMultiByteStr = new CHAR[dwBytesToRead + 1];

	if (!ReadFile(hf, lpMultiByteStr, dwBytesToRead, &dwBytesRead, NULL) || dwBytesRead == 0)
	{
		CloseHandle(hf);
		delete[]lpMultiByteStr;
		MessageBox(hWnd, "Произошла ошибка при чтении файла!", windowTitle, MB_OK | MB_ICONWARNING);
		return;
	}

	CloseHandle(hf);

	if (dwBytesRead > 0 && dwBytesRead <= dwBytesToRead)
	{
		lpMultiByteStr[dwBytesRead] = '\0';
	}
	else if (dwBytesRead == 0)
	{
		MessageBox(hWnd, "Нет данных в файле!", windowTitle, MB_OK | MB_ICONWARNING);
	}
	else
	{
		MessageBox(hWnd, "Произошла ошибка при чтении файла!", windowTitle, MB_OK | MB_ICONWARNING);
	}

	SetWindowText(hEdit, (LPCSTR)lpMultiByteStr);
	delete[]lpMultiByteStr;
	bUpdate = FALSE;
	return;
}
// Диалог сохранения файла
HANDLE saveFile(HWND hWnd) {
	HANDLE hf;
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));

	char *pfile = new char[256];
	ZeroMemory(pfile, 256);

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = 0;
	ofn.lpstrFilter = "(*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0L;
	ofn.nFilterIndex = 0L;
	ofn.lpstrFile = pfile;
	ofn.nMaxFile = 1000;
	ofn.lpstrFileTitle = 0;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = 0;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = "*.txt";
	ofn.lCustData = 0;
	ofn.lpstrTitle = "Select file";
	ofn.Flags = OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

	GetSaveFileName(&ofn);

	hf = CreateFile(pfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	delete[] pfile;
	return hf;
}
//функция сохранения файла
void OpenSaveFile() {
	DWORD dwCharsToWrite;
	DWORD dwBytesToWrite;
	DWORD dwBytesWritten;
	HANDLE hTxtBuf;
	PWCHAR npTextBuffer;
	HANDLE hf = saveFile(hWnd);

	if (hf == 0) {
		return;
	}

	if (hf == INVALID_HANDLE_VALUE)
		return;

	dwCharsToWrite = GetWindowTextLength(hEdit); //Узнаем длину строки

	hTxtBuf = (HANDLE)SendMessage(hEdit, EM_GETHANDLE, 0, 0);
	npTextBuffer = (PWCHAR)LocalLock(hTxtBuf);

	dwBytesToWrite = dwCharsToWrite;

	if (!WriteFile(hf, npTextBuffer, dwBytesToWrite, &dwBytesWritten, NULL) || dwBytesWritten != dwBytesToWrite) {
		MessageBoxA(0, "Обнаружена ошибка во время сохранения файла!", windowTitle, MB_OK | MB_ICONWARNING);
	}

	CloseHandle(hf);
	LocalUnlock(hTxtBuf);
	return;
}
//поток для подсчета символов и слов в тексте
DWORD WINAPI GetNumWordAndSymbols(CONST LPVOID lpParam)
{
	int charNum = 0;
	char *tempBuf = 0;
	char *posTemp = 0;
	int numWords = 0;
	char numWordsCh[100];
	char numSymCh[100];
	char result[200];

	while (1) {
		numWords = 0;
		tempBuf = 0;
		posTemp = 0;
		charNum = GetWindowTextLength(hEdit);
		tempBuf = new char[charNum + 1];

		ZeroMemory(tempBuf, charNum + 1);
		ZeroMemory(numWordsCh, 100);
		ZeroMemory(numSymCh, 100);
		ZeroMemory(result, 200);

		GetWindowText(hEdit, tempBuf, charNum + 1);

		_itoa_s(lstrlen(tempBuf), numSymCh, 100, 10);

		char * pch;
		pch = strtok_s(tempBuf, " ,.-\"!;:", &posTemp);
		while (pch != NULL)
		{
			pch = strtok_s(NULL, " ,.-\"!;:", &posTemp);
			numWords++;
		}

		_itoa_s(numWords, numWordsCh, 100, 10);

		strcat_s(result, 200, "symbols: ");
		strcat_s(result, 200, numSymCh);
		strcat_s(result, 200, "; words: ");
		strcat_s(result, 200, numWordsCh);

		SendMessage(hStatusWindow, SB_SETTEXT, 0, (LPARAM)result);

		delete[] tempBuf;

		Sleep(1000);
	}

	ExitThread(0);
}

//Обработчик событий
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_SIZE:
	{
		SendMessage(hStatusWindow, WM_SIZE, 0, 0);
		MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam)-20, TRUE);
		break;
	}
	case WM_COMMAND: //Обработчик кнопок, полей ввода и т.д.
	{
		wmEvent = HIWORD(wParam);

		switch (LOWORD(wParam))
		{
		case ID_OPEN:
		{
			OpenFile();
		}
		break;
		case ID_SAVE:
		{
			OpenSaveFile();
		}
		break;
		case ID_ABOUT:
		{
			MessageBox(hWnd, TEXT("Версия программы 0.0.1"), TEXT("О программе"), MB_OK);
		}
		break;
		case ID_EDIT:
		{
			if (wmEvent == EN_ERRSPACE)
				MessageBox(hWnd, "Слишком мало памяти!", "Text Edit", MB_OK);
			else if (wmEvent == EN_CHANGE)
				bUpdate = TRUE;

			break;
		}
		case ID_PASTE:
			ReadFromClipboard();
			break;
		case ID_COPY:
			WriteToClipboard();
			break;
		case ID_EXIT:
		{
			if (bUpdate)
			{
				//Если были изменения в файле, или пытаететь открыть другой файл
				if (IDYES == MessageBox(hWnd, "Файл был изменен. Сохранить изменения?", windowTitle, MB_YESNO | MB_ICONQUESTION))
				{
					SendMessage(hWnd, BM_CLICK, 0, 0);

					// следующая строка - условие что сохранение файла не удалось и сразу же, если это так, - запрос: все равно продолжить или выйти из обработчика сообщения 
					if (bUpdate	&& IDNO == MessageBox(hWnd, "Старые данные не сохранены. Открыть файл в противном случае (потеряв данные)?", windowTitle, MB_YESNO | MB_ICONQUESTION))
						break;
				}

				bUpdate = FALSE;
			}

			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		}
	}
	break;
	case WM_CREATE:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

//Точка входа в программу
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	//Создаем класс окна
	WNDCLASS WindowClass;

	//Заполняем структуру 
	WindowClass.style = 0;
	WindowClass.lpfnWndProc = (WNDPROC)WndProc;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = hInstance;
	WindowClass.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION);
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WindowClass.lpszMenuName = 0;
	WindowClass.lpszClassName = TEXT("Class");

	//Зарегистируем класс окна
	RegisterClass(&WindowClass);

	//Создаем переменную, в которой поместим идентификатор окна
	hWnd = CreateWindow("Class", "Text Edit", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, hInstance, NULL);

	//Создание кнопок

	//Создание Текст. поля
	hEdit = CreateWindow("Edit", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL | ES_LEFT | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE, 10, 10, 760, 500, hWnd, (HMENU)ID_EDIT, hInstance, NULL);

	hStatusWindow = CreateWindow(STATUSCLASSNAME, "",
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_BOTTOM,
		0, 0, 0, 0, hWnd, (HMENU)0, hInstance, NULL);

	//Создание меню
	HMENU main_menu = CreateMenu();
	HMENU menu_help = CreatePopupMenu();
	HMENU menu_edit = CreatePopupMenu();
	HMENU menu_view = CreatePopupMenu();
	//главное меню
	AppendMenu(main_menu, MF_STRING | MF_POPUP, (UINT)menu_view, "&Файл");
	AppendMenu(main_menu, MF_STRING | MF_POPUP, (UINT)menu_edit, "&Редактировать");
	AppendMenu(main_menu, MF_STRING | MF_POPUP, (UINT)menu_help, "&Справка");

	//подменю 
	AppendMenu(menu_view, MF_STRING, ID_OPEN, ("О&ткрыть"));
	AppendMenu(menu_view, MF_STRING, ID_SAVE, ("&Сохранить"));
	AppendMenu(menu_view, MF_STRING, ID_EXIT, ("&Выход"));
	AppendMenu(menu_edit, MF_SEPARATOR, NULL, "");
	AppendMenu(menu_edit, MF_STRING, ID_COPY, "&Копировать\tCtrl+c");
	AppendMenu(menu_edit, MF_STRING, ID_PASTE, "&Вставить\tCtrl+v");
	AppendMenu(menu_help, MF_STRING, ID_ABOUT, "&О программе");

	SetMenu(hWnd, main_menu);
	DestroyMenu(main_menu);

	//показать окно
	ShowWindow(hWnd, SW_SHOW); 
	InvalidateRect(hWnd, NULL, true);
	//обновить содержимое окна
	UpdateWindow(hWnd);

	//Создадим переменную для храненния сообщений
	MSG msg;

	CreateThread(NULL, 0, &GetNumWordAndSymbols, 0, 0, NULL);

	//Создадим цикл обработки сообщений
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
