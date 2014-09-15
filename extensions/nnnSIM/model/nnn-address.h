/* -*- Mode:C++; c-file-style:"gnu" -*- */
/*
 * Copyright 2014 Waseda University, Sato Laboratory
 *   Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
 *
 *  nnn-address.h is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  nnn-address.h is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero Public License for more details.
 *
 *  You should have received a copy of the GNU Affero Public License
 *  along with nnn-address.h.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NNN_ADDRESS_H
#define NNN_ADDRESS_H

#include <iostream>

#include <ns3-dev/ns3/simple-ref-count.h>
#include <ns3-dev/ns3/attribute.h>
#include <ns3-dev/ns3/attribute-helper.h>

#include "nnn-common.h"

NNN_NAMESPACE_BEGIN

/**
 * @brief Class for NNN Address
 */
class NNNAddress : public SimpleRefCount<NNNAddress>
{
public:
  ///////////////////////////////////////////////////////////////////////////////
  //                              CONSTRUCTORS                                 //
  ///////////////////////////////////////////////////////////////////////////////

  /**
   * @brief Default constructor to create an NNN Address
   */
  NNNAddress ();

  /**
   * @brief Copy constructor
   *
   * @param other reference to a NNN Address object
   */
  NNNAddress (const NNNAddress &other);

  /**
   * @brief Create a name from string
   *
   * @param name Dot separated address
   */
  NNNAddress (const std::string &name);

  /**
   * @brief Assignment operator
   */
  NNNAddress &
  operator= (const NNNAddress &other);

  /////
  ///// Static helpers to convert name component to appropriate value
  /////

  /**
   * @brief Get text representation of the name (Dot notation)
   */
  std::string
  toString () const;

  /**
   * @brief Write name in Dot notation to the specified output stream
   * @param os output stream
   */
  void
  toString (std::ostream &os) const;

  /////////////////////////////////////////////////
  // Helpers and compatibility wrappers
  /////////////////////////////////////////////////

  /**
   * @brief Compare two names, using canonical ordering for each component
   * @return 0  They compare equal
   *         <0 If *this comes before other in the canonical ordering
   *         >0 If *this comes after in the canonical ordering
   */
  int
  compare (const NNNAddress &name) const;

  /**
   * @brief Check if 2 NNNAddress objects are equal
   */
  inline bool
  operator == (const NNNAddress &name) const;

  /**
   * @brief Check if 2 NNNAddress objects are not equal
   */
  inline bool
  operator != (const NNNAddress &name) const;

  /**
   * @brief Less or equal comparison of 2 NNNAddress objects
   */
  inline bool
  operator <= (const NNNAddress &name) const;

  /**
   * @brief Less comparison of 2 NNNAddress objects
   */
  inline bool
  operator < (const NNNAddress &name) const;

  /**
   * @brief Great or equal comparison of 2 NNNAddress objects
   */
  inline bool
  operator >= (const NNNAddress &name) const;

  /**
   * @brief Great comparison of 2 NNNAddress objects
   */
  inline bool
  operator > (const NNNAddress &name) const;

  /**
   * @brief Create a new Name object, by copying components from first and second name
   */
  NNNAddress
  operator + (const NNNAddress &name) const;

  bool
  sameSector (const NNNAddress &name) const;

public:
  // Data Members (public):
  ///  Value returned by various member functions when they fail.
  const static size_t npos = static_cast<size_t> (-1);
  const static uint64_t nversion = static_cast<uint64_t> (-1);

private:
  std::vector<char> m_address;
};

inline bool
NNNAddress::operator ==(const NNNAddress &name) const
{
  return (compare (name) == 0);
}

inline bool
NNNAddress::operator !=(const NNNAddress &name) const
{
  return (compare (name) != 0);
}

inline bool
NNNAddress::operator <= (const NNNAddress &name) const
{
  return (compare (name) <= 0);
}

inline bool
NNNAddress::operator < (const NNNAddress &name) const
{
  return (compare (name) < 0);
}

inline bool
NNNAddress::operator >= (const NNNAddress &name) const
{
  return (compare (name) >= 0);
}

inline bool
NNNAddress::operator > (const NNNAddress &name) const
{
  return (compare (name) > 0);
}

inline std::ostream &
operator << (std::ostream &os, const NNNAddress &name)
{
  name.toString (os);
  return os;
}

inline std::istream &
operator >> (std::istream &is, NNNAddress &name)
{
  std::string line;
  is >> line;
  name = NNNAddress (line);

  return is;
}

ATTRIBUTE_HELPER_HEADER (NNNAddress);

NNN_NAMESPACE_END

#endif
