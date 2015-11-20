/*ckwg +29
 * Copyright 2014 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * \brief Implementation of file IO functions for a \ref kwiver::vital::landmark_map
 *
 * Uses the PLY file format
 */

#include "landmark_map_io.h"

#include <vital/exceptions.h>
#include <vital/vital_foreach.h>
#include <kwiversys/SystemTools.hxx>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

namespace kwiver {
namespace vital {

/// Output the given \c landmark_map object to the specified PLY file path
void
write_ply_file( landmark_map_sptr const&  landmarks,
                path_t const&             file_path )
{
  // If the landmark map is empty, throw
  if ( ! landmarks || ( landmarks->size() == 0 ) )
  {
    throw file_write_exception( file_path,
         "No landmarks in the given landmark map!" );
  }

  // If the given path is a directory, we obviously can't write to it.
  if ( kwiversys::SystemTools::FileIsDirectory( file_path ) )
  {
    throw file_write_exception( file_path,
         "Path given is a directory, can not write file." );
  }

  // Check that the directory of the given filepath exists, creating necessary
  // directories where needed.
  std::string parent_dir =  kwiversys::SystemTools::GetFilenamePath(
    kwiversys::SystemTools::CollapseFullPath( file_path ) );
  if ( ! kwiversys::SystemTools::FileIsDirectory( parent_dir ) )
  {
    if ( ! kwiversys::SystemTools::MakeDirectory( parent_dir ) )
    {
      throw file_write_exception( parent_dir,
            "Attempted directory creation, but no directory created! No idea what happened here..." );
    }
  }


  // open output file and write the tracks
  std::ofstream ofile( file_path.c_str() );
  // write the PLY header
  ofile << "ply\n"
           "format ascii 1.0\n"
           "comment written by VITAL\n"
           "element vertex " << landmarks->size() << "\n"
                                                     "property float x\n"
                                                     "property float y\n"
                                                     "property float z\n"
                                                     "property uchar red\n"
                                                     "property uchar green\n"
                                                     "property uchar blue\n"
                                                     "property uint track_id\n"
                                                     "end_header\n";

  landmark_map::map_landmark_t lm_map = landmarks->landmarks();
  typedef  landmark_map::map_landmark_t::value_type lm_map_val_t;
  VITAL_FOREACH( lm_map_val_t const& p, lm_map )
  {
    vector_3d loc = p.second->loc();
    rgb_color color = p.second->color();

    // the '+' prefix on the color values causes them to be printed
    // as decimal numbers instead of ASCII characters
    ofile << loc.x() << " " << loc.y() << " " << loc.z()
          << " " << +color.r << " " << +color.g << " " << +color.b
          << " " << p.first << "\n";
  }
  ofile.close();
} // write_ply_file


namespace {

/// Split a string into tokens delimited by whitespace
std::vector<std::string>
get_tokens(std::string const& line)
{
  std::istringstream iss( line );
  std::vector<std::string> tokens((std::istream_iterator<std::string>(iss)),
                                  std::istream_iterator<std::string>());
  return tokens;
}

} // end anonymous namespace


/// Load a given \c landmark_map object from the specified PLY file path
landmark_map_sptr
read_ply_file( path_t const& file_path )
{
  if ( ! kwiversys::SystemTools::FileExists( file_path ) )
  {
    throw file_not_found_exception( file_path, "Cannot find file." );
  }

  landmark_map::map_landmark_t landmarks;

  // open input file and read the tracks
  std::ifstream ifile( file_path.c_str() );

  if ( ! ifile )
  {
    throw file_not_read_exception( file_path, "Cannot read file." );
  }

  // enumeration of the vertex properties we can handle
  enum vertex_property_t {INVALID, VX, VY, VZ, CR, CG, CB, INDEX};
  // mapping between PLY vertex property names and our enum
  std::map<std::string, vertex_property_t> prop_map;
  prop_map["x"] = VX;
  prop_map["y"] = VY;
  prop_map["z"] = VZ;
  prop_map["red"] = CR;
  prop_map["diffuse_red"] = CR;
  prop_map["green"] = CG;
  prop_map["diffuse_green"] = CG;
  prop_map["blue"] = CB;
  prop_map["diffuse_blue"] = CB;
  prop_map["track_id"] = INDEX;

  bool parsed_header = false;
  bool parsing_vertex_props = false;
  std::vector<vertex_property_t> vert_props;
  std::string line;

  landmark_id_t id = 0;
  while ( std::getline( ifile, line ) )
  {
    std::vector<std::string> tokens = get_tokens(line);
    if ( line.empty() || tokens.empty() )
    {
      continue;
    }
    if ( ! parsed_header )
    {
      if ( line == "end_header" )
      {
        parsed_header = true;
        // TODO check that provided properties are meaningful
        // (e.g. has X, Y, and Z; has R, G, and B or no color, etc.)
        continue;
      }

      if ( tokens.size() == 3 &&
           tokens[0] == "element" &&
           tokens[1] == "vertex" )
      {
        parsing_vertex_props = true;
      }
      else if ( tokens[0] == "element" )
      {
        parsing_vertex_props = false;
      }

      if ( parsing_vertex_props )
      {
        if ( tokens.size() == 3 && tokens[0] == "property" )
        {
          // map property names into enum values if supported
          std::string name = tokens[2];
          std::transform(name.begin(), name.end(), name.begin(), ::tolower);
          vertex_property_t prop = INVALID;
          const auto p = prop_map.find(name);
          if ( p != prop_map.end() )
          {
            prop = p->second;
          }
          vert_props.push_back(prop);
        }
      }

      continue;
    }

    // TODO throw exceptions if tokens.size() != vert_props.size()
    // or if the values do not parse as expected
    double x, y, z;
    rgb_color color;
    ++id;
    for( unsigned int i=0; i<tokens.size() && i < vert_props.size(); ++i )
    {
      std::istringstream iss(tokens[i]);
      switch( vert_props[i] )
      {
        case VX:
          iss >> x;
          break;
        case VY:
          iss >> y;
          break;
        case VZ:
          iss >> z;
          break;
        case CR:
          iss >> color.r;
          break;
        case CG:
          iss >> color.g;
          break;
        case CB:
          iss >> color.b;
          break;
        case INDEX:
          iss >> id;
          break;
        default:
          break;
      }
    }

    landmark_d* lm = new landmark_d( vector_3d( x, y, z ) );
    lm->set_color( color );
    landmarks[id] = landmark_sptr( lm );
  }

  ifile.close();

  return landmark_map_sptr( new simple_landmark_map( landmarks ) );
} // read_ply_file

} } // end namespace
