#include "headbar.h"
#include "libraries.h"
#include "Settings.h" // Include the header file
//make it gloabl
//file strct
struct FileNode {
    std::wstring name;
    bool isDirectory;
    std::wstring path;
    std::vector<FileNode> children;
};

//filenode
std::vector<FileNode> fileTree;

std::wstring save = L"";
//new stuff

std::stringstream buffer;
std::ifstream file("CURRENT");
std::string content;
static char bufferContent[999999999 * 10];
std::string CURRENT = "CURRENT";
std::ofstream outFile(CURRENT);



//this prevents a big error
void PrintFileContent(const FileNode& node, const std::string& CURRENT);
void ListFilesRecursively(const std::wstring& directory, FileNode& node);
void OpenFile();
std::string CWstrTostr(const std::wstring& wstr);
void DisplayFile(const FileNode& node);

void ListFilesRecursively(const std::wstring& directory, FileNode& node) {
    std::wstring searchPath = directory + L"\\*";

    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
                FileNode child;
                child.name = findFileData.cFileName;
                child.path = directory + L"\\" + findFileData.cFileName; 
                child.isDirectory = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                node.children.push_back(child);

                if (child.isDirectory) {
                    std::wstring subdir = directory + L"\\" + findFileData.cFileName;
                    ListFilesRecursively(subdir, node.children.back());
                }
            }
        } while (FindNextFile(hFind, &findFileData) != 0);

        FindClose(hFind);
    }
    else {
        std::wcerr << L"Error: " << directory.c_str() << std::endl;
    }
}
//display file


void OpenFile()
{
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"Select a folder";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != nullptr)
    {
        wchar_t szDir[MAX_PATH];
        if (SHGetPathFromIDList(pidl, szDir))
        {
            std::wcout << L"Selected folder: " << szDir << std::endl;
            //get path and files in the path!
            FileNode root;
            root.name = szDir;
            root.isDirectory = true;
            ListFilesRecursively(szDir, root);
            fileTree.push_back(root);
            std::wcout << L"output:" << save << std::endl;

            std::wcout << L"Loaded!" <<std::endl;

            std::string savefolder = "Psettings";
            std::string fileP = savefolder + "/CfilePath.FUNCT";
            //make a folder
            _mkdir("Psettings");//this is not what im spost to do, but oops lol
            //create save file
            std::ofstream Savefile(fileP);
            //write it it
            
           std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
           std::string narrow_string = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(save);

            Savefile << narrow_string;

            Savefile.close();
           
        }
        else
        {
            std::wcerr << "Error getting folder path" << std::endl;
        }

        CoTaskMemFree(pidl);
    }
    else
    {
        std::wcerr << "Folder selection canceled" << std::endl;
    }
}

//this is so much simpler than copying and pasting
std::string CWstrTostr(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

void DisplayFile(const FileNode& node) {
    std::string nodeName = CWstrTostr(node.name);
    if (node.isDirectory) {
        if (ImGui::TreeNode(nodeName.c_str())) {
            for (const auto& child : node.children) {
                DisplayFile(child);
            }
            ImGui::TreePop();
        }
    }
    else {
        if (ImGui::Selectable(nodeName.c_str())) {
            PrintFileContent(node, CURRENT);
        }
    }
}

void PrintFileContent(const FileNode& node, const std::string& CURRENT) {
    std::ifstream fileStream(CWstrTostr(node.path));
    if (fileStream.is_open()) {
        std::string line;
        content.clear();
        buffer.str("");
        buffer.clear();

        while (std::getline(fileStream, line)) {
            buffer << line << '\n';
        }

        content = buffer.str();
        strncpy(bufferContent, content.c_str(), sizeof(bufferContent));
        bufferContent[sizeof(bufferContent) - 1] = '\0';  // Ensure null-termination

        fileStream.close();
    }
    else {
        std::cerr << "ERROR: file_not_found, error 101 " << CWstrTostr(node.path) << std::endl;
        std::cerr << "Cannot find the selected file, sorry! " << CWstrTostr(node.path) << std::endl;
    }
}

static float terminalHeightPercent = 0.2f;
void Renderbar() {
    float windowHeight = ImGui::GetIO().DisplaySize.y;
    float windowWidth = ImGui::GetIO().DisplaySize.x;
    float terminalHeight = windowHeight * terminalHeightPercent;
    float editorHeight = windowHeight - terminalHeight - 20;
    float editorWidth = windowWidth;

    ImGui::SetNextWindowSize(ImVec2(200, windowHeight - terminalHeight - 20));
    ImGui::SetNextWindowPos(ImVec2(0, 20)); // Lock top position
    ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    for (const auto& node : fileTree) {
        DisplayFile(node);
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(windowWidth, terminalHeight));
    ImGui::SetNextWindowPos(ImVec2(0, windowHeight - terminalHeight));
    ImGui::Begin("Terminal", nullptr, ImGuiWindowFlags_NoCollapse);
    // Terminal code here...
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(editorWidth, editorHeight));
    ImGui::SetNextWindowPos(ImVec2(200.1f, 20)); // Lock top position
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    ImGui::InputTextMultiline("##CodeEditor", bufferContent, IM_ARRAYSIZE(bufferContent), ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);
  

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::Separator();
            if (ImGui::MenuItem("Open Directory")) {
                OpenFile();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                // Handle exit
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Settings")) {
                settings = true;
            }
            if (ImGui::MenuItem("Close All Windows")) {
                settings = false;
                buffer.str("");
                buffer.clear();
                strncpy(bufferContent, content.c_str(), sizeof(bufferContent));
                bufferContent[sizeof(bufferContent) - 1] = '\0';  // Ensure null-termination

                std::ifstream fileStream(CURRENT);
                if (fileStream.is_open()) {
                    std::string line;
                    content.clear();
                    buffer.str("");
                    buffer.clear();

                    while (std::getline(fileStream, line)) {
                        buffer << line << '\n';
                    }

                    content = buffer.str();
                    strncpy(bufferContent, content.c_str(), sizeof(bufferContent));
                    bufferContent[sizeof(bufferContent) - 1] = '\0';  // Ensure null-termination

                    fileStream.close();
                }
               

            }
            ImGui::Separator();
            ImGui::EndMenu();
        }

        if (settings) {
            Settingsrender();
        }
        ImGui::EndMainMenuBar();
    }
}




//this would be wayyyy more cleaner if i could just leave all the funct on the bottom, but nope :(



