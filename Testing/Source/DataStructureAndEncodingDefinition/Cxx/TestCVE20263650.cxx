/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmExplicitDataElement.h"
#include "gdcmImplicitDataElement.h"
#include "gdcmFragment.h"
#include "gdcmSwapper.h"
#include "gdcmConfigure.h"

#include <cstring>
#include <sstream>
#include <string>

#if defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define GDCM_TEST_SANITIZED
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define GDCM_TEST_SANITIZED
#endif

#if defined(__linux__) && !defined(GDCM_TEST_SANITIZED)
#define GDCM_TEST_LIMIT_ADDRESS_SPACE
#include <cstdio>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace
{
const uint32_t HugeLength = 0xfffffff0;

void WriteTag(std::ostream &os, uint16_t group, uint16_t element)
{
  os.write(reinterpret_cast<const char*>(&group), 2);
  os.write(reinterpret_cast<const char*>(&element), 2);
}

void WriteU32(std::ostream &os, uint32_t v)
{
  os.write(reinterpret_cast<const char*>(&v), 4);
}

void WriteExplicitOB(std::ostream &os, uint32_t vl)
{
  WriteTag(os, 0x0009, 0x1010);
  os.write("OB\0\0", 4);
  WriteU32(os, vl);
}

// Without a cap an allocation of HugeLength can succeed on a large machine
// and leave the old code path indistinguishable from the fixed one.
void LimitAddressSpace()
{
#if defined(GDCM_TEST_LIMIT_ADDRESS_SPACE)
  unsigned long pages = 0;
  if( FILE *f = std::fopen("/proc/self/statm", "r") )
    {
    if( std::fscanf(f, "%lu", &pages) != 1 ) pages = 0;
    std::fclose(f);
    }
  if( pages )
    {
    struct rlimit rl;
    rl.rlim_cur = rl.rlim_max =
      pages * static_cast<rlim_t>(sysconf(_SC_PAGESIZE)) + (rlim_t(1) << 30);
    setrlimit(RLIMIT_AS, &rl);
    }
#endif
}

bool IsAllocationFailure(const gdcm::Exception &ex)
{
  return std::strstr(ex.what(), "Impossible to allocate") != nullptr;
}

int TestExplicitHugeLength()
{
  std::stringstream ss;
  WriteExplicitOB(ss, HugeLength);
  ss.write("0123456789", 10);
  gdcm::ExplicitDataElement de;
  try
    {
    de.Read<gdcm::SwapperNoOp>(ss);
    }
  catch( const gdcm::ParseException & )
    {
    if( !ss.fail() || !ss.eof() )
      {
      std::cerr << "Explicit: stream not left as a short read leaves it" << std::endl;
      return 1;
      }
    return 0;
    }
  catch( const gdcm::Exception &ex )
    {
    std::cerr << "Explicit: " << (IsAllocationFailure(ex) ? "allocated " : "unexpected ")
      << ex.what() << std::endl;
    return 1;
    }
  std::cerr << "Explicit: read succeeded" << std::endl;
  return 1;
}

int TestImplicitHugeLength()
{
  std::stringstream ss;
  WriteTag(ss, 0x0009, 0x1010);
  WriteU32(ss, HugeLength);
  ss.write("0123456789", 10);
  gdcm::ImplicitDataElement de;
  try
    {
    de.Read<gdcm::SwapperNoOp>(ss);
    }
  catch( const gdcm::Exception &ex )
    {
    if( IsAllocationFailure(ex) )
      {
      std::cerr << "Implicit: allocated " << ex.what() << std::endl;
      return 1;
      }
    return 0;
    }
  std::cerr << "Implicit: read succeeded" << std::endl;
  return 1;
}

int TestFragmentHugeLength()
{
  std::stringstream ss;
  WriteTag(ss, 0xfffe, 0xe000);
  WriteU32(ss, HugeLength);
  ss.write("0123456789", 10);
  gdcm::Fragment frag;
  try
    {
    frag.Read<gdcm::SwapperNoOp>(ss);
    }
  catch( const gdcm::ParseException & )
    {
    // Decoders size buffers from the declared fragment lengths.
    if( frag.GetVL() != 0 )
      {
      std::cerr << "Fragment: rejected fragment keeps VL " << frag.GetVL() << std::endl;
      return 1;
      }
    return 0;
    }
  catch( const gdcm::Exception &ex )
    {
    std::cerr << "Fragment: " << ex.what() << std::endl;
    return 1;
    }
  std::cerr << "Fragment: read succeeded" << std::endl;
  return 1;
}

// A length that overruns what remains but fits in the stream keeps the
// historical read-then-fail behavior.
int TestOverrunWithinStream()
{
  const std::string prefix(4 * 1024 * 1024, '\0');
  std::stringstream ss;
  ss.write(prefix.data(), prefix.size());
  WriteExplicitOB(ss, 3 * 1024 * 1024);
  ss.write(prefix.data(), 1024 * 1024);
  ss.seekg(prefix.size());
  gdcm::ExplicitDataElement de;
  try
    {
    de.Read<gdcm::SwapperNoOp>(ss);
    }
  catch( const gdcm::ParseException & )
    {
    return 0;
    }
  std::cerr << "Overrun: expected a ParseException" << std::endl;
  return 1;
}

int TestTruncatedPixelData()
{
#ifdef GDCM_SUPPORT_BROKEN_IMPLEMENTATION
  std::stringstream ss;
  WriteTag(ss, 0x7fe0, 0x0010);
  ss.write("OW\0\0", 4);
  WriteU32(ss, 8 * 1024 * 1024);
  ss.write("0123456789", 10);
  gdcm::ExplicitDataElement de;
  try
    {
    de.Read<gdcm::SwapperNoOp>(ss);
    }
  catch( const gdcm::Exception &ex )
    {
    std::cerr << "Truncated Pixel Data: " << ex.what() << std::endl;
    return 1;
    }
  if( de.GetVL() != 10 )
    {
    std::cerr << "Truncated Pixel Data: VL " << de.GetVL() << ", expected 10" << std::endl;
    return 1;
    }
#endif
  return 0;
}

int TestLargeValueStillReads()
{
  const uint32_t vl = 2 * 1024 * 1024;
  const std::string payload(vl, 'x');
  std::stringstream ss;
  WriteExplicitOB(ss, vl);
  ss.write(payload.data(), payload.size());
  WriteExplicitOB(ss, 2);
  ss.write("ok", 2);
  gdcm::ExplicitDataElement de1, de2;
  try
    {
    de1.Read<gdcm::SwapperNoOp>(ss);
    de2.Read<gdcm::SwapperNoOp>(ss);
    }
  catch( const gdcm::Exception &ex )
    {
    std::cerr << "Large value: " << ex.what() << std::endl;
    return 1;
    }
  const gdcm::ByteValue *bv = de2.GetByteValue();
  if( de1.GetVL() != vl || !bv || std::memcmp(bv->GetPointer(), "ok", 2) != 0 )
    {
    std::cerr << "Large value: stream position not restored" << std::endl;
    return 1;
    }
  return 0;
}
} // end anonymous namespace

int TestCVE20263650(int, char *[])
{
  LimitAddressSpace();
  int ret = 0;
  ret += TestExplicitHugeLength();
  ret += TestImplicitHugeLength();
  ret += TestFragmentHugeLength();
  ret += TestOverrunWithinStream();
  ret += TestTruncatedPixelData();
  ret += TestLargeValueStillReads();
  return ret;
}
