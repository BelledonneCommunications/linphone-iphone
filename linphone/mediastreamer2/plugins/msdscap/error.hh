/* HornetsEye - Computer Vision with Ruby
   Copyright (C) 2006, 2007   Jan Wedekind
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */
#ifndef HORNETSEYE_ERROR_HH
#define HORNETSEYE_ERROR_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <exception>
#include <sstream>
#include <string>
#ifdef WIN32
#include <stdio.h>
#include <windef.h>
#include <wchar.h>
#include <winsock.h>
#endif

namespace Hornetseye {

  /** Exception class deriving from std::exception.
      This class provides a syntax similar to output-streams for convenience.
      For compability with other libraries it is inheriting the class
      \c std::exception.
    
      Here is an example how to use an instance of this class in C++:
      \include exceptiontest/exceptiontest.cc
      
      Exception-handling also can be done using the macro \c ERRORMACRO:
      \include exceptiontest/macrotest.cc
      
      Mind that the macro uses a variable with the name \c _e. Make sure, that
      you don't use this variable-name in any of the macro-arguments!

      Ruby already comes with exception classes:
      \include exceptiontest/exceptiontest.rb
      
      @date Mon Aug 23 14:37:05 UTC 2004 */
  class Error: public std::exception
  {
  public:
    /// Constructor.
    Error(void) {}
    /// Copy constructor.
    Error( Error &e ): std::exception( e )
      { m_message << e.m_message.str(); }
    /// Destructor.
    virtual ~Error(void) throw() {}
    ///
    template< typename T >
    std::ostream &operator<<( const T &t )
      { m_message << t; return m_message; }
    /** Interface for manipulators.
        Manipulators such as \c std::endl and \c std::hex use these
        functions in constructs like "Error e; e << std::endl".
        For more information, see the iomanip header. */
    std::ostream &operator<<( std::ostream& (*__pf)( std::ostream&) )
      { (*__pf)( m_message ); return m_message; }
    /// Returns error message (not thread safe).
    virtual const char* what(void) const throw() {
      temp = m_message.str();
      return temp.c_str();
      return NULL;
    }
  protected:
    /// Memory-stream containing the error message.
    std::ostringstream m_message;
    /** Temporary to do null-termination.
        The method \c what() requires a null-terminated string. */
    static std::string temp;
  };
  
};

#define ERRORMACRO( condition, class, params, message ) \
  if ( !( condition ) ) {                               \
    class _e params;                                    \
    _e << message;                                      \
    throw _e;                                           \
  };

#ifdef WIN32
#define COERRORMACRO( condition, class, params, message )                     \
  {                                                                           \
    HRESULT _hr = condition;                                                  \
    if ( FAILED( _hr ) ) {                                                    \
      class _e params;                                                        \
      _e << message;                                                          \
      TCHAR *_msg;                                                            \
      if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |                    \
                          FORMAT_MESSAGE_FROM_SYSTEM, 0, _hr, 0,              \
                          (LPTSTR)&_msg, 0, NULL ) != 0 ) {                   \
        _e << ": " << _msg;                                                   \
        LocalFree( _msg );                                                    \
      };                                                                      \
      throw _e;                                                               \
    };                                                                        \
  };

#define W32ERRORMACRO( condition, class, params, message )                    \
  {                                                                           \
    if ( !( condition ) ) {                                                   \
      class _e params;                                                        \
      _e << message;                                                          \
      TCHAR *_msg;                                                            \
      DWORD _errCode = GetLastError();                                        \
      if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |                    \
                          FORMAT_MESSAGE_FROM_SYSTEM, 0, _errCode, 0,         \
                          (LPTSTR)&_msg, 0, NULL ) != 0 ) {                   \
        _e << ": " << _msg;                                                   \
        LocalFree( _msg );                                                    \
      };                                                                      \
      throw _e;                                                               \
    };                                                                        \
  };

#endif

#endif
