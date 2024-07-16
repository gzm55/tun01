#if ! defined( RefPtr_h )
#define RefPtr_h
/*
 * <p>Copyright (c) 2001-2004  Michael A. MacDonald<p>
 * ----- - - -- - - --
 * <p>
 *     This package is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 * <p>
 *     This package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * <p>
 *     You should have received a copy of the GNU General Public License
 *     along with the package (see gpl.txt); if not, see www.gnu.org
 * <p>
 * ----- - - -- - - --
 */

#if _MSC_VER > 1000
#pragma once
#pragma warning ( disable: 4786 4290 )
#endif // _MSC_VER > 1000

#include "RefObject.h"
#include <typeinfo>

//class bad_cast;

//namespace com { namespace antlersoft {

class RefPtrException : public std::exception
{
	const char* m_what;
public :
	RefPtrException( const char* msg)
	: m_what( msg) {}
	virtual const char* what() { return m_what; }
};

template<class C> class RefPtr
{
	// Attributes
	public :
		RefObject* holder;

	public :
		RefPtr() throw()
			: holder( static_cast<RefObject*>(0))
		{
		}

		RefPtr( C* newObject) throw()
			: holder( newObject )
		{
			holder->m__ReferenceCount++;
		}

		template<class D> RefPtr( const RefPtr<D>& toCopy) throw(std::bad_cast)
			: holder( toCopy.holder)
		{
			if ( holder)
			{
				dynamic_cast<C&>( *toCopy.holder);
				holder->m__ReferenceCount++;
			}
		}

		// Specialization of above
		RefPtr( const RefPtr<C>& toCopy) throw()
			: holder( toCopy.holder)
		{
			if ( holder)
				holder->m__ReferenceCount++;
		}

		~RefPtr() throw()
		{
			if ( holder)
			{
				if ( ( --holder->m__ReferenceCount)==0)
				{
					delete holder;
					holder=static_cast<RefObject*>(0);
				}
			}
		}

		bool isNull() const throw()
		{
			return holder==static_cast<RefObject*>(0);
		}

		C* operator->() const
		{
			if ( isNull())
				throw RefPtrException( "Dereferencing null pointer");
			return static_cast<C*>( holder);
		}

		template<class D> RefPtr& operator=( const RefPtr<D>& toAssign) throw( std::bad_cast)
		{
			if ( toAssign.holder)
			{
				dynamic_cast<C&>( *(toAssign.holder));

				toAssign.holder->m__ReferenceCount++;
			}
			if ( holder)
			{
				if ( ( --holder->m__ReferenceCount)==0)
				{
					delete holder;
				}
			}
			holder=toAssign.holder;

			return *this;
		}

		// Specialization of above
		RefPtr& operator=( const RefPtr<C>& toAssign) throw()
		{
			if ( toAssign.holder)
				toAssign.holder->m__ReferenceCount++;
			if ( holder)
			{
				if ( ( --holder->m__ReferenceCount)==0)
				{
					delete holder;
				}
			}
			holder=toAssign.holder;

			return *this;
		}

		template<class T> T& operator->*( T C::*pm)
		{
			if ( isNull())
				throw RefPtrException( "Dereferencing null pointer");
			return (static_cast<C*>(holder)->*pm);
		}

		friend class ExtractorHelper;
};

//} /* namespace antlersoft */ } /* namespace com */

#endif
