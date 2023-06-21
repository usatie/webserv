#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <iostream.h>

#define buf_size 256

unsigned long __stdcall handle_error(void *pipe) {
// The control (and only) function for a thread handling the standard
// error from the child process.  We'll handle it by displaying a
// message box each time we receive data on the standard error stream.
//
    char buffer[buf_size];
    HANDLE child_error_rd = (HANDLE)pipe;
    unsigned bytes;

    while (ERROR_BROKEN_PIPE != GetLastError() &&
        ReadFile(child_error_rd, buffer, 256, (LPDWORD)&bytes, NULL))
    {
        buffer[bytes+1] = '\0';
        cerr << "Error: " << buffer << endl;
//        MessageBox(NULL, buffer, "Error", MB_ICONSTOP | MB_OK);
    }
    return 0;
}

unsigned long __stdcall handle_output(void *pipe) {
// A similar thread function to handle standard output from the child
// process.  Nothing special is done with the output - it's simply
// displayed in our console.  However, just for fun it opens a C high-
// level FILE * for the handle, and uses fgets to read it.  As
// expected, fgets detects the broken pipe as the end of the file.
//
    char buffer[buf_size];
    HANDLE child_error_rd = (HANDLE)pipe;
    unsigned bytes;

    while (ERROR_BROKEN_PIPE != GetLastError() &&
        ReadFile(child_error_rd, buffer, 256, (LPDWORD)&bytes, NULL))
    {
        buffer[bytes+1] = '\0';
        cout << buffer << endl;
    }
    return 0;
/*
    char buffer[buf_size];
    int handle;
    FILE *file;

    handle = _open_osfhandle((long)pipe, _O_RDONLY | _O_BINARY);
    file = _fdopen(handle, "r");

    if ( NULL == file )
        return 1;

    while ( fgets(buffer, buf_size, file))
        printf("%s", buffer);

    return 0;
*/
}

void system_error(char *name) {
// A function to retrieve, format, and print out a message from the
// last errror.  The `name' that's passed should be in the form of a
// present tense noun (phrase) such as "opening file".
//
    char *ptr = NULL;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        0,
        GetLastError(),
        0,
        (char *)&ptr,
        1024,
        NULL);

    printf("\nError %s: %s\n", name, ptr);
    LocalFree(ptr);
}

int main(int argc, char **argv) {
    HANDLE child_input;
    HANDLE child_output_rd, child_output_wrt;
    HANDLE child_input_rd, child_input_wrt;
    HANDLE child_error_rd, child_error_wrt;
    STARTUPINFO s;
    PROCESS_INFORMATION p;
    SECURITY_ATTRIBUTES sa;
    int ignore;
    HANDLE threads[3];
    DWORD iNum, iRc;
    char szBuf[buf_size];

    if ( argc < 3 ) {
        fputs("Usage: spawn prog datafile"
            "\nwhich will spawn `prog' with its standard input set to"
            "\nread from `datafile'.  Then `prog's standard output"
            "\nwill be captured and printed.  If `prog' writes to its"
            "\nstandard error, that will not be capatilized.",
              stderr);
        return 1;
    }

    sa.nLength = sizeof sa;
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    child_input = CreateFile(
        argv[2],
        GENERIC_READ,
        FILE_SHARE_READ,
        &sa,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if ( INVALID_HANDLE_VALUE == child_input) {
        system_error("Opening input file");
        return 1;
    }

// Create pipes for the child to use as its standard error and
// standard output.  Both need to be inheritable.
    if ( !CreatePipe(&child_output_rd, &child_output_wrt, &sa, 0)) {
        system_error("Creating pipe");
        return 1;
    }

    if ( !CreatePipe(&child_input_rd, &child_input_wrt, &sa, 0)) {
        system_error("Creating pipe");
        return 1;
    }
    if ( !CreatePipe(&child_error_rd, &child_error_wrt, &sa, 0)) {
        system_error("Creating pipe");
        return 1;
    }


// Okay, now the child can inherit the pipes.  However, we don't want
// the child to inherit the read ends of the pipes, so we set its
// handle to non-inheritable.  As per usual MS stupidity, we use
// DuplicateHandle to change the original handle, not to dupicate
// anything.
// The second to last parameter being false signifies that the handle
// is not to be inheritable.
    DuplicateHandle(
        GetCurrentProcess(),
        child_output_rd,
        GetCurrentProcess(),
        NULL,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS);

    DuplicateHandle(
        GetCurrentProcess(),
        child_input_wrt,
        GetCurrentProcess(),
        NULL,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS);

    DuplicateHandle(
        GetCurrentProcess(),
        child_error_rd,
        GetCurrentProcess(),
        NULL,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS);

    memset(&s, 0, sizeof s);
    s.cb = sizeof(s);
    s.dwFlags = STARTF_USESTDHANDLES;
    s.hStdInput = child_input_wrt;
    s.hStdOutput = child_output_wrt;
    s.hStdError = child_error_wrt;

// Since we've redirected the standard input, output and error handles
// of the child process, we create it without a console of its own.
// (That's the `DETACHED_PROCESS' part of the call.)  Other
// possibilities include passing 0 so the child inherits our console,
// or passing CREATE_NEW_CONSOLE so the child gets a console of its
// own.
//
    if (!CreateProcess(
        argv[1],
        NULL, NULL, NULL,
        TRUE,
        DETACHED_PROCESS,
        NULL, NULL,
        &s,
        &p))
    {
        system_error("Spawning program");
        return 1;
    }

// Since we don't need the handle to the child's thread, close it to
// save some resources.
    CloseHandle(p.hThread);

// We no longer need the handles to the child's write pipes - close
// our handles to them.  Rather than simply reducing resource usage,
// this is crucial - until all handles to the write end of the pipe
// are closed ReadFile won't detect the broken pipe we use to detect
// the child process' terminating communication.
//
    CloseHandle(child_output_wrt);
    CloseHandle(child_input_rd);
    CloseHandle(child_error_wrt);

// Start the threads to process the child's output, one for each
// output stream.
    threads[0] = CreateThread(
        NULL,
        0,
        handle_error,
        (void *)child_error_rd,
        0,
        (LPDWORD)&ignore);

    threads[1] = CreateThread(
        NULL,
        0,
        handle_output,
        (void *)child_output_rd,
        0,
        (LPDWORD)&ignore);
//    threads[2] = p.hProcess;

  while ( ReadFile(child_input, szBuf, buf_size, (LPDWORD)&iNum, NULL) )
    {
      WriteFile(child_input_wrt, szBuf, iNum, (LPDWORD)&iRc, NULL);
    }
  CloseHandle(child_input_wrt);


  cerr << "parent sleeping" << endl;
  Sleep(10000); // sleep 10 seconds
  cerr << "terminating child" << endl;
  TerminateProcess(p.hProcess, 1);
  cerr << "waiting for threads" << endl;
// Finally, wait for the child's output to be processed, and we're
// done.
    WaitForMultipleObjects(2, threads, TRUE, INFINITE);

    return 0;
}

