/* HornetsEye - Computer Vision with Ruby
   Copyright (C) 2006, 2007 Jan Wedekind
   
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
#ifndef HORNETSEYE_COMPTR_HH
#define HORNETSEYE_COMPTR_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32
#include <cassert>
#include <objbase.h>
#include "error.hh"

namespace Hornetseye {

  template< class I >
  class ComPtr
  {
  public:
    ComPtr(void): m_i(NULL) {}
    ComPtr( const ComPtr< I > &ptr ): m_i(ptr.get()) {
      if ( m_i != NULL ) m_i->AddRef();
    }
    ~ComPtr(void) { reset(); }
    ComPtr< I > &operator=( const ComPtr< I > &other ) {
      reset();
      m_i = other.get();
      if ( m_i != NULL ) m_i->AddRef();
      return *this;
    };
    I **operator&(void) {
      reset();
      return &m_i;
    }
    void coCreateInstance( REFCLSID clsid, REFIID iid, const char *errorText )
      throw (Error) {
      reset();
      COERRORMACRO( CoCreateInstance( clsid, NULL, CLSCTX_INPROC, iid,
                                      (void **)&m_i ), Error, , errorText );
    }
    I *get(void) const { return m_i; }
    I &operator*(void) {
      assert( m_i != NULL );
      return *m_i;
    }
    I *operator->(void) {
      assert( m_i != NULL );
      return m_i;
    }
    void reset(void) { if ( m_i != NULL ) { m_i->Release(); m_i = NULL; } }
  protected:
    I *m_i;
  };

};

#endif

#endif
