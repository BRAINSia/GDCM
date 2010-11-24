/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#include "gdcmTransferSyntaxSub.h"
#include "gdcmSwapper.h"

namespace gdcm
{
namespace network
{
const uint8_t TransferSyntaxSub::ItemType = 0x40;
const uint8_t TransferSyntaxSub::Reserved2 = 0x00;

TransferSyntaxSub::TransferSyntaxSub()
{
  //UpdateName( "1.2.840.10008.1.1" );
  ItemLength = 0;
}

void TransferSyntaxSub::SetName( const char *name )
{
  Name = name;
  ItemLength = Name.size();
}

std::istream &TransferSyntaxSub::Read(std::istream &is)
{
  uint8_t itemtype = 0xf;
  is.read( (char*)&itemtype, sizeof(ItemType) );
  assert( itemtype == ItemType );
  uint8_t reserved2;
  is.read( (char*)&reserved2, sizeof(Reserved2) );
  uint16_t itemlength;
  is.read( (char*)&itemlength, sizeof(ItemLength) );
  SwapperDoOp::SwapArray(&itemlength,1);
  ItemLength = itemlength;

  char name[256];
  assert( itemlength < 256 );
  is.read( name, itemlength );
  Name = std::string(name,itemlength);

  return is;
}

const std::ostream &TransferSyntaxSub::Write(std::ostream &os) const
{
  os.write( (char*)&ItemType, sizeof(ItemType) );
  os.write( (char*)&Reserved2, sizeof(Reserved2) );
  //os.write( (char*)&ItemLength, sizeof(ItemLength) );
  uint16_t copy = ItemLength;
  SwapperDoOp::SwapArray(&copy,1);
  os.write( (char*)&copy, sizeof(ItemLength) );

  os.write( Name.c_str(), Name.size() );
  return os;
}

size_t TransferSyntaxSub::Size() const
{
  size_t ret = 0;
  assert( Name.size() == ItemLength );
  ret += sizeof(ItemType);
  ret += sizeof(Reserved2);
  ret += sizeof(ItemLength);
  ret += ItemLength;
  return ret;
}

void TransferSyntaxSub::UpdateName( const char *name )
{
  if( name )
    {
    gdcm::UIDs uids;
    bool b = uids.SetFromUID( name );
    if( b )
      {
      Name = name;
      ItemLength = Name.size();
      assert( (size_t)ItemLength + 4 == Size() );
      return;
      }
    }

  gdcmErrorMacro( "Invalid Name: " << name );
  throw "Invalid Name";
}

void TransferSyntaxSub::SetNameFromUID( gdcm::UIDs::TSName tsname )
{
  const char *name = gdcm::UIDs::GetUIDString( tsname );
  UpdateName( name );
}

void TransferSyntaxSub::Print(std::ostream &os) const
{
  os << "Name: " << Name;
  UIDs uids;
  if( uids.SetFromUID( Name.c_str() ) )
    {
    os << " (" << uids.GetName() << ")" << std::endl;
    }
  os << std::endl;
}

} // end namespace network
} // end namespace gdcm
