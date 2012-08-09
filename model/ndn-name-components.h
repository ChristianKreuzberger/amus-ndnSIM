/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Ilya Moiseenko <iliamo@cs.ucla.edu>
 */

#ifndef _NDN_NAME_COMPONENTS_H_
#define _NDN_NAME_COMPONENTS_H_

#include "ns3/simple-ref-count.h"
#include "ns3/attribute.h"
#include "ns3/attribute-helper.h"

#include <string>
#include <algorithm>
#include <list>
#include "ns3/object.h"

#include <boost/ref.hpp>

namespace ns3 {

/**
 * \ingroup ndn
 * \brief Hierarchical NDN name
 * A Name element represents a hierarchical name for Ndn content. 
 * It simply contains a sequence of Component elements. 
 * Each Component element contains a sequence of zero or more bytes. 
 * There are no restrictions on what byte sequences may be used.
 * The Name element in an Interest is often referred to with the term name prefix or simply prefix.
 */ 
class NdnNameComponents : public SimpleRefCount<NdnNameComponents>
{
public:
  typedef std::list<std::string>::iterator       iterator;            
  typedef std::list<std::string>::const_iterator const_iterator;

  /**
   * \brief Constructor 
   * Creates a prefix with zero components (can be looked as root "/")
   */
  NdnNameComponents ();
  
  /**
   * \brief Constructor
   * Creates a prefix from a list of strings where every string represents a prefix component
   * @param[in] components A list of strings
   */
  NdnNameComponents (const std::list<boost::reference_wrapper<const std::string> > &components);

  /**
   * @brief Constructor
   * Creates a prefix from the string (string is parsed using operator>>)
   * @param[in] prefix A string representation of a prefix
   */
  NdnNameComponents (const std::string &prefix);

  /**
   * @brief Constructor
   * Creates a prefix from the string (string is parsed using operator>>)
   * @param[in] prefix A string representation of a prefix
   */
  NdnNameComponents (const char *prefix);
  
  /**
   * \brief Generic Add method
   * Appends object of type T to the list of components 
   * @param[in] value The object to be appended
   */
  template<class T>
  inline void
  Add (const T &value);
  
  /**
   * \brief Generic constructor operator
   * The object of type T will be appended to the list of components
   */
  template<class T>
  inline NdnNameComponents&
  operator () (const T &value);

  /**
   * \brief Get a name
   * Returns a list of components (strings)
   */
  const std::list<std::string> &
  GetComponents () const;

  /**
   * @brief Helper call to get the last component of the name
   */
  std::string
  GetLastComponent () const;

  /**
   * \brief Get subcomponents of the name, starting with first component
   * @param[in] num Number of components to return. Valid value is in range [1, GetComponents ().size ()]
   */
  std::list<boost::reference_wrapper<const std::string> >
  GetSubComponents (size_t num) const;

  /**
   * @brief Get prefix of the name, containing less  minusComponents right components
   */
  NdnNameComponents
  cut (size_t minusComponents) const;
  
  /**
   * \brief Print name
   * @param[in] os Stream to print 
   */
  void Print (std::ostream &os) const;

  /**
   * \brief Returns the size of NdnNameComponents
   */
  inline size_t
  size () const;

  /**
   * @brief Get read-write begin() iterator
   */
  inline iterator
  begin ();

  /**
   * @brief Get read-only begin() iterator
   */
  inline const_iterator
  begin () const;

  /**
   * @brief Get read-write end() iterator
   */
  inline iterator
  end ();

  /**
   * @brief Get read-only end() iterator
   */
  inline const_iterator
  end () const;

  /**
   * \brief Equality operator for NdnNameComponents
   */
  inline bool
  operator== (const NdnNameComponents &prefix) const;

  /**
   * \brief Less than operator for NdnNameComponents
   */
  inline bool
  operator< (const NdnNameComponents &prefix) const;

  typedef std::string partial_type;
  
private:
  std::list<std::string> m_prefix;                              ///< \brief a list of strings (components)
};

/**
 * \brief Print out name components separated by slashes, e.g., /first/second/third
 */
std::ostream &
operator << (std::ostream &os, const NdnNameComponents &components);

/**
 * \brief Read components from input and add them to components. Will read input stream till eof
 * Substrings separated by slashes will become separate components
 */
std::istream &
operator >> (std::istream &is, NdnNameComponents &components);

/**
 * \brief Returns the size of NdnNameComponents object
 */  
size_t
NdnNameComponents::size () const
{
  return m_prefix.size ();
}

NdnNameComponents::iterator
NdnNameComponents::begin ()
{
  return m_prefix.begin ();
}

/**
 * @brief Get read-only begin() iterator
 */
NdnNameComponents::const_iterator
NdnNameComponents::begin () const
{
  return m_prefix.begin ();
}  

/**
 * @brief Get read-write end() iterator
 */
NdnNameComponents::iterator
NdnNameComponents::end ()
{
  return m_prefix.end ();
}

/**
 * @brief Get read-only end() iterator
 */
NdnNameComponents::const_iterator
NdnNameComponents::end () const
{
  return m_prefix.end ();
}


/**
 * \brief Generic constructor operator
 * The object of type T will be appended to the list of components
 */
template<class T>
NdnNameComponents&
NdnNameComponents::operator () (const T &value)
{
  Add (value);
  return *this;
}

/**
 * \brief Generic Add method
 * Appends object of type T to the list of components 
 * @param[in] value The object to be appended
 */
template<class T>
void
NdnNameComponents::Add (const T &value)
{
  std::ostringstream os;
  os << value;
  m_prefix.push_back (os.str ());
}

/**
 * \brief Equality operator for NdnNameComponents
 */
bool
NdnNameComponents::operator== (const NdnNameComponents &prefix) const
{
  if (m_prefix.size () != prefix.m_prefix.size ())
    return false;
  
  return std::equal (m_prefix.begin (), m_prefix.end (), prefix.m_prefix.begin ());
}

/**
 * \brief Less than operator for NdnNameComponents
 */
bool
NdnNameComponents::operator< (const NdnNameComponents &prefix) const
{
  return std::lexicographical_compare (m_prefix.begin (), m_prefix.end (),
                                       prefix.m_prefix.begin (), prefix.m_prefix.end ());
}

    
ATTRIBUTE_HELPER_HEADER (NdnNameComponents);
} // namespace ns3

#endif // _NDN_NAME_COMPONENTS_H_
