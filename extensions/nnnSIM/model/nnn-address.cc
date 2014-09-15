/* -*- Mode:C++; c-file-style:"gnu" -*- */
/*
 * Copyright 2014 Waseda University, Sato Laboratory
 *   Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
 *
 *  nnn-address.cc is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  nnn-address.cc is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero Public License for more details.
 *
 *  You should have received a copy of the GNU Affero Public License
 *  along with nnn-address.cc.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "nnn-address.h"

#include <boost/algorithm/string.hpp>

#include <ctype.h>

using namespace std;

NNN_NAMESPACE_BEGIN

ATTRIBUTE_HELPER_CPP (NNNAddress);

///////////////////////////////////////////////////////////////////////////////
//                              CONSTRUCTORS                                 //
///////////////////////////////////////////////////////////////////////////////

NNNAddress::NNNAddress ()
{
}

NNNAddress::NNNAddress (const string &name)
{

}

NNNAddress::NNNAddress (const NNNAddress &other)
{

}

NNNAddress &
NNNAddress::operator= (const NNNAddress &other)
{
  return *this;
}

std::string
NNNAddress::toString () const
{
  ostringstream os;
  toString (os);
  return os.str ();
}

NNNAddress
NNNAddress::operator+ (const NNNAddress &name) const
{
  NNNAddress newName;
  return newName;
}

void
NNNAddress::toString (std::ostream &os) const
{
    os << ".";
    os << ".";
}

int
NNNAddress::compare (const NNNAddress &NNNAddress) const
{

}

NNN_NAMESPACE_END
