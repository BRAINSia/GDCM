/*=========================================================================

  Program: GDCM (Grass Root DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2008 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmByteSwapFilter.h"
#include "gdcmElement.h"
#include "gdcmByteValue.h"
#include "gdcmSequenceOfFragments.h"
#include "gdcmSequenceOfItems.h"
#include "gdcmSwapper.h"

namespace gdcm
{

//-----------------------------------------------------------------------------
//ByteSwapFilter::ByteSwapFilter()
//{
//}
//-----------------------------------------------------------------------------
ByteSwapFilter::~ByteSwapFilter()
{
}

void ByteSwapFilter::SetDicts(const Dicts &dicts)
{
  abort(); // FIXME
}

bool ByteSwapFilter::ByteSwap()
{
  DataSet::ConstIterator it = DS.Begin();
  for( ; it != DS.End(); ++it)
    {
    const DataElement& de = *it; // FIXME !!!
    VR const & vr = de.GetVR();
    assert( vr & VR::VRASCII || vr & VR::VRBINARY );
    if( de.IsEmpty() )
      {
      }
    else if( const ByteValue *bv = de.GetByteValue() )
      {
      // ASCII do not need byte swap
      if( vr & VR::VRBINARY /*&& de.GetTag().IsPrivate()*/ )
        {
        //assert( de.GetTag().IsPrivate() );
        switch(vr)
          {
        case VR::AT: 
          assert( 0 && "Should not happen" );
          break;
        case VR::FL: 
          // FIXME: Technically FL should not be byte-swapped...
          //std::cerr << "ByteSwap FL:" << de.GetTag() << std::endl;
          //SwapperDoOp::SwapArray((uint32_t*)bv->GetPointer(), bv->GetLength() / sizeof(uint32_t) );
          break;
        case VR::FD: 
          assert( 0 && "Should not happen" );
          break;
        case VR::OB: 
          // I think we are fine, unless this is one of those OB_OW thingy
          break;
        case VR::OF: 
          assert( 0 && "Should not happen" );
          break;
        case VR::OW: 
          assert( 0 && "Should not happen" );
          break;
        case VR::SL: 
          SwapperDoOp::SwapArray((uint32_t*)bv->GetPointer(), bv->GetLength() / sizeof(uint32_t) );
          break;
        case VR::SQ: 
          assert( 0 && "Should not happen" );
          break;
        case VR::SS: 
          SwapperDoOp::SwapArray((uint16_t*)bv->GetPointer(), bv->GetLength() / sizeof(uint16_t) );
          break;
        case VR::UL: 
          SwapperDoOp::SwapArray((uint32_t*)bv->GetPointer(), bv->GetLength() / sizeof(uint32_t) );
          break;
        case VR::UN: 
          assert( 0 && "Should not happen" );
          break;
        case VR::US: 
          SwapperDoOp::SwapArray((uint16_t*)bv->GetPointer(), bv->GetLength() / sizeof(uint16_t) );
          break;
        case VR::UT:
          assert( 0 && "Should not happen" );
          break;
        default:
          assert( 0 && "Should not happen" );
          }
        }
      }
    else if( const SequenceOfItems *si = de.GetSequenceOfItems() )
      {
      //if( de.GetTag().IsPrivate() )
        {
        //std::cerr << "ByteSwap SQ:" << de.GetTag() << std::endl;
        SequenceOfItems::ConstIterator it = si->Begin();
        for( ; it != si->End(); ++it)
          {
          const Item &item = *it;
          DataSet &ds = const_cast<DataSet&>(item.GetNestedDataSet()); // FIXME
          ByteSwapFilter bsf(ds);
          bsf.ByteSwap();
          }
        }
      }
    else if( const SequenceOfFragments *sf = de.GetSequenceOfFragments() )
      {
          assert( 0 && "Should not happen" );
      }
    else
      {
      assert( 0 && "error" );
      }

    }
  return true;
}

}
