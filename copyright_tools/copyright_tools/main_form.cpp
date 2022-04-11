#include "framework.h"
#include "main_form.h"
#include "about_form.h"
#include <atlstr.h>

// 文件窗口
#include <shlobj.h>

// 迭代查找文件
#include <filesystem>
#include <fstream>
#include <set>

using namespace std;
using namespace ui;
namespace fs = std::filesystem;

const std::wstring MainForm::kClassName = L"Copyright";

CString BrowseForFolder(HWND hwnd, LPCWSTR title, LPARAM folder);
INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData);
std::map<std::wstring, int> findExtsInPath(std::wstring path);

MainForm::MainForm()
{
}

MainForm::~MainForm()
{
}

std::wstring MainForm::GetSkinFolder()
{
	return L"copyright";
}

std::wstring MainForm::GetSkinFile()
{
	return L"copyright.xml";
}

std::wstring MainForm::GetWindowClassName() const
{
	return kClassName;
}

ui::Control* MainForm::CreateControl(const std::wstring& pstrClass)
{
	ui::Control* control = nullptr;

	return control;
}

bool cmp(const pair<std::wstring, int>& p1, const pair<std::wstring, int>& p2) // 要用常数，不然编译错误 
{
	return p1.second > p2.second;
}

void MainForm::InitWindow()
{
	logEdit = static_cast<ui::RichEdit*>(FindControl(L"edit_log"));
	extEdit = static_cast<ui::RichEdit*>(FindControl(L"edit_ext"));
	// 源文件夹
	sourceBtn = static_cast<ui::Button*>(FindControl(L"btn_source"));
	sourceEdit = static_cast<ui::RichEdit*>(FindControl(L"edit_source"));

	sourceBtn->AttachClick([this](ui::EventArgs* msg) {
		CString folder = BrowseForFolder(NULL, L"选择一个文件夹", CSIDL_DESKTOP);
		std::wstring path = folder.GetString();
		sourceEdit->SetText(path);

		std::map<std::wstring, int> extmap = findExtsInPath(path);

		vector<pair<std::wstring, int> > arr;
		for (map<std::wstring, int>::iterator it = extmap.begin(); it != extmap.end(); ++it)
		{
			arr.push_back(make_pair(it->first, it->second));
		}
		sort(arr.begin(), arr.end(), cmp);

		extEdit->Clear();
		for (vector<pair<std::wstring, int> >::iterator it = arr.begin(); it != arr.end(); ++it)
		{
			extEdit->AppendText(nbase::StringPrintf(L"%s", it->first.c_str()));
			if (it != arr.end() - 1) extEdit->AppendText(L", ");
		}
		return true; 
	});

	// 目标文件夹
	targetBtn = static_cast<ui::Button*>(FindControl(L"btn_target"));
	targetEdit = static_cast<ui::RichEdit*>(FindControl(L"edit_target"));

	targetBtn->AttachClick([this](ui::EventArgs* msg) {
		CString folder = BrowseForFolder(NULL, L"选择一个文件夹", CSIDL_DESKTOP);
		targetEdit->SetText(folder.GetString());
		return true;
	});

	startBtn = static_cast<ui::Button*>(FindControl(L"btn_start"));
	startBtn->AttachClick([this](ui::EventArgs* msg) {
		generate();
		return true;
	});

	/* Show settings menu */
	ui::Button* settings = static_cast<ui::Button*>(FindControl(L"settings"));
	settings->AttachClick([this](ui::EventArgs* args) {
		RECT rect = args->pSender->GetPos();
		ui::CPoint point;
		point.x = rect.left - 175;
		point.y = rect.top + 10;
		::ClientToScreen(m_hWnd, &point);

		nim_comp::CMenuWnd* sub_menu = new nim_comp::CMenuWnd(NULL);
		ui::STRINGorID xml(L"settings_menu.xml");
		sub_menu->Init(xml, _T("xml"), point);

		/* About menu */
		nim_comp::CMenuElementUI* menu_about = static_cast<nim_comp::CMenuElementUI*>(sub_menu->FindControl(L"about"));
		menu_about->AttachClick([this](ui::EventArgs* args) {
			AboutForm* about_form = (AboutForm*)(nim_comp::WindowsManager::GetInstance()->GetWindow(AboutForm::kClassName, AboutForm::kClassName));
			if (!about_form)
			{
				about_form = new AboutForm();
				about_form->Create(GetHWND(), AboutForm::kClassName, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, 0);
				about_form->CenterWindow();
				about_form->ShowWindow();
			}
			else
			{
				about_form->ActiveWindow();
			}
			return true;
			});

		return true;
		});
}

void MainForm::generate()
{
	std::wstring sourcePath = sourceEdit->GetText();

	if (sourcePath.size() == 0) {
		MessageBox(NULL, L"请选择源代码文件夹", L"提示", NULL);
		return;
	}

	std::wstring targetPath = targetEdit->GetText();

	if (targetPath.size() == 0) {
		
		return;
	}

	std::wstring exts = extEdit->GetText();

	std::string targetFilePath = targetEdit->GetUTF8Text() + "\\code.txt";

	std::ofstream outfile;
	outfile.open(targetFilePath, std::ios_base::out|ios::trunc);
	writeLinesToFile(sourcePath, outfile, exts);
	outfile.close();
}

std::map<std::wstring, int> findExtsInPath(std::wstring path)
{
	std::map<std::wstring, int> extmap;
	for (const auto& file : fs::directory_iterator(path)) {
		if (file.is_directory()) {
			std::map<std::wstring, int> subexts = findExtsInPath(file.path());
			for (auto const& kv : subexts)
			{
				extmap[kv.first] = extmap[kv.first] + kv.second;
			}
		}
		else if (file.path().has_extension()) {
			std::wstring ext = file.path().extension().wstring();
			extmap[ext] = extmap[ext] + 1;
		}
	}
	return extmap;
}

void MainForm::writeLinesToFile(std::wstring path, std::ofstream &outfile, std::wstring exts)
{
	for (const auto& file : fs::directory_iterator(path)) {
		std::wstring ext = file.path().extension();
		if (file.is_directory()) {
			writeLinesToFile(file.path(), outfile, exts);
		}
		else if (ext.size() > 0 && exts.find(ext) != std::string::npos) {
			std::wstring filePath = file.path().wstring();
			logEdit->AppendText(filePath);
			logEdit->AppendText(L"\r\n");

			// 每个文件处理开始, 记录文件名
			std::string filename = file.path().filename().u8string();
			outfile << "// " + filename << endl << endl;

			std::ifstream file(filePath);
			std::string str;
			while (std::getline(file, str))
			{
				outfile << str << endl;
			}
			// 每个文件处理结束, 加一个空行
			outfile << endl;
		}
	}
}

CString BrowseForFolder(HWND hwnd, LPCWSTR title, LPARAM folder)
{
	CString ret;

	BROWSEINFO br;
	ZeroMemory(&br, sizeof(BROWSEINFO));
	br.lpfn = NULL;
	br.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	br.hwndOwner = hwnd;
	br.lpszTitle = title;
	br.lParam = folder;

	LPITEMIDLIST pidl = NULL;
	if ((pidl = SHBrowseForFolder(&br)) != NULL)
	{
		wchar_t buffer[MAX_PATH];
		if (SHGetPathFromIDList(pidl, buffer)) ret = buffer;
	}

	return ret;
}

// callback function
INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED) SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	return 0;
}

LRESULT MainForm::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	PostQuitMessage(0L);
	return __super::OnClose(uMsg, wParam, lParam, bHandled);
}

