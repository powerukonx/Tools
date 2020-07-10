/* =============================================================================

                  ██╗   ██╗██╗  ██╗ ██████╗ ███╗   ██╗██╗  ██╗
                  ██║   ██║██║ ██╔╝██╔═══██╗████╗  ██║╚██╗██╔╝
                  ██║   ██║█████╔╝ ██║   ██║██╔██╗ ██║ ╚███╔╝
                  ██║   ██║██╔═██╗ ██║   ██║██║╚██╗██║ ██╔██╗
                  ╚██████╔╝██║  ██╗╚██████╔╝██║ ╚████║██╔╝ ██╗
                   ╚═════╝ ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═══╝╚═╝  ╚═╝

  File name:    main.cpp
  Date:         23 07 2017
  Author:       Power.
  Description:  Logic analyzer - Body file.

============================================================================= */

/* =============================================================================
                                 DEBUG Section
============================================================================= */


/* =============================================================================
                                 Include Files
============================================================================= */
#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "resource.h"


/* =============================================================================
                          Private defines and typedefs
============================================================================= */
#define FALLING_EDGE_TRIGGER    (0)                 // Edges trigger
#define RISING_EDGE_TRIGGER     (1)
#define SERIAL_MAX_BUF_SIZE     (1024)              // Serial port buffer size in bytes
#define DEFAULT_TIMEBASE        (64)                // Default timebase 4us/div
#define DEFAULT_CAPTURE_DELAY   (0)                 // No capture delay at beginning
#define NBR_DIVISION            (16.0f)             // Default 16 divisions

#define BIT7                    (0b10000000)        // Bit position
#define BIT6                    (0b01000000)
#define BIT5                    (0b00100000)
#define BIT4                    (0b00010000)
#define BIT3                    (0b00001000)
#define BIT2                    (0b00000100)
#define BIT1                    (0b00000010)
#define BIT0                    (0b00000001)

#define DEFAULT_TRIGGER_SRC     (PROBE16)
#define DEFAULT_TRIGGER_TYPE    (FALLING_EDGE_TRIGGER)
#define DEFAULT_MEMORY_DEPTH    (MEMORY_DEPTH_8192)

#define COMMAND_SAMPLING        (71)
#define COMMAND_CONFIGURATION   (91)

#define COLORREF2RGB(Color)     (Color & 0xff00) | ((Color >> 16) & 0xff) | ((Color << 16) & 0xff0000)

typedef enum _MEMORY_DEPTH_ {
  MEMORY_DEPTH_1 = 0u,
  MEMORY_DEPTH_2,
  MEMORY_DEPTH_4,
  MEMORY_DEPTH_8,
  MEMORY_DEPTH_16,
  MEMORY_DEPTH_32,
  MEMORY_DEPTH_64,
  MEMORY_DEPTH_128,
  MEMORY_DEPTH_256,
  MEMORY_DEPTH_512,
  MEMORY_DEPTH_1024,
  MEMORY_DEPTH_2048,
  MEMORY_DEPTH_4096,
  MEMORY_DEPTH_8192
} e_MemoryDepth_t;

typedef enum _PROBE_ {
  PROBE1 = 0u,
  PROBE2,
  PROBE3,
  PROBE4,
  PROBE5,
  PROBE6,
  PROBE7,
  PROBE8,
  PROBE9,
  PROBE10,
  PROBE11,
  PROBE12,
  PROBE13,
  PROBE14,
  PROBE15,
  PROBE16,
  PROBE17,
  PROBE18,
  PROBE19,
  PROBE20,
  PROBE21,
  PROBE22,
  PROBE23,
  PROBE24,
  PROBE25,
  PROBE26,
  PROBE27,
  PROBE28,
  PROBE29,
  PROBE30,
  PROBE31,
  PROBE32,
  TOTAL_PROBES
} e_Probe_t;

typedef enum _TRACEx_ {
  TRACE1 = 0u,
  TRACE2,
  TRACE3,
  TRACE4,
  TRACE5,
  TRACE6,
  TRACE7,
  TRACE8,
  TRACE9,
  TRACE10,
  TRACE11,
  TRACE12,
  TRACE13,
  TRACE14,
  TRACE15,
  TRACE16,
  TOTAL_TRACES
} e_Trace_t;

typedef struct _TRACE_ {
    BOOL bState;                    // Trace state, TRUE if trace is displayed
    DWORD dwColor;                  // Trace RGB color
    TCHAR szName[MAX_PATH];         // Trace name (string zero ended)
    DWORD dwProbeNumber;            // Probe associated to Trace

    HBITMAP hColor;                 // Windows Bitmap Handle
    HBITMAP hTrigger;               // Windows Bitmap Handle
    DWORD dwIDState;                // Windows ressources ID
    DWORD dwIDProbeNumber;          // Windows ressources ID
    DWORD dwIDColor;                // Windows ressources ID
    DWORD dwIDName;                 // Windows ressources ID
    DWORD dwIDTrigger;              // Windows ressources ID
} s_Trace_t;

typedef struct _TRACEVISU_ {
    TCHAR szID[4];            // ID for Save/load

    TCHAR szTitle[MAX_PATH];        // Capture title

    s_Trace_t trace[TOTAL_TRACES];    // 16 traces on screen

    DWORD dwProbeTrigger;           // Probe where signal is trigged
    BYTE byTriggerType;             // Trigger type (Rising or Falling edge)
    DWORD dwCaptureSize;            // Capture size
//    DWORD dwCaptureToRead;          // Nbr of capture bytes to read
    HWND p_hwndDlg;                   // Handle to main windows
} s_TraceVisu_t;


/* =============================================================================
                        Private constants and variables
============================================================================= */
static BOOL g_bCOMAvailable;
static volatile BOOL g_bSerialThreadEnable = FALSE;
static BOOL g_bConfigurationChanged;
static BYTE g_byCOMFind;
static FLOAT g_fTimeBase;
static FLOAT g_fBoardSamplePeriod = 5.0f/72.0f;
static WORD g_wLogicFormOffset;
static WORD g_wCaptureDelay;
static volatile DWORD* g_pdwCapture;
static DWORD g_dwSerialThreadID;
static DWORD g_dwMemoryDepth;     // *5 to be sure to have all data
static TCHAR g_szCOMPort[16][8];
static TCHAR g_szCOMSelected[MAX_PATH];
static UINT g_uiTimerID = 1;
static INT32 g_i32FormatLogicScreen;
static PIXELFORMATDESCRIPTOR g_pfdLogicScreen;
static HDC g_hDCLogicScreen;
static HGLRC g_hRCLogicScreen;
static HINSTANCE g_hInstance;
static HBITMAP g_hProbeColor;
static HBITMAP g_hFallingEdge;
static HBITMAP g_hRisingEdge;
static HBITMAP g_hNoTrig;
static HANDLE g_hSerialThread = NULL;
static HANDLE g_hCommPort = NULL;
static s_TraceVisu_t tracevisu = {
    TEXT("ULA"),
    "\0",
    {
    {BST_CHECKED, 0x00000055, "\0", PROBE1, NULL, NULL, IDSTATETRACE1, IDNUMBERPROBE1, IDCOLORTRACE1, IDNAMETRACE1, IDTRIGGERTRACE1},
    {BST_CHECKED, 0x000000AA, "\0", PROBE2, NULL, NULL, IDSTATETRACE2, IDNUMBERPROBE2, IDCOLORTRACE2, IDNAMETRACE2, IDTRIGGERTRACE2},
    {BST_CHECKED, 0x000000FF, "\0", PROBE3, NULL, NULL, IDSTATETRACE3, IDNUMBERPROBE3, IDCOLORTRACE3, IDNAMETRACE3, IDTRIGGERTRACE3},
    {BST_CHECKED, 0x00005500, "\0", PROBE4, NULL, NULL, IDSTATETRACE4, IDNUMBERPROBE4, IDCOLORTRACE4, IDNAMETRACE4, IDTRIGGERTRACE4},
    {BST_CHECKED, 0x00005555, "\0", PROBE5, NULL, NULL, IDSTATETRACE5, IDNUMBERPROBE5, IDCOLORTRACE5, IDNAMETRACE5, IDTRIGGERTRACE5},
    {BST_CHECKED, 0x000055AA, "\0", PROBE6, NULL, NULL, IDSTATETRACE6, IDNUMBERPROBE6, IDCOLORTRACE6, IDNAMETRACE6, IDTRIGGERTRACE6},
    {BST_CHECKED, 0x000055FF, "\0", PROBE7, NULL, NULL, IDSTATETRACE7, IDNUMBERPROBE7, IDCOLORTRACE7, IDNAMETRACE7, IDTRIGGERTRACE7},
    {BST_CHECKED, 0x0000AA00, "\0", PROBE8, NULL, NULL, IDSTATETRACE8, IDNUMBERPROBE8, IDCOLORTRACE8, IDNAMETRACE8, IDTRIGGERTRACE8},
    {BST_CHECKED, 0x0000AA55, "\0", PROBE9, NULL, NULL, IDSTATETRACE9, IDNUMBERPROBE9, IDCOLORTRACE9, IDNAMETRACE9, IDTRIGGERTRACE9},
    {BST_CHECKED, 0x0000AAAA, "\0", PROBE10,NULL, NULL, IDSTATETRACE10,IDNUMBERPROBE10,IDCOLORTRACE10,IDNAMETRACE10,IDTRIGGERTRACE10},
    {BST_CHECKED, 0x0000AAFF, "\0", PROBE11,NULL, NULL, IDSTATETRACE11,IDNUMBERPROBE11,IDCOLORTRACE11,IDNAMETRACE11,IDTRIGGERTRACE11},
    {BST_CHECKED, 0x0000FF00, "\0", PROBE12,NULL, NULL, IDSTATETRACE12,IDNUMBERPROBE12,IDCOLORTRACE12,IDNAMETRACE12,IDTRIGGERTRACE12},
    {BST_CHECKED, 0x0000FF55, "\0", PROBE13,NULL, NULL, IDSTATETRACE13,IDNUMBERPROBE13,IDCOLORTRACE13,IDNAMETRACE13,IDTRIGGERTRACE13},
    {BST_CHECKED, 0x0000FFAA, "\0", PROBE14,NULL, NULL, IDSTATETRACE14,IDNUMBERPROBE14,IDCOLORTRACE14,IDNAMETRACE14,IDTRIGGERTRACE14},
    {BST_CHECKED, 0x0000FFFF, "\0", PROBE15,NULL, NULL, IDSTATETRACE15,IDNUMBERPROBE15,IDCOLORTRACE15,IDNAMETRACE15,IDTRIGGERTRACE15},
    {BST_CHECKED, 0x00550000, "\0", PROBE16,NULL, NULL, IDSTATETRACE16,IDNUMBERPROBE16,IDCOLORTRACE16,IDNAMETRACE16,IDTRIGGERTRACE16}
    }, DEFAULT_TRIGGER_SRC, DEFAULT_TRIGGER_TYPE, (1<<DEFAULT_MEMORY_DEPTH)/*DEFAULT_CAPTURE_SIZE, DEFAULT_CAPTURE_SIZE*5*/, NULL};

/* =============================================================================
                        Private function declarations
============================================================================= */
VOID vUpdateTrigger (HWND, s_TraceVisu_t*);
HBITMAP hReplaceColor(HBITMAP, COLORREF, COLORREF, HDC);
VOID vUpdateColorTrace (HWND, s_TraceVisu_t*, BYTE);
DWORD WINAPI dwSerialThread (LPVOID);
BOOL bSerialSendCommand (HWND, s_TraceVisu_t*, BYTE);
BOOL bSerialInit (HWND);
BOOL bSerialSearch (HWND);
VOID vLogicScreenOpenGLRender (VOID);
BOOL CALLBACK bMainDialogCallback(HWND, UINT, WPARAM, LPARAM);


/* =============================================================================
                               Public functions
============================================================================= */

/*==============================================================================
Function    : WinMain.

Describe    : Main windows function.

Parameters  : p_hInstance => A handle to the current instance of the application.
              p_hPrevInstance => This parameter is always NULL.
              p_lpCmdLine => The command line for the application.
              p_nShowCmd => Controls how the window is to be shown.

Returns     : Dialog box EndDialog() return.
==============================================================================*/
int APIENTRY WinMain (HINSTANCE p_hInstance, HINSTANCE p_hPrevInstance, LPSTR p_lpCmdLine, int p_nShowCmd)
{
  /* Save program instance. */
  g_hInstance = p_hInstance;

  /* Initialize common controls. */
  InitCommonControls ();

  /*Creates a modal dialog box from a dialog box template resource. */
  return DialogBox (g_hInstance, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)bMainDialogCallback);
}


/* =============================================================================
                              Private functions
============================================================================= */

/*==============================================================================
Function    : vUpdateTrigger.

Describe    : Update trigger information.

Parameters  : p_hwndDlg = Handle to dialog box,
              ptracevisu = Pointer to TRACEVISU structure.

Returns     : None.
==============================================================================*/
void vUpdateTrigger (HWND p_hwndDlg, s_TraceVisu_t* ptracevisu, BOOL byNoChange)
{
    // Locals variables definition
    WORD l_wTemp1;

    // If change enabled
    if (byNoChange == FALSE)
    {
        // Update trigger type
        ptracevisu->byTriggerType ^= 0x01;
    }

    // For each trace
    for (l_wTemp1 = TRACE1; l_wTemp1 < TOTAL_TRACES; l_wTemp1++)
    {
        // If trigger enable on this trace/probe
        if (ptracevisu->dwProbeTrigger == ptracevisu->trace[l_wTemp1].dwProbeNumber)
        {
            // If falling edge
            if (ptracevisu->byTriggerType == FALLING_EDGE_TRIGGER)
            {
                SendDlgItemMessage(p_hwndDlg, ptracevisu->trace[l_wTemp1].dwIDTrigger, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM) g_hFallingEdge);
            }
            else    // Rising edge
            {
                SendDlgItemMessage(p_hwndDlg, ptracevisu->trace[l_wTemp1].dwIDTrigger, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM) g_hRisingEdge);
            }
        }
        else
        {
            SendDlgItemMessage(p_hwndDlg, ptracevisu->trace[l_wTemp1].dwIDTrigger, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM) g_hNoTrig);
        }
    }

    // Serial configuration changed
    g_bConfigurationChanged = TRUE;
}


/*==============================================================================
Function    : hReplaceColor.

Describe    : Replace a color from bitmap.

Parameters  : p_hBmp => Source Bitmap
              p_cOldColor => Color to replace in p_hBmp
              p_cNewColor => Color used for replacement
              p_hBmpDC => DC of p_hBmp ( default NULL ) could be NULL if p_hBmp is not selected

Returns     : HBITMAP of the modified bitmap or NULL for errors.
==============================================================================*/
HBITMAP hReplaceColor (HBITMAP p_hBmp,COLORREF p_cOldColor,COLORREF p_cNewColor,HDC p_hBmpDC)
{
  /* Locals variables declaration. */
  HBITMAP l_hRetBmp = NULL;
  HDC l_hBufferDC = NULL;
  HBITMAP hTmpBitmap = NULL;
  HGDIOBJ PreviousBufferObject = NULL;
  HDC DirectDC = NULL;
  BITMAP bm;
  HBITMAP DirectBitmap;
  HGDIOBJ PreviousObject;
  
// pointer used for direct Bitmap pixels access
        UINT * ptPixels;
  /* Inputs parameters checking. */
  if (   (NULL != p_hBmp)
      && (NULL != p_hBmpDC) )
  {
    /* Creates a memory device context (DC) compatible with application. */
    l_hBufferDC = CreateCompatibleDC (NULL);
    if (NULL != l_hBufferDC)
    {
      /* Retrieves a handle to OBJ_BITMAP. */
      p_hBmp == (HBITMAP)GetCurrentObject (p_hBmpDC, OBJ_BITMAP);
      if (NULL != p_hBmp)
      {
        /* Creates a bitmap with the specified width, height, and color format. */
        hTmpBitmap = CreateBitmap (1, 1, 1, 1, NULL);
        
        /* Selects hTmpBitmap into p_hBmpDC. */
        SelectObject (p_hBmpDC, hTmpBitmap);
      }

      PreviousBufferObject=SelectObject(l_hBufferDC,p_hBmp);
        
      // here l_hBufferDC contains the bitmap
      DirectDC=CreateCompatibleDC(NULL); // DC for working
      if (DirectDC)
      {
        // Get bitmap size
        GetObject(p_hBmp, sizeof(bm), &bm);

        // create a BITMAPINFO with minimal initilisation
        // for the CreateDIBSection
        BITMAPINFO RGB32BitsBITMAPINFO;
        ZeroMemory(&RGB32BitsBITMAPINFO,sizeof(BITMAPINFO));
        RGB32BitsBITMAPINFO.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        RGB32BitsBITMAPINFO.bmiHeader.biWidth=bm.bmWidth;
        RGB32BitsBITMAPINFO.bmiHeader.biHeight=bm.bmHeight;
        RGB32BitsBITMAPINFO.bmiHeader.biPlanes=1;
        RGB32BitsBITMAPINFO.bmiHeader.biBitCount=32;

        DirectBitmap = CreateDIBSection(DirectDC,
                               (BITMAPINFO *)&RGB32BitsBITMAPINFO,
                               DIB_RGB_COLORS,
                               (void **)&ptPixels,
                               NULL, 0);
        if (DirectBitmap)
        {
          // here DirectBitmap!=NULL so ptPixels!=NULL no need to test
          PreviousObject=SelectObject(DirectDC, DirectBitmap);
          BitBlt(DirectDC,0,0,
                         bm.bmWidth,bm.bmHeight,
                           l_hBufferDC,0,0,SRCCOPY);

          // here the DirectDC contains the bitmap

          // Convert COLORREF to RGB (Invert RED and BLUE)
          p_cOldColor=COLORREF2RGB(p_cOldColor);
          p_cNewColor=COLORREF2RGB(p_cNewColor);

          // After all the inits we can do the job : Replace Color
          for (int i=((bm.bmWidth*bm.bmHeight)-1);i>=0;i--)
          {
            if (ptPixels[i]==p_cOldColor) ptPixels[i]=p_cNewColor;
          }
      // little clean up
      // Don't delete the result of SelectObject because it's
      // our modified bitmap (DirectBitmap)
          SelectObject(DirectDC,PreviousObject);

          // finish
          l_hRetBmp = DirectBitmap;
        }
        // clean up
        DeleteDC(DirectDC);
      }

      if (hTmpBitmap)
      {
        SelectObject(p_hBmpDC, p_hBmp);
        DeleteObject(hTmpBitmap);
      }

      SelectObject(l_hBufferDC,PreviousBufferObject);

      // l_hBufferDC is now useless
      DeleteDC(l_hBufferDC);
    }
  }
  return l_hRetBmp;
}


/*==============================================================================
Function    : vUpdateColorTrace.

Describe    : Update trace color.

Parameters  : p_hwndDlg => handle of the main dialog box
              ptracevisu => Pointer to TRACEVISU structure
              byTrace => Trace to update

Returns     : None.
==============================================================================*/
void vUpdateColorTrace (HWND p_hwndDlg, s_TraceVisu_t* ptracevisu, BYTE byTrace)
{
  // Locals variables declaration
  CHOOSECOLOR ccolor;
  COLORREF acrCustClr[16];

  // Update choosecolor structure
  ZeroMemory (&ccolor, sizeof (CHOOSECOLOR));
  ccolor.lStructSize = sizeof(CHOOSECOLOR);
  ccolor.hwndOwner = p_hwndDlg;
  ccolor.lpCustColors = (LPDWORD) acrCustClr;
  ccolor.rgbResult = ptracevisu->trace[byTrace].dwColor;
  ccolor.Flags = CC_FULLOPEN | CC_RGBINIT;
  if (ChooseColor (&ccolor) == TRUE)
  {
    // Update color
    ptracevisu->trace[byTrace].dwColor = ccolor.rgbResult;
    ptracevisu->trace[byTrace].hColor = hReplaceColor (g_hProbeColor, 0xffffff, ptracevisu->trace[byTrace].dwColor, GetDC(p_hwndDlg));
    SendDlgItemMessage(p_hwndDlg, ptracevisu->trace[byTrace].dwIDColor, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)ptracevisu->trace[byTrace].hColor);
  }
}


/*==============================================================================
Function    : dwSerialThread.

Describe    : Thread for serial.

Parameters  : Some parameters...

Returns     : Some returns...
==============================================================================*/
DWORD WINAPI dwSerialThread (LPVOID p_LpParameter)
{
  // Locals variables declaration
  BYTE *pbyDest;
  BYTE byCommand;
  s_TraceVisu_t *ptracevisu;
  DWORD dwWriteByteLength;

  // Tracevisu pointer to input parameters
  ptracevisu = (s_TraceVisu_t *)p_LpParameter;

  // Byte pointer to DWORD buffer
  pbyDest = (BYTE *)g_pdwCapture;

  // Disable acquire windows
  EnableWindow (GetDlgItem(ptracevisu->p_hwndDlg, IDACQUIRE), FALSE);

  // Thread enabled
  g_bSerialThreadEnable = TRUE;

  // Send COMMAND_SAMPLING
  byCommand = COMMAND_SAMPLING;
  WriteFile (g_hCommPort, &byCommand, 1, &dwWriteByteLength, NULL);

  // Read bytes
  ReadFile (g_hCommPort, pbyDest++, g_dwMemoryDepth*(sizeof(DWORD) + 1)/*ptracevisu->dwCaptureToRead*/, &ptracevisu->dwCaptureSize, NULL);

  // Adjust value for BYTE => DWORD
  ptracevisu->dwCaptureSize>>=2;

  // Thread disabled
  g_bSerialThreadEnable = FALSE;

  // Enable acquire windows
  EnableWindow (GetDlgItem(ptracevisu->p_hwndDlg, IDACQUIRE), TRUE);

  // End thread
  ExitThread(0);

  return 0;
}


/*==============================================================================
Function    : bSerialSendCommand.

Describe    : Send command to serial.

Parameters  : p_hwndDlg => handle of the main dialog box

Returns     : TRUE if no errors
                FALSE else
==============================================================================*/
BOOL bSerialSendCommand (HWND p_hwndDlg, s_TraceVisu_t* p_psTraceVisu, BYTE p_byCommand)
{
  /* Locals variables declaration. */
  BOOL bReturn = FALSE;
  DWORD dwWriteByteLength;
  BYTE l_byCommand;

  /* COM port already open ? */
  if (   (NULL != g_hCommPort) 
      && (INVALID_HANDLE_VALUE != g_hCommPort) )
  {
    /* Command dispatch. */
    switch (p_byCommand)
    {
      case COMMAND_SAMPLING:

        /* Create serial communication thread. */
        g_hSerialThread = CreateThread (NULL, 0, dwSerialThread, p_psTraceVisu, 0, &g_dwSerialThreadID);
        bReturn = TRUE;
        break;

      case COMMAND_CONFIGURATION:

        /* Send COMMAND_CONFIGURATION. */
        l_byCommand = COMMAND_CONFIGURATION;
        WriteFile (g_hCommPort, &l_byCommand, 1, &dwWriteByteLength, NULL);

        /* Set Trigger source and type. */
        l_byCommand = p_psTraceVisu->dwProbeTrigger | (p_psTraceVisu->byTriggerType<<5);
        WriteFile (g_hCommPort, &l_byCommand, 1, &dwWriteByteLength, NULL);

        /* Set Depth memory. */
        l_byCommand = DEFAULT_MEMORY_DEPTH;
        WriteFile (g_hCommPort, &l_byCommand, 1, &dwWriteByteLength, NULL);

        /* Set Delay time. */
        l_byCommand = g_wCaptureDelay&0xFF;         // LSB
        WriteFile (g_hCommPort, &l_byCommand, 1, &dwWriteByteLength, NULL);
        l_byCommand = (g_wCaptureDelay>>8)&0xFF;    // MSB
        WriteFile (g_hCommPort, &l_byCommand, 1, &dwWriteByteLength, NULL);
        bReturn = TRUE;
        break;

      default:
        MessageBox (p_hwndDlg, "Error : unknow command", "bSerialSendCommand", MB_ICONERROR|MB_OK);
        break;
    }
  }
  else
  {
    MessageBox (p_hwndDlg, "Warning : select COM first", "bSerialSendCommand", MB_ICONWARNING|MB_OK);
  }

  return bReturn;
}


/*==============================================================================
Function    : bSerialInit.

Describe    : Serial Initialisation.

Parameters  : p_hwndDlg => handle of the main dialog box

Returns     : TRUE if no errors
                FALSE else
==============================================================================*/
BOOL bSerialInit (HWND p_hwndDlg)
{
  /* Locals variables declaration. */
  BOOL bReturn = FALSE;
  WORD l_wTemp1;
  COMMCONFIG dcbSerialParams;
  COMMTIMEOUTS timeouts;

  // If a port already open
  if (   (NULL != g_hCommPort) 
      && (INVALID_HANDLE_VALUE != g_hCommPort) )
  {
    /* Close it. */
    CloseHandle(g_hCommPort);
    
    g_hCommPort = NULL;
  }

  /* Get selected COM. */
  l_wTemp1 = SendDlgItemMessage (p_hwndDlg, IDCOMNUMBER,CB_GETCURSEL, 0, 0);
  SendDlgItemMessage (p_hwndDlg, IDCOMNUMBER, CB_GETLBTEXT, l_wTemp1, (LPARAM) g_szCOMSelected);

  /* Open serial port. */
  g_hCommPort = CreateFile (g_szCOMSelected, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
  if (g_hCommPort  != INVALID_HANDLE_VALUE)
  {
    /* Set buffer sizes. */
    if (SetupComm(g_hCommPort, SERIAL_MAX_BUF_SIZE, SERIAL_MAX_BUF_SIZE))
    {
      /* Get com settings. */
      if (GetCommState(g_hCommPort, &dcbSerialParams.dcb))
      {
        /* Change settings. */
        dcbSerialParams.dcb.BaudRate = CBR_115200;
        dcbSerialParams.dcb.ByteSize = DATABITS_8;
        dcbSerialParams.dcb.StopBits = ONESTOPBIT;
        dcbSerialParams.dcb.Parity = NOPARITY;
        dcbSerialParams.dcb.fBinary = TRUE;
        dcbSerialParams.dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcbSerialParams.dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcbSerialParams.dcb.fOutxCtsFlow = FALSE;
        dcbSerialParams.dcb.fOutxDsrFlow = FALSE;
        dcbSerialParams.dcb.fDsrSensitivity = FALSE;
        dcbSerialParams.dcb.fAbortOnError = FALSE;

        /* Set com settings. */
        if (SetCommState(g_hCommPort, &dcbSerialParams.dcb))
        {
          /* Purge COM port. */
          PurgeComm(g_hCommPort, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

          /* Configure timeout. */
          timeouts.ReadIntervalTimeout = 2;
          timeouts.ReadTotalTimeoutMultiplier = 10;
          timeouts.ReadTotalTimeoutConstant = 10;
          timeouts.WriteTotalTimeoutMultiplier = 100;
          timeouts.WriteTotalTimeoutConstant = 100;

          /* Set com port timeout. */
          if (SetCommTimeouts(g_hCommPort, &timeouts) != 0)
          {
            /* Init done. */
            bReturn = TRUE;
          }
        }
      }
    }
  }
  else
  {
    g_bCOMAvailable = FALSE;
  }

  return bReturn;
}


/*==============================================================================
Function    : bSerialSearch.

Describe    : .

Parameters  : p_hwndDlg => handle of the main dialog box

Returns     : TRUE if no errors
                FALSE else
==============================================================================*/
BOOL bSerialSearch (HWND p_hwndDlg)
{
  /* Locals variables declaration. */
  BOOL l_bResult = FALSE;
  BYTE l_byComNumber;

  /* Clear list if COM port. */
  ZeroMemory (g_szCOMPort, sizeof (g_szCOMPort) );

  /* Index of list. */
  g_byCOMFind = 0;

  /* Search for 16 first COM PC. */
  for (l_byComNumber = 1; l_byComNumber < 17; l_byComNumber++)
  {
    /* Construct COM name. */
    sprintf(g_szCOMPort[g_byCOMFind], "COM%i", l_byComNumber);

    /* try to open. */
    g_hCommPort = CreateFile(g_szCOMPort[g_byCOMFind], GENERIC_READ | GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);

    /* If COM exist. */
    if (g_hCommPort != INVALID_HANDLE_VALUE)
    {
      /* COM Port available. */
      l_bResult = TRUE;

      /* Next COM Port. */
      g_byCOMFind++;

      /* Close open COM. */
      CloseHandle(g_hCommPort);
    }
  }

  return (l_bResult);
}


/*==============================================================================
Function    : vLogicScreenOpenGLRender.

Describe    : Update OpenGL windows vi trace.

Parameters  : None

Returns     : None
==============================================================================*/
void vLogicScreenOpenGLRender (void)
{
  /* Locals variables declaration. */
  FLOAT fTime;
  WORD wIndex;
  FLOAT fValue1;
  FLOAT fValue2;
  FLOAT fOffsetX1;
  FLOAT fOffsetX2;
  FLOAT fOffsetY1;
  FLOAT fOffsetY2;
  FLOAT fSizeX = (1.0f / g_fBoardSamplePeriod) * g_fTimeBase*16;
  FLOAT fOffsetY;
  UINT8 l_u8Trace;

  /* Select Spectrum render GL context. */
  wglMakeCurrent (g_hDCLogicScreen, g_hRCLogicScreen);

  /* Clear GL buffer. */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Print verticals grids. */
  fOffsetX2 = 0.0f;//2.0f/22.0f;
  for (fOffsetX1 = -1.0; fOffsetX1 < 1.0; fOffsetX1 += 2.0f / 16.0f)
  {
    for (fOffsetY1 = -1.0; fOffsetY1 < 1.0; fOffsetY1 += 2.0f / 16.0f)
    {
      glBegin(GL_QUADS);
          glColor3f (0.4f, 0.4f, 0.4f);
          glVertex3d (fOffsetX2 + fOffsetX1 - 0.0010f, fOffsetY1, 0.0f);
          glVertex3d (fOffsetX2 + fOffsetX1 + 0.0010f, fOffsetY1, 0.0f);
          glVertex3d (fOffsetX2 + fOffsetX1 + 0.0010f, fOffsetY1 + (2.0f / 64.0f), 0.0f);
          glVertex3d (fOffsetX2 + fOffsetX1 - 0.0010f, fOffsetY1 + (2.0f / 64.0f), 0.0f);
      glEnd();
    }
  }

  wIndex = g_wLogicFormOffset;

  /* Display traces. */
  for (fTime = -1.0f; fTime < 1.0f; fTime += 2.0f/fSizeX)
  {
    fOffsetY = 0.900f;

    for (l_u8Trace = TRACE1; l_u8Trace <= TRACE16; l_u8Trace++)
    {
      if (tracevisu.trace[l_u8Trace].bState == BST_CHECKED)
      {
        // Logic probe #1
        fValue1 = 0.09f * ((g_pdwCapture[wIndex]>>tracevisu.trace[l_u8Trace].dwProbeNumber) & 0x01);
        fValue2 = 0.09f * ((g_pdwCapture[wIndex + 1]>>tracevisu.trace[l_u8Trace].dwProbeNumber) & 0x01);
        if (fValue1 == fValue2)
        {
          fOffsetX1 = 2.0/fSizeX;
          fOffsetY1 = 0.00312f;
          fOffsetY2 = 0.00312f;
        }
        else if (fValue1 < fValue2)
        {
          fOffsetX1 = 0.00312f/2.0f;
          fOffsetY1 = fValue1 + 0.00312f;
          fOffsetY2 = fValue2 + 0.00312f;
        }
        else
        {
          fOffsetX1 = 0.00312f/2.0f;
          fOffsetY1 = fValue1;
          fOffsetY2 = fValue2;
        }

        glBegin(GL_QUADS);
          glColor3f ((float) (tracevisu.trace[l_u8Trace].dwColor&0xff)/256.0, (float)(tracevisu.trace[l_u8Trace].dwColor>>8&0xff)/256.0, (float)(tracevisu.trace[l_u8Trace].dwColor>>16&0xff)/256.0);
          glVertex3d (fTime - fOffsetX1, fOffsetY + fValue1 - fOffsetY1, 0.0f);
          glVertex3d (fTime + fOffsetX1, fOffsetY + fValue1 - fOffsetY1, 0.0f);
          glVertex3d (fTime + fOffsetX1, fOffsetY + fValue1 + fOffsetY2, 0.0f);
          glVertex3d (fTime - fOffsetX1, fOffsetY + fValue1 + fOffsetY2, 0.0f);
        glEnd();
      }

      fOffsetY -= 0.127f;
    }

    wIndex++;
  }

  /* Swap buffer. */
  SwapBuffers (g_hDCLogicScreen);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bMsgClose (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = TRUE;

  /* Wait end thread. */
  while (g_bSerialThreadEnable == TRUE);

  /* Is COM port opened ? */
  if (   (NULL != g_hCommPort)
      && (INVALID_HANDLE_VALUE != g_hCommPort) )
  {
    /* Close it. */
    CloseHandle(g_hCommPort);
  }

  /* Is capture buffer allocated ? */
  if (g_pdwCapture != NULL)
  {
    /* Free it ! */
    free ((void*)g_pdwCapture);
  }

  /* End dialog box. */
  EndDialog (p_hwndDlg, 0);

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bMsgInitDialog (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = TRUE;
  WORD l_wTemp1;
  WORD l_wTemp2;
  TCHAR l_szTemp[MAX_PATH]= "";
  FLOAT l_fTemp;

  /* Load trace bitmap. */
  g_hProbeColor = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_PROBECOLOR));
  g_hNoTrig = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_NOEDGE));
  g_hRisingEdge = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_RISINGEDGE));
  g_hFallingEdge = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_FALLINGEDGE));

  /* Initializes globals variables. */
  g_bConfigurationChanged = TRUE;

  /* Allocate and clear memory for capture. */
  g_pdwCapture = (DWORD *) malloc ((1<<MEMORY_DEPTH_8192) * sizeof(DWORD));
  ZeroMemory ((void *)g_pdwCapture, (1<<MEMORY_DEPTH_8192) * sizeof(DWORD));

  /* Search COM. */
  g_bCOMAvailable = bSerialSearch (p_hwndDlg);

  /* Is COM found ?. */
  if (g_bCOMAvailable == TRUE)
  {
    /* Fill combo box control. */
    for (l_wTemp1 = 0; l_wTemp1 < g_byCOMFind; l_wTemp1++)
    {
      SendDlgItemMessage (p_hwndDlg, IDCOMNUMBER, CB_ADDSTRING, 0, (LPARAM) g_szCOMPort[l_wTemp1]);
    }

    /* Select first COM by default. */
    SendDlgItemMessage (p_hwndDlg, IDCOMNUMBER, CB_SETCURSEL, 0, 0);

    /* Initialize COM. */
    bSerialInit (p_hwndDlg);
  }
  else
  {
    /* Disable acquire button. */
    EnableWindow ( GetDlgItem(p_hwndDlg, IDACQUIRE), FALSE);
  }

  /* Save handle to main windows. */
  tracevisu.p_hwndDlg = p_hwndDlg;

  /* Print capture title. */
  SendDlgItemMessage (p_hwndDlg, IDTITLE, WM_SETTEXT, 0, (LPARAM) tracevisu.szTitle);

  /* Initialize trigger source. */
  for (l_wTemp1 = PROBE1; l_wTemp1 < TOTAL_PROBES; l_wTemp1++)
  {
    /* Add probes number to combo box. */
    sprintf (l_szTemp, "PROBE%i", l_wTemp1+1);
    SendDlgItemMessage(p_hwndDlg, IDTRIGGERSRC, CB_ADDSTRING, 0, (LPARAM) l_szTemp);
  }

  /* Select first trigger source. */
  tracevisu.dwProbeTrigger = PROBE16;
  SendDlgItemMessage(p_hwndDlg, IDTRIGGERSRC, CB_SETCURSEL, tracevisu.dwProbeTrigger, 0);
  vUpdateTrigger (p_hwndDlg, &tracevisu, TRUE);

  /* For each trace. */
  for (l_wTemp1 = TRACE1; l_wTemp1 < TOTAL_TRACES; l_wTemp1++)
  {
    /* Initialize trace color. */
    tracevisu.trace[l_wTemp1].hColor = hReplaceColor (g_hProbeColor, 0xffffff, tracevisu.trace[l_wTemp1].dwColor, GetDC(p_hwndDlg));
    SendDlgItemMessage(p_hwndDlg, tracevisu.trace[l_wTemp1].dwIDColor, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)tracevisu.trace[l_wTemp1].hColor);

    /* Initialize probe number. */
    for (l_wTemp2 = PROBE1; l_wTemp2 < TOTAL_PROBES; l_wTemp2++)
    {
      /* Add probes number to combo box. */
      sprintf (l_szTemp, "%i", l_wTemp2+1);
      SendDlgItemMessage(p_hwndDlg, tracevisu.trace[l_wTemp1].dwIDProbeNumber, CB_ADDSTRING, 0, (LPARAM) l_szTemp);
    }

    /* Select probe number. */
    SendDlgItemMessage(p_hwndDlg, tracevisu.trace[l_wTemp1].dwIDProbeNumber, CB_SETCURSEL, tracevisu.trace[l_wTemp1].dwProbeNumber, 0);

    /* Init trace name. */
    SendDlgItemMessage (p_hwndDlg, tracevisu.trace[l_wTemp1].dwIDName, WM_SETTEXT, 0, (LPARAM) tracevisu.trace[l_wTemp1].szName);

    /* Init trace state. */
    SendDlgItemMessage (p_hwndDlg, tracevisu.trace[l_wTemp1].dwIDState, BM_SETCHECK, tracevisu.trace[l_wTemp1].bState, 0);
  }

  /* Initialize Logic screen (OpenGL window). */
  g_hDCLogicScreen = GetDC ( GetDlgItem(p_hwndDlg, IDSTATICVISU));
  ZeroMemory ( &g_pfdLogicScreen, sizeof (g_pfdLogicScreen));
  g_pfdLogicScreen.nSize = sizeof (g_pfdLogicScreen);
  g_pfdLogicScreen.nVersion = 1;
  g_pfdLogicScreen.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  g_pfdLogicScreen.iPixelType = PFD_TYPE_RGBA;
  g_pfdLogicScreen.cColorBits = 32;
  g_pfdLogicScreen.cRedBits = 8;
  g_pfdLogicScreen.cRedShift = 16;
  g_pfdLogicScreen.cGreenBits = 8;
  g_pfdLogicScreen.cGreenShift = 8;
  g_pfdLogicScreen.cBlueBits = 8;
  g_pfdLogicScreen.cBlueShift = 0;
  g_pfdLogicScreen.cAlphaBits = 0;
  g_pfdLogicScreen.cAlphaShift = 0;
  g_pfdLogicScreen.cAccumBits = 64;
  g_pfdLogicScreen.cAccumRedBits = 16;
  g_pfdLogicScreen.cAccumGreenBits = 16;
  g_pfdLogicScreen.cAccumBlueBits = 16;
  g_pfdLogicScreen.cAccumAlphaBits = 0;
  g_pfdLogicScreen.cDepthBits = 32;
  g_pfdLogicScreen.cStencilBits = 8;
  g_pfdLogicScreen.cAuxBuffers = 0;
  g_pfdLogicScreen.iLayerType = PFD_MAIN_PLANE;
  g_pfdLogicScreen.bReserved = 0;
  g_pfdLogicScreen.dwLayerMask = 0;
  g_pfdLogicScreen.dwVisibleMask = 0;
  g_pfdLogicScreen.dwDamageMask = 0;
  g_i32FormatLogicScreen = ChoosePixelFormat ( g_hDCLogicScreen, &g_pfdLogicScreen );
  SetPixelFormat ( g_hDCLogicScreen, g_i32FormatLogicScreen, &g_pfdLogicScreen );
  g_hRCLogicScreen = wglCreateContext (g_hDCLogicScreen);

  /* Initialize capture delay. */
  g_wCaptureDelay = DEFAULT_CAPTURE_DELAY;

  /* Initialize range and position of capture delay. */
  SendDlgItemMessage (p_hwndDlg, IDCAPTUREDELAY, TBM_SETRANGE, (WPARAM) 1, (LPARAM) MAKELONG(-32768,32767));
  SendDlgItemMessage (p_hwndDlg, IDCAPTUREDELAY, TBM_SETPOS, (WPARAM) 1, (LPARAM) (g_wCaptureDelay - 32768));

  /* Update static capture delay. */
  sprintf (l_szTemp,"%01.2f",(float) g_wCaptureDelay);
  SendDlgItemMessage(p_hwndDlg, IDEDITCAPTUREDELAY, WM_SETTEXT, 0, (LPARAM)l_szTemp);

  /* Initialize Timebase. */
  g_fTimeBase = 0.0625f*DEFAULT_TIMEBASE;

  /* Initialize range and position of slide Timebase. */
  SendDlgItemMessage (p_hwndDlg, IDTIMEBASE, TBM_SETRANGE, (WPARAM) 1, (LPARAM) MAKELONG(0,(tracevisu.dwCaptureSize>>4) - 1));
  SendDlgItemMessage (p_hwndDlg, IDTIMEBASE, TBM_SETPOS, (WPARAM) 1, (LPARAM) (DEFAULT_TIMEBASE-1));

  /* Update static Timebase. */
  sprintf (l_szTemp,"%01.3f us",(float) g_fTimeBase);
  SendDlgItemMessage(p_hwndDlg, IDTIMEBASEVALUE, WM_SETTEXT, 0, (LPARAM)l_szTemp);

  /* Initialize range and position of Time line slider. */
  l_wTemp1 = (g_fTimeBase * NBR_DIVISION * tracevisu.dwCaptureSize) / (tracevisu.dwCaptureSize * g_fBoardSamplePeriod);
  l_wTemp1 = tracevisu.dwCaptureSize - l_wTemp1 - 2;
  g_wLogicFormOffset = 0.0f;
  SendDlgItemMessage (p_hwndDlg, IDDELAYTIME, TBM_SETRANGE, (WPARAM) 1, (LPARAM) MAKELONG(0,l_wTemp1) );
  SendDlgItemMessage (p_hwndDlg, IDDELAYTIME, TBM_SETPOS, (WPARAM) 1, (LPARAM) (g_wLogicFormOffset) );

  /* Update delay time static. */
  sprintf (l_szTemp,"%01.02f",(float)g_wLogicFormOffset * g_fBoardSamplePeriod);
  SendDlgItemMessage(p_hwndDlg, IDEDITDELAYTIME, WM_SETTEXT, 0, (LPARAM)l_szTemp);

  /* Update time static. */
  l_fTemp = 0.0f;
  for (l_wTemp1 = IDS_TIME0; l_wTemp1 <= IDS_TIME16; l_wTemp1++)
  {
    sprintf (l_szTemp,"%01.02fus",l_fTemp);
    SendDlgItemMessage(p_hwndDlg, l_wTemp1, WM_SETTEXT, 0, (LPARAM)l_szTemp);
    l_fTemp += g_fTimeBase;
  }

  /* Update memory depth. */
  for (l_wTemp1 = 0; l_wTemp1 < 14; l_wTemp1++)
  {
    sprintf (l_szTemp,"%i", 1<<l_wTemp1);
    SendDlgItemMessage(p_hwndDlg, IDMEMORYDEPTH, CB_ADDSTRING, 0, (LPARAM) l_szTemp);
  }

  /* Select memory depth. */
  SendDlgItemMessage (p_hwndDlg, IDMEMORYDEPTH, CB_SETCURSEL, DEFAULT_MEMORY_DEPTH, 0);
  g_dwMemoryDepth = (1<<DEFAULT_MEMORY_DEPTH);

  /* Initialize video timer. */
  SetTimer (p_hwndDlg, g_uiTimerID, 50, NULL);

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bMsgTimer (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  TCHAR l_szTemp[MAX_PATH] = "";

  /* Is video timer ?*/
  if (g_uiTimerID == p_wParam)
  {
    /* Render only if serial thread disable. */
    if (g_bSerialThreadEnable == FALSE)
    {
      /* Update capture frame. */
      sprintf (l_szTemp, "%li", tracevisu.dwCaptureSize);
      SendDlgItemMessage (p_hwndDlg, IDDEBUG, WM_SETTEXT, 0, (LPARAM) l_szTemp);

      /* Rendering logic screen. */
      vLogicScreenOpenGLRender ();

      /* Video render done. */
      l_bReturn = TRUE;
    }
    else
    {
      /* Nothing to do. */
    }
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bMsgHScroll (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  TCHAR l_szTemp[MAX_PATH] = "";

  /* Is Fc changed ? */
  if ((LPARAM) GetDlgItem (p_hwndDlg, IDDELAYTIME) == p_lParam)
  {
    /* Get slide position. */
    g_wLogicFormOffset = SendDlgItemMessage (p_hwndDlg, IDDELAYTIME,TBM_GETPOS, 0, 0);

    /* Update delay time static. */
    sprintf (l_szTemp,"%01.02f",(float)g_wLogicFormOffset * 5.0f/72.0f);
    SendDlgItemMessage(p_hwndDlg, IDEDITDELAYTIME, WM_SETTEXT, 0, (LPARAM)l_szTemp);

    l_bReturn = TRUE;
  }
  else if ((LPARAM) GetDlgItem (p_hwndDlg, IDCAPTUREDELAY) == p_lParam)
  {
    /* Get slide position. */
    g_wCaptureDelay = SendDlgItemMessage (p_hwndDlg, IDCAPTUREDELAY,TBM_GETPOS, 0, 0) + 32768;

    /* Update static capture delay. */
    if (g_wCaptureDelay > 0)
    {
      sprintf (l_szTemp, "%01.2f", ((float) (g_wCaptureDelay - 1) * 4.0/72.0) + 3.0/72.0);
    }
    else
    {
      sprintf (l_szTemp, "%01.2f", 0.0f);
    }

    SendDlgItemMessage(p_hwndDlg, IDEDITCAPTUREDELAY, WM_SETTEXT, 0, (LPARAM)l_szTemp);

    /* Configuration modified. */
    g_bConfigurationChanged = TRUE;

    l_bReturn = TRUE;
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bMsgVScroll (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  WORD l_wTemp1;
  TCHAR l_szTemp[MAX_PATH] = "";
  FLOAT l_fTemp;

  /* If Fc change */
  if ((LPARAM) GetDlgItem(p_hwndDlg,IDTIMEBASE) == p_lParam)
  {
    /* Get slide position */
    g_fTimeBase = (SendDlgItemMessage(p_hwndDlg, IDTIMEBASE,TBM_GETPOS, 0, 0) + 1)*0.0625;
    sprintf (l_szTemp,"%01.3fus/div", g_fTimeBase);
    SendDlgItemMessage(p_hwndDlg, IDTIMEBASEVALUE, WM_SETTEXT, 0, (LPARAM)l_szTemp);

    /* Update time static */
    l_fTemp = 0.0f;
    for (l_wTemp1 = IDS_TIME0; l_wTemp1 <= IDS_TIME16; l_wTemp1++)
    {
      sprintf (l_szTemp,"%01.02fus",l_fTemp);
      SendDlgItemMessage(p_hwndDlg, l_wTemp1, WM_SETTEXT, 0, (LPARAM)l_szTemp);
      l_fTemp += g_fTimeBase;
    }

    /* Compute Time delay slide */
    l_wTemp1 = (g_fTimeBase * NBR_DIVISION * tracevisu.dwCaptureSize) / (tracevisu.dwCaptureSize * g_fBoardSamplePeriod);
    l_wTemp1 = tracevisu.dwCaptureSize - l_wTemp1 - 2;

    /* Crop */
    if (g_wLogicFormOffset > l_wTemp1)
    {
      /* Max position */
      g_wLogicFormOffset = l_wTemp1;
      SendDlgItemMessage (p_hwndDlg, IDDELAYTIME, TBM_SETPOS, (WPARAM) 1, (LPARAM) (g_wLogicFormOffset) );

      /* Update timeline */
      SendMessage (p_hwndDlg, WM_HSCROLL, 0, (LPARAM) GetDlgItem (p_hwndDlg, IDDELAYTIME));
    }

    /* Update Time delay slide. */
    SendDlgItemMessage (p_hwndDlg, IDDELAYTIME, TBM_SETRANGE, (WPARAM) 1, (LPARAM) MAKELONG (0, l_wTemp1) );

    l_bReturn = TRUE;
  }
  else
  {
    /* Nothing to do. */
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bCommandComboProbeNumber (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  UINT l_u8Trace = LOWORD(p_wParam) - IDNUMBERPROBE1;

  /* Is selection changed ? */
  if (CBN_SELCHANGE == HIWORD(p_wParam) )
  {
    /* Get new probe number. */
    tracevisu.trace[l_u8Trace].dwProbeNumber = SendDlgItemMessage (p_hwndDlg, LOWORD(p_wParam),CB_GETCURSEL, 0, 0);

    /* Update trigger. */
    vUpdateTrigger (p_hwndDlg, &tracevisu, TRUE);

    l_bReturn = TRUE;
  }
  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bCommandButtonTriggerTrace (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  UINT l_u8Trace = LOWORD(p_wParam) - IDTRIGGERTRACE1;

  {
    if (tracevisu.trace[l_u8Trace].dwProbeNumber == tracevisu.dwProbeTrigger)
    {
      vUpdateTrigger (p_hwndDlg, &tracevisu, FALSE);
    }

    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bCommandButtonColorTrace (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  UINT l_u8Trace = LOWORD(p_wParam) - IDCOLORTRACE1;

  {
    vUpdateColorTrace (p_hwndDlg, &tracevisu, l_u8Trace);

    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bCommandCheckboxStateTrace (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
  UINT l_u8Trace = LOWORD(p_wParam) - IDSTATETRACE1;

  {
    tracevisu.trace[l_u8Trace].bState ^= BST_CHECKED;

    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bCommandComboMemoryDepth (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  /* Is selection changed ? */
  if (HIWORD(p_wParam) == CBN_SELCHANGE)
  {
    /* Save new memory depth. */
    g_dwMemoryDepth = 1<<(SendDlgItemMessage (p_hwndDlg, IDMEMORYDEPTH,CB_GETCURSEL, 0, 0));

    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bCommandComboComNumber (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  /* Is selection changed ? */
  if (HIWORD(p_wParam) == CBN_SELCHANGE)
  {
    /* Try to init new COM. */
    g_bCOMAvailable = bSerialInit (p_hwndDlg);

    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*===================================================================================================
Function  :  v_LoadData

Describe  :   Load capture file (*.ULA)

Parameters  :   p_hwndDlg => handle of the main dialog box

Returns    :   None
-----------------------------------------------------------------------------------------------------
Revision A  :  Creation
===================================================================================================*/
BOOL bLoadData (HWND p_hwndDlg, s_TraceVisu_t *ptracevisu)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  {
    // Locals variables definitions
    HANDLE hFile;
    DWORD dwReadBytes;
    DWORD l_wTemp1;
    s_TraceVisu_t local;
    OPENFILENAME LoadFile;
    TCHAR szFileName[MAX_PATH] = "\0";
    TCHAR szFilePath[MAX_PATH] = "\0";
    TCHAR l_szTemp[MAX_PATH];

    // Update OPENFILENAME structure
    ZeroMemory (&LoadFile, sizeof(LoadFile));
    LoadFile.lStructSize = sizeof(LoadFile);
    LoadFile.lpstrFile = szFilePath;
    LoadFile.lpstrFile[0] = '\0';
    LoadFile.hwndOwner = p_hwndDlg;
    LoadFile.nMaxFile = sizeof(szFilePath);
    LoadFile.lpstrFilter = TEXT("ULA files(*.*)\0*.ULA\0");
    LoadFile.nFilterIndex = 1;
    LoadFile.lpstrInitialDir = NULL;
    LoadFile.lpstrFileTitle = szFileName;
    LoadFile.nMaxFileTitle = sizeof(szFileName);
    LoadFile.Flags = OFN_FILEMUSTEXIST;
    if(GetOpenFileName(&LoadFile) != FALSE)
    {
        // Open savefile
        hFile = CreateFile (szFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // Read header
            ReadFile (hFile, &local,  sizeof(s_TraceVisu_t), &dwReadBytes, NULL);

            // If a ULA file
            if (strncmp (local.szID, "ULA", 3) == 0)
            {
                // Update capture title
                strcpy(ptracevisu->szTitle, local.szTitle);
                SendDlgItemMessage (p_hwndDlg, IDTITLE, WM_SETTEXT, 0, (LPARAM) ptracevisu->szTitle);

                // Update trace
                for (l_wTemp1 = TRACE1; l_wTemp1 < TOTAL_TRACES; l_wTemp1++)
                {
                    // Update trace state
                    ptracevisu->trace[l_wTemp1].bState = local.trace[l_wTemp1].bState;
                    SendDlgItemMessage (p_hwndDlg, ptracevisu->trace[l_wTemp1].dwIDState, BM_SETCHECK, ptracevisu->trace[l_wTemp1].bState, 0);

                    // Update trace name
                    strcpy(ptracevisu->trace[l_wTemp1].szName, local.trace[l_wTemp1].szName);
                    SendDlgItemMessage (p_hwndDlg, ptracevisu->trace[l_wTemp1].dwIDName , WM_SETTEXT, 0, (LPARAM) local.trace[l_wTemp1].szName);

                    // Update probe number
                    ptracevisu->trace[l_wTemp1].dwProbeNumber = local.trace[l_wTemp1].dwProbeNumber;
                    SendDlgItemMessage (p_hwndDlg, ptracevisu->trace[l_wTemp1].dwIDProbeNumber, CB_SETCURSEL, ptracevisu->trace[l_wTemp1].dwProbeNumber, 0);

                    // Update trace color
                    ptracevisu->trace[l_wTemp1].dwColor = local.trace[l_wTemp1].dwColor;
                    ptracevisu->trace[l_wTemp1].hColor = hReplaceColor (g_hProbeColor, 0xffffff, ptracevisu->trace[l_wTemp1].dwColor, GetDC(p_hwndDlg));
                    SendDlgItemMessage (p_hwndDlg, ptracevisu->trace[l_wTemp1].dwIDColor, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)ptracevisu->trace[l_wTemp1].hColor);
                }

                // Update trigger source
                ptracevisu->dwProbeTrigger = local.dwProbeTrigger;
                ptracevisu->byTriggerType = local.byTriggerType^0x01;
                vUpdateTrigger (p_hwndDlg, ptracevisu, FALSE);

                // Update capture data
                ptracevisu->dwCaptureSize = local.dwCaptureSize;
                ReadFile (hFile, (void *)g_pdwCapture , ptracevisu->dwCaptureSize*sizeof(DWORD), &dwReadBytes, NULL);

                 // Update static Timebase
                sprintf (l_szTemp,"%01.3f us//div",(float) g_fTimeBase);
                SendDlgItemMessage(p_hwndDlg, IDTIMEBASEVALUE, WM_SETTEXT, 0, (LPARAM)l_szTemp);

                // Update timebase
                SendDlgItemMessage (p_hwndDlg, IDTIMEBASE, TBM_SETPOS, (WPARAM) 1, (LPARAM) (DEFAULT_TIMEBASE-1));
                SendDlgItemMessage (p_hwndDlg, IDTIMEBASE, TBM_SETRANGE, (WPARAM) 1, (LPARAM) MAKELONG(0,(local.dwCaptureSize>>4) - 1));

                // Update visu delay
                l_wTemp1 = (g_fTimeBase * NBR_DIVISION * local.dwCaptureSize) / (local.dwCaptureSize * g_fBoardSamplePeriod);
                l_wTemp1 = local.dwCaptureSize - l_wTemp1 - 2;
                g_wLogicFormOffset = 0.0f;
                SendDlgItemMessage (p_hwndDlg, IDDELAYTIME, TBM_SETPOS, (WPARAM) 1, (LPARAM) (g_wLogicFormOffset) );
                SendDlgItemMessage (p_hwndDlg, IDDELAYTIME, TBM_SETRANGE, (WPARAM) 1, (LPARAM) MAKELONG(0,l_wTemp1) );

                // Update delay time static
                sprintf (l_szTemp,"%01.02f",(float)g_wLogicFormOffset * g_fBoardSamplePeriod);
                SendDlgItemMessage(p_hwndDlg, IDEDITDELAYTIME, WM_SETTEXT, 0, (LPARAM)l_szTemp);

                // Send WM_VSCROLL message to update timebase
                SendMessage (p_hwndDlg, WM_VSCROLL, 0, (LPARAM) GetDlgItem(p_hwndDlg,IDTIMEBASE));
            }

            // Close file
            CloseHandle (hFile);
        }
    }
    else
    {
        MessageBox (p_hwndDlg,"Error : Not a ULA files !", "Open capture", MB_ICONERROR | MB_OK);
    }
    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*==============================================================================
Function    : v_SaveData.

Describe    : Save capture file (*.ULA).

Parameters  : p_hwndDlg => handle of the main dialog box

Returns     : None
==============================================================================*/
BOOL bSaveData (HWND p_hwndDlg, s_TraceVisu_t *ptracevisu)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  {
    // Locals variables definitions
    HANDLE hFile;
    DWORD dwWrittenBytes;
    WORD l_wTemp1;
    OPENFILENAME SaveFile;
    TCHAR szFileName[MAX_PATH] = "\0";
    TCHAR szFilePath[MAX_PATH] = "\0";

    // Update OPENFILENAME structure
    ZeroMemory (&SaveFile, sizeof(SaveFile));
    SaveFile.lStructSize = sizeof(SaveFile);
    SaveFile.lpstrFile = szFilePath;
    SaveFile.lpstrFile[0] = '\0';
    SaveFile.hwndOwner = p_hwndDlg;
    SaveFile.nMaxFile = sizeof(szFilePath);
    SaveFile.lpstrFilter = TEXT("ULA files(*.*)\0*.ULA\0");
    SaveFile.nFilterIndex = 1;
    SaveFile.lpstrInitialDir = NULL;
    SaveFile.lpstrFileTitle = szFileName;
    SaveFile.nMaxFileTitle = sizeof(szFileName);
    SaveFile.Flags = 0;
    if(GetSaveFileName (&SaveFile) != FALSE)
    {
        // If no extension
        if (SaveFile.nFileExtension == 0)
        {
            // Add extension to end filename
            strcat (szFilePath, TEXT(".ULA"));
        }

        // Get Capture title
        SendDlgItemMessage (p_hwndDlg, IDTITLE, WM_GETTEXT, MAX_PATH, (LPARAM) ptracevisu->szTitle);

        // Get trace name
        for (l_wTemp1 = TRACE1; l_wTemp1 < TOTAL_TRACES; l_wTemp1++)
        {
            SendDlgItemMessage (p_hwndDlg, ptracevisu->trace[l_wTemp1].dwIDName, WM_GETTEXT, MAX_PATH, (LPARAM) ptracevisu->trace[l_wTemp1].szName);
        }

        // Open file for write
        hFile = CreateFile (szFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // Write Header
            WriteFile (hFile, ptracevisu, sizeof (s_TraceVisu_t), &dwWrittenBytes, NULL);

            // Write Capture data
            WriteFile (hFile, (void*) g_pdwCapture, ptracevisu->dwCaptureSize * sizeof (DWORD), &dwWrittenBytes, NULL);

            // Close file
            CloseHandle (hFile);
        }
    }
    l_bReturn = TRUE;
  }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bCommandButtonAcquire (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;
/* Is button clicked ? */
      if (BN_CLICKED == HIWORD(p_wParam) )
      {
        // If com available and button clicked
        if (   (g_bCOMAvailable == TRUE)
            && (g_bSerialThreadEnable == FALSE))
        {
          // If configuration changed
          if (g_bConfigurationChanged == TRUE)
          {
            // Send COMMAND_CONFIGURATION
            bSerialSendCommand (p_hwndDlg, &tracevisu, COMMAND_CONFIGURATION);

            // Configuration modified
            g_bConfigurationChanged = FALSE;
          }

          // Send COMMAND_SAMPLING
          bSerialSendCommand (p_hwndDlg, &tracevisu, COMMAND_SAMPLING);
        }

        l_bReturn = TRUE;
      }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bCommandComboTriggerSource (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

      // If change notification
      if (HIWORD(p_wParam) == CBN_SELCHANGE)
      {
        // Update new trigger source
        tracevisu.dwProbeTrigger = SendDlgItemMessage (p_hwndDlg, IDTRIGGERSRC,CB_GETCURSEL, 0, 0);

        // Update trigger
        vUpdateTrigger (p_hwndDlg, &tracevisu, TRUE);
        l_bReturn = TRUE;
      }

  return (l_bReturn);
}


/*==============================================================================
Function    :

Describe    : .

Parameters  : .

Returns     : .
==============================================================================*/
BOOL bMsgCommand (HWND p_hwndDlg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  /* Command dispatch. */
  switch (LOWORD(p_wParam))
  {
    case IDMEMORYDEPTH: {
      l_bReturn = bCommandComboMemoryDepth (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case IDCOMNUMBER: {
      l_bReturn = bCommandComboComNumber (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case IDNUMBERPROBE1:
    case IDNUMBERPROBE2:
    case IDNUMBERPROBE3:
    case IDNUMBERPROBE4:
    case IDNUMBERPROBE5:
    case IDNUMBERPROBE6:
    case IDNUMBERPROBE7:
    case IDNUMBERPROBE8:
    case IDNUMBERPROBE9:
    case IDNUMBERPROBE10:
    case IDNUMBERPROBE11:
    case IDNUMBERPROBE12:
    case IDNUMBERPROBE13:
    case IDNUMBERPROBE14:
    case IDNUMBERPROBE15:
    case IDNUMBERPROBE16: {
      l_bReturn = bCommandComboProbeNumber (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case IDSAVEDATA: {
      l_bReturn = bSaveData (p_hwndDlg, &tracevisu);
      break;
    }
    case IDLOADDATA: {
      l_bReturn = bLoadData (p_hwndDlg, &tracevisu);
      break;
    }
    case IDACQUIRE: {
      l_bReturn = bCommandButtonAcquire (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case IDTRIGGERSRC: {
      l_bReturn = bCommandComboTriggerSource (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case IDTRIGGERTRACE1:
    case IDTRIGGERTRACE2:
    case IDTRIGGERTRACE3:
    case IDTRIGGERTRACE4:
    case IDTRIGGERTRACE5:
    case IDTRIGGERTRACE6:
    case IDTRIGGERTRACE7:
    case IDTRIGGERTRACE8:
    case IDTRIGGERTRACE9:
    case IDTRIGGERTRACE10:
    case IDTRIGGERTRACE11:
    case IDTRIGGERTRACE12:
    case IDTRIGGERTRACE13:
    case IDTRIGGERTRACE14:
    case IDTRIGGERTRACE15:
    case IDTRIGGERTRACE16: {
      l_bReturn = bCommandButtonTriggerTrace (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case IDCOLORTRACE1:
    case IDCOLORTRACE2:
    case IDCOLORTRACE3:
    case IDCOLORTRACE4:
    case IDCOLORTRACE5:
    case IDCOLORTRACE6:
    case IDCOLORTRACE7:
    case IDCOLORTRACE8:
    case IDCOLORTRACE9:
    case IDCOLORTRACE10:
    case IDCOLORTRACE11:
    case IDCOLORTRACE12:
    case IDCOLORTRACE13:
    case IDCOLORTRACE14:
    case IDCOLORTRACE15:
    case IDCOLORTRACE16: {
      l_bReturn = bCommandButtonColorTrace (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case IDSTATETRACE1:
    case IDSTATETRACE2:
    case IDSTATETRACE3:
    case IDSTATETRACE4:
    case IDSTATETRACE5:
    case IDSTATETRACE6:
    case IDSTATETRACE7:
    case IDSTATETRACE8:
    case IDSTATETRACE9:
    case IDSTATETRACE10:
    case IDSTATETRACE11:
    case IDSTATETRACE12:
    case IDSTATETRACE13:
    case IDSTATETRACE14:
    case IDSTATETRACE15:
    case IDSTATETRACE16: {
      l_bReturn = bCommandCheckboxStateTrace (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    default: {
      /* Nothing to do. */
      break;
    }
  }
  return (l_bReturn);
}


/*==============================================================================
Function    : bMainDialogCallback.

Describe    :  .

Parameters  :

Returns     :
==============================================================================*/
BOOL CALLBACK bMainDialogCallback (HWND p_hwndDlg, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
{
  /* Locals variables declaration. */
  BOOL l_bReturn = FALSE;

  /* Dispatch message. */
  switch (p_uMsg)
  {
    case WM_INITDIALOG: {
      l_bReturn = bMsgInitDialog (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case WM_CLOSE: {
      l_bReturn = bMsgClose (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case WM_TIMER: {
      l_bReturn = bMsgTimer (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case WM_HSCROLL: {
      l_bReturn = bMsgHScroll (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case WM_VSCROLL: {
      l_bReturn = bMsgVScroll (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    case WM_COMMAND: {
      l_bReturn = bMsgCommand (p_hwndDlg, p_wParam, p_lParam);
      break;
    }
    default: {
      /* Nothing to do. */
      break;
    }
  }

  return (l_bReturn);
}

