//
// This Software uses the 
// PLX API libraries, Copyright (c) 2003 PLX Technology, Inc.
//

#include "ConsoleUtilities.h"
//#include "PlxTypes.h"

#include "Specs.h"

static unsigned short _Gbl_LineCountMax    = 
  DEFAULT_SCREEN_SIZE - SCREEN_THROTTLE_OFFSET;

// ============================================================================
// Initialize Console
// ============================================================================
void ConsoleInitialize( ) {
#if defined(PLX_LINUX)

  WINDOW *pActiveWin;
  
  
  // Initialize console
  pActiveWin = initscr();
  
  // Allow insert/delete of lines (for scrolling)
  idlok( pActiveWin, TRUE ) ;

  // Allow console to scroll
  scrollok( pActiveWin, TRUE ) ;

  // Disable buffering to provide input immediately to app
  cbreak();
   
  // getch will echo characters
  echo();
  
  // Enable blocking mode (for getch & scanw)
  nodelay( stdscr, FALSE ) ;

  // Set the max line count based on current screen size
  _Gbl_LineCountMax = getmaxy(stdscr) - SCREEN_THROTTLE_OFFSET;
  
#elif defined(_WIN32)

    CONSOLE_SCREEN_BUFFER_INFO csbi;


    // Get the current console information
    GetConsoleScreenBufferInfo(
        GetStdHandle(STD_OUTPUT_HANDLE),
        &csbi
        );

    // Set the max line count based on current screen size
    _Gbl_LineCountMax = csbi.dwSize.Y - SCREEN_THROTTLE_OFFSET;
  
#endif
}

// ============================================================================
// 
// ============================================================================
void ConsoleFinalize( ) {
#if defined(PLX_LINUX)
  
  // Restore console
  endwin();
  
#endif
}

// ============================================================================
// 
// ============================================================================
void SpecsIoWriteString( const char *pStr ) {
  while (*pStr != '\0') {
    SpecsIoWriteByte( *pStr ) ;
    
    pStr++;
  }        
}

// ============================================================================
// 
// ============================================================================
void SpecsPrintf( const char *format, ... ) {
  int          i;
  int          Width;
  int          Precision;
  char         pFormatSpec[16];
  char         pOutput[MAX_DECIMAL_BUFFER_SIZE];
  va_list      pArgs;
  const char  *pFormat;
  signed char  Size;
  signed char  Flag_64_bit;
  signed char  FlagDirective;
  signed char  Flag_LeadingZero;
  U64    value;

  // Initialize the optional arguments pointer
  va_start( pArgs, format ) ;
  
  pFormat = format;

  while (*pFormat != '\0') {
    if (*pFormat != '%') {
      // Non-field, just print it
      SpecsIoWriteByte( *pFormat ) ;
    } else {
      // Reached '%' character
      Size             = -1;
      Width            = -1;
      Precision        = -1;
      Flag_64_bit      = 0;
      FlagDirective    = -1;
      Flag_LeadingZero = -1;
      
      pFormat++;
      
      // Check for any flag directives
      if ( (*pFormat == '-') || (*pFormat == '+') ||
           (*pFormat == '#') || (*pFormat == ' ') ) {
        FlagDirective = *pFormat;
        
        pFormat++;
      }
      
      // Check for a width specification
      while ((*pFormat >= '0') && (*pFormat <= '9')) {
        // Check for leading 0
        if (Flag_LeadingZero == -1) {
          if (*pFormat == '0') Flag_LeadingZero = 1;
          else Flag_LeadingZero = 0;
        }
        
        if (Width == -1) Width = 0;
        
        // Build the width
        Width = (Width * 10) + (*pFormat - '0');
        
        pFormat++;
      }
      
      // Check for a precision specification
      if (*pFormat == '.') {
        pFormat++;
        
        Precision = 0;
        
        // Get precision
        while ((*pFormat >= '0') && (*pFormat <= '9')) {
          // Build the Precision
          Precision = (signed char)(Precision * 10) + (*pFormat - '0');
          
          pFormat++;
        }
      }
      
      // Check for size specification
      if ( (*pFormat == 'l') || (*pFormat == 'L') ||
           (*pFormat == 'h') || (*pFormat == 'H') ) {
        Size = *pFormat;
        
        pFormat++;
      }
      else if ((*pFormat == 'q') || (*pFormat == 'Q')) {
        // 64-bit precision requested
        Flag_64_bit = 1;
        Size        = 'l';
        
        pFormat++;
      }
      
      if (*pFormat == '\0') break ;
      
      // Build the format specification
      i = 1;
      pFormatSpec[0] = '%';
      
      if (FlagDirective != -1) {
        pFormatSpec[i] = FlagDirective;
        i++;
      }
      
      if (Flag_LeadingZero == 1) {
        pFormatSpec[i] = '0';
        i++;
      }
      
      if ((Width != -1) && (Width != 0)) {
        // Split up widths if 64-bit data
        if (Flag_64_bit) {
          if (Width > 8) {
            Width = Width - 8;
          }
        }
        
        sprintf( &(pFormatSpec[i]), "%d", Width ) ;
        
        if (Width < 10) i += 1;
        else if (Width < 100) i += 2;
      }
      
      if ((Precision != -1) && (Precision != 0)) {
        pFormatSpec[i] = '.';
        i++;
        
        sprintf( &(pFormatSpec[i]), "%d", Precision ) ;
        
        if (Precision < 10) i += 1 ;
        else if (Precision < 100) i += 2 ;
      }
      
      if (Size != -1) {
        pFormatSpec[i] = Size;
        i++;
      }
      
      
      // Check type
      switch (*pFormat) {
      case 'd':
      case 'i':
        pFormatSpec[i] = *pFormat;
        i++;
        pFormatSpec[i] = '\0';
        
        // Convert value to string
        sprintf( pOutput, pFormatSpec, va_arg(pArgs, S32) ) ;
        
        // Display output
        SpecsIoWriteString( pOutput ) ;
        break;
        
      case 'x':
      case 'X':
        pFormatSpec[i] = *pFormat;
        i++;
        
        pFormatSpec[i] = '\0';
        
        // Display upper 32-bits if 64-bit data
        if (Flag_64_bit) {
          // Get data from stack
          value = va_arg( pArgs, U64 ) ;
          
          // Display value if not zero or a width is specified
          if (((value >> 32) != 0) ||
              ((Width != -1) && (Width != 0))) {
            // Prepare output string
            sprintf( pOutput, pFormatSpec, (U32)(value >> 32) ) ;
            
            // Display upper 32-bits
            SpecsIoWriteString( pOutput ) ;
            
            // Prepare format string for lower 32-bits
            sprintf( pFormatSpec, "%%08%c", *pFormat ) ;
            
            // Clear upper 32-bits
            value = (U32)value;
          }
        }
        else {
          // Get data from stack
          value = va_arg( pArgs, U32 ) ;
        }
        
        // Prepare output string
        sprintf( pOutput, pFormatSpec, (U32)value ) ;
    
        // Display 32-bit value
        SpecsIoWriteString( pOutput ) ;
        break;
        
      case 'c':
        // Display the character
        SpecsIoWriteByte( (char) va_arg(pArgs, int) ) ;
        break;
        
      case 'o':
      case 'u':
        pFormatSpec[i] = *pFormat;
        i++;
        pFormatSpec[i] = '\0';
        
        // Convert value to string
        sprintf( pOutput, pFormatSpec, va_arg(pArgs, unsigned int) ) ;
        
        // Display output
        SpecsIoWriteString( pOutput ) ;
        break ;
        
      case 's':
        SpecsIoWriteString( va_arg(pArgs, char *) ) ;
        break;
        
      default:
        // Display the character
        SpecsIoWriteByte( *pFormat ) ;
        break ;
      }
    }
    
    pFormat++;
  }
  
  va_end( pArgs ) ;
}


#if defined(_WIN32)
void ConsoleClear( void ) {
  HANDLE                     hConsole;
  COORD                      CoordScreen;
  DWORD                      cCharsWritten;
  DWORD                      dwWinSize;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  
  
  // Get handle to the console
  hConsole = GetStdHandle( STD_OUTPUT_HANDLE ) ;
  
  // Get the number of character cells in the current buffer
  GetConsoleScreenBufferInfo( hConsole, &csbi ) ;
  
  dwWinSize = csbi.dwSize.X * csbi.dwSize.Y;
  
  // Set screen coordinates
  CoordScreen.X = 0;
  CoordScreen.Y = 0;
  
  // Fill the entire screen with blanks
  FillConsoleOutputCharacter( hConsole, (TCHAR) ' ', dwWinSize, CoordScreen,
                              &cCharsWritten ) ;
  
  // Put the cursor at origin
  SetConsoleCursorPosition( hConsole, CoordScreen ) ;
}

#endif // _WIN32

