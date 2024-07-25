#include "headbar.h"
#include "libraries.h"
#include "Settings.h" // Include the header file
#include <thread> // For sleep_for
#include <chrono> // For chrono::seconds



//make it gloabl
//file strct
struct FileNode {
    std::wstring name;
    bool isDirectory;
    std::wstring path;
    std::vector<FileNode> children;
};

int fps;
std::string fpsString;
std::vector<FileNode> fileTree;
std::wstring save = L"";
std::string content;
std::vector<char> bufferContent;

std::string CURRENT = "CURRENT";

void PrintFileContent(const FileNode& node, const std::string& CURRENT);
void ListFilesRecursively(const std::wstring& directory, FileNode& node);
void OpenFile();
std::string CWstrTostr(const std::wstring& wstr);
void DisplayFile(const FileNode& node);

std::atomic<int> cout(0); // Use atomic to safely increment cout across threads

void incrementCout() {
    while (true) {
       
        if (cout < 2) {
            std::cout << "count" << std::endl;
            cout++;

        }
        
        std::this_thread::sleep_for(std::chrono::seconds(5));

      
    }
}


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

void OpenFile() {
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"Select a folder";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != nullptr) {
        wchar_t szDir[MAX_PATH];
        if (SHGetPathFromIDList(pidl, szDir)) {
            std::wcout << L"Selected folder: " << szDir << std::endl;
            // Get path and files in the path
            FileNode root;
            root.name = szDir;
            root.isDirectory = true;
            ListFilesRecursively(szDir, root);
            fileTree.push_back(root);
            std::wcout << L"Output: " << save << std::endl;
            std::wcout << L"Loaded!" << std::endl;

            std::string savefolder = "Psettings";
            std::string fileP = savefolder + "/CfilePath.FUNCT";
            // Make a folder
            _mkdir("Psettings");
            // Create save file
            std::ofstream Savefile(fileP);
            // Write to it
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::string narrow_string = converter.to_bytes(save);
            Savefile << narrow_string;
            Savefile.close();
        }
        else {
            std::wcerr << "Error getting folder path" << std::endl;
        }
        CoTaskMemFree(pidl);
    }
    else {
        std::wcerr << "Folder selection canceled" << std::endl;
    }
}

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
        std::stringstream buffer;
        buffer << fileStream.rdbuf();
  
        content = buffer.str();
        bufferContent.resize(content.size() + content.size()+1); // this should alow infint size :)q
        std::copy(content.begin(), content.end(), bufferContent.begin());
        bufferContent.back() = '\0';  
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
    bufferContent.resize(content.size() + bufferContent.size() );
    ImGui::SetNextWindowSize(ImVec2(editorWidth + 10, editorHeight));
    ImGui::SetNextWindowPos(ImVec2(200.1f,20)); // Lock top position
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    std::string line;
    std::stringstream fileBuffer;

    // Convert fileBuffer to std::vector<char>
    std::string newContent = fileBuffer.str();
    std::vector<char> newContentVec(newContent.begin(), newContent.end());

    // Append new content to bufferContent
    bufferContent.insert(bufferContent.end(), newContentVec.begin(), newContentVec.end());

    // Ensure there's a null-terminator at the end
    bufferContent.push_back('\0');

    // Dynamically resize the buffer to handle user input
    std::string buf = bufferContent.data();
    std::cout << buf.length() << std::endl;
    bufferContent.resize(buf.length() * buf.length());
    //std::cout << buf.length() << std::endl;

    if (!bufferContent.empty()) {
       
        // Render the text editor
        ImGui::InputTextMultiline("##CodeEditor", bufferContent.data(), bufferContent.size(), ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);

        // Shrink the buffer to fit the actual content
        //bufferContent.shrink_to_fit();
    }
    else {
        ImGui::Text("Please Open A file...");
    }



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
                bufferContent.clear();
                content.clear();

                std::ifstream fileStream(CURRENT);
                if (fileStream.is_open()) {
                    std::string line;
                    std::stringstream buffer;

                    while (std::getline(fileStream, line)) {
                        buffer << line << '\n';
                    }

                    bufferContent.resize(content.size() + bufferContent.size()+1); // +1 for null terminator
                    std::copy(content.begin(), content.end(), bufferContent.begin());
                    bufferContent.back() = '\0';

                    fileStream.close();
                }
            }
            ImGui::Separator();
            ImGui::EndMenu();
        }

        if (settings) {
            Settingsrender();

        }

        if (winfpsread) {
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Documentation").x - ImGui::GetStyle().ItemSpacing.x * 2);
          
      
                // Wait for 1 second
                //so thread is effecting everything, moveing!
                std::thread incrementThread(incrementCout);

                // Detach the thread to allow it to run independently
                incrementThread.detach();

                if (cout < 2) {
                    std::cout << "count" << std::endl;
                    cout++;

                }
                else
                {
                    fpsString = "FPS:" + std::to_string(fps);
                    fps = ImGui::GetIO().Framerate;
                    cout = 0;

                }

                if (ImGui::BeginMenu(fpsString.c_str())) {


                    ImGui::EndMenu();

                }
              
               
            
           
           
            
           
        }
        


        ImGui::EndMainMenuBar();
    }
}












//this would be wayyyy more cleaner if i could just leave all the funct on the bottom, but nope :(



