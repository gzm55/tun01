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
// RefObject.h: interface for the RefObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REFOBJECT_H__6AE34B36_FA14_49CF_956E_6C0F4BE6164C__INCLUDED_)
#define AFX_REFOBJECT_H__6AE34B36_FA14_49CF_956E_6C0F4BE6164C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//namespace com { namespace antlersoft {

class RefObject  
{
public:
	unsigned m__ReferenceCount;
	RefObject()
		: m__ReferenceCount( 0)
	{}
	virtual ~RefObject();
};
//} /* namespace antlersoft */ } /* namespace com */

#endif // !defined(AFX_REFOBJECT_H__6AE34B36_FA14_49CF_956E_6C0F4BE6164C__INCLUDED_)
